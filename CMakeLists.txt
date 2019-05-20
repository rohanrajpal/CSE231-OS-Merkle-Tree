cmake_minimum_required(VERSION 3.13)
project(Assignment4 C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
    set(OPENSSL_USE_STATIC_LIBS TRUE)
endif()


add_executable(Assignment4
#        base.c
        testcase2.c
        filesys.c
        filesys.h
#        test_merkle_tree.c
        )

target_link_libraries(${PROJECT_NAME} OpenSSL::Crypto)
