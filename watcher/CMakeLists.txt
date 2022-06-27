cmake_minimum_required(VERSION 3.10)

PROJECT(XWatcher)

set(CMAKE_CXX_FLAGS "-fPIC")
set(CMAKE_INCLUDE_CURRENT_DIR ON)

SET(AppVersion 0.4.0)
SET(XWatcher_Version ${PROJECT_NAME}_${AppVersion})

# 默认输出debug版
#SET(CMAKE_BUILD_TYPE "Release")
#SET(CMAKE_BUILD_TYPE "debug")
SET(CMAKE_BUILD_TYPE "RelWithDebInfo")
set(CMAKE_CXX_STANDARD 11)

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

SET(APIVersion "")

add_definitions(-DAPP_COMMITID=\"${APP_COMMITID}\")
add_definitions(-DUTILS_COMMITID=\"${UTILS_COMMITID}\") 
add_definitions(-DAPI_VERSION=\"${APIVersion}\")

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
            HPPackServer.cpp 
            HPPackClient.cpp
            WatcherEngine.cpp
            ${CMAKE_SOURCE_DIR}/../Utils/Logger.cpp)
add_executable(${XWatcher_Version} ${SOURCES})

target_link_libraries(${XWatcher_Version} spdlog pthread hpsocket4c yaml-cpp)                      
