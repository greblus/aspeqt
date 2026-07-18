#include "printeroutput.h"

#include <QFont>
#include <QIODevice>
#include <QPageSize>
#include <QPdfWriter>
#include <QTextDocument>
#include <QTextStream>

PrinterOutput::PrinterOutput(QObject *parent)
    : QObject(parent)
{
}

void PrinterOutput::print(const QString &text)
{
    m_atascii += text;

    // The Atari marks inverse video with the high bit; clear it for the
    // readable copy, leaving the raw one for the ATASCII font view.
    QString ascii = text;
    for (int i = 0; i < ascii.size(); ++i) {
        const ushort c = ascii.at(i).unicode();
        if (c >= 0x80)
            ascii[i] = QChar(c ^ 0x80);
    }
    m_ascii += ascii;

    emit textChanged();
}

void PrinterOutput::clearText()
{
    m_atascii.clear();
    m_ascii.clear();
    emit textChanged();
}

bool PrinterOutput::saveText(QIODevice *out) const
{
    if (!out)
        return false;
    QTextStream stream(out);
    stream << m_ascii;
    stream.flush();
    return stream.status() == QTextStream::Ok;
}

bool PrinterOutput::savePdf(QIODevice *out) const
{
    if (!out)
        return false;

    QPdfWriter pdf(out);
    pdf.setPageSize(QPageSize(QPageSize::A4));
    pdf.setResolution(300);

    QTextDocument doc;
    QFont font(QStringLiteral("monospace"));
    font.setStyleHint(QFont::Monospace);   // a printout only lines up in a fixed pitch
    font.setPointSize(10);
    doc.setDefaultFont(font);
    doc.setPlainText(m_ascii);
    doc.print(&pdf);
    return true;
}
