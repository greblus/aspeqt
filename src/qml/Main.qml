import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import "."

ApplicationWindow {
    id: win
    visible: true
    // Desktop default only; Android forces fullscreen. Content is capped +
    // centred below so it stays a comfortable column on wide screens/tablets.
    width: Math.min(Screen.width, 480)
    height: Math.min(Screen.height, 900)
    title: "AspeQt"

    // Max width of the content column (phones use full width; tablets centre it).
    readonly property int contentMaxWidth: 720
    property int pendingFlashHw: -1   // slot to scroll to + flash after an add
    // The slot column may grow until the log is down to two slots' worth of
    // height. Adding slots shrinks the log, never hides it.
    readonly property int logMinHeight: 2 * loaderCard.height

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
                    // The preselected first entry comes from keyboard-focus
                    // handling; clearing it in onOpened only makes it blink, so
                    // drop the focus and clear before the menu is shown.
                    focus: false
                    onAboutToShow: currentIndex = -1
                    onClosed: currentIndex = -1

                    MenuItem {
                        text: app.sioRunning ? qsTr("Stop emulation")
                                             : qsTr("Start emulation")
                        onTriggered: app.toggleSio()
                    }
                    MenuItem {
                        text: qsTr("Printer emulation")
                        checkable: true
                        checked: app.printerOn
                        onTriggered: app.togglePrinter()
                    }
                    MenuItem { text: qsTr("Show printer output"); onTriggered: printWindow.open() }
                    MenuItem {
                        text: qsTr("Phonebook…")
                        enabled: app.modemEnabled
                        onTriggered: phoneBookDialog.openBook()
                    }
                    MenuItem { text: qsTr("Network browser…"); onTriggered: networkBrowserDialog.openBrowser() }
                    MenuSeparator {}

                    Menu {
                        title: qsTr("File")
                        focus: false
                        onAboutToShow: currentIndex = -1
                        onClosed: currentIndex = -1
                        MenuItem {
                            text: qsTr("Open session…")
                            onTriggered: filePicker.openFile(
                                qsTr("Open session"),
                                [qsTr("AspeQt sessions (*.aspeqt)"), qsTr("All files (*)")],
                                app.startDir("session"),
                                function (url) { if (url.length > 0) app.openSessionPath(url) })
                        }
                        MenuItem {
                            text: qsTr("Save session…")
                            onTriggered: filePicker.saveFile(
                                qsTr("Save session as"),
                                [qsTr("AspeQt sessions (*.aspeqt)"), qsTr("All files (*)")],
                                app.startDir("session"), "aspeqt.aspeqt",
                                function (url) { if (url.length > 0) app.saveSessionPath(url) })
                        }
                    }

                    Menu {
                        title: qsTr("Disk")
                        focus: false
                        onAboutToShow: currentIndex = -1
                        onClosed: currentIndex = -1
                        MenuItem { text: qsTr("New disk image…"); onTriggered: createDiskDialog.open2() }
                        MenuItem { text: qsTr("Eject all")
                                   onTriggered: win.withUnsaved(function () { app.ejectAll() }) }
                    }

                    Menu {
                        id: recentMenu
                        title: qsTr("Recent")
                        focus: false
                        onClosed: currentIndex = -1
                        onAboutToShow: {
                            currentIndex = -1
                            var arr = []
                            var files = app.recentFiles()
                            for (var i = 0; i < files.length; ++i)
                                if (files[i].length > 0)
                                    arr.push({ name: files[i], idx: i })
                            recentInst.model = arr
                        }
                        Instantiator {
                            id: recentInst
                            model: []
                            delegate: MenuItem {
                                required property var modelData
                                text: modelData.name
                                onTriggered: app.mountRecent(modelData.idx)
                            }
                            onObjectAdded: (index, object) => recentMenu.insertItem(index, object)
                            onObjectRemoved: (index, object) => recentMenu.removeItem(object)
                        }
                    }

                    MenuSeparator {}
                    MenuItem {
                        text: qsTr("Options")
                        onTriggered: { optionsDialog.load(); optionsDialog.open() }
                    }
                    MenuItem { text: qsTr("Quit")
                        onTriggered: win.withUnsaved(function () { app.quit() }) }
                }
            }
        }
    }

    // ---- body --------------------------------------------------------------
    Item {
        id: body
        anchors.fill: parent

        ColumnLayout {
            anchors.horizontalCenter: parent.horizontalCenter
            width: Math.min(parent.width, win.contentMaxWidth)
            height: parent.height
            spacing: 0

        // scrollable slot column: loader (pinned) + disk slots + add row
        ScrollView {
            id: slotScroll
            Layout.fillWidth: true
            Layout.leftMargin: 6
            Layout.rightMargin: 6
            Layout.fillHeight: false
            // Take what the slots need, but never squeeze the log below its
            // minimum. The status bar shares this column, so its height is not
            // ours to spend either -- forgetting that left the log a sliver.
            Layout.preferredHeight: Math.min(slotCol.implicitHeight + 12,
                                             Math.max(0, body.height - statusBar.height
                                                          - win.logMinHeight))
            clip: true
            contentWidth: availableWidth      // clamp: never scroll horizontally
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            ScrollBar.vertical.policy: ScrollBar.AlwaysOff   // finger-scroll only

            ColumnLayout {
                id: slotCol
                width: slotScroll.availableWidth
                spacing: 6
                y: 6

                LoaderCard {
                    id: loaderCard
                    Layout.fillWidth: true
                    onRequestLoad: filePicker.openFile(
                        qsTr("Load executable or cassette"),
                        [qsTr("Atari programs (*.xex *.com *.exe *.cas)"), qsTr("All files (*)")],
                        app.startDir("exe"),
                        function (url) { if (url.length > 0) app.loaderLoadPath(url) })
                }

                Repeater {
                    id: slotRepeater
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
                        isBootSlot: model.isBootSlot
                        onRequestSwap: (fromHw, toHw) => app.swapSlots(fromHw, toHw)
                        onRequestEditor: (hw) => diskViewer.openFor(hw)
                        onRequestMount: (hw) => win.withUnsavedSlot(hw, modified && !autoCommit,
                            function () {
                                filePicker.openFile(
                                    qsTr("Open a disk image"),
                                    [qsTr("All Atari disk images (*.atr *.xfd *.pro)"), qsTr("All files (*)")],
                                    app.startDir("disk"),
                                    function (url) { if (url.length > 0) app.mountDiskPath(hw, url) })
                            })
                        onRequestSave: (hw, isDos) => win.saveSlot(hw, isDos)
                        onRequestEject: (hw, dirty) => win.withUnsavedSlot(hw, dirty,
                            function () { app.eject(hw) })
                        onRequestSaveName: (hw) => win.askSaveName(hw)
                        onRequestMountFolder: (hw) => win.withUnsavedSlot(hw, modified && !autoCommit,
                            function () {
                                filePicker.chooseFolder(
                                    qsTr("Open a folder image"),
                                    app.startDir("folder"),
                                    function (url) { if (url.length > 0) app.mountFolderPath(hw, url) })
                            })
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
                    MouseArea {
                        anchors.fill: parent
                        // Only scroll when a slot is appended at the end (not when
                        // "+" fills a gap left by a removed slot).
                        onClicked: {
                            var hw = app.addSlot()
                            if (hw >= 0) { win.pendingFlashHw = hw; scrollEndTimer.restart() }
                        }
                    }
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

            // Non-interactive Text in a vertical-only Flickable: lets the
            // TapHandler get the double-tap, and never scrolls horizontally.
            Flickable {
                id: logFlick
                anchors.fill: parent
                anchors.margins: 6
                clip: true
                contentWidth: width
                contentHeight: logText.implicitHeight
                flickableDirection: Flickable.VerticalFlick
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                TapHandler { onDoubleTapped: logWindow.open() }

                Text {
                    id: logText
                    width: logFlick.width
                    wrapMode: Text.Wrap
                    // StyledText, not RichText: the rich-text parser builds a
                    // whole QTextDocument per update, which cannot keep up with
                    // fast SIO. StyledText handles <font color> and <br> cheaply.
                    textFormat: Text.StyledText
                    text: app.logTailHtml
                    font.pixelSize: 15
                }
                onContentHeightChanged:
                    contentY = Math.max(0, contentHeight - height)
            }
        }

        // bottom status bar: right cluster [speed][connect][printer][clear],
        // flat pixmap icons like the widget statusbar's permanent widgets.
        ToolBar {
            id: statusBar
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
                    source: app.printerOn ? Theme.icon("devices/printer.svg")
                                          : Theme.icon("status/printer-error.svg")
                    on: true
                    tip: app.printerOn ? qsTr("Stop printer emulation")
                                       : qsTr("Start printer emulation")
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

    // Smoothly scroll the slot column to the bottom after a slot is appended
    // (deferred so the new row's height is included).
    NumberAnimation {
        id: scrollAnim
        target: slotScroll.contentItem
        property: "contentY"
        duration: 320
        easing.type: Easing.OutCubic
    }
    Timer {
        id: scrollEndTimer
        interval: 80
        onTriggered: {
            var f = slotScroll.contentItem
            for (var i = 0; i < slotRepeater.count; ++i) {
                var d = slotRepeater.itemAt(i)
                if (d && d.hwIndex === win.pendingFlashHw) {
                    // delegate position in flickable content coordinates
                    var contentPos = d.mapToItem(f, 0, 0).y + f.contentY
                    var target = Math.max(0, Math.min(contentPos - (f.height - d.height) / 2,
                                                       f.contentHeight - f.height))
                    scrollAnim.from = f.contentY
                    scrollAnim.to = target
                    scrollAnim.restart()
                    d.flash()
                    break
                }
            }
        }
    }

    OptionsDialog { id: optionsDialog }
    LogWindow { id: logWindow }
    FilePicker { id: filePicker }
    ConfirmDialog { id: confirmDialog }
    DiskViewer { id: diskViewer }


    // --- unsaved changes -----------------------------------------------------
    // The engine no longer asks anything: it is told to save, eject or quit.
    // The decision is made here, where the model already carries `modified`.

    // Saves the listed drives one at a time; a drive with no file name yet
    // needs the picker, which is asynchronous, hence the chain.
    function saveEach(list, i, done) {
        if (i >= list.length) { done(); return }
        var hw = list[i].hwIndex
        if (app.save(hw) === 1) {
            filePicker.saveFile(
                qsTr("Save image as"),
                [qsTr("ATR image (*.atr)"), qsTr("All files (*)")],
                app.startDir("disk"), "disk.atr",
                function (url) {
                    if (url.length > 0) app.saveAsPath(hw, url)
                    win.saveEach(list, i + 1, done)
                })
        } else {
            saveEach(list, i + 1, done)
        }
    }

    // Runs `proceed` once the user has settled what to do with every modified
    // image (one prompt listing them all).
    function withUnsaved(proceed) {
        var mods = app.modifiedDisks()
        if (mods.length === 0) { proceed(); return }
        var names = []
        for (var i = 0; i < mods.length; ++i)
            names.push("D" + mods[i].slot + ": " + mods[i].name)
        confirmDialog.askSave(
            qsTr("Unsaved changes"),
            qsTr("These images have unsaved changes:\n\n%1").arg(names.join("\n")),
            function (answer) {
                if (answer === "cancel") return
                if (answer === "save") win.saveEach(mods, 0, proceed)
                else proceed()
            })
    }

    // A single drive, named so the prompt says which one.
    function withUnsavedSlot(hw, dirty, proceed) {
        if (!dirty) { proceed(); return }
        var mods = app.modifiedDisks()
        var one = []
        for (var i = 0; i < mods.length; ++i)
            if (mods[i].hwIndex === hw) one.push(mods[i])
        if (one.length === 0) { proceed(); return }
        confirmDialog.askSave(
            qsTr("Unsaved changes"),
            qsTr("'%1' has unsaved changes.").arg(one[0].name),
            function (answer) {
                if (answer === "cancel") return
                if (answer === "save") win.saveEach(one, 0, proceed)
                else proceed()
            })
    }

    // Saving asks here rather than in the engine: a modal dialog down there
    // would block the SIO path. app.save() reports 1 when it needs a name.
    function saveSlot(hw, isDos) {
        if (isDos) {
            confirmDialog.ask(qsTr("Install DOS"),
                qsTr("Copy high-speed MyPicoDOS ($boot.bin + picodos.sys) into this folder? "
                     + "The Atari will then be able to boot DOS from it."),
                function (yes) { if (yes) app.installDos(hw) })
            return
        }
        if (app.save(hw) === 1)
            askSaveName(hw)
    }

    function askSaveName(hw) {
        filePicker.saveFile(
            qsTr("Save image as"),
            [qsTr("ATR image (*.atr)"), qsTr("XFD image (*.xfd)"), qsTr("All files (*)")],
            app.startDir("disk"), "disk.atr",
            function (url) { if (url.length > 0) app.saveAsPath(hw, url) })
    }
    CreateDiskDialog { id: createDiskDialog }
    PhoneBookDialog { id: phoneBookDialog }
    NetworkBrowserDialog { id: networkBrowserDialog }
    PrintWindow { id: printWindow }
}
