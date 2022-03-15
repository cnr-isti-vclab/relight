#include <ui_zoomdialog.h>
#include "zoom.h"

Zoom::Zoom(QWidget* parent) : QDialog(parent), m_Ui(new Ui::ZoomDialog)
{
    m_Ui->setupUi(this);

    // Deep zoom
    connect(m_Ui->buttonBrowseRtiDz, SIGNAL(clicked()), this, SLOT(buttonBrowseRtiClicked()));
    connect(m_Ui->buttonBrowseOutputDz, SIGNAL(clicked()), this, SLOT(buttonBrowseOutputClicked()));
    connect(m_Ui->inputJpegQuality, SIGNAL(textChanged(const QString&)), this, SLOT(inputQualityChanged(const QString&)));

    connect(m_Ui->buttonConfirm, SIGNAL(clicked()), this, SLOT(buttonConfirmClicked()));

    // Tarzoom
}

void Zoom::setTabIndex(int index)
{
    m_Ui->zoomTab->setCurrentIndex(index);
}
