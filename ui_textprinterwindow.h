/********************************************************************************
** Form generated from reading UI file 'textprinterwindow.ui'
**
** Created by: Qt User Interface Compiler version 4.8.6
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_TEXTPRINTERWINDOW_H
#define UI_TEXTPRINTERWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QFontComboBox>
#include <QtGui/QGridLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QMainWindow>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QStatusBar>
#include <QtGui/QToolBar>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_TextPrinterWindow
{
public:
    QAction *actionSave;
    QAction *actionClear;
    QAction *actionWord_wrap;
    QAction *actionPrint;
    QAction *actionAtasciiFont;
    QAction *actionFont_Size;
    QAction *actionHideShow_Ascii;
    QAction *actionHideShow_Atascii;
    QAction *actionStrip_Line_Numbers;
    QWidget *centralwidget;
    QGridLayout *gridLayout;
    QPlainTextEdit *printerTextEdit;
    QPlainTextEdit *printerTextEditASCII;
    QLabel *atasciiFontName;
    QFontComboBox *asciiFontName;
    QStatusBar *statusbar;
    QToolBar *toolBar;

    void setupUi(QMainWindow *TextPrinterWindow)
    {
        if (TextPrinterWindow->objectName().isEmpty())
            TextPrinterWindow->setObjectName(QString::fromUtf8("TextPrinterWindow"));
        TextPrinterWindow->resize(666, 437);
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/icons/silk-icons/icons/printer.png"), QSize(), QIcon::Normal, QIcon::Off);
        TextPrinterWindow->setWindowIcon(icon);
        actionSave = new QAction(TextPrinterWindow);
        actionSave->setObjectName(QString::fromUtf8("actionSave"));
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/icons/silk-icons/icons/disk.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionSave->setIcon(icon1);
        actionClear = new QAction(TextPrinterWindow);
        actionClear->setObjectName(QString::fromUtf8("actionClear"));
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/icons/silk-icons/icons/page_white.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionClear->setIcon(icon2);
        actionWord_wrap = new QAction(TextPrinterWindow);
        actionWord_wrap->setObjectName(QString::fromUtf8("actionWord_wrap"));
        actionWord_wrap->setCheckable(true);
        actionWord_wrap->setChecked(false);
        QIcon icon3;
        icon3.addFile(QString::fromUtf8(":/icons/silk-icons/icons/text_align_left.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionWord_wrap->setIcon(icon3);
        actionPrint = new QAction(TextPrinterWindow);
        actionPrint->setObjectName(QString::fromUtf8("actionPrint"));
        actionPrint->setIcon(icon);
        actionAtasciiFont = new QAction(TextPrinterWindow);
        actionAtasciiFont->setObjectName(QString::fromUtf8("actionAtasciiFont"));
        QIcon icon4;
        icon4.addFile(QString::fromUtf8(":/icons/silk-icons/icons/font_go.png"), QSize(), QIcon::Normal, QIcon::Off);
        icon4.addFile(QString::fromUtf8(":/icons/silk-icons/icons/font_go.png"), QSize(), QIcon::Normal, QIcon::On);
        actionAtasciiFont->setIcon(icon4);
        actionAtasciiFont->setVisible(true);
        actionAtasciiFont->setIconVisibleInMenu(true);
        actionFont_Size = new QAction(TextPrinterWindow);
        actionFont_Size->setObjectName(QString::fromUtf8("actionFont_Size"));
        QIcon icon5;
        icon5.addFile(QString::fromUtf8(":/icons/oxygen-icons/16x16/actions/format_font_size_more.png"), QSize(), QIcon::Normal, QIcon::Off);
        icon5.addFile(QString::fromUtf8(":/icons/oxygen-icons/16x16/actions/format_font_size_more.png"), QSize(), QIcon::Normal, QIcon::On);
        actionFont_Size->setIcon(icon5);
        actionFont_Size->setVisible(true);
        actionHideShow_Ascii = new QAction(TextPrinterWindow);
        actionHideShow_Ascii->setObjectName(QString::fromUtf8("actionHideShow_Ascii"));
        actionHideShow_Ascii->setCheckable(true);
        actionHideShow_Ascii->setChecked(false);
        QIcon icon6;
        icon6.addFile(QString::fromUtf8(":/icons/oxygen-icons/16x16/actions/view_right_close.png"), QSize(), QIcon::Normal, QIcon::Off);
        icon6.addFile(QString::fromUtf8(":/icons/oxygen-icons/16x16/actions/view_right_close.png"), QSize(), QIcon::Normal, QIcon::On);
        actionHideShow_Ascii->setIcon(icon6);
        actionHideShow_Ascii->setVisible(true);
        actionHideShow_Atascii = new QAction(TextPrinterWindow);
        actionHideShow_Atascii->setObjectName(QString::fromUtf8("actionHideShow_Atascii"));
        actionHideShow_Atascii->setCheckable(true);
        actionHideShow_Atascii->setChecked(false);
        actionHideShow_Atascii->setEnabled(true);
        QIcon icon7;
        icon7.addFile(QString::fromUtf8(":/icons/oxygen-icons/16x16/actions/view_left_close.png"), QSize(), QIcon::Normal, QIcon::Off);
        icon7.addFile(QString::fromUtf8(":/icons/oxygen-icons/16x16/actions/view_left_close.png"), QSize(), QIcon::Normal, QIcon::On);
        actionHideShow_Atascii->setIcon(icon7);
        actionHideShow_Atascii->setVisible(true);
        actionStrip_Line_Numbers = new QAction(TextPrinterWindow);
        actionStrip_Line_Numbers->setObjectName(QString::fromUtf8("actionStrip_Line_Numbers"));
        actionStrip_Line_Numbers->setEnabled(true);
        QIcon icon8;
        icon8.addFile(QString::fromUtf8(":/icons/silk-icons/icons/cut_red.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionStrip_Line_Numbers->setIcon(icon8);
        centralwidget = new QWidget(TextPrinterWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(centralwidget->sizePolicy().hasHeightForWidth());
        centralwidget->setSizePolicy(sizePolicy);
        gridLayout = new QGridLayout(centralwidget);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        printerTextEdit = new QPlainTextEdit(centralwidget);
        printerTextEdit->setObjectName(QString::fromUtf8("printerTextEdit"));
        QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(printerTextEdit->sizePolicy().hasHeightForWidth());
        printerTextEdit->setSizePolicy(sizePolicy1);
        QFont font;
        font.setFamily(QString::fromUtf8("MS Shell Dlg 2"));
        font.setBold(true);
        font.setWeight(75);
        printerTextEdit->setFont(font);
        printerTextEdit->setMouseTracking(false);
        printerTextEdit->setLineWrapMode(QPlainTextEdit::WidgetWidth);
        printerTextEdit->setReadOnly(true);

        gridLayout->addWidget(printerTextEdit, 0, 0, 1, 1);

        printerTextEditASCII = new QPlainTextEdit(centralwidget);
        printerTextEditASCII->setObjectName(QString::fromUtf8("printerTextEditASCII"));
        printerTextEditASCII->setEnabled(true);
        printerTextEditASCII->setReadOnly(true);

        gridLayout->addWidget(printerTextEditASCII, 0, 1, 1, 1);

        atasciiFontName = new QLabel(centralwidget);
        atasciiFontName->setObjectName(QString::fromUtf8("atasciiFontName"));
        sizePolicy.setHeightForWidth(atasciiFontName->sizePolicy().hasHeightForWidth());
        atasciiFontName->setSizePolicy(sizePolicy);
        atasciiFontName->setMaximumSize(QSize(16777215, 16777215));

        gridLayout->addWidget(atasciiFontName, 1, 0, 1, 1);

        asciiFontName = new QFontComboBox(centralwidget);
        asciiFontName->setObjectName(QString::fromUtf8("asciiFontName"));
        QFont font1;
        font1.setFamily(QString::fromUtf8("MS Shell Dlg 2"));
        font1.setPointSize(8);
        asciiFontName->setFont(font1);
        QFont font2;
        font2.setFamily(QString::fromUtf8("Arial"));
        asciiFontName->setCurrentFont(font2);

        gridLayout->addWidget(asciiFontName, 1, 1, 1, 1);

        TextPrinterWindow->setCentralWidget(centralwidget);
        statusbar = new QStatusBar(TextPrinterWindow);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        TextPrinterWindow->setStatusBar(statusbar);
        toolBar = new QToolBar(TextPrinterWindow);
        toolBar->setObjectName(QString::fromUtf8("toolBar"));
        QSizePolicy sizePolicy2(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(toolBar->sizePolicy().hasHeightForWidth());
        toolBar->setSizePolicy(sizePolicy2);
        toolBar->setMovable(false);
        toolBar->setIconSize(QSize(16, 16));
        toolBar->setFloatable(false);
        TextPrinterWindow->addToolBar(Qt::TopToolBarArea, toolBar);

        toolBar->addAction(actionSave);
        toolBar->addAction(actionPrint);
        toolBar->addAction(actionAtasciiFont);
        toolBar->addAction(actionFont_Size);
        toolBar->addAction(actionHideShow_Atascii);
        toolBar->addAction(actionHideShow_Ascii);
        toolBar->addAction(actionStrip_Line_Numbers);
        toolBar->addAction(actionWord_wrap);
        toolBar->addAction(actionClear);

        retranslateUi(TextPrinterWindow);

        QMetaObject::connectSlotsByName(TextPrinterWindow);
    } // setupUi

    void retranslateUi(QMainWindow *TextPrinterWindow)
    {
        TextPrinterWindow->setWindowTitle(QApplication::translate("TextPrinterWindow", "AspeQt - Printer text output", 0, QApplication::UnicodeUTF8));
        actionSave->setText(QApplication::translate("TextPrinterWindow", "Save to a file...", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        actionSave->setToolTip(QApplication::translate("TextPrinterWindow", "Save contents to a file (Ctrl+S)", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_STATUSTIP
        actionSave->setStatusTip(QApplication::translate("TextPrinterWindow", "Save contents to a file", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_STATUSTIP
        actionSave->setShortcut(QApplication::translate("TextPrinterWindow", "Ctrl+S", 0, QApplication::UnicodeUTF8));
        actionClear->setText(QApplication::translate("TextPrinterWindow", "Clear", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        actionClear->setToolTip(QApplication::translate("TextPrinterWindow", "Clear contents (Ctrl+C)", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_STATUSTIP
        actionClear->setStatusTip(QApplication::translate("TextPrinterWindow", "Clear contents", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_STATUSTIP
        actionClear->setShortcut(QApplication::translate("TextPrinterWindow", "Ctrl+C", 0, QApplication::UnicodeUTF8));
        actionWord_wrap->setText(QApplication::translate("TextPrinterWindow", "Word wrap", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        actionWord_wrap->setToolTip(QApplication::translate("TextPrinterWindow", "Toggle word wrapping (Ctrl+W)", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_STATUSTIP
        actionWord_wrap->setStatusTip(QApplication::translate("TextPrinterWindow", "Toggle word wrapping", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_STATUSTIP
        actionWord_wrap->setShortcut(QApplication::translate("TextPrinterWindow", "Ctrl+W", 0, QApplication::UnicodeUTF8));
        actionPrint->setText(QApplication::translate("TextPrinterWindow", "Print", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        actionPrint->setToolTip(QApplication::translate("TextPrinterWindow", "Send contents to printer (Ctrl+P)", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_STATUSTIP
        actionPrint->setStatusTip(QApplication::translate("TextPrinterWindow", "Send contents to printer", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_STATUSTIP
        actionPrint->setShortcut(QApplication::translate("TextPrinterWindow", "Ctrl+P", 0, QApplication::UnicodeUTF8));
        actionAtasciiFont->setText(QApplication::translate("TextPrinterWindow", "Atascii Font", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        actionAtasciiFont->setToolTip(QApplication::translate("TextPrinterWindow", "Toggle ATASCII fonts (Alt+F)", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_STATUSTIP
        actionAtasciiFont->setStatusTip(QApplication::translate("TextPrinterWindow", "Toggle ATASCII fonts", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_STATUSTIP
        actionAtasciiFont->setShortcut(QApplication::translate("TextPrinterWindow", "Alt+F", 0, QApplication::UnicodeUTF8));
        actionFont_Size->setText(QApplication::translate("TextPrinterWindow", "Font Size", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        actionFont_Size->setToolTip(QApplication::translate("TextPrinterWindow", "Toggle Font Size (6, 9, 12 pt) (Alt+Shift+F)", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_STATUSTIP
        actionFont_Size->setStatusTip(QApplication::translate("TextPrinterWindow", "Toggle Font Size (6, 9, 12 pt)", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_STATUSTIP
        actionFont_Size->setShortcut(QApplication::translate("TextPrinterWindow", "Alt+Shift+F", 0, QApplication::UnicodeUTF8));
        actionHideShow_Ascii->setText(QApplication::translate("TextPrinterWindow", "Hide/Show Ascii", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        actionHideShow_Ascii->setToolTip(QApplication::translate("TextPrinterWindow", "Hide/Show Ascii Printer Output (Alt+Shift+H)", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_STATUSTIP
        actionHideShow_Ascii->setStatusTip(QApplication::translate("TextPrinterWindow", "Hide/Show Ascii Printer Output", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_STATUSTIP
        actionHideShow_Ascii->setShortcut(QApplication::translate("TextPrinterWindow", "Alt+Shift+H", 0, QApplication::UnicodeUTF8));
        actionHideShow_Atascii->setText(QApplication::translate("TextPrinterWindow", "HideShow_Atascii", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        actionHideShow_Atascii->setToolTip(QApplication::translate("TextPrinterWindow", " Hide/Show Atascii Printer Output (Alt+H)", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_STATUSTIP
        actionHideShow_Atascii->setStatusTip(QApplication::translate("TextPrinterWindow", "Hide/Show Atascii Printer Output", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_STATUSTIP
        actionHideShow_Atascii->setShortcut(QApplication::translate("TextPrinterWindow", "Alt+H", 0, QApplication::UnicodeUTF8));
        actionStrip_Line_Numbers->setText(QApplication::translate("TextPrinterWindow", "Strip Line Numbers", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        actionStrip_Line_Numbers->setToolTip(QApplication::translate("TextPrinterWindow", "Strip Line numbers from the text output (Ctrl-S)", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_STATUSTIP
        actionStrip_Line_Numbers->setStatusTip(QApplication::translate("TextPrinterWindow", "Strip Line numbers from the text output", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_STATUSTIP
        actionStrip_Line_Numbers->setShortcut(QApplication::translate("TextPrinterWindow", "Ctrl+S", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        printerTextEdit->setToolTip(QApplication::translate("TextPrinterWindow", "Atari Output (Atascii)", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_STATUSTIP
        printerTextEdit->setStatusTip(QApplication::translate("TextPrinterWindow", "Atari Output (Atascii)", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_STATUSTIP
        printerTextEdit->setPlainText(QString());
#ifndef QT_NO_TOOLTIP
        printerTextEditASCII->setToolTip(QApplication::translate("TextPrinterWindow", "Atari Output (Ascii)", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_STATUSTIP
        printerTextEditASCII->setStatusTip(QApplication::translate("TextPrinterWindow", "Atari Output (Ascii)", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_STATUSTIP
        atasciiFontName->setText(QString());
        toolBar->setWindowTitle(QApplication::translate("TextPrinterWindow", "toolBar", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class TextPrinterWindow: public Ui_TextPrinterWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_TEXTPRINTERWINDOW_H
