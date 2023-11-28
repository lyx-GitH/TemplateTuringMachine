cmake_minimum_required(VERSION 3.22)
project(TemplateTuringMachine)

set(CMAKE_CXX_STANDARD 17)

add_executable(TemplateTuringMachine main.cpp TuringMachine.h rules.h)
