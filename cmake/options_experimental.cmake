#
#  Copyright (C) 2024 MariaDB Corporation AB
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the COPYING-CMAKE-SCRIPTS file.
#

### 2024-08-29 by fmauger
### !!!BEWARE: this is an experimental feature!!!

### Special option available only  on Unix-like systems (likely Linux,
### macos not tested) to be able to use and link a non-system external
### installation of  the MariaDB Connector/C library  (also noted C/C)
### on which the MariaDB Connector/C++ library depends.

### This feature is provided in the  hope to solve some specific build
### issues from the  source tarball when we need to  target an already
### installed C/C  library in a non  system path and we  don't want to
### build and use that C/C library as an embedded subproject of C/C++.
### Note: At the time of this writting, this last technique was broken
### in the C/C++ source tarball version  1.1.5 built on a CentOS Linux
### release 7.9.2009 based cloud.

### The MariaDB Connector/C (example:  version 3.4.1 built from source
### tarball) is provided with some  pkg-config support, so we use this
### tool within CMake to locate the C/C lib (includes and libs) on the
### host machine (of course the PKG_CONFIG_PATH env.  var. should have
### been set properly by the user).

### Also, RPATH  is used  to unambiguously link  the C/C++  shared lib
### (Linux) to the C/C lib because we cannot take the risk to catch on
### a  wrong  file   at  runtime  and  also  we  won't   rely  on  the
### LD_LIBRARY_PATH env. var. in a production environment

IF(UNIX AND NOT APPLE)
  OPTION(USE_UNIX_SPECIAL_LIBMARIADBCONC "Use a specific installed (not system) MariaDB Connector/C library and do not build one (Unix only)" OFF)
  IF(USE_UNIX_SPECIAL_LIBMARIADBCONC)
    # Disable incompatible USE_SYSTEM_INSTALLED_LIB option:
    set(USE_SYSTEM_INSTALLED_LIB OFF)
  ENDIF()
ELSE()
  # Make sure this option is not set for other systems
  IF(USE_UNIX_SPECIAL_LIBMARIADBCONC)
    SET(USE_UNIX_SPECIAL_LIBMARIADBCONC OFF)
  ENDIF()
ENDIF()
