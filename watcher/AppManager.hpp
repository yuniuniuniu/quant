#ifndef APPMANAGER_HPP
#define APPMANAGER_HPP

#include <stdlib.h>
#include "ShellEngine.hpp"

class AppManager
{
public:
    static void UpdateAppStatus(const std::string& cmd, Message::TAppStatus& AppStatus)
    {
        std::vector<std::string> ItemVec;
        Utils::Split(cmd, " ", ItemVec);
        std::string Account;
        for(int i = 0; i < ItemVec.size(); i++)
        {
            if(Utils::equalWith(ItemVec.at(i), "-a"))
            {
                Account = ItemVec.at(i + 1);
                break;
            }
        }
        strncpy(AppStatus.Account, Account.c_str(), sizeof(AppStatus.Account));

        std::vector<std::string> Vec;
        Utils::Split(ItemVec.at(0), "/", Vec);
        std::string AppName = Vec.at(Vec.size() - 1);
        strncpy(AppStatus.AppName, AppName.c_str(), sizeof(AppStatus.AppName));
        AppStatus.PID = getpid();
        strncpy(AppStatus.Status, "Start", sizeof(AppStatus.Status));

        char command[512] = {0};
        std::string AppLogPath;
        char* p = getenv("APP_LOG_PATH");
        if(p == NULL)
        {
            AppLogPath = "./log/";
        }
        else
        {
            AppLogPath = p;
        }
        sprintf(command, "nohup %s > %s/%s_%s_run.log 2>&1 &", cmd.c_str(), AppLogPath.c_str(), 
        AppName.c_str(), AppStatus.Account);
        strncpy(AppStatus.StartScript, command, sizeof(AppStatus.StartScript));
        strncpy(AppStatus.CommitID, APP_COMMITID, sizeof(AppStatus.CommitID));
        strncpy(AppStatus.UtilsCommitID, UTILS_COMMITID, sizeof(AppStatus.UtilsCommitID));
        strncpy(AppStatus.APIVersion, API_VERSION, sizeof(AppStatus.APIVersion));
        strncpy(AppStatus.StartTime, Utils::getCurrentTimeUs(), sizeof(AppStatus.StartTime));
        strncpy(AppStatus.LastStartTime, Utils::getCurrentTimeUs(), sizeof(AppStatus.LastStartTime));
        strncpy(AppStatus.UpdateTime, Utils::getCurrentTimeUs(), sizeof(AppStatus.UpdateTime));
    }

    static void UpdateAppStatus(Message::TAppStatus& AppStatus)
    {
        std::string Status;
        char cmd[128] = {0};
        sprintf(cmd, "ps aux | grep %s | grep %d | grep -v grep", AppStatus.AppName, AppStatus.PID);
        std::vector<std::string> result;
        int ret = ShellEngine::ExecuteShellCommand(cmd, result);
        // USER       PID %CPU %MEM    VSZ   RSS TTY      STAT START   TIME COMMAND
        // xtrader  13444 92.9  0.0 388132  4724 pts/2    Sl   00:51  10:08 /home/xtrader/Test/XWatcher/XWatcher_1.0.0 -d -f /home/xtrader/Test/XWatcher/XWatcher.yml
        if(ret > 0)
        {
            std::vector<std::string> ItemVec;
            Utils::Split(result.at(0), " ", ItemVec);
            AppStatus.UsedMemSize = atof(ItemVec.at(4).c_str()) / 1024;
            AppStatus.UsedCPURate = atof(ItemVec.at(2).c_str()) / 100;
            if(Utils::startWith(ItemVec.at(7), "S"))
            {
                Status = "S:Running";
            }
            else if(Utils::startWith(ItemVec.at(7), "D"))
            {
                Status = "D:Running";
            }
            else if(Utils::startWith(ItemVec.at(7), "R"))
            {
                Status = "R:Running";
            }
            else if(Utils::startWith(ItemVec.at(7), "Z"))
            {
                Status = "Z:Zombie";
            }
            else if(Utils::startWith(ItemVec.at(7), "T"))
            {
                Status = "T:Stoped";
            }
            else
            {
                Status = ItemVec.at(7) + ":Unkown";
            }
            strncpy(AppStatus.UpdateTime, Utils::getCurrentTimeUs(), sizeof(AppStatus.UpdateTime));
        }
        else
        {
            Status = "X:Stoped";
        }
        strncpy(AppStatus.Status, Status.c_str(), sizeof(AppStatus.Status));
    }

    static void KillApp(const std::string& cmd)
    {
        ShellEngine::ExecuteProcess(cmd.c_str());
    }
    static void StartApp(const std::string& cmd)
    {
        ShellEngine::ExecuteProcess(cmd.c_str());
    }

    static void PrintAppStatus(const Message::TAppStatus& AppStatus)
    {
        printf("Colo: %s\n", AppStatus.Colo);
        printf("Account: %s\n", AppStatus.Account);
        printf("AppName: %s\n", AppStatus.AppName);

        printf("PID: %d\n", AppStatus.PID);
        printf("Status: %s\n", AppStatus.Status);
        printf("UsedCPURate: %.2f%\n", AppStatus.UsedCPURate * 100);
        printf("UsedMemSize: %.2f MB\n", AppStatus.UsedMemSize);
        printf("StartTime: %s\n", AppStatus.StartTime);
        printf("LastStartTime: %s\n", AppStatus.LastStartTime);
        printf("CommitID: %s\n", AppStatus.CommitID);
        printf("UtilsCommitID: %s\n", AppStatus.UtilsCommitID);
        printf("APIVersion: %s\n", AppStatus.APIVersion);
        printf("StartScript: %s\n", AppStatus.StartScript);
        printf("UpdateTime: %s\n", AppStatus.UpdateTime);
    }
};

#endif // APPMANAGER_HPP
