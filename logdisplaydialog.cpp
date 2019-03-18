#include "logdisplaydialog.h"
#include "ui_logdisplaydialog.h"
#include "mainwindow.h"

#include <QTranslator>
#include <QDir>
#include <QMessageBox>
#include <QScreen>
#include <QScroller>

QString g_savedLog, g_filter;
extern bool g_logOpen;

LogDisplayDialog::LogDisplayDialog(QWidget *parent) :
    QDialog(parent),
    l_ui(new Ui::LogDisplayDialog)
{
    Qt::WindowFlags flags = windowFlags();
    flags = flags & (~Qt::WindowContextHelpButtonHint);
    setWindowFlags(flags);

    l_ui->setupUi(this);


#ifdef Q_OS_ANDROID
    setWindowState(Qt::WindowFullScreen);
    QScreen *screen = qApp->screens().at(0);
    int rx = screen->availableSize().width();
    int ry = screen->availableSize().height();

    l_ui->textEdit->setMaximumHeight(ry-300);
    l_ui->textEdit->setMaximumWidth(ry);

    QWidget *w = l_ui->textEdit;
    QScroller::grabGesture(w, QScroller::TouchGesture);
#endif

    connect(l_ui->listByDisk, SIGNAL(currentIndexChanged(QString)), this, SLOT(diskFilter()));
    connect(l_ui->buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(onClick(QAbstractButton*)));
}

LogDisplayDialog::~LogDisplayDialog()
{
    delete l_ui;
}
void LogDisplayDialog::onClick(QAbstractButton* button)
{
    g_logOpen = false;
    return;
}

void LogDisplayDialog::closeEvent(QCloseEvent *)
{
    g_logOpen = false;
}

void LogDisplayDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        l_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void LogDisplayDialog::getLogText(QString logText)
{
    g_savedLog.clear();
    g_filter = "ALL";
    l_ui->listByDisk->setCurrentIndex(0);
    l_ui->textEdit->clear();
    l_ui->textEdit->ensureCursorVisible();
    if (!logText.isEmpty()){
        l_ui->textEdit->setHtml(logText);
        g_savedLog.append(logText);
    }
}
void LogDisplayDialog::getLogTextChange (QString logChange)
{
    if (g_filter == "ALL" || logChange.contains("["+g_filter+"]")) {
        l_ui->textEdit->append(logChange);
    }
        g_savedLog.append(logChange);
        g_savedLog.append("<br>");
}

void LogDisplayDialog::diskFilter()
{
    QTextEdit searchResults;
    QTextDocument  *search = l_ui->textEdit->document();
    QTextCursor cursor;
    g_filter = l_ui->listByDisk->currentText();
    searchResults.clear();
    if (g_filter != "ALL") {
        cursor.setPosition(0);
        cursor = search->find(g_filter,cursor,QTextDocument::FindWholeWords);
        while (!cursor.isNull()) {
            int i = cursor.position();
            cursor.setPosition(i-g_filter.length()-1);
            cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
            searchResults.append(cursor.selectedText());
            cursor.setPosition(cursor.position(), QTextCursor::MoveAnchor);
            cursor = search->find(g_filter,cursor,QTextDocument::FindWholeWords);
        }
        l_ui->textEdit->setHtml(searchResults.toHtml());
    } else {
        l_ui->textEdit->clear();
        l_ui->textEdit->setHtml(g_savedLog);
    }
    show();
}
