import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import "."

// Printer output: the page the Epson emulation rendered, scrollable and
// zoomable, with clear and save-as-PNG. The image comes from the "paper" image
// provider; app.paperRevision is in the URL so a repaint actually reloads it.
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

    // Fit-to-width on open; the buttons and pinch then scale from there.
    property real zoom: 1.0
    function fitWidth() {
        if (paper.implicitWidth > 0)
            zoom = (flick.width - 12) / paper.implicitWidth
    }

    onOpened: {
        fitWidth()
        // Show the font the printer is actually using (stored between runs).
        var f = app.printerFontFamily()
        var i = f.length > 0 ? fontCombo.model.indexOf(f) : -1
        if (i >= 0) fontCombo.currentIndex = i
    }

    background: Rectangle { color: Material.background }

    FilePicker { id: pwPicker }
    ConfirmDialog { id: pwConfirm }

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
                    text: qsTr("Printer output")
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
                // Which font the print head "has fitted". Changing it re-runs the
                // parser over the stored job, so the page restyles in place.
                ComboBox {
                    id: fontCombo
                    Layout.fillWidth: true
                    Layout.preferredHeight: 36
                    font.pixelSize: 13
                    model: Qt.fontFamilies()
                    onActivated: app.printerSetFont(currentText)
                }
                SlotButton {
                    iconSize: 26
                    source: Theme.icon("actions/view-fullscreen.svg")
                    tip: qsTr("Fit width")
                    onClicked: pw.fitWidth()
                }
                SlotButton {
                    iconSize: 26
                    source: Theme.icon("actions/edit-clear.svg")
                    tip: qsTr("Clear the paper")
                    onClicked: pwConfirm.ask(qsTr("Printer"),
                        qsTr("Throw away the printed page?"),
                        function (yes) { if (yes) app.printerClear() })
                }
                SlotButton {
                    id: saveBtn
                    iconSize: 26
                    source: Theme.icon("actions/document-save.svg")
                    tip: qsTr("Save as…")
                    onClicked: saveMenu.open()

                    Menu {
                        id: saveMenu
                        y: saveBtn.height
                        MenuItem {
                            text: qsTr("Save as PNG image")
                            onTriggered: pwPicker.saveFile(
                                qsTr("Save printout"), [qsTr("PNG images (*.png)")], "", "printout.png",
                                function (url) { if (url.length > 0) app.printerSavePaper(url) })
                        }
                        MenuItem {
                            text: qsTr("Save as PDF document")
                            onTriggered: pwPicker.saveFile(
                                qsTr("Save printout"), [qsTr("PDF documents (*.pdf)")], "", "printout.pdf",
                                function (url) { if (url.length > 0) app.printerSavePdf(url) })
                        }
                    }
                }
            }
        }

        // the paper
        Flickable {
            id: flick
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 6
            clip: true
            contentWidth: Math.max(width, paper.width)
            contentHeight: Math.max(height, paper.height)
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {}
            ScrollBar.horizontal: ScrollBar {}

            // Two-finger zoom. target stays null so the handler only reports the
            // gesture: scaling the item itself would fight the Flickable, which
            // has to keep sizing its content from the zoom factor.
            PinchHandler {
                target: null
                property real zoomAtStart: 1.0
                onActiveChanged: if (active) zoomAtStart = pw.zoom
                onActiveScaleChanged: pw.zoom =
                    Math.max(0.1, Math.min(4.0, zoomAtStart * activeScale))
            }

            Image {
                id: paper
                // The revision is what busts the cache: the path never changes.
                source: "image://paper/page?rev=" + app.paperRevision
                cache: false
                asynchronous: true
                smooth: true
                fillMode: Image.PreserveAspectFit
                width: implicitWidth * pw.zoom
                height: implicitHeight * pw.zoom
                anchors.horizontalCenter: parent.horizontalCenter
                onStatusChanged: if (status === Image.Ready && pw.zoom === 1.0) pw.fitWidth()
            }

            Label {
                anchors.centerIn: parent
                visible: paper.implicitWidth <= 0
                text: qsTr("Nothing printed yet.")
                color: Theme.typeGrey
                font.pixelSize: 15
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
