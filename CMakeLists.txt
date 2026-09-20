cmake_minimum_required(VERSION 4.2)
project(mJNI)

set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE mJNI  "mJNI/*.cpp" "mJNI/*.hpp" "mJNI/*.h")

add_executable(mJNI main.cpp ${mJNI} )
