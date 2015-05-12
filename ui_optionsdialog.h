/********************************************************************************
** Form generated from reading UI file 'optionsdialog.ui'
**
** Created by: Qt User Interface Compiler version 4.8.6
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_OPTIONSDIALOG_H
#define UI_OPTIONSDIALOG_H

#include <QtCore/QLocale>
#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QGridLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QSpacerItem>
#include <QtGui/QSpinBox>
#include <QtGui/QStackedWidget>
#include <QtGui/QTreeWidget>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_OptionsDialog
{
public:
    QTreeWidget *treeWidget;
    QStackedWidget *stackedWidget;
    QWidget *page_serialPort;
    QVBoxLayout *verticalLayout_2;
    QGroupBox *serialPortGroup;
    QVBoxLayout *verticalLayout_3;
    QCheckBox *serialPortBox;
    QLabel *serialPortDeviceNameLabel;
    QLineEdit *serialPortDeviceNameEdit;
    QLabel *serialPortHandshakeLabel;
    QComboBox *serialPortHandshakeCombo;
    QLabel *serialPortBaudLabel;
    QComboBox *serialPortBaudCombo;
    QCheckBox *serialPortUseDivisorsBox;
    QLabel *serialPortDivisorLabel;
    QSpinBox *serialPortDivisorEdit;
    QSpacerItem *serialPortSpacer1;
    QWidget *page_atariSio;
    QVBoxLayout *verticalLayout_4;
    QGroupBox *atariSioGroup;
    QVBoxLayout *verticalLayout_5;
    QCheckBox *atariSioBox;
    QLabel *atariSioDriverNameLabel;
    QLineEdit *atariSioDriverNameEdit;
    QLabel *atariSioHandshakingMethodLabel;
    QComboBox *atariSioHandshakingMethodCombo;
    QSpacerItem *atariSioSpacer1;
    QWidget *page_emulation;
    QVBoxLayout *verticalLayout_10;
    QGroupBox *emulationGroup;
    QGridLayout *gridLayout_3;
    QCheckBox *emulationHighSpeedExeLoaderBox;
    QSpacerItem *emulationSpacer1;
    QCheckBox *emulationUseCustomCasBaudBox;
    QSpinBox *emulationCustomCasBaudSpin;
    QSpacerItem *verticalSpacer_2;
    QLabel *label;
    QCheckBox *filterUscore;
    QLabel *label_2;
    QSpacerItem *emulationSpacer2;
    QWidget *page_i18n;
    QGridLayout *gridLayout;
    QGroupBox *uiGroup;
    QGridLayout *gridLayout_2;
    QLabel *i18nLanguageLabel;
    QComboBox *i18nLanguageCombo;
    QCheckBox *minimizeToTrayBox;
    QCheckBox *saveWinPosBox;
    QSpacerItem *i18nSpacer2;
    QSpacerItem *verticalSpacer;
    QCheckBox *saveDiskVisBox;
    QCheckBox *enableShade;
    QCheckBox *useLargerFont;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *OptionsDialog)
    {
        if (OptionsDialog->objectName().isEmpty())
            OptionsDialog->setObjectName(QString::fromUtf8("OptionsDialog"));
        OptionsDialog->setWindowModality(Qt::ApplicationModal);
        OptionsDialog->resize(638, 443);
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(OptionsDialog->sizePolicy().hasHeightForWidth());
        OptionsDialog->setSizePolicy(sizePolicy);
        OptionsDialog->setLocale(QLocale(QLocale::English, QLocale::UnitedStates));
        OptionsDialog->setModal(true);
        treeWidget = new QTreeWidget(OptionsDialog);
        QTreeWidgetItem *__qtreewidgetitem = new QTreeWidgetItem(treeWidget);
        __qtreewidgetitem->setFlags(Qt::ItemIsDragEnabled|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
        QTreeWidgetItem *__qtreewidgetitem1 = new QTreeWidgetItem(__qtreewidgetitem);
        __qtreewidgetitem1->setCheckState(0, Qt::Checked);
        QTreeWidgetItem *__qtreewidgetitem2 = new QTreeWidgetItem(__qtreewidgetitem);
        __qtreewidgetitem2->setCheckState(0, Qt::Unchecked);
        new QTreeWidgetItem(treeWidget);
        new QTreeWidgetItem(treeWidget);
        treeWidget->setObjectName(QString::fromUtf8("treeWidget"));
        treeWidget->setGeometry(QRect(9, 9, 256, 192));
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(treeWidget->sizePolicy().hasHeightForWidth());
        treeWidget->setSizePolicy(sizePolicy1);
        treeWidget->setHeaderHidden(true);
        stackedWidget = new QStackedWidget(OptionsDialog);
        stackedWidget->setObjectName(QString::fromUtf8("stackedWidget"));
        stackedWidget->setGeometry(QRect(306, 9, 320, 299));
        QSizePolicy sizePolicy2(QSizePolicy::Expanding, QSizePolicy::Preferred);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(stackedWidget->sizePolicy().hasHeightForWidth());
        stackedWidget->setSizePolicy(sizePolicy2);
        page_serialPort = new QWidget();
        page_serialPort->setObjectName(QString::fromUtf8("page_serialPort"));
        verticalLayout_2 = new QVBoxLayout(page_serialPort);
        verticalLayout_2->setSpacing(0);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        verticalLayout_2->setContentsMargins(0, 0, 0, 0);
        serialPortGroup = new QGroupBox(page_serialPort);
        serialPortGroup->setObjectName(QString::fromUtf8("serialPortGroup"));
        verticalLayout_3 = new QVBoxLayout(serialPortGroup);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        serialPortBox = new QCheckBox(serialPortGroup);
        serialPortBox->setObjectName(QString::fromUtf8("serialPortBox"));
        serialPortBox->setEnabled(false);
        serialPortBox->setCheckable(true);
        serialPortBox->setChecked(true);

        verticalLayout_3->addWidget(serialPortBox);

        serialPortDeviceNameLabel = new QLabel(serialPortGroup);
        serialPortDeviceNameLabel->setObjectName(QString::fromUtf8("serialPortDeviceNameLabel"));
        QSizePolicy sizePolicy3(QSizePolicy::Expanding, QSizePolicy::Fixed);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(serialPortDeviceNameLabel->sizePolicy().hasHeightForWidth());
        serialPortDeviceNameLabel->setSizePolicy(sizePolicy3);

        verticalLayout_3->addWidget(serialPortDeviceNameLabel);

        serialPortDeviceNameEdit = new QLineEdit(serialPortGroup);
        serialPortDeviceNameEdit->setObjectName(QString::fromUtf8("serialPortDeviceNameEdit"));

        verticalLayout_3->addWidget(serialPortDeviceNameEdit);

        serialPortHandshakeLabel = new QLabel(serialPortGroup);
        serialPortHandshakeLabel->setObjectName(QString::fromUtf8("serialPortHandshakeLabel"));
        QSizePolicy sizePolicy4(QSizePolicy::Preferred, QSizePolicy::Fixed);
        sizePolicy4.setHorizontalStretch(0);
        sizePolicy4.setVerticalStretch(0);
        sizePolicy4.setHeightForWidth(serialPortHandshakeLabel->sizePolicy().hasHeightForWidth());
        serialPortHandshakeLabel->setSizePolicy(sizePolicy4);

        verticalLayout_3->addWidget(serialPortHandshakeLabel);

        serialPortHandshakeCombo = new QComboBox(serialPortGroup);
        serialPortHandshakeCombo->setObjectName(QString::fromUtf8("serialPortHandshakeCombo"));

        verticalLayout_3->addWidget(serialPortHandshakeCombo);

        serialPortBaudLabel = new QLabel(serialPortGroup);
        serialPortBaudLabel->setObjectName(QString::fromUtf8("serialPortBaudLabel"));
        sizePolicy4.setHeightForWidth(serialPortBaudLabel->sizePolicy().hasHeightForWidth());
        serialPortBaudLabel->setSizePolicy(sizePolicy4);

        verticalLayout_3->addWidget(serialPortBaudLabel);

        serialPortBaudCombo = new QComboBox(serialPortGroup);
        serialPortBaudCombo->setObjectName(QString::fromUtf8("serialPortBaudCombo"));

        verticalLayout_3->addWidget(serialPortBaudCombo);

        serialPortUseDivisorsBox = new QCheckBox(serialPortGroup);
        serialPortUseDivisorsBox->setObjectName(QString::fromUtf8("serialPortUseDivisorsBox"));

        verticalLayout_3->addWidget(serialPortUseDivisorsBox);

        serialPortDivisorLabel = new QLabel(serialPortGroup);
        serialPortDivisorLabel->setObjectName(QString::fromUtf8("serialPortDivisorLabel"));
        serialPortDivisorLabel->setEnabled(false);
        sizePolicy4.setHeightForWidth(serialPortDivisorLabel->sizePolicy().hasHeightForWidth());
        serialPortDivisorLabel->setSizePolicy(sizePolicy4);

        verticalLayout_3->addWidget(serialPortDivisorLabel);

        serialPortDivisorEdit = new QSpinBox(serialPortGroup);
        serialPortDivisorEdit->setObjectName(QString::fromUtf8("serialPortDivisorEdit"));
        serialPortDivisorEdit->setEnabled(false);
        serialPortDivisorEdit->setMaximum(40);
        serialPortDivisorEdit->setValue(6);

        verticalLayout_3->addWidget(serialPortDivisorEdit);

        serialPortSpacer1 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_3->addItem(serialPortSpacer1);


        verticalLayout_2->addWidget(serialPortGroup);

        stackedWidget->addWidget(page_serialPort);
        page_atariSio = new QWidget();
        page_atariSio->setObjectName(QString::fromUtf8("page_atariSio"));
        verticalLayout_4 = new QVBoxLayout(page_atariSio);
        verticalLayout_4->setSpacing(0);
        verticalLayout_4->setObjectName(QString::fromUtf8("verticalLayout_4"));
        verticalLayout_4->setContentsMargins(0, 0, 0, 0);
        atariSioGroup = new QGroupBox(page_atariSio);
        atariSioGroup->setObjectName(QString::fromUtf8("atariSioGroup"));
        verticalLayout_5 = new QVBoxLayout(atariSioGroup);
        verticalLayout_5->setObjectName(QString::fromUtf8("verticalLayout_5"));
        atariSioBox = new QCheckBox(atariSioGroup);
        atariSioBox->setObjectName(QString::fromUtf8("atariSioBox"));
        atariSioBox->setEnabled(false);

        verticalLayout_5->addWidget(atariSioBox);

        atariSioDriverNameLabel = new QLabel(atariSioGroup);
        atariSioDriverNameLabel->setObjectName(QString::fromUtf8("atariSioDriverNameLabel"));
        sizePolicy3.setHeightForWidth(atariSioDriverNameLabel->sizePolicy().hasHeightForWidth());
        atariSioDriverNameLabel->setSizePolicy(sizePolicy3);

        verticalLayout_5->addWidget(atariSioDriverNameLabel);

        atariSioDriverNameEdit = new QLineEdit(atariSioGroup);
        atariSioDriverNameEdit->setObjectName(QString::fromUtf8("atariSioDriverNameEdit"));

        verticalLayout_5->addWidget(atariSioDriverNameEdit);

        atariSioHandshakingMethodLabel = new QLabel(atariSioGroup);
        atariSioHandshakingMethodLabel->setObjectName(QString::fromUtf8("atariSioHandshakingMethodLabel"));
        sizePolicy4.setHeightForWidth(atariSioHandshakingMethodLabel->sizePolicy().hasHeightForWidth());
        atariSioHandshakingMethodLabel->setSizePolicy(sizePolicy4);

        verticalLayout_5->addWidget(atariSioHandshakingMethodLabel);

        atariSioHandshakingMethodCombo = new QComboBox(atariSioGroup);
        atariSioHandshakingMethodCombo->setObjectName(QString::fromUtf8("atariSioHandshakingMethodCombo"));

        verticalLayout_5->addWidget(atariSioHandshakingMethodCombo);

        atariSioSpacer1 = new QSpacerItem(20, 148, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_5->addItem(atariSioSpacer1);


        verticalLayout_4->addWidget(atariSioGroup);

        stackedWidget->addWidget(page_atariSio);
        page_emulation = new QWidget();
        page_emulation->setObjectName(QString::fromUtf8("page_emulation"));
        verticalLayout_10 = new QVBoxLayout(page_emulation);
        verticalLayout_10->setSpacing(0);
        verticalLayout_10->setObjectName(QString::fromUtf8("verticalLayout_10"));
        verticalLayout_10->setContentsMargins(0, 0, 0, 0);
        emulationGroup = new QGroupBox(page_emulation);
        emulationGroup->setObjectName(QString::fromUtf8("emulationGroup"));
        sizePolicy1.setHeightForWidth(emulationGroup->sizePolicy().hasHeightForWidth());
        emulationGroup->setSizePolicy(sizePolicy1);
        emulationGroup->setMinimumSize(QSize(300, 0));
        gridLayout_3 = new QGridLayout(emulationGroup);
        gridLayout_3->setObjectName(QString::fromUtf8("gridLayout_3"));
        emulationHighSpeedExeLoaderBox = new QCheckBox(emulationGroup);
        emulationHighSpeedExeLoaderBox->setObjectName(QString::fromUtf8("emulationHighSpeedExeLoaderBox"));
        emulationHighSpeedExeLoaderBox->setMinimumSize(QSize(300, 0));
        emulationHighSpeedExeLoaderBox->setChecked(true);

        gridLayout_3->addWidget(emulationHighSpeedExeLoaderBox, 0, 0, 1, 2);

        emulationSpacer1 = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Fixed);

        gridLayout_3->addItem(emulationSpacer1, 1, 1, 1, 1);

        emulationUseCustomCasBaudBox = new QCheckBox(emulationGroup);
        emulationUseCustomCasBaudBox->setObjectName(QString::fromUtf8("emulationUseCustomCasBaudBox"));
        QSizePolicy sizePolicy5(QSizePolicy::Minimum, QSizePolicy::Fixed);
        sizePolicy5.setHorizontalStretch(0);
        sizePolicy5.setVerticalStretch(0);
        sizePolicy5.setHeightForWidth(emulationUseCustomCasBaudBox->sizePolicy().hasHeightForWidth());
        emulationUseCustomCasBaudBox->setSizePolicy(sizePolicy5);
        emulationUseCustomCasBaudBox->setMinimumSize(QSize(300, 0));

        gridLayout_3->addWidget(emulationUseCustomCasBaudBox, 2, 0, 1, 2);

        emulationCustomCasBaudSpin = new QSpinBox(emulationGroup);
        emulationCustomCasBaudSpin->setObjectName(QString::fromUtf8("emulationCustomCasBaudSpin"));
        emulationCustomCasBaudSpin->setEnabled(false);
        emulationCustomCasBaudSpin->setMinimumSize(QSize(230, 0));
        emulationCustomCasBaudSpin->setMinimum(425);
        emulationCustomCasBaudSpin->setMaximum(875);
        emulationCustomCasBaudSpin->setValue(600);

        gridLayout_3->addWidget(emulationCustomCasBaudSpin, 3, 0, 1, 2);

        verticalSpacer_2 = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Fixed);

        gridLayout_3->addItem(verticalSpacer_2, 4, 1, 1, 1);

        label = new QLabel(emulationGroup);
        label->setObjectName(QString::fromUtf8("label"));
        label->setMinimumSize(QSize(0, 0));
        QFont font;
        font.setItalic(false);
        label->setFont(font);

        gridLayout_3->addWidget(label, 5, 0, 1, 1);

        filterUscore = new QCheckBox(emulationGroup);
        filterUscore->setObjectName(QString::fromUtf8("filterUscore"));
        filterUscore->setMinimumSize(QSize(300, 0));
        filterUscore->setMaximumSize(QSize(16777215, 16777215));
        filterUscore->setChecked(true);

        gridLayout_3->addWidget(filterUscore, 6, 0, 1, 2);

        label_2 = new QLabel(emulationGroup);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setMinimumSize(QSize(300, 0));
        QFont font1;
        font1.setItalic(true);
        label_2->setFont(font1);

        gridLayout_3->addWidget(label_2, 7, 0, 1, 2);

        emulationSpacer2 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout_3->addItem(emulationSpacer2, 8, 1, 1, 1);


        verticalLayout_10->addWidget(emulationGroup);

        stackedWidget->addWidget(page_emulation);
        page_i18n = new QWidget();
        page_i18n->setObjectName(QString::fromUtf8("page_i18n"));
        gridLayout = new QGridLayout(page_i18n);
        gridLayout->setSpacing(0);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        gridLayout->setContentsMargins(0, 0, 0, 0);
        uiGroup = new QGroupBox(page_i18n);
        uiGroup->setObjectName(QString::fromUtf8("uiGroup"));
        uiGroup->setMinimumSize(QSize(258, 208));
        gridLayout_2 = new QGridLayout(uiGroup);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        i18nLanguageLabel = new QLabel(uiGroup);
        i18nLanguageLabel->setObjectName(QString::fromUtf8("i18nLanguageLabel"));

        gridLayout_2->addWidget(i18nLanguageLabel, 0, 0, 1, 1);

        i18nLanguageCombo = new QComboBox(uiGroup);
        i18nLanguageCombo->setObjectName(QString::fromUtf8("i18nLanguageCombo"));

        gridLayout_2->addWidget(i18nLanguageCombo, 1, 0, 1, 1);

        minimizeToTrayBox = new QCheckBox(uiGroup);
        minimizeToTrayBox->setObjectName(QString::fromUtf8("minimizeToTrayBox"));

        gridLayout_2->addWidget(minimizeToTrayBox, 4, 0, 1, 1);

        saveWinPosBox = new QCheckBox(uiGroup);
        saveWinPosBox->setObjectName(QString::fromUtf8("saveWinPosBox"));
        saveWinPosBox->setChecked(true);

        gridLayout_2->addWidget(saveWinPosBox, 5, 0, 1, 1);

        i18nSpacer2 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout_2->addItem(i18nSpacer2, 9, 0, 1, 1);

        verticalSpacer = new QSpacerItem(20, 10, QSizePolicy::Minimum, QSizePolicy::Fixed);

        gridLayout_2->addItem(verticalSpacer, 3, 0, 1, 1);

        saveDiskVisBox = new QCheckBox(uiGroup);
        saveDiskVisBox->setObjectName(QString::fromUtf8("saveDiskVisBox"));
        saveDiskVisBox->setChecked(true);

        gridLayout_2->addWidget(saveDiskVisBox, 6, 0, 1, 1);

        enableShade = new QCheckBox(uiGroup);
        enableShade->setObjectName(QString::fromUtf8("enableShade"));
        enableShade->setChecked(true);

        gridLayout_2->addWidget(enableShade, 7, 0, 1, 1);

        useLargerFont = new QCheckBox(uiGroup);
        useLargerFont->setObjectName(QString::fromUtf8("useLargerFont"));
        useLargerFont->setEnabled(true);
        useLargerFont->setChecked(false);

        gridLayout_2->addWidget(useLargerFont, 8, 0, 1, 1);


        gridLayout->addWidget(uiGroup, 0, 0, 1, 1);

        stackedWidget->addWidget(page_i18n);
        buttonBox = new QDialogButtonBox(OptionsDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setGeometry(QRect(9, 411, 156, 23));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Save);

        retranslateUi(OptionsDialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), OptionsDialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), OptionsDialog, SLOT(reject()));

        stackedWidget->setCurrentIndex(1);
        serialPortBaudCombo->setCurrentIndex(2);


        QMetaObject::connectSlotsByName(OptionsDialog);
    } // setupUi

    void retranslateUi(QDialog *OptionsDialog)
    {
        OptionsDialog->setWindowTitle(QApplication::translate("OptionsDialog", "Options", 0, QApplication::UnicodeUTF8));
        QTreeWidgetItem *___qtreewidgetitem = treeWidget->headerItem();
        ___qtreewidgetitem->setText(0, QApplication::translate("OptionsDialog", "1", 0, QApplication::UnicodeUTF8));

        const bool __sortingEnabled = treeWidget->isSortingEnabled();
        treeWidget->setSortingEnabled(false);
        QTreeWidgetItem *___qtreewidgetitem1 = treeWidget->topLevelItem(0);
        ___qtreewidgetitem1->setText(0, QApplication::translate("OptionsDialog", "Serial I/O backends", 0, QApplication::UnicodeUTF8));
        QTreeWidgetItem *___qtreewidgetitem2 = ___qtreewidgetitem1->child(0);
        ___qtreewidgetitem2->setText(0, QApplication::translate("OptionsDialog", "Standard serial port", 0, QApplication::UnicodeUTF8));
        QTreeWidgetItem *___qtreewidgetitem3 = ___qtreewidgetitem1->child(1);
        ___qtreewidgetitem3->setText(0, QApplication::translate("OptionsDialog", "AtariSIO", 0, QApplication::UnicodeUTF8));
        QTreeWidgetItem *___qtreewidgetitem4 = treeWidget->topLevelItem(1);
        ___qtreewidgetitem4->setText(0, QApplication::translate("OptionsDialog", "Emulation", 0, QApplication::UnicodeUTF8));
        QTreeWidgetItem *___qtreewidgetitem5 = treeWidget->topLevelItem(2);
        ___qtreewidgetitem5->setText(0, QApplication::translate("OptionsDialog", "User interface", 0, QApplication::UnicodeUTF8));
        treeWidget->setSortingEnabled(__sortingEnabled);

        serialPortGroup->setTitle(QApplication::translate("OptionsDialog", "Standard serial port backend options", 0, QApplication::UnicodeUTF8));
        serialPortBox->setText(QApplication::translate("OptionsDialog", "Use this backend", 0, QApplication::UnicodeUTF8));
        serialPortDeviceNameLabel->setText(QApplication::translate("OptionsDialog", "Port name:", 0, QApplication::UnicodeUTF8));
        serialPortDeviceNameEdit->setText(QApplication::translate("OptionsDialog", "COM1", 0, QApplication::UnicodeUTF8));
        serialPortHandshakeLabel->setText(QApplication::translate("OptionsDialog", "Handshake method:", 0, QApplication::UnicodeUTF8));
        serialPortHandshakeCombo->clear();
        serialPortHandshakeCombo->insertItems(0, QStringList()
         << QApplication::translate("OptionsDialog", "RI", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("OptionsDialog", "DSR", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("OptionsDialog", "CTS", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("OptionsDialog", "NONE (Windows ONLY - Experimental)", 0, QApplication::UnicodeUTF8)
        );
        serialPortBaudLabel->setText(QApplication::translate("OptionsDialog", "High speed mode baud rate:", 0, QApplication::UnicodeUTF8));
        serialPortBaudCombo->clear();
        serialPortBaudCombo->insertItems(0, QStringList()
         << QApplication::translate("OptionsDialog", "19200 (1x)", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("OptionsDialog", "38400 (2x)", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("OptionsDialog", "57600 (3x)", 0, QApplication::UnicodeUTF8)
        );
        serialPortUseDivisorsBox->setText(QApplication::translate("OptionsDialog", "Use non-standard speeds", 0, QApplication::UnicodeUTF8));
        serialPortDivisorLabel->setText(QApplication::translate("OptionsDialog", "High speed mode POKEY divisor:", 0, QApplication::UnicodeUTF8));
        atariSioGroup->setTitle(QApplication::translate("OptionsDialog", "AtariSIO backend options", 0, QApplication::UnicodeUTF8));
        atariSioBox->setText(QApplication::translate("OptionsDialog", "Use this backend", 0, QApplication::UnicodeUTF8));
        atariSioDriverNameLabel->setText(QApplication::translate("OptionsDialog", "Device name:", 0, QApplication::UnicodeUTF8));
        atariSioDriverNameEdit->setText(QApplication::translate("OptionsDialog", "/dev/atarisio0", 0, QApplication::UnicodeUTF8));
        atariSioHandshakingMethodLabel->setText(QApplication::translate("OptionsDialog", "Handshake method:", 0, QApplication::UnicodeUTF8));
        atariSioHandshakingMethodCombo->clear();
        atariSioHandshakingMethodCombo->insertItems(0, QStringList()
         << QApplication::translate("OptionsDialog", "RI", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("OptionsDialog", "DSR", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("OptionsDialog", "CTS", 0, QApplication::UnicodeUTF8)
        );
        emulationGroup->setTitle(QApplication::translate("OptionsDialog", "Emulation", 0, QApplication::UnicodeUTF8));
        emulationHighSpeedExeLoaderBox->setText(QApplication::translate("OptionsDialog", "Use high speed executable loader", 0, QApplication::UnicodeUTF8));
        emulationUseCustomCasBaudBox->setText(QApplication::translate("OptionsDialog", "Use custom baud rate for cassette emulation", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("OptionsDialog", " Folder Images:", 0, QApplication::UnicodeUTF8));
        filterUscore->setText(QApplication::translate("OptionsDialog", "Filter out underscore character from file names", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("OptionsDialog", "        (Required for AtariDOS compatibility)", 0, QApplication::UnicodeUTF8));
        uiGroup->setTitle(QApplication::translate("OptionsDialog", "User inteface", 0, QApplication::UnicodeUTF8));
        i18nLanguageLabel->setText(QApplication::translate("OptionsDialog", "Language:", 0, QApplication::UnicodeUTF8));
        minimizeToTrayBox->setText(QApplication::translate("OptionsDialog", "Minimize to system tray", 0, QApplication::UnicodeUTF8));
        saveWinPosBox->setText(QApplication::translate("OptionsDialog", "Save window positions and sizes", 0, QApplication::UnicodeUTF8));
        saveDiskVisBox->setText(QApplication::translate("OptionsDialog", "Save D9-DO drive visibility status", 0, QApplication::UnicodeUTF8));
        enableShade->setText(QApplication::translate("OptionsDialog", "Enable Shade in Mini Mode by default", 0, QApplication::UnicodeUTF8));
        useLargerFont->setText(QApplication::translate("OptionsDialog", "Use larger font in drive slot descriptions", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        buttonBox->setToolTip(QApplication::translate("OptionsDialog", "Save/Commit or Cancel/Ignore changes made to the settings", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_STATUSTIP
        buttonBox->setStatusTip(QString());
#endif // QT_NO_STATUSTIP
    } // retranslateUi

};

namespace Ui {
    class OptionsDialog: public Ui_OptionsDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_OPTIONSDIALOG_H
