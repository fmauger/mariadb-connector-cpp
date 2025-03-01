/************************************************************************************
   Copyright (C) 2020,2024 MariaDB Corporation plc

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


#ifndef _SELECTRESULTSET_H_
#define _SELECTRESULTSET_H_

#include <exception>
#include <vector>

// Should go before Consts
#include "ColumnDefinition.h"

#include "Consts.h"

#include "MariaDbStatement.h"

#include "ResultSet.hpp"
#include "ColumnType.h"
//#include "ColumnNameMap.h"
#include "io/StandardPacketInputStream.h"

#include "jdbccompat.hpp"
#include "util/ServerPrepareResult.h"

namespace sql
{
namespace mariadb
{
class TimeZone;
class RowProtocol;
class PacketInputStream;
class ColumnDefinition;
//class ServerPrepareResult;

namespace capi
{
#include "mysql.h"
}
class SelectResultSet : public ResultSet
{
  // We need this function since INSERT_ID_COLUMNS depends on the static data from other translation unit,
  // and we cannot guarantee it's initialized before INSERT_ID_COLUMNS
  static bool InitIdColumns();
protected:
  static const SQLString NOT_UPDATABLE_ERROR; /*"Updates are not supported when using ResultSet*.CONCUR_READ_ONLY"*/
  static std::vector<Shared::ColumnDefinition> INSERT_ID_COLUMNS;
  static uint64_t MAX_ARRAY_SIZE; /*Integer.MAX_VALUE -8*/

  static int32_t TINYINT1_IS_BIT; /*1*/
  static int32_t YEAR_IS_DATE_TYPE; /*2*/
  bool released=  false;
  int32_t dataFetchTime= 0;
  bool streaming= false;
  int32_t fetchSize;

  SelectResultSet(int32_t _fetchSize) :
    fetchSize(_fetchSize) 
  {}
public:
  static SelectResultSet* create(
    std::vector<Shared::ColumnDefinition>& columnInformation,
    Results* results,
    Protocol* protocol,
    PacketInputStream* reader,
    bool callableResult,
    bool eofDeprecated);

  static SelectResultSet* create(
    Results* results,
    Protocol* protocol,
    ServerPrepareResult* pr,
    bool callableResult,
    bool eofDeprecated);

  static SelectResultSet* create(
    Results* results,
    Protocol* protocol,
    capi::MYSQL* capiConnHandle,
    bool eofDeprecated);

  static SelectResultSet* create(
    std::vector<Shared::ColumnDefinition>& columnInformation,
    /*std::unique_ptr<*/std::vector<std::vector<sql::bytes>>& resultSet,
    Protocol* protocol,
    int32_t resultSetScrollType);

  static ResultSet* createGeneratedData(std::vector<int64_t>& data, Protocol* protocol, bool findColumnReturnsOne);
  static SelectResultSet* createEmptyResultSet();

  /**
  * Create a result set from given data. Useful for creating "fake" resultSets for
  * DatabaseMetaData, (one example is MariaDbDatabaseMetaData.getTypeInfo())
  *
  * @param columnNames - string array of column names
  * @param columnTypes - column types
  * @param data - each element of this array represents a complete row in the ResultSet. Each value
  *     is given in its string representation, as in MariaDB text protocol, except boolean (BIT(1))
  *     values that are represented as "1" or "0" strings
  * @param protocol protocol
  * @return resultset
  */

  static ResultSet* createResultSet(std::vector<SQLString>& columnNames, std::vector<ColumnType>& columnTypes,
    std::vector<std::vector<sql::bytes>>& data, Protocol* protocol);

  virtual ~SelectResultSet();

  virtual bool isFullyLoaded() const=0;
  virtual void fetchRemaining()=0;
 
protected:
  //SelectResultSet() : released(false) {}
  virtual std::vector<sql::bytes>& getCurrentRowData()=0;
  virtual void updateRowData(std::vector<sql::bytes>& rawData)=0;
  virtual void deleteCurrentRowData()=0;
  virtual void addRowData(std::vector<sql::bytes>& rawData)=0;
  void addStreamingValue(bool cacheLocally= false);
  virtual bool readNextValue(bool cacheLocally= false)= 0;

public:
  // These 2 methods are currently hidden in the ResultSet, but used internally. Thus (temporary) adding them here.
  //virtual sql::bytes* getBytes(const SQLString& columnLabel)=0;
  //virtual sql::bytes* getBytes(int32_t columnIndex)=0;
  virtual void abort()=0;
  virtual bool isCallableResult() const=0;
  virtual MariaDbStatement* getStatement()=0;
  virtual void setStatement(MariaDbStatement* statement)=0;
  virtual void setForceTableAlias()=0;
  virtual int32_t getRowPointer()=0;

protected:
  virtual void setRowPointer(int32_t pointer)=0;
  
public:
  /* Some classes(Results) may hold pointer to this object - it may be required in case of RS streaming to
     to fetch remaining rows in order to unblock connection for new queries, or to close RS, if next RS is requested or statements is destructed.
     After releasing the RS by API methods, it's owned by application. If app destructs the RS, this method is called by destructor, and
     implementation should do the job on checking out of the object, so it can't be attempted to use any more */
  virtual void checkOut()= 0;
  virtual std::size_t getDataSize()=0;
  virtual bool isBinaryEncoded()=0;
  ResultSet* release();
  virtual void realClose(bool noLock=false)=0;
  // If we need to cache rs, that did not stream, it will not have protocol, as it's kinda not needed after fetching everything
  virtual void cacheCompleteLocally(/*Protocol**/)=0;
  };
}
}
#endif
