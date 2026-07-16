import QtQuick
import QtQuick.Layouts
import "."

// One disk drive slot. Reproduces MainWindow::layoutSlotFrame:
//   [ number badge | (icon row) / (name + type row) ]
// Six icons: mount-disk, mount-folder, save, auto-commit, edit  ...  eject.
Rectangle {
    id: card

    // model roles (set by the delegate)
    property int  hwIndex: 0
    property int  slotNumber: 1
    property bool mounted: false
    property bool isFolder: false
    property string fileName: ""
    property string typeText: ""
    property bool modified: false
    property bool writeProtected: false
    property bool autoCommit: false
    property bool editOpen: false
    property bool isBootSlot: false

    // derived affordances (see MainWindow::deviceStatusChanged)
    readonly property bool saveIsDos:   isFolder
    readonly property bool saveEnabled: mounted && (isFolder || (modified && !autoCommit))
    readonly property bool ejectIsRemove: !mounted

    radius: Theme.cardRadius
    color: mounted ? Theme.cardActiveBg : Theme.cardEmptyBg
    border.width: 1
    border.color: mounted ? Theme.cardActiveBorder : Theme.cardEmptyBorder
    implicitHeight: row.implicitHeight + 2 * 6

    RowLayout {
        id: row
        anchors.fill: parent
        anchors.leftMargin: Theme.cardMargin
        anchors.rightMargin: Theme.cardMargin
        anchors.topMargin: 6
        anchors.bottomMargin: 6
        spacing: Theme.gap

        Badge {
            text: card.slotNumber
            Layout.alignment: Qt.AlignVCenter
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            // icon row
            RowLayout {
                Layout.fillWidth: true
                spacing: 4
                Item { Layout.fillWidth: true }   // left stretch (centres the five)

                SlotButton {
                    source: Theme.icon("devices/drive-optical.svg")
                    tip: qsTr("Mount disk image")
                    onClicked: app.mountDisk(card.hwIndex)
                }
                SlotButton {
                    source: Theme.icon("places/folder.svg")
                    tip: qsTr("Mount folder image")
                    onClicked: app.mountFolder(card.hwIndex)
                }
                // save / install-DOS
                Item {
                    implicitWidth: saveBtn.implicitWidth
                    implicitHeight: saveBtn.implicitHeight
                    SlotButton {
                        id: saveBtn
                        anchors.fill: parent
                        source: card.saveIsDos ? Theme.icon("devices/drive-harddisk.svg")
                                               : Theme.icon("devices/media-floppy.svg")
                        enabledState: card.saveEnabled
                        tip: card.saveIsDos ? qsTr("Install high-speed DOS into this folder")
                                            : qsTr("Save disk")
                        onClicked: app.save(card.hwIndex)
                    }
                    // "DOS" caption overlay for the folder hard-disk icon
                    Text {
                        visible: card.saveIsDos
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.verticalCenterOffset: 3
                        text: "DOS"
                        font.pixelSize: 9
                        font.bold: true
                        color: "#333333"
                    }
                }
                SlotButton {
                    source: Theme.icon("actions/document-save-as.svg")
                    checked: card.autoCommit
                    enabledState: card.mounted && !card.isFolder
                    tip: qsTr("Auto-commit")
                    onClicked: app.toggleAutoCommit(card.hwIndex)
                }
                SlotButton {
                    source: Theme.icon("apps/system-file-manager.svg")
                    checked: card.editOpen
                    enabledState: card.mounted
                    tip: qsTr("Disk explorer")
                    onClicked: app.openEditor(card.hwIndex)
                }

                Item { Layout.fillWidth: true }   // right stretch
                Item { width: 6 }

                // eject (mounted) / remove-slot (empty)
                SlotButton {
                    source: card.ejectIsRemove ? Theme.icon("emblems/emblem-unreadable.svg")
                                               : Theme.icon("actions/media-eject.svg")
                    tip: card.ejectIsRemove ? qsTr("Remove slot") : qsTr("Eject")
                    onClicked: card.ejectIsRemove ? app.removeSlot(card.hwIndex)
                                                  : app.eject(card.hwIndex)
                }
            }

            // name + type row
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.gap

                Text {
                    text: card.mounted ? card.fileName
                                       : qsTr("Mount a disk image or folder.")
                    font.bold: card.mounted
                    font.italic: !card.mounted
                    font.pixelSize: card.mounted ? 15 : 12
                    color: !card.mounted ? Theme.placeholder
                                         : (card.isFolder ? Theme.nameFolder : Theme.nameDark)
                    elide: Text.ElideRight
                    Layout.maximumWidth: parent.width * 0.6
                }
                Item { Layout.fillWidth: true }
                Text {
                    visible: card.mounted && card.typeText.length > 0
                    text: card.typeText
                    color: Theme.typeGrey
                    font.pixelSize: 13
                    elide: Text.ElideRight
                }
                Item { width: 6 }
            }
        }
    }
}
