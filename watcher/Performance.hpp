#ifndef PERFORMANCE_HPP
#define PERFORMANCE_HPP

#include <stdio.h>
#include <vector>
#include <string>
#include <string.h>
#include <stdlib.h>
#include "Util.hpp"
#include "PackMessage.hpp"
#include "ShellEngine.hpp"

class Performance
{
public:
    static void UpdateColoStatus(const std::string& Mount1, const std::string& Mount2, Message::TColoStatus& ColoStatus)
    {
        UpdateOSVersion(ColoStatus); 
        UpdateKernelVersion(ColoStatus);
        UpdateLoadAverage(ColoStatus.LoadAverage);
        UpdateCPUUsage(ColoStatus.CPUUsage);
        UpdateMemoryInfo(ColoStatus.MemoryInfo);
        UpdateDiskInfo(Mount1, Mount2, ColoStatus.DiskInfo);
        strncpy(ColoStatus.UpdateTime, Utils::getCurrentTimeUs(), sizeof(ColoStatus.UpdateTime));
    }

    static void UpdateOSVersion(Message::TColoStatus& ColoStatus)
    {
        std::vector<std::string> result;
        const char* cmd = "lsb_release -a";
        int ret = ShellEngine::ExecuteShellCommand(cmd, result);
        if(ret > 0)
        {
            std::string line = result.at(2);
            std::vector<std::string> vec;
            Utils::Split(line, " ", vec);
            if(vec.size() == 3)
            {
                std::string line = vec[0] + " " + vec[1];
                std::string pattern = "Ubuntu";
                int pos = line.find(pattern);
                if (std::string::npos != pos)
                {
                    std::string OSVersion = line.substr(pos);
                    strncpy(ColoStatus.OSVersion, OSVersion.c_str(), sizeof(ColoStatus.OSVersion));
                }          
            }
        }
    }

    static void UpdateKernelVersion(Message::TColoStatus& ColoStatus)
    {
        std::vector<std::string> result;
        const char* cmd = "uname -r";
        int ret = ShellEngine::ExecuteShellCommand(cmd, result);
        if(ret > 0)
        {
            //  3.10.0-1160.45.1.el7.x86_64
            strncpy(ColoStatus.KernelVersion, result.at(0).c_str(), sizeof(ColoStatus.KernelVersion));
        }
    }

    static void UpdateLoadAverage(Message::TLoadAverage& LoadAverage)
    {
        std::vector<std::string> result;
        const char* cmd = "uptime";
        int ret = ShellEngine::ExecuteShellCommand(cmd, result);
        if(ret > 0)
        {
            std::string line = result.at(0);
            //  17:56:29 up 12 days,  1:49,  6 users,  load average: 10.00, 9.99, 9.96
            std::string pattern = "load average:";
            int pos = line.find(pattern);
            if(std::string::npos != pos)
            {
                std::string data = line.substr(pos + pattern.length());
                std::vector<std::string> vec;
                Utils::Split(data, ",", vec);
                if(vec.size() == 3)
                {
                    LoadAverage.Min1 = atof(vec.at(0).c_str());
                    LoadAverage.Min5 = atof(vec.at(1).c_str());
                    LoadAverage.Min15 = atof(vec.at(2).c_str());
                }
            }
        }
        const char* cmd1 = "cat /proc/cpuinfo|grep 'processor'|wc -l";
        ret = ShellEngine::ExecuteShellCommand(cmd1, result);
        if(ret > 0)
        {
            std::string line = result.at(0);
            LoadAverage.CPUS = atoi(line.c_str());
        }
    }

    static void UpdateCPUUsage(Message::TCPUUsage& CPUUsage)
    {
        std::vector<std::string> result;
        const char* cmd = "top -bn 1";
        int ret = ShellEngine::ExecuteShellCommand(cmd, result);
        if(ret > 0)
        {
            std::string line = result.at(2);
            // %Cpu(s): 27.5 us,  0.9 sy,  0.0 ni, 71.6 id,  0.0 wa,  0.0 hi,  0.0 si,  0.0 st
            std::string pattern = "%Cpu(s):";
            int pos = line.find(pattern);
            if(std::string::npos != pos)
            {
                std::string data = line.substr(pos + pattern.length());
                std::vector<std::string> vec;
                Utils::Split(data, ",", vec);
                if(vec.size() == 8)
                {
                    std::string UserItem = vec.at(0);
                    std::vector<std::string> UserItemVec;
                    Utils::Split(UserItem, " ", UserItemVec);
                    CPUUsage.UserRate = atof(UserItemVec.at(0).c_str()) / 100;

                    std::string SysItem = vec.at(1);
                    std::vector<std::string> SysItemVec;
                    Utils::Split(SysItem, " ", SysItemVec);
                    CPUUsage.SysRate = atof(SysItemVec.at(0).c_str()) / 100;

                    std::string IdleItem = vec.at(3);
                    std::vector<std::string> IdleItemVec;
                    Utils::Split(IdleItem, " ", IdleItemVec);
                    CPUUsage.IdleRate = atof(IdleItemVec.at(0).c_str()) / 100;

                    std::string IOWaitItem = vec.at(4);
                    std::vector<std::string> IOWaitItemVec;
                    Utils::Split(IOWaitItem, " ", IOWaitItemVec);
                    CPUUsage.IOWaitRate = atof(IOWaitItemVec.at(0).c_str()) / 100;

                    std::string IrqItem = vec.at(5);
                    std::vector<std::string> IrqItemVec;
                    Utils::Split(IrqItem, " ", IrqItemVec);
                    CPUUsage.IrqRate = atof(IrqItemVec.at(0).c_str()) / 100;

                    std::string SoftIrqItem = vec.at(6);
                    std::vector<std::string> SoftIrqItemVec;
                    Utils::Split(SoftIrqItem, " ", SoftIrqItemVec);
                    CPUUsage.SoftIrqRate = atof(SoftIrqItemVec.at(0).c_str()) / 100;

                    CPUUsage.UsedRate = 1 - CPUUsage.IdleRate;
                }
            }
        }
    }

    static void UpdateMemoryInfo(Message::TMemoryInfo& MemoryInfo)
    {
        std::vector<std::string> result;
        const char* cmd = "free -m";
        int ret = ShellEngine::ExecuteShellCommand(cmd, result);
        if(ret > 0)
        {
            std::string line = result.at(1);
            //               total        used        free      shared  buff/cache   available
            // Mem:          63721        5514       27630         891       30576       56830
            std::string pattern = "Mem:";
            int pos = line.find(pattern);
            if(std::string::npos != pos)
            {
                std::string data = line.substr(pos + pattern.length());
                std::vector<std::string> vec;
                Utils::Split(data, " ", vec);
                if(vec.size() == 6)
                {
                    int total = atoi(vec.at(0).c_str());
                    int used = atoi(vec.at(1).c_str());
                    int free = atoi(vec.at(2).c_str());
                    MemoryInfo.Total = total / 1024 + 1;
                    MemoryInfo.Free = free / 1024.0;
                    MemoryInfo.UsedRate = 1.0 * used / total;
                }
            }
        }
    }

    static void UpdateDiskInfo(const std::string& Mount1, const std::string& Mount2, Message::TDiskInfo& DiskInfo)
    {
        std::vector<std::string> result;
        std::string cmd = "df -m";
        int ret = ShellEngine::ExecuteShellCommand(cmd.c_str(), result);
        // Filesystem              1M-blocks  Used Available Use% Mounted on
        // devtmpfs                    15802     0     15802   0% /dev
        // tmpfs                       15814     0     15814   0% /dev/shm
        // tmpfs                       15814    27     15788   1% /run
        // tmpfs                       15814     0     15814   0% /sys/fs/cgroup
        // /dev/mapper/centos-root    511750  7121    504630   2% /
        // /dev/sda2                     497   154       344  31% /boot
        // /dev/sda1                     300    12       289   4% /boot/efi
        // /dev/mapper/centos-home   1363007   356   1362652   1% /home
        // tmpfs                        3163     0      3163   0% /run/user/1000
        if(ret > 0)
        {
            int total = 0;
            int used = 0;
            for(int i = 1; i < result.size(); i++)
            {
                std::string line = result.at(i);
                std::vector<std::string> vec;
                Utils::Split(line, " ", vec);
                if(vec.size() == 6)
                {
                    total += atoi(vec.at(1).c_str());
                    used += atoi(vec.at(2).c_str());
                }
            }
            DiskInfo.Total = total / 1024 + 1;
            DiskInfo.Free = (total - used) / 1024.0;
            DiskInfo.UsedRate = 1.0 * used / total;
        }

        cmd = "df -m " + Mount1;
        result.clear();
        ret = ShellEngine::ExecuteShellCommand(cmd.c_str(), result);
        if(ret > 0)
        {
            int total = 0;
            int used = 0;
            for(int i = 1; i < result.size(); i++)
            {
                std::string line = result.at(i);
                std::vector<std::string> vec;
                Utils::Split(line, " ", vec);
                if(vec.size() == 6)
                {
                    total += atoi(vec.at(1).c_str());
                    used += atoi(vec.at(2).c_str());
                }
            }
            DiskInfo.Mount1UsedRate = 1.0 * used / total;
        }

        cmd = "df -m " + Mount2;
        result.clear();
        ret = ShellEngine::ExecuteShellCommand(cmd.c_str(), result);
        if(ret > 0)
        {
            int total = 0;
            int used = 0;
            for(int i = 1; i < result.size(); i++)
            {
                std::string line = result.at(i);
                std::vector<std::string> vec;
                Utils::Split(line, " ", vec);
                if(vec.size() == 6)
                {
                    total += atoi(vec.at(1).c_str());
                    used += atoi(vec.at(2).c_str());
                }
            }
            DiskInfo.Mount2UsedRate = 1.0 * used / total;
        }
    }
    static void PrintColoStatus(const Message::TColoStatus& ColoStatus)
    {
        printf("Colo: %s\n", ColoStatus.Colo);
        printf("OSVersion: %s\n", ColoStatus.OSVersion);
        printf("KernelVersion: %s\n", ColoStatus.KernelVersion);

        printf("CPUS: %d\n", ColoStatus.LoadAverage.CPUS);
        printf("Load Min1: %.2f\n", ColoStatus.LoadAverage.Min1);
        printf("Load Min5: %.2f\n", ColoStatus.LoadAverage.Min5);
        printf("Load Min15: %.2f\n", ColoStatus.LoadAverage.Min15);

        printf("CPU UserRate: %.2f%\n", ColoStatus.CPUUsage.UserRate * 100);
        printf("CPU SysRate: %.2f%\n", ColoStatus.CPUUsage.SysRate * 100);
        printf("CPU IdleRate: %.2f%\n", ColoStatus.CPUUsage.IdleRate * 100);
        printf("CPU IOWaitRate: %.2f%\n", ColoStatus.CPUUsage.IOWaitRate * 100);
        printf("CPU IrqRate: %.2f%\n", ColoStatus.CPUUsage.IrqRate * 100);
        printf("CPU SoftIrqRate: %.2f%\n", ColoStatus.CPUUsage.SoftIrqRate * 100);
        printf("CPU UsedRate: %.2f%\n", ColoStatus.CPUUsage.UsedRate * 100);

        printf("Memory Total: %.2f GB\n", ColoStatus.MemoryInfo.Total);
        printf("Memory Free: %.2f GB\n", ColoStatus.MemoryInfo.Free);
        printf("Memory UsedRate: %.2f%\n", ColoStatus.MemoryInfo.UsedRate * 100);

        printf("Disk Total: %.2f GB\n", ColoStatus.DiskInfo.Total);
        printf("Disk Free: %.2f GB\n", ColoStatus.DiskInfo.Free);
        printf("Disk UsedRate: %.2f%\n", ColoStatus.DiskInfo.UsedRate * 100);
        printf("Disk Mount1UsedRate: %.2f%\n", ColoStatus.DiskInfo.Mount1UsedRate * 100);
        printf("Disk Mount2UsedRate: %.2f%\n", ColoStatus.DiskInfo.Mount2UsedRate * 100);
        printf("UpdateTime: %s\n", ColoStatus.UpdateTime);
    }
};

#endif // PERFORMANCE_HPP