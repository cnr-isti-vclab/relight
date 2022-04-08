#include <QFileDialog>
#include <QDebug>
#include <ui_zoomdialog.h>
#include <QIntValidator>
#include <functional>
#include <QLineEdit>
#include "processqueue.h"
#include "zoomdialog.h"

ZoomDialog::ZoomDialog(QWidget* parent) : QDialog(parent), m_Ui(new Ui::ZoomDialog), m_OutputFolder(""), m_InputFolder(""),
    m_ZoomType(ZoomTask::ZoomType::None), m_ZoomString("None"), m_JpegQuality(95), m_Overlap(1), m_TileSize(254)
{
    m_Ui->setupUi(this);

    // Configure text fields to only accept numbers
    m_Ui->inputJpegQuality->setValidator(new QIntValidator(1, 100, this));
    m_Ui->inputOverlap->setValidator(new QIntValidator(0, std::numeric_limits<unsigned int>::max(), this));
    m_Ui->inputTileSize->setValidator(new QIntValidator(1, 8192, this));

    // Input file
    connect(m_Ui->buttonBrowseRtiDz, SIGNAL(clicked()), this, SLOT(buttonBrowseInputClicked()));
    connect(m_Ui->buttonBrowseRtiTz, SIGNAL(clicked()), this, SLOT(buttonBrowseInputClicked()));
    connect(m_Ui->buttonBrowseRtiIt, SIGNAL(clicked()), this, SLOT(buttonBrowseInputClicked()));

    // Deepzoom-specific
    connect(m_Ui->inputJpegQuality, &QLineEdit::textChanged, this, [=](const QString& newValue) {
        this->m_JpegQuality = newValue.toUInt();
    });
    connect(m_Ui->inputOverlap, &QLineEdit::textChanged, this, [=](const QString& newValue) {
        this->m_Overlap = newValue.toUInt();
    });
    connect(m_Ui->inputTileSize, &QLineEdit::textChanged, this, [=](const QString& newValue) {
        this->m_TileSize = newValue.toUInt();
    });

    // Delete files checkbox
    connect(m_Ui->checkboxTzDeleteInput, &QCheckBox::toggled, this, [=](bool newValue) {
        this->m_DeleteFiles = newValue;
    });
    connect(m_Ui->checkboxIzDeleteInput, &QCheckBox::toggled, this, [=](bool newValue) {
        this->m_DeleteFiles = newValue;
    });

    // Tab changing
    connect(m_Ui->zoomTab, &QTabWidget::currentChanged, this, [=](int newIdx) {
        this->m_ZoomType = (ZoomTask::ZoomType)newIdx;
    });
    // Confirm button
    connect(m_Ui->buttonConfirm, SIGNAL(clicked()), this, SLOT(buttonConfirmClicked()));
}

void ZoomDialog::doZoom()
{
    ProcessQueue& queue = ProcessQueue::instance();
    ZoomTask* task = new ZoomTask(this, m_ZoomType);

    task->input_folder = m_InputFolder;
    task->output = m_OutputFolder;
    task->addParameter("quality", Parameter::Type::INT, m_JpegQuality);
    task->addParameter("overlap", Parameter::Type::INT, m_Overlap);
    task->addParameter("tilesize", Parameter::Type::INT, m_TileSize);
    task->addParameter("deletefiles", Parameter::Type::BOOL, m_DeleteFiles);

    queue.addTask(task);
}

void ZoomDialog::buttonBrowseInputClicked()
{
    QString input = QFileDialog::getExistingDirectory(this, QString("Select an input folder for %1").arg(m_ZoomString));
    if(input.isNull()) return;

    m_InputFolder = input;
    m_Ui->outputRtiFolderDz->setText(m_InputFolder);
    m_Ui->outputRtiFolderTz->setText(m_InputFolder);
    m_Ui->outputRtiFolderIt->setText(m_InputFolder);
}

void ZoomDialog::buttonConfirmClicked()
{
    m_OutputFolder = QFileDialog::getExistingDirectory(this, QString("Select an output folder for %1").arg(m_ZoomString));
    doZoom();
    close();
}

void ZoomDialog::setTabIndex(int index)
{
    m_Ui->zoomTab->setCurrentIndex(index);
    m_ZoomType = (ZoomTask::ZoomType)index;

    updateZoomString();
}


void ZoomDialog::updateZoomString()
{
    switch (m_ZoomType)
    {
    case ZoomTask::ZoomType::DeepZoom:
        m_ZoomString = "DeepZoom";
        break;
    case ZoomTask::ZoomType::Tarzoom:
        m_ZoomString = "TarZoom";
        break;
    case ZoomTask::ZoomType::ITarzoom:
        m_ZoomString = "ItarZoom";
        break;
    default:
        qDebug() << "Unsupported zoom type";
        break;
    }
}
