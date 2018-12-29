cmake_minimum_required(VERSION 3.12)
project(Hust_Base_kernel)

set(CMAKE_CXX_STANDARD 14)

add_executable(Hust_Base_kernel yacc_sql.y yacc_sql.tab.c yacc_sql.tab.h lex_sql.l parse.c parse.h lex.yy.c main.cpp RM_Manager.cpp RM_Manager.h RC.h str.h Bit_tools.cpp Bit_tools.h IX_Manager.cpp IX_Manager.h SYS_Manager.cpp SYS_Manager.h Condition_tool.cpp Condition_tool.h QU_Manager.cpp QU_Manager.h PF_Manager.cpp PF_Manager.h)