#!/bin/sh

REMOTE_HOST="192.168.100.115"

cd rbd && rm -f rbd Makefile libwiringPiStub.a CMakeCache.txt cmake_install.cmake  && rm -rf CMakeFiles && cd -

rsync -avr . --exclude .git --exclude .DS_Store $REMOTE_HOST:/home/pi/AddOn/RoomBot
ssh $REMOTE_HOST -x "( cd /home/pi/AddOn/RoomBot/rbd && cmake CMakeLists.txt && make )"