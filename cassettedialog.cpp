#include "cassettedialog.h"
#include "ui_cassettedialog.h"

#include <QTimer>
#include <QtDebug>
#include <QScreen>
#include <QMovie>

QMovie *movie = NULL;

CassetteDialog::CassetteDialog(QWidget *parent, const QString &fileName)
    : QDialog(parent), ui(new Ui::CassetteDialog)
{
    Qt::WindowFlags flags = windowFlags();
    flags = flags & (~Qt::WindowContextHelpButtonHint);
    setWindowFlags(flags);
    setWindowState(Qt::WindowFullScreen);

    mFileName = fileName;

    ui->setupUi(this);

#ifdef Q_OS_ANDROID
    movie = new QMovie(":images/tape.gif");
    ui->tape_label->setMovie(movie);
    ui->tape_label->setAlignment(Qt::AlignCenter);
    movie->start();
    movie->setPaused(true);
#endif
    ui->progressBar->setVisible(true);
    worker = new CassetteWorker;
    mTotalDuration = worker->mTotalDuration;
    mRemainingTime = mTotalDuration;
    int minutes = mRemainingTime / 60000;
    int seconds = (mRemainingTime - minutes * 60000)/1000;
    ui->label->setText(tr("AspeQt is ready to playback the cassette image file '%1'.\n\n"
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

    worker->start(QThread::TimeCriticalPriority);
    mTimer = new QTimer(this);
    connect(mTimer, SIGNAL(timeout()), this, SLOT(tick()));
    mTimer->start(1000);
    movie->setPaused(false);
}

void CassetteDialog::progress(int remainingTime)
{
    mTotalDuration = worker->mTotalDuration;
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
