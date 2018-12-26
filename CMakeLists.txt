cmake_minimum_required(VERSION 3.12)
project(Hust_Base_kernel)

set(CMAKE_CXX_STANDARD 14)

add_executable(Hust_Base_kernel main.cpp RM_Manager.cpp RM_Manager.h RC.h str.h Bit_tools.cpp Bit_tools.h IX_Manager.cpp IX_Manager.h SYS_Manager.cpp SYS_Manager.h Condition_tool.cpp Condition_tool.h QU_Manager.cpp QU_Manager.h PF_Manager_No_Buff.cpp PF_Manager_No_Buff.h)