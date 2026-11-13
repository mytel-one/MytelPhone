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
        var s = raw.trim()
        if (s.toLowerCase().startsWith("sip:")) return s
        var digits = s.replace(/[\s\-\.\(\)]/g, "")
        digits = digits.replace(/[\u06F0-\u06F9]/g, function(d) {
            return String.fromCharCode(d.charCodeAt(0) - 0x06F0 + 0x30)
        })
        digits = digits.replace(/[\u0660-\u0669]/g, function(d) {
            return String.fromCharCode(d.charCodeAt(0) - 0x0660 + 0x30)
        })
        if (/^\+98/.test(digits)) digits = "0" + digits.slice(3)
        else if (/^0098/.test(digits)) digits = "0" + digits.slice(4)
        else if (/^098/.test(digits)) digits = "0" + digits.slice(3)
        return digits
    }

    function placeCall() {
        dialError = ""
        var destination = normalizeInput(dialField.text)
        if (destination.length === 0) {
            dialError = qsTr("mytel_chat_invalid_destination")
            return
        }
        console.log("[MyTel] Placing call to [" + destination + "]")
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
                text: qsTr("mytel_call_destination_label")
                color: DefaultStyle.main2_700
                font.pixelSize: Typography.p1.pixelSize
                font.weight: Typography.p1.weight
            }

            Control.TextField {
                id: dialField
                Layout.fillWidth: true
                Layout.preferredHeight: Utils.getSizeWithScreenRatio(48)
                Layout.rightMargin: mainItem.searchBarRightMaring
                focus: true
                //: "Extension or number"
                placeholderText: qsTr("mytel_chat_destination_hint")
                font.pixelSize: Utils.getSizeWithScreenRatio(18)
                color: DefaultStyle.main2_900
                background: Rectangle {
                    radius: height / 2
                    color: DefaultStyle.grey_100
                    border.width: dialField.activeFocus ? 2 : 0
                    border.color: DefaultStyle.main1_500_main
                }
                onAccepted: mainItem.placeCall()
                Keys.onReturnPressed: mainItem.placeCall()
            }

            Text {
                Layout.fillWidth: true
                visible: mainItem.dialError.length > 0
                text: mainItem.dialError
                color: DefaultStyle.danger_500_main
                font.pixelSize: Typography.p1.pixelSize
                wrapMode: Text.Wrap
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Utils.getSizeWithScreenRatio(12)

                Control.Button {
                    id: placeCallButton
                    Layout.preferredWidth: Utils.getSizeWithScreenRatio(180)
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
                        text: qsTr("mytel_call_action")
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
                    leftPadding: Utils.getSizeWithScreenRatio(16)
                    rightPadding: Utils.getSizeWithScreenRatio(16)
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
                        text: qsTr("mytel_chat_browse_contacts")
                        color: DefaultStyle.main2_700
                        anchors.centerIn: parent
                        font.pixelSize: Typography.p1.pixelSize
                    }
                }

                Item { Layout.fillWidth: true }
            }
        }
    ]
}
