import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic as Control

import Linphone
import UtilsCpp
import SettingsCpp
import 'qrc:/qt/qml/Linphone/view/Control/Tool/Helper/utils.js' as Utils

// MyTel (owner requirement 2026-11-12): direct destination entry is the
// PRIMARY path — type an extension and press Enter or "Start chat".
CreationFormLayout {
	id: mainItem

	//: Nouveau groupe
	startGroupButtonText: qsTr("chat_start_group_chat_title")
	searchBarVisible: false
	contactListVisible: false

	property string destinationError: ""

	signal chatDestinationRequested(string destination)

	function normalizeInput(raw) {
		var s = raw.trim()
		if (s.toLowerCase().startsWith("sip:")) return s
		var digits = s.replace(/[\s\-\.\(\)]/g, "")
		digits = digits.replace(/[\u06F0-\u06F9]/g, function (d) {
			return String.fromCharCode(d.charCodeAt(0) - 0x06F0 + 0x30)
		})
		digits = digits.replace(/[\u0660-\u0669]/g, function (d) {
			return String.fromCharCode(d.charCodeAt(0) - 0x0660 + 0x30)
		})
		if (/^\+98/.test(digits)) digits = "0" + digits.slice(3)
		else if (/^0098/.test(digits)) digits = "0" + digits.slice(4)
		else if (/^098/.test(digits)) digits = "0" + digits.slice(3)
		return digits
	}

	function startChat() {
		destinationError = ""
		var destination = normalizeInput(destinationField.text)
		if (destination.length === 0) {
			destinationError = qsTr("mytel_chat_invalid_destination")
			return
		}
		console.log("[MyTel] Start chat with [" + destination + "]")
		chatDestinationRequested(destination)
	}

	topContent: [
		ColumnLayout {
			Layout.fillWidth: true
			spacing: Utils.getSizeWithScreenRatio(10)

			Text {
				Layout.fillWidth: true
				text: qsTr("mytel_chat_destination_label")
				color: DefaultStyle.main2_700
				font.pixelSize: Typography.p1.pixelSize
				font.weight: Typography.p1.weight
			}

			// Simple TextField (no SearchBar machinery — direct entry only)
			Control.TextField {
				id: destinationField
				Layout.fillWidth: true
				Layout.preferredHeight: Utils.getSizeWithScreenRatio(48)
				Layout.rightMargin: mainItem.searchBarRightMaring
				focus: true
				placeholderText: qsTr("mytel_chat_destination_hint")
				font.pixelSize: Utils.getSizeWithScreenRatio(18)
				color: DefaultStyle.main2_900
				background: Rectangle {
					radius: height / 2
					color: DefaultStyle.grey_100
					border.width: destinationField.activeFocus ? 2 : 0
					border.color: DefaultStyle.main1_500_main
				}
				onAccepted: mainItem.startChat()
				Keys.onReturnPressed: mainItem.startChat()
			}

			Text {
				Layout.fillWidth: true
				visible: mainItem.destinationError.length > 0
				text: mainItem.destinationError
				color: DefaultStyle.danger_500_main
				font.pixelSize: Typography.p1.pixelSize
				wrapMode: Text.Wrap
			}

			RowLayout {
				Layout.fillWidth: true
				spacing: Utils.getSizeWithScreenRatio(12)

				Control.Button {
					id: startChatButton
					Layout.preferredWidth: Utils.getSizeWithScreenRatio(220)
					Layout.preferredHeight: Utils.getSizeWithScreenRatio(44)
					enabled: destinationField.text.length > 0
					onClicked: mainItem.startChat()
					background: Rectangle {
						anchors.fill: parent
						radius: height / 2
						color: startChatButton.enabled
							? DefaultStyle.main1_500_main
							: DefaultStyle.main1_200
					}
					contentItem: Text {
						text: qsTr("mytel_chat_start_action")
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
					id: browseButton
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
