#ifndef USERDBMANAGER_HPP
#define USERDBMANAGER_HPP

#include "Singleton.hpp"
#include "Util.hpp"
#include "Logger.h"
#include "YMLConfig.hpp"
#include "SQLiteManager.hpp"

class UserDBManager
{
    friend class Utils::Singleton<UserDBManager>;
public:
    bool LoadDataBase(const std::string& dbPath, std::string& errorString)
    {
        m_DBManager = Utils::Singleton<Utils::SQLiteManager>::GetInstance();
        bool ret = m_DBManager->LoadDataBase(dbPath, errorString);
        if(!ret)
        {
            Utils::gLogger->Log->warn("UserDBManager::LoadDataBase failed, {}", errorString.c_str());
        }
        return ret;
    }

    bool UpdateUserPermissionTable(const std::string& sql, const std::string& op, sqlite3_callback cb, std::string& errorString)
    {
        errorString.clear();
        char errorBuffer[256] = {0};
        bool ret = m_DBManager->Execute(sql, cb, op.c_str(), errorString);
        if(!ret)
        {
            sprintf(errorBuffer, "ErrorMsg:%s, SQL:%s", errorString.c_str(), sql.c_str());
            Utils::gLogger->Log->warn("UserDBManager::UpdateUserPermissionTable failed, ErrorMsg:{} sql:{}",
                                      errorString.c_str(), sql.c_str());
        }
        else
        {
            sprintf(errorBuffer, "SQL: %s", sql.c_str());
            Utils::gLogger->Log->info("UserDBManager::UpdateUserPermissionTable successed, sql:{}", sql.c_str());
        }
        errorString = errorBuffer;
        return ret;
    }

    bool QueryUserPermission(sqlite3_callback cb, std::string& errorString)
    {
        std::string SQL_SELECT_USER_PERMISSION = "SELECT * FROM UserPermissionTable;";
        bool ret = m_DBManager->Execute(SQL_SELECT_USER_PERMISSION, cb, "SELECT", errorString);
        if(!ret)
        {
            Utils::gLogger->Log->warn("UserDBManager::Select UserPermissionTable failed, {}", errorString.c_str());
        }
        return ret;
    }

    bool UpdateAppStatusTable(const std::string& sql, const std::string& op, sqlite3_callback cb, std::string& errorString)
    {
        errorString.clear();
        char errorBuffer[256] = {0};
        bool ret = m_DBManager->Execute(sql, cb, op.c_str(), errorString);
        if(!ret)
        {
            sprintf(errorBuffer, "ErrorMsg: %s, SQL: %s", errorString.c_str(), sql.c_str());
            Utils::gLogger->Log->warn("UserDBManager::UpdateAppStatusTable failed, ErrorMsg:{} sql:{}",
                                      errorString.c_str(), sql.c_str());
        }
        else
        {
            sprintf(errorBuffer, "SQL: %s", sql.c_str());
            Utils::gLogger->Log->info("UserDBManager::UpdateAppStatusTable successed, sql:{}", sql.c_str());
        }
        errorString = errorBuffer;
        return ret;
    }

    bool QueryAppStatus(sqlite3_callback cb, std::string& errorString)
    {
        std::string SQL_SELECT_APP_STATUS = "SELECT * FROM AppStatusTable;";
        bool ret = m_DBManager->Execute(SQL_SELECT_APP_STATUS, cb, "SELECT", errorString);
        if(!ret)
        {
            Utils::gLogger->Log->warn("UserDBManager::QueryAppStatus failed, {}", errorString.c_str());
        }
        return ret;
    }
private:
    UserDBManager() {}
    UserDBManager &operator=(const UserDBManager&);
    UserDBManager(const UserDBManager&);
private:
    Utils::SQLiteManager* m_DBManager;
};


#endif // USERDBMANAGER_HPP