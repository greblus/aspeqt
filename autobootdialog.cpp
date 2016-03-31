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
    int rx = screen->size().width();
    int ry = screen->size().height();

    int bs, ts, nx, ny;
    ts = (rx > ry) ? ry : rx;
    nx = ts*0.8; ny = ts;

    this->resize(nx, ny);
    this->setGeometry(rx/2-nx/2, ry/2-ny/2, nx, ny);

    ui->verticalLayoutWidget->setGeometry(0, 0, nx, ny);
    ui->verticalLayoutWidget->setMaximumWidth(nx);
    ui->verticalLayoutWidget->setMaximumHeight(ny);

    ui->progressBar->setMaximumWidth(nx*0.8);

    bs = ts*70/800;
    ui->pushButton1->setMaximumHeight(bs+30);
    ui->pushButton1->setMaximumWidth(nx*0.8);
    ui->pushButton2->setMaximumHeight(bs+30);
    ui->pushButton2->setMaximumWidth(nx*0.8);

    ui->label->setMaximumWidth(nx*0.8);

    connect(ui->pushButton2, SIGNAL(clicked()), this, SLOT(reject()));
    connect(ui->pushButton1, SIGNAL(clicked()), this, SLOT(reloadExe()));

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
