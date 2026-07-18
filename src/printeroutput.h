#ifndef PRINTEROUTPUT_H
#define PRINTEROUTPUT_H

#include <QObject>
#include <QString>

class QIODevice;

// Collects what the emulated printer receives. Replaces TextPrinterWindow: the
// text used to live in two hidden QTextEdits of a QMainWindow that was never
// shown, with the QML print window reading it back out of them.
//
// Saving goes through QPdfWriter/QTextDocument, both of which are QtGui -- the
// QtPrintSupport module (QPrinter, QPrintDialog) is not needed for producing a
// PDF, only for talking to a physical printer.
class PrinterOutput : public QObject
{
    Q_OBJECT

public:
    explicit PrinterOutput(QObject *parent = nullptr);

    QString plainText() const   { return m_ascii; }    // inverse video stripped
    QString atasciiText() const { return m_atascii; }  // raw, for the Atari font
    bool    isEmpty() const     { return m_atascii.isEmpty(); }

    void clearText();
    bool saveText(QIODevice *out) const;
    bool savePdf(QIODevice *out) const;

public slots:
    void print(const QString &text);

signals:
    void textChanged();

private:
    QString m_atascii;   // exactly what the Atari sent
    QString m_ascii;     // same, with the inverse-video bit cleared
};

#endif // PRINTEROUTPUT_H
