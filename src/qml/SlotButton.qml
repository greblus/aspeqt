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
    property bool spinning: false          // rotate the icon (e.g. XEX loading)
    property url animatedSource: ""        // GIF shown while `animated` is true
    property bool animated: false          // e.g. CAS playing -> tape.gif
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
        id: icon
        visible: !(root.animated && root.animatedSource != "")
        anchors.centerIn: parent
        width: Theme.iconSize
        height: Theme.iconSize
        source: root.source
        sourceSize.width: Theme.iconSize * 2
        sourceSize.height: Theme.iconSize * 2
        fillMode: Image.PreserveAspectFit
        smooth: true
        RotationAnimator on rotation {
            running: root.spinning
            loops: Animation.Infinite
            from: 0; to: 360
            duration: 2000
        }
        onVisibleChanged: if (!visible) rotation = 0
    }

    AnimatedImage {
        visible: root.animated && root.animatedSource != ""
        anchors.centerIn: parent
        width: Theme.iconSize
        height: Theme.iconSize
        source: root.animatedSource
        fillMode: Image.PreserveAspectFit
        playing: visible
        smooth: true
    }

    MouseArea {
        id: ma
        anchors.fill: parent
        enabled: root.enabledState
        hoverEnabled: true
        onClicked: root.clicked()
        // Hint only on press-and-hold (a plain tap must not show it).
        onPressAndHold: if (root.tip.length > 0) ToolTip.show(root.tip, 2500)
    }
}
