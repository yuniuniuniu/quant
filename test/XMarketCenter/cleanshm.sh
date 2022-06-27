#!/bin/bash
ipcs |grep 0xff | awk '{print $1}' | xargs -d '\n' ipcrm -M
