import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import "."

// BBS phonebook for the R: device: the list the Atari dials by name (ATDT
// <name>) plus the credentials the ESC-U / ESC-P macros type. Stored as XML in
// the AspeQt-2k26 format, so the file can be shared with that program.
Popup {
    id: pb
    parent: Overlay.overlay
    x: 0
    y: 0
    width: parent ? parent.width : 420
    height: parent ? parent.height : 900
    modal: true
    padding: 0
    closePolicy: Popup.NoAutoClose

    // Working copy; written back only on Save.
    property var entries: []
    property bool dirty: false
    property int editIndex: -1
    // Re-read on open: a binding to app.phonebookPath() would evaluate once at
    // startup and miss the path being set in Options afterwards.
    property string bookPath: ""
    property bool favOnly: false
    // Fed by the search field on every keystroke.
    property string query: ""
    // Smaller than the drive slots': a phonebook row carries four of them.
    readonly property int iconPx: 25

    // Filtered view, rebuilt explicitly whenever the query, the favourites
    // filter or the list itself changes. Each item carries the row's index in
    // `entries`, because edit/remove/star must act on the real entry, not on
    // the filtered position.
    property var view: []

    function rebuildView() {
        var q = query.trim().toLowerCase()
        var out = []
        for (var i = 0; i < entries.length; ++i) {
            var e = entries[i]
            if (favOnly && !e.favourite)
                continue
            if (q.length > 0
                && e.name.toLowerCase().indexOf(q) < 0
                && e.ip.toLowerCase().indexOf(q) < 0)
                continue
            out.push({ "e": e, "idx": i })
        }
        view = out
    }

    onQueryChanged:   rebuildView()
    onFavOnlyChanged: rebuildView()
    onEntriesChanged: rebuildView()

    function setFavourite(i, on) {
        var a = entries.slice()
        var e = {}
        for (var k in a[i]) e[k] = a[i][k]
        e.favourite = on
        a[i] = e
        entries = a
        dirty = true
    }

    function openBook() {
        bookPath = app.phonebookPath()
        entries = app.phonebookEntries()
        dirty = false
        open()
    }
    function commit() {
        if (app.phonebookSave(entries)) {
            dirty = false
            app.toast(qsTr("Phonebook saved."))
        } else {
            app.toast(qsTr("Could not save the phonebook, see the log."))
        }
    }
    function removeAt(i) {
        var a = entries.slice()
        a.splice(i, 1)
        entries = a
        dirty = true
    }

    ConfirmDialog { id: pbConfirm }

    background: Rectangle { color: Material.background }

    contentItem: ColumnLayout {
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: pb.SafeArea.margins.top
            color: Theme.primary
        }

        ToolBar {
            Layout.fillWidth: true
            Material.primary: Theme.primary
            Material.foreground: "white"
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 4
                Label {
                    text: qsTr("Phonebook") + (pb.dirty ? " *" : "")
                    color: "white"
                    font.pixelSize: 20
                    font.bold: true
                    Layout.fillWidth: true
                }
            }
        }

        // operations toolbar -- same style as the disk explorer's
        ToolBar {
            Layout.fillWidth: true
            Material.background: "#ECECEC"
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 4
                SlotButton {
                    iconSize: pb.iconPx
                    source: Theme.icon("actions/contact-new.svg")
                    tip: qsTr("Add a BBS")
                    onClicked: editor.openNew()
                }
                SlotButton {
                    iconSize: pb.iconPx
                    source: Theme.icon("emblems/emblem-star.svg")
                    tip: qsTr("Show favourites only")
                    checked: pb.favOnly
                    onClicked: pb.favOnly = !pb.favOnly
                }
                TextField {
                    id: search
                    Layout.fillWidth: true
                    Layout.preferredHeight: 34
                    inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
                    font.pixelSize: 14
                    // Filter as the user types. Android keyboards compose a
                    // word before committing it, and until they do the letters
                    // live in preeditText while `text` stays behind -- which is
                    // why filtering used to wait for a space.
                    onTextChanged:        pb.query = text + preeditText
                    onPreeditTextChanged: pb.query = text + preeditText
                }
                SlotButton {
                    iconSize: pb.iconPx
                    source: Theme.icon("actions/edit-clear.svg")
                    tip: qsTr("Clear search")
                    enabledState: pb.query.length > 0
                    onClicked: { search.clear(); pb.query = "" }
                }
            }
        }

        // result count / empty-filter hint
        Label {
            Layout.fillWidth: true
            Layout.leftMargin: 12
            Layout.topMargin: 2
            visible: pb.bookPath.length > 0
            text: pb.view.length === pb.entries.length
                  ? qsTr("%1 entries").arg(pb.entries.length)
                  : qsTr("%1 of %2 entries").arg(pb.view.length).arg(pb.entries.length)
            color: Theme.typeGrey
            font.pixelSize: 12
        }

        // No file chosen yet -> the list would silently stay empty.
        Label {
            visible: pb.bookPath.length === 0
            Layout.fillWidth: true
            Layout.margins: 14
            text: qsTr("No phonebook file is set yet. In Options, either choose a file or "
                     + "press \"Use the bundled BBS list\".")
            color: Theme.typeGrey
            wrapMode: Text.WordWrap
        }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: pb.view
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            delegate: ItemDelegate {
                required property var modelData
                // The entry and its index in pb.entries (not the filtered row).
                readonly property var entry: modelData.e
                readonly property int entryIndex: modelData.idx
                width: ListView.view.width
                height: 62
                onClicked: editor.openAt(entryIndex)

                contentItem: RowLayout {
                    spacing: 8
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1
                        Label {
                            text: entry.name.length > 0 ? entry.name : qsTr("(unnamed)")
                            font.bold: true
                            font.pixelSize: 16
                            color: Theme.nameDark
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Label {
                            text: entry.ip + ":" + entry.port
                                  + (entry.login.length > 0 ? "   " + entry.login : "")
                            color: Theme.typeGrey
                            font.pixelSize: 13
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }
                    SlotButton {
                        iconSize: pb.iconPx
                        source: Theme.icon("emblems/emblem-star.svg")
                        tip: entry.favourite ? qsTr("Remove from favourites")
                                             : qsTr("Add to favourites")
                        checked: entry.favourite
                        onClicked: pb.setFavourite(entryIndex, !entry.favourite)
                    }
                    SlotButton {
                        iconSize: pb.iconPx
                        source: Theme.icon("actions/document-properties.svg")
                        tip: qsTr("Edit")
                        onClicked: editor.openAt(entryIndex)
                    }
                    SlotButton {
                        iconSize: pb.iconPx
                        source: Theme.icon("actions/edit-delete.svg")
                        tip: qsTr("Remove")
                        onClicked: pbConfirm.ask(qsTr("Confirmation"),
                            qsTr("Remove \"%1\" from the phonebook?").arg(entry.name),
                            function (yes) { if (yes) pb.removeAt(entryIndex) })
                    }
                    SlotButton {
                        iconSize: pb.iconPx
                        source: Theme.icon("actions/go-jump.svg")
                        tip: qsTr("Dial")
                        onClicked: {
                            app.phonebookDial(entry)
                            pb.close()
                        }
                    }
                }
            }

            Label {
                anchors.centerIn: parent
                width: parent.width - 40
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                visible: list.count === 0 && pb.bookPath.length > 0
                text: pb.entries.length === 0
                      ? qsTr("Empty phonebook. Use the add button to enter a BBS.")
                      : qsTr("Nothing matches the current search or filter.")
                color: Theme.typeGrey
                font.pixelSize: 15
            }
        }

        Frame {
            Layout.fillWidth: true
            padding: 8
            background: Rectangle { color: "#ECECEC" }
            RowLayout {
                anchors.fill: parent
                Item { Layout.fillWidth: true }
                Button {
                    text: qsTr("Close")
                    flat: true
                    onClicked: {
                        if (!pb.dirty) { pb.close(); return }
                        pbConfirm.askSave(qsTr("Phonebook"),
                            qsTr("Save the changes to the phonebook?"),
                            function (answer) {
                                if (answer === "cancel") return
                                if (answer === "save") pb.commit()
                                pb.close()
                            })
                    }
                }
                Button {
                    text: qsTr("Save")
                    highlighted: true
                    onClicked: pb.commit()
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: pb.SafeArea.margins.bottom
            color: "#ECECEC"
        }
    }

    // ---- entry editor -----------------------------------------------------
    Dialog {
        id: editor
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(parent ? parent.width - 32 : 380, 460)
        modal: true
        title: editIndex < 0 ? qsTr("New BBS") : qsTr("Edit BBS")
        standardButtons: Dialog.Ok | Dialog.Cancel

        function openNew() {
            pb.editIndex = -1
            fName.text = ""; fHost.text = ""; fPort.text = "23"
            fLogin.text = ""; fPass.text = ""; fProto.currentIndex = 0
            open()
        }
        function openAt(i) {
            pb.editIndex = i
            var e = pb.entries[i]
            fName.text = e.name; fHost.text = e.ip; fPort.text = e.port
            fLogin.text = e.login; fPass.text = e.password
            // Exact match: "SSH-SHELL" also starts with "SSH", so a prefix test
            // would land it on the wrong entry.
            var p = (e.protocol || "").toUpperCase()
            fProto.currentIndex = (p === "SSH-SHELL") ? 2 : (p.indexOf("SSH") === 0 ? 1 : 0)
            open()
        }

        onAccepted: {
            var e = {
                "name": fName.text.trim(),
                "ip": fHost.text.trim(),
                "port": parseInt(fPort.text) || 23,
                "protocol": fProto.currentText,
                "login": fLogin.text,
                "password": fPass.text,
                // Preserve the star; editing must not silently unfavourite.
                "favourite": (pb.editIndex >= 0
                              && pb.entries[pb.editIndex].favourite) === true
            }
            var a = pb.entries.slice()
            if (pb.editIndex < 0) a.push(e); else a[pb.editIndex] = e
            pb.entries = a
            pb.dirty = true
        }

        ColumnLayout {
            anchors.fill: parent
            spacing: 4

            Label { text: qsTr("Name (used by ATDT <name>):"); font.bold: true }
            TextField { id: fName; Layout.fillWidth: true; placeholderText: qsTr("Basement BBS") }

            Label { text: qsTr("Address:"); font.bold: true }
            RowLayout {
                Layout.fillWidth: true
                TextField {
                    id: fHost
                    Layout.fillWidth: true
                    placeholderText: "bbs.example.com"
                    inputMethodHints: Qt.ImhUrlCharactersOnly | Qt.ImhNoAutoUppercase
                }
                TextField {
                    id: fPort
                    Layout.preferredWidth: 78
                    text: "23"
                    horizontalAlignment: Text.AlignHCenter
                    inputMethodHints: Qt.ImhDigitsOnly
                    validator: IntValidator { bottom: 1; top: 65535 }
                }
            }

            Label { text: qsTr("Protocol:"); font.bold: true }
            ComboBox {
                id: fProto
                Layout.fillWidth: true
                // SSH-SHELL asks for the host key and the credentials on the
                // Atari instead of storing them, so the entry keeps only host+port.
                model: ["TELNET", "SSH", "SSH-SHELL"]
                // Nudge the port to the SSH default when it is still the telnet one.
                onActivated: if (currentText !== "TELNET" && fPort.text === "23") fPort.text = "22"
            }

            // SSH-SHELL types its credentials on the Atari, so there is nothing
            // to store -- and nothing to leak from the phonebook file.
            readonly property bool wantsCreds: fProto.currentText !== "SSH-SHELL"

            Label {
                text: qsTr("Login (ESC-U):"); font.bold: true
                visible: parent.wantsCreds
            }
            TextField {
                id: fLogin
                visible: parent.wantsCreds
                Layout.fillWidth: true
                inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
            }

            Label {
                text: qsTr("Password (ESC-P):"); font.bold: true
                visible: parent.wantsCreds
            }
            TextField {
                id: fPass
                visible: parent.wantsCreds
                Layout.fillWidth: true
                inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
            }
            Label {
                text: parent.wantsCreds
                      ? qsTr("Stored as plain text in the phonebook file.")
                      : qsTr("Asked for on the Atari at dial time; nothing is stored.")
                color: Theme.typeGrey
                font.pixelSize: 12
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }
    }
}
