#include "optionsdialog.h"
#include "ui_optionsdialog.h"
#include "aspeqtsettings.h"

#include <QTranslator>
#include <QDir>
#include <QScreen>
#include <QSize>
#include <QScroller>

#ifdef Q_OS_ANDROID
#include <QAndroidJniObject>
#endif

OptionsDialog::OptionsDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::OptionsDialog)
{
    Qt::WindowFlags flags = windowFlags();
    flags = flags & (~Qt::WindowContextHelpButtonHint);
    setWindowFlags(flags);

    m_ui->setupUi(this);

#ifdef Q_OS_ANDROID
    QWidget *w = m_ui->scrollArea->viewport();
    QScroller::grabGesture(w, QScroller::LeftMouseButtonGesture);
    setWindowState(Qt::WindowFullScreen);
    
    QScreen *screen = qApp->screens().at(0);
    int rx = screen->availableSize().width();
    int ry = screen->availableSize().height();

    this->setMinimumWidth(rx);
    this->setMinimumHeight(ry);

    m_ui->scrollArea->resize(rx,ry);
    
    int wsize;
    if (rx > ry)
        wsize = ry*90/800;
    else
        wsize = rx*90/800;

    m_ui->serialPortHandshakeCombo->setMinimumHeight(wsize);
    m_ui->serialPortBaudCombo->setMinimumHeight(wsize);
    m_ui->serialPortDivisorEdit->setMinimumHeight(wsize);
    m_ui->emulationCustomCasBaudSpin->setMinimumHeight(wsize);
    m_ui->i18nLanguageCombo->setMinimumHeight(wsize);
    m_ui->pushButton->setMinimumHeight(wsize);
#else
    m_ui->treeWidget->expandAll();
    itemStandard = m_ui->treeWidget->topLevelItem(0)->child(0);
    itemAtariSio = m_ui->treeWidget->topLevelItem(0)->child(1);
    itemEmulation = m_ui->treeWidget->topLevelItem(1);
    itemI18n = m_ui->treeWidget->topLevelItem(2);
#endif

//#ifndef Q_OS_LINUX
//    m_ui->treeWidget->topLevelItem(0)->removeChild(itemAtariSio);
//#endif

    connect(this, SIGNAL(accepted()), this, SLOT(OptionsDialog_accepted()));

    /* Retrieve application settings */
    #ifndef Q_OS_ANDROID
    m_ui->serialPortDeviceNameEdit->setText(aspeqtSettings->serialPortName());
    #endif
    m_ui->serialPortInterfaceCombo->setCurrentIndex(aspeqtSettings->serialPortInterface());
    m_ui->serialPortHandshakeCombo->setCurrentIndex(aspeqtSettings->serialPortHandshakingMethod());
    m_ui->serialPortBaudCombo->setCurrentIndex(aspeqtSettings->serialPortMaximumSpeed());
    m_ui->serialPortUseDivisorsBox->setChecked(aspeqtSettings->serialPortUsePokeyDivisors());
    m_ui->serialPortDivisorEdit->setValue(aspeqtSettings->serialPortPokeyDivisor());
    #ifndef Q_OS_ANDROID
    m_ui->atariSioDriverNameEdit->setText(aspeqtSettings->atariSioDriverName());
    m_ui->atariSioHandshakingMethodCombo->setCurrentIndex(aspeqtSettings->atariSioHandshakingMethod());
    #endif
    m_ui->emulationHighSpeedExeLoaderBox->setChecked(aspeqtSettings->useHighSpeedExeLoader());
    m_ui->emulationUseCustomCasBaudBox->setChecked(aspeqtSettings->useCustomCasBaud());
    m_ui->emulationCustomCasBaudSpin->setValue(aspeqtSettings->customCasBaud());
//    m_ui->minimizeToTrayBox->setChecked(aspeqtSettings->minimizeToTray());
    m_ui->saveWinPosBox->setChecked(aspeqtSettings->saveWindowsPos());
//    m_ui->saveDiskVisBox->setChecked(aspeqtSettings->saveDiskVis());
    m_ui->filterUscore->setChecked(aspeqtSettings->filterUnderscore());
    m_ui->useLargerFont->setChecked(aspeqtSettings->useLargeFont());
//    m_ui->enableShade->setChecked(aspeqtSettings->enableShade());

    m_ui->serialPortInterfaceCombo->setCurrentIndex(aspeqtSettings->serialPortInterface());
    m_ui->writeACKDelayEdit->setValue(aspeqtSettings->writeACKDelay());

    if (aspeqtSettings->serialPortInterface() == SIO2BT)
    {
        m_ui->writeACKDelayEdit->setEnabled(true);
        m_ui->writeACKDelayLabel->setEnabled(true);
        m_ui->serialPortBaudCombo->setDisabled(true);
        m_ui->serialPortBaudLabel->setDisabled(true);
        m_ui->serialPortDivisorEdit->setDisabled(true);
        m_ui->serialPortDivisorLabel->setDisabled(true);
        m_ui->serialPortUseDivisorsBox->setDisabled(true);
        m_ui->emulationHighSpeedExeLoaderBox->setDisabled(true);
        m_ui->serialPortUseDivisorsBox->setStyleSheet("QCheckBox:!enabled {color: grey;}");
        m_ui->emulationHighSpeedExeLoaderBox->setStyleSheet("QCheckBox:!enabled {color: grey;}");
    }

#ifndef Q_OS_ANDROID
    switch (aspeqtSettings->backend()) {
        case 0:
            itemStandard->setCheckState(0, Qt::Checked);
            itemAtariSio->setCheckState(0, Qt::Unchecked);
            m_ui->treeWidget->setCurrentItem(itemStandard);
            break;
        case 1:
            itemStandard->setCheckState(0, Qt::Unchecked);
            itemAtariSio->setCheckState(0, Qt::Checked);
            m_ui->treeWidget->setCurrentItem(itemAtariSio);
            break;
    }
   // m_ui->serialPortBox->setCheckState(itemStandard->checkState(0));
    m_ui->atariSioBox->setCheckState(itemAtariSio->checkState(0));
#endif
    
    /* list available translations */
    QTranslator local_translator;
    m_ui->i18nLanguageCombo->clear();
    m_ui->i18nLanguageCombo->addItem(tr("Automatic"), "auto");
    if (aspeqtSettings->i18nLanguage().compare("auto") == 0)
      m_ui->i18nLanguageCombo->setCurrentIndex(0);
    m_ui->i18nLanguageCombo->addItem(QT_TR_NOOP("English"), "en");
    if (aspeqtSettings->i18nLanguage().compare("en") == 0)
      m_ui->i18nLanguageCombo->setCurrentIndex(1);
    QDir dir(":/translations/i18n/");
    QStringList filters;
    filters << "aspeqt_*.qm";
    dir.setNameFilters(filters);
    for (int i = 0; i < dir.entryList().size(); ++i) {
        local_translator.load(":/translations/i18n/" + dir.entryList()[i]);
    m_ui->i18nLanguageCombo->addItem(local_translator.translate("OptionsDialog", "English"), dir.entryList()[i].mid(7).replace(".qm", ""));
	if (dir.entryList()[i].mid(7).replace(".qm", "").compare(aspeqtSettings->i18nLanguage()) == 0) {
        m_ui->i18nLanguageCombo->setCurrentIndex(i+2);
	}
    }
}

OptionsDialog::~OptionsDialog()
{
    delete m_ui;
}

void OptionsDialog::changeEvent(QEvent *e)
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

void OptionsDialog::OptionsDialog_accepted()
{
    #ifndef Q_OS_ANDROID
    aspeqtSettings->setSerialPortName(m_ui->serialPortDeviceNameEdit->text());
    #else
    aspeqtSettings->setSerialPortName(m_ui->serialPortInterfaceCombo->currentText());
    aspeqtSettings->setSerialPortInterface(m_ui->serialPortInterfaceCombo->currentIndex());
    aspeqtSettings->setWriteACKDelay(m_ui->writeACKDelayEdit->value());
    #endif
    aspeqtSettings->setSerialPortHandshakingMethod(m_ui->serialPortHandshakeCombo->currentIndex());
    aspeqtSettings->setSerialPortMaximumSpeed(m_ui->serialPortBaudCombo->currentIndex());
    aspeqtSettings->setSerialPortUsePokeyDivisors(m_ui->serialPortUseDivisorsBox->isChecked());
    aspeqtSettings->setSerialPortPokeyDivisor(m_ui->serialPortDivisorEdit->value());
    #ifndef Q_OS_ANDROID
    aspeqtSettings->setAtariSioDriverName(m_ui->atariSioDriverNameEdit->text());
    aspeqtSettings->setAtariSioHandshakingMethod(m_ui->atariSioHandshakingMethodCombo->currentIndex());
    #endif
    aspeqtSettings->setUseHighSpeedExeLoader(m_ui->emulationHighSpeedExeLoaderBox->isChecked());
    aspeqtSettings->setUseCustomCasBaud(m_ui->emulationUseCustomCasBaudBox->isChecked());
    aspeqtSettings->setCustomCasBaud(m_ui->emulationCustomCasBaudSpin->value());
//    aspeqtSettings->setMinimizeToTray(m_ui->minimizeToTrayBox->isChecked());
    aspeqtSettings->setsaveWindowsPos(m_ui->saveWinPosBox->isChecked());
//    aspeqtSettings->setsaveDiskVis(m_ui->saveDiskVisBox->isChecked());
    aspeqtSettings->setfilterUnderscore(m_ui->filterUscore->isChecked());
    aspeqtSettings->setUseLargeFont(m_ui->useLargerFont->isChecked());
//    aspeqtSettings->setEnableShade(m_ui->enableShade->isChecked());

    int serial_int = aspeqtSettings->serialPortInterface();
    QAndroidJniObject::callStaticMethod<void>("net/greblus/SerialActivity", "changeDevice", "(I)V", serial_int);

    int backend = 0;
#ifndef Q_OS_ANDROID
    if (itemAtariSio->checkState(0) == Qt::Checked) {
        backend = 1;
    }
#endif

    aspeqtSettings->setBackend(backend);
    aspeqtSettings->setI18nLanguage(m_ui->i18nLanguageCombo->itemData(m_ui->i18nLanguageCombo->currentIndex()).toString());
}

void OptionsDialog::on_treeWidget_currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* /*previous*/)
{
#ifndef Q_OS_ANDROID
    if (current == itemStandard) {
        m_ui->stackedWidget->setCurrentIndex(0);
    } else if (current == itemAtariSio) {
        m_ui->stackedWidget->setCurrentIndex(1);
    } else if (current == itemEmulation) {
        m_ui->stackedWidget->setCurrentIndex(2);
    } else if (current == itemI18n) {
	m_ui->stackedWidget->setCurrentIndex(3);
    }
#endif
}

void OptionsDialog::on_treeWidget_itemClicked(QTreeWidgetItem* item, int /*column*/)
{
#ifndef Q_OS_ANDROID
    if (item->checkState(0) == Qt::Checked) {
        if (item != itemStandard) {
            itemStandard->setCheckState(0, Qt::Unchecked);
        }
        if (item != itemAtariSio) {
            itemAtariSio->setCheckState(0, Qt::Unchecked);
        }
    } else if ((itemStandard->checkState(0) == Qt::Unchecked) &&
               (itemAtariSio->checkState(0) == Qt::Unchecked)) {
        item->setCheckState(0, Qt::Checked);
    }
    //m_ui->serialPortBox->setCheckState(itemStandard->checkState(0));
    m_ui->atariSioBox->setCheckState(itemAtariSio->checkState(0));
#endif
}

void OptionsDialog::on_serialPortUseDivisorsBox_toggled(bool checked)
{
//    m_ui->serialPortBaudLabel->setEnabled(!checked);
    m_ui->serialPortBaudCombo->setEnabled(!checked);
    m_ui->serialPortDivisorLabel->setEnabled(checked);
    m_ui->serialPortDivisorEdit->setEnabled(checked);
}

void OptionsDialog::on_emulationUseCustomCasBaudBox_toggled(bool checked)
{
    m_ui->emulationCustomCasBaudSpin->setEnabled(checked);
}

void OptionsDialog::on_serialPortInterfaceCombo_currentIndexChanged(int index)
{
    if (index == SIO2BT) {
        m_ui->writeACKDelayEdit->setEnabled(true);
        m_ui->serialPortHandshakeCombo->setCurrentIndex(3); //SOFT
        m_ui->serialPortHandshakeCombo->setDisabled(true);
        m_ui->serialPortHandshakeLabel->setDisabled(true);
        m_ui->serialPortBaudCombo->setDisabled(true);
        m_ui->serialPortBaudLabel->setDisabled(true);
        m_ui->serialPortDivisorLabel->setDisabled(true);
        m_ui->serialPortDivisorLabel->setDisabled(true);
        m_ui->serialPortDivisorEdit->setDisabled(true);
        m_ui->serialPortUseDivisorsBox->setDisabled(true);
        m_ui->serialPortUseDivisorsBox->setChecked(false);
        m_ui->emulationHighSpeedExeLoaderBox->setDisabled(true);
        m_ui->emulationHighSpeedExeLoaderBox->setChecked(false);
    } else {
        m_ui->writeACKDelayEdit->setDisabled(true);
        m_ui->serialPortHandshakeCombo->setEnabled(true);
        m_ui->serialPortHandshakeCombo->setEnabled(true);
        m_ui->serialPortHandshakeLabel->setEnabled(true);
        m_ui->serialPortBaudCombo->setEnabled(true);
        m_ui->serialPortBaudLabel->setEnabled(true);
        m_ui->serialPortDivisorLabel->setEnabled(true);
        m_ui->serialPortDivisorEdit->setEnabled(true);
        m_ui->serialPortUseDivisorsBox->setEnabled(true);
        m_ui->emulationHighSpeedExeLoaderBox->setEnabled(true);
    }
}
