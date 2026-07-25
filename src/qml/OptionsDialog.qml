import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import "."

// Options screen — faithful to the Android QtWidgets OptionsDialog layout, minus
// the obsolete "custom drive numbers" (slot order), "larger font" and "save
// window position" fields (the latter two are now always on). Values load from
// and save to the engine via app.loadOptions()/applyOptions().
Popup {
    id: dlg
    parent: Overlay.overlay
    // Full-screen; the breathing room is internal padding on the body content.
    x: 0
    y: 0
    width: parent ? parent.width : 420
    height: parent ? parent.height : 900
    modal: true
    padding: 0
    closePolicy: Popup.NoAutoClose

    function load() {
        var o = app.loadOptions()
        ifaceGroup.select(o.iface)
        hsGroup.select(o.handshake)
        baudGroup.select(o.baud)
        btName.text = o.btName
        ackDelay.value = o.ackDelay
        useDivisors.checked = o.useDivisors
        pokeyDivisor.value = o.pokeyDivisor
        hsExeLoader.checked = o.hsExeLoader
        useCustomCas.checked = o.useCustomCasBaud
        customCasBaud.value = o.customCasBaud
        filterUscore.checked = o.filterUscore
        rEnabled.checked = o.rEnabled
        dlg.phonebook = o.rPhonebook
        rBundled.checked = o.rPhonebook.length > 0
                           && o.rPhonebook === app.phonebookBundledPath()
        rListen.checked = o.rListen
        rListenPort.text = o.rListenPort
        langBox.model = app.languages()
        langBox.currentIndex = Math.max(0, langBox.indexOfValue(o.language))
    }

    function save() {
        app.applyOptions({
            "iface": ifaceGroup.value,
            "handshake": hsGroup.value,
            "baud": baudGroup.value,
            "btName": btName.text,
            "ackDelay": ackDelay.value,
            "useDivisors": useDivisors.checked,
            "pokeyDivisor": pokeyDivisor.value,
            "hsExeLoader": hsExeLoader.checked,
            "useCustomCasBaud": useCustomCas.checked,
            "customCasBaud": customCasBaud.value,
            "filterUscore": filterUscore.checked,
            "rEnabled": rEnabled.checked,
            "rPhonebook": dlg.phonebook,
            "rListen": rListen.checked,
            "rListenPort": rListenPort.value,
            "language": langBox.currentValue
        })
        dlg.close()
    }

    readonly property bool isBT: ifaceGroup.value === 1

    // Phonebook file path; picked with the file dialog, shown by basename.
    property string phonebook: ""
    FilePicker { id: optPicker }

    // Raised after printing a test page / replaying a capture, so the main
    // window can bring the printer output up (this dialog does not own it).
    signal showPrinter()

    // A radio group that remembers an integer value per button.
    component IntGroup: QtObject {
        property ButtonGroup group: ButtonGroup {}
        property int value: -1
        function select(v) {
            var b = group.buttons
            for (var i = 0; i < b.length; ++i)
                if (b[i].val === v) { b[i].checked = true; value = v; return }
        }
    }
    // main group title (blue, large)
    component SectionTitle: Label {
        Layout.topMargin: 10
        font.pixelSize: 21
        font.bold: true
        color: Theme.primary
    }
    // sub-category label (bold)
    component FieldLabel: Label {
        Layout.topMargin: 4
        font.bold: true
    }
    // Compact -/value/+ stepper (the Material SpinBox is far too tall/wide here).
    component Spin: RowLayout {
        id: spin
        property int from: 0
        property int to: 100
        property int step: 1
        property int value: 0
        spacing: 2
        Button {
            text: "−"
            flat: true
            implicitWidth: 38; implicitHeight: 38
            padding: 0
            enabled: spin.value > spin.from
            onClicked: spin.value = Math.max(spin.from, spin.value - spin.step)
        }
        Label {
            text: spin.value
            horizontalAlignment: Text.AlignHCenter
            Layout.preferredWidth: 52
            font.pixelSize: 16
        }
        Button {
            text: "+"
            flat: true
            implicitWidth: 38; implicitHeight: 38
            padding: 0
            enabled: spin.value < spin.to
            onClicked: spin.value = Math.min(spin.to, spin.value + spin.step)
        }
    }

    IntGroup { id: ifaceGroup }
    IntGroup { id: hsGroup }
    IntGroup { id: baudGroup }

    Connections { target: ifaceGroup.group; function onClicked(b) { ifaceGroup.value = b.val } }
    Connections { target: hsGroup.group;    function onClicked(b) { hsGroup.value = b.val } }
    Connections { target: baudGroup.group;  function onClicked(b) { baudGroup.value = b.val } }

    background: Rectangle { color: Material.background }

    contentItem: ColumnLayout {
        spacing: 0

        // header bar
        ToolBar {
            Layout.fillWidth: true
            Material.primary: Theme.primary
            Material.foreground: "white"
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 4
                Label {
                    text: qsTr("Options")
                    color: "white"
                    font.pixelSize: 20
                    font.bold: true
                    Layout.fillWidth: true
                }
            }
        }

        // scrollable body
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            ColumnLayout {
                x: 14
                width: parent.width - 28      // internal horizontal padding
                spacing: 6

                // ---- SIO port emulation -----------------------------------
                SectionTitle { text: qsTr("SIO port emulation") }

                FieldLabel { text: qsTr("Serial interface:") }
                RowLayout {
                    RadioButton { property int val: 0; text: "SIO2PC"; ButtonGroup.group: ifaceGroup.group }
                    RadioButton { property int val: 1; text: "SIO2BT"; ButtonGroup.group: ifaceGroup.group }
                }

                FieldLabel { text: qsTr("Handshake method:") }
                Flow {
                    Layout.fillWidth: true
                    spacing: 4
                    RadioButton { property int val: 0; text: "RI";   ButtonGroup.group: hsGroup.group }
                    RadioButton { property int val: 1; text: "DSR";  ButtonGroup.group: hsGroup.group }
                    RadioButton { property int val: 2; text: "CTS";  ButtonGroup.group: hsGroup.group }
                    RadioButton { property int val: 3; text: "SOFT"; ButtonGroup.group: hsGroup.group }
                }

                FieldLabel {
                    text: qsTr("Transmission speed [bps]:")
                    enabled: !useDivisors.checked
                }
                ColumnLayout {
                    spacing: 0
                    enabled: !useDivisors.checked
                    RadioButton { property int val: 0; text: "19200 (1x)"; ButtonGroup.group: baudGroup.group }
                    RadioButton { property int val: 1; text: "38400 (2x)"; ButtonGroup.group: baudGroup.group }
                    RadioButton { property int val: 2; text: "57600 (3x)"; ButtonGroup.group: baudGroup.group }
                }

                FieldLabel { text: qsTr("Bluetooth name:"); enabled: dlg.isBT }
                TextField {
                    id: btName
                    Layout.fillWidth: true
                    placeholderText: "SIO2BT"
                    enabled: dlg.isBT
                }

                RowLayout {
                    Layout.fillWidth: true
                    enabled: dlg.isBT
                    FieldLabel { text: qsTr("Write ACK delay [ms]"); Layout.fillWidth: true }
                    Spin { id: ackDelay; from: 0; to: 40 }
                }

                CheckBox { id: useDivisors; text: qsTr("Use non-standard speeds") }
                RowLayout {
                    Layout.fillWidth: true
                    enabled: useDivisors.checked
                    Label { text: qsTr("POKEY divisor:"); Layout.fillWidth: true }
                    Spin { id: pokeyDivisor; from: 0; to: 40 }
                }

                MenuSeparator { Layout.fillWidth: true }

                // ---- Emulation settings -----------------------------------
                SectionTitle { text: qsTr("Emulation settings") }
                CheckBox { id: hsExeLoader; text: qsTr("Use high speed executable loader") }
                CheckBox { id: useCustomCas; text: qsTr("Use custom baud rate for cassette emulation") }
                RowLayout {
                    Layout.fillWidth: true
                    enabled: useCustomCas.checked
                    FieldLabel { text: qsTr("Cassette baud rate:"); Layout.fillWidth: true }
                    Spin { id: customCasBaud; from: 425; to: 875 }
                }

                MenuSeparator { Layout.fillWidth: true }

                // ---- Folder images ----------------------------------------
                SectionTitle { text: qsTr("Folder images") }
                CheckBox { id: filterUscore; text: qsTr("Filter out underscore character from file names") }
                Label {
                    text: qsTr("(Required for AtariDOS compatibility)")
                    color: Theme.typeGrey
                    font.pixelSize: 12
                    Layout.leftMargin: 8
                }

                MenuSeparator { Layout.fillWidth: true }

                // ---- printer ----------------------------------------------
                SectionTitle { text: qsTr("Printer emulation") }
                CheckBox {
                    text: qsTr("Emulate an Epson ESC/P printer")
                    checked: app.printerOn
                    onToggled: if (checked !== app.printerOn) app.togglePrinter()
                }
                Label {
                    text: qsTr("What the Atari prints is rendered onto a page you can view "
                             + "and save. The font is chosen in the printer output window.")
                    color: Theme.typeGrey
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                    Layout.leftMargin: 8
                }
                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 8
                    spacing: 8
                    Button {
                        text: qsTr("Print a test page")
                        onClicked: { app.printerTestPage(); dlg.showPrinter() }
                    }
                    Button {
                        flat: true
                        text: qsTr("Replay a capture…")
                        onClicked: optPicker.openFile(
                            qsTr("Open a print capture"),
                            [qsTr("Print captures (*.prn)"), qsTr("All files (*)")], "",
                            function (url) { if (url.length > 0) { app.printerReplay(url); dlg.showPrinter() } })
                    }
                }

                MenuSeparator { Layout.fillWidth: true }

                // ---- R: device (850 modem emulation) ----------------------
                SectionTitle { text: qsTr("R: device (modem)") }
                CheckBox { id: rEnabled; text: qsTr("Emulate an Atari 850 interface") }
                Label {
                    text: qsTr("Dial BBSes over TCP with a terminal program. Needs a hardware "
                             + "handshake method (RI/DSR/CTS), not SOFT.")
                    color: Theme.typeGrey
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                    Layout.leftMargin: 8
                }

                CheckBox {
                    id: rBundled
                    text: qsTr("Use the bundled BBS list")
                    enabled: rEnabled.checked
                    // State is set in load(); toggling installs/clears the path,
                    // so no binding here (it would fight the click).
                    onToggled: {
                        if (checked) {
                            var p = app.phonebookUseBundled()
                            if (p.length > 0) {
                                dlg.phonebook = p
                            } else {
                                checked = false
                                app.toast(qsTr("Could not install the bundled list, see the log."))
                            }
                        } else {
                            dlg.phonebook = ""
                        }
                    }
                }
                Label {
                    text: qsTr("A copy of the telnet BBS list that ships with AspeQt-2k26. "
                             + "An existing copy is kept, not overwritten.")
                    color: Theme.typeGrey
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                    Layout.leftMargin: 8
                }

                FieldLabel {
                    text: qsTr("Phonebook file:")
                    enabled: rEnabled.checked && !rBundled.checked
                }
                RowLayout {
                    Layout.fillWidth: true
                    enabled: rEnabled.checked && !rBundled.checked
                    Label {
                        text: dlg.phonebook.length > 0
                              ? dlg.phonebook.split("/").pop().split("%2F").pop()
                              : qsTr("(none)")
                        color: dlg.phonebook.length > 0 ? Theme.nameDark : Theme.placeholder
                        font.italic: dlg.phonebook.length === 0
                        elide: Text.ElideMiddle
                        Layout.fillWidth: true
                    }
                    Button {
                        text: qsTr("Choose")
                        flat: true
                        onClicked: optPicker.openFile(
                            qsTr("Phonebook file"), [qsTr("XML files (*.xml)"), qsTr("All files (*)")], "",
                            function (url) { if (url.length > 0) dlg.phonebook = url })
                    }
                }

                CheckBox {
                    id: rListen
                    text: qsTr("Answer incoming calls")
                    enabled: rEnabled.checked
                }
                RowLayout {
                    Layout.fillWidth: true
                    enabled: rEnabled.checked && rListen.checked
                    FieldLabel { text: qsTr("Listen on port:"); Layout.fillWidth: true }
                    TextField {
                        id: rListenPort
                        // A stepper would be unusable over a 1..65535 range.
                        property int value: parseInt(text) || 2323
                        text: "2323"
                        inputMethodHints: Qt.ImhDigitsOnly
                        validator: IntValidator { bottom: 1; top: 65535 }
                        Layout.preferredWidth: 96
                        horizontalAlignment: Text.AlignHCenter
                    }
                }

                MenuSeparator { Layout.fillWidth: true }

                // ---- User interface ---------------------------------------
                SectionTitle { text: qsTr("User interface") }
                FieldLabel { text: qsTr("Language:") }
                ComboBox {
                    id: langBox
                    Layout.fillWidth: true
                    textRole: "name"
                    valueRole: "code"
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
                Button { text: qsTr("Save"); highlighted: true; onClicked: dlg.save() }
            }
        }
    }
}
