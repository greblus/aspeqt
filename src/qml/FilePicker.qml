import QtQuick
import QtQuick.Dialogs

// Asks the user for a file or folder and hands the result to a callback.
//
// Two backends. On Android the request goes to our own SAF intents
// (ACTION_OPEN_DOCUMENT / CREATE_DOCUMENT / OPEN_DOCUMENT_TREE): Qt's file
// dialog validates the document it returns by stat()ing a partly-decoded URI,
// so any name holding a space or a bracket is discarded as "not existing", and
// talking to the intent directly also preserves Android's exact URI encoding,
// which the content resolver needs. Everywhere else the Qt Quick dialogs are
// already the platform's native ones.
//
// Both are asynchronous, so callers pass a callback instead of getting a
// return value; it always fires exactly once, with "" when cancelled.
Item {
    id: root

    property var _cb: null
    property int _reqId: 0

    function openFile(caption, filters, startDir, cb) {
        if (app.isAndroid) {
            _arm(cb)
            app.pickDocument(_reqId, "*/*")
            return
        }
        _cb = cb
        fileDlg.title = caption
        fileDlg.nameFilters = filters
        fileDlg.fileMode = FileDialog.OpenFile
        if (startDir.length > 0)
            fileDlg.currentFolder = startDir
        fileDlg.open()
    }

    function saveFile(caption, filters, startDir, suggestedName, cb) {
        if (app.isAndroid) {
            _arm(cb)
            app.createDocument(_reqId, "*/*", suggestedName)
            return
        }
        _cb = cb
        fileDlg.title = caption
        fileDlg.nameFilters = filters
        fileDlg.fileMode = FileDialog.SaveFile
        if (startDir.length > 0)
            fileDlg.currentFolder = startDir
        fileDlg.open()
    }

    function chooseFolder(caption, startDir, cb) {
        if (app.isAndroid) {
            _arm(cb)
            app.pickFolder(_reqId)
            return
        }
        _cb = cb
        folderDlg.title = caption
        if (startDir.length > 0)
            folderDlg.currentFolder = startDir
        folderDlg.open()
    }

    function _arm(cb) {
        _cb = cb
        _reqId += 1
    }

    function _deliver(url) {
        var cb = _cb
        _cb = null
        if (cb)
            cb(String(url))
    }

    // Android: the SAF result comes back as Android's own URI string.
    Connections {
        target: app
        function onDocumentPicked(reqId, uri) {
            if (reqId === root._reqId)
                root._deliver(uri)
        }
    }

    FileDialog {
        id: fileDlg
        onAccepted: root._deliver(selectedFile)
        onRejected: root._deliver("")
    }

    FolderDialog {
        id: folderDlg
        onAccepted: root._deliver(selectedFolder)
        onRejected: root._deliver("")
    }
}
