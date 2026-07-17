import QtQuick
import QtQuick.Layouts
import "."

// The always-on-top loader slot (XEX/CAS). Reproduces
// MainWindow::androidBuildLoaderSlot: badge "cas/xex", three controls
// (load / play / retry) aligned with the first three disk-slot icons, eject at
// the right, and a left-to-right progress fill during loading.
Rectangle {
    id: card
    readonly property bool loaded: app.loaderKind !== 0

    radius: Theme.cardRadius
    color: loaded ? Theme.cardActiveBg : Theme.cardEmptyBg
    border.width: 1
    border.color: loaded ? Theme.cardActiveBorder : Theme.cardEmptyBorder
    implicitHeight: row.implicitHeight + 2 * 6
    clip: true

    // progress fill (0..1)
    Rectangle {
        visible: app.loaderFill > 0.0 && app.loaderFill < 1.0
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: 1
        width: (parent.width - 2) * app.loaderFill
        radius: Theme.cardRadius
        color: Theme.loaderFill
    }

    RowLayout {
        id: row
        anchors.fill: parent
        anchors.leftMargin: Theme.cardMargin
        anchors.rightMargin: Theme.cardMargin
        anchors.topMargin: 6
        anchors.bottomMargin: 6
        spacing: Theme.gap

        Badge {
            text: "cas\nxex"
            fontPx: 10
            Layout.alignment: Qt.AlignVCenter
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            RowLayout {
                Layout.fillWidth: true
                spacing: 4
                Item { Layout.fillWidth: true }

                SlotButton {
                    source: Theme.icon("categories/applications-system.svg")
                    spinning: app.loaderLoading
                    tip: qsTr("Load executable or cassette")
                    onClicked: app.loaderLoad()
                }
                SlotButton {
                    source: Theme.icon("actions/media-playback-start.svg")
                    animatedSource: "qrc:/images/tape.gif"
                    animated: app.loaderCasPlaying
                    enabledState: app.loaderPlayEnabled || app.loaderCasPlaying
                    tip: qsTr("Start cassette playback")
                    onClicked: app.loaderPlay()
                }
                SlotButton {
                    source: Theme.icon("actions/view-refresh.svg")
                    enabledState: app.loaderRetryEnabled
                    tip: qsTr("Retry")
                    onClicked: app.loaderRetry()
                }
                // invisible spacers so the three controls line up with the
                // first three of the five disk-slot icons
                Item { implicitWidth: Theme.iconSize + 10; implicitHeight: 1 }
                Item { implicitWidth: Theme.iconSize + 10; implicitHeight: 1 }

                Item { Layout.fillWidth: true }
                Item { width: 6 }

                SlotButton {
                    source: Theme.icon("actions/media-eject.svg")
                    enabledState: app.loaderEjectEnabled
                    tip: qsTr("Eject")
                    onClicked: app.loaderEject()
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.gap
                Text {
                    text: card.loaded ? app.loaderFileName
                                      : qsTr("Load an executable or cassette.")
                    font.bold: card.loaded
                    font.italic: !card.loaded
                    font.pixelSize: card.loaded ? 15 : 12
                    color: card.loaded ? Theme.nameDark : Theme.placeholder
                    elide: Text.ElideRight
                    Layout.maximumWidth: parent.width * 0.6
                }
                Item { Layout.fillWidth: true }
                Text {
                    visible: card.loaded && app.loaderTypeText.length > 0
                    text: app.loaderTypeText
                    color: Theme.typeGrey
                    font.pixelSize: 13
                    elide: Text.ElideRight
                }
                Item { width: 6 }
            }
        }
    }
}
