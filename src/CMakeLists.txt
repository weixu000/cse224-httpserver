cmake_minimum_required(VERSION 3.10)
project(proj1)

set(CMAKE_CXX_STANDARD 11)

find_package(Threads REQUIRED)

add_executable(httpd
        server-main.cc
        utils.cc utils.hpp
        HttpdServer.cc HttpdServer.hpp
        HTTPRequest.cc HTTPRequest.hpp
        HTTPResponse.cc HTTPResponse.hpp
        HTTPHandler.cc HTTPHandler.hpp
        ThreadPool.cc ThreadPool.hpp)
target_compile_options(httpd PRIVATE -Wall -Wextra -pedantic -Werror -Wnon-virtual-dtor)
target_include_directories(httpd PRIVATE ../dependencies/include)
target_link_libraries(httpd PRIVATE Threads::Threads)