/**
* Qml template used for overview pages : Calls, Contacts, Conversations, Meetings, Settings
**/

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic as Control

import Linphone
import UtilsCpp
import 'qrc:/qt/qml/Linphone/view/Style/buttonStyle.js' as ButtonStyle
import "qrc:/qt/qml/Linphone/view/Control/Tool/Helper/utils.js" as Utils

FocusScope {
	id: mainItem
	property string noItemButtonText
	property string newItemIconSource
	property string emptyListDescription: ""
	property string emptyListText
	property bool showDefaultItem: true
	property color rightPanelColor: DefaultStyle.grey_100
	property alias leftPanelContent: leftPanel.children
	property alias rightPanelStackView: rightPanelStackView
	property alias rightPanel: rightPanel
	property int rightPanelStackTopMargin: 0
	property int rightPanelStackBottomMargin: 0
	signal noItemButtonPressed()

	Control.SplitView {
		id: splitView
		anchors.fill: parent
    	anchors.topMargin: Utils.getSizeWithScreenRatio(10)

		handle: Rectangle {
    		implicitWidth: Utils.getSizeWithScreenRatio(6)
			color: Control.SplitHandle.hovered ? DefaultStyle.main2_200 : DefaultStyle.grey_200
		}
		ColumnLayout {
			id: leftPanel
			spacing:0
			// Control.SplitView.fillWidth:false
			Control.SplitView.fillHeight: true
			Control.SplitView.preferredWidth: Utils.getSizeWithScreenRatio(404)
			Control.SplitView.minimumWidth: 300
    		Control.SplitView.maximumWidth: Utils.getSizeWithScreenRatio(500)
		}
		Rectangle {
			id: rightPanel
			clip: true
			color: mainItem.rightPanelColor
			Control.SplitView.fillWidth: true
			Control.SplitView.fillHeight: true

			StackLayout {
				currentIndex: mainItem.showDefaultItem ? 0 : 1
				anchors.fill: parent
                Item {
                    ColumnLayout {
                        anchors.centerIn: parent
                        width: Math.min(parent.width - 48, 430)
                        spacing: 20
                        Image {
                            source: "image://internal/mytel_logo.png"
                            fillMode: Image.PreserveAspectFit
                            Layout.preferredWidth: 72
                            Layout.preferredHeight: 72
                            Layout.alignment: Qt.AlignHCenter
                        }
                        Text {
                            text: mainItem.emptyListText
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.WordWrap
                            font.pixelSize: 24
                            font.weight: Font.DemiBold
                            color: DefaultStyle.main2_900
                        }
                        Text {
                            visible: text.length > 0
                            text: mainItem.emptyListDescription
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.WordWrap
                            font.pixelSize: 15
                            color: DefaultStyle.main2_600
                        }
                        BigButton {
                            Layout.alignment: Qt.AlignHCenter
                            icon.source: mainItem.newItemIconSource
                            style: ButtonStyle.main
                            text: mainItem.noItemButtonText
                            spacing: 8
                            onClicked: mainItem.noItemButtonPressed()
                        }
                    }
                }
				Control.StackView {
					id: rightPanelStackView
					Layout.fillWidth: true
					Layout.fillHeight: true
					Layout.topMargin: mainItem.rightPanelStackTopMargin
					Layout.bottomMargin: mainItem.rightPanelStackBottomMargin
					visible: false
				}
			}
		}
	}
}
