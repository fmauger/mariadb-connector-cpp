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


#ifndef _TEXTROWPROTOCOLCAPI_H_
#define _TEXTROWPROTOCOLCAPI_H_

#include "Consts.h"

#include "com/RowProtocol.h"

namespace sql
{
namespace mariadb
{
namespace capi
{
#include "mysql.h"

class TextRowProtocolCapi  : public RowProtocol {

  std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)> capiResults;
  MYSQL_ROW  rowData;
  unsigned long* lengthArr;

public:
  TextRowProtocolCapi(int32_t maxFieldSize, Shared::Options options, MYSQL_RES* capiTextResults);
  ~TextRowProtocolCapi() {}

  void setPosition(int32_t newIndex);

#ifdef JDBC_SPECIFIC_TYPES_IMPLEMENTED
  sql::Object* getInternalObject(ColumnDefinition* columnInfo,TimeZone* timeZone);
  ZonedDateTime getInternalZonedDateTime( ColumnDefinition* columnInfo,Class clazz,TimeZone* timeZone);
  OffsetTime getInternalOffsetTime(ColumnDefinition* columnInfo,TimeZone* timeZone);
  LocalTime getInternalLocalTime(ColumnDefinition* columnInfo,TimeZone* timeZone);
  LocalDate getInternalLocalDate(ColumnDefinition* columnInfo,TimeZone* timeZone);
  BigInteger getInternalBigInteger(ColumnDefinition* columnInfo);
#endif
  int32_t fetchNext();
  void installCursorAtPosition(int32_t rowPtr);

  Date getInternalDate(ColumnDefinition* columnInfo, Calendar* cal=nullptr, TimeZone* timeZone=nullptr);
  Time getInternalTime(ColumnDefinition* columnInfo, Calendar* cal=nullptr, TimeZone* timeZone=nullptr);
  Timestamp getInternalTimestamp( ColumnDefinition* columnInfo, Calendar* cal=nullptr, TimeZone* timeZone=nullptr);
  SQLString getInternalString(ColumnDefinition* columnInfo, Calendar* cal=nullptr, TimeZone* timeZone=nullptr);
  int32_t getInternalInt(ColumnDefinition* columnInfo);
  int64_t getInternalLong(ColumnDefinition* columnInfo);
  uint64_t getInternalULong(ColumnDefinition* columnInfo);
  float getInternalFloat(ColumnDefinition* columnInfo);
  long double getInternalDouble(ColumnDefinition* columnInfo);
  BigDecimal getInternalBigDecimal(ColumnDefinition* columnInfo);

  bool getInternalBoolean(ColumnDefinition* columnInfo);
  int8_t getInternalByte(ColumnDefinition* columnInfo);
  int16_t getInternalShort(ColumnDefinition* columnInfo);
  SQLString getInternalTimeString(ColumnDefinition* columnInfo);

  bool isBinaryEncoded();
  void cacheCurrentRow(std::vector<sql::bytes>& rowData, std::size_t columnCount);
  };

}
}
}
#endif