/********************************************************************************
** Form generated from reading UI file 'aboutdialog.ui'
**
** Created by: Qt User Interface Compiler version 4.8.6
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ABOUTDIALOG_H
#define UI_ABOUTDIALOG_H

#include <QtCore/QLocale>
#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QGridLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QTextBrowser>

QT_BEGIN_NAMESPACE

class Ui_AboutDialog
{
public:
    QGridLayout *gridLayout;
    QLabel *aspeqtLabel;
    QDialogButtonBox *buttonBox;
    QTextBrowser *textBrowser;
    QLabel *versionLabel;

    void setupUi(QDialog *AboutDialog)
    {
        if (AboutDialog->objectName().isEmpty())
            AboutDialog->setObjectName(QString::fromUtf8("AboutDialog"));
        AboutDialog->resize(640, 480);
        AboutDialog->setLocale(QLocale(QLocale::English, QLocale::UnitedStates));
        gridLayout = new QGridLayout(AboutDialog);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        aspeqtLabel = new QLabel(AboutDialog);
        aspeqtLabel->setObjectName(QString::fromUtf8("aspeqtLabel"));
        QFont font;
        font.setBold(true);
        font.setWeight(75);
        aspeqtLabel->setFont(font);
        aspeqtLabel->setLocale(QLocale(QLocale::English, QLocale::UnitedStates));
        aspeqtLabel->setAlignment(Qt::AlignCenter);

        gridLayout->addWidget(aspeqtLabel, 0, 0, 1, 1);

        buttonBox = new QDialogButtonBox(AboutDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setLocale(QLocale(QLocale::English, QLocale::UnitedStates));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Close);

        gridLayout->addWidget(buttonBox, 5, 0, 1, 1);

        textBrowser = new QTextBrowser(AboutDialog);
        textBrowser->setObjectName(QString::fromUtf8("textBrowser"));
        textBrowser->setSource(QUrl(QString::fromUtf8("qrc:/documentation/about.html")));

        gridLayout->addWidget(textBrowser, 2, 0, 1, 1);

        versionLabel = new QLabel(AboutDialog);
        versionLabel->setObjectName(QString::fromUtf8("versionLabel"));
        versionLabel->setFont(font);
        versionLabel->setAlignment(Qt::AlignCenter);

        gridLayout->addWidget(versionLabel, 1, 0, 1, 1);


        retranslateUi(AboutDialog);
        QObject::connect(buttonBox, SIGNAL(rejected()), AboutDialog, SLOT(reject()));
        QObject::connect(buttonBox, SIGNAL(accepted()), AboutDialog, SLOT(accept()));

        QMetaObject::connectSlotsByName(AboutDialog);
    } // setupUi

    void retranslateUi(QDialog *AboutDialog)
    {
        AboutDialog->setWindowTitle(QApplication::translate("AboutDialog", "About AspeQt", 0, QApplication::UnicodeUTF8));
        aspeqtLabel->setText(QApplication::translate("AboutDialog", "AspeQt: Atari Serial Peripheral Emulator for Qt", 0, QApplication::UnicodeUTF8));
        textBrowser->setHtml(QApplication::translate("AboutDialog", "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><title>AspeQt</title><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'MS Shell Dlg 2'; font-size:8.25pt; font-weight:400; font-style:normal;\">\n"
"<p style=\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-size:8pt;\">\302\240</span><img src=\":/icons/main-icon/AspeQt.ico\" /><span style=\" font-size:8pt; font-weight:600; color:#4c920e;\">\302\240 </span><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt; font-weight:600; font-style:italic; color:#4c920e;\">AspeQt, Atari Serial Peripheral Emulator for Qt</span><span style=\" font-size:10pt; font-weight:600; font-style:italic; color:#4c920e;\"><br /></span><span style=\" font-size:10pt;\"><br /></span><span style=\" font-family:'Arial,Helvetica"
                        ",sans-serif'; font-size:10pt; font-weight:600;\">Summary</span><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt;\"><br />AspeQt emulates Atari SIO peripherals when connected to an Atari 8-bit computer with an SIO2PC cable.<br />In that respect it's similar to programs like APE and Atari810. The main difference is that it's free (unlike APE) and it's cross-platform (unlike Atari810 and APE).<br /><br /></span><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt; font-weight:600;\">Some features</span><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt;\"><br />* Qt based GUI with drag and drop support.<br />* Cross-platform</span><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt; font-style:italic;\"> (currently Windows and x86-Linux)</span><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt;\"><br />* 15 disk drive emulation </span><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt; font"
                        "-style:italic;\">(drives 9-15 are only supported by SpartaDOS X)</span><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt;\"><br />* Text-only printer emulation with saving and printing of the output<br />* Cassette image playback<br />* Folders can be mounted as simulated Dos20s disks. </span><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt; font-style:italic;\">(read-only, now with SDX compatibility, and bootable)</span><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt;\"><br />* Atari executables can be booted directly, optionally with high speed.<br />* Contents of image files can be viewed / changed<br />* AspeQt Client module ASPECL.COM. Runs on the Atari and is used to get/set Date/Time on the Atari plus a variety of other remote tasks.<br />* Upto 6xSIO speed and more if the serial port adaptor supports it </span><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt; font-style:italic;\">(FTDI chip based cables are reco"
                        "mmanded).</span><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt;\"><br />* Localization support (Currently for English, German, Polish, Russian, Slovak, Spanish and Turkish)<br />* Multi-session support<br /><br /></span><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt; font-weight:600;\">License</span><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt;\"><br />Original code up to version 0.6.0 </span><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt; font-style:italic;\">Copyright 2009 by Fatih Ayg\303\274n. </span><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt;\"><br />Updates since v0.6.0 </span><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt; font-style:italic;\">Copyright 2012 by Ray Ataergin</span><span style=\" font-size:10pt;\"> </span></p>\n"
"<p style=\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">"
                        "<span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt;\">You can freely copy, use, modify and distribute AspeQt under the GPL 2.0 license. Please see license.txt for details.</span><span style=\" font-size:10pt;\"> </span></p>\n"
"<p style=\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt;\">Qt libraries: </span><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt; font-style:italic;\">Copyright 2009 Nokia Corporation</span><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt;\"> and/or its subsidiary(-ies). Used in this package under LGPL 2.0 license.<br /><br />Silk Icons: </span><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt; font-style:italic;\">Copyright </span><a href=\"www.famfamfam.com\"><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt; font-style:italic; text-decora"
                        "tion: underline; color:#0000ff;\">Mark James</span></a><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt; font-style:italic;\">.</span><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt;\"> Used in this package under Creative Commons Attribution 3.0 license.<br /><br />Additional Icons by </span><a href=\"http://www.oxygen-icons.org/\"><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt; font-style:italic; text-decoration: underline; color:#0000ff;\">Oxygen Team</span></a><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt;\">. Used in this package under Creative Commons Attribution-ShareAlike 3.0 license.<br /><br />AtariSIO Linux kernel module and high speed code used in the EXE loader </span><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt; font-style:italic;\">Copyright </span><a href=\"http://www.horus.com/~hias/atari/\"><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt; font-sty"
                        "le:italic; text-decoration: underline; color:#0000ff;\">Matthias Reichl.</span></a><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt;\"> Used in this package under GPL 2.0 license.<br /><br />Atascii Fonts by </span><a href=\"http://members.bitstream.net/~marksim/atarimac\"><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt; font-style:italic; text-decoration: underline; color:#0000ff;\">Mark Simonson</span></a><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt;\">. Used in this package under Freeware License.</span><span style=\" font-size:10pt;\"><br /><br /></span><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt; font-weight:600; font-style:italic;\">DOS files distributed with AspeQt</span><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt; font-style:italic;\"> are copyright of their respective owners, Atari8Warez and AspeQt distributes those files with the understanding that they are either abandonw"
                        "are or public domain, and are widely available for download through the internet. If you are the copyright holder of one or more of these files, and believe that distribution of these files constitutes a breach of your rights please contact </span><a href=\"mailto:ray@atari8warez.com\"><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt; font-style:italic; text-decoration: underline; color:#0000ff;\">Atari8Warez</span></a><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt; font-style:italic;\">. We respect the rights of copyright holders and won't distribute copyrighted work without the rights holder's consent. </span></p>\n"
"<p style=\" margin-top:12px; margin-bottom:12px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt; font-weight:600;\">Contact</span><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt;\"><br />Ray Ataergin, </span><a href=\"mailto:r"
                        "ay@atari8warez.com\"><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt; text-decoration: underline; color:#0000ff;\">email</span></a><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt;\">, </span><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt; font-style:italic;\">Please include the word &quot;aspeqt&quot; in the subject field.</span><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt;\"><br /><br />Fatih Aygun, </span><a href=\"mailto:cyco130@yahoo.com\"><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt; text-decoration: underline; color:#0000ff;\">email</span></a><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt;\">, </span><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt; font-style:italic;\">Please include the word &quot;aspeqt&quot; in the subject field.</span><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt;\"><br /><br />For SIO"
                        "2PC/10502PC hardware purchase/support visit </span><a href=\"http://www.atari8warez.com\"><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt; text-decoration: underline; color:#0000ff;\">Atari8Warez</span></a><span style=\" font-family:'Arial,Helvetica,sans-serif'; font-size:10pt;\"><br /></span></p></body></html>", 0, QApplication::UnicodeUTF8));
        versionLabel->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class AboutDialog: public Ui_AboutDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ABOUTDIALOG_H
