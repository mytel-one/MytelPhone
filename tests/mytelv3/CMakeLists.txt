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
file(READ "${MYTEL_SOURCE_ROOT}/Linphone/main.cpp" mytel_main)
string(REGEX MATCH "QGuiApplication::setApplicationDisplayName\\(APPLICATION_NAME\\);" mytel_display_setter "${mytel_main}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/display_name_production.inc" "${mytel_display_setter}\n")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${MYTEL_SOURCE_ROOT}/Linphone/main.cpp")
add_executable(mytel-display-name-test display_name_test.cpp)
target_link_libraries(mytel-display-name-test PRIVATE Qt6::Core Qt6::Gui)
target_include_directories(mytel-display-name-test PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
add_test(NAME mytel-display-name-test COMMAND mytel-display-name-test)
add_executable(mytel-combo-palette-test combo_palette_test.cpp)
target_link_libraries(mytel-combo-palette-test PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick)
target_compile_definitions(mytel-combo-palette-test PRIVATE MYTEL_SOURCE_ROOT="${MYTEL_SOURCE_ROOT}")
qt_add_resources(mytel-combo-palette-test "mytel-combo-helpers"
    PREFIX "/qt/qml/Linphone/view/Control/Tool/Helper"
    BASE "${MYTEL_SOURCE_ROOT}/Linphone/view/Control/Tool/Helper"
    FILES "${MYTEL_SOURCE_ROOT}/Linphone/view/Control/Tool/Helper/utils.js")
add_test(NAME mytel-combo-palette-test COMMAND mytel-combo-palette-test)
find_package(Qt6 REQUIRED COMPONENTS Test)
add_executable(mytel-secondary-contrast-test secondary_contrast_test.cpp)
target_link_libraries(mytel-secondary-contrast-test PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::Test)
target_compile_definitions(mytel-secondary-contrast-test PRIVATE MYTEL_SOURCE_ROOT="${MYTEL_SOURCE_ROOT}")
qt_add_resources(mytel-secondary-contrast-test "mytel-secondary-helpers"
    PREFIX "/qt/qml/Linphone/view/Control/Tool/Helper"
    BASE "${MYTEL_SOURCE_ROOT}/Linphone/view/Control/Tool/Helper"
    FILES "${MYTEL_SOURCE_ROOT}/Linphone/view/Control/Tool/Helper/utils.js")
qt_add_resources(mytel-secondary-contrast-test "mytel-secondary-style"
    PREFIX "/qt/qml/Linphone/view/Style"
    BASE "${MYTEL_SOURCE_ROOT}/Linphone/view/Style"
    FILES "${MYTEL_SOURCE_ROOT}/Linphone/view/Style/buttonStyle.js")
add_test(NAME mytel-secondary-contrast-test COMMAND mytel-secondary-contrast-test)
find_package(Qt6 REQUIRED COMPONENTS Svg)
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
extract_production_method("Linphone/tool/Utils.cpp"
    "QString Utils::getIconAsPng(const QString &imagePath, const QSize &size) {" "icon_cache_production.inc")
add_executable(mytel-icon-cache-test icon_cache_test.cpp)
target_link_libraries(mytel-icon-cache-test PRIVATE Qt6::Core Qt6::Gui Qt6::Svg)
target_include_directories(mytel-icon-cache-test PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_definitions(mytel-icon-cache-test PRIVATE MYTEL_SOURCE_ROOT="${MYTEL_SOURCE_ROOT}")
add_test(NAME mytel-icon-cache-test COMMAND mytel-icon-cache-test)
if(WIN32)
    enable_language(RC)
    function(add_mytel_resource_fixture)
        set(CMAKE_CURRENT_SOURCE_DIR "${MYTEL_SOURCE_ROOT}/Linphone")
        set(LINPHONE_MAJOR_VERSION 3)
        set(LINPHONE_MINOR_VERSION 0)
        set(LINPHONE_MICRO_VERSION 1)
        set(LINPHONEAPP_SHORT_VERSION "3.0.1")
        set(APPLICATION_NAME "MyTel")
        set(APPLICATION_VENDOR "MyTel")
        set(APPLICATION_DESCRIPTION "SIP phone")
        set(EXECUTABLE_NAME "mytellphone")
        set(COPYRIGHT_RANGE_DATE "2010-2026")
        configure_file("${MYTEL_SOURCE_ROOT}/cmake/install/windows/appDetailsWindows.rc.in"
            "${CMAKE_CURRENT_BINARY_DIR}/mytel-resource-fixture.rc")
        add_executable(mytel-resource-fixture "${MYTEL_SOURCE_ROOT}/tests/mytelv3/resource_fixture.cpp"
            "${CMAKE_CURRENT_BINARY_DIR}/mytel-resource-fixture.rc")
    endfunction()
    add_mytel_resource_fixture()
    extract_production_method("Linphone/core/notifier/DesktopNotificationManagerCompat.cpp"
        "HRESULT RegisterAumidInRegistry(const wchar_t *aumid, const wchar_t *iconPath) {" "notification_brand_production.inc")
    add_executable(mytel-notification-brand-test notification_brand_test.cpp)
    target_link_libraries(mytel-notification-brand-test PRIVATE advapi32)
    target_include_directories(mytel-notification-brand-test PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
    add_test(NAME mytel-notification-brand-test COMMAND mytel-notification-brand-test)
endif()
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

extract_production_method("Linphone/tool/Utils.cpp"
    "void Utils::forceInputLeftToRight(QObject *input) {" "dial_input_production.inc")
add_executable(mytel-dial-input-test dial_input_test.cpp)
target_link_libraries(mytel-dial-input-test PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick)
target_include_directories(mytel-dial-input-test PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
# The installed Qt package does not export GuiPrivate. These versioned headers
# are a test-only keyboard seam; the application has no private Qt dependency.
get_target_property(qt_gui_includes Qt6::Gui INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(qt_core_includes Qt6::Core INTERFACE_INCLUDE_DIRECTORIES)
list(GET qt_gui_includes 0 qt_gui_include)
list(GET qt_core_includes 0 qt_core_include)
target_include_directories(mytel-dial-input-test PRIVATE
    "${qt_gui_include}/${Qt6Gui_VERSION}" "${qt_gui_include}/${Qt6Gui_VERSION}/QtGui"
    "${qt_core_include}/${Qt6Core_VERSION}" "${qt_core_include}/${Qt6Core_VERSION}/QtCore")
add_test(NAME mytel-dial-input-test COMMAND mytel-dial-input-test)

add_executable(mytel-tab-layout-test tab_layout_test.cpp)
target_link_libraries(mytel-tab-layout-test PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick)
target_compile_definitions(mytel-tab-layout-test PRIVATE MYTEL_SOURCE_ROOT="${MYTEL_SOURCE_ROOT}")
add_test(NAME mytel-tab-layout-test COMMAND mytel-tab-layout-test)

extract_production_method("Linphone/tool/Utils.cpp"
    "void Utils::openCallsWindow(CallGui *call) {" "incoming_open_production.inc")
extract_production_method("Linphone/tool/Utils.cpp"
    "void Utils::showCallsWindow(QQuickWindow *window) {" "incoming_show_production.inc")
extract_production_method("Linphone/core/App.cpp"
    "void App::closeCallsWindow() {" "incoming_close_production.inc")
extract_production_method("Linphone/core/notifier/Notifier.cpp"
    "void Notifier::notifyReceivedCall(const shared_ptr<linphone::Call> &call) {" "incoming_full_production.inc")
file(READ "${MYTEL_SOURCE_ROOT}/Linphone/core/notifier/Notifier.cpp" notifier_source)
file(READ "${CMAKE_CURRENT_BINARY_DIR}/incoming_full_production.inc" incoming_method)
if(MYTEL_NOTIFIER_TEST_SOURCE)
    file(READ "${MYTEL_NOTIFIER_TEST_SOURCE}" notifier_source)
    string(FIND "${notifier_source}" "void Notifier::notifyReceivedCall(" method_start)
    string(SUBSTRING "${notifier_source}" ${method_start} -1 incoming_method)
endif()
string(FIND "${notifier_source}" "#define CREATE_NOTIFICATION" macro_start)
string(SUBSTRING "${notifier_source}" ${macro_start} -1 macro_source)
string(FIND "${macro_source}" "createNotification(TYPE, DATA);" macro_end)
string(LENGTH "createNotification(TYPE, DATA);" macro_suffix_length)
math(EXPR macro_length "${macro_end} + ${macro_suffix_length}")
string(SUBSTRING "${macro_source}" 0 ${macro_length} macro_source)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/incoming_macro_production.inc" "${macro_source}\n")
string(FIND "${incoming_method}" "App::postCoreAsync(" callback_start)
string(SUBSTRING "${incoming_method}" 0 ${callback_start} account_guard)
string(FIND "${account_guard}" "getNotificationsAllowed()" account_guard_start)
string(FIND "${account_guard}" "return;" account_guard_return)
if(account_guard_start LESS 0 OR account_guard_return LESS account_guard_start)
    message(FATAL_ERROR "Incoming GUI harness requires the unchanged account early-return boundary")
endif()
string(SUBSTRING "${incoming_method}" ${callback_start} -1 callback)
string(FIND "${callback}" "\n\t});" callback_end)
math(EXPR callback_length "${callback_end} + 6")
string(SUBSTRING "${callback}" 0 ${callback_length} callback)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/incoming_callback_production.inc" "${callback}\n")
add_executable(mytel-incoming-call-test incoming_call_test.cpp)
target_link_libraries(mytel-incoming-call-test PRIVATE Qt6::Core Qt6::Gui Qt6::Quick)
target_include_directories(mytel-incoming-call-test PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
add_test(NAME mytel-incoming-call-test COMMAND mytel-incoming-call-test)

add_executable(mytel-ux-v310-test "${MYTEL_SOURCE_ROOT}/tests/mytelv310/ux_render_test.cpp")
target_link_libraries(mytel-ux-v310-test PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::Test Qt6::Svg)
target_compile_definitions(mytel-ux-v310-test PRIVATE MYTEL_SOURCE_ROOT="${MYTEL_SOURCE_ROOT}")
qt_add_resources(mytel-ux-v310-test "mytel-ux-helpers"
    PREFIX "/qt/qml/Linphone/view/Control/Tool/Helper"
    BASE "${MYTEL_SOURCE_ROOT}/Linphone/view/Control/Tool/Helper"
    FILES "${MYTEL_SOURCE_ROOT}/Linphone/view/Control/Tool/Helper/utils.js")
qt_add_resources(mytel-ux-v310-test "mytel-ux-style"
    PREFIX "/qt/qml/Linphone/view/Style"
    BASE "${MYTEL_SOURCE_ROOT}/Linphone/view/Style"
    FILES "${MYTEL_SOURCE_ROOT}/Linphone/view/Style/buttonStyle.js")
add_test(NAME mytel-ux-v310-test COMMAND mytel-ux-v310-test)

add_executable(mytel-predial-v310-test "${MYTEL_SOURCE_ROOT}/tests/mytelv310/predial_popup_test.cpp")
target_link_libraries(mytel-predial-v310-test PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::Test Qt6::Svg)
target_compile_definitions(mytel-predial-v310-test PRIVATE MYTEL_SOURCE_ROOT="${MYTEL_SOURCE_ROOT}")
qt_add_resources(mytel-predial-v310-test "mytel-predial-helpers"
    PREFIX "/qt/qml/Linphone/view/Control/Tool/Helper"
    BASE "${MYTEL_SOURCE_ROOT}/Linphone/view/Control/Tool/Helper"
    FILES "${MYTEL_SOURCE_ROOT}/Linphone/view/Control/Tool/Helper/utils.js")
qt_add_resources(mytel-predial-v310-test "mytel-predial-style"
    PREFIX "/qt/qml/Linphone/view/Style"
    BASE "${MYTEL_SOURCE_ROOT}/Linphone/view/Style"
    FILES "${MYTEL_SOURCE_ROOT}/Linphone/view/Style/buttonStyle.js")
add_test(NAME mytel-predial-v310-test COMMAND mytel-predial-v310-test)

if(MYTEL_TEST_SDK_ROOT)
    add_executable(mytel-setup-address-test "${MYTEL_SOURCE_ROOT}/tests/mytelv310/setup_address_test.cpp")
    target_link_libraries(mytel-setup-address-test PRIVATE Qt6::Core
        "${MYTEL_TEST_SDK_ROOT}/lib/liblinphone++.lib" "${MYTEL_TEST_SDK_ROOT}/lib/liblinphone.lib")
    target_include_directories(mytel-setup-address-test PRIVATE "${MYTEL_SOURCE_ROOT}/Linphone"
        "${MYTEL_TEST_SDK_ROOT}/include")
    add_test(NAME mytel-setup-address-test COMMAND mytel-setup-address-test)
    add_executable(mytel-login-auth-test "${MYTEL_SOURCE_ROOT}/tests/mytelv310/login_auth_test.cpp")
    target_link_libraries(mytel-login-auth-test PRIVATE Qt6::Core Qt6::Gui
        "${MYTEL_TEST_SDK_ROOT}/lib/liblinphone++.lib" "${MYTEL_TEST_SDK_ROOT}/lib/liblinphone.lib")
    target_include_directories(mytel-login-auth-test PRIVATE "${MYTEL_SOURCE_ROOT}/Linphone"
        "${MYTEL_TEST_SDK_ROOT}/include")
    add_test(NAME mytel-login-auth-test COMMAND mytel-login-auth-test)
    find_package(Qt6 REQUIRED COMPONENTS Network)
    extract_production_method("Linphone/model/account/AccountModel.cpp"
        "void AccountModel::removeAccount(" "pending_model_remove.inc")
    extract_production_method("Linphone/model/account/AccountManager.cpp"
        "AccountManager::~AccountManager() {" "pending_manager_destructor.inc")
    extract_production_method("Linphone/model/account/AccountManager.cpp"
        "void AccountManager::cancelLogin() {" "pending_manager_cancel.inc")
    add_executable(mytel-pending-account-test "${MYTEL_SOURCE_ROOT}/tests/mytelv310/pending_account_test.cpp")
    target_link_libraries(mytel-pending-account-test PRIVATE Qt6::Core Qt6::Gui Qt6::Network
        "${MYTEL_TEST_SDK_ROOT}/lib/liblinphone++.lib" "${MYTEL_TEST_SDK_ROOT}/lib/liblinphone.lib")
    target_include_directories(mytel-pending-account-test PRIVATE "${MYTEL_SOURCE_ROOT}/Linphone"
        "${MYTEL_TEST_SDK_ROOT}/include" "${CMAKE_CURRENT_BINARY_DIR}")
    add_test(NAME mytel-pending-account-test COMMAND mytel-pending-account-test)
    add_executable(mytel-chat-identity-test "${MYTEL_SOURCE_ROOT}/tests/mytelv310/chat_identity_test.cpp")
    target_link_libraries(mytel-chat-identity-test PRIVATE Qt6::Core Qt6::Gui Qt6::Network
        "${MYTEL_TEST_SDK_ROOT}/lib/liblinphone++.lib" "${MYTEL_TEST_SDK_ROOT}/lib/liblinphone.lib")
    target_include_directories(mytel-chat-identity-test PRIVATE "${MYTEL_SOURCE_ROOT}/Linphone" "${MYTEL_TEST_SDK_ROOT}/include")
    file(TO_CMAKE_PATH "${MYTEL_TEST_SDK_ROOT}" MYTEL_CHAT_TEST_SDK)
    target_compile_definitions(mytel-chat-identity-test PRIVATE MYTEL_PROBE_SDK="${MYTEL_CHAT_TEST_SDK}")
    add_test(NAME mytel-chat-identity-test COMMAND mytel-chat-identity-test)
endif()
