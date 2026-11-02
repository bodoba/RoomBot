#!/bin/sh
rsync -avr . --exclude .git --exclude .DS_Store RoomBot:/home/pi/AddOn/RoomBot
ssh RoomBot -x "( cd /home/pi/AddOn/RoomBot/rbd && cmake CMakeLists.txt && make )"