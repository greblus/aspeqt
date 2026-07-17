import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import "."

// Printer text-output window (ASCII view), in QML. Shows the accumulated printer
// output with a selectable font/size, word-wrap, clear and save. (The widget's
// second ATASCII view + physical Print are omitted on Android.)
Popup {
    id: pw
    parent: Overlay.overlay
    x: 0
    y: 0
    width: parent ? parent.width : 420
    height: parent ? parent.height : 900
    modal: true
    padding: 0
    closePolicy: Popup.NoAutoClose

    readonly property var sizes: [9, 12, 15, 18]

    FontLoader { id: atariFont; source: "qrc:/images/AtariClassicChunky.ttf" }

    background: Rectangle { color: Material.background }

    contentItem: ColumnLayout {
        spacing: 0

        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: pw.SafeArea.margins.top; color: Theme.primary }

        // header
        ToolBar {
            Layout.fillWidth: true
            Material.primary: Theme.primary
            Material.foreground: "white"
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                Label {
                    text: qsTr("Printer text output")
                    color: "white"
                    font.pixelSize: 20
                    font.bold: true
                    Layout.fillWidth: true
                }
            }
        }

        // controls
        ToolBar {
            Layout.fillWidth: true
            Material.background: "#ECECEC"
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 6
                ComboBox {
                    id: fontCombo
                    Layout.fillWidth: true
                    Layout.preferredHeight: 36
                    font.pixelSize: 13
                    model: Qt.fontFamilies()
                }
                ComboBox {
                    id: sizeCombo
                    Layout.preferredWidth: 74
                    Layout.preferredHeight: 36
                    font.pixelSize: 13
                    model: pw.sizes
                    currentIndex: 1
                }
                SlotButton {
                    source: Theme.icon("actions/format-justify-fill.svg")
                    checked: wrapOn.on
                    tip: qsTr("Word wrap")
                    onClicked: wrapOn.on = !wrapOn.on
                }
                QtObject { id: wrapOn; property bool on: true }
                SlotButton {
                    source: Theme.icon("mimetypes/font-x-generic.svg")
                    checked: atasciiOn.on
                    tip: qsTr("Show ATASCII")
                    onClicked: atasciiOn.on = !atasciiOn.on
                }
                QtObject { id: atasciiOn; property bool on: false }
                SlotButton {
                    source: Theme.icon("actions/edit-clear.svg")
                    tip: qsTr("Clear contents")
                    onClicked: app.printerClear()
                }
                SlotButton {
                    source: Theme.icon("actions/document-save.svg")
                    tip: qsTr("Save to a file")
                    onClicked: app.printerSave()
                }
            }
        }

        // text views: ASCII (top) and optional ATASCII (bottom, Atari font)
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 6
            spacing: 6

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                TextArea {
                    readOnly: true
                    text: app.printerText
                    font.family: fontCombo.currentText
                    font.pixelSize: pw.sizes[sizeCombo.currentIndex]
                    wrapMode: wrapOn.on ? TextArea.Wrap : TextArea.NoWrap
                }
            }
            ScrollView {
                visible: atasciiOn.on
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                background: Rectangle { color: "#F4F4F4"; border.color: "#D0D0D0" }
                TextArea {
                    readOnly: true
                    text: app.printerTextAtascii
                    font.family: atariFont.name
                    font.pixelSize: pw.sizes[sizeCombo.currentIndex]
                    wrapMode: wrapOn.on ? TextArea.Wrap : TextArea.NoWrap
                }
            }
        }

        // footer
        ToolBar {
            Layout.fillWidth: true
            Material.background: "#ECECEC"
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 8
                Item { Layout.fillWidth: true }
                Button { text: qsTr("Close"); highlighted: true; onClicked: pw.close() }
            }
        }

        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: pw.SafeArea.margins.bottom; color: "#ECECEC" }
    }
}
