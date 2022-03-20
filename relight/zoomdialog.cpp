#include <QFileDialog>
#include <QDebug>
#include <ui_zoomdialog.h>
#include <QIntValidator>
#include "processqueue.h"
#include "zoomdialog.h"

/** TODO
 * - Togli megacommenti
 * - Rebase su depython
 * - Verificare che ci siano i piani nella cartella di input
 * - Lambda per slot semplici
 * - Modal per quando viene mostrata l'interfaccia
 */

ZoomDialog::ZoomDialog(QWidget* parent) : QDialog(parent), m_Ui(new Ui::ZoomDialog), m_OutputFolder(""), m_InputFolder(""),
    m_ZoomType(ZoomTask::ZoomType::None), m_ZoomString("None"), m_JpegQuality(95), m_Overlap(0), m_TileSize(256)
{
    m_Ui->setupUi(this);

    // Configure text fields to only accept numbers
    m_Ui->inputJpegQuality->setValidator(new QIntValidator(1, 100, this));
    m_Ui->inputOverlap->setValidator(new QIntValidator(0, std::numeric_limits<unsigned int>::max(), this));
    m_Ui->inputTileSize->setValidator(new QIntValidator(1, 8192, this));

    // Deep zoom
    connect(m_Ui->buttonBrowseRtiDz, SIGNAL(clicked()), this, SLOT(buttonBrowseInputClicked()));
    connect(m_Ui->buttonBrowseOutputDz, SIGNAL(clicked()), this, SLOT(buttonBrowseOutputClicked()));
    connect(m_Ui->inputJpegQuality, SIGNAL(textChanged(const QString&)), this, SLOT(inputQualityChanged(const QString&)));

    // Tarzoom
    connect(m_Ui->buttonBrowseRtiTz, SIGNAL(clicked()), this, SLOT(buttonBrowseInputClicked()));
    connect(m_Ui->buttonBrowseOutputTz, SIGNAL(clicked()), this, SLOT(buttonBrowseOutputClicked()));

    // Common
    connect(m_Ui->buttonConfirm, SIGNAL(clicked()), this, SLOT(buttonConfirmClicked()));
}

void ZoomDialog::doZoom()
{
    ProcessQueue& queue = ProcessQueue::instance();
    ZoomTask* task = new ZoomTask(this, m_ZoomType);

    task->addParameter("output", Parameter::Type::STRING, m_OutputFolder);
    task->addParameter("input", Parameter::Type::STRING, m_InputFolder);
    task->addParameter("quality", Parameter::Type::INT, m_JpegQuality);
    task->addParameter("overlap", Parameter::Type::INT, m_Overlap);
    task->addParameter("tilesize", Parameter::Type::INT, m_TileSize);

    queue.addTask(task);
}


void ZoomDialog::buttonBrowseOutputClicked()
{
    QString output = QFileDialog::getExistingDirectory(this, QString("Select an output folder for %1").arg(m_ZoomString));
    if(output.isNull()) return;

    m_OutputFolder = output;
    m_Ui->outputOutputFolderDz->setText(m_OutputFolder);
    m_Ui->outputOutputFolderTz->setText(m_OutputFolder);
}

void ZoomDialog::buttonBrowseInputClicked()
{
    QString input = QFileDialog::getExistingDirectory(this, QString("Select an input folder for %1").arg(m_ZoomString));
    if(input.isNull()) return;

    m_InputFolder = input;
    m_Ui->outputRtiFolderDz->setText(m_InputFolder);
    m_Ui->outputRtiFolderTz->setText(m_InputFolder);

    // It's likely that the input folder is also the output folder, but don't automatically update it if the user
    // has explicitly set a different output folder
    if (m_OutputFolder == "")
    {
        m_OutputFolder = input;
        m_Ui->outputOutputFolderDz->setText(m_InputFolder);
        m_Ui->outputOutputFolderTz->setText(m_InputFolder);
    }
}

void ZoomDialog::inputQualityChanged(const QString& newValue)
{
    m_JpegQuality = newValue.toUInt();
}

void ZoomDialog::inputTileSizeChanged(const QString& newValue)
{
    m_TileSize = newValue.toUInt();
}

void ZoomDialog::inputOverlapChanged(const QString& newValue)
{
    m_Overlap = newValue.toUInt();
}

void ZoomDialog::buttonConfirmClicked()
{
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
        m_ZoomString = "Tarzoom";
        break;
    case ZoomTask::ZoomType::ITarzoom:
        break;
    default:
        qDebug() << "Unsupported zoom type";
        break;
    }
}
