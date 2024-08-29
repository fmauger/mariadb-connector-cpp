
IF(USE_UNIX_SPECIAL_LIBMARIADBCONC)
  # Support for pkg-config script:
  CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/libmariadbcpp.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/libmariadbcpp.pc @ONLY)
  INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/libmariadbcpp.pc
    DESTINATION "${INSTALL_PCDIR}"
    COMPONENT Development)
ENDIF()
 
