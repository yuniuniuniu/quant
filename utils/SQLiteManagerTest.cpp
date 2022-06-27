#include "SQLiteManager.hpp"
#include <stdio.h>


int main(int argc, char* argv[])
{
    Utils::SQLiteManager* sqliteManager = Utils::Singleton<Utils::SQLiteManager>::GetInstance();
    std::string errorString;
    if(sqliteManager->LoadDataBase("test.db", errorString))
    {
        printf("LoadDataBase successed.\n");
    }
    else
    {
        printf("LoadDataBase %s\n", errorString.c_str());
    }
    // Create Table
    std::string CREATE_TABLE = "CREATE TABLE IF NOT EXISTS RiskLimit(RiskID CHAR(32) PRIMARY KEY NOT NULL,\
                                                    FlowLimit  INT NOT NULL,\
                                                    TickerCancelledLimit INT NOT NULL,\
                                                    OrderCancelLimit INT NOT NULL);";
    if(sqliteManager->Execute(CREATE_TABLE, &Utils::SQLiteManager::callback, "CREATE", errorString))
    {
        printf("CREATE_TABLE Execute successed.\n");
    }
    else
    {
        printf("CREATE_TABLE %s\n", errorString.c_str());
    }
    // Insert
    std::string INSERT = "INSERT INTO RiskLimit(RiskID, FlowLimit, TickerCancelledLimit, OrderCancelLimit) VALUES ('Risk1',100, 500, 5);";
    if(sqliteManager->Execute(INSERT, &Utils::SQLiteManager::callback, "INSERT", errorString))
    {
        printf("INSERT Execute successed.\n");
    }
    else
    {
        printf("INSERT %s\n", errorString.c_str());
    }
    // Select
    std::string SELECT = "SELECT * FROM RiskLimit;";
    if(sqliteManager->Execute(SELECT, &Utils::SQLiteManager::callback, "SELECT", errorString))
    {
        printf("SELECT Execute successed.\n");
    }
    else
    {
        printf("SELECT %s\n", errorString.c_str());
    }
    // Update
    std::string UPDATE = "UPDATE RiskLimit SET TickerCancelledLimit = 450 WHERE RiskID='Risk1';";
    if(sqliteManager->Execute(UPDATE, &Utils::SQLiteManager::callback, "UPDATE", errorString))
    {
        printf("UPDATE Execute successed.\n");
    }
    else
    {
        printf("UPDATE %s\n", errorString.c_str());
    }
    sqliteManager->CloseDataBase(errorString);

    return 0;
}

// g++ SQLiteManagerTest.cpp -o test -lsqlite3