cmake_minimum_required(VERSION 3.11)

project(fourier_analyser)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/findFFTW")

if (WIN32)
    set(FFTW_USE_STATIC_LIBS ON)
endif()
find_package(FFTW COMPONENTS DOUBLE_LIB REQUIRED)
include_directories(${FFTW_INCLUDE_DIRS})
link_directories(${FFTW_LIBRARY_DIRS})

if (WIN32)
    set(Boost_USE_STATIC_LIBS ON)
endif()
find_package(Boost COMPONENTS program_options filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0")

add_executable(fourier_analyser
        src/main.cpp)

target_link_libraries(fourier_analyser LINK_PUBLIC ${Boost_LIBRARIES})
target_link_libraries(fourier_analyser LINK_PUBLIC ${FFTW_LIBRARIES})