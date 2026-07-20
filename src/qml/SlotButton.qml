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
    property int  spinDuration: 2000       // ms per turn
    property url animatedSource: ""        // GIF shown while `animated` is true
    property bool animated: false          // e.g. CAS playing -> tape.gif
    // Per-instance override; the phonebook packs four of these into a row and
    // needs them smaller than the drive slots do.
    property int iconSize: Theme.iconSize
    signal clicked

    implicitWidth: root.iconSize + 2 * Theme.btnPad
    implicitHeight: root.iconSize + 2 * Theme.btnPad
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
        width: root.iconSize
        height: root.iconSize
        source: root.source
        sourceSize.width: root.iconSize * 2
        sourceSize.height: root.iconSize * 2
        fillMode: Image.PreserveAspectFit
        smooth: true
        RotationAnimator on rotation {
            id: spinAnim
            running: root.spinning
            loops: Animation.Infinite
            from: 0; to: 360
            duration: root.spinDuration
        }
        onVisibleChanged: if (!visible) rotation = 0
        // Animators hand their config to the render thread when they start, so
        // a duration change while spinning is ignored until it restarts.
        Connections {
            target: root
            function onSpinDurationChanged() {
                if (!root.spinning) return
                // Restore the binding after the imperative restart, or the
                // animation can never be stopped again.
                spinAnim.running = false
                spinAnim.running = Qt.binding(function () { return root.spinning })
            }
        }
    }

    AnimatedImage {
        visible: root.animated && root.animatedSource != ""
        anchors.centerIn: parent
        width: root.iconSize
        height: root.iconSize
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
