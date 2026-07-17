import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import "."

// Create-a-disk-image window, in the Options-window style (full-screen popup with
// a scrollable body and a bottom Cancel/Create bar). Mirrors CreateImageDialog.
Popup {
    id: dlg
    parent: Overlay.overlay
    x: 0
    y: 0
    width: parent ? parent.width : 420
    height: parent ? parent.height : 900
    modal: true
    padding: 0
    closePolicy: Popup.NoAutoClose

    readonly property var sizes: [128, 256, 512, 8192]
    property bool custom: false

    function sectorSize() { return sizes[densityCombo.currentIndex] }

    function capacity() {
        var sectors = parseInt(sectorsField.text) || 0
        var size = sectors * sectorSize()
        if (densityCombo.currentIndex === 1) {           // 256 bytes/sector
            if (sectors <= 3) size = sectors * 128
            else              size -= 384
        }
        var sizeK = Math.floor((size + 512) / 1024)
        return qsTr("Total image capacity: %1 bytes (%2 K)").arg(size).arg(sizeK)
    }

    function preset(sectors, densityIndex) {
        sectorsField.text = sectors
        densityCombo.currentIndex = densityIndex
        dlg.custom = false
    }

    function open2() { presetSingle.checked = true; open() }

    function create() {
        var sectors = parseInt(sectorsField.text) || 0
        if (sectors > 0) app.createDisk(sectors, sectorSize())
        dlg.close()
    }

    ButtonGroup { id: presets }

    background: Rectangle { color: Material.background }

    contentItem: ColumnLayout {
        spacing: 0

        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: dlg.SafeArea.margins.top; color: Theme.primary }

        // header
        ToolBar {
            Layout.fillWidth: true
            Material.primary: Theme.primary
            Material.foreground: "white"
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                Label {
                    text: qsTr("Create a disk image")
                    color: "white"
                    font.pixelSize: 20
                    font.bold: true
                    Layout.fillWidth: true
                }
            }
        }

        // body
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            ColumnLayout {
                x: 14
                width: parent.width - 28
                spacing: 6

                RadioButton { id: presetSingle;  text: qsTr("Standard single density"); ButtonGroup.group: presets
                    onCheckedChanged: if (checked) dlg.preset(720, 0) }
                RadioButton { text: qsTr("Standard enhanced (also called medium or dual) density"); ButtonGroup.group: presets
                    onCheckedChanged: if (checked) dlg.preset(1040, 0) }
                RadioButton { text: qsTr("Standard double density"); ButtonGroup.group: presets
                    onCheckedChanged: if (checked) dlg.preset(720, 1) }
                RadioButton { text: qsTr("Double sided double density"); ButtonGroup.group: presets
                    onCheckedChanged: if (checked) dlg.preset(1440, 1) }
                RadioButton { text: qsTr("Double density harddisk"); ButtonGroup.group: presets
                    onCheckedChanged: if (checked) dlg.preset(65535, 1) }
                RadioButton { text: qsTr("Custom"); ButtonGroup.group: presets
                    onCheckedChanged: if (checked) dlg.custom = true }

                MenuSeparator { Layout.fillWidth: true }

                RowLayout {
                    Layout.fillWidth: true
                    enabled: dlg.custom
                    Label { text: qsTr("Number of sectors:"); Layout.fillWidth: true }
                    TextField {
                        id: sectorsField
                        text: "720"
                        Layout.preferredWidth: 110
                        inputMethodHints: Qt.ImhDigitsOnly
                        validator: IntValidator { bottom: 1; top: 65535 }
                        onTextChanged: capLabel.text = dlg.capacity()
                    }
                }
                Label {
                    text: qsTr("Sector density:")
                    enabled: dlg.custom
                }
                ComboBox {
                    id: densityCombo
                    enabled: dlg.custom
                    Layout.fillWidth: true
                    model: [qsTr("Single (128 bytes per sector)"),
                            qsTr("Double (256 bytes per sector)"),
                            qsTr("512 bytes per sector"),
                            qsTr("8192 bytes per sector")]
                    onCurrentIndexChanged: capLabel.text = dlg.capacity()
                }

                Label {
                    id: capLabel
                    Layout.topMargin: 8
                    text: dlg.capacity()
                    color: Theme.typeGrey
                    font.pixelSize: 13
                }

                Item { Layout.preferredHeight: 8 }
            }
        }

        // footer
        Frame {
            Layout.fillWidth: true
            padding: 8
            background: Rectangle { color: "#ECECEC" }
            RowLayout {
                anchors.fill: parent
                Item { Layout.fillWidth: true }
                Button { text: qsTr("Cancel"); flat: true; onClicked: dlg.close() }
                Button { text: qsTr("Create"); highlighted: true; onClicked: dlg.create() }
            }
        }

        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: dlg.SafeArea.margins.bottom; color: "#ECECEC" }
    }
}
