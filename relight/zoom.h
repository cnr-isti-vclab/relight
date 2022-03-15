#ifndef ZOOM_H
#define ZOOM_H

#include <ui_zoomdialog.h>
#include <QDialog>


class Zoom : public QDialog
{
    Q_OBJECT
public:
    explicit Zoom(QWidget* parent = nullptr);
    ~Zoom(){};

    void setTabIndex(int index);

private:
    Ui::ZoomDialog* m_Ui;
};

#endif // ZOOM_H
