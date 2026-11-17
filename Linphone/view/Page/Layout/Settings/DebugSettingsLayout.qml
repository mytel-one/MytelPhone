import QtQuick
import QtQuick.Layouts
import Linphone
import SettingsCpp 1.0
import UtilsCpp 1.0
import "qrc:/qt/qml/Linphone/view/Style/buttonStyle.js" as ButtonStyle

AbstractSettingsLayout {
    id: mainItem
    Layout.fillWidth: true
    Layout.fillHeight: true
    contentModel: [
        { title: "MyTel", subTitle: "", contentComponent: versionContent },
        { title: qsTr("Local diagnostics"), subTitle: qsTr("Keep diagnostic files on this device and share them with MyTel support when needed."), contentComponent: logContent }
    ]
    onSave: SettingsCpp.save()

    Dialog {
        id: deleteLogs
        text: qsTr("settings_debug_clean_logs_message")
        onAccepted: SettingsCpp.cleanLogs()
    }
    Component {
        id: versionContent
        HelpIconLabelButton {
            enabled: false
            title: qsTr("Version")
            subTitle: AppCpp.shortApplicationVersion
            iconSource: AppIcons.appWindow
        }
    }
    Component {
        id: logContent
        ColumnLayout {
            spacing: 20
            SwitchSetting {
                titleText: qsTr("settings_debug_enable_logs_title")
                propertyName: "logsEnabled"
                propertyOwner: SettingsCpp
            }
            SwitchSetting {
                titleText: qsTr("settings_debug_enable_full_logs_title")
                propertyName: "fullLogsEnabled"
                propertyOwner: SettingsCpp
            }
            GridLayout {
                columns: width >= 540 ? 3 : 1
                Layout.fillWidth: true
                columnSpacing: 12
                rowSpacing: 12
                MediumButton {
                    text: qsTr("Open logs folder")
                    style: ButtonStyle.secondary
                    Layout.fillWidth: true
                    Layout.minimumHeight: 44
                    onClicked: UtilsCpp.openNativeDialog(SettingsCpp.logsFolder)
                }
                MediumButton {
                    text: qsTr("Contact MyTel")
                    style: ButtonStyle.secondary
                    Layout.fillWidth: true
                    Layout.minimumHeight: 44
                    onClicked: Qt.openUrlExternally("https://mytel.one/contact/")
                }
                MediumButton {
                    text: qsTr("settings_debug_delete_logs_title")
                    style: ButtonStyle.tertiary
                    Layout.fillWidth: true
                    Layout.minimumHeight: 44
                    onClicked: deleteLogs.open()
                }
            }
        }
    }
}
