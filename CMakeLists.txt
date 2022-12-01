cmake_minimum_required(VERSION 3.0)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

project(oslearn
	VERSION 22.11.09.01
	# LANGUAGES ASM_NASM C CXX 
	DESCRIPTION "OS 学习 项目"
)

# 禁止直接在源码目录编译, 要放在 project指令之后
if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
	message(FATAL_ERROR "You cannot build in a source directory (or any directory with a  CMakeLists.txt file). Please make a build subdirectory. Feel free to remove CMakeCache.txt and CMakeFiles.")
endif()

include(CMakePrintHelpers)	# cmake_print_variables

include_directories(include)
# add_compile_definitions(-I ${CMAKE_CURRENT_SOURCE_DIR}/include/)
# 输出路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# aux_source_directory(. SRC)
set(SRC a.asm b.asm)
add_executable(abc ${SRC})
set_target_properties(abc PROPERTIES LINKER_LANGUAGE NASM)

add_subdirectory(boot)
