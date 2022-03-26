#ifndef ZOOMDIALOG_H
#define ZOOMDIALOG_H

#include <ui_zoomdialog.h>
#include "zoomtask.h"
#include <QDialog>

class ZoomDialog : public QDialog
{
    Q_OBJECT
public:

    explicit ZoomDialog(QWidget* parent = nullptr);
    ~ZoomDialog(){delete m_Ui;};

    void doZoom();

    void setTabIndex(int index);
    void setZoomType(ZoomTask::ZoomType type) {m_ZoomType = type;};

private slots:
    void buttonBrowseInputClicked();
    void buttonConfirmClicked();

private:
    void updateZoomString();

private:
    Ui::ZoomDialog* m_Ui;

    QString m_OutputFolder = "";
    QString m_InputFolder = "";

    ZoomTask::ZoomType m_ZoomType;
    QString m_ZoomString;

    int m_JpegQuality;
    int m_Overlap;
    int m_TileSize;
};

#endif // ZOOMDIALOG_H
