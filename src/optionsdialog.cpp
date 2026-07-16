#include "optionsdialog.h"
#include "ui_optionsdialog.h"
#include "aspeqtsettings.h"

#include <QTranslator>
#include <QDir>
#include <QScreen>
#include <QSize>

#ifdef Q_OS_ANDROID
#include <QScroller>
#include <QScrollerProperties>
#include <QScrollBar>
#include <QButtonGroup>
#include <QRadioButton>
#include <QVBoxLayout>
#include <QTimer>
#include <QKeyEvent>
#include <QJniObject>
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
    // Touch-friendly kinetic scrolling without the elastic overshoot that
    // leaves ghost widgets behind, plus immersive fullscreen.
    QWidget *vp = m_ui->scrollArea->viewport();
    QScroller::grabGesture(vp, QScroller::LeftMouseButtonGesture);
    QScroller *scroller = QScroller::scroller(vp);
    QScrollerProperties sp = scroller->scrollerProperties();
    sp.setScrollMetric(QScrollerProperties::VerticalOvershootPolicy,
                       QVariant::fromValue(QScrollerProperties::OvershootAlwaysOff));
    sp.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy,
                       QVariant::fromValue(QScrollerProperties::OvershootAlwaysOff));
    scroller->setScrollerProperties(sp);
    setWindowState(Qt::WindowFullScreen);
    androidInsetMargins();
    // Bigger checkbox hit targets and a more legible Save button for touch.
    setStyleSheet(styleSheet() +
                  "\nQCheckBox::indicator { width: 23px; height: 23px; }"
                  "\nQRadioButton::indicator { width: 12px; height: 12px; }");
    {
        QFont bf = m_ui->pushButton->font();
        bf.setPointSize(bf.pointSize() + 4);
        bf.setBold(true);
        m_ui->pushButton->setFont(bf);
        m_ui->pushButton->setMinimumHeight(52);
    }
    // Repaint on scroll: the bitblt scroll leaves ghost pixels on Android.
    connect(m_ui->scrollArea->verticalScrollBar(), &QScrollBar::valueChanged,
            this, [this]{ repaint(); });

    // The combo popup does not receive touch on Android, so every combo is
    // presented as a radio-button group instead.
    ifaceGroup = new QButtonGroup(this);
    ifaceGroup->addButton(m_ui->rbSIO2PC, 0);   // SIO2PC
    ifaceGroup->addButton(m_ui->rbSIO2BT, SIO2BT);
    handshakeGroup = new QButtonGroup(this);
    handshakeGroup->addButton(m_ui->rbHsRI, 0);
    handshakeGroup->addButton(m_ui->rbHsDSR, 1);
    handshakeGroup->addButton(m_ui->rbHsCTS, 2);
    handshakeGroup->addButton(m_ui->rbHsSOFT, 3);
    baudGroup = new QButtonGroup(this);
    baudGroup->addButton(m_ui->rbBaud0, 0);
    baudGroup->addButton(m_ui->rbBaud1, 1);
    baudGroup->addButton(m_ui->rbBaud2, 2);
    langGroup = new QButtonGroup(this);
#else
    m_ui->treeWidget->expandAll();
    itemStandard = m_ui->treeWidget->topLevelItem(0)->child(0);
    itemAtariSio = m_ui->treeWidget->topLevelItem(0)->child(1);
    itemEmulation = m_ui->treeWidget->topLevelItem(1);
    itemI18n = m_ui->treeWidget->topLevelItem(2);
#endif
    connect(this, SIGNAL(accepted()), this, SLOT(OptionsDialog_accepted()));
    /* Retrieve application settings */
    #ifdef Q_OS_ANDROID
    if (QAbstractButton *b = ifaceGroup->button(aspeqtSettings->serialPortInterface()))
        b->setChecked(true);
    if (QAbstractButton *b = handshakeGroup->button(aspeqtSettings->serialPortHandshakingMethod()))
        b->setChecked(true);
    if (QAbstractButton *b = baudGroup->button(aspeqtSettings->serialPortMaximumSpeed()))
        b->setChecked(true);
    #else
    m_ui->serialPortDeviceNameEdit->setText(aspeqtSettings->serialPortName());
    m_ui->serialPortHandshakeCombo->setCurrentIndex(aspeqtSettings->serialPortHandshakingMethod());
    m_ui->serialPortBaudCombo->setCurrentIndex(aspeqtSettings->serialPortMaximumSpeed());
    #endif
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
    #ifdef Q_OS_ANDROID
    m_ui->writeACKDelayEdit->setValue(aspeqtSettings->writeACKDelay());
    m_ui->bluetoothNameEdit->setText(aspeqtSettings->bluetoothName());
    // Apply the enable/disable state for the current interface, then keep it
    // in sync when the user picks another one.
    on_serialPortInterfaceCombo_currentIndexChanged(ifaceGroup->checkedId());
    connect(ifaceGroup, &QButtonGroup::idClicked,
            this, &OptionsDialog::on_serialPortInterfaceCombo_currentIndexChanged);
    #else
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
    #ifdef Q_OS_ANDROID
    addAndroidLanguage(tr("Automatic"), "auto");
    addAndroidLanguage(QT_TR_NOOP("English"), "en");
    #else
    m_ui->i18nLanguageCombo->clear();
    m_ui->i18nLanguageCombo->addItem(tr("Automatic"), "auto");
    if (aspeqtSettings->i18nLanguage().compare("auto") == 0)
      m_ui->i18nLanguageCombo->setCurrentIndex(0);
    m_ui->i18nLanguageCombo->addItem(QT_TR_NOOP("English"), "en");
    if (aspeqtSettings->i18nLanguage().compare("en") == 0)
      m_ui->i18nLanguageCombo->setCurrentIndex(1);
    #endif
    QDir dir(":/translations/i18n/");
    QStringList filters;
    filters << "aspeqt_*.qm";
    dir.setNameFilters(filters);
    for (int i = 0; i < dir.entryList().size(); ++i) {
        local_translator.load(":/translations/i18n/" + dir.entryList()[i]);
    QString langText = local_translator.translate("OptionsDialog", "English");
    QString langCode = dir.entryList()[i].mid(7).replace(".qm", "");
    #ifdef Q_OS_ANDROID
    addAndroidLanguage(langText, langCode);
    #else
    m_ui->i18nLanguageCombo->addItem(langText, langCode);
	if (langCode.compare(aspeqtSettings->i18nLanguage()) == 0) {
        m_ui->i18nLanguageCombo->setCurrentIndex(i+2);
	}
    #endif
    }
    #ifdef Q_OS_ANDROID
    // Default to "Automatic" if nothing matched the stored language.
    if (!langGroup->checkedButton() && langGroup->button(0))
        langGroup->button(0)->setChecked(true);
    #endif
}

OptionsDialog::~OptionsDialog()
{
    delete m_ui;
}

#ifdef Q_OS_ANDROID
void OptionsDialog::androidInsetMargins()
{
    long packed = QJniObject::callStaticMethod<jlong>("net/greblus/SerialActivity", "systemBarInsets");
    qreal dpr = devicePixelRatioF();
    if (dpr < 1.0) dpr = 1.0;
    int mt = qRound(((int)((packed >> 32) & 0xffff)) / dpr);
    int mb = qRound(((int)(packed & 0xffff)) / dpr);
    if (mt <= 0) mt = 96;   // fallback if insets not ready yet
    layout()->setContentsMargins(6, mt + 6, 6, mb + 6);
    if (packed == 0)
        QTimer::singleShot(200, this, [this]{ androidInsetMargins(); });
}

void OptionsDialog::addAndroidLanguage(const QString &text, const QString &code)
{
    QRadioButton *rb = new QRadioButton(text, m_ui->languageRadios);
    rb->setMinimumHeight(44);
    rb->setProperty("langCode", code);
    langGroup->addButton(rb, langGroup->buttons().count());
    m_ui->languageLayout->addWidget(rb);
    if (aspeqtSettings->i18nLanguage().compare(code) == 0)
        rb->setChecked(true);
}
#endif

#ifdef Q_OS_ANDROID
bool OptionsDialog::event(QEvent *e)
{
    // The Android back button/gesture must close only this dialog (cancel),
    // not the whole application. Catch it here before it propagates to the
    // main window / activity.
    if (e->type() == QEvent::KeyPress || e->type() == QEvent::KeyRelease) {
        int k = static_cast<QKeyEvent *>(e)->key();
        if (k == Qt::Key_Back || k == Qt::Key_Escape) {
            if (e->type() == QEvent::KeyRelease)
                reject();
            return true;
        }
    } else if (e->type() == QEvent::Close) {
        reject();
        return true;
    }
    return QDialog::event(e);
}
#endif

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
    aspeqtSettings->setSerialPortName(ifaceGroup->checkedId() == SIO2BT ? "SIO2BT" : "SIO2PC");
    aspeqtSettings->setSerialPortInterface(ifaceGroup->checkedId());
    aspeqtSettings->setWriteACKDelay(m_ui->writeACKDelayEdit->value());
    aspeqtSettings->setBluetoothName(m_ui->bluetoothNameEdit->text());
    #endif
    #ifdef Q_OS_ANDROID
    aspeqtSettings->setSerialPortHandshakingMethod(handshakeGroup->checkedId());
    aspeqtSettings->setSerialPortMaximumSpeed(baudGroup->checkedId());
    #else
    aspeqtSettings->setSerialPortHandshakingMethod(m_ui->serialPortHandshakeCombo->currentIndex());
    aspeqtSettings->setSerialPortMaximumSpeed(m_ui->serialPortBaudCombo->currentIndex());
    #endif
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
    #ifdef Q_OS_ANDROID
    int serial_int = aspeqtSettings->serialPortInterface();
    QJniObject::callStaticMethod<void>("net/greblus/SerialActivity", "changeDevice", "(I)V", serial_int);
    QJniObject b_name = QJniObject::fromString(aspeqtSettings->bluetoothName());
    jstring bluetooth_name = b_name.object<jstring>();
    QJniObject::setStaticField("net/greblus/SerialActivity", "bluetoothName", bluetooth_name);
    #endif
    int backend = 0;
    #ifndef Q_OS_ANDROID
    if (itemAtariSio->checkState(0) == Qt::Checked) {
        backend = 1;
    }
    #endif

    aspeqtSettings->setBackend(backend);
    #ifdef Q_OS_ANDROID
    QAbstractButton *lang = langGroup->checkedButton();
    aspeqtSettings->setI18nLanguage(lang ? lang->property("langCode").toString() : QString("auto"));
    #else
    aspeqtSettings->setI18nLanguage(m_ui->i18nLanguageCombo->itemData(m_ui->i18nLanguageCombo->currentIndex()).toString());
    #endif
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
#ifdef Q_OS_ANDROID
    m_ui->baudRadios->setEnabled(!checked);
    m_ui->serialPortBaudLabel->setEnabled(!checked);
#else
//    m_ui->serialPortBaudLabel->setEnabled(!checked);
    m_ui->serialPortBaudCombo->setEnabled(!checked);
#endif
    m_ui->serialPortDivisorLabel->setEnabled(checked);
    m_ui->serialPortDivisorEdit->setEnabled(checked);
}

void OptionsDialog::on_emulationUseCustomCasBaudBox_toggled(bool checked)
{
    m_ui->emulationCustomCasBaudSpin->setEnabled(checked);
}

void OptionsDialog::on_serialPortInterfaceCombo_currentIndexChanged(int index)
{
    #ifdef Q_OS_ANDROID
    bool bt = (index == SIO2BT);
    m_ui->writeACKDelayEdit->setEnabled(bt);
    m_ui->writeACKDelayLabel->setEnabled(bt);
    m_ui->bluetoothNameLabel->setEnabled(bt);
    m_ui->bluetoothNameEdit->setEnabled(bt);
    if (bt) {
        if (QAbstractButton *b = handshakeGroup->button(3)) b->setChecked(true); // SOFT
        m_ui->serialPortUseDivisorsBox->setChecked(false);
        m_ui->emulationHighSpeedExeLoaderBox->setChecked(false);
    }
    bool useDiv = m_ui->serialPortUseDivisorsBox->isChecked();
    m_ui->handshakeRadios->setEnabled(!bt);
    m_ui->serialPortHandshakeLabel->setEnabled(!bt);
    m_ui->baudRadios->setEnabled(!bt && !useDiv);
    m_ui->serialPortBaudLabel->setEnabled(!bt && !useDiv);
    m_ui->serialPortUseDivisorsBox->setEnabled(!bt);
    m_ui->serialPortDivisorLabel->setEnabled(!bt && useDiv);
    m_ui->serialPortDivisorEdit->setEnabled(!bt && useDiv);
    m_ui->emulationHighSpeedExeLoaderBox->setEnabled(!bt);
    QString cbStyle = bt ? "QCheckBox:!enabled {color: grey;}" : "color: black";
    m_ui->serialPortUseDivisorsBox->setStyleSheet(cbStyle);
    m_ui->emulationHighSpeedExeLoaderBox->setStyleSheet(cbStyle);
    #else
    Q_UNUSED(index)
    #endif
}
