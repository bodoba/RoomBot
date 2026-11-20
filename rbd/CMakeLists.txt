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

add_executable(rbd rbd.c logging.c daemon.c mqtt.c sensors.c serial.c commands.c signals.c remotecontrol.c json.c watchdog.c)

pkg_check_modules(CJSON QUIET IMPORTED_TARGET libcjson)
if(NOT CJSON_FOUND)
    pkg_check_modules(CJSON QUIET IMPORTED_TARGET cjson)
endif()

if(CJSON_FOUND)
    target_include_directories(rbd PRIVATE ${CJSON_INCLUDE_DIRS})
    target_link_libraries(rbd PRIVATE PkgConfig::CJSON)
else()
    if(APPLE)
        find_program(HOMEBREW_EXECUTABLE brew)
        if(HOMEBREW_EXECUTABLE)
            execute_process(
                COMMAND "${HOMEBREW_EXECUTABLE}" --prefix cjson
                OUTPUT_VARIABLE CJSON_BREW_PREFIX
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET)
            if(CJSON_BREW_PREFIX)
                list(APPEND CMAKE_PREFIX_PATH "${CJSON_BREW_PREFIX}")
            endif()
        endif()
    endif()

    find_path(CJSON_INCLUDE_DIR NAMES cjson/cJSON.h)
    find_library(CJSON_LIBRARY NAMES cjson)

    if(NOT CJSON_INCLUDE_DIR)
        message(FATAL_ERROR "cJSON headers not found. Install cjson and ensure its include directory is discoverable.")
    endif()

    target_include_directories(rbd PRIVATE "${CJSON_INCLUDE_DIR}")
    if(CJSON_LIBRARY)
        target_link_libraries(rbd PRIVATE "${CJSON_LIBRARY}")
    endif()
endif()

option(ROOMBOT_USE_WIRINGPI_STUB "Build with the no-op WiringPi implementation" OFF)

if(APPLE)
    add_library(wiringPiStub STATIC ../wiringPi_stub/wiringPi.c)
    target_include_directories(wiringPiStub PUBLIC ../wiringPi_stub)
    set(WIRINGPI_LIBRARY wiringPiStub)
    set(WIRINGPI_INCLUDE_DIR ../wiringPi_stub)
else()
    find_path(WIRINGPI_INCLUDE_DIR NAMES wiringPi.h)
    find_library(WIRINGPI_LIBRARY NAMES wiringPi)

    if(NOT WIRINGPI_INCLUDE_DIR OR NOT WIRINGPI_LIBRARY)
        message(FATAL_ERROR "wiringPi not found. Install it or enable ROOMBOT_USE_WIRINGPI_STUB.")
    endif()
endif()

target_include_directories(rbd PRIVATE "${WIRINGPI_INCLUDE_DIR}")
target_link_libraries(rbd PRIVATE "${WIRINGPI_LIBRARY}" PkgConfig::MOSQUITTO)