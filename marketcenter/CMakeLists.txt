cmake_minimum_required(VERSION 3.16)

PROJECT(XMarketCenter)
SET(AppVersion 0.3.0)
SET(XMarketCenter_Version ${PROJECT_NAME}_${AppVersion})

set(CMAKE_CXX_FLAGS "-fPIC")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# 默认输出debug版
#SET(CMAKE_BUILD_TYPE "Release")
#SET(CMAKE_BUILD_TYPE "debug")
SET(CMAKE_BUILD_TYPE "RelWithDebInfo")
set(CMAKE_CXX_STANDARD 11)

include_directories(${CMAKE_SOURCE_DIR}/../Utils/)

macro(Get_CommitID APP_COMMITID)   
    find_package(Git QUIET)     # 查找Git，QUIET静默方式不报错
    if(GIT_FOUND)
      execute_process(          # 执行一个子进程
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD # 命令
        OUTPUT_VARIABLE ${APP_COMMITID}        # 输出字符串存入变量
        OUTPUT_STRIP_TRAILING_WHITESPACE    # 删除字符串尾的换行符
        ERROR_QUIET                         # 对执行错误静默
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} # 执行路径
        )
    endif()
endmacro() 

macro(Get_UtilsCommitID GIT_UTILSCOMMITID)   
    find_package(Git QUIET)     # 查找Git，QUIET静默方式不报错
    if(GIT_FOUND)
      execute_process(          # 执行一个子进程
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD # 命令
        OUTPUT_VARIABLE ${GIT_UTILSCOMMITID}        # 输出字符串存入变量
        OUTPUT_STRIP_TRAILING_WHITESPACE    # 删除字符串尾的换行符
        ERROR_QUIET                         # 对执行错误静默
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../Utils/ # 执行路径
        )
    endif()
endmacro() 

set(APP_COMMITID "")
Get_CommitID(APP_COMMITID)
message(STATUS "AppCommitID is ${APP_COMMITID}")

set(UTILS_COMMITID "")
Get_UtilsCommitID(UTILS_COMMITID)
message(STATUS "UtilsCommitID is ${UTILS_COMMITID}")

SET(APIVersion "CTP 6.5.1")
add_definitions(-DAPP_COMMITID=\"${APP_COMMITID}\")
add_definitions(-DUTILS_COMMITID=\"${UTILS_COMMITID}\") 
add_definitions(-DAPI_VERSION=\"${APIVersion}\")

# HP-Scoket
include_directories(${CMAKE_SOURCE_DIR}/../XAPI/HP-Socket/5.8.2/include/)
link_directories(${CMAKE_SOURCE_DIR}/../XAPI/HP-Socket/5.8.2/lib)

# SPDLog
include_directories(${CMAKE_SOURCE_DIR}/../XAPI/SPDLog/1.8.5/include/)
link_directories(${CMAKE_SOURCE_DIR}/../XAPI/SPDLog/1.8.5/lib)

# YAML-CPP
include_directories(${CMAKE_SOURCE_DIR}/../XAPI/YAML-CPP/0.6.3/include/)
link_directories(${CMAKE_SOURCE_DIR}/../XAPI/YAML-CPP/0.6.3/lib)

# CTP 6.5.1
include_directories(${CMAKE_SOURCE_DIR}/../XAPI/CTP/6.5.1/include)
link_directories(${CMAKE_SOURCE_DIR}/../XAPI/CTP/6.5.1/lib)

set(SOURCES main.cpp 
            HPPackClient.cpp
            XMarketCenter.cpp 
            MarketGateWay/CTPMarketGateWay.cpp
            ${CMAKE_SOURCE_DIR}/../Utils/Logger.cpp)

add_executable(${XMarketCenter_Version} ${SOURCES})

target_link_libraries(${XMarketCenter_Version} thostmduserapi_se yaml-cpp spdlog pthread hpsocket4c)  