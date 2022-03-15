#include <ui_zoomdialog.h>
#include "zoom.h"

Zoom::Zoom(QWidget* parent) : QDialog(parent), m_Ui(new Ui::ZoomDialog)
{
    m_Ui->setupUi(this);
}

void Zoom::setTabIndex(int index)
{
    m_Ui->zoomTab->setCurrentIndex(index);
}
