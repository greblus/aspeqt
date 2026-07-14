#include "logdisplaydialog.h"
#include "ui_logdisplaydialog.h"
#include "mainwindow.h"

#include <QTranslator>
#include <QDir>
#include <QMessageBox>
#include <QScreen>
#include <QScroller>
#include <QScrollerProperties>
#include <QLineEdit>
#include <QBoxLayout>
#include <QTextCursor>
#include <QColor>
#include <QPushButton>
#include <QIcon>
#include <QApplication>
#include <QComboBox>

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

    // Let the log fill the whole window. The old code capped the height to
    // screen-height-300 (hence the ~2/3 cut-off) and, worse, set the *width* to
    // the screen *height*.
    l_ui->textEdit->setMaximumHeight(QWIDGETSIZE_MAX);
    l_ui->textEdit->setMaximumWidth(QWIDGETSIZE_MAX);
    l_ui->textEdit->setReadOnly(true);
    l_ui->textEdit->setLineWrapMode(QTextEdit::WidgetWidth);

    // Smooth finger scrolling (grab the viewport, per-pixel, no overshoot).
    l_ui->textEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    l_ui->textEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    QScroller::grabGesture(l_ui->textEdit->viewport(), QScroller::LeftMouseButtonGesture);
    {
        QScrollerProperties sp = QScroller::scroller(l_ui->textEdit->viewport())->scrollerProperties();
        sp.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy,
                           QVariant::fromValue(QScrollerProperties::OvershootAlwaysOff));
        sp.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy,
                           QVariant::fromValue(QScrollerProperties::OvershootAlwaysOff));
        QScroller::scroller(l_ui->textEdit->viewport())->setScrollerProperties(sp);
    }

    l_ui->listByDisk->setMaximumHeight(38);
    l_ui->listByDisk->setMinimumWidth(0);
    l_ui->listByDisk->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    if (l_ui->gridLayout)
        l_ui->gridLayout->setContentsMargins(8, 48, 8, 8);

    QSize scr = qApp->screens().at(0)->size();
    int isz = qMin(scr.width(), scr.height()) * 60 / 800;

    QLineEdit *searchEdit = new QLineEdit(this);
    searchEdit->setPlaceholderText(tr("Search text…"));
    searchEdit->setClearButtonEnabled(true);
    searchEdit->setFixedHeight(38);

    auto runSearch = [this, searchEdit](bool next) {
        const QString needle = searchEdit->text();
        if (needle.isEmpty()) { l_ui->textEdit->setExtraSelections({}); return; }
        if (!next) {                       // live: always start from the top
            QTextCursor c = l_ui->textEdit->textCursor();
            c.movePosition(QTextCursor::Start);
            l_ui->textEdit->setTextCursor(c);
        }
        bool found = l_ui->textEdit->find(needle);
        if (!found) {                      // wrap
            QTextCursor c = l_ui->textEdit->textCursor();
            c.movePosition(QTextCursor::Start);
            l_ui->textEdit->setTextCursor(c);
            found = l_ui->textEdit->find(needle);
        }
        if (found) {
            QTextEdit::ExtraSelection es;
            es.cursor = l_ui->textEdit->textCursor();
            es.format.setBackground(QColor(255, 235, 59));
            l_ui->textEdit->setExtraSelections({ es });
            l_ui->textEdit->ensureCursorVisible();
        }
    };
    connect(searchEdit, &QLineEdit::textChanged, this, [runSearch]{ runSearch(false); });
    connect(searchEdit, &QLineEdit::returnPressed, this, [runSearch]{ runSearch(true); });

    QPushButton *nextBtn = new QPushButton(this);
    nextBtn->setIcon(QIcon(":/icons/tango-icons/actions/go-next.svg"));
    nextBtn->setFlat(true);
    nextBtn->setIconSize(QSize(isz, isz));
    connect(nextBtn, &QPushButton::clicked, this, [runSearch]{ runSearch(true); });

    QPushButton *closeBtn = new QPushButton(this);
    closeBtn->setIcon(QIcon(":/icons/tango-icons/actions/system-log-out.svg"));
    closeBtn->setFlat(true);
    closeBtn->setIconSize(QSize(isz, isz));
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::close);

    if (QHBoxLayout *hb = qobject_cast<QHBoxLayout *>(l_ui->groupBox->layout())) {
        hb->removeWidget(l_ui->buttonBox);
        l_ui->buttonBox->hide();
        hb->insertWidget(1, searchEdit);
        hb->addWidget(nextBtn);
        hb->addWidget(closeBtn);
        hb->setStretch(0, 0);   // disk combo: sized to its content
        hb->setStretch(1, 1);   // search: fills the rest
    }
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
