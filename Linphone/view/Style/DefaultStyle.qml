pragma Singleton
import QtQuick
import QtCore
import Linphone
import SettingsCpp

QtObject {
    property Settings appearance: Settings {
        id: appearanceSettings
        category: "Appearance"
        property string theme: "system"
    }
    property alias themeMode: appearanceSettings.theme
    readonly property bool dark: themeMode === "dark"
                                || (themeMode === "system" && Qt.styleHints.colorScheme === Qt.Dark)
    readonly property color surface: dark ? "#142030" : "#FFFFFF"
    readonly property color background: dark ? "#0D1623" : "#F5F8FC"

    // Media surfaces stay dark so white call labels and controls remain
    // readable over avatars/video in either application appearance.
    readonly property color callBackground: "#0D1623"
    readonly property color callSurface: "#22334D"
    readonly property color callControlSurface: "#334861"
    readonly property color callControlHovered: "#496480"
    readonly property color callControlDisabled: "#26364A"
    readonly property color callForeground: "#F5F8FD"


	property var currentTheme: Themes.themes.hasOwnProperty(SettingsCpp.themeMainColor)
							  ? Themes.themes[SettingsCpp.themeMainColor]
							  : Themes.themes["mytel"]
    property var main1_100: dark ? "#173356" : currentTheme.main100
    property var main1_200: dark ? "#24476F" : currentTheme.main200
    property var main1_300: dark ? "#39628D" : currentTheme.main300
    property var main1_500_main: currentTheme.main500
    property var main1_600: currentTheme.main600
    property var main1_700: currentTheme.main700

    property var main2_0: dark ? "#101C2B" : "#FAFEFF"
    property var main2_100: dark ? "#1C2C40" : "#EEF6F8"
    property var main2_200: dark ? "#293C54" : "#DFECF2"
    property var main2_300: dark ? "#425873" : "#C0D1D9"
    property var main2_400: dark ? "#8E9FB5" : "#9AABB5"
    property var main2_500_main: dark ? "#A1B4CD" : "#6C7A87"
    property var main2_600: dark ? "#C6D4E4" : "#4E6074"
    property var main2_700: dark ? "#E1E9F4" : "#364860"
    property var main2_800: dark ? "#E9F0F9" : "#22334D"
    property var main2_900: dark ? "#F3F7FD" : "#2D3648"

    property var grey_0: "#FFFFFF"
    property var grey_100: dark ? "#101A28" : "#F9F9F9"
    property var grey_200: dark ? "#253448" : "#EDEDED"
    property var grey_300: dark ? "#40526A" : "#C9C9C9"
    property var grey_400: dark ? "#8DA0B8" : "#949494"
    property var grey_500: dark ? "#BECCDD" : "#4E4E4E"
    property var grey_600: dark ? "#DAE4EF" : "#2E3030"
    property var grey_850: dark ? "#35465D" : "#D9D9D9"
    property var grey_900: dark ? "#F2F6FB" : "#070707"
    property var grey_1000: "#000000"

    property var warning_600: "#DBB820"
    property var warning_700: "#AF9308"
    property var danger_500_main: "#DD5F5F"
    property var warning_500_main: "#FFDC2E"
    property var danger_700: "#9E3548"
    property var danger_900: "#723333"
    property var success_500_main: "#4FAE80"
    property var success_700: "#377d71"
    property var success_900: "#1E4C53"
    property var info_500_main: "#4AA8FF"
    property var info_800_main: dark ? "#8BCBFF" : "#02528D"

    property var vue_meter_light_green: "#6FF88D"
    property var vue_meter_dark_green: "#00D916"

    property var account_status_green: "#2A7050"
    property var account_status_background_green: "#E4F4EC"
    property var account_status_yellow: "#776208"
    property var account_status_background_yellow: "#F8F1D4"
    property var account_status_red: "#B03A3A"
    property var account_status_background_red: "#FBE9E9"
    property var account_status_grey: "#37475A"
    property var account_status_background_grey: "#E9EDF1"

    property real defaultHeight: 1007.0
    property real defaultWidth: 1512.0
    property real maxDp: 0.98
    // Qt already scales logical pixels for each monitor. Screen aspect ratio
    // must not change button sizes, font weights or window minimum sizes.
    property real dp: 1.0

    onDpChanged: {
        console.log("Screen ratio changed", dp)
        AppCpp.setScreenRatio(dp)
    }

    // Warning: Qt 6.8.1 (current version) and previous versions, Qt only support COLRv0 fonts. Don't try to use v1.
    property string emojiFont: "Noto Color Emoji"
    property string flagFont: "Noto Color Emoji"
    property string defaultFont: "Noto Sans"

    property var numericPadPressedButtonColor: dark ? "#293C54" : "#EEF7F8"

    property var groupCallButtonColor: dark ? "#1C2C40" : "#EEF7F8"

    property var placeholders: dark ? "#8E9FB5" : '#CACACA'	// No name in design

}
