cmake_minimum_required(VERSION 3.5)

project(TicTacToe LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(TicTacToe game.h agent.h minmax_agent.h qvalues_agent.h main.cpp)
