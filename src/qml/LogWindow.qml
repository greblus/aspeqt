import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import "."

// Full-screen log viewer, modelled on the QtWidgets LogDisplayDialog: a bottom
// bar with a disk filter, a search field (highlights matches, "next" steps
// through them) and a close button. Opened by double-tapping the inline log.
Popup {
    id: logWin
    parent: Overlay.overlay
    x: 0
    y: 0
    width: parent ? parent.width : 420
    height: parent ? parent.height : 900
    modal: true
    padding: 0
    closePolicy: Popup.NoAutoClose

    property var matches: []
    property int matchIdx: -1

    function diskModel() {
        var a = [{ label: qsTr("ALL"), no: 0 }]
        for (var i = 1; i <= 15; ++i)
            a.push({ label: qsTr("Disk %1").arg(i), no: i })
        return a
    }

    function escapeRe(s) { return s.replace(/[.*+?^${}()|[\]\\]/g, "\\$&") }

    // HTML for the log: keep only the selected disk's lines, highlight the needle
    // inside text (never inside tags).
    function buildHtml() {
        var lines = app.logHtml.split("<br>")
        var no = diskCombo.currentValue
        var needle = search.text
        var re = needle.length > 0 ? new RegExp("(" + escapeRe(needle) + ")", "gi") : null
        var out = []
        for (var i = 0; i < lines.length; ++i) {
            var ln = lines[i]
            if (ln.length === 0)
                continue
            var plain = ln.replace(/<[^>]*>/g, "")
            if (no > 0) {
                var m = plain.match(/\[[^\]]*?(\d+)\]/)
                if (!m || parseInt(m[1]) !== no)
                    continue
            }
            if (re)
                ln = ln.replace(/(>)([^<]+)(<)/g, function (_, a, txt, b) {
                    return a + txt.replace(re, '<span style="background:#FFE08A">$1</span>') + b
                })
            out.push(ln)
        }
        return out.join("<br>")
    }

    function updateMatches() {
        var arr = []
        matchIdx = -1
        var needle = search.text.toLowerCase()
        if (needle.length > 0) {
            var plain = logEdit.getText(0, logEdit.length).toLowerCase()
            var idx = plain.indexOf(needle)
            while (idx >= 0) { arr.push(idx); idx = plain.indexOf(needle, idx + needle.length) }
        }
        matches = arr
        if (arr.length > 0) { matchIdx = 0; scrollToMatch() }
    }

    function scrollToMatch() {
        if (matchIdx < 0 || matchIdx >= matches.length)
            return
        var r = logEdit.positionToRectangle(matches[matchIdx])
        var target = r.y - logFlick.height / 2 + r.height / 2
        var maxY = Math.max(0, logEdit.implicitHeight - logFlick.height)
        logFlick.contentY = Math.max(0, Math.min(target, maxY))
    }

    function nextMatch() {
        if (matches.length === 0) { updateMatches(); return }
        matchIdx = (matchIdx + 1) % matches.length
        scrollToMatch()
    }

    background: Rectangle { color: Material.background }

    contentItem: ColumnLayout {
        spacing: 0

        // top safe-area strip (status bar) — ApplicationWindow insets its header
        // automatically, but a Popup does not, so match it here.
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: logWin.SafeArea.margins.top
            color: Theme.primary
        }

        // title bar — same layout as the main window header
        ToolBar {
            Layout.fillWidth: true
            Material.primary: Theme.primary
            Material.foreground: "white"
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 4
                Label {
                    text: qsTr("AspeQt Log View")
                    color: "white"
                    font.pixelSize: 20
                    font.bold: true
                    Layout.fillWidth: true
                }
            }
        }

        // log body — vertical scroll only
        Flickable {
            id: logFlick
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 8
            clip: true
            contentWidth: width
            contentHeight: logEdit.implicitHeight
            flickableDirection: Flickable.VerticalFlick
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            TextEdit {
                id: logEdit
                width: logFlick.width
                readOnly: true
                selectByMouse: false
                wrapMode: TextEdit.Wrap
                textFormat: TextEdit.RichText
                font.pixelSize: 15
                text: logWin.buildHtml()
                onTextChanged: Qt.callLater(logWin.updateMatches)
            }
        }

        // bottom controls, two rows (each ~30% shorter than default):
        //   1: search field (full width)
        //   2: filter label + disk combo ......... next | close
        ToolBar {
            Layout.fillWidth: true
            Material.background: "#ECECEC"
            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 6
                anchors.topMargin: 2
                anchors.bottomMargin: 2
                spacing: 4

                TextField {
                    id: search
                    Layout.fillWidth: true
                    Layout.preferredHeight: 34
                    font.pixelSize: 14
                    placeholderText: qsTr("Search text…")
                    onAccepted: logWin.nextMatch()
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 44   // match the main window's status bar
                    spacing: 6
                    Label { text: qsTr("Filter log by:") }
                    ComboBox {
                        id: diskCombo
                        model: logWin.diskModel()
                        textRole: "label"
                        valueRole: "no"
                        font.pixelSize: 13
                        Layout.preferredWidth: 108
                        Layout.preferredHeight: 34
                    }
                    Item { Layout.fillWidth: true }
                    ToolButton {
                        icon.source: Theme.icon("actions/go-next.svg")
                        icon.color: "transparent"
                        icon.width: 26; icon.height: 26
                        Layout.preferredHeight: 34
                        onClicked: logWin.nextMatch()
                    }
                    ToolButton {
                        icon.source: Theme.icon("actions/system-log-out.svg")
                        icon.color: "transparent"
                        icon.width: 26; icon.height: 26
                        Layout.preferredHeight: 34
                        onClicked: logWin.close()
                    }
                }
            }
        }

        // bottom safe-area strip (navigation bar)
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: logWin.SafeArea.margins.bottom
            color: "#ECECEC"
        }
    }
}
