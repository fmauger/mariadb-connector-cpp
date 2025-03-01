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


#include <cassert>

#include "Consts.h"
#include "HostAddress.h"
#include "logger/LoggerFactory.h"
#include "Exception.hpp"

namespace sql
{
  namespace mariadb
  {
    Logger* HostAddress::logger= LoggerFactory::getLogger(typeid(HostAddress));

    HostAddress::HostAddress() : host(""), port(DefaultPort) {}


    HostAddress::HostAddress(const SQLString& _host, uint32_t _port)
      : host(_host)
      , port(_port)
      , type(ParameterConstant::TYPE_MASTER)
    {
    }


    HostAddress::HostAddress(const SQLString& _host, uint32_t _port, const SQLString& _type)
      : host(_host)
      , port(_port)
      , type(_type)
    {
    }

    static bool prepareCapiFailoverHost(const SQLString& specOrig, std::vector<HostAddress>& arr)
    {
      // Our Hosts lists format is digestable by Conn/C, thus adding it as is
      arr.emplace_back(specOrig, 3306);
      return false;
    }

    HostAddress::HostAddress(const HostAddress& other)
      : host(other.host)
      , port(other.port)
      , type(other.type)
    {}

    HostAddress& HostAddress::operator=(const HostAddress& right)
    {
      host= right.host;
      port= right.port;
      type= right.type;
      return *this;
    }

    HostAddress::HostAddress(HostAddress &&moved) :
      host(std::move(moved.host)), port(moved.port), type(std::move(moved.type))
    {
    }

    HostAddress::~HostAddress()
    {}

    std::vector<HostAddress> HostAddress::parse(const SQLString& specOrig, enum HaMode haMode) {
      //TODO: "upstream has here difference between NULL and empty str, and looks like that has reason for us in this case, too
      //       thus we will need to decide what do do here - throw, return empty array, or make some equivalent of NULL and empty as well
      if (specOrig.empty()) {
        throw IllegalArgumentException("Invalid connection URL, host address must not be empty");
      }

      std::vector<HostAddress> arr;

      if (specOrig.empty()) {
        return arr;
      }

      if (haMode == HaMode::SEQUENTIAL) {
        if (prepareCapiFailoverHost(specOrig, arr)) {
          throw IllegalArgumentException("Could not parse failover hosts list");
        }
        return arr;
      }

      SQLString spec(specOrig);
      Tokens tokens= split(spec.trim(), ",");
      size_t size= tokens->size();

      /* AURORA is not supported atm */
      if (haMode == HaMode::AURORA) {
        /* clusterPattern("(.+)\\.cluster-([a-z0-9]+\\.[a-z0-9\\-]+\\.rds\\.amazonaws\\.com)",
          std::regex_constants::ECMAScript | std::regex_constants::icase);
        if (!std::regex_search(StringImp::get(spec), clusterPattern)) {
          logger->warn("Aurora recommended connection URL must only use cluster end-point like "
            "\"jdbc:mariadb:aurora://xx.cluster-yy.zz.rds.amazonaws.com\". "
            "Using end-point permit auto-discovery of new replicas");
        }*/
      }
      for (auto& token : *tokens) {
        if (token.startsWith("address=")) {
          arr.emplace_back(parseParameterHostAddress(token));
        }
        else {
          arr.emplace_back(parseSimpleHostAddress(token));
        }
      }
      if (haMode == HaMode::REPLICATION)
      {
        for (size_t i= 0; i < size; i++)
        {
          if (i == 0 && arr[i].type.empty()) {
            arr[i].type= ParameterConstant::TYPE_MASTER;
          }
          else if (i !=0 &&arr[i].type.empty()) {
            arr[i].type= ParameterConstant::TYPE_SLAVE;
          }
        }
      }
      return arr;
    }


    HostAddress HostAddress::parseSimpleHostAddress(const SQLString& str)
    {
      HostAddress result;

      if (str.at(0) == '[') {
        size_t ind= str.find_first_of(']');
        result.host= str.substr(1, ind);
        if (ind != (str.length() - 1) && str.at(ind + 1) == ':') {
          result.port= getPort(str.substr(ind +2));
        }
      }
      else if ((str.find_first_of(':') != std::string::npos)) {
        Tokens hostPort= split(str, ":");
        result.host= (*hostPort)[0];
        assert(hostPort->size() > 1);
        result.port= getPort((*hostPort)[1]);
      }
      else {
        result.host= str;
        result.port= 3306;
      }
      return result;
    }


    int32_t HostAddress::getPort(const SQLString& portString)
    {
      try {
        return std::stoi(StringImp::get(portString));
      }
      catch (std::invalid_argument &) {
        throw IllegalArgumentException("Incorrect port value : " + portString);
      }
    }


    HostAddress HostAddress::parseParameterHostAddress(const SQLString& _str)
    {
      HostAddress result;
      Tokens array= split(_str, "(?=\\()|(?<=\\))");
      std::size_t parenthesis= 0, closing= 0;
      while ((parenthesis= _str.find_first_of('(')) != std::string::npos) 
      {
        closing= _str.find_first_of(')', parenthesis + 1);
        if (closing == std::string::npos) {
          throw IllegalArgumentException("Invalid connection URL, expected (key=value) pairs, found " + _str);
        }
        SQLString str(_str.c_str() + parenthesis + 1, closing - parenthesis - 1);
        Tokens token= split(str.trim(), "=");

        if (token->size() != 2) {
          throw IllegalArgumentException("Invalid connection URL, expected key=value pairs, found " + str);
        }

        SQLString key((*token)[0].toLowerCase());
        SQLString value((*token)[1].toLowerCase());

        if ((key.compare("host") == 0))
        {
          replaceAny(result.host, "[]", "");
        }
        else if ((key.compare("port") == 0)) {
          result.port= getPort(value);
        }
        else if ((key.compare("type") == 0)
          && (value.compare(ParameterConstant::TYPE_MASTER) == 0
            || value.compare(ParameterConstant::TYPE_SLAVE) == 0 )) {
          result.type= value;
        }
        ++closing;
      }
      return result;
    }


    SQLString HostAddress::toString(std::vector<HostAddress> addrs)
    {
      /* stringstream would probably be more optimal to use here */
      SQLString str;
      for (std::size_t i= 0; i < addrs.size(); ++i) {
        auto& addr= addrs[i];
        if (! addr.type.empty()) {
          str.append("address=(host=")
            .append(addr.host)
            .append(")(port=")
            .append(std::to_string(addr.port))
            .append(")(type=")
            .append(addr.type)
            .append(")");
        }
        else
        {
          bool isIPv6= !addr.host.empty() && (addr.host.find_first_of(':') != std::string::npos);
          SQLString host= (isIPv6) ? ("["+addr.host +"]") : addr.host;
          str.append(host).append(":").append(std::to_string(addr.port));
        }
        if (i < addrs.size() - 1) {
          str.append(",");
        }
      }
      return str;
    }
#ifdef WEVE_FIGURED_OUT_WE_NEED_IT
    SQLString HostAddress::toString(HostAddress* addrs) {
      SQLString str;
      for (int32_t i= 0; i <addrs.length; i++) {
        if (addrs[i].type != nullptr) {
          str.append("address=(host=")
            .append(addrs[i].host)
            .append(")(port=")
            .append(addrs[i].port)
            .append(")(type=")
            .append(addrs[i].type)
            .append(")");
        }
        else {
          bool isIPv6= addrs[i].host != nullptr && (addrs[i].host.find_first_of(':') != std::string::npos);
          SQLString host= (isIPv6) ? ("["+addrs[i].host +"]") : addrs[i].host;
          str.append(host).append(":").append(addrs[i].port);
        }
        if (i <addrs.length -1) {
          str.append(",");
        }
      }
      return str;
    }
#endif
    SQLString HostAddress::toString() const
    {
      SQLString result("HostAddress{host='");
      return  result.append(host).append("'").append(", port=") + std::to_string(port) + (!type.empty() ? (", type='"+type +"'") : "") +"}";
    }

    bool HostAddress::equals(HostAddress* obj)
    {
      if (this == obj) {
        return true;
      }
      if (obj == nullptr) {
        return false;
      }
      return port == obj->port &&
        (!host.empty() ? host.compare(obj->host) == 0 : obj->host.empty()) &&
        (!type.empty() ? type.compare(obj->type) == 0 : obj->type.empty());
    }
    int64_t HostAddress::hashCode()
    {
      int64_t result= host.hashCode();
      result= 31 *result + port;
      return result;
    }
  }
}
