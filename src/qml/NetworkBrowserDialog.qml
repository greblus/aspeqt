import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import "."

// Network file browser: connect to a TNFS/SFTP/FTP server and mount or download
// Atari images. Driven by the C++ NetworkBrowser exposed as `netbrowser`.
Popup {
    id: nb
    parent: Overlay.overlay
    x: 0
    y: 0
    width: parent ? parent.width : 420
    height: parent ? parent.height : 900
    modal: true
    padding: 0
    closePolicy: Popup.NoAutoClose

    // Shared height for the address field and its buttons (matches the phonebook).
    readonly property int rowH: 40

    // Within a session the previous connection stays alive across window
    // open/close (closing the window or mounting only hides the popup), so
    // reopening shows the same listing. A cold start (app restart) begins
    // disconnected and offers the favourite servers to pick from.
    function openBrowser() {
        open()
        syncAddress()
    }
    function syncAddress() {
        addr.editText = netbrowser.connected
            ? netbrowser.currentUrl
            : (netbrowser.favorites.length > 0 ? netbrowser.favorites[0]
               : (netbrowser.history.length > 0 ? netbrowser.history[0] : "tnfs://"))
    }

    // Address dropdown = favourites first, then recent history, de-duplicated.
    property var addressModel: {
        var out = [], seen = ({})
        var all = netbrowser.favorites.concat(netbrowser.history)
        for (var i = 0; i < all.length; ++i) {
            var u = all[i]
            if (u && !seen[u]) { seen[u] = 1; out.push(u) }
        }
        return out
    }

    // Bottom file filter, applied live over the current listing.
    property string fileQuery: ""
    property var filteredEntries: []
    function rebuildFilter() {
        var q = fileQuery.trim().toLowerCase()
        var src = netbrowser.entries
        if (q.length === 0) { filteredEntries = src; return }
        var out = []
        for (var i = 0; i < src.length; ++i)
            if (String(src[i].name).toLowerCase().indexOf(q) >= 0)
                out.push(src[i])
        filteredEntries = out
    }
    onFileQueryChanged: rebuildFilter()

    // Transient status line at the bottom (errors and "saved" notices).
    property string message: ""
    function notify(m) { message = m; msgTimer.restart() }
    Timer { id: msgTimer; interval: 5000; onTriggered: nb.message = "" }

    Connections {
        target: netbrowser
        function onError(message) { nb.notify(message) }
        function onMounted(localPath) { app.mountNetworkTemp(0, localPath); nb.close() }
        function onSaved(localPath) { nb.notify(qsTr("Saved to ") + localPath) }
        function onEntriesChanged() { nb.rebuildFilter() }
        function onPathChanged() { if (netbrowser.connected) nb.syncAddress() }
    }

    ConfirmDialog { id: nbConfirm }

    background: Rectangle { color: Material.background }

    contentItem: ColumnLayout {
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: nb.SafeArea.margins.top
            color: Theme.primary
        }

        // header: title
        ToolBar {
            Layout.fillWidth: true
            Material.primary: Theme.primary
            Material.foreground: "white"
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                Label {
                    text: qsTr("Network browser")
                    color: "white"
                    font.pixelSize: 20
                    font.bold: true
                    Layout.fillWidth: true
                }
            }
        }

        // address bar: URL (with history) + favourite + connect + refresh
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 8
            spacing: 6
            ComboBox {
                id: addr
                Layout.fillWidth: true
                Layout.preferredHeight: nb.rowH
                editable: true
                model: nb.addressModel
                font.pixelSize: 14
                onAccepted: netbrowser.open(editText)
                onActivated: (i) => { editText = nb.addressModel[i]; netbrowser.open(editText) }
                Component.onCompleted: nb.syncAddress()
            }
            SlotButton {
                iconSize: nb.rowH - 2 * Theme.btnPad
                source: Theme.icon("emblems/emblem-star.svg")
                tip: qsTr("Favourite this address")
                checked: netbrowser.favorites.indexOf(addr.editText) >= 0
                onClicked: netbrowser.toggleFavorite(addr.editText)
            }
            SlotButton {
                iconSize: nb.rowH - 2 * Theme.btnPad
                source: Theme.icon("emblems/emblem-arrow.svg")
                tip: qsTr("Connect")
                onClicked: netbrowser.open(addr.editText)
            }
            SlotButton {
                iconSize: nb.rowH - 2 * Theme.btnPad
                source: Theme.icon("actions/view-refresh.svg")
                tip: qsTr("Refresh")
                enabledState: netbrowser.connected
                onClicked: netbrowser.refresh()
            }
        }

        // parent-directory row: ".." styled like a list entry, its icon aligned
        // with the file icons below. The location itself lives in the address bar.
        ItemDelegate {
            Layout.fillWidth: true
            Layout.leftMargin: 6
            Layout.rightMargin: 6
            visible: netbrowser.connected
            // At the server root, going up disconnects and returns to the
            // favourites list; deeper down it climbs one directory.
            onClicked: {
                if (netbrowser.path === "/" || netbrowser.path === "")
                    netbrowser.close()
                else
                    netbrowser.up()
            }
            contentItem: RowLayout {
                spacing: 10
                Image {
                    source: Theme.icon("actions/go-up.svg")
                    sourceSize.width: 28; sourceSize.height: 28
                }
                Label {
                    text: ".."
                    Layout.fillWidth: true
                    font.bold: true
                }
            }
        }

        BusyIndicator {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 6
            running: netbrowser.busy
            visible: netbrowser.busy
        }

        // listing area: files when connected, favourite servers when not
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 6

            // file listing
            ListView {
                id: list
                anchors.fill: parent
                visible: netbrowser.connected
                clip: true
                model: nb.filteredEntries
                ScrollBar.vertical: ScrollBar {}

                delegate: ItemDelegate {
                    width: list.width
                    enabled: !netbrowser.busy
                    onClicked: {
                        if (modelData.isDir) {
                            netbrowser.enter(modelData.name)
                        } else if (modelData.isDisk) {
                            nbConfirm.ask(qsTr("Mount"),
                                qsTr("Mount \"%1\" in drive 1?").arg(modelData.name),
                                function (yes) { if (yes) netbrowser.mount(modelData.name) })
                        } else {
                            nbConfirm.ask(qsTr("Download"),
                                qsTr("Download \"%1\"?").arg(modelData.name),
                                function (yes) { if (yes) netbrowser.save(modelData.name) })
                        }
                    }

                    contentItem: RowLayout {
                        spacing: 10
                        Image {
                            source: Theme.icon(modelData.isDir ? "places/folder.svg"
                                            : modelData.isDisk ? "devices/drive-removable-media.svg"
                                                               : "mimetypes/text-x-generic.svg")
                            sourceSize.width: 28; sourceSize.height: 28
                        }
                        Label {
                            text: modelData.name
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                            font.bold: modelData.isDir
                        }
                        // mount (disk images only) + download (any file)
                        SlotButton {
                            iconSize: 26
                            visible: modelData.isDisk
                            source: Theme.icon("devices/drive-harddisk.svg")
                            tip: qsTr("Mount in drive 1")
                            onClicked: netbrowser.mount(modelData.name)
                        }
                        SlotButton {
                            iconSize: 26
                            visible: !modelData.isDir
                            source: Theme.icon("actions/document-save.svg")
                            tip: qsTr("Download")
                            onClicked: netbrowser.save(modelData.name)
                        }
                    }
                }

                Label {
                    anchors.centerIn: parent
                    visible: list.count === 0 && !netbrowser.busy
                    text: nb.fileQuery.length > 0 ? qsTr("(no match)") : qsTr("(empty)")
                    opacity: 0.6
                }
            }

            // favourite servers (shown before connecting)
            ListView {
                id: favList
                anchors.fill: parent
                visible: !netbrowser.connected
                clip: true
                model: netbrowser.favorites
                ScrollBar.vertical: ScrollBar {}

                delegate: ItemDelegate {
                    width: favList.width
                    onClicked: netbrowser.open(modelData)

                    contentItem: RowLayout {
                        spacing: 10
                        Image {
                            source: Theme.icon("places/network-server.svg")
                            sourceSize.width: 28; sourceSize.height: 28
                        }
                        Label {
                            text: modelData
                            Layout.fillWidth: true
                            elide: Text.ElideMiddle
                        }
                        SlotButton {
                            iconSize: 26
                            source: Theme.icon("emblems/emblem-star.svg")
                            checked: true
                            tip: qsTr("Remove from favourites")
                            onClicked: netbrowser.toggleFavorite(modelData)
                        }
                    }
                }

                Label {
                    anchors.centerIn: parent
                    width: parent.width - 40
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    visible: favList.count === 0 && !netbrowser.busy
                    text: qsTr("No favourite servers yet. Enter a tnfs://, sftp:// or "
                             + "ftp:// address, connect, and tap the star to save it.")
                    opacity: 0.6
                }
            }
        }

        // file filter (dynamic, like the phonebook search)
        Rectangle {
            Layout.fillWidth: true
            visible: netbrowser.connected
            color: "#ECECEC"
            implicitHeight: filterRow.implicitHeight + 10
            RowLayout {
                id: filterRow
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 4
                TextField {
                    id: fileSearch
                    Layout.fillWidth: true
                    Layout.preferredHeight: 34
                    placeholderText: qsTr("Filter files…")
                    inputMethodHints: Qt.ImhNoAutoUppercase | Qt.ImhNoPredictiveText
                    font.pixelSize: 14
                    onTextChanged:        nb.fileQuery = text + preeditText
                    onPreeditTextChanged: nb.fileQuery = text + preeditText
                }
                SlotButton {
                    iconSize: 24
                    source: Theme.icon("actions/edit-clear.svg")
                    tip: qsTr("Clear filter")
                    enabledState: nb.fileQuery.length > 0
                    onClicked: { fileSearch.clear(); nb.fileQuery = "" }
                }
            }
        }

        // transient status / error banner
        Rectangle {
            Layout.fillWidth: true
            visible: nb.message.length > 0
            color: Material.color(Material.Grey, Material.Shade800)
            implicitHeight: msgLabel.implicitHeight + 16
            Label {
                id: msgLabel
                anchors.fill: parent
                anchors.margins: 8
                text: nb.message
                color: "white"
                wrapMode: Text.WordWrap
                elide: Text.ElideMiddle
            }
        }

        // footer: close (same style/position as the phonebook's)
        Frame {
            Layout.fillWidth: true
            padding: 8
            background: Rectangle { color: "#ECECEC" }
            RowLayout {
                anchors.fill: parent
                Item { Layout.fillWidth: true }
                Button {
                    text: qsTr("Close")
                    highlighted: true
                    onClicked: nb.close()
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: nb.SafeArea.margins.bottom
            color: "#ECECEC"
        }
    }
}
