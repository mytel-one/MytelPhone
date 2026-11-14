import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic as Control
import Linphone
import SettingsCpp
import 'qrc:/qt/qml/Linphone/view/Style/buttonStyle.js' as ButtonStyle

Rectangle {
    id: mainItem
    signal goBack()
    signal goToRegister()
    property bool showBackButton: false
    property bool advanced: false
    readonly property bool connecting: LoginPageCpp.registrationState === LinphoneEnums.RegistrationState.Progress
                                       || LoginPageCpp.registrationState === LinphoneEnums.RegistrationState.Refreshing
    property string validationError: ""
    color: DefaultStyle.background

    function connectAccount() {
        if (connecting) return
        validationError = ""
        if (!usernameEdit.text.trim() || !passwordEdit.text || !domainEdit.text.trim()) {
            validationError = qsTr("Enter your extension, password and server address.")
            return
        }
        LoginPageCpp.login(usernameEdit.text.trim(), passwordEdit.text, displayName.text.trim(),
                           domainEdit.text.trim(), transport.currentValue, registrar.text.trim(),
                           proxy.text.trim(), authenticationId.text.trim())
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 32
        spacing: 48

        Rectangle {
            visible: mainItem.width >= 1050
            Layout.preferredWidth: Math.min(420, mainItem.width * 0.34)
            Layout.fillHeight: true
            radius: 24
            color: "#071E31"
            ColumnLayout {
                anchors.centerIn: parent
                spacing: 20
                Image {
                    Layout.alignment: Qt.AlignHCenter
                    source: "image://internal/mytel_logo.png"
                    fillMode: Image.PreserveAspectFit
                    Layout.preferredWidth: 96
                    Layout.preferredHeight: 96
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: applicationName
                    color: "#F4F8FD"
                    font.pixelSize: 34
                    font.weight: Font.Bold
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Your team, one conversation away.")
                    color: "#ADC4DE"
                    font.pixelSize: 15
                }
            }
        }

        Flickable {
            id: scroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: width
            contentHeight: form.implicitHeight + 48
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            Control.ScrollBar.vertical: Control.ScrollBar { policy: Control.ScrollBar.AsNeeded }

            ColumnLayout {
                id: form
                width: Math.min(480, scroll.width - 32)
                x: (scroll.width - width) / 2
                y: 24
                spacing: 16

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Connect to your phone system")
                    color: DefaultStyle.main2_900
                    font.pixelSize: 27
                    font.weight: Font.DemiBold
                    wrapMode: Text.WordWrap
                }
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Use the extension and password from your PBX administrator.")
                    color: DefaultStyle.main2_600
                    font.pixelSize: 14
                    wrapMode: Text.WordWrap
                    Layout.bottomMargin: 12
                }
                Text { text: qsTr("Extension"); font.pixelSize: 14 }
                TextField {
                    id: usernameEdit
                    Layout.fillWidth: true
                    enabled: !mainItem.connecting
                    placeholderText: "301"
                    placeholderTextColor: DefaultStyle.main2_600
                    Accessible.name: qsTr("Extension")
                    focus: true
                    onAccepted: passwordEdit.forceActiveFocus()
                }
                Text { text: qsTr("Password"); font.pixelSize: 14 }
                TextField {
                    id: passwordEdit
                    Layout.fillWidth: true
                    enabled: !mainItem.connecting
                    hidden: true
                    Accessible.name: qsTr("Password")
                    onAccepted: domainEdit.forceActiveFocus()
                }
                Text { text: qsTr("Server address"); font.pixelSize: 14 }
                TextField {
                    id: domainEdit
                    Layout.fillWidth: true
                    enabled: !mainItem.connecting
                    initialText: SettingsCpp.assistantThirdPartySipAccountDomain
                    placeholderText: "pbx.example.com:5060"
                    placeholderTextColor: DefaultStyle.main2_600
                    Accessible.name: qsTr("Server address")
                    onAccepted: mainItem.connectAccount()
                }
                RowLayout {
                    Layout.fillWidth: true
                    Text { text: qsTr("Transport"); font.pixelSize: 14; Layout.fillWidth: true }
                    ComboBox {
                        id: transport
                        Layout.preferredWidth: 140
                        enabled: !mainItem.connecting
                        textRole: "text"
                        valueRole: "value"
                        model: [
                            {text: "UDP", value: LinphoneEnums.TransportType.Udp},
                            {text: "TCP", value: LinphoneEnums.TransportType.Tcp},
                            {text: "TLS", value: LinphoneEnums.TransportType.Tls}
                        ]
                        currentIndex: SettingsCpp.assistantThirdPartySipAccountTransport.toLowerCase() === "tls" ? 2
                                      : SettingsCpp.assistantThirdPartySipAccountTransport.toLowerCase() === "tcp" ? 1 : 0
                        accessibleLabel: qsTr("Transport")
                    }
                }
                SmallButton {
                    text: mainItem.advanced ? qsTr("Hide advanced settings") : qsTr("Advanced settings")
                    style: ButtonStyle.noBackground
                    onClicked: mainItem.advanced = !mainItem.advanced
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    visible: mainItem.advanced
                    enabled: !mainItem.connecting
                    spacing: 10
                    Text { text: qsTr("Display name (optional)"); font.pixelSize: 13 }
                    TextField { id: displayName; Layout.fillWidth: true; Accessible.name: qsTr("Display name") }
                    Text { text: qsTr("Authentication ID (if different)"); font.pixelSize: 13 }
                    TextField { id: authenticationId; Layout.fillWidth: true; Accessible.name: qsTr("Authentication ID") }
                    Text { text: qsTr("Registrar URI (optional)"); font.pixelSize: 13 }
                    TextField { id: registrar; Layout.fillWidth: true; Accessible.name: qsTr("Registrar URI") }
                    Text { text: qsTr("Outbound proxy (optional)"); font.pixelSize: 13 }
                    TextField { id: proxy; Layout.fillWidth: true; Accessible.name: qsTr("Outbound proxy") }
                }
                Text {
                    Layout.fillWidth: true
                    text: mainItem.validationError || LoginPageCpp.errorMessage
                    visible: text.length > 0
                    color: DefaultStyle.danger_500_main
                    font.pixelSize: 14
                    wrapMode: Text.WordWrap
                    Accessible.role: Accessible.AlertMessage
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12
                    BigButton {
                        Layout.fillWidth: true
                        enabled: !mainItem.connecting
                        text: mainItem.connecting ? qsTr("Connecting…") : qsTr("Connect")
                        style: ButtonStyle.main
                        onClicked: mainItem.connectAccount()
                    }
                    BigButton {
                        visible: mainItem.connecting
                        text: qsTr("Cancel")
                        style: ButtonStyle.secondary
                        onClicked: LoginPageCpp.cancelLogin()
                    }
                }
                SmallButton {
                    visible: mainItem.showBackButton
                    text: qsTr("Back")
                    style: ButtonStyle.noBackground
                    onClicked: { LoginPageCpp.cancelLogin(); mainItem.goBack() }
                }
            }
        }
    }
}
