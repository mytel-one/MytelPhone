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

function(extract_production_method file signature output)
    file(READ "${MYTEL_SOURCE_ROOT}/${file}" source)
    string(FIND "${source}" "${signature}" start)
    if(start LESS 0)
        message(FATAL_ERROR "Production test method missing: ${signature}")
    endif()
    string(SUBSTRING "${source}" ${start} -1 method)
    string(FIND "${method}" "\n}\n" finish)
    math(EXPR length "${finish} + 3")
    string(SUBSTRING "${method}" 0 ${length} method)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${output}" "${method}")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${MYTEL_SOURCE_ROOT}/${file}")
endfunction()
extract_production_method("Linphone/core/friend/FriendCore.cpp"
    "QList<QVariant> FriendCore::getAllAddresses() const {" "friend_production.inc")
extract_production_method("Linphone/tool/Utils.cpp"
    "QString Utils::getUsername(const QString &address) {" "username_production.inc")
add_executable(mytel-friend-addresses-test friend_addresses_test.cpp)
target_link_libraries(mytel-friend-addresses-test PRIVATE Qt6::Core)
target_include_directories(mytel-friend-addresses-test PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
add_test(NAME mytel-friend-addresses-test COMMAND mytel-friend-addresses-test)

extract_production_method("Linphone/core/friend/FriendCore.cpp"
    "void FriendCore::removeAddress(int index) {" "remove_address_production.inc")
extract_production_method("Linphone/core/friend/FriendCore.cpp"
    "void FriendCore::removePhoneNumber(int index) {" "remove_phone_production.inc")
add_executable(mytel-friend-removal-test friend_removal_test.cpp)
target_link_libraries(mytel-friend-removal-test PRIVATE Qt6::Core)
target_include_directories(mytel-friend-removal-test PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
add_test(NAME mytel-friend-removal-test COMMAND mytel-friend-removal-test)

add_executable(mytel-chat-layout-test chat_layout_test.cpp)
target_link_libraries(mytel-chat-layout-test PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick)
target_compile_definitions(mytel-chat-layout-test PRIVATE MYTEL_SOURCE_ROOT="${MYTEL_SOURCE_ROOT}")
add_test(NAME mytel-chat-layout-test COMMAND mytel-chat-layout-test)
