/*
 * Copyright (c) 2008, 2018, Oracle and/or its affiliates. All rights reserved.
 *               2020, 2022 MariaDB Corporation AB
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2.0, as
 * published by the Free Software Foundation.
 *
 * This program is also distributed with certain software (including
 * but not limited to OpenSSL) that is licensed under separate terms,
 * as designated in a particular file or component or in included license
 * documentation.  The authors of MySQL hereby grant you an
 * additional permission to link the program and your derivative works
 * with the separately licensed software that they have included with
 * MySQL.
 *
 * Without limiting anything contained in the foregoing, this file,
 * which is part of MySQL Connector/C++, is also subject to the
 * Universal FOSS Exception, version 1.0, a copy of which can be found at
 * http://oss.oracle.com/licenses/universal-foss-exception.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License, version 2.0, for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA
 */



#include "Warning.hpp"

#include <sstream>
#include <cstdlib>
#include <cstring>
#include <stdlib.h>
#include "ResultSet.hpp"
#include "conncpp/Types.hpp"
#include "conncpp/Connection.hpp"
#include "resultsettest.h"


namespace testsuite
{
namespace classes
{

static const sql::SQLString id("id");

void resultset::getInt()
{
  bool on_off=true;
  //con->setClientOption("clientTrace", &on_off);
  // Message for --verbose output
  logMsg("resultset::getInt - MySQL_ResultSet::getInt*");
  try
  {
    stmt.reset(con->createStatement());
    stmt->execute("DROP TABLE IF EXISTS test");
    stmt->execute("CREATE TABLE test(i integer, i_uns integer unsigned, b bigint, b_uns bigint unsigned)");

    int64_t r1_c1=L64(2147483646),
            r1_c2=L64(2147483650),
            r1_c3=L64(9223372036854775806),
            r2_c1=L64(2147483647),
            r2_c2=L64(2147483649),
            r2_c3=L64(9223372036854775807);

    uint64_t r1_c4=UL64(9223372036854775810),
            r2_c4=UL64(18446744073709551615);

    pstmt.reset(con->prepareStatement("INSERT INTO test(i, i_uns, b, b_uns) VALUES(?,?,?,?)"));

    ASSERT(pstmt.get() != NULL);
    pstmt->clearParameters();
    pstmt->setInt(1, static_cast<int> (r1_c1));
    pstmt->setInt64(2, r1_c2);
    pstmt->setInt64(3, r1_c3);
    pstmt->setUInt64(4, r1_c4);
    ASSERT_EQUALS(false, pstmt->execute());

    pstmt->clearParameters();
    pstmt->setInt(1, static_cast<int> (r2_c1));
    pstmt->setInt64(2, r2_c2);
    pstmt->setInt64(3, r2_c3);
    pstmt->setUInt64(4, r2_c4);
    ASSERT_EQUALS(false, pstmt->execute());

    pstmt.reset(con->prepareStatement("SELECT i, i_uns, b, b_uns FROM test"));
    ASSERT(pstmt.get() != NULL);
    ASSERT(pstmt->execute());

    res.reset(pstmt->getResultSet());
    checkResultSetScrolling(res);

    ASSERT(res->next());

    ASSERT_EQUALS(r1_c1, (int64_t) res->getInt("i"));
    ASSERT_EQUALS(r1_c1, (int64_t) res->getInt(1));

    ASSERT_EQUALS(r1_c2, res->getInt64("i_uns"));
    ASSERT_EQUALS(r1_c2, res->getInt64(2));

    ASSERT_EQUALS(r1_c3, res->getInt64("b"));
    ASSERT_EQUALS(r1_c3, res->getInt64(3));

    ASSERT_EQUALS(r1_c4, res->getUInt64("b_uns"));
    ASSERT_EQUALS(r1_c4, res->getUInt64(4));

    ASSERT(res->next());

    ASSERT_EQUALS(r2_c1, (int64_t) res->getInt("i"));
    ASSERT_EQUALS(r2_c1, (int64_t) res->getInt(1));

    ASSERT_EQUALS(r2_c2, res->getInt64("i_uns"));
    ASSERT_EQUALS(r2_c2, res->getInt64(2));

    ASSERT_EQUALS(r2_c3, res->getInt64("b"));
    ASSERT_EQUALS(r2_c3, res->getInt64(3));

    ASSERT_EQUALS(r2_c4, res->getUInt64("b_uns"));
    ASSERT_EQUALS(r2_c4, res->getUInt64(4));

    ASSERT_EQUALS(res->next(), false);

    res.reset(stmt->executeQuery("SELECT i, i_uns, b, b_uns FROM test"));
    checkResultSetScrolling(res);

    ASSERT(res->next());

    ASSERT_EQUALS(r1_c1, (int64_t) res->getInt("i"));
    ASSERT_EQUALS(r1_c1, (int64_t) res->getInt(1));

    ASSERT_EQUALS(r1_c2, res->getInt64("i_uns"));
    ASSERT_EQUALS(r1_c2, res->getInt64(2));

    ASSERT_EQUALS(r1_c3, res->getInt64("b"));
    ASSERT_EQUALS(r1_c3, res->getInt64(3));

    ASSERT_EQUALS(r1_c4, res->getUInt64("b_uns"));
    ASSERT_EQUALS(r1_c4, res->getUInt64(4));

    ASSERT(res->next());

    ASSERT_EQUALS(r2_c1, (int64_t) res->getInt("i"));
    ASSERT_EQUALS(r2_c1, (int64_t) res->getInt(1));

    ASSERT_EQUALS(r2_c2, res->getInt64("i_uns"));
    ASSERT_EQUALS(r2_c2, res->getInt64(2));

    ASSERT_EQUALS(r2_c3, res->getInt64("b"));
    ASSERT_EQUALS(r2_c3, res->getInt64(3));

    ASSERT_EQUALS(r2_c4, res->getUInt64("b_uns"));
    ASSERT_EQUALS(r2_c4, res->getUInt64(4));

    ASSERT_EQUALS(res->next(), false);

    stmt->execute("DROP TABLE IF EXISTS test");
  }
  catch (sql::SQLException &e)
  {
    logErr(e.what());
    logErr("SQLState: " + std::string(e.getSQLState()));
    fail(e.what(), __FILE__, __LINE__);
  }
  on_off=false;
  //con->setClientOption("clientTrace", &on_off);
}

void resultset::getTypes()
{
  logMsg("resultset::getTypes - MySQL_ResultSet::get*");
  std::vector<columndefinition>::iterator it;
  std::stringstream msg;
  bool got_warning=false;
  ResultSet pres;
  std::string ps_value;
  std::string::size_type len_st;
  std::string::size_type len_ps;

  try
  {
    stmt.reset(con->createStatement());
    logMsg("... looping over all kinds of column types");

    for (it = columns.begin(); it != columns.end(); it++)
    {
      stmt->execute("DROP TABLE IF EXISTS test");
      msg.str("");
      msg << "CREATE TABLE test(dummy TIMESTAMP, id " << it->sqldef << ")";
      try
      {
        stmt->execute(msg.str());
        msg.str("");
        msg << "... testing " << it->sqldef << ", value = '" << it->value << "'";
        logMsg(msg.str());
      }
      catch (sql::SQLException&)
      {
        msg.str("");
        msg << "... skipping " << it->sqldef;
        logMsg(msg.str());
        continue;
      }

      msg.str("");
      switch (it->ctype)
      {
      case sql::DataType::BIT:
        msg << "INSERT INTO test(id) VALUES (" << it->value << ")";
        break;
      default:
        msg << "INSERT INTO test(id) VALUES (\"" << it->value << "\")";
      }

      try
      {
        stmt->execute(msg.str());
        ASSERT_EQUALS(1, (int)stmt->getUpdateCount());
      }
      catch (sql::SQLException & e)
      {
        logErr(e.what());
        logErr("SQLState: " + std::string(e.getSQLState()));
        logErr("SqlDef: " + it->sqldef);
        fail(e.what(), __FILE__, __LINE__);
      }

      res.reset(stmt->executeQuery("SELECT id, NULL FROM test"));
      checkResultSetScrolling(res);
      ASSERT(res->next());

      pstmt.reset(con->prepareStatement("SELECT id, NULL FROM test"));
      pstmt->clearParameters();
      pres.reset(pstmt->executeQuery());
      checkResultSetScrolling(pres);
      ASSERT(pres->next());


      if (it->check_as_string)
      {
        logMsg("... checking string value");
        if (it->as_string != res->getString(id))
        {
          msg.str("");
          msg << "... expecting '" << it->as_string << "', got '" << res->getString(id) << "'";
          logMsg(msg.str());
          got_warning = true;
        }
      }

      ASSERT(res->getString(id).compare(res->getString(1)) == 0);

      try
      {
        res->getString(0);
        FAIL("Invalid argument not detected");
      }
      catch (sql::InvalidArgumentException&)
      {
      }

      try
      {
        res->getString(3);
        FAIL("Invalid argument not detected");
      }
      catch (sql::InvalidArgumentException&)
      {
      }

      if (res->getType() == sql::ResultSet::TYPE_FORWARD_ONLY) {
        // We can't do beforeFirst and other cursor operation on forward only cursor
        continue;
      }
      res->beforeFirst();
      try
      {
        res->getString(1);
        FAIL("Invalid argument not detected");
      }
      catch (sql::SQLDataException&)
      {
      }
      res->afterLast();
      try
      {
        res->getString(1);
        FAIL("Invalid argument not detected");
      }
      catch (sql::SQLDataException&)
      {
      }
      res->first();
      ASSERT_EQUALS(res->getBoolean(id), res->getBoolean(1));
      try
      {
        res->getBoolean(0);
        FAIL("Invalid argument not detected");
      }
      catch (sql::InvalidArgumentException&)
      {
      }

      try
      {
        res->getBoolean(3);
        FAIL("Invalid argument not detected");
      }
      catch (sql::InvalidArgumentException&)
      {
      }

      res->beforeFirst();
      try
      {
        res->getBoolean(1);
        FAIL("Invalid argument not detected");
      }
      catch (sql::SQLDataException&)
      {
      }
      res->afterLast();
      try
      {
        res->getBoolean(1);
        FAIL("Invalid argument not detected");
      }
      catch (sql::SQLDataException&)
      {
      }

      res->first();
      // Comparing prepared statement resultset and statement resultset
      if (it->check_as_string && (pres->getString(id) != res->getString(id)))
      {
        if (it->sqldef.find("ZEROFILL", 0) == std::string::npos)
        {
          ps_value = pres->getString(id);
          len_st = res->getString(id).length();
          len_ps = ps_value.length();
          if (len_ps > len_st)
          {
            // Something like 1.01000 vs. 1.01 ?
            std::string::size_type i;
            for (i = len_st; i < len_ps; i++)
            {
              if (ps_value.at(i) != '0')
                break;
            }
            if (i < (len_ps - 1))
            {
              got_warning = true;
              msg.str("");
              msg << "... \t\tWARNING - getString(), PS: '" << pres->getString(id) << "'";
              msg << ", Statement: '" << res->getString(id) << "'";
              logMsg(msg.str());
            }
          }
        }
      }

      try
      {
        ASSERT_EQUALS(res->getDouble(id), res->getDouble(1));
      }
      catch (sql::SQLException & e)
      {
        if ((it->name.compare("DATE") != 0 && it->name.compare("DATETIME") != 0 && it->name.compare("TIMESTAMP") != 0 && it->name.compare("TIME") != 0 &&
             it->name.compare("CHAR") != 0 && it->name.compare("BINARY") != 0 && it->name.compare("VARCHAR") != 0 && it->name.compare("VARBINARY") != 0 &&
             it->name.compare("TINYBLOB") != 0 && it->name.compare("TINYTEXT") != 0 && it->name.compare("TEXT") != 0 && it->name.compare("BLOB") != 0 &&
             it->name.compare("MEDIUMTEXT") != 0 && it->name.compare("MEDIUMBLOB") != 0 && it->name.compare("LONGBLOB") != 0 &&
          it->name.compare("LONGTEXT") != 0 && it->name.compare("ENUM") && it->name.compare("SET"))
          || (!e.getMessage().startsWith("getDouble not available for data field type Types::") && !e.getMessage().startsWith("Incorrect format ")))
        {
          throw e;
        }
        if (it->name.compare("TINYBLOB") != 0 && it->name.compare("BLOB") != 0 && it->name.compare("MEDIUMBLOB") != 0 &&
          it->name.compare("LONGBLOB") != 0)
        {
          try
          {
            res->getInt(id);
            FAIL("getInt shouldn't be available for this type");
          }
          catch (sql::SQLException&)
          {
          }
          try
          {
            pres->getUInt(id);
            FAIL("getUInt shouldn't be available for this type");
          }
          catch (sql::SQLException&)
          {
          }
        }
        // There is not sense to continue for this type further
        continue;
      }
      try
      {
        res->getDouble(0);
        FAIL("Invalid argument not detected");
      }
      catch (sql::InvalidArgumentException&)
      {
      }

      try
      {
        res->getDouble(3);
        FAIL("Invalid argument not detected");
      }
      catch (sql::InvalidArgumentException&)
      {
      }

      res->beforeFirst();
      try
      {
        res->getDouble(1);
        FAIL("Invalid argument not detected");
      }
      catch (sql::SQLDataException&)
      {
      }
      res->afterLast();
      try
      {
        res->getDouble(1);
        FAIL("Invalid argument not detected");
      }
      catch (sql::SQLDataException&)
      {
      }
      res->first();

      int64_t intValue= 0;
      bool isNumber= true, inIntRange= true, inUintRange= !it->is_negative, inInt64Range= true;
      try
      {
        if (it->ctype == sql::DataType::BINARY || it->ctype == sql::DataType::VARBINARY ||
          it->ctype == sql::DataType::LONGVARBINARY)
        {
          std::size_t len= it->precision;
          if (it->ctype != sql::DataType::BINARY)
          {
            len= it->value.length();
          }

          if ( len > sizeof(int32_t))
          {
            inIntRange= false;
            inUintRange= false;
          }
          if (len > sizeof(int64_t))
          {
            inInt64Range= false;
          }
        }
        else
        {
          if (it->is_negative)
          {
            intValue= std::stoll(it->value.c_str());
          }
          else
          {
            intValue= static_cast<int64_t>(std::stoull(it->value.c_str()));
          }
        }
      }
      catch (...)
      {
        isNumber= false;
      }

      if ((!it->is_negative && intValue < 0) || intValue < INT32_MIN || intValue > INT32_MAX)
      {
        inIntRange= false;
        try
        {
          res->getInt(1);
          FAIL("Overflow of int value is not detected");
        }
        catch (sql::SQLException & e)
        {
          //All is good
          ASSERT_EQUALS(1264, e.getErrorCode());
          ASSERT_EQUALS("22003", e.getSQLState());
        }

        if (intValue >= 0 && intValue <= UINT32_MAX)
        {
          ASSERT_EQUALS(res->getUInt(1), res->getUInt(id));
        }
        else
        {
          inUintRange= false;
          if (!it->is_negative && intValue < 0)// i.e. value is in fact uint64_t
          {
            try
            {
              res->getLong(1); //getInt64 is the same
              FAIL("Overflow of int64 value is not detected");
            }
            catch (sql::SQLException & e)
            {
              //All is good
              ASSERT_EQUALS(1264, e.getErrorCode());
              ASSERT_EQUALS("22003", e.getSQLState());
            }
          }
          else
          {
            ASSERT_EQUALS(res->getInt64(1), res->getInt64(id));
          }
        }
      }
      else
      {
        try
        {
          ASSERT_EQUALS(res->getInt(1), res->getInt(id));
        }
        catch (sql::SQLException & e)
        {
 //         if (!isNumber)
          {
            ASSERT_EQUALS(1264, e.getErrorCode());
            ASSERT_EQUALS("22003", e.getSQLState());
          }
         /* else
          {
            throw e;
          }*/
        }
      }

      try
      {
        res->getInt(0);
        FAIL("Invalid argument not detected");
      }
      catch (sql::InvalidArgumentException&)
      {
      }

      try
      {
        res->getInt(3);
        FAIL("Invalid argument not detected");
      }
      catch (sql::InvalidArgumentException&)
      {
      }

      res->beforeFirst();
      try
      {
        res->getInt(1);
        FAIL("Invalid argument not detected");
      }
      catch (sql::SQLDataException&)
      {
      }
      res->afterLast();
      try
      {
        res->getInt(1);
        FAIL("Invalid argument not detected");
      }
      catch (sql::SQLDataException&)
      {
      }
      res->first();

      if (!isNumber || it->is_negative || !inUintRange)
      {
        try
        {
          ASSERT_EQUALS(res->getUInt(1), res->getUInt(id));

          FAIL("Range error(negative value) has not been detected");
        }
        catch (sql::SQLException & e)
        {
          ASSERT_EQUALS(1264, e.getErrorCode());
          ASSERT_EQUALS("22003", e.getSQLState());
        }
      }
      else
      {
        ASSERT_EQUALS(res->getUInt(1), res->getUInt(id));
      }

      try
      {
        res->getUInt(0);
        FAIL("Invalid argument not detected");
      }
      catch (sql::InvalidArgumentException &)
      {
      }

      try
      {
        res->getUInt(3);
        FAIL("Invalid argument not detected");
      }
      catch (sql::InvalidArgumentException &)
      {
      }

      res->beforeFirst();
      try
      {
        res->getUInt(1);
        FAIL("Invalid argument not detected");
      }
      catch (sql::SQLDataException&)
      {
      }
      res->afterLast();
      try
      {
        res->getUInt(1);
        FAIL("Invalid argument not detected");
      }
      catch (sql::SQLDataException&)
      {
      }
      res->first();

      // intValue >= 0 means it's in int64 range
      if (it->is_negative || intValue >= 0)
      {
        try
        {
          ASSERT_EQUALS(res->getInt64(id), res->getInt64(1));
        }
        catch (sql::SQLException & e)
        {
          //if (!isNumber)
          {
            ASSERT_EQUALS(1264, e.getErrorCode());
            ASSERT_EQUALS("22003", e.getSQLState());
          }
          /*else
          {
            throw e;
          }*/
        }
      }
      try
      {
        res->getInt64(0);
        FAIL("Invalid argument not detected");
      }
      catch (sql::InvalidArgumentException &)
      {
      }

      try
      {
        res->getInt64(3);
        FAIL("Invalid argument not detected");
      }
      catch (sql::InvalidArgumentException &)
      {
      }

      res->beforeFirst();
      try
      {
        res->getInt64(1);
        FAIL("Invalid argument not detected");
      }
      catch (sql::SQLDataException&)
      {
      }
      res->afterLast();
      try
      {
        res->getInt64(1);
        FAIL("Invalid argument not detected");
      }
      catch (sql::SQLDataException&)
      {
      }
      res->first();

      if (it->is_negative || !isNumber || !inInt64Range)
      {
        try
        {
          ASSERT_EQUALS(res->getUInt64(1), res->getUInt64(id));

          FAIL("Range error(negative value) has not been detected");
        }
        catch (sql::SQLException & e)
        {
          ASSERT_EQUALS(1264, e.getErrorCode());
          ASSERT_EQUALS("22003", e.getSQLState());
        }
      }
      else
      {
        ASSERT_EQUALS(res->getUInt64(1), res->getUInt64(id));
      }

      try
      {
        res->getUInt64(0);
        FAIL("Invalid argument not detected");
      }
      catch (sql::InvalidArgumentException &)
      {
      }

      try
      {
        res->getUInt64(3);
        FAIL("Invalid argument not detected");
      }
      catch (sql::InvalidArgumentException &)
      {
      }

      res->beforeFirst();
      try
      {
        res->getUInt64(1);
        FAIL("Invalid argument not detected");
      }
      catch (sql::SQLDataException&)
      {
      }
      res->afterLast();
      try
      {
        res->getUInt64(1);
        FAIL("Invalid argument not detected");
      }
      catch (sql::SQLDataException&)
      {
      }

      res->first();

      try
      {
        if (!fuzzyEquals(pres->getDouble(id), res->getDouble(id), 0.001))
        {
          msg.str("");
          msg << "... \t\tWARNING - getDouble(), PS: '" << pres->getDouble(id) << "'";
          msg << ", Statement: '" << res->getDouble(id) << "'";
          msg << ", Difference: '" << (pres->getDouble(id) - res->getDouble(id)) << "'";
          logMsg(msg.str());
          got_warning = true;
        }
      }
      catch (sql::SQLException & e)
      {
        if (it->name.compare("DATE") != 0 || !e.getMessage().startsWith("getDouble not available for data field type Types::"))
        {
          throw e;
        }
      }

      if (isNumber && inIntRange && (pres->getInt(id) != res->getInt(id)))
      {
        msg.str("");
        msg << "... \t\tWARNING - getInt(), PS: '" << pres->getInt(id) << "'";
        msg << ", Statement: '" << res->getInt(id) << "'";
        logMsg(msg.str());
        got_warning=true;
      }

      if (isNumber && !it->is_negative && inInt64Range && (pres->getUInt64(id) != res->getUInt64(id)))
      {
        msg.str("");
        msg << "... \t\tWARNING - getUInt64(), PS: '" << pres->getUInt64(id) << "'";
        msg << ", Statement: '" << res->getUInt64(id) << "'";
        logMsg(msg.str());
        got_warning=true;
      }

      if (isNumber && inUintRange)
      {
        ASSERT_EQUALS(pres->getUInt(id), res->getUInt(id));
      }

      if (isNumber && inInt64Range && (it->is_negative || intValue >= 0) && pres->getInt64(id) != res->getInt64(id))
      {
        msg.str("");
        msg << "... \t\tWARNING - getInt64(), PS: '" << pres->getInt64(id) << "'";
        msg << ", Statement: '" << res->getInt64(id) << "'";
        logMsg(msg.str());
        got_warning=true;
      }
      // ASSERT_EQUALS(pres->getUInt64(id), res->getUInt64(id));

      ASSERT_EQUALS(pres->getBoolean(id), res->getBoolean(1));

    }
    if (got_warning)
      FAIL("See warnings!");

    stmt->execute("DROP TABLE IF EXISTS test");

  }
  catch (sql::SQLException &e)
  {
    logErr(e.what());
    logErr("SQLState: " + std::string(e.getSQLState()));
    fail(e.what(), __FILE__, __LINE__);
  }
}

void resultset::getTypesMinorIssues()
{
  logMsg("resultset::getTypesMinorIssues - MySQL_ResultSet::get*");

  std::vector<columndefinition>::iterator it;
  std::stringstream msg;
  bool got_warning=false;
  bool got_minor_warning=false;
  ResultSet pres;
  std::string ps_value;
  std::string::size_type len_st;
  std::string::size_type len_ps;

  try
  {
    stmt.reset(con->createStatement());
    logMsg("... looping over all kinds of column types");
    for (it=columns.begin(); it != columns.end(); it++)
    {
      stmt->execute("DROP TABLE IF EXISTS test");
      msg.str("");
      msg << "CREATE TABLE test(dummy TIMESTAMP, id " << it->sqldef << ")";
      try
      {
        stmt->execute(msg.str());
        msg.str("");
        msg << "... testing " << it->sqldef << ", value = '" << it->value << "'";
        logMsg(msg.str());
      }
      catch (sql::SQLException &)
      {
        msg.str("");
        msg << "... skipping " << it->sqldef;
        logMsg(msg.str());
        continue;
      }

      msg.str("");
      switch(it->ctype)
      {
      case sql::DataType::BIT:
        msg << "INSERT INTO test(id) VALUES (" << it->value << ")";
        break;
      default:
        msg << "INSERT INTO test(id) VALUES (\"" << it->value << "\")";
      }

      try
      {
        stmt->execute(msg.str());
        ASSERT_EQUALS(1, (int)stmt->getUpdateCount());
      }
      catch (sql::SQLException &e)
      {
        logErr(e.what());
        logErr("SQLState: " + std::string(e.getSQLState()));
        logErr("SqlDef: " + it->sqldef);
        fail(e.what(), __FILE__, __LINE__);
      }

      res.reset(stmt->executeQuery("SELECT id, NULL FROM test"));
      checkResultSetScrolling(res);
      ASSERT(res->next());

      pstmt.reset(con->prepareStatement("SELECT id, NULL FROM test"));
      pstmt->clearParameters();
      pres.reset(pstmt->executeQuery());
      checkResultSetScrolling(pres);
      ASSERT(pres->next());

      if (it->check_as_string && (it->as_string != res->getString(id)))
      {
        msg.str("");
        msg << "... expecting '" << it->as_string << "', got '" << res->getString(id) << "'";
        logMsg(msg.str());
        got_warning=true;

      }

      // Comparing prepared statement resultset and statement resultset
      if (pres->getString(id) != res->getString(id))
      {
        if (it->sqldef.find("ZEROFILL", 0) == std::string::npos)
        {
          bool is_minor=false;
          ps_value=pres->getString(id);
          len_st=res->getString(id).length();
          len_ps=ps_value.length();
          if (len_ps > len_st)
          {
            // Something like 1.01000 vs. 1.01 ?
            std::string::size_type i;
            for (i=len_st; i < len_ps; i++)
            {
              if (ps_value.at(i) != '0')
                break;
            }
            if (i < (len_ps - 1))
            {
              got_warning=true;
            }
            else
            {
              is_minor=true;
              got_minor_warning=true;
            }
          }
          if (!it->check_as_string)
          {
            is_minor=true;
            got_minor_warning=true;
          }
          else
          {
            got_warning=true;
          }
          msg.str("");
          if (is_minor)
          {
            msg << "... \t\tMINOR WARNING - getString(), PS: '" << pres->getString(id) << "'";
          }
          else
          {
            msg << "... \t\tWARNING - getString(), PS: '" << pres->getString(id) << "'";
          }
          msg << ", Statement: '" << res->getString(id) << "'";
          logMsg(msg.str());
        }

      }

    }
    if (got_warning)
      FAIL("See --verbose warnings!");

    /*
    * We decided to ignore the differences. It is
    * about number formatting only. Any application using C/C++
    * will apply their own formatting style anyway.
    * No simple fix came to our mind. If we ever have an
    * idea we should fix it. Meanwhile: won't fix.
    */
    if (got_minor_warning && false)
    {
      FAIL("TODO - see MINOR WARNING when using --verbose");
    }

    stmt->execute("DROP TABLE IF EXISTS test");
  }
  catch (sql::SQLException &e)
  {
    logErr(e.what());
    logErr("SQLState: " + std::string(e.getSQLState()));
    fail(e.what(), __FILE__, __LINE__);
  }
}


#ifdef INCLUDE_NOT_IMPLEMENTED_METHODS

void resultset::notImplemented()
{
  logMsg("resultset::notImplemented - MySQL_ResultSet::*");

  try
  {
    stmt.reset(con->createStatement());
    stmt->execute("DROP TABLE IF EXISTS test");
    stmt->execute("CREATE TABLE test(id INT)");
    ASSERT_EQUALS(1, stmt->executeUpdate("INSERT INTO test(id) VALUES (1)"));
    res.reset(stmt->executeQuery("SELECT id FROM test"));
    doNotImplemented();

    pstmt.reset(con->prepareStatement("SELECT id FROM test"));
    res.reset(pstmt->executeQuery());
    doNotImplemented();

    stmt->execute("DROP TABLE IF EXISTS test");
  }
  catch (sql::SQLException &e)
  {
    logErr(e.what());
    logErr("SQLState: " + std::string(e.getSQLState()));
    fail(e.what(), __FILE__, __LINE__);
  }
}

void resultset::doNotImplemented()
{

  try
  {
    res->getWarnings();
  }
  catch (sql::MethodNotImplementedException)
  {
  }

  try
  {
    res->clearWarnings();
  }
  catch (sql::MethodNotImplementedException)
  {
  }

  try
  {
    res->insertRow();
  }
  catch (sql::MethodNotImplementedException)
  {
  }

  try
  {
    res->moveToCurrentRow();
  }
  catch (sql::MethodNotImplementedException)
  {
  }

  try
  {
    res->moveToInsertRow();
  }
  catch (sql::MethodNotImplementedException)
  {
  }

  try
  {
    res->refreshRow();
  }
  catch (sql::MethodNotImplementedException)
  {
  }

  try
  {
    res->rowDeleted();
  }
  catch (sql::MethodNotImplementedException)
  {
  }

  try
  {
    res->rowInserted();
  }
  catch (sql::MethodNotImplementedException)
  {
  }

  try
  {
    res->rowUpdated();
  }
  catch (sql::MethodNotImplementedException)
  {
  }
}
#endif

void resultset::fetchBigint()
{
  std::stringstream msg;

  logMsg("resultset::fetchBigint - MySQL_ResultSet::*");
  try
  {
    stmt.reset(con->createStatement());
    stmt->execute("DROP TABLE IF EXISTS test");
    stmt->execute("CREATE TABLE test(id BIGINT UNSIGNED)");
    stmt->execute("INSERT INTO test(id) VALUES (18446744073709551615)");
    res.reset(stmt->executeQuery("SELECT id FROM test"));

#ifdef INCLUDE_NOT_IMPLEMENTED_METHODS
    doNotImplemented();
#endif

    pstmt.reset(con->prepareStatement("SELECT id FROM test"));
    res.reset(pstmt->executeQuery());
    res->next();

    msg.str("");
    msg << "... PS: id = " << res->getDouble(1);
    logMsg(msg.str());

    res.reset(stmt->executeQuery("SELECT id FROM test"));
    res->next();

    msg.str("");
    msg << "... Statement: id = " << res->getDouble(1);
    logMsg(msg.str());

    stmt->execute("DROP TABLE IF EXISTS test");
  }
  catch (sql::SQLException &e)
  {
    logErr(e.what());
    logErr("SQLState: " + std::string(e.getSQLState()));
    fail(e.what(), __FILE__, __LINE__);
  }
}

void resultset::fetchBitAsInt()
{
  std::stringstream msg;

  logMsg("resultset::fetchBitAsInt - MySQL_ResultSet::*");
  try
  {
    logMsg("... BIT(0) - non-PS");
    stmt.reset(con->createStatement());
    stmt->execute("DROP TABLE IF EXISTS test");
    stmt->execute("CREATE TABLE test(id BIT(1))");
    stmt->execute("INSERT INTO test(id) VALUES (0)");
    stmt->execute("INSERT INTO test(id) VALUES (1)");

    res.reset(stmt->executeQuery("SELECT id, CAST(id AS SIGNED) AS bit_as_signed FROM test ORDER BY id"));
    while (res->next())
    {
      ASSERT_EQUALS(res->getInt(id), res->getInt("bit_as_signed"));
      ASSERT_EQUALS(res->getInt(id), res->getInt(1));
    }

    logMsg("... BIT(0) - PS");
    pstmt.reset(con->prepareStatement("SELECT id, CAST(id AS SIGNED) AS bit_as_signed FROM test ORDER BY id"));
    res.reset(pstmt->executeQuery());
    while (res->next())
    {
      ASSERT_EQUALS(res->getInt(id), res->getInt("bit_as_signed"));
      ASSERT_EQUALS(res->getInt(id), res->getInt(1));
    }


    uint64_t c1=UL64(4294967295),
            c2=UL64(18446744073709551615);

    logMsg("... BIT(32), BIT(64) - non-PS");
    stmt->execute("DROP TABLE IF EXISTS test");
    stmt->execute("CREATE TABLE test(col1 BIT(32), col2 BIT(64))");
    stmt->execute("INSERT INTO test(col1, col2) VALUES(b'11111111111111111111111111111111', b'1111111111111111111111111111111111111111111111111111111111111111')");

    res.reset(stmt->executeQuery("SELECT col1, CAST(col1 AS UNSIGNED) AS col1_as_unsigned, col2, CAST(col2 AS UNSIGNED) AS col2_as_unsigned FROM test"));
    ASSERT(res->next());
    ASSERT_EQUALS(c1, res->getUInt64(1));
    ASSERT_EQUALS(res->getUInt64("col1_as_unsigned"), res->getUInt64(1));
    ASSERT_EQUALS(c2, res->getUInt64("col2"));
    ASSERT_EQUALS(res->getUInt64("col2_as_unsigned"), res->getUInt64("col2"));

    logMsg("... BIT(32), BIT(64) - PS");
    stmt->execute("DELETE FROM test");
    pstmt.reset(con->prepareStatement("INSERT INTO test(col1, col2) VALUES(?,?)"));
    pstmt->clearParameters();
    pstmt->setUInt64(1, c1);
    pstmt->setUInt64(2, c2);
    ASSERT_EQUALS(false, pstmt->execute());

    pstmt.reset(con->prepareStatement("SELECT col1, CAST(col1 AS UNSIGNED) AS col1_as_unsigned, col2, CAST(col2 AS UNSIGNED) AS col2_as_unsigned FROM test"));
    res.reset(pstmt->executeQuery());
    ASSERT(res->next());
    ASSERT_EQUALS(c1, res->getUInt64(1));
    ASSERT_EQUALS(res->getUInt64("col1_as_unsigned"), res->getUInt64(1));
    ASSERT_EQUALS(c2, res->getUInt64("col2"));
    ASSERT_EQUALS(res->getUInt64("col2_as_unsigned"), res->getUInt64("col2"));

    stmt->execute("DROP TABLE IF EXISTS test");
  }
  catch (sql::SQLException &e)
  {
    logErr(e.what());
    logErr("SQLState: " + std::string(e.getSQLState()));
    fail(e.what(), __FILE__, __LINE__);
  }
}


void resultset::getResultSetType()
{
  sql::ConnectOptionsMap connection_properties;

  logMsg("resultset::getResultSetType - MySQL_ResultSet::*");

  SKIP("defaultStatementResultType connection option is not supported")
  try
  {
    /* user comes from the unit testing framework */
    connection_properties["user"]= user;

    connection_properties["password"]= passwd;

    connection_properties.erase("defaultStatementResultType");
    {
      logMsg("... testing defaultStatementResultType");
      connection_properties["defaultStatementResultType"]= std::to_string(sql::ResultSet::TYPE_FORWARD_ONLY);
      try
      {
        created_objects.clear();
        con.reset(driver->connect(url, connection_properties));
      }
      catch (sql::SQLException &e)
      {
        fail(e.what(), __FILE__, __LINE__);
      }
      stmt.reset(con->createStatement());
      ASSERT_EQUALS(stmt->getResultSetType(), sql::ResultSet::TYPE_FORWARD_ONLY);
      pstmt.reset(con->prepareStatement("SELECT 1"));
      /* NOTE: no bug - PS supports TYPE_SCROLL_INSENSITIVE only and we
       are setting StatementResultType not PreparedStatementResultType */
      res.reset(pstmt->executeQuery());
      ASSERT_EQUALS(pstmt->getResultSetType(), sql::ResultSet::TYPE_FORWARD_ONLY);// sql::ResultSet::TYPE_SCROLL_INSENSITIVE);

      connection_properties.erase("defaultStatementResultType");
      connection_properties["defaultStatementResultType"]= std::to_string(sql::ResultSet::TYPE_SCROLL_INSENSITIVE);
      try
      {
        created_objects.clear();
        con.reset(driver->connect(url, connection_properties));
      }
      catch (sql::SQLException &e)
      {
        fail(e.what(), __FILE__, __LINE__);
      }
      stmt.reset(con->createStatement());
      ASSERT_EQUALS(stmt->getResultSetType(), sql::ResultSet::TYPE_SCROLL_INSENSITIVE);
      pstmt.reset(con->prepareStatement("SELECT 1"));
      res.reset(pstmt->executeQuery());
      ASSERT_EQUALS(pstmt->getResultSetType(), sql::ResultSet::TYPE_SCROLL_INSENSITIVE);

      connection_properties.erase("defaultStatementResultType");
      connection_properties["defaultStatementResultType"]= std::to_string(sql::ResultSet::TYPE_SCROLL_SENSITIVE);
      try
      {
        created_objects.clear();
        try
        {
          con.reset(driver->connect(url, connection_properties));
          FAIL("Bug or API change - TYPE_SCROLL_SENSITIVE is unsupported");
          stmt.reset(con->createStatement());
          ASSERT_EQUALS(stmt->getResultSetType(), sql::ResultSet::TYPE_SCROLL_SENSITIVE);
          pstmt.reset(con->prepareStatement("SELECT 1"));
          res.reset(pstmt->executeQuery());
          ASSERT_EQUALS(pstmt->getResultSetType(), sql::ResultSet::TYPE_SCROLL_INSENSITIVE);
        }
        catch (sql::SQLException &e)
        {
          logMsg("... expected exception because TYPE_SCROLL_SENSITIVE is unsupported!");
          logMsg(e.what());
        }
      }
      catch (sql::SQLException &e)
      {
        fail(e.what(), __FILE__, __LINE__);
      }

    }
    connection_properties.erase("defaultStatementResultType");
  }
  catch (sql::SQLException &e)
  {
    logErr(e.what());
    logErr("SQLState: " + std::string(e.getSQLState()));
    fail(e.what(), __FILE__, __LINE__);
  }
}


void resultset::JSON_support()
{
  std::stringstream msg;

  logMsg("resultset::JSON_support - MySQL_ResultSet::*");

  if (getServerVersion(con) < 1002007)
  {
    SKIP("Server does not support JSON");
  }

  try
  {
    stmt.reset(con->createStatement());
    stmt->execute("DROP TABLE IF EXISTS test");
    stmt->execute("CREATE TABLE test(id INT, jval JSON)");
    stmt->execute("INSERT INTO test(id, jval) VALUES(1, '[1]')");

#ifdef INCLUDE_NOT_IMPLEMENTED_METHODS
    doNotImplemented();
#endif

    pstmt.reset(con->prepareStatement("SELECT * FROM test"));
    res.reset(pstmt->executeQuery());
    res->next();

    msg.str("");
    ASSERT_EQUALS(1, res->getInt(1));
    msg << "... PS: id = " << res->getInt(1);
    ASSERT_EQUALS("[1]", res->getString(2));
    msg << "... PS: jval = " << res->getString(2);
    logMsg(msg.str());

    res.reset(stmt->executeQuery("SELECT * FROM test"));
    res->next();

    msg.str("");
    ASSERT_EQUALS(1, res->getInt(1));
    msg << "... Statement: id = " << res->getInt(1);
    ASSERT_EQUALS("[1]", res->getString(2));
    msg << "... Statement: jval = " << res->getString(2);
    logMsg(msg.str());

    stmt->execute("DROP TABLE IF EXISTS test");
  }
  catch (sql::SQLException &e)
  {
    logErr(e.what());
    logErr("SQLState: " + std::string(e.getSQLState()));
    fail(e.what(), __FILE__, __LINE__);
  }
}


void resultset::concpp72_rs_streaming()
{
  sql::Properties connection_properties;
  logMsg("resultset::concpp72_rs_streaming - MySQL_ResultSet::*");
  std::vector<int32_t> id({ 1, 2, 3, 4, 7 });
  std::vector<sql::SQLString> strVal({ "string longer", "SHORT", "Some text 3", "midsize", "Another value 5" });
  std::vector<std::string> binVal({ {"a\0\0b", 4}, {"\0a\0b", 4}, {"z\0o\0b", 5}, {"xz\0\0", 4}, {"xxxxxx", 5} });

  try
  {
    /* user comes from the unit testing framework */
    connection_properties["defaultFetchSize"]= "1";
    connection_properties["useServerPrepStmts"]= "true";

    logMsg("... testing ResultSet streaming with fetchSize=1");

    try
    {
      created_objects.clear();
      con.reset(getConnection(&connection_properties));
    }
    catch (sql::SQLException & e)
    {
      fail(e.what(), __FILE__, __LINE__);
    }
    stmt.reset(con->createStatement());
    createSchemaObject("TABLE", "t_concpp72", "(id int unsigned not null primary key, strVal varchar(32) not null, binVal varbinary(8) not null)");
    stmt->executeUpdate("INSERT INTO t_concpp72 VALUES(1, 'string longer', _binary'a\\0\\0b'),(2, 'SHORT', _binary'\\0a\\0b'),"
      "(4, 'midsize', _binary'xz\\0\\0')");

    ASSERT_EQUALS(stmt->getResultSetType(), sql::ResultSet::TYPE_FORWARD_ONLY);
    ASSERT_EQUALS(1, stmt->getFetchSize());
    res.reset(stmt->executeQuery("SELECT id, strVal, binVal FROM t_concpp72 ORDER BY id"));
    
    ASSERT(res->next());
    ASSERT_EQUALS(1, res->getInt(1));
    ASSERT_EQUALS("string longer", res->getString(2));
    sql::SQLString col3(res->getString(3));
    ASSERT_EQUALS(4ULL, static_cast<uint64_t>(col3.length()));
    ASSERT(std::memcmp(col3.c_str(), "a\0\0b", 4) == 0);

    pstmt.reset(con->prepareStatement("SELECT id, strVal, binVal FROM t_concpp72 ORDER BY id DESC"));
    ASSERT_EQUALS(1, pstmt->getFetchSize());
    ResultSet rs2(pstmt->executeQuery());
    ASSERT_EQUALS(pstmt->getResultSetType(), sql::ResultSet::TYPE_FORWARD_ONLY);

    ASSERT_EQUALS(1, res->getInt(1));
    ASSERT_EQUALS("string longer", res->getString(2));
    col3= res->getString(3);
    ASSERT_EQUALS(4ULL, static_cast<uint64_t>(col3.length()));
    ASSERT(std::memcmp(col3.c_str(), "a\0\0b", 4) == 0);
    ASSERT(res->next());
    ASSERT_EQUALS(2, res->getInt(1));
    ASSERT_EQUALS("SHORT", res->getString(2));
    col3= res->getString(3);
    ASSERT_EQUALS(4ULL, static_cast<uint64_t>(col3.length()));
    ASSERT(std::memcmp(col3.c_str(), "\0a\0b", 4) == 0);
    ASSERT(res->next());
    ASSERT_EQUALS(4, res->getInt(1));
    ASSERT_EQUALS("midsize", res->getString(2));
    col3 = res->getString(3);
    ASSERT_EQUALS(4ULL, static_cast<uint64_t>(col3.length()));
    ASSERT(std::memcmp(col3.c_str(), "xz\0\0", 4) == 0);
    ASSERT(!res->next());

    // Now the result from PS
    ASSERT(rs2->next());
    ASSERT_EQUALS(4, rs2->getInt(1));
    ASSERT_EQUALS("midsize", rs2->getString(2));
    col3 = rs2->getString(3);
    ASSERT_EQUALS(4ULL, static_cast<uint64_t>(col3.length()));
    ASSERT(std::memcmp(col3.c_str(), "xz\0\0", 4) == 0);
    // We just need to send any command. to force driver to cache the data
    ASSERT_EQUALS(2, stmt->executeUpdate("INSERT INTO t_concpp72 VALUES(3, 'Some text 3', _binary'z\\0o\\0b'),(7, 'Another value 5', _binary'xxxxx')"));

    ASSERT_EQUALS(4, rs2->getInt(1));
    ASSERT_EQUALS("midsize", rs2->getString(2));
    col3 = rs2->getString(3);
    ASSERT_EQUALS(4ULL, static_cast<uint64_t>(col3.length()));
    ASSERT(std::memcmp(col3.c_str(), "xz\0\0", 4) == 0);

    ASSERT(rs2->next());
    ASSERT_EQUALS(2, rs2->getInt(1));
    ASSERT_EQUALS("SHORT", rs2->getString(2));
    col3 = rs2->getString(3);
    ASSERT_EQUALS(4ULL, static_cast<uint64_t>(col3.length()));
    ASSERT(std::memcmp(col3.c_str(), "\0a\0b", 4) == 0);
    ASSERT(rs2->next());
    ASSERT_EQUALS(1, rs2->getInt(1));
    ASSERT_EQUALS("string longer", rs2->getString(2));
    col3 = rs2->getString(3);
    ASSERT_EQUALS(4ULL, static_cast<uint64_t>(col3.length()));
    ASSERT(std::memcmp(col3.c_str(), "a\0\0b", 4) == 0);
    ASSERT(!rs2->next());

    stmt->setFetchSize(4);
    res.reset(stmt->executeQuery("SELECT id, strVal, binVal, CAST(NULL AS CHAR) FROM t_concpp72 ORDER BY id"));

    for (std::size_t i = 0; i < id.size(); ++i)
    {
      ASSERT(res->next());
      ASSERT_EQUALS(id[i], res->getInt(1));
      ASSERT_EQUALS(strVal[i], res->getString(2));
      col3 = res->getString(3);
      /* Some how this is ambiquous without casting on macos */
      ASSERT_EQUALS(static_cast<uint64_t>(binVal[i].length()), static_cast<uint64_t>(col3.length()));
      ASSERT(std::memcmp(col3.c_str(), binVal[i].c_str(), col3.length()) == 0);
      ASSERT(res->isNull(4));
    }
    ASSERT(!res->next());

    pstmt.reset(con->prepareStatement("SELECT id, strVal, binVal, CAST(NULL AS CHAR), CAST(id AS CHAR) FROM t_concpp72 ORDER BY id DESC"));
    pstmt->setFetchSize(4);
    rs2.reset(pstmt->executeQuery());
    for (std::size_t i= id.size() - 1; i != static_cast<std::size_t>(-1); --i)
    {
      ASSERT(rs2->next());
      ASSERT_EQUALS(id[i], rs2->getInt(1));
      ASSERT_EQUALS(static_cast<int64_t>(id[i]), rs2->getLong(1));
      ASSERT_EQUALS(strVal[i], rs2->getString(2));
      col3 = rs2->getString(3);
      ASSERT_EQUALS(static_cast<uint64_t>(binVal[i].length()), static_cast<uint64_t>(col3.length()));
      ASSERT(std::memcmp(col3.c_str(), binVal[i].c_str(), col3.length()) == 0);
      ASSERT(rs2->isNull(4));
      ASSERT_EQUALS(id[i], rs2->getInt(5));
      ASSERT_EQUALS(static_cast<int64_t>(id[i]), rs2->getLong(5));
      ASSERT_EQUALS(static_cast<uint64_t>(id[i]), rs2->getUInt64(5));
      ASSERT_EQUALS(static_cast<uint32_t>(id[i]), rs2->getUInt(5));
    }
    ASSERT(!rs2->next());
  }
  catch (sql::SQLException & e)
  {
    logErr(e.what());
    logErr("SQLState: " + std::string(e.getSQLState()));
    fail(e.what(), __FILE__, __LINE__);
  }
}

} /* namespace resultset */
} /* namespace testsuite */
