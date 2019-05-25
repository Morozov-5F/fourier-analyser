cmake_minimum_required(VERSION 3.9)

project(fourier_analyser)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/findFFTW")

find_package(FFTW COMPONENTS DOUBLE_LIB REQUIRED)
find_package(Boost COMPONENTS program_options filesystem REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${FFTW_INCLUDE_DIRS})

add_executable(fourier_analyser
        src/main.cpp)

target_link_libraries(fourier_analyser LINK_PUBLIC ${Boost_LIBRARIES})
target_link_libraries(fourier_analyser LINK_PUBLIC ${FFTW_LIBRARIES})