cmake_minimum_required(VERSION 3.10)

PROJECT(mysqlite)
set(CMAKE_CXX_STANDARD 20)

ADD_EXECUTABLE(mysqlite main.c page_table.c row.c b-tree.c)
