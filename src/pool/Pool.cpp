/************************************************************************************
   Copyright (C) 2020,2021 MariaDB Corporation AB

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this library; if not see <http://www.gnu.org/licenses>
   or write to the Free Software Foundation, Inc.,
   51 Franklin St., Fifth Floor, Boston, MA 02110, USA
*************************************************************************************/


#include <iostream>
#include <sstream>

#include "ExceptionFactory.h"
#include "Pools.h"
#include "Pool.h"
#include "logger/LoggerFactory.h"
#include "ExceptionFactory.h"
#include "ThreadPoolExecutor.h"
#include "MariaDbThreadFactory.h"
#include "util/Utils.h"
#include "ConnectionEventListener.h"

// Otherwise operations with time tend to be few screens wide
using namespace std::chrono;

namespace sql
{
namespace mariadb
{
  Logger* Pool::logger= LoggerFactory::getLogger(typeid(Pool));

  /**
    * Create pool from configuration.
    *
    * @param urlParser configuration parser
    * @param poolIndex pool index to permit distinction of thread name
    * @param poolExecutor pools common executor
    */
  Pool::Pool(Shared::UrlParser &_urlParser, int32_t poolIndex, ScheduledThreadPoolExecutor& _poolExecutor) :
    urlParser(_urlParser),
    options(urlParser->getOptions()),
    poolState(POOL_STATE_OK),
    //maxIdleTime(options->maxIdleTime),
    connectionAppenderQueue(urlParser->getOptions()->maxPoolSize),
    poolTag(generatePoolTag(poolIndex)),
    connectionAppender(
        1,
        1,
        std::chrono::seconds(10),
        connectionAppenderQueue,
        new MariaDbThreadFactory(poolTag +"-appender")),
    poolExecutor(_poolExecutor),
    pendingRequestNumber(0),
    totalConnection(0),
    scheduledFuture(nullptr)
  {
    connectionAppender.allowCoreThreadTimeOut(true);

    auto cit= options->nonMappedOptions.find("testMinRemovalDelay");
    int32_t minDelay= 30;
    if (cit != options->nonMappedOptions.end()) {
      minDelay= std::stoi(cit->second.c_str());
    }

    try
    {
      addConnection();
      // Doing heave thing after first connection - so if it fails and throws, we throw further with light heart
      connectionAppender.prestartCoreThread();
      int32_t scheduleDelay= std::min(minDelay, options->maxIdleTime / 2);
      scheduledFuture.reset(
        poolExecutor.scheduleAtFixedRate(
          std::bind(&Pool::removeIdleTimeoutConnection, this), std::chrono::seconds(scheduleDelay),
          std::chrono::seconds(scheduleDelay)));
      for (int32_t i= 1; i < options->minPoolSize; ++i) {
        addConnectionRequest();
      }
      if (!idleConnections.empty()) {
        Unique::Statement stmt(idleConnections.front()->getConnection()->createStatement());
        Unique::ResultSet rs(stmt->executeQuery("SELECT @@wait_timeout"));
        if (rs->next()) {
          waitTimeout= rs->getUInt(1);
        }
      }
    }
    catch (sql::SQLException& sqle) {
      logger->error("Error initializing pool connection", sqle);
      // Why to continue if things already went wrong?
      throw sqle;
    }
  }


  Pool::~Pool()
  {
    GET_LOGGER()->trace("Pool", "Pool::~Pool");
    scheduledFuture->cancel(true);
    connectionAppender.shutdown();
    /* Normally that is done while pool is close()-ed. TODO: lock? */
    for (auto& item : idleConnections)
    {
      delete item;
    }
  }

  /**
    * Add new connection if needed. Only one thread create new connection, so new connection request
    * will wait to newly created connection or for a released connection.
    */
  void Pool::addConnectionRequest()
  {
    if (totalConnection.load(std::memory_order_relaxed) < options->maxPoolSize &&
        poolState.load(std::memory_order_relaxed) == POOL_STATE_OK)
    {
      connectionAppender.prestartCoreThread();
      connectionAppenderQueue.emplace_back(
        [&]()->void{
        logger->trace("Pool","Doing adding task");
        if ((totalConnection.load() < options->minPoolSize || pendingRequestNumber.load() > 0)
          && totalConnection.load() < options->maxPoolSize) {
          try {
            addConnection();
          }
          catch (sql::SQLException&)
          {
          }
        }
        logger->trace("Pool","Done adding task");
      });
    }
  }

  /**
    * Removing idle connection. Close them and recreate connection to reach minimal number of
    * connection.
    */
  void Pool::removeIdleTimeoutConnection()
  {
    logger->trace("Pool: Checking idles");
    std::lock_guard<std::mutex> synchronized(idleConnections.getLock());

    Idles::iterator iterator= idleConnections.begin();
    MariaDbInnerPoolConnection* item;

    while (iterator != idleConnections.end())
    {
      item= *iterator;
      auto now= steady_clock::now();
      //urlParser->getOptions()->maxIdleTime << "Now:" << now.time_since_epoch().count() << " Last used:" << item->getLastUsed().time_since_epoch().count() << " Diff:" << std::chrono::duration_cast<std::chrono::nanoseconds>(now - item->getLastUsed()).count() << "/" << std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::seconds(urlParser->getOptions()->maxIdleTime)).count();
      auto idleNanos= duration_cast<nanoseconds>(now.time_since_epoch()).count() - item->getLastUsed();
      bool timedOut= (idleNanos > duration_cast<nanoseconds>(seconds(urlParser->getOptions()->maxIdleTime)).count());
      bool shouldBeReleased= false;
      MariaDbConnection* con= dynamic_cast<MariaDbConnection*>(item->getConnection());

      if (waitTimeout > 0) {

        // idle time is reaching server @@wait_timeout
        if (idleNanos >
          duration_cast<nanoseconds>(seconds(waitTimeout > 45U ? waitTimeout - 45U : waitTimeout)).count()) {
          shouldBeReleased= true;
        }

        //  idle has reach option maxIdleTime value and pool has more connections than minPoolSiz
        if (timedOut && totalConnection.load() > options->minPoolSize) {
          shouldBeReleased= true;
        }

      }
      else if (timedOut) {
        shouldBeReleased= true;
      }

      if (shouldBeReleased) {
        --totalConnection;
        silentCloseConnection(*con);
        delete item;
        iterator= idleConnections.erase(iterator);
        
        addConnectionRequest();
        if (logger->isDebugEnabled()) {
          std::ostringstream s(poolTag);
          s << " connection removed due to inactivity (total:" << totalConnection.load(std::memory_order_relaxed) <<
            ", active:" << getActiveConnections() << ", pending:" << pendingRequestNumber.load(std::memory_order_relaxed) << ")";
          logger->debug(s.str());
        }
      }
      else {
        ++iterator;
      }
    }
    GET_LOGGER()->trace("Pool: Done checking idles");
  }

  /**
    * Create new connection.
    *
    * @throws SQLException if connection creation failed
    */
  void Pool::addConnection() {

    Shared::Protocol protocol= Utils::retrieveProxy(urlParser, nullptr/*&globalInfo*/);
    MariaDbConnection* connection= new MariaDbConnection(protocol);
    MariaDbInnerPoolConnection* item(new MariaDbInnerPoolConnection(connection));

    item->addConnectionEventListener(new MariaDbConnectionEventListener(std::bind(&Pool::connectionClosed, this, std::placeholders::_1),
      std::bind(&Pool::connectionErrorOccurred, this, std::placeholders::_1)));
    /*if (options->staticGlobal) {

      if (globalInfo) {
        initializePoolGlobalState(*connection);
      }

      connection->setDefaultTransactionIsolation(globalInfo.getDefaultTransactionIsolation());
    }
    else {

      connection->setDefaultTransactionIsolation(connection->getTransactionIsolation());
    }*/

    if (poolState.load() == POOL_STATE_OK
      && (++totalConnection) <= options->maxPoolSize) {
      idleConnections.push(item);

      if (logger->isDebugEnabled()) {
        std::ostringstream s(poolTag);
        s << " new physical connection created (total:" << totalConnection.load(std::memory_order_relaxed) <<
          ", active:" << getActiveConnections() << ", pending:" << pendingRequestNumber.load(std::memory_order_relaxed) << ")";
        logger->debug(s.str());
      }
      return;
    }

    silentCloseConnection(*connection);
    // silentCloseConnection resets item's pointer to real connection. TODO: smells bad - this can be done better
    delete connection;
    delete item;
  }

  MariaDbInnerPoolConnection* Pool::getIdleConnection() {
    return getIdleConnection(::mariadb::Timer::Duration(0));
  }

  /**
    * Get an existing idle connection in pool.
    *
    * @return an IDLE connection.
    */
  MariaDbInnerPoolConnection* Pool::getIdleConnection(const ::mariadb::Timer::Clock::duration& timeout) {

    while (true) {
      auto item= 
        (timeout == ::mariadb::Timer::Duration(0))
        ? idleConnections.pollFirst()
        : idleConnections.pollFirst(timeout);

      if (item) {
        MariaDbConnection* connection= dynamic_cast<MariaDbConnection*>(item->getConnection());
        try {
          if (duration_cast<milliseconds>(nanoseconds(duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count() -
            item->getLastUsed())).count() > urlParser->getOptions()->poolValidMinDelay) {
            // validate connection
            if (connection->isValid(10)) { // 10 seconds timeout
              // It's probably not quite right to operate here with MariaDbConnection
              connection->markClosed(false);
              item->lastUsedToNow();
              GET_LOGGER()->trace("Pool Connection Closed:", connection->isClosed(), "getting idle 2", std::hex, item, "Protocol:", connection->getProtocol().get(),
                "expClosed:", connection->getProtocol()->isExplicitClosed());
              return item;
            }
          }
          else {
            // connection has been retrieved recently -> skip connection validation
            connection->markClosed(false);
            item->lastUsedToNow();
            GET_LOGGER()->trace("Pool", connection->isClosed(), "getting idle 2", std::hex, item, "Protocol:", connection->getProtocol().get(),
              "expClosed:", connection->getProtocol()->isExplicitClosed());
            return item;
          }
        }
        catch (SQLException&) {
        }

        --totalConnection;
        // validation failed
        silentAbortConnection(*item);
        // Destructor will take care of underlying connetion
        delete item;
        addConnectionRequest();
        if (logger->isDebugEnabled()) {
          std::ostringstream s(poolTag);
          s << "  connection removed from pool due to failed validation (total:" << totalConnection.load(std::memory_order_relaxed) <<
            ", active:" << getActiveConnections() << ", pending:" << pendingRequestNumber.load(std::memory_order_relaxed) << ")";
          logger->debug(s.str());
        }
        continue;
      }

      return nullptr;
    }
  }

  void Pool::silentCloseConnection(MariaDbConnection& con) {
    con.setPoolConnection(nullptr);
    try {
      con.close();
    }
    catch (SQLException&) {
    }
  }

  void Pool::silentAbortConnection(MariaDbInnerPoolConnection& item) {
    try {
      item.abort(&poolExecutor);
    }
    catch (SQLException&) {

    }
  }


  /*MariaDbInnerPoolConnection& Pool::createPoolConnection(MariaDbConnection connection) {
    MariaDbInnerPoolConnection *pooledConnection= new MariaDbInnerPoolConnection(connection);
    pooledConnection.addConnectionEventListener(
      new ConnectionEventListener(){

      void Pool::connectionClosed(ConnectionEvent event) {
      MariaDbInnerPoolConnection item =(MariaDbInnerPoolConnection)event.getSource();
      if (poolState.get()==POOL_STATE_OK) {
      try {
      if (!(idleConnections.find_first_of(item) >= 0)) {
      item.getConnection().reset();
      idleConnections.addFirst(item);
      }
      }
      catch (SQLException& sqle) {


      totalConnection.decrementAndGet();
      silentCloseConnection(item);
      logger->debug("connection removed from pool {} due to error during reset",poolTag);
      }
      }
      else {

    try {
    item.close();
    }
    catch (SQLException& sqle) {

    }
    totalConnection.decrementAndGet();
    }
    }

    void Pool::connectionClosed(ConnectionEvent event) {

    MariaDbInnerPoolConnection item =((MariaDbInnerPoolConnection)event.getSource());
    if (idleConnections.remove(item)) {
    totalConnection.decrementAndGet();
    }
    silentCloseConnection(item);
    addConnectionRequest();
    logger->debug(
    "connection {} removed from pool {} due to having throw a Connection* exception (total:{}, active:{}, pending:{})",
    item.getConnection().getServerThreadId(),
    poolTag,
    totalConnection.get(),
    getActiveConnections(),
    pendingRequestNumber.get());
    }
      });
    return pooledConnection;
  }*/

  /**
    * Retrieve new connection. If possible return idle connection, if not, stack connection query,
    * ask for a connection creation, and loop until a connection become idle / a new connection is
    * created.
    *
    * @return a connection object
    * @throws SQLException if no connection is created when reaching timeout (connectTimeout option)
    */
  MariaDbInnerPoolConnection* Pool::getPoolConnection()
  {
    ++pendingRequestNumber;

    MariaDbInnerPoolConnection *pooledConnection;

    /*try*/ {
      // try to get Idle connection if any (with a very small timeout)
      if ((pooledConnection=
        getIdleConnection(::mariadb::Timer::Clock::duration(std::chrono::microseconds(totalConnection.load() > 4 ? 0 : 50))))) {
        --pendingRequestNumber;
        return pooledConnection;
      }

      // ask for new connection creation if max is not reached
      addConnectionRequest();

      // try to create new connection
      if ((pooledConnection=
        getIdleConnection(::mariadb::Timer::Clock::duration(std::chrono::milliseconds(urlParser->getOptions()->connectTimeout))))) {
        --pendingRequestNumber;
        return pooledConnection;
      }
      --pendingRequestNumber;
      if (logger->isDebugEnabled()) {
        std::ostringstream s(poolTag);
        s << "Connection could not been got (total:" << totalConnection.load(std::memory_order_relaxed) <<
          ", active:" << getActiveConnections() << ", pending:" << pendingRequestNumber.load(std::memory_order_relaxed) << ")";
        logger->debug(s.str());
      }
      throw SQLException(
        "No connection available within the specified time of connectTimeout("
        + std::to_string(urlParser->getOptions()->connectTimeout)
        + " ms)");

    }
    /* TODO: Needs to catch different exception class - otherwise it cathces also "No connection available" */
    //catch (/*InterruptedException*/std::exception& interrupted)
    //{
    //  --pendingRequestNumber;
    //  throw SQLException("Thread was interrupted", "70100", 0, &interrupted);
    //}
  }

  /**
    * Get new connection from pool if user and password correspond to pool. If username and password
    * are different from pool, will return a dedicated connection.
    *
    * @param username username
    * @param password password
    * @return connection
    * @throws SQLException if any error occur during connection
    */
  MariaDbInnerPoolConnection* Pool::getPoolConnection(const SQLString& username, const SQLString& password) {

    if ((urlParser->getUsername().compare(username) == 0)
      && (urlParser->getUsername().compare(username) == 0)) {
      return getPoolConnection();
    }

    Shared::UrlParser tmpUrlParser(urlParser->clone());
    tmpUrlParser->setUsername(username);
    tmpUrlParser->setPassword(password);
    auto protocol= Utils::retrieveProxy(tmpUrlParser, nullptr);
    // TODO check for leaks
    return new MariaDbInnerPoolConnection(new MariaDbConnection(protocol));
  }

  std::string Pool::generatePoolTag(int32_t poolIndex)
  {
    if (options->poolName.empty())
    {
      options->poolName= "MariaDB-pool";
    }
    return StringImp::get(options->poolName) + "-" + std::to_string(poolIndex);
  }

  const UrlParser& Pool::getUrlParser()
  {
    return *urlParser;
  }

  /**
    * Close pool and underlying connections.
    *
    * @throws InterruptedException if interrupted
    */
  void Pool::close()
  {
    logger->trace("Pool::close");
    // If here must be a lock - then some other lock
    //std::unique_lock<std::mutex> lock(listsLock);
    poolState.store(POOL_STATE_CLOSING);
    pendingRequestNumber.store(0);

    scheduledFuture->cancel(true);
    connectionAppender.shutdown();

    try
    {
      //connectionAppender.awaitTermination(std::chrono::seconds(10));
    }
    catch (std::exception &)
    {
      //closeAll(idleConnections);
    }
    if (logger->isInfoEnabled()) {
      std::ostringstream s(poolTag);
      s << " closing pool (total:" << totalConnection.load(std::memory_order_relaxed) <<
        ", active:" << getActiveConnections() << ", pending:" << pendingRequestNumber.load(std::memory_order_relaxed) << ")";
      logger->info(s.str());
    }

    // TODO not sure if that is really needed
    /*ThreadPoolExecutor connectionRemover(
        totalConnection.load(),
        urlParser->getOptions()->maxPoolSize,
        10,
        TimeUnit::SECONDS,
        blocking_deque<Runnable>(urlParser->getOptions()->maxPoolSize),
        new MariaDbThreadFactory(poolTag + "-destroyer"));*/


    auto start = std::chrono::high_resolution_clock::now();
    do {
      closeAll(idleConnections);
      if (totalConnection.load() > 0) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
      }
    } while (totalConnection.load() > 0
      && std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - start).count()
          < 10);

    if (totalConnection.load() > 0 || idleConnections.empty()) {
      closeAll(idleConnections);
    }

    Pools::remove(*this);
    /*connectionRemover.shutdown();
    connectionRemover.awaitTermination(10, TimeUnit::SECONDS);*/
  }

  void Pool::closeAll(Idles &collection)
  {
    std::lock_guard<std::mutex> synchronized(collection.getLock());

    for (auto item= collection.begin(); item != collection.end();) {
      --totalConnection;
      silentAbortConnection(**item);
      auto poolConn= (*item);
      /*auto conn= poolConn->getConnection();
      delete conn;*/
      delete poolConn;
      item= collection.erase(item);
    }
  }

  /*void Pool::initializePoolGlobalState(MariaDbConnection connection) {
    try {
      Statement * stmt = connection.createStatement())
      SQLString sql(
        "SELECT @@max_allowed_packet,"
        "@@wait_timeout,"
        "@@autocommit,"
        "@@auto_increment_increment,"
        "@@time_zone,"
        "@@system_time_zone,"
        "@@tx_isolation");
      if (!connection.isServerMariaDb()) {
        int32_t major= connection->getMetaData().getDatabaseMajorVersion();
        if ((major >=8 && connection.versionGreaterOrEqual(8, 0, 3))
          ||(major <8 && connection.versionGreaterOrEqual(5, 7, 20))) {
          sql=
            "SELECT @@max_allowed_packet,"
            "@@wait_timeout,"
            "@@autocommit,"
            "@@auto_increment_increment,"
            "@@time_zone,"
            "@@system_time_zone,"
            "@@transaction_isolation";
        }
      }

      try (ResultSet* rs= stmt.executeQuery(sql)) {

        rs.next();

        int32_t transactionIsolation= Utils.transactionFromString(rs.getString(7));

        globalInfo =
          new GlobalStateInfo(
            rs.getLong(1),
            rs.getInt(2),
            rs.getBoolean(3),
            rs.getInt(4),
            rs.getString(5),
            rs.getString(6),
            transactionIsolation);



        maxIdleTime= std::min(options->maxIdleTime, globalInfo.getWaitTimeout()-45);


      }
    }*/

    std::string Pool::getPoolTag() {
      return poolTag;
    }

    /*bool Pool::equals(sql::Object* obj) {
      if (this ==obj) {
        return true;
      }
      if (obj ||getClass()!=obj.getClass()) {
        return false;
      }

      Pool pool =(Pool)obj;

      return (poolTag.compare(pool.poolTag) == 0);
    }*/

    /*int64_t Pool::hashCode() {
      return poolTag.hashCode();
    }*/


    int64_t Pool::getActiveConnections() {
      return totalConnection.load(std::memory_order_relaxed) - idleConnections.size();
    }

    int64_t Pool::getTotalConnections() {
      return totalConnection.load(std::memory_order_relaxed);
    }

    int64_t Pool::getIdleConnections() {
      //Lock? 
      return idleConnections.size();
    }

    int64_t Pool::getConnectionRequests() {
      return pendingRequestNumber.load(std::memory_order_relaxed);
    }


    /**
      * For testing purpose only.
      *
      * @return current thread id's
      */
    std::vector<int64_t> Pool::testGetConnectionIdleThreadIds() {
      std::vector<int64_t> threadIds(idleConnections.size());
      for (auto it= idleConnections.begin(); it != idleConnections.end(); ++it) {
        threadIds.push_back((dynamic_cast<MariaDbConnection*>((*it)->getConnection()))->getServerThreadId());
      }
      return threadIds;
    }


    void Pool::connectionClosed(ConnectionEvent& event)
    {
      MariaDbInnerPoolConnection &item= dynamic_cast<MariaDbInnerPoolConnection&>(event.getSource());
      MariaDbConnection& conn = *dynamic_cast<MariaDbConnection*>(item.getConnection());

      if (poolState.load() == POOL_STATE_OK) {
        try {
          bool contains= false;
          {
            std::lock_guard<std::mutex> synchronized(idleConnections.getLock());
            
            for (Idles::iterator it= idleConnections.begin(); it != idleConnections.end(); ++it) {
              if (*it == &item) {
                contains= true;
                break;
              }
            }
          } // need to release the lock as push_back will obtain it, and it is not re-entrant
          if (!contains) {
            MariaDbConnection& newConn= *item.makeFreshConnectionObj();
            newConn.setPoolConnection(nullptr);
            newConn.reset();
            newConn.setPoolConnection(&item);
            idleConnections.push_back(&item);
          }
        }
        catch (SQLException & /*sqle*/) {
          // sql exception during reset, removing connection from pool
          --totalConnection;
          silentCloseConnection(conn);
          std::ostringstream msg("connection ", std::ios_base::ate);
          msg << conn.getServerThreadId() << " removed from pool " << poolTag << "due to error during reset (total:" ;
          msg << totalConnection.load() << ", active:" << getActiveConnections() << ", pending:" << pendingRequestNumber.load() << ")";
          logger->debug(msg.str());
        }
      }
      else {
        // pool is closed, should then not be rendered to pool, but closed.
        try {
          conn.setPoolConnection(nullptr);
          conn.close();
        }
        catch (SQLException & /*sqle*/) {
          // eat
        }
        --totalConnection;
      }
    }


    void Pool::connectionErrorOccurred(ConnectionEvent& event)
    {
      MariaDbInnerPoolConnection& item= dynamic_cast<MariaDbInnerPoolConnection&>(event.getSource());
      MariaDbConnection& conn= *dynamic_cast<MariaDbConnection*>(item.getConnection());

      --totalConnection;
      {
        std::lock_guard<std::mutex> synchronized(idleConnections.getLock());
        for (auto it= idleConnections.begin(); it != idleConnections.end(); std::advance(it, 1)) {
          if (*it == &item) {
            idleConnections.erase(it);
            break;
          }
        }
        // Erase (in the middle) invalidates all iterators, thus need new
        for (auto const& it : idleConnections) {
          it->ensureValidation();
        }
      } // lock can be freed by now
      silentCloseConnection(conn);
      addConnectionRequest();
      std::ostringstream msg("connection ", std::ios_base::ate);
      msg << conn.getServerThreadId() << " removed from pool " << poolTag << "due to having throw a Connection exception (total:";
      msg << totalConnection.load() << ", active:" << getActiveConnections() << ", pending:" << pendingRequestNumber.load() << ")";
      logger->debug(msg.str());
    }
  }
  }
