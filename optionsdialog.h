#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QtWidgets/QDialog>
#include <QTreeWidget>
#include <QtDebug>

#include "serialport.h"

namespace Ui {
    class OptionsDialog;
}
class QButtonGroup;

class OptionsDialog : public QDialog {
    Q_OBJECT

public:
    OptionsDialog(QWidget *parent = 0);
    ~OptionsDialog();

protected:
    void changeEvent(QEvent *e);
#ifdef Q_OS_ANDROID
    bool event(QEvent *e) override;
#endif

private:
    Ui::OptionsDialog *m_ui;
    QTreeWidgetItem *itemStandard, *itemAtariSio, *itemEmulation, *itemI18n;
#ifdef Q_OS_ANDROID
    QButtonGroup *ifaceGroup;
    QButtonGroup *handshakeGroup;
    QButtonGroup *baudGroup;
    QButtonGroup *langGroup;
    void androidInsetMargins();
    void addAndroidLanguage(const QString &text, const QString &code);
#endif

private slots:
    void on_serialPortUseDivisorsBox_toggled(bool checked);
    void on_treeWidget_itemClicked(QTreeWidgetItem* item, int column);
    void on_treeWidget_currentItemChanged(QTreeWidgetItem* current, QTreeWidgetItem* previous);
    void OptionsDialog_accepted();
    void on_emulationUseCustomCasBaudBox_toggled(bool checked);
    void on_serialPortInterfaceCombo_currentIndexChanged(int index);
};

#endif // OPTIONSDIALOG_H
