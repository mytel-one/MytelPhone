import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic as Control
import Linphone
import "qrc:/qt/qml/Linphone/view/Style/buttonStyle.js" as ButtonStyle

Dialog {
    id: mainItem
    parent: Control.Overlay.overlay
    width: Math.min(760, parent ? parent.width - 40 : 760)
    title: qsTr("Software licenses")
    property string gplText: ""
    property string lgplText: ""

    function loadLicense(url, propertyName) {
        const request = new XMLHttpRequest()
        request.onreadystatechange = function() {
            if (request.readyState === XMLHttpRequest.DONE) {
                mainItem[propertyName] = (request.status === 200 || request.status === 0) && request.responseText.length > 0
                    ? request.responseText
                    : qsTr("The full license text is included with the installation package. Contact MyTel if you need another copy.")
            }
        }
        request.open("GET", url)
        request.send()
    }
    Component.onCompleted: {
        loadLicense("qrc:/data/licenses/GPL-3.0.txt", "gplText")
        loadLicense("qrc:/data/licenses/LGPL-3.0.txt", "lgplText")
    }
    content: Control.ScrollView {
        Layout.preferredWidth: mainItem.width - mainItem.leftPadding - mainItem.rightPadding
        Layout.preferredHeight: Math.max(160, Math.min(420, mainItem.parent ? mainItem.parent.height - 240 : 420))
        contentWidth: availableWidth
        contentHeight: legalText.implicitHeight
        clip: true
        Control.TextArea {
            id: legalText
            width: parent.width
            height: implicitHeight
            readOnly: true
            selectByMouse: true
            textFormat: TextEdit.PlainText
            wrapMode: TextEdit.Wrap
            color: DefaultStyle.main2_900
            font: Typography.p1
            background: Rectangle { color: DefaultStyle.surface }
            text: "MyTel branding, design and modifications: © 2026 MyTel.\n\n"
                + "This application includes Linphone Desktop and related components.\nCopyright (c) 2010-2024 Belledonne Communications SARL.\n\n"
                + "Qt libraries: © The Qt Company Ltd. and other contributors.\nThese components retain their own GNU GPL/LGPL and third-party license notices.\n\n"
                + "This program is free software. You may redistribute and modify it under the GNU General Public License, version 3 or later. It is distributed WITHOUT ANY WARRANTY, including without implied warranties of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n"
                + "MyTel application source and corresponding-source information:\nhttps://github.com/mytel-one/MytelPhone/tree/windows-v3\nFor source or license-copy requests: https://mytel.one/contact/\n\n"
                + mainItem.gplText + "\n\n" + mainItem.lgplText
        }
    }
    buttons: [
        MediumButton {
            text: qsTr("Source code")
            style: ButtonStyle.secondary
            onClicked: Qt.openUrlExternally("https://github.com/mytel-one/MytelPhone/tree/windows-v3")
        },
        MediumButton {
            text: qsTr("Close")
            style: ButtonStyle.main
            onClicked: mainItem.close()
        }
    ]
}
