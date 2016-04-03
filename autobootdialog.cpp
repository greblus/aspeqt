#include "autobootdialog.h"
#include "ui_autobootdialog.h"
#include "mainwindow.h"

#include <QTime>
#include <QDebug>
#include <QScreen>

extern QString g_exefileName;
bool reload;

AutoBootDialog::AutoBootDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AutoBootDialog)
{
    QScreen *screen = qApp->screens().at(0);

    ui->setupUi(this);
    ui->progressBar->setVisible(false);

    #ifdef Q_OS_ANDROID
    int rx = screen->availableSize().width();
    int ry = screen->availableSize().height();

    int ts = (rx > ry) ? ry : rx;

    this->setGeometry(0, 0, rx, ry);

    ui->verticalLayoutWidget->setGeometry(0, 0, rx, ry);
    ui->verticalLayoutWidget->setMaximumWidth(rx);
    ui->verticalLayoutWidget->setMaximumHeight(ry);

    ui->progressBar->setMaximumWidth(rx*0.8);

    int bs = ts*70/800;
    ui->pushButton1->setMaximumHeight(bs+30);
    ui->pushButton1->setMaximumWidth(rx*0.8);
    ui->pushButton2->setMaximumHeight(bs+30);
    ui->pushButton2->setMaximumWidth(rx*0.8);

    ui->label->setMaximumWidth(rx*0.8);
    ui->verticalLayoutWidget->setContentsMargins(int(rx*0.2/2), int(ry/8), int(rx*0.2/2), 0);

    connect(ui->pushButton1, SIGNAL(clicked()), this, SLOT(reloadExe()));
    connect(ui->pushButton2, SIGNAL(clicked(QAbstractButton*)), this, SLOT(onClick(QAbstractButton*)));
    connect(ui->pushButton2, SIGNAL(clicked()), this, SLOT(close()));
    #else
    connect(ui->buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(onClick(QAbstractButton*)));
    #endif
    reload = false;
    ui->progressBar->setVisible(true);
}

AutoBootDialog::~AutoBootDialog()
{
    delete ui;
}

void AutoBootDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void AutoBootDialog::closeEvent(QCloseEvent *)
{
    if (!reload) g_exefileName = "";
}

void AutoBootDialog::booterStarted()
{
    ui->label->setText(tr("Atari is loading the booter."));
}

void AutoBootDialog::booterLoaded()
{
    ui->label->setText(tr("Atari is loading the program.\n\nFor some programs you may have to close this dialog manually when the program starts."));
}

void AutoBootDialog::blockRead(int current, int all)
{
    ui->progressBar->setMaximum(all);
    ui->progressBar->setValue(current);
}

void AutoBootDialog::loaderDone()
{
    accept();
}
void AutoBootDialog::onClick(QAbstractButton *button)
{
    if(button->text() == "Cancel") {
        g_exefileName="";
        return;
    }
}

void AutoBootDialog::reloadExe()
{
    reload = true;
    close();
}
