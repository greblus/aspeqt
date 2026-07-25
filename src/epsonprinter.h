// Epson ESC/P printer emulation: a virtual print head that renders the Atari's
// print jobs onto a paper image. Taken from AspeQt-2k26 by
// Paul Jones <pjones1063@gmail.com>, GPL-2. See AUTHORS.txt.

#ifndef EPSONPRINTER_H
#define EPSONPRINTER_H

#include "sioworker.h"
#include <QByteArray>
#include <QImage>
#include <QPainter>
#include <QFont>
#include <QElapsedTimer>

class EpsonPrinter : public SioDevice
{
    Q_OBJECT
public:
    // Height of one printed page, in paper pixels. Public so a PDF export can
    // split the paper into pages the same way the print head does.
    static constexpr int PageLengthPx = 2376;

    EpsonPrinter(SioWorker *worker);
    void handleCommand(quint8 command, quint16 aux) override;

signals:
    void paperUpdated(const QImage &image);

public slots:
    void forceClear();
    // Re-render the job that is on the paper with a different font. The bytes
    // are kept for exactly this reason: the page is pixels, so the only way to
    // restyle it is to run the parser over the job again.
    void setFontFamily(const QString &family);
    // Feed bytes straight into the parser, bypassing SIO. Lets a captured print
    // job be replayed without an Atari, which is the only sane way to iterate
    // on ESC/P rendering.
    void feedBytes(const QByteArray &data);

private:
    int m_lastOperation;

    // --- Persistent State Machine Variables ---
    enum ParserState {
        State_Text,
        State_Escape,
        State_Graphic_N1,
        State_Graphic_N2,
        State_Graphic_Data,
        State_Spacing_A,
        State_Spacing_3,
        State_Feed_J,
        State_Underline,
        State_Expanded_W,
        State_MasterPrint,
        State_Proportional_p,
        State_SuperSub_S
    };

    ParserState m_state;
    int m_graphicBytesExpected;
    char m_currentGraphicMode;
    QByteArray m_currentGraphicPayload;
    QString m_currentTextLine;
    QByteArray m_rawJob;      // everything printed since the last clear
    QString m_fontFamily;     // empty = whatever the platform calls fixed-pitch

    // --- Virtual Print Head ---
    QImage m_paper;
    // Rate-limits the live preview: the paper is repainted for every 40-byte SIO
    // frame, and pushing a full page image to QML that often is wasteful.
    QElapsedTimer m_lastEmit;
    int m_cursorX;
    int m_cursorY;
    int m_lineHeight;

    // --- Hardware Font Flags ---
    bool m_isBold;
    bool m_isUnderlined;
    bool m_isItalic;
    bool m_isCondensed;
    bool m_isExpanded;

    // --- ENHANCED FONT FLAGS ---
    bool m_isElite;
    bool m_isProportional;
    int m_scriptMode; // 0 = Normal, 1 = Superscript, 2 = Subscript

    void initializePaper();
    void drawTextString(const QString &text);
    void drawGraphics(const QByteArray &payload);
    void lineFeed();
    void parsePrintJob(const QByteArray &data);
    char translateAtascii(unsigned char b);
    void fillPaperBackground(QImage &img);
    void rerender();          // wipe the paper and replay m_rawJob

};

#endif // EPSONPRINTER_H
