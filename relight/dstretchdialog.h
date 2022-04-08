#ifndef DSTRETCHDIALOG_H
#define DSTRETCHDIALOG_H

#include <QDialog>
#include "ui_dstretchdialog.h"

class DStretchDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DStretchDialog(QWidget *parent = nullptr);
    ~DStretchDialog();

public slots:
    void buttonBrowseClicked();
    void buttonGenerateClicked();


private:
    Ui::dstretchdialog* m_Ui;

    QString m_InputFile = "";
    QString m_OutputFile = "";
};

#endif // DSTRETCHDIALOG_H
