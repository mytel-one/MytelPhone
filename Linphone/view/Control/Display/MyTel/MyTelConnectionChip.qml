import QtQuick
import QtQuick.Layouts

// MyTel connection indicator (owner requirement 2026-11-11, labels revised
// to Online/Offline). Extracted as a reusable component so the production
// MainWindow and the GUI test load the SAME indicator and binding formula.
//
// `registered` is injected by the consumer (default-account check).
// Network reachability comes from the reactive AppCpp.networkReachable
// NOTIFY property (CoreModel::onNetworkReachable -> App::setNetworkReachable).
Item {
    id: root

    property bool registered: false
    readonly property bool connected: registered && AppCpp.networkReachable
    readonly property string connectedLabel: "Online"
    readonly property string disconnectedLabel: "Offline"
    readonly property color connectedColor: "#22C55E"
    readonly property color disconnectedColor: "#EF4444"

    // Consumer positions the chip; content sizes itself.
    width: chipRow.width + 24
    height: 34

    Rectangle {
        id: chipBackground
        objectName: "chipBackground"
        anchors.fill: parent
        radius: height / 2
        color: root.connected ? root.connectedColor : root.disconnectedColor
    }

    RowLayout {
        id: chipRow
        anchors.centerIn: parent
        spacing: 8
        Rectangle {
            width: 10
            height: 10
            radius: 5
            color: "white"
        }
        Text {
            id: chipLabel
            objectName: "chipLabel"
            text: root.connected ? root.connectedLabel : root.disconnectedLabel
            color: "white"
            font.pixelSize: 16
            font.weight: Font.DemiBold
        }
    }

    // Blinking makes the failure state obvious at a glance; the 0.6 floor
    // keeps the text legible and reduced motion disables the blink (leader
    // correction). Connected state restores full opacity.
    SequentialAnimation on opacity {
        running: root.visible && !root.connected && !UtilsCpp.isReduceMotionEnabled()
        loops: Animation.Infinite
        NumberAnimation { to: 0.6; duration: 600 }
        NumberAnimation { to: 1; duration: 600 }
    }
    onConnectedChanged: if (connected) opacity = 1
}
