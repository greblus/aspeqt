#include "aboutdialog.h"
#include "ui_aboutdialog.h"
#include <QScreen>

AboutDialog::AboutDialog(QWidget *parent, QString version) :
    QDialog(parent),
    m_ui(new Ui::AboutDialog)
{
    Qt::WindowFlags flags = windowFlags();
    flags = flags & (~Qt::WindowContextHelpButtonHint);
    setWindowFlags(flags);
    setWindowState(Qt::WindowFullScreen);

    m_ui->setupUi(this);

    m_ui->versionLabel->setText(tr("version %1").arg(version));

    QScreen *screen = qApp->screens().at(0);
    int rx = screen->availableSize().width();
    int ry = screen->availableSize().height();

    this->setMaximumWidth(rx);
    this->setMaximumHeight(ry);

    m_ui->textBrowser->setMinimumWidth(rx-20);
    m_ui->textBrowser->setMinimumHeight(ry-200);

    //connect(this, SIGNAL(accepted()), this, SLOT(AboutDialog_accepted()));
}

AboutDialog::~AboutDialog()
{
    delete m_ui;
}

void AboutDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
