/************************************************************************************
   Copyright (C) 2020,2023 MariaDB Corporation AB

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


#include <deque>

#include "ServerSidePreparedStatement.h"
#include "logger/LoggerFactory.h"
#include "ExceptionFactory.h"
#include "Results.h"
#include "MariaDbParameterMetaData.h"
#include "MariaDbResultSetMetaData.h"

namespace sql
{
  namespace mariadb
{

  Logger* ServerSidePreparedStatement::logger= LoggerFactory::getLogger(typeid(ServerSidePreparedStatement));
  ServerSidePreparedStatement::~ServerSidePreparedStatement()
  {
    // Statement has to be deleted before prepare result, because prepare result owns(and closes) C API stmt handle, and Results deleted in
    // MariaDBStatement might need to fetch remaining results(in case of streaming). Basically, closing stmt handle would be enough - this
    // fetches remaining results as well, but we can also have here CSPS, not only SSPS
    stmt.reset();
    if (serverPrepareResult) {
      if (serverPrepareResult->canBeDeallocate()) {
        delete serverPrepareResult;
      }
      else {
        serverPrepareResult->decrementShareCounter();
      }
    }
  }
  /**
    * Constructor for creating Server prepared statement.
    *
    * @param connection current connection
    * @param sql Sql String to prepare
    * @param resultSetScrollType one of the following <code>ResultSet</code> constants: <code>
    *     ResultSet.TYPE_FORWARD_ONLY</code>, <code>ResultSet.TYPE_SCROLL_INSENSITIVE</code>, or
    *     <code>ResultSet.TYPE_SCROLL_SENSITIVE</code>
    * @param resultSetConcurrency a concurrency type; one of <code>ResultSet.CONCUR_READ_ONLY</code>
    *     or <code>ResultSet.CONCUR_UPDATABLE</code>
    * @param autoGeneratedKeys a flag indicating whether auto-generated keys should be returned; one
    *     of <code>Statement.RETURN_GENERATED_KEYS</code> or <code>Statement.NO_GENERATED_KEYS</code>
    * @throws SQLException exception
    */
  ServerSidePreparedStatement::ServerSidePreparedStatement(
    MariaDbConnection* connection, const SQLString& _sql,
    int32_t resultSetScrollType,
    int32_t resultSetConcurrency,
    int32_t autoGeneratedKeys,
    Shared::ExceptionFactory& factory)
    : ServerSidePreparedStatement(connection, resultSetScrollType, resultSetConcurrency, autoGeneratedKeys, connection->getProtocol()->isMasterConnection(), factory)
  {
    serverPrepareResult= nullptr;
    sql= _sql;
    prepare(sql);
  }

  ServerSidePreparedStatement::ServerSidePreparedStatement(
    MariaDbConnection* _connection,
    int32_t resultSetScrollType,
    int32_t resultSetConcurrency,
    int32_t autoGeneratedKeys,
    bool _mustExecuteOnMaster,
    Shared::ExceptionFactory& factory)
    : BasePrepareStatement(_connection, resultSetScrollType, resultSetConcurrency, autoGeneratedKeys, factory)
    , serverPrepareResult(nullptr)
    , mustExecuteOnMaster(_mustExecuteOnMaster)
  {
  }

  /**
    * Clone statement.
    *
    * @param connection connection
    * @return Clone statement.
    * @throws CloneNotSupportedException if any error occur.
    */
  ServerSidePreparedStatement* ServerSidePreparedStatement::clone(MariaDbConnection* connection)
  {
    Shared::ExceptionFactory ef(ExceptionFactory::of(this->exceptionFactory->getThreadId(), this->exceptionFactory->getOptions()));
    ServerSidePreparedStatement* clone= new ServerSidePreparedStatement(connection, this->stmt->getResultSetType(), this->stmt->getResultSetConcurrency(),
      this->autoGeneratedKeys, this->mustExecuteOnMaster, ef);
    clone->metadata= metadata;
    clone->parameterMetaData= this->parameterMetaData;

    try {
      clone->prepare(sql);
    }
    catch (SQLException&) {
      throw SQLException("PreparedStatement could not be cloned"); //CloneNotSupportedException
    }
    return clone;
  }


  void ServerSidePreparedStatement::prepare(const SQLString& sql)
  {
    try {
      serverPrepareResult= protocol->prepare(sql, mustExecuteOnMaster);
      setMetaFromResult();
    }
    catch (SQLException& e) {
      try {
        this->close();
      }
      catch (std::exception&) {
      }
      logger->error("error preparing query", e);
      exceptionFactory->raiseStatementError(connection, stmt.get())->create(e).Throw();
    }
  }

  void ServerSidePreparedStatement::setMetaFromResult()
  {
    parameterCount= static_cast<int32_t>(serverPrepareResult->getParameters().size());
    initParamset(parameterCount);
    metadata.reset(new MariaDbResultSetMetaData(serverPrepareResult->getColumns(), protocol->getUrlParser().getOptions(), false));
    // TODO: these transfer of the vector can be optimized for sure
    parameterMetaData.reset(new MariaDbParameterMetaData(serverPrepareResult->getParameters()));
  }

  void ServerSidePreparedStatement::setParameter(int32_t parameterIndex, ParameterHolder* holder)
  {
    // TODO: does it really has to be map? can be, actually
    if (parameterIndex > 0 && static_cast<std::size_t>(parameterIndex) < serverPrepareResult->getParamCount() + 1) {
      parameters[parameterIndex - 1].reset(holder);
    }
    else {
      SQLString error("Could not set parameter at position ");

      error.append(std::to_string(parameterIndex)).append(" (values was ").append(holder->toString()).append(")\nQuery - conn:");

      // A bit ugly - index validity is checked after parameter holder objects have been created.
      delete holder;

      error.append(std::to_string(getServerThreadId())).append(protocol->isMasterConnection() ? "(M)" : "(S)");
      error.append(" - \"");

      uint32_t maxQuerySizeToLog= protocol->getOptions()->maxQuerySizeToLog;
      if (maxQuerySizeToLog > 0) {
        if (sql.size() < maxQuerySizeToLog) {
          error.append(sql);
        }
        else {
          error.append(sql.substr(0, maxQuerySizeToLog - 3) + "...");
        }
      }
      else {
        error.append(sql);
      }
      error.append(" - \"");
      logger->error(error);
      ExceptionFactory::INSTANCE.create(error).Throw();
    }
  }


  ParameterMetaData* ServerSidePreparedStatement::getParameterMetaData()
  {
    if (isClosed()) {
      throw SQLException("The query has been already closed");
    }

    return new MariaDbParameterMetaData(*parameterMetaData);
  }

  sql::ResultSetMetaData* ServerSidePreparedStatement::getMetaData()
  {
    return new MariaDbResultSetMetaData(*metadata);
  }

  const sql::Ints& ServerSidePreparedStatement::executeBatch()
  {
    stmt->checkClose();
    sql::Ints& res= stmt->getBatchResArr();
    res.wrap(nullptr, 0);
    int32_t queryParameterSize= static_cast<int32_t>(parameterList.size());
    if (queryParameterSize == 0) {
      return res;
    }
    executeBatchInternal(queryParameterSize);
    return res.wrap(stmt->getInternalResults()->getCmdInformation()->getUpdateCounts());
  }

  const sql::Longs& ServerSidePreparedStatement::executeLargeBatch()
  {
    stmt->checkClose();
    sql::Longs& res = stmt->getLargeBatchResArr();
    int32_t queryParameterSize= static_cast<int32_t>(parameterList.size());
    if (queryParameterSize == 0) {
      return res;
    }
    executeBatchInternal(queryParameterSize);
    return res.wrap(stmt->getInternalResults()->getCmdInformation()->getLargeUpdateCounts());
  }

  void ServerSidePreparedStatement::executeBatchInternal(int32_t queryParameterSize)
  {
    std::unique_lock<std::mutex> localScopeLock(*protocol->getLock());

    stmt->setExecutingFlag();

    try {
      executeQueryPrologue(serverPrepareResult);

      if (stmt->getQueryTimeout() != 0) {
        stmt->setTimerTask(true);
      }
      std::vector<Unique::ParameterHolder> dummy;
      stmt->setInternalResults(
        new Results(
          stmt.get(),
          0,
          true,
          queryParameterSize,
          true,
          stmt->getResultSetType(),
          stmt->getResultSetConcurrency(),
          autoGeneratedKeys,
          protocol->getAutoIncrementIncrement(),
          nullptr,
          dummy));

      serverPrepareResult->resetParameterTypeHeader();

      if ((protocol->getOptions()->useBatchMultiSend || protocol->getOptions()->useBulkStmts)
       && (protocol->executeBatchServer(
                                          mustExecuteOnMaster,
                                          serverPrepareResult,
                                          stmt->getInternalResults().get(),
                                          sql,
                                          parameterList,
                                          hasLongData)))
      {
        if (!metadata) {
          setMetaFromResult();
        }
        stmt->getInternalResults()->commandEnd();
        return;
      }

      SQLException exception("");
      bool exceptionSet= false;
      if (stmt->getQueryTimeout() > 0)
      {
        for (int32_t counter= 0; counter < queryParameterSize; counter++)
        {
          // TODO: verify if paramsets are guaranteed to exist at this point for all queryParameterSize
          std::vector<Unique::ParameterHolder>& parameterHolder= parameterList[counter];
          try {
            protocol->stopIfInterrupted();
            protocol->executePreparedQuery(mustExecuteOnMaster, serverPrepareResult, stmt->getInternalResults().get(), parameterHolder);
          }
          catch (SQLException& queryException)
          {
            if (protocol->getOptions()->continueBatchOnError
              && protocol->isConnected()
              &&!protocol->isInterrupted())
            {
              if (exceptionSet) {
                exception= queryException;
                exceptionSet= true;
              }
            }
            else {
              throw queryException;
            }
          }
        }
      }
      else {
        for (int32_t counter= 0; counter < queryParameterSize; counter++) {
          std::vector<Unique::ParameterHolder>& parameterHolder= parameterList[counter];
          try {
            protocol->executePreparedQuery(
              mustExecuteOnMaster, serverPrepareResult, stmt->getInternalResults().get(), parameterHolder);
          }
          catch (SQLException& queryException) {
            if (protocol->getOptions()->continueBatchOnError) {
              if (!exceptionSet) {
                exception= queryException;
              }
            }
            else {
              throw queryException;
            }
          }
        }
      }
      if (exceptionSet) {
        throw exception;
      }

      stmt->getInternalResults()->commandEnd();
    }
    catch (SQLException& initialSqlEx) {
      localScopeLock.unlock();
      throw stmt->executeBatchExceptionEpilogue(initialSqlEx, queryParameterSize);
    }
    stmt->executeBatchEpilogue();
  }

  // must have "lock" locked before invoking
  void ServerSidePreparedStatement::executeQueryPrologue(ServerPrepareResult* serverPrepareResult)
  {
    stmt->setExecutingFlag();

    stmt->checkClose();

    protocol->prologProxy(
      serverPrepareResult, stmt->getMaxRows(), protocol->getProxy()/*!= nullptr*/, connection, this->stmt.get());
  }


  bool ServerSidePreparedStatement::executeInternal(int32_t fetchSize)
  {
    validateParamset(serverPrepareResult->getParamCount());

    std::unique_lock<std::mutex> localScopeLock(*protocol->getLock());
    try {
      executeQueryPrologue(serverPrepareResult);
      if (stmt->getQueryTimeout() !=0) {
        stmt->setTimerTask(false);
      }

      stmt->setInternalResults(
        new Results(
          this,
          fetchSize,
          false,
          1,
          true,
          stmt->getResultSetType(),
          stmt->getResultSetConcurrency(),
          autoGeneratedKeys,
          protocol->getAutoIncrementIncrement(),
          sql,
          parameters));

      serverPrepareResult->resetParameterTypeHeader();
      protocol->executePreparedQuery(
        mustExecuteOnMaster, serverPrepareResult, stmt->getInternalResults().get(), parameters);

      stmt->getInternalResults()->commandEnd();
      stmt->executeEpilogue();
      return stmt->getInternalResults()->getResultSet() != nullptr;

    }
    catch (SQLException& exception) {
      stmt->executeEpilogue();
      localScopeLock.unlock();
      executeExceptionEpilogue(exception).Throw();
    }
    //To please compilers etc
    return false;
  }

  void ServerSidePreparedStatement::close()
  {
    if (stmt->isClosed()) {
      return;
    }
    std::lock_guard<std::mutex> localScopeLock(*protocol->getLock());

    stmt->markClosed();
    if (stmt->getInternalResults()) {
      if (stmt->getInternalResults()->getFetchSize()!=0) {
        stmt->skipMoreResults();
      }
      stmt->getInternalResults()->close();
    }

    if (serverPrepareResult != nullptr && protocol) {
      try {
        serverPrepareResult->getUnProxiedProtocol()->releasePrepareStatement(serverPrepareResult);
      }
      catch (SQLException&) {
      }
      serverPrepareResult= nullptr;
    }
    if (protocol->isClosed()
     || !connection->poolConnection
     || connection->poolConnection->noStmtEventListeners()) {
      connection= nullptr;
      return;
    }
    connection->poolConnection->fireStatementClosed(this);
    connection= nullptr;
  }

  int32_t ServerSidePreparedStatement::getParameterCount() const
  {
    return parameterCount;
  }

  /**
    * Return sql String value.
    *
    * @return String representation
    */
  SQLString ServerSidePreparedStatement::toString()
  {
    SQLString sb("sql : '" + serverPrepareResult->getSql() + "'");
    if (parameterCount > 0) {
      sb.append(", parameters : [");
      for (int32_t i= 0; i < parameterCount; i++)
      {
        if (!parameters[i]) {
          sb.append("NULL");
        }
        else {
          sb.append(parameters[i]->toString());
        }
        if (i != parameterCount - 1) {
          sb.append(",");
        }
      }
      sb.append("]");
    }
    return sb;
  }

  /**
    * Permit to retrieve current connection thread id, or -1 if unknown.
    *
    * @return current connection thread id.
    */
  int64_t ServerSidePreparedStatement::getServerThreadId()
  {
    return serverPrepareResult->getUnProxiedProtocol()->getServerThreadId();
  }
}
}
