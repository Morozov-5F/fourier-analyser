cmake_minimum_required(VERSION 3.9)

project(fourier_analyser)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

find_package(FFTW REQUIRED)
find_package(Boost COMPONENTS program_options REQUIRED)

add_executable(fourier_analyser
        src/main.cpp)
target_link_libraries(fourier_analyser LINK_PUBLIC ${Boost_LIBRARIES})
