/********************************************************************************
** Form generated from reading UI file 'bootoptionsdialog.ui'
**
** Created by: Qt User Interface Compiler version 4.8.6
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_BOOTOPTIONSDIALOG_H
#define UI_BOOTOPTIONSDIALOG_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QGridLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QRadioButton>
#include <QtGui/QSpacerItem>

QT_BEGIN_NAMESPACE

class Ui_BootOptionsDialog
{
public:
    QDialogButtonBox *buttonBox;
    QGroupBox *groupBox;
    QGridLayout *gridLayout;
    QLabel *label;
    QRadioButton *atariDOS;
    QRadioButton *myDOS;
    QRadioButton *dosXL;
    QRadioButton *smartDOS;
    QRadioButton *spartaDOS;
    QRadioButton *myPicoDOS;
    QSpacerItem *horizontalSpacer_2;
    QCheckBox *disablePicoHiSpeed;
    QSpacerItem *horizontalSpacer;
    QLabel *label_2;

    void setupUi(QDialog *BootOptionsDialog)
    {
        if (BootOptionsDialog->objectName().isEmpty())
            BootOptionsDialog->setObjectName(QString::fromUtf8("BootOptionsDialog"));
        BootOptionsDialog->resize(319, 294);
        BootOptionsDialog->setMinimumSize(QSize(319, 294));
        BootOptionsDialog->setMaximumSize(QSize(319, 294));
        buttonBox = new QDialogButtonBox(BootOptionsDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setGeometry(QRect(20, 250, 281, 32));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Apply|QDialogButtonBox::Close);
        groupBox = new QGroupBox(BootOptionsDialog);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        groupBox->setGeometry(QRect(10, 10, 291, 232));
        QFont font;
        font.setBold(true);
        font.setWeight(75);
        groupBox->setFont(font);
        gridLayout = new QGridLayout(groupBox);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        label = new QLabel(groupBox);
        label->setObjectName(QString::fromUtf8("label"));
        label->setFont(font);
        label->setStyleSheet(QString::fromUtf8("color: rgb(111, 111, 111);"));

        gridLayout->addWidget(label, 0, 0, 1, 3);

        atariDOS = new QRadioButton(groupBox);
        atariDOS->setObjectName(QString::fromUtf8("atariDOS"));
        atariDOS->setFont(font);
        atariDOS->setStyleSheet(QString::fromUtf8("color: rgb(140, 96, 115);"));
        atariDOS->setChecked(true);

        gridLayout->addWidget(atariDOS, 1, 0, 1, 3);

        myDOS = new QRadioButton(groupBox);
        myDOS->setObjectName(QString::fromUtf8("myDOS"));
        myDOS->setFont(font);
        myDOS->setStyleSheet(QString::fromUtf8("color: rgb(140, 96, 115);"));

        gridLayout->addWidget(myDOS, 2, 0, 1, 3);

        dosXL = new QRadioButton(groupBox);
        dosXL->setObjectName(QString::fromUtf8("dosXL"));
        dosXL->setStyleSheet(QString::fromUtf8("color: rgb(140, 96, 115);"));

        gridLayout->addWidget(dosXL, 3, 0, 1, 3);

        smartDOS = new QRadioButton(groupBox);
        smartDOS->setObjectName(QString::fromUtf8("smartDOS"));
        smartDOS->setStyleSheet(QString::fromUtf8("color: rgb(140, 96, 115);"));

        gridLayout->addWidget(smartDOS, 4, 0, 1, 3);

        spartaDOS = new QRadioButton(groupBox);
        spartaDOS->setObjectName(QString::fromUtf8("spartaDOS"));
        spartaDOS->setFont(font);
        spartaDOS->setStyleSheet(QString::fromUtf8("color: rgb(140, 96, 115);"));

        gridLayout->addWidget(spartaDOS, 5, 0, 1, 3);

        myPicoDOS = new QRadioButton(groupBox);
        myPicoDOS->setObjectName(QString::fromUtf8("myPicoDOS"));
        myPicoDOS->setFont(font);
        myPicoDOS->setStyleSheet(QString::fromUtf8("color: rgb(140, 96, 115);"));

        gridLayout->addWidget(myPicoDOS, 6, 0, 1, 3);

        horizontalSpacer_2 = new QSpacerItem(30, 38, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer_2, 7, 0, 2, 1);

        disablePicoHiSpeed = new QCheckBox(groupBox);
        disablePicoHiSpeed->setObjectName(QString::fromUtf8("disablePicoHiSpeed"));
        disablePicoHiSpeed->setEnabled(false);
        QFont font1;
        font1.setBold(false);
        font1.setWeight(50);
        disablePicoHiSpeed->setFont(font1);

        gridLayout->addWidget(disablePicoHiSpeed, 7, 1, 1, 1);

        horizontalSpacer = new QSpacerItem(88, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer, 7, 2, 1, 1);

        label_2 = new QLabel(groupBox);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        QFont font2;
        font2.setBold(false);
        font2.setItalic(true);
        font2.setWeight(50);
        label_2->setFont(font2);

        gridLayout->addWidget(label_2, 8, 1, 1, 2);


        retranslateUi(BootOptionsDialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), BootOptionsDialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), BootOptionsDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(BootOptionsDialog);
    } // setupUi

    void retranslateUi(QDialog *BootOptionsDialog)
    {
        BootOptionsDialog->setWindowTitle(QApplication::translate("BootOptionsDialog", "Folder Boot Options", 0, QApplication::UnicodeUTF8));
        groupBox->setTitle(QApplication::translate("BootOptionsDialog", "Folder Image Boot Options", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("BootOptionsDialog", "Select the DOS you want to boot your Atari with", 0, QApplication::UnicodeUTF8));
        atariDOS->setText(QApplication::translate("BootOptionsDialog", "AtariDOS 2.x", 0, QApplication::UnicodeUTF8));
        myDOS->setText(QApplication::translate("BootOptionsDialog", "MyDOS 4.x", 0, QApplication::UnicodeUTF8));
        dosXL->setText(QApplication::translate("BootOptionsDialog", "DosXL 2.x", 0, QApplication::UnicodeUTF8));
        smartDOS->setText(QApplication::translate("BootOptionsDialog", "SmartDOS 6.1D", 0, QApplication::UnicodeUTF8));
        spartaDOS->setText(QApplication::translate("BootOptionsDialog", "SpartaDOS 3.2F", 0, QApplication::UnicodeUTF8));
        myPicoDOS->setText(QApplication::translate("BootOptionsDialog", "MyPicoDOS 4.05 (Standard)", 0, QApplication::UnicodeUTF8));
        disablePicoHiSpeed->setText(QApplication::translate("BootOptionsDialog", "Disable high speed SIO", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("BootOptionsDialog", "(Check if you're already using a high-speed OS)", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class BootOptionsDialog: public Ui_BootOptionsDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_BOOTOPTIONSDIALOG_H
