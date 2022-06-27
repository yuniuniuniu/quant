#ifndef RISKDBMANAGER_HPP
#define RISKDBMANAGER_HPP

#include "Singleton.hpp"
#include "Util.hpp"
#include "Logger.h"
#include "YMLConfig.hpp"
#include "SQLiteManager.hpp"

class RiskDBManager
{
    friend class Utils::Singleton<RiskDBManager>;
public:
    bool LoadDataBase(const std::string& dbPath, std::string& errorString)
    {
        m_DBManager = Utils::Singleton<Utils::SQLiteManager>::GetInstance();
        bool ret = m_DBManager->LoadDataBase(dbPath, errorString);
        if(!ret)
        {
            Utils::gLogger->Log->warn("RiskDBManager::LoadDataBase failed, {}", errorString.c_str());
        }
        return ret;
    }

    bool UpdateCancelledCountTable(const std::string& sql, const std::string& op, sqlite3_callback cb, std::string& errorString)
    {
        errorString.clear();
        char errorBuffer[256] = {0};
        bool ret = m_DBManager->Execute(sql, cb, op.c_str(), errorString);
        if(!ret)
        {
            sprintf(errorBuffer, "ErrorMsg: %s, SQL: %s", errorString.c_str(), sql.c_str());
            Utils::gLogger->Log->warn("RiskDBManager::UpdateCancelledCountTable failed, CancelledCountTable failed, ErrorMsg:{} sql:{}",
                                      errorString.c_str(), sql.c_str());
        }
        else
        {
            sprintf(errorBuffer, "SQL: %s", sql.c_str());
            Utils::gLogger->Log->info("RiskDBManager::UpdateCancelledCountTable successed, sql:{}", sql.c_str());
        }
        errorString = errorBuffer;
        return ret;
    }

    bool UpdateLockedAccountTable(const std::string& sql, const std::string& op, sqlite3_callback cb, std::string& errorString)
    {
        errorString.clear();
        char errorBuffer[256] = {0};
        bool ret = m_DBManager->Execute(sql, cb, op, errorString);
        if(!ret)
        {
            Utils::gLogger->Log->warn("RiskDBManager::UpdateLockedAccountTable failed, ErrorMsg:{} sql:{}", errorString.c_str(), sql.c_str());
            sprintf(errorBuffer, "ErrorMsg:%s SQL:%s", errorString.c_str(), sql.c_str());
        }
        else
        {
            Utils::gLogger->Log->info("RiskDBManager::UpdateLockedAccountTable successed, sql:{}", sql.c_str());
            sprintf(errorBuffer, "SQL:%s", sql.c_str());
        }
        errorString = errorBuffer;
        return ret;
    }

    bool UpdateRiskLimitTable(const std::string& sql, const std::string& op, sqlite3_callback cb, std::string& errorString)
    {
        errorString.clear();
        char errorBuffer[256] = {0};
        bool ret = m_DBManager->Execute(sql, cb, op, errorString);
        if(!ret)
        {
            Utils::gLogger->Log->warn("RiskDBManager::UpdateRiskLimitTable failed, ErrorMsg:{} sql:{}", errorString.c_str(), sql.c_str());
            sprintf(errorBuffer, "ErrorMsg:%s SQL:%s", errorString.c_str(), sql.c_str());
        }
        else
        {
            Utils::gLogger->Log->info("RiskDBManager::UpdateRiskLimitTable successed, sql:{}", sql.c_str());
            sprintf(errorBuffer, "SQL:%s", sql.c_str());
        }
        errorString = errorBuffer;
        return ret;
    }

    bool QueryRiskLimit(sqlite3_callback cb, std::string& errorString)
    {
        std::string SQL_SELECT_RISK_LIMIT = "SELECT * FROM RiskLimitTable;";
        bool ret = m_DBManager->Execute(SQL_SELECT_RISK_LIMIT, cb, "SELECT", errorString);
        if(!ret)
        {
            Utils::gLogger->Log->warn("RiskDBManager::Select RiskLimitTable failed, {}", errorString.c_str());
        }
        return ret;
    }

    bool QueryLockedAccount(sqlite3_callback cb, std::string& errorString)
    {
        std::string SQL_SELECT_LOCKED_ACCOUNT = "SELECT * FROM LockedAccountTable;";
        bool ret = m_DBManager->Execute(SQL_SELECT_LOCKED_ACCOUNT, cb, "SELECT", errorString);
        if(!ret)
        {
            Utils::gLogger->Log->warn("RiskDBManager::Select LockedAccountTable failed, {}", errorString.c_str());
        }
        return ret;
    }

    bool QueryCancelledCount(sqlite3_callback cb, std::string& errorString)
    {
        std::string SQL_SELECT_TICKER_LIMIT = "SELECT * FROM CancelledCountTable;";
        bool ret = m_DBManager->Execute(SQL_SELECT_TICKER_LIMIT, cb, "SELECT", errorString);
        if(!ret)
        {
            Utils::gLogger->Log->warn("RiskDBManager::Select CancelledCountTable failed, {}", errorString.c_str());
        }
        return ret;
    }

private:
    RiskDBManager() {}
    RiskDBManager &operator=(const RiskDBManager&);
    RiskDBManager(const RiskDBManager&);
private:
    Utils::SQLiteManager* m_DBManager;
};


#endif // RISKDBMANAGER_HPP