#ifndef SQLITEMANAGER_HPP
#define SQLITEMANAGER_HPP
#include <stdio.h>
#include <sqlite3.h>
#include <string>
#include <unordered_map>
#include "Singleton.hpp"

namespace Utils
{
typedef int (*sqlite3_callback)(
    void*,    /* Data provided in the 4th argument of sqlite3_exec() */
    int,      /* The number of columns in row */
    char**,   /* An array of strings representing fields in the row */
    char**    /* An array of strings representing column names */
);

class SQLiteManager
{
    friend Utils::Singleton<SQLiteManager>;
public:
    bool LoadDataBase(const std::string& dbPath, std::string& errorString)
    {
        bool ok = true;
        errorString.clear();
        m_DBConnection = NULL;
        try
        {
            int ret = sqlite3_open(dbPath.c_str(), &m_DBConnection);
            if(SQLITE_OK != ret)
            {
                errorString = std::string("SQLiteManager::LoadDataBase ") + dbPath + " failed, ErrorMsg: " + sqlite3_errmsg(m_DBConnection);
                ok = false;
            }
            else
            {
                errorString = std::string("SQLiteManager::LoadDataBase ") + dbPath + " successed";
            }
        }
        catch(const std::exception& e)
        {
            ok = false;
            errorString =  e.what();
        }

        return ok;
    }
    static int callback(void *data, int argc, char **argv, char **azColName)
    {
        int i;
        for(i = 0; i < argc; i++)
        {
            printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
        }
        return 0;
    }
    bool Execute(const std::string& sql, sqlite3_callback cb, const std::string& op, std::string& errorString)
    {
        errorString.clear();
        bool ok = true;
        char* buffer = NULL;
        try
        {
            int ret = sqlite3_exec(m_DBConnection, sql.c_str(), cb, (void*)(op.c_str()), &buffer);
            if(SQLITE_OK != ret)
            {
                ok = false;
                errorString = buffer;
            }
        }
        catch(const std::exception& e)
        {
            ok = false;
            errorString = e.what();
        }
        return ok;
    }

    bool CloseDataBase(std::string& errorString)
    {
        errorString.clear();
        bool ok = true;
        try
        {
            int ret = sqlite3_close(m_DBConnection);
            if(SQLITE_OK != ret)
            {
                ok = false;
                errorString = sqlite3_errmsg(m_DBConnection);
            }
        }
        catch(const std::exception& e)
        {
            errorString = e.what();
            ok = false;
        }
        return ok;
    }
private:
    SQLiteManager() {}
    SQLiteManager &operator=(const SQLiteManager &);
    SQLiteManager(const SQLiteManager &);

private:
    sqlite3* m_DBConnection;
};

}

#endif // SQLITEMANAGER_HPP