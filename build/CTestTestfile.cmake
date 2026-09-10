# CMake generated Testfile for 
# Source directory: C:/Users/himanshu/.gemini/antigravity/scratch/Retour
# Build directory: C:/Users/himanshu/.gemini/antigravity/scratch/Retour/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(AnalyticsTest "C:/Users/himanshu/.gemini/antigravity/scratch/Retour/build/Debug/retour_test.exe")
  set_tests_properties(AnalyticsTest PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/himanshu/.gemini/antigravity/scratch/Retour/CMakeLists.txt;64;add_test;C:/Users/himanshu/.gemini/antigravity/scratch/Retour/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(AnalyticsTest "C:/Users/himanshu/.gemini/antigravity/scratch/Retour/build/Release/retour_test.exe")
  set_tests_properties(AnalyticsTest PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/himanshu/.gemini/antigravity/scratch/Retour/CMakeLists.txt;64;add_test;C:/Users/himanshu/.gemini/antigravity/scratch/Retour/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(AnalyticsTest "C:/Users/himanshu/.gemini/antigravity/scratch/Retour/build/MinSizeRel/retour_test.exe")
  set_tests_properties(AnalyticsTest PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/himanshu/.gemini/antigravity/scratch/Retour/CMakeLists.txt;64;add_test;C:/Users/himanshu/.gemini/antigravity/scratch/Retour/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(AnalyticsTest "C:/Users/himanshu/.gemini/antigravity/scratch/Retour/build/RelWithDebInfo/retour_test.exe")
  set_tests_properties(AnalyticsTest PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/himanshu/.gemini/antigravity/scratch/Retour/CMakeLists.txt;64;add_test;C:/Users/himanshu/.gemini/antigravity/scratch/Retour/CMakeLists.txt;0;")
else()
  add_test(AnalyticsTest NOT_AVAILABLE)
endif()
subdirs("_deps/googletest-build")
