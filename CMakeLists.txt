cmake_minimum_required(VERSION 3.17)
project(cppProjectPublisher)

set(CMAKE_CXX_STANDARD 14)

add_executable(cppProjectPublisher main.cpp ledMicroserviceSubscriber.cpp)