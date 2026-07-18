pragma Singleton
import QtQuick

// Central palette + metrics for the QML UI. Colours are taken 1:1 from the
// widget UI (MainWindow::deviceStatusChanged / androidBuild*), so the QML port
// stays visually faithful; the chrome (top bar) is free to be Material.
QtObject {
    // Material chrome
    readonly property color primary: "#1976D2"   // top bar
    readonly property color accent:  "#E0A030"    // badges / FAB

    // Cards
    readonly property color cardEmptyBg:     "#F7F7F7"
    readonly property color cardEmptyBorder: "#B0B0B0"
    readonly property color cardActiveBg:     "#EAF3FB"
    readonly property color cardActiveBorder: "#6FA8DC"
    readonly property color loaderFill:       "#CFE6FF"

    // Text
    readonly property color nameDark:    "#202020"
    readonly property color nameFolder:  "#36A8A4"
    readonly property color typeGrey:    "#8A8A8A"
    readonly property color placeholder: "#B0B0B0"

    // Raised tool-button box (matches the QToolButton bevel in the widget UI)
    readonly property color btnBgTop:    "#FDFDFD"
    readonly property color btnBgBottom: "#E8E8E8"
    readonly property color btnBorder:   "#B8B8B8"
    readonly property color btnPressed:  "#D6D6D6"
    readonly property color btnCheckedBg: "#D8E6F5"
    readonly property color btnCheckedBorder: "#6FA8DC"

    // Metrics
    readonly property int cardRadius: 6
    readonly property int cardMargin: 6
    readonly property int iconSize:   36
    readonly property int btnPad:     4
    readonly property int badgeSize:  32
    readonly property int gap:        6

    function icon(name) { return "qrc:/icons/tango-icons/" + name }
    function image(name) { return "qrc:/images/" + name }
}
