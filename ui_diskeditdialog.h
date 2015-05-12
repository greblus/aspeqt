/********************************************************************************
** Form generated from reading UI file 'diskeditdialog.ui'
**
** Created by: Qt User Interface Compiler version 4.8.6
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DISKEDITDIALOG_H
#define UI_DISKEDITDIALOG_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QGridLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QMainWindow>
#include <QtGui/QMenuBar>
#include <QtGui/QStatusBar>
#include <QtGui/QTableView>
#include <QtGui/QToolBar>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_DiskEditDialog
{
public:
    QAction *actionToParent;
    QAction *actionAddFiles;
    QAction *actionExtractFiles;
    QAction *actionTextConversion;
    QAction *actionDeleteSelectedFiles;
    QAction *actionPrint;
    QWidget *centralwidget;
    QGridLayout *gridLayout;
    QTableView *aView;
    QCheckBox *onTopBox;
    QMenuBar *menubar;
    QStatusBar *statusbar;
    QToolBar *toolBar;

    void setupUi(QMainWindow *DiskEditDialog)
    {
        if (DiskEditDialog->objectName().isEmpty())
            DiskEditDialog->setObjectName(QString::fromUtf8("DiskEditDialog"));
        DiskEditDialog->setWindowModality(Qt::NonModal);
        DiskEditDialog->resize(504, 424);
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(DiskEditDialog->sizePolicy().hasHeightForWidth());
        DiskEditDialog->setSizePolicy(sizePolicy);
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/icons/silk-icons/icons/folder_edit.png"), QSize(), QIcon::Normal, QIcon::Off);
        DiskEditDialog->setWindowIcon(icon);
        actionToParent = new QAction(DiskEditDialog);
        actionToParent->setObjectName(QString::fromUtf8("actionToParent"));
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/icons/silk-icons/icons/arrow_up.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionToParent->setIcon(icon1);
        actionAddFiles = new QAction(DiskEditDialog);
        actionAddFiles->setObjectName(QString::fromUtf8("actionAddFiles"));
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/icons/silk-icons/icons/add.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionAddFiles->setIcon(icon2);
        actionExtractFiles = new QAction(DiskEditDialog);
        actionExtractFiles->setObjectName(QString::fromUtf8("actionExtractFiles"));
        actionExtractFiles->setEnabled(false);
        QIcon icon3;
        icon3.addFile(QString::fromUtf8(":/icons/silk-icons/icons/page_white_go.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionExtractFiles->setIcon(icon3);
        actionTextConversion = new QAction(DiskEditDialog);
        actionTextConversion->setObjectName(QString::fromUtf8("actionTextConversion"));
        actionTextConversion->setCheckable(true);
        QIcon icon4;
        icon4.addFile(QString::fromUtf8(":/icons/silk-icons/icons/page_white_text.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionTextConversion->setIcon(icon4);
        actionDeleteSelectedFiles = new QAction(DiskEditDialog);
        actionDeleteSelectedFiles->setObjectName(QString::fromUtf8("actionDeleteSelectedFiles"));
        actionDeleteSelectedFiles->setEnabled(false);
        QIcon icon5;
        icon5.addFile(QString::fromUtf8(":/icons/silk-icons/icons/delete.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionDeleteSelectedFiles->setIcon(icon5);
        actionPrint = new QAction(DiskEditDialog);
        actionPrint->setObjectName(QString::fromUtf8("actionPrint"));
        QIcon icon6;
        icon6.addFile(QString::fromUtf8(":/icons/silk-icons/icons/printer.png"), QSize(), QIcon::Normal, QIcon::Off);
        actionPrint->setIcon(icon6);
        centralwidget = new QWidget(DiskEditDialog);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        gridLayout = new QGridLayout(centralwidget);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        aView = new QTableView(centralwidget);
        aView->setObjectName(QString::fromUtf8("aView"));
        aView->setContextMenuPolicy(Qt::ActionsContextMenu);
        aView->setAcceptDrops(true);
        aView->setEditTriggers(QAbstractItemView::EditKeyPressed);
        aView->setProperty("showDropIndicator", QVariant(false));
        aView->setDragEnabled(true);
        aView->setDragDropOverwriteMode(false);
        aView->setDragDropMode(QAbstractItemView::DragDrop);
        aView->setAlternatingRowColors(true);
        aView->setSelectionMode(QAbstractItemView::ExtendedSelection);
        aView->setSelectionBehavior(QAbstractItemView::SelectRows);
        aView->setShowGrid(false);
        aView->setSortingEnabled(true);
        aView->horizontalHeader()->setDefaultSectionSize(150);
        aView->horizontalHeader()->setHighlightSections(false);
        aView->horizontalHeader()->setProperty("showSortIndicator", QVariant(true));
        aView->horizontalHeader()->setStretchLastSection(true);
        aView->verticalHeader()->setVisible(false);
        aView->verticalHeader()->setDefaultSectionSize(17);

        gridLayout->addWidget(aView, 0, 0, 1, 1);

        onTopBox = new QCheckBox(centralwidget);
        onTopBox->setObjectName(QString::fromUtf8("onTopBox"));

        gridLayout->addWidget(onTopBox, 1, 0, 1, 1);

        DiskEditDialog->setCentralWidget(centralwidget);
        menubar = new QMenuBar(DiskEditDialog);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 504, 21));
        DiskEditDialog->setMenuBar(menubar);
        statusbar = new QStatusBar(DiskEditDialog);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        DiskEditDialog->setStatusBar(statusbar);
        toolBar = new QToolBar(DiskEditDialog);
        toolBar->setObjectName(QString::fromUtf8("toolBar"));
        toolBar->setMovable(false);
        toolBar->setAllowedAreas(Qt::NoToolBarArea);
        toolBar->setIconSize(QSize(16, 16));
        toolBar->setFloatable(true);
        DiskEditDialog->addToolBar(Qt::TopToolBarArea, toolBar);

        toolBar->addAction(actionToParent);
        toolBar->addSeparator();
        toolBar->addAction(actionAddFiles);
        toolBar->addAction(actionExtractFiles);
        toolBar->addSeparator();
        toolBar->addAction(actionDeleteSelectedFiles);
        toolBar->addSeparator();
        toolBar->addAction(actionTextConversion);
        toolBar->addAction(actionPrint);

        retranslateUi(DiskEditDialog);

        QMetaObject::connectSlotsByName(DiskEditDialog);
    } // setupUi

    void retranslateUi(QMainWindow *DiskEditDialog)
    {
        DiskEditDialog->setWindowTitle(QApplication::translate("DiskEditDialog", "MainWindow", 0, QApplication::UnicodeUTF8));
        actionToParent->setText(QApplication::translate("DiskEditDialog", "Go to the parent directory", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        actionToParent->setToolTip(QApplication::translate("DiskEditDialog", "Go to the parent directory", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_STATUSTIP
        actionToParent->setStatusTip(QApplication::translate("DiskEditDialog", "Go to the parent directory", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_STATUSTIP
        actionAddFiles->setText(QApplication::translate("DiskEditDialog", "Add files...", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        actionAddFiles->setToolTip(QApplication::translate("DiskEditDialog", "Add files to this directory", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_STATUSTIP
        actionAddFiles->setStatusTip(QApplication::translate("DiskEditDialog", "Add files to this directory", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_STATUSTIP
        actionExtractFiles->setText(QApplication::translate("DiskEditDialog", "Extract files...", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        actionExtractFiles->setToolTip(QApplication::translate("DiskEditDialog", "Extract selected files", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_STATUSTIP
        actionExtractFiles->setStatusTip(QApplication::translate("DiskEditDialog", "Extract selected files", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_STATUSTIP
        actionTextConversion->setText(QApplication::translate("DiskEditDialog", "Text conversion", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        actionTextConversion->setToolTip(QApplication::translate("DiskEditDialog", "Text conversion is off", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_STATUSTIP
        actionTextConversion->setStatusTip(QApplication::translate("DiskEditDialog", "Text conversion is off", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_STATUSTIP
        actionDeleteSelectedFiles->setText(QApplication::translate("DiskEditDialog", "Delete", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        actionDeleteSelectedFiles->setToolTip(QApplication::translate("DiskEditDialog", "Delete selected files", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_STATUSTIP
        actionDeleteSelectedFiles->setStatusTip(QApplication::translate("DiskEditDialog", "Delete selected files", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_STATUSTIP
        actionDeleteSelectedFiles->setShortcut(QApplication::translate("DiskEditDialog", "Del", 0, QApplication::UnicodeUTF8));
        actionPrint->setText(QApplication::translate("DiskEditDialog", "Print", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        actionPrint->setToolTip(QApplication::translate("DiskEditDialog", "Print Directory Listing", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        onTopBox->setText(QApplication::translate("DiskEditDialog", "Stay on Top", 0, QApplication::UnicodeUTF8));
        toolBar->setWindowTitle(QApplication::translate("DiskEditDialog", "toolBar", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class DiskEditDialog: public Ui_DiskEditDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DISKEDITDIALOG_H
