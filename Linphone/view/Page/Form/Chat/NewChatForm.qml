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
        return Utils.normalizeSipDestination(raw)
    }

	function startChat() {
		destinationError = ""
		var destination = normalizeInput(destinationField.text)
		if (destination.length === 0) {
			destinationError = qsTr("Enter a valid extension, number or SIP address.")
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
				text: qsTr("Message an extension")
				color: DefaultStyle.main2_700
				font.pixelSize: 14
				font.weight: Typography.p1.weight
			}

			// Simple TextField (no SearchBar machinery — direct entry only)
			Control.TextField {
				id: destinationField
				Layout.fillWidth: true
				Layout.preferredHeight: Utils.getSizeWithScreenRatio(48)
				Layout.rightMargin: 0
				focus: true
				placeholderTextColor: DefaultStyle.main2_600
                placeholderText: qsTr("Extension or SIP address")
				font.pixelSize: Utils.getSizeWithScreenRatio(18)
				color: DefaultStyle.main2_900
				background: Rectangle {
					radius: height / 2
					color: DefaultStyle.grey_100
					border.width: destinationField.activeFocus ? 2 : 0
					border.color: DefaultStyle.main1_500_main
				}
				onAccepted: mainItem.startChat()

			}

			Text {
				Layout.fillWidth: true
				visible: mainItem.destinationError.length > 0
				text: mainItem.destinationError
				color: DefaultStyle.danger_500_main
				font.pixelSize: 14
				wrapMode: Text.Wrap
			}

			RowLayout {
				Layout.fillWidth: true
				spacing: Utils.getSizeWithScreenRatio(12)

				Control.Button {
					id: startChatButton
					Layout.fillWidth: true
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
						text: qsTr("Start chat")
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
