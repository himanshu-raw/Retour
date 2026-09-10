cmake_minimum_required(VERSION 3.14)
project(Retour CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set thread flags
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Include directories for our third party libraries
include_directories(${CMAKE_SOURCE_DIR}/third_party/sqlite)
include_directories(${CMAKE_SOURCE_DIR}/third_party/httplib)
include_directories(${CMAKE_SOURCE_DIR}/third_party/nlohmann)
include_directories(${CMAKE_SOURCE_DIR}/third_party)
include_directories(${CMAKE_SOURCE_DIR}/include)

# SQLite Library
add_library(sqlite3 STATIC ${CMAKE_SOURCE_DIR}/third_party/sqlite/sqlite3.c)
if(WIN32)
    # Exclude Win32 threading warnings for SQLite if needed, or link dependencies
endif()

# Main application source files
set(SOURCES
    src/main.cpp
    src/ingestion.cpp
    src/storage.cpp
    src/analytics.cpp
    src/forecasting.cpp
    src/api.cpp
)

add_executable(retour_app ${SOURCES})
target_link_libraries(retour_app PRIVATE sqlite3 Threads::Threads)
if(WIN32)
    target_link_libraries(retour_app PRIVATE ws2_32 crypt32)
endif()

# Setup Testing with GoogleTest via FetchContent
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

# Analytics Test
add_executable(retour_test 
    tests/test_analytics.cpp 
    src/analytics.cpp 
    src/storage.cpp
    src/forecasting.cpp
)
target_link_libraries(retour_test PRIVATE sqlite3 Threads::Threads GTest::gtest_main)
if(WIN32)
    target_link_libraries(retour_test PRIVATE ws2_32 crypt32)
endif()

add_test(NAME AnalyticsTest COMMAND retour_test)
