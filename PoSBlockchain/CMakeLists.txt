cmake_minimum_required(VERSION 3.3)
project(PoSBlockchain)
set(CMAKE_CXX_STANDARD 11)

link_libraries(ws2_32 wsock32)


set(AWS_SDK_PATH "C:/aws-sdk")
set(aws-sdk-cpp_DIR ${AWS_SDK_PATH})

find_package(aws-sdk-cpp)

set(AWSSDK_LIB_DIR C:/ProgramFilesx86/aws-cpp-sdk-all/lib)
set(AWSSDK_BIN_DIR C:/ProgramFilesx86/aws-cpp-sdk-all/bin)
set(AWSSDK_INCLUDE_DIR C:/ProgramFilesx86/aws-cpp-sdk-all/include)

include_directories(C:/rapidjson-master/include)
include_directories(C:/PicoSHA2-master)
add_executable(PoSBlockchain main.cpp cmake-build-debug/awsfunctions.h)
target_link_libraries(PoSBlockchain aws-cpp-sdk-core aws-cpp-sdk-dynamodb)




