#ifndef YD_EXAMPLE_H
#define YD_EXAMPLE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WINDOWS
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "ydApi.h"

void startExample1(const char *configFilename,const char *username,const char *password,const char *instrumentID);
void startExample2(const char *configFilename,const char *username,const char *password,const char *instrumentID);
void startExample3(const char *configFilename,const char *username,const char *password,const char *instrumentID);
void startExample4(const char *configFilename,const char *username,const char *password,const char *instrumentID);
void startExample5(const char *configFilename,const char *username,const char *password,const char *instrumentID);
void startExample6(const char *configFilename,const char *username,const char *password,const char *instrumentID);
void startExample7(const char *configFilename,const char *username,const char *password);
void startExample8(const char *configFilename,const char *username,const char *password);

#endif
