cmake_minimum_required(VERSION 2.8)
PROJECT(XServer)

set(CMAKE_CXX_FLAGS "-fPIC")
set(CMAKE_INCLUDE_CURRENT_DIR ON)

SET(AppVersion 0.6.0)
SET(XServer_Version ${PROJECT_NAME}_${AppVersion})

# 默认输出debug版
#SET(CMAKE_BUILD_TYPE "Release")
#SET(CMAKE_BUILD_TYPE "debug")
SET(CMAKE_BUILD_TYPE "RelWithDebInfo")
set(CMAKE_CXX_STANDARD 11)

include_directories(${CMAKE_SOURCE_DIR}/../Utils/)

# HP-Scoket
include_directories(${CMAKE_SOURCE_DIR}/../XAPI/HP-Socket/5.8.2/include/)
link_directories(${CMAKE_SOURCE_DIR}/../XAPI/HP-Socket/5.8.2/lib)
# SPDLog
include_directories(${CMAKE_SOURCE_DIR}/../XAPI/SPDLog/1.8.5/include/)
link_directories(${CMAKE_SOURCE_DIR}/../XAPI/SPDLog/1.8.5/lib)
# YAML-CPP
include_directories(${CMAKE_SOURCE_DIR}/../XAPI/YAML-CPP/0.6.3/include/)
link_directories(${CMAKE_SOURCE_DIR}/../XAPI/YAML-CPP/0.6.3/lib)

set(SOURCES main.cpp 
            ServerEngine.cpp
            HPPackServer.cpp
            ${CMAKE_SOURCE_DIR}/../Utils/Logger.cpp)
add_executable(${XServer_Version} ${SOURCES})

target_link_libraries(${XServer_Version} spdlog pthread hpsocket4c yaml-cpp sqlite3)                      
