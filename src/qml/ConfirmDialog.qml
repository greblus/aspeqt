import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import "."

// Yes/No confirmation, asked before the engine is called rather than from
// inside it: a modal widget dialog run from engine code blocks it in a nested
// event loop, which is what the QML delegates and the SIO path cannot take.
Dialog {
    id: root

    property var _cb: null

    function ask(titleText, bodyText, cb) {
        _cb = cb
        title = titleText
        body.text = bodyText
        open()
    }

    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(parent ? parent.width - 48 : 320, 420)
    modal: true
    closePolicy: Popup.NoAutoClose
    standardButtons: Dialog.Yes | Dialog.No

    Material.accent: Theme.primary

    contentItem: Label {
        id: body
        wrapMode: Text.Wrap
        font.pixelSize: 15
        leftPadding: 8
        rightPadding: 8
        topPadding: 8
    }

    function _deliver(answer) {
        var cb = _cb
        _cb = null
        if (cb)
            cb(answer)
    }

    onAccepted: _deliver(true)
    onRejected: _deliver(false)
}
