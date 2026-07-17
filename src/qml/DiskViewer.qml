import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import "."

// Atari disk explorer, in QML. Lists the mounted image's filesystem (reusing the
// engine's AtariFileSystem via app.disk*()), with directory navigation and the
// same operations as the widget dialog: add / extract / text-conversion / delete.
Popup {
    id: dv
    parent: Overlay.overlay
    x: 0
    y: 0
    width: parent ? parent.width : 420
    height: parent ? parent.height : 900
    modal: true
    padding: 0
    closePolicy: Popup.NoAutoClose

    property var entries: []
    property string path: ""
    property bool canParent: false
    property bool readOnly: false
    property int fsType: 0
    property var sel: []           // selected file rows in the current directory

    function openFor(hw) { app.diskOpen(hw); refresh(); open() }
    function refresh() {
        entries = app.diskEntries()
        path = app.diskPath()
        canParent = app.diskCanParent()
        readOnly = app.diskReadOnly()
        fsType = app.diskFsType()
        fsCombo.currentIndex = fsType
        sel = []
    }
    function isSel(i) { return sel.indexOf(i) >= 0 }
    function toggleSel(i) {
        var a = sel.slice()
        var k = a.indexOf(i)
        if (k >= 0) a.splice(k, 1); else a.push(i)
        sel = a
    }
    onClosed: app.diskClose()

    background: Rectangle { color: Material.background }

    contentItem: ColumnLayout {
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: dv.SafeArea.margins.top
            color: Theme.primary
        }

        // header: up | path | close
        ToolBar {
            Layout.fillWidth: true
            Material.primary: Theme.primary
            Material.foreground: "white"
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 4
                anchors.rightMargin: 4
                spacing: 4
                ToolButton {
                    // Only hierarchical filesystems (MyDOS/SpartaDOS) have
                    // subdirectories, so only show "up" once we're inside one.
                    visible: dv.canParent
                    icon.source: Theme.icon("actions/go-up.svg")
                    icon.color: "transparent"
                    icon.width: 26; icon.height: 26
                    onClicked: { app.diskParent(); dv.refresh() }
                }
                Label {
                    text: (dv.path.length > 0 ? dv.path : qsTr("Disk explorer"))
                          + (dv.readOnly ? "  (" + qsTr("read only") + ")" : "")
                    color: "white"
                    font.pixelSize: 19
                    font.bold: true
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }
        }

        // operations toolbar
        ToolBar {
            Layout.fillWidth: true
            Material.background: "#ECECEC"
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 4

                SlotButton {
                    source: Theme.icon("actions/list-add.svg")
                    tip: qsTr("Add files")
                    enabledState: !dv.readOnly
                    onClicked: { if (app.diskAddFiles()) dv.refresh() }
                }
                SlotButton {
                    source: Theme.icon("actions/document-save-as.svg")
                    tip: qsTr("Extract selected files")
                    enabledState: !dv.readOnly && dv.sel.length > 0
                    onClicked: app.diskExtract(dv.sel)
                }
                SlotButton {
                    source: Theme.icon("mimetypes/text-x-generic.svg")
                    tip: qsTr("Text conversion")
                    checked: textConv.on
                    enabledState: !dv.readOnly
                    onClicked: { textConv.on = !textConv.on; app.diskSetTextConversion(textConv.on) }
                }
                QtObject { id: textConv; property bool on: false }

                Item { Layout.fillWidth: true }

                SlotButton {
                    source: Theme.icon("actions/edit-delete.svg")
                    tip: qsTr("Delete selected files")
                    enabledState: !dv.readOnly && dv.sel.length > 0
                    onClicked: { if (app.diskDelete(dv.sel)) dv.refresh() }
                }
            }
        }

        // file list
        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: dv.entries
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            delegate: ItemDelegate {
                required property var modelData
                required property int index
                width: ListView.view.width
                height: 58
                highlighted: !modelData.isDir && dv.isSel(index)
                onClicked: {
                    if (modelData.isDir) { app.diskEnter(index); dv.refresh() }
                    else dv.toggleSel(index)
                }

                contentItem: RowLayout {
                    spacing: 12
                    Image {
                        source: modelData.isDir ? Theme.icon("places/folder.svg")
                                                : Theme.icon("mimetypes/text-x-generic.svg")
                        sourceSize.width: 64; sourceSize.height: 64
                        Layout.preferredWidth: 32
                        Layout.preferredHeight: 32
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1
                        Label {
                            text: modelData.name
                            font.bold: true
                            font.pixelSize: 16
                            color: Theme.nameDark
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Label {
                            text: (modelData.isDir ? qsTr("folder") : (modelData.size + " B"))
                                  + (modelData.attrs && modelData.attrs.length > 0 ? "   " + modelData.attrs : "")
                                  + (modelData.date && modelData.date.length > 0 ? "   " + modelData.date : "")
                            color: Theme.typeGrey
                            font.pixelSize: 13
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }
                    Image {
                        visible: modelData.isDir
                        source: Theme.icon("actions/go-next.svg")
                        sourceSize.width: 44; sourceSize.height: 44
                        Layout.preferredWidth: 22
                        Layout.preferredHeight: 22
                        opacity: 0.6
                    }
                }
            }

            Label {
                anchors.centerIn: parent
                visible: list.count === 0
                text: qsTr("Empty or unrecognised filesystem.")
                color: Theme.typeGrey
                font.pixelSize: 15
            }
        }

        // bottom bar — filesystem-type override (auto-detected by default);
        // same style as the main window / log window bottom bars.
        ToolBar {
            Layout.fillWidth: true
            Material.background: "#ECECEC"
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 8
                spacing: 8
                ComboBox {
                    id: fsCombo
                    Layout.preferredWidth: 160
                    Layout.preferredHeight: 34
                    font.pixelSize: 13
                    model: [qsTr("No file system"), "Atari Dos 1.0", "Atari Dos 2.0",
                            "Atari Dos 2.5", "MyDos", "SpartaDos"]
                    onActivated: (index) => { app.diskSetFsType(index); dv.refresh() }
                }
                Item { Layout.fillWidth: true }
                Button {
                    text: qsTr("Close")
                    highlighted: true
                    onClicked: dv.close()
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: dv.SafeArea.margins.bottom
            color: "#ECECEC"
        }
    }
}
