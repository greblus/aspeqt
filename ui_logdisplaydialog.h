/********************************************************************************
** Form generated from reading UI file 'logdisplaydialog.ui'
**
** Created by: Qt User Interface Compiler version 4.8.6
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_LOGDISPLAYDIALOG_H
#define UI_LOGDISPLAYDIALOG_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QComboBox>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QGridLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QTextEdit>

QT_BEGIN_NAMESPACE

class Ui_LogDisplayDialog
{
public:
    QGridLayout *gridLayout;
    QTextEdit *textEdit;
    QGroupBox *groupBox;
    QHBoxLayout *horizontalLayout;
    QComboBox *listByDisk;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *LogDisplayDialog)
    {
        if (LogDisplayDialog->objectName().isEmpty())
            LogDisplayDialog->setObjectName(QString::fromUtf8("LogDisplayDialog"));
        LogDisplayDialog->resize(376, 461);
        LogDisplayDialog->setSizeGripEnabled(true);
        LogDisplayDialog->setModal(false);
        gridLayout = new QGridLayout(LogDisplayDialog);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        gridLayout->setContentsMargins(-1, -1, -1, 3);
        textEdit = new QTextEdit(LogDisplayDialog);
        textEdit->setObjectName(QString::fromUtf8("textEdit"));
        textEdit->setReadOnly(true);

        gridLayout->addWidget(textEdit, 0, 0, 1, 2);

        groupBox = new QGroupBox(LogDisplayDialog);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(groupBox->sizePolicy().hasHeightForWidth());
        groupBox->setSizePolicy(sizePolicy);
        horizontalLayout = new QHBoxLayout(groupBox);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        listByDisk = new QComboBox(groupBox);
        listByDisk->setObjectName(QString::fromUtf8("listByDisk"));
        listByDisk->setEditable(false);
        listByDisk->setModelColumn(0);

        horizontalLayout->addWidget(listByDisk);


        gridLayout->addWidget(groupBox, 1, 0, 1, 1);

        buttonBox = new QDialogButtonBox(LogDisplayDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        QSizePolicy sizePolicy1(QSizePolicy::Fixed, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(buttonBox->sizePolicy().hasHeightForWidth());
        buttonBox->setSizePolicy(sizePolicy1);
        buttonBox->setStandardButtons(QDialogButtonBox::Close);

        gridLayout->addWidget(buttonBox, 1, 1, 1, 1);


        retranslateUi(LogDisplayDialog);
        QObject::connect(buttonBox, SIGNAL(clicked(QAbstractButton*)), LogDisplayDialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(clicked(QAbstractButton*)), LogDisplayDialog, SLOT(reject()));
        QObject::connect(listByDisk, SIGNAL(currentIndexChanged(QString)), LogDisplayDialog, SLOT(repaint()));

        listByDisk->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(LogDisplayDialog);
    } // setupUi

    void retranslateUi(QDialog *LogDisplayDialog)
    {
        LogDisplayDialog->setWindowTitle(QApplication::translate("LogDisplayDialog", "AspeQt Log View", 0, QApplication::UnicodeUTF8));
        groupBox->setTitle(QApplication::translate("LogDisplayDialog", "Filter log by:", 0, QApplication::UnicodeUTF8));
        listByDisk->clear();
        listByDisk->insertItems(0, QStringList()
         << QApplication::translate("LogDisplayDialog", "ALL", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("LogDisplayDialog", "Disk 1", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("LogDisplayDialog", "Disk 2", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("LogDisplayDialog", "Disk 3", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("LogDisplayDialog", "Disk 4", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("LogDisplayDialog", "Disk 5", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("LogDisplayDialog", "Disk 6", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("LogDisplayDialog", "Disk 7", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("LogDisplayDialog", "Disk 8", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("LogDisplayDialog", "Disk 9", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("LogDisplayDialog", "Disk 10", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("LogDisplayDialog", "Disk 11", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("LogDisplayDialog", "Disk 12", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("LogDisplayDialog", "Disk 13", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("LogDisplayDialog", "Disk 14", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("LogDisplayDialog", "Disk 15", 0, QApplication::UnicodeUTF8)
        );
    } // retranslateUi

};

namespace Ui {
    class LogDisplayDialog: public Ui_LogDisplayDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_LOGDISPLAYDIALOG_H
