/************************************************************************************
   Copyright (C) 2020 MariaDB Corporation AB

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


#ifndef _SERVERSIDEPREPAREDSTATEMENT_H_
#define _SERVERSIDEPREPAREDSTATEMENT_H_

#include "Consts.h"

#include "util/ServerPrepareResult.h"
#include "parameters/ParameterHolder.h"
#include "BasePrepareStatement.h"

namespace sql
{
namespace mariadb
{
class MariaDbResultSetMetaData;

/* For the sake of speeed(of initial development), leaving it derived from BasePreparedStatement and it's partial PS implementation
 * In future I guess we should get rid of that
 */
class ServerSidePreparedStatement : public BasePrepareStatement {

  static Logger* logger;

protected:
  int32_t parameterCount; /*-1*/

private:
  SQLString sql;

  ServerPrepareResult *serverPrepareResult;

  Shared::MariaDbResultSetMetaData metadata;
  Shared::MariaDbParameterMetaData parameterMetaData;

  bool mustExecuteOnMaster;

public:
  ~ServerSidePreparedStatement();
  ServerSidePreparedStatement(
    MariaDbConnection* connection, const SQLString& sql,
    int32_t resultSetScrollType,
    int32_t resultSetConcurrency,
    int32_t autoGeneratedKeys,
    Shared::ExceptionFactory& factory);
  ServerSidePreparedStatement* clone(MariaDbConnection* connection);

private:
  ServerSidePreparedStatement(
    MariaDbConnection* connection,
    int32_t resultSetScrollType,
    int32_t resultSetConcurrency,
    int32_t autoGeneratedKeys,
    bool mustExecuteOnMaster,
    Shared::ExceptionFactory& factory);

  void prepare(const SQLString& sql);
  void setMetaFromResult();

public:
  void setParameter(int32_t parameterIndex,/*const*/ ParameterHolder* holder);
  sql::ParameterMetaData* getParameterMetaData();
  sql::ResultSetMetaData* getMetaData();
  const sql::Ints& executeBatch();
  const sql::Longs& executeLargeBatch();

private:
  void executeBatchInternal(int32_t queryParameterSize);
  void executeQueryPrologue(ServerPrepareResult* serverPrepareResult);
  Logger* getLogger() const { return logger; }

public:
  PrepareResult* getPrepareResult() { return dynamic_cast<PrepareResult*>(serverPrepareResult); }
  bool executeInternal(int32_t fetchSize);

public:
  void close();

//protected:
  int32_t getParameterCount() const;

public:
  SQLString toString();
  int64_t getServerThreadId();
  };
}
}
#endif