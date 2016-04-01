#include "cassettedialog.h"
#include "ui_cassettedialog.h"

#include <QTimer>
#include <QtDebug>
#include <QScreen>

CassetteDialog::CassetteDialog(QWidget *parent, const QString &fileName)
    : QDialog(parent), ui(new Ui::CassetteDialog)
{
    Qt::WindowFlags flags = windowFlags();
    flags = flags & (~Qt::WindowContextHelpButtonHint);
    setWindowFlags(flags);

    mFileName = fileName;

    ui->setupUi(this);

#ifdef Q_OS_ANDROID
    QScreen *screen = qApp->screens().at(0);

    int rx = screen->size().width();
    int ry = screen->size().height();

    int ts = (rx > ry) ? ry : rx;

//    this->resize(nx, ny);
    this->setGeometry(0, 0, rx, ry);

    ui->verticalLayoutWidget->setGeometry(0, 0, rx, ry);
    ui->verticalLayoutWidget->setMaximumWidth(rx);
    ui->verticalLayoutWidget->setMaximumHeight(ry);

    ui->progressBar->setMaximumWidth(rx*0.8);

    int bs = ts*70/800;
    ui->buttonBox->setMaximumHeight(bs+30);
    ui->label->setMaximumWidth(rx*0.8);
    ui->verticalLayoutWidget->setContentsMargins(int(rx*0.2/2), int(ry/8), int(rx*0.2/2), int(ry/10));
#endif
//    ui->progressBar->setVisible(false);
    worker = new CassetteWorker;
    mTotalDuration = worker->mTotalDuration;
    mRemainingTime = mTotalDuration;
    int minutes = mRemainingTime / 60000;
    int seconds = (mRemainingTime - minutes * 60000)/1000;
    ui->label->setText(tr("AspeQt is ready to playback the cassette image file '%1'.\n\n"
                          "The estimated playback duration is: %2:%3\n\n"
                          "Do whatever is necessary in your Atari to load this cassette "
                          "image like rebooting while holding Option and Start buttons "
                          "or entering \"CLOAD\" in the BASIC prompt.\n\n"
                          "When you hear the beep sound, push the OK button below and press "
                          "a key on your Atari at about the same time.")
                       .arg(mFileName)
                       .arg(minutes)
                       .arg(seconds, 2, 10, QChar('0')));

    connect(worker, SIGNAL(statusChanged(int)), this, SLOT(progress(int)), Qt::QueuedConnection);
    connect(worker, SIGNAL(finished()), this, SLOT(reject()));
}

CassetteDialog::~CassetteDialog()
{
    disconnect(this, SLOT(reject()));
    if (worker->isRunning()) {
        worker->setPriority(QThread::NormalPriority);
        worker->wait();
    }
    delete worker;
    delete ui;
}

void CassetteDialog::changeEvent(QEvent *e)
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

int CassetteDialog::exec()
{
    if (!worker->loadCasImage(mFileName)) {
        return 0;
    }

    return QDialog::exec();
}

void CassetteDialog::accept()
{
    ui->buttonBox->setStandardButtons(QDialogButtonBox::Cancel);
    int minutes = mRemainingTime / 60000;
    int seconds = (mRemainingTime - minutes * 60000)/1000;
    ui->label->setText(tr("Playing back cassette image.\n\n"
                          "Estimated time left: %1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0')));
    ui->progressBar->setVisible(true);

    worker->start(QThread::TimeCriticalPriority);
    mTimer = new QTimer(this);
    connect(mTimer, SIGNAL(timeout()), this, SLOT(tick()));
    mTimer->start(1000);
}

void CassetteDialog::progress(int remainingTime)
{
    mRemainingTime = remainingTime;
    int minutes = mRemainingTime / 60000;
    int seconds = (mRemainingTime - minutes * 60000)/1000;
    ui->label->setText(tr("Playing back cassette image.\n\n"
                          "Estimated time left: %1:%2").arg(minutes).arg(seconds, 2, 10, QChar('0')));
    ui->progressBar->setMaximum(mTotalDuration);
    ui->progressBar->setValue(mTotalDuration - mRemainingTime);
}

void CassetteDialog::tick()
{
    if (mRemainingTime < 1000) {
        mRemainingTime = 1000;
    }
    emit progress(mRemainingTime - 1000);
}
