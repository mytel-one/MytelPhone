import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic as Control
import Linphone
import "qrc:/qt/qml/Linphone/view/Style/buttonStyle.js" as ButtonStyle

FocusScope {
    id: mainItem
    signal goBack()

    Control.StackView {
        id: helpStack
        anchors.fill: parent
        initialItem: aboutPage
    }
    Component {
        id: aboutPage
        ColumnLayout {
            spacing: 20
            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 32
                Layout.rightMargin: 32
                Layout.topMargin: 16
                spacing: 12
                RoundButton {
                    icon.source: AppIcons.leftArrow
                    style: ButtonStyle.noBackground
                    Accessible.name: qsTr("Back")
                    onClicked: mainItem.goBack()
                }
                Text {
                    text: qsTr("About MyTel")
                    font: Typography.h2
                    color: DefaultStyle.main2_900
                    Layout.fillWidth: true
                }
            }
            Control.ScrollView {
                id: aboutScroll
                Layout.fillWidth: true
                Layout.fillHeight: true
                contentWidth: availableWidth
                contentHeight: aboutColumn.implicitHeight + 32
                clip: true
                ColumnLayout {
                    id: aboutColumn
                    width: Math.min(760, Math.max(0, aboutScroll.availableWidth - 64))
                    x: (aboutScroll.availableWidth - width) / 2
                    spacing: 20
                    RoundedPane {
                        Layout.fillWidth: true
                        padding: 28
                        contentItem: MyTelAboutContent {}
                    }
                    HelpIconLabelButton {
                        title: qsTr("Diagnostics")
                        subTitle: qsTr("View local logs or contact MyTel for help.")
                        iconSource: AppIcons.debug
                        arrowImageVisible: true
                        Layout.fillWidth: true
                        Layout.leftMargin: 12
                        Layout.rightMargin: 12
                        onClicked: helpStack.push("qrc:/qt/qml/Linphone/view/Page/Layout/Settings/DebugSettingsLayout.qml", {
                            titleText: qsTr("MyTel diagnostics"), container: helpStack
                        })
                    }
                }
            }
        }
    }
}
