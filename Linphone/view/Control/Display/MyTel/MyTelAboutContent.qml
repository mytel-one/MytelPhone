import QtQuick
import QtQuick.Layouts
import Linphone
import "qrc:/qt/qml/Linphone/view/Style/buttonStyle.js" as ButtonStyle

ColumnLayout {
    id: mainItem
    spacing: 20

    RowLayout {
        Layout.fillWidth: true
        spacing: 18
        Image {
            source: "image://internal/mytel_logo.png"
            fillMode: Image.PreserveAspectFit
            Layout.preferredWidth: 64
            Layout.preferredHeight: 64
            Accessible.ignored: true
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4
            Text {
                text: "MyTel"
                font: Typography.h2
                color: DefaultStyle.main2_900
                Layout.fillWidth: true
            }
            Text {
                text: qsTr("Your business phone, wherever you work.")
                font: Typography.p1
                color: DefaultStyle.main2_600
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }
    }
    Text {
        text: qsTr("Calls, messages and your team in one simple workspace. Connect MyTel to your company's phone system and keep your business conversations together.")
        font: Typography.p1
        color: DefaultStyle.main2_600
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
    }
    Text {
        text: qsTr("Version %1").arg(AppCpp.shortApplicationVersion)
        font: Typography.p1
        color: DefaultStyle.main2_600
        Layout.fillWidth: true
    }
    GridLayout {
        columns: width >= 540 ? 2 : 1
        columnSpacing: 12
        rowSpacing: 12
        Layout.fillWidth: true
        BigButton {
            objectName: "mytelWebsiteButton"
            text: qsTr("Visit mytel.one")
            style: ButtonStyle.main
            Layout.fillWidth: true
            Layout.minimumHeight: 44
            onClicked: Qt.openUrlExternally("https://mytel.one/")
        }
        BigButton {
            objectName: "mytelContactButton"
            text: qsTr("Contact MyTel")
            style: ButtonStyle.secondary
            Layout.fillWidth: true
            Layout.minimumHeight: 44
            onClicked: Qt.openUrlExternally("https://mytel.one/contact/")
        }
    }
    GridLayout {
        columns: width >= 540 ? 2 : 1
        columnSpacing: 24
        rowSpacing: 18
        Layout.fillWidth: true
        HelpIconLabelButton {
            title: qsTr("Call MyTel")
            subTitle: "021-91018746"
            iconSource: AppIcons.phone
            Layout.fillWidth: true
            onClicked: Qt.openUrlExternally("tel:+982191018746")
        }
        HelpIconLabelButton {
            title: qsTr("Email MyTel")
            subTitle: "info@mytel.one"
            iconSource: AppIcons.envelope
            Layout.fillWidth: true
            onClicked: Qt.openUrlExternally("mailto:info@mytel.one")
        }
    }
    Text {
        text: "© 2026 MyTel"
        color: DefaultStyle.main2_600
        font: Typography.p1
        Layout.fillWidth: true
    }
    SmallButton {
        objectName: "mytelSoftwareLicensesButton"
        text: qsTr("Software licenses")
        style: ButtonStyle.noBackground
        Layout.minimumHeight: 44
        onClicked: softwareLicenses.open()
    }
    SoftwareLicensesDialog { id: softwareLicenses }
}
