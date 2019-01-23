# CMake generated Testfile for 
# Source directory: C:/git/shar/src/network/rtnet/tests
# Build directory: C:/git/shar/build/src/network/rtnet/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(rtnettest "C:/git/shar/build/bin/rtnettest.exe")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(rtnettest "C:/git/shar/build/bin/rtnettest.exe")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(rtnettest "C:/git/shar/build/bin/rtnettest.exe")
elseif("${CTEST_CONFIGURATION_TYPE}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(rtnettest "C:/git/shar/build/bin/rtnettest.exe")
else()
  add_test(rtnettest NOT_AVAILABLE)
endif()
