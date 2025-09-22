cmake_minimum_required(VERSION 3.13.1)

# by uchan
set(DTC_OVERLAY_FILE "boards/led1.overlay")

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(nucleo-f446re-zephyr-threads)

FILE(GLOB app_sources ../src/*.c*)
target_sources(app PRIVATE ${app_sources})
