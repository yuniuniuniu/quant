#!/bin/bash
echo "CREATE TABLE IF NOT EXISTS UserPermissionTable(ID INTEGER PRIMARY KEY AUTOINCREMENT, UserName CHAR(16) NOT NULL, PassWord CHAR(16), Role CHAR(32) NOT NULL, Plugins CHAR(320) NOT NULL, Messages CHAR(160) NOT NULL, UpdateTime CHAR(32));" | sqlite3 XServer.db
echo "INSERT INTO UserPermissionTable(UserName, PassWord, Role, Plugins, Messages, UpdateTime) VALUES ('admin', '123456', 'Admin', '', '', '2021-07-21 13:25:45.235641');" | sqlite3 XServer.db
echo "CREATE TABLE IF NOT EXISTS AppStatusTable(ID INTEGER PRIMARY KEY AUTOINCREMENT, Colo CHAR(16) NOT NULL, AppName CHAR(32) NOT NULL, Account CHAR(16), PID CHAR(16), Status CHAR(16) NOT NULL, UpdateTime CHAR(32));" | sqlite3 XServer.db
echo "INSERT INTO AppStatusTable(Colo, AppName, Account, PID, Status, UpdateTime) VALUES ('XServer', 'XMarketCenter_1.0.0', 'XMarketCenter', '256', 'S:Running', '');" | sqlite3 XServer.db
