# *********************************************************************************** #
#                                                                                     #
#  Copyright (c) 2018 by Bodo Bauer <bb@bb-zone.com>                                  #
#                                                                                     #
#  This program is free software: you can redistribute it and/or modify               #
#  it under the terms of the GNU General Public License as published by               #
#  the Free Software Foundation, either version 3 of the License, or                  #
#  (at your option) any later version.                                                #
#                                                                                     #
#  This program is distributed in the hope that it will be useful,                    #
#  but WITHOUT ANY WARRANTY; without even the implied warranty of                     #
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                      #
#  GNU General Public License for more details.                                       #
#                                                                                     #
#  You should have received a copy of the GNU General Public License                  #
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.              #
# *********************************************************************************** #
include(CheckIncludeFile)
cmake_minimum_required(VERSION 3.6)
find_package(PkgConfig REQUIRED)

project (RoomBot C)

set (CMAKE_C_FLAGS "-std=gnu11 -Wall")

pkg_check_modules(MOSQUITTO REQUIRED IMPORTED_TARGET libmosquitto)

add_executable(rbd rbd.c logging.c daemon.c mqtt.c sensors.c serial.c commands.c signals.c)

find_path(WIRINGPI_INCLUDE_DIR NAMES wiringPi.h)
find_library(WIRINGPI_LIBRARY NAMES wiringPi)

if(NOT WIRINGPI_INCLUDE_DIR OR NOT WIRINGPI_LIBRARY)
    message(FATAL_ERROR "wiringPi not found. Install its headers and library on the target system.")
endif()

target_include_directories(rbd PRIVATE "${WIRINGPI_INCLUDE_DIR}")
target_link_libraries(rbd PRIVATE "${WIRINGPI_LIBRARY}" PkgConfig::MOSQUITTO)