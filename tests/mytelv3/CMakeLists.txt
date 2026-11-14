cmake_minimum_required(VERSION 3.21)
project(MytelV3Tests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)
add_executable(mytel-theme-test theme_test.cpp)
target_link_libraries(mytel-theme-test PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick)
get_filename_component(MYTEL_SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
target_compile_definitions(mytel-theme-test PRIVATE MYTEL_SOURCE_ROOT="${MYTEL_SOURCE_ROOT}")
qt_add_resources(mytel-theme-test "mytel-style"
    PREFIX "/qt/qml/Linphone/view/Style"
    BASE "${MYTEL_SOURCE_ROOT}/Linphone/view/Style"
    FILES "${MYTEL_SOURCE_ROOT}/Linphone/view/Style/buttonStyle.js")
enable_testing()
add_test(NAME mytel-theme-test COMMAND mytel-theme-test)
add_executable(mytel-login-attempt-test login_attempt_test.cpp)
target_include_directories(mytel-login-attempt-test PRIVATE "${MYTEL_SOURCE_ROOT}/Linphone")
add_test(NAME mytel-login-attempt-test COMMAND mytel-login-attempt-test)
