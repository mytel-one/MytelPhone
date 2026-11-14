cmake_minimum_required(VERSION 3.22)
project(MyTelDirectoryRegression LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
find_package(Qt6 6.10 REQUIRED COMPONENTS Core Network)
add_executable(mytel-directory-test
    test_main.cpp
    ../../Linphone/core/search/ServerDirectoryManager.cpp
    ../../Linphone/core/search/ServerDirectoryManager.hpp)
# Replace only the SDK/config boundary; exercise the shipped directory manager.
target_include_directories(mytel-directory-test PRIVATE stubs ../../Linphone)
target_link_libraries(mytel-directory-test PRIVATE Qt6::Core Qt6::Network)
enable_testing()
add_test(NAME directory-regressions COMMAND mytel-directory-test)
