cmake_minimum_required(VERSION 3.16)
PROJECT(TestTrader)

set(CMAKE_CXX_FLAGS "-fPIC")
set(CMAKE_INCLUDE_CURRENT_DIR ON)

SET(AppVersion 0.1.0)
SET(APIVersion "Test 1.0.0")
SET(TradeGateWay_Version ${PROJECT_NAME}_${AppVersion})

# 默认输出debug版
#SET(CMAKE_BUILD_TYPE "Release")
#SET(CMAKE_BUILD_TYPE "debug")
SET(CMAKE_BUILD_TYPE "RelWithDebInfo")
set(CMAKE_CXX_STANDARD 11)

macro(Get_CommitID SO_COMMITID)
    find_package(Git QUIET)     # 查找Git，QUIET静默方式不报错
    if(GIT_FOUND)
      execute_process(          # 执行一个子进程
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD # 命令
        OUTPUT_VARIABLE ${SO_COMMITID}        # 输出字符串存入变量
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
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../../Utils/ # 执行路径
        )
    endif()
endmacro()

set(SO_COMMITID "")
Get_CommitID(SO_COMMITID)
message(STATUS "SoCommitID is ${SO_COMMITID}")

set(SO_UTILS_COMMITID "")
Get_UtilsCommitID(SO_UTILS_COMMITID)
message(STATUS "SoUtilsCommitID is ${SO_UTILS_COMMITID}")

add_definitions(-DSO_COMMITID=\"${SO_COMMITID}\")
add_definitions(-DSO_UTILS_COMMITID=\"${SO_UTILS_COMMITID}\")
add_definitions(-DAPI_VERSION=\"${APIVersion}\")

include_directories(${CMAKE_SOURCE_DIR}/../Utils/)
include_directories(${CMAKE_SOURCE_DIR}/TraderAPI/)
include_directories(${CMAKE_SOURCE_DIR}/)

# SPDLog
include_directories(${CMAKE_SOURCE_DIR}/../XAPI/SPDLog/1.8.5/include/)
link_directories(${CMAKE_SOURCE_DIR}/../XAPI/SPDLog/1.8.5/lib)

set(SOURCES TestTradeGateWay.cpp
            ${CMAKE_SOURCE_DIR}/../Utils/Logger.cpp)
add_library(${TradeGateWay_Version} SHARED ${SOURCES})

target_link_libraries(${TradeGateWay_Version} spdlog)   