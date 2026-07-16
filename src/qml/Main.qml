import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import "."

ApplicationWindow {
    id: win
    visible: true
    width: 420
    height: 900
    title: "AspeQt"

    Material.theme: Material.Light
    Material.primary: Theme.primary
    Material.accent: Theme.accent

    // ---- top bar (Material) ------------------------------------------------
    header: ToolBar {
        Material.foreground: "white"
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 4
            Label {
                text: "AspeQt"
                font.pixelSize: 20
                font.bold: true
                color: "white"
                Layout.fillWidth: true
            }
            ToolButton {
                text: "⋮"
                font.pixelSize: 22
                onClicked: mainMenu.open()
                Menu {
                    id: mainMenu
                    y: parent.height
                    MenuItem { text: qsTr("Options") }
                    MenuItem { text: qsTr("Log window") }
                    MenuItem { text: qsTr("Start emulation") }
                    MenuItem { text: qsTr("About") }
                }
            }
        }
    }

    // ---- body --------------------------------------------------------------
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // scrollable slot column: loader (pinned) + disk slots + add row
        ScrollView {
            id: slotScroll
            Layout.fillWidth: true
            Layout.leftMargin: 6
            Layout.rightMargin: 6
            Layout.fillHeight: false
            Layout.preferredHeight: Math.min(slotCol.implicitHeight + 12,
                                             win.height * 0.5)
            clip: true
            contentWidth: availableWidth      // clamp: never scroll horizontally
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical.policy: ScrollBar.AlwaysOff   // finger-scroll only

            ColumnLayout {
                id: slotCol
                width: slotScroll.availableWidth
                spacing: 6
                y: 6

                LoaderCard { Layout.fillWidth: true }

                Repeater {
                    model: app.drives
                    delegate: SlotCard {
                        required property var model
                        Layout.fillWidth: true
                        hwIndex: model.hwIndex
                        slotNumber: model.slotNumber
                        mounted: model.mounted
                        isFolder: model.isFolder
                        fileName: model.fileName
                        typeText: model.typeText
                        modified: model.modified
                        writeProtected: model.writeProtected
                        autoCommit: model.autoCommit
                        editOpen: model.editOpen
                        isBootSlot: model.isBootSlot
                        onRequestSwap: (fromHw, toHw) => app.swapSlots(fromHw, toHw)
                    }
                }

                // "+" add-slot row
                Rectangle {
                    visible: app.canAddSlot
                    Layout.fillWidth: true
                    implicitHeight: 46
                    radius: Theme.cardRadius
                    color: Theme.cardEmptyBg
                    border.width: 1
                    border.color: Theme.cardEmptyBorder
                    Image {
                        anchors.centerIn: parent
                        width: 28; height: 28
                        source: Theme.icon("actions/list-add.svg")
                        sourceSize.width: 56; sourceSize.height: 56
                    }
                    MouseArea { anchors.fill: parent; onClicked: app.addSlot() }
                }
            }
        }

        // log pane
        Frame {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 6
            Layout.rightMargin: 6
            Layout.topMargin: 4
            Layout.bottomMargin: 4
            padding: 0
            background: Rectangle { color: "white"; border.color: "#D0D0D0" }

            ScrollView {
                id: logScroll
                anchors.fill: parent
                clip: true
                TextArea {
                    readOnly: true
                    wrapMode: TextArea.Wrap
                    textFormat: TextArea.RichText
                    text: app.logHtml
                    font.pixelSize: 15
                    background: null
                    onTextChanged: logScroll.ScrollBar.vertical.position =
                                   1.0 - logScroll.ScrollBar.vertical.size
                }
            }
        }

        // bottom status bar: right cluster [speed][connect][printer][clear],
        // flat pixmap icons like the widget statusbar's permanent widgets.
        ToolBar {
            Layout.fillWidth: true
            Material.background: "#ECECEC"

            component StatusIcon : Item {
                id: si
                property url source
                property bool on: false
                property string tip: ""
                signal clicked
                implicitWidth: 38
                implicitHeight: 38
                Image {
                    anchors.centerIn: parent
                    width: 30; height: 30
                    source: si.source
                    sourceSize.width: 60; sourceSize.height: 60
                    opacity: si.on ? 1.0 : 0.85
                }
                MouseArea { anchors.fill: parent; onClicked: si.clicked() }
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 14
                spacing: 10
                Item { Layout.fillWidth: true }
                Label {
                    visible: app.sioRunning
                    text: app.statusText
                    color: "#404040"
                    font.pixelSize: 15
                    Layout.minimumWidth: 80
                    horizontalAlignment: Text.AlignRight
                }
                StatusIcon {
                    source: app.sioRunning ? Theme.icon("actions/media-playback-stop.svg")
                                           : Theme.icon("actions/media-playback-start.svg")
                    tip: app.sioRunning ? qsTr("Stop emulation") : qsTr("Start emulation")
                    onClicked: app.toggleSio()
                }
                StatusIcon {
                    source: Theme.icon("devices/printer.svg")
                    on: app.printerOn
                    tip: qsTr("Printer emulation")
                    onClicked: app.togglePrinter()
                }
                StatusIcon {
                    source: Theme.icon("actions/edit-clear.svg")
                    tip: qsTr("Clear messages")
                    onClicked: app.clearLog()
                }
            }
        }
    }
}
