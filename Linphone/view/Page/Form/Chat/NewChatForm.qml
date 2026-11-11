import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic as Control
import QtQuick.Effects

import Linphone
import UtilsCpp
import SettingsCpp

// MyTel (owner requirement 2026-11-12, leader design verdict): direct
// destination entry is the PRIMARY path for starting a chat — type or
// paste an extension / phone number / SIP URI and press Enter or
// "Start chat". No contact list is displayed by default; browsing the
// directory stays behind the explicit "Browse contacts" toggle below.
CreationFormLayout {
	id: mainItem

	//: Nouveau groupe
	startGroupButtonText: qsTr("chat_start_group_chat_title")

	// MyTel: hide the baked-in contact search + list of the base layout
	searchBarVisible: false
	contactListVisible: false

	property string destinationError: ""

	// Emitted with the (normalized) destination; the consumer (ChatPage)
	// feeds it into remoteAddress -> UtilsCpp.getChatForAddress which uses
	// the SDK-aware ToolModel::interpretUrl and the existing room lookup.
	signal chatDestinationRequested(string destination)

	// Iranian telephone normalization for plain numbers (mirrors the call
	// pre-dial normalizer); explicit SIP URIs are passed through untouched
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

	function startChat(raw) {
		destinationError = ""
		var destination = normalizeInput(raw)
		if (destination.length === 0) {
			//: "Invalid number or extension"
			destinationError = qsTr("mytel_chat_invalid_destination")
			return
		}
		console.log("[MyTel] Start chat requested with [" + raw + "] -> [" + destination + "]")
		chatDestinationRequested(destination)
	}

	topContent: [
		ColumnLayout {
			id: directEntry
			Layout.fillWidth: true
			spacing: Utils.getSizeWithScreenRatio(10)

			Text {
				Layout.fillWidth: true
				//: "Chat with an extension"
				text: qsTr("mytel_chat_destination_label")
				color: DefaultStyle.main2_700
				font.pixelSize: Typography.p1.pixelSize
				font.weight: Typography.p1.weight
			}

			SearchBar {
				id: destinationInput
				Layout.fillWidth: true
				Layout.rightMargin: mainItem.searchBarRightMaring
				focus: true
				delaySearch: false
				magnifierVisible: false
				//: "Extension or number"
				placeholderText: qsTr("mytel_chat_destination_hint")

				Keys.onReturnPressed: {
					if (text.length > 0) mainItem.startChat(text)
				}
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

				Button {
					id: startChatButton
					Layout.preferredWidth: Utils.getSizeWithScreenRatio(220)
					Layout.preferredHeight: Utils.getSizeWithScreenRatio(44)
					padding: 0
					enabled: destinationInput.text.length > 0
					onClicked: mainItem.startChat(destinationInput.text)
					Accessible.name: qsTr("mytel_chat_start_action")
					background: Rectangle {
						anchors.fill: parent
						radius: height / 2
						color: startChatButton.enabled
							? DefaultStyle.main1_500_main
							: DefaultStyle.main1_200
					}
					contentItem: Text {
						//: "Start chat"
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

				Button {
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
						//: "Browse contacts"
						text: qsTr("mytel_chat_browse_contacts")
						color: DefaultStyle.main2_700
						anchors.centerIn: parent
						font.pixelSize: Typography.p1.pixelSize
					}
				}

				Item {
					Layout.fillWidth: true
				}
			}
		}
	]
}
