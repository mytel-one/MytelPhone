import QtQuick
import QtQuick.Layouts
import Linphone
import 'qrc:/qt/qml/Linphone/view/Style/buttonStyle.js' as ButtonStyle

AbstractSettingsLayout {
    width: parent?.width
    saveButtonVisible: false
    contentModel: [{
        title: qsTr("Appearance"),
        subTitle: qsTr("Choose a comfortable theme. Changes apply immediately."),
        contentComponent: appearanceOptions
    }]
    Component {
        id: appearanceOptions
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 12
            Repeater {
                model: [{name: qsTr("Use system theme"), value: "system"},
                        {name: qsTr("Light"), value: "light"},
                        {name: qsTr("Dark"), value: "dark"}]
                BigButton {
                    required property var modelData
                    Layout.fillWidth: true
                    text: modelData.name
                    style: DefaultStyle.themeMode === modelData.value ? ButtonStyle.main : ButtonStyle.secondary
                    onClicked: DefaultStyle.themeMode = modelData.value
                    Accessible.name: modelData.name
                }
            }
        }
    }
}
