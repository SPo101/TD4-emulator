cmake_minimum_required(VERSION 4.1.2)
project(TD4_cpus)

set(SRC
	src/cpu.cpp
	src/logic.cpp
	src/emu.cpp
)

#Libs for td4m cpu
add_library(libs STATIC ${SRC})
target_include_directories(libs PUBLIC include)
target_compile_options(libs PRIVATE -std=c++11)


#td4m cpu
add_executable(emu_td4m src/advanced_emulator_m.cpp)
target_link_libraries(emu_td4m PUBLIC libs)
target_compile_options(emu_td4m PRIVATE -std=c++11)


#td4 cpu
add_executable(emu_td4 src/advanced_emulator.c)


#compiler for td4 cpu
add_executable(comp src/assembly_compiler.cpp)
target_compile_options(comp PRIVATE -std=c++11)
