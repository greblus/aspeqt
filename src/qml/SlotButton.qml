import QtQuick
import QtQuick.Controls
import "."

// Raised icon button matching the widget UI's QToolButtons: a bevelled, bordered
// box holding a plain (multicolour) SVG icon; a blue tint when checked; dimming
// when disabled. No Material tinting so the tango icons keep their colours.
Item {
    id: root
    property url source
    property bool enabledState: true
    property bool checked: false
    property string tip: ""
    signal clicked

    implicitWidth: Theme.iconSize + 2 * Theme.btnPad
    implicitHeight: Theme.iconSize + 2 * Theme.btnPad
    opacity: enabledState ? 1.0 : 0.38

    Rectangle {
        anchors.fill: parent
        radius: 5
        border.width: 1
        border.color: root.checked ? Theme.btnCheckedBorder : Theme.btnBorder
        gradient: Gradient {
            GradientStop { position: 0.0
                color: root.checked ? Theme.btnCheckedBg
                                    : (ma.pressed ? Theme.btnPressed : Theme.btnBgTop) }
            GradientStop { position: 1.0
                color: root.checked ? Theme.btnCheckedBg
                                    : (ma.pressed ? Theme.btnPressed : Theme.btnBgBottom) }
        }
    }

    Image {
        anchors.centerIn: parent
        width: Theme.iconSize
        height: Theme.iconSize
        source: root.source
        sourceSize.width: Theme.iconSize * 2
        sourceSize.height: Theme.iconSize * 2
        fillMode: Image.PreserveAspectFit
        smooth: true
    }

    MouseArea {
        id: ma
        anchors.fill: parent
        enabled: root.enabledState
        hoverEnabled: true
        onClicked: root.clicked()
        // Touch has no hover: reveal the hint on press-and-hold.
        onPressAndHold: if (root.tip.length > 0) ToolTip.show(root.tip, 2500)
    }

    // Desktop/mouse hover tooltip.
    ToolTip.visible: tip.length > 0 && ma.containsMouse
    ToolTip.text: tip
}
