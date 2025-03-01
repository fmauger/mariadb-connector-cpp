/************************************************************************************
   Copyright (C) 2020, 2023 MariaDB Corporation plc

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


#ifndef _SERVERPREPARERESULT_H_
#define _SERVERPREPARERESULT_H_

#include <atomic>
#include <mutex>

#include "Consts.h"

#include "PrepareResult.h"

namespace sql
{
namespace mariadb
{
namespace capi
{
#include "mysql.h"
}

class ColumnDefinition;
class ColumnType;
class ParameterHolder;

class ServerPrepareResult  : public PrepareResult {

  std::vector<Shared::ColumnDefinition> columns;
  std::vector<Shared::ColumnDefinition> parameters; // atm it's always containempty elements and only used for parameters number
  const SQLString sql;
  capi::MYSQL_STMT* statementId;
  std::unique_ptr<capi::MYSQL_RES, decltype(&capi::mysql_free_result)> metadata;
  std::vector<capi::MYSQL_BIND> paramBind;
  Protocol* unProxiedProtocol;
  volatile int32_t shareCounter= 1;
  volatile bool isBeingDeallocate= false;
  std::mutex lock;

public:
  typedef std::vector<Unique::ParameterHolder> ParamsetType;
  typedef std::vector<ParamsetType> ParamsetArrType;
  ~ServerPrepareResult();

  /*ServerPrepareResult(
    const SQLString& sql,
    capi::MYSQL_STMT* statementId,
    std::vector<Shared::ColumnDefinition>& columns,
    std::vector<Shared::ColumnDefinition>& parameters,
    Protocol* unProxiedProtocol);*/

  ServerPrepareResult(
    const SQLString& sql,
    capi::MYSQL_STMT* statementId,
    Protocol* unProxiedProtocol);

  void reReadColumnInfo();

  void resetParameterTypeHeader();
  void failover(capi::MYSQL_STMT* statementId, Shared::Protocol& unProxiedProtocol);
  bool incrementShareCounter();
  void decrementShareCounter();
  bool canBeDeallocate();
  size_t getParamCount() const;
  int32_t getShareCounter();
  capi::MYSQL_STMT* getStatementId();
  void resetStmtId();
  const std::vector<Shared::ColumnDefinition>& getColumns() const;
  const std::vector<Shared::ColumnDefinition>& getParameters() const;
  Protocol* getUnProxiedProtocol();
  const SQLString& getSql() const;
  const std::vector<capi::MYSQL_BIND>& getParameterTypeHeader() const;
  void bindParameters(ParamsetType& parameters);
  void bindParameters(ParamsetArrType& parameters, const int16_t *type= nullptr);
  };
}
}
#endif
