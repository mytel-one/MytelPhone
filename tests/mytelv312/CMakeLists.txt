cmake_minimum_required(VERSION 3.21)
project(MytelHelpTests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
get_filename_component(MYTEL_SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)

# Use the same small production-method seam as the existing MyTel tests.
function(extract_production_method file signature output)
    file(READ "${MYTEL_SOURCE_ROOT}/${file}" source)
    string(REPLACE "\r\n" "\n" source "${source}")
    string(FIND "${source}" "${signature}" start)
    if(start LESS 0)
        message(FATAL_ERROR "Production method missing: ${signature}")
    endif()
    string(SUBSTRING "${source}" ${start} -1 method)
    string(FIND "${method}" "\n}\n" finish)
    if(finish LESS 0)
        message(FATAL_ERROR "Production method end missing: ${signature}")
    endif()
    math(EXPR length "${finish} + 3")
    string(SUBSTRING "${method}" 0 ${length} method)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${output}" "${method}")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${MYTEL_SOURCE_ROOT}/${file}")
endfunction()

extract_production_method("Linphone/model/setting/SettingsModel.cpp"
    "bool SettingsModel::autoCheckForUpdateOnStart() const {" "auto_update.inc")
extract_production_method("Linphone/model/setting/SettingsModel.cpp"
    "bool SettingsModel::isCheckForUpdateAvailable() const {" "update_available.inc")
extract_production_method("Linphone/model/setting/SettingsModel.cpp"
    "bool SettingsModel::isCheckForUpdateEnabled() const {" "update_enabled.inc")
extract_production_method("Linphone/model/core/CoreModel.cpp"
    "void CoreModel::checkForUpdate(const std::string &applicationVersion, bool requestedByUser) {" "update_dispatch.inc")

add_executable(mytel-update-disabled-test update_disabled_test.cpp)
target_include_directories(mytel-update-disabled-test PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
enable_testing()
add_test(NAME mytel-update-disabled-test COMMAND mytel-update-disabled-test)
