import QtQuick
import "."

// Orange rounded badge: slot number for disk slots, "cas/xex" for the loader.
Rectangle {
    property alias text: label.text
    property int fontPx: 14
    width: Theme.badgeSize
    height: Theme.badgeSize
    radius: 8
    color: Theme.accent

    Text {
        id: label
        anchors.centerIn: parent
        horizontalAlignment: Text.AlignHCenter
        color: "white"
        font.bold: true
        font.pixelSize: parent.fontPx
        lineHeight: 0.9
    }
}
