import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic as Control

import Linphone
import UtilsCpp
import SettingsCpp
import 'qrc:/qt/qml/Linphone/view/Control/Tool/Helper/utils.js' as Utils

// MyTel (owner requirement 2026-11-12): direct dial is the PRIMARY call
// path — type a number/extension and press Enter or the Call button.
CreationFormLayout {
    id: mainItem
    property bool displayCurrentCalls: false
    signal transferCallToAnotherRequested(CallGui dest)
    signal transferDestinationRequested(string destination)

    //: Appel de groupe
    startGroupButtonText: qsTr("call_start_group_call_title")
    startGroupButtonVisible: !SettingsCpp.disableMeetingsFeature

    // MyTel: hide contact search+list, direct dial is primary
    searchBarVisible: false
    contactListVisible: false

    property string dialError: ""

    // CRITICAL FIX (owner 2026-11-13): the entire dial UI lives inside
    // topContent, which was invisible because the upstream binding
    // required displayCurrentCalls (only true in the transfer window).
    // Force it visible — the dial field IS the primary content here.
    topLayoutVisible: true

    function normalizeInput(raw) {
        return Utils.normalizeSipDestination(raw)
    }

    function placeCall() {
        dialError = ""
        var destination = normalizeInput(dialField.text)
        if (destination.length === 0) {
            dialError = qsTr("Enter a valid extension, number or SIP address.")
            return
        }
        if (displayCurrentCalls)
            mainItem.transferDestinationRequested(destination)
        else
            UtilsCpp.createCall(destination)
    }

    topContent: [
        Text {
            visible: callList.count > 0
            text: qsTr("call_transfer_active_calls_label")
            font {
                pixelSize: Typography.h4.pixelSize
                weight: Typography.h4.weight
            }
        },
        Flickable {
            visible: callList.count > 0
            Layout.fillWidth: true
            Layout.preferredHeight: callListBackground.height
            Layout.maximumHeight: mainItem.height/2
            contentHeight: callListBackground.height
            contentWidth: width
            RoundedPane {
                id: callListBackground
                anchors.left: parent.left
                anchors.right: parent.right
                contentItem: CallListView {
                    id: callList
                    isTransferList: true
                    onTransferCallToAnotherRequested: (dest) => {
                        mainItem.transferCallToAnotherRequested(dest)
                    }
                }
            }
        },
        ColumnLayout {
            Layout.fillWidth: true
            spacing: Utils.getSizeWithScreenRatio(10)

            Text {
                Layout.fillWidth: true
                //: "Call an extension"
                text: qsTr("Call an extension or outside number")
                color: DefaultStyle.main2_700
                font.pixelSize: 14
                font.weight: Typography.p1.weight
            }

            Control.TextField {
                id: dialField
                Layout.fillWidth: true
                Layout.preferredHeight: Utils.getSizeWithScreenRatio(48)
                Layout.rightMargin: 0
                focus: true
                //: "Extension or number"
                placeholderTextColor: DefaultStyle.main2_600
                placeholderText: qsTr("Extension, number or SIP address")
                font.pixelSize: Utils.getSizeWithScreenRatio(18)
                color: DefaultStyle.main2_900
                background: Rectangle {
                    radius: height / 2
                    color: DefaultStyle.grey_100
                    border.width: dialField.activeFocus ? 2 : 0
                    border.color: DefaultStyle.main1_500_main
                }
                onAccepted: mainItem.placeCall()

            }

            Text {
                Layout.fillWidth: true
                visible: mainItem.dialError.length > 0
                text: mainItem.dialError
                color: DefaultStyle.danger_500_main
                font.pixelSize: 14
                wrapMode: Text.Wrap
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Utils.getSizeWithScreenRatio(12)

                Control.Button {
                    id: placeCallButton
                    Layout.fillWidth: true
                    Layout.preferredHeight: Utils.getSizeWithScreenRatio(44)
                    enabled: dialField.text.length > 0
                    onClicked: mainItem.placeCall()
                    background: Rectangle {
                        anchors.fill: parent
                        radius: height / 2
                        color: placeCallButton.enabled
                            ? DefaultStyle.main1_500_main
                            : DefaultStyle.main1_200
                    }
                    contentItem: Text {
                        //: "Call"
                        text: mainItem.displayCurrentCalls ? qsTr("Transfer") : qsTr("Call")
                        color: DefaultStyle.grey_0
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font {
                            pixelSize: Typography.p1.pixelSize
                            weight: Typography.p1.weight
                        }
                    }
                }

                Control.Button {
                    id: browseContactsBtn
                    Layout.preferredHeight: Utils.getSizeWithScreenRatio(44)
                    leftPadding: 12
                    rightPadding: 12
                    onClicked: {
                        mainItem.searchBarVisible = !mainItem.searchBarVisible
                        mainItem.contactListVisible = !mainItem.contactListVisible
                    }
                    background: Rectangle {
                        anchors.fill: parent
                        radius: height / 2
                        color: "transparent"
                        border.color: DefaultStyle.main2_300
                        border.width: 1
                    }
                    contentItem: Text {
                        text: qsTr("Contacts")
                        color: DefaultStyle.main2_700
                        anchors.centerIn: parent
                        font.pixelSize: 14
                    }
                }

                Item { Layout.fillWidth: true }
            }
        }
    ]
}
