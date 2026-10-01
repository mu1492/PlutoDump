cmake_minimum_required(VERSION 3.5)

project(PlutoDump LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(PlutoDump main.cpp PlutoSdr.cpp PlutoSdr.h SysMgmt.cpp SysMgmt.h)

set(IIO_LIBRARIES "libiio.so")
target_link_libraries(PlutoDump PRIVATE ${IIO_LIBRARIES})
