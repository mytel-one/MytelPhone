import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic as Control
import Linphone
import SettingsCpp
import 'qrc:/qt/qml/Linphone/view/Style/buttonStyle.js' as ButtonStyle
import 'qrc:/qt/qml/Linphone/view/Control/Tool/Helper/utils.js' as Utils

Rectangle {
    id: mainItem
    signal goBack()
    signal goToRegister()
    property bool showBackButton: false
    property bool advanced: false
    readonly property bool connecting: LoginPageCpp.registrationState === LinphoneEnums.RegistrationState.Progress
                                       || LoginPageCpp.registrationState === LinphoneEnums.RegistrationState.Refreshing
    property string validationError: ""
    property string validationField: ""
    color: DefaultStyle.background

    function clearValidation(field) {
        if (mainItem.validationField !== field) return
        mainItem.validationField = ""
        mainItem.validationError = ""
    }

    function connectAccount() {
        if (mainItem.connecting) return
        mainItem.validationError = ""
        mainItem.validationField = ""
        if (!usernameEdit.text.trim()) {
            mainItem.validationField = "extension"
            mainItem.validationError = qsTr("Enter your extension or SIP username.")
            usernameEdit.forceActiveFocus()
            return
        }
        if (!passwordEdit.text) {
            mainItem.validationField = "password"
            mainItem.validationError = qsTr("Enter your account password.")
            passwordEdit.forceActiveFocus()
            return
        }
        if (!domainEdit.text.trim()) {
            mainItem.validationField = "server"
            mainItem.validationError = qsTr("Enter your phone system's server address.")
            domainEdit.forceActiveFocus()
            return
        }
        var resolvedTransport = Utils.resolveSetupTransport(domainEdit.text, transport.currentValue)
        var transportType = resolvedTransport === "TLS" ? LinphoneEnums.TransportType.Tls
                          : resolvedTransport === "TCP" ? LinphoneEnums.TransportType.Tcp
                          : LinphoneEnums.TransportType.Udp
        LoginPageCpp.login(usernameEdit.text.trim(), passwordEdit.text, displayName.text.trim(),
                           domainEdit.text.trim(), transportType, registrar.text.trim(),
                           proxy.text.trim(), authenticationId.text.trim())
    }

    component FieldLabel: Text {
        Layout.fillWidth: true
        color: DefaultStyle.main2_700
        font.pixelSize: 13
        font.weight: Font.Medium
        wrapMode: Text.WordWrap
    }

    component Benefit: RowLayout {
        property string iconSource
        property string label
        Layout.fillWidth: true
        spacing: 14
        EffectImage {
            imageSource: parent.iconSource
            colorizationColor: "#79C4FF"
            Layout.preferredWidth: 24
            Layout.preferredHeight: 24
        }
        Text {
            text: parent.label
            Layout.fillWidth: true
            color: "#E5EFF9"
            font.pixelSize: 15
            wrapMode: Text.WordWrap
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: mainItem.width < 1180 ? 24 : 36
        spacing: 32

        Rectangle {
            visible: mainItem.width >= 1180
            Layout.preferredWidth: Math.min(360, mainItem.width * 0.30)
            Layout.fillHeight: true
            radius: 24
            color: "#071E31"
            border.color: "#183B58"
            ColumnLayout {
                anchors.centerIn: parent
                width: parent.width - 64
                spacing: 22
                Image {
                    Layout.alignment: Qt.AlignLeft
                    source: "image://internal/mytel_logo.png"
                    fillMode: Image.PreserveAspectFit
                    Layout.preferredWidth: 84
                    Layout.preferredHeight: 84
                }
                Text {
                    Layout.fillWidth: true
                    text: applicationName
                    color: "#F4F8FD"
                    font.pixelSize: 32
                    font.weight: Font.Bold
                }
                Rectangle {
                    Layout.preferredWidth: 44
                    Layout.preferredHeight: 4
                    radius: 2
                    color: "#FFAB2E"
                }
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Your work phone.\nWherever you work.")
                    color: "#F4F8FD"
                    font.pixelSize: 25
                    font.weight: Font.DemiBold
                    wrapMode: Text.WordWrap
                    Layout.bottomMargin: 12
                }
                Benefit { iconSource: AppIcons.phone; label: qsTr("Call extensions and customers") }
                Benefit { iconSource: AppIcons.chatTeardropText; label: qsTr("Keep team conversations together") }
                Benefit { iconSource: AppIcons.adressBook; label: qsTr("Reach your contacts quickly") }
                Text {
                    Layout.topMargin: 14
                    text: "mytel.one"
                    color: "#ADC4DE"
                    font.pixelSize: 14
                }
            }
        }

        Flickable {
            id: scroll
            objectName: "sipLoginScroll"
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: width
            contentHeight: formPanel.height + 24
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            Control.ScrollBar.vertical: Control.ScrollBar { policy: Control.ScrollBar.AsNeeded }

            Rectangle {
                id: formPanel
                width: Math.min(548, Math.max(0, scroll.width - 24))
                height: form.implicitHeight + 48
                x: (scroll.width - width) / 2
                y: Math.max(12, (scroll.height - height) / 2)
                radius: 22
                color: DefaultStyle.surface
                border.color: DefaultStyle.main2_200

                ColumnLayout {
                    id: form
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.margins: 24
                    spacing: 12

                    RowLayout {
                        visible: mainItem.width < 1180
                        Layout.fillWidth: true
                        spacing: 12
                        Image {
                            source: "image://internal/mytel_logo.png"
                            fillMode: Image.PreserveAspectFit
                            Layout.preferredWidth: 40
                            Layout.preferredHeight: 40
                        }
                        Text {
                            text: applicationName
                            color: DefaultStyle.main2_900
                            font.pixelSize: 22
                            font.weight: Font.Bold
                        }
                        Item { Layout.fillWidth: true }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 5
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Connect your work phone")
                            color: DefaultStyle.main2_900
                            font.pixelSize: 27
                            font.weight: Font.DemiBold
                            wrapMode: Text.WordWrap
                        }
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Enter the account details from your phone system administrator.")
                            color: DefaultStyle.main2_600
                            font.pixelSize: 13
                            wrapMode: Text.WordWrap
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        FieldLabel { text: qsTr("Extension or SIP username") }
                        TextField {
                            id: usernameEdit
                            objectName: "sipUsernameInput"
                            Layout.fillWidth: true
                            Layout.preferredHeight: 44
                            enabled: !mainItem.connecting
                            placeholderText: "301"
                            placeholderTextColor: DefaultStyle.main2_600
                            isError: mainItem.validationField === "extension"
                            Accessible.name: qsTr("Extension or SIP username")
                            focus: true
                            onTextEdited: mainItem.clearValidation("extension")
                            onAccepted: passwordEdit.forceActiveFocus()
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        FieldLabel { text: qsTr("Password") }
                        TextField {
                            id: passwordEdit
                            objectName: "sipPasswordInput"
                            Layout.fillWidth: true
                            Layout.preferredHeight: 44
                            enabled: !mainItem.connecting
                            hidden: true
                            isError: mainItem.validationField === "password"
                            Accessible.name: qsTr("Password")
                            onTextEdited: mainItem.clearValidation("password")
                            onAccepted: domainEdit.forceActiveFocus()
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6
                        FieldLabel { text: qsTr("Server address") }
                        TextField {
                            id: domainEdit
                            objectName: "sipServerInput"
                            Layout.fillWidth: true
                            Layout.preferredHeight: 44
                            enabled: !mainItem.connecting
                            initialText: SettingsCpp.assistantThirdPartySipAccountDomain
                            placeholderText: "pbx.company.com:5060"
                            placeholderTextColor: DefaultStyle.main2_600
                            isError: mainItem.validationField === "server"
                            Accessible.name: qsTr("Server address")
                            onTextEdited: mainItem.clearValidation("server")
                            onAccepted: mainItem.connectAccount()
                        }
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Use a hostname or IP address, with a port if provided.")
                            color: DefaultStyle.main2_600
                            font.pixelSize: 12
                            wrapMode: Text.WordWrap
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Connection: %1").arg(transport.currentIndex === 0 ? qsTr("Automatic") : transport.currentText)
                            color: DefaultStyle.main2_600
                            font.pixelSize: 12
                        }
                        SmallButton {
                            objectName: "sipConnectionOptionsButton"
                            Layout.minimumHeight: 40
                            enabled: !mainItem.connecting
                            text: mainItem.advanced ? qsTr("Hide connection options") : qsTr("Connection options")
                            style: ButtonStyle.noBackground
                            Accessible.name: text
                            Accessible.description: qsTr("Transport, display name and optional server settings")
                            onClicked: mainItem.advanced = !mainItem.advanced
                        }
                    }

                    ColumnLayout {
                        objectName: "sipAdvancedOptions"
                        Layout.fillWidth: true
                        visible: mainItem.advanced
                        enabled: !mainItem.connecting
                        spacing: 12
                        Text {
                            Layout.fillWidth: true
                            text: qsTr("Only change these if your administrator asks.")
                            color: DefaultStyle.main2_600
                            font.pixelSize: 12
                            wrapMode: Text.WordWrap
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            FieldLabel { text: qsTr("Transport") }
                            ComboBox {
                                id: transport
                                objectName: "sipTransportInput"
                                Layout.preferredWidth: 280
                                Layout.preferredHeight: 44
                                pixelSize: 13
                                oneLine: true
                                textRole: "text"
                                valueRole: "value"
                                model: [
                                    {text: qsTr("Automatic (recommended)"), value: "Automatic"},
                                    {text: "UDP", value: "UDP"},
                                    {text: "TCP", value: "TCP"},
                                    {text: "TLS", value: "TLS"}
                                ]
                                currentIndex: 0
                                accessibleLabel: qsTr("Transport")
                            }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 6
                            FieldLabel { text: qsTr("Display name (optional)") }
                            TextField { id: displayName; objectName: "sipDisplayNameInput"; Layout.fillWidth: true; Layout.preferredHeight: 44; Accessible.name: qsTr("Display name") }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 6
                            FieldLabel { text: qsTr("Authentication ID (if different)") }
                            TextField { id: authenticationId; objectName: "sipAuthenticationIdInput"; Layout.fillWidth: true; Layout.preferredHeight: 44; Accessible.name: qsTr("Authentication ID") }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 6
                            FieldLabel { text: qsTr("Registrar URI (optional)") }
                            TextField { id: registrar; objectName: "sipRegistrarInput"; Layout.fillWidth: true; Layout.preferredHeight: 44; Accessible.name: qsTr("Registrar URI") }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 6
                            FieldLabel { text: qsTr("Outbound proxy (optional)") }
                            TextField { id: proxy; objectName: "sipProxyInput"; Layout.fillWidth: true; Layout.preferredHeight: 44; Accessible.name: qsTr("Outbound proxy") }
                        }
                    }

                    Text {
                        objectName: "sipRegistrationError"
                        Layout.fillWidth: true
                        text: mainItem.validationError || LoginPageCpp.errorMessage
                        visible: text.length > 0
                        color: DefaultStyle.dark ? DefaultStyle.danger_500_main : DefaultStyle.danger_700
                        font.pixelSize: 13
                        wrapMode: Text.WordWrap
                        Accessible.role: Accessible.AlertMessage
                    }

                    Control.ProgressBar {
                        visible: mainItem.connecting
                        Layout.fillWidth: true
                        indeterminate: true
                        Accessible.name: qsTr("Connecting to your phone system")
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12
                        BigButton {
                            objectName: "sipConnectButton"
                            Layout.fillWidth: true
                            Layout.minimumHeight: 48
                            enabled: !mainItem.connecting
                            text: mainItem.connecting ? qsTr("Connecting…") : qsTr("Connect")
                            style: ButtonStyle.main
                            onClicked: mainItem.connectAccount()
                        }
                        BigButton {
                            objectName: "sipCancelButton"
                            visible: mainItem.connecting
                            Layout.minimumHeight: 48
                            text: qsTr("Cancel")
                            style: ButtonStyle.secondary
                            onClicked: LoginPageCpp.cancelLogin()
                        }
                    }
                    SmallButton {
                        visible: mainItem.showBackButton && !mainItem.connecting
                        Layout.alignment: Qt.AlignHCenter
                        Layout.minimumHeight: 40
                        text: qsTr("Back to MyTel")
                        style: ButtonStyle.noBackground
                        onClicked: { LoginPageCpp.cancelLogin(); mainItem.goBack() }
                    }
                }
            }
        }
    }
}
