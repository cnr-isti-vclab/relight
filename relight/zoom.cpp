#include <vips/vips.h>
#include <QFileDialog>
#include <QDebug>
#include <ui_zoomdialog.h>
#include <QIntValidator>
#include "processqueue.h"
#include "zoom.h"

/******************************************************************************************************************************/
/***********************************************ZOOM: TAKES CARE OF THE ZOOM DIALOG********************************************/
/******************************************************************************************************************************/

Zoom::Zoom(QWidget* parent) : QDialog(parent), m_Ui(new Ui::ZoomDialog), m_OutputFolder(""), m_InputFolder(""),
    m_ZoomType(ZoomType::None), m_ZoomString("None"), m_JpegQuality(95), m_Overlap(0), m_TileSize(256)
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

void Zoom::doZoom()
{
    ProcessQueue& queue = ProcessQueue::instance();
    ZoomTask* toSubmit = new ZoomTask(this, m_ZoomType);

    toSubmit->addParameter("output", Parameter::Type::STRING, m_OutputFolder);
    toSubmit->addParameter("input", Parameter::Type::STRING, m_InputFolder);
    toSubmit->addParameter("quality", Parameter::Type::INT, m_JpegQuality);
    toSubmit->addParameter("overlap", Parameter::Type::INT, m_Overlap);
    toSubmit->addParameter("tilesize", Parameter::Type::INT, m_TileSize);

    queue.addTask(toSubmit);
    queue.start();
}


void Zoom::buttonBrowseOutputClicked()
{
    QString output = QFileDialog::getExistingDirectory(this, QString("Select an output folder for %1").arg(m_ZoomString));
    if(output.isNull()) return;

    m_OutputFolder = output;
    m_Ui->outputOutputFolderDz->setText(m_InputFolder);
    m_Ui->outputOutputFolderTz->setText(m_InputFolder);
}

void Zoom::buttonBrowseInputClicked()
{
    QString input = QFileDialog::getExistingDirectory(this, QString("Select an input folder for %1").arg(m_ZoomString));
    if(input.isNull()) return;

    m_InputFolder = input;
    m_Ui->outputRtiFolderDz->setText(m_InputFolder);
    m_Ui->outputRtiFolderTz->setText(m_InputFolder);
}

void Zoom::inputQualityChanged(const QString& newValue)
{
    m_JpegQuality = newValue.toUInt();
}

void Zoom::inputTileSizeChanged(const QString& newValue)
{
    m_TileSize = newValue.toUInt();
}

void Zoom::inputOverlapChanged(const QString& newValue)
{
    m_Overlap = newValue.toUInt();
}

void Zoom::buttonConfirmClicked()
{
    doZoom();
    close();
}

void Zoom::setTabIndex(int index)
{
    m_Ui->zoomTab->setCurrentIndex(index);
    m_ZoomType = (ZoomType)index;

    updateZoomString();
}


void Zoom::updateZoomString()
{
    switch (m_ZoomType)
    {
    case ZoomType::DeepZoom:
        m_ZoomString = "DeepZoom";
        break;
    case ZoomType::Tarzoom:
        m_ZoomString = "Tarzoom";
        break;
    case ZoomType::ITarzoom:
        break;
    default:
        qDebug() << "Unsupported zoom type";
        break;
    }
}

/******************************************************************************************************************************/
/***********************************************ZOOMTASK: IMPLEMENTS ZOOM FUNCTIONS********************************************/
/******************************************************************************************************************************/

void ZoomTask::run()
{
    // Parameter retrieval
    QString outFolder = hasParameter("output") ? (*this)["output"].value.toString() : "";
    QString inFolder = hasParameter("input") ? (*this)["input"].value.toString() : "";
    int quality = hasParameter("quality") ? (*this)["quality"].value.toInt() : -1;
    int overlap = hasParameter("overlap") ? (*this)["overlap"].value.toInt() : -1;
    int tilesize = hasParameter("tilesize") ? (*this)["tilesize"].value.toInt() : -1;

    // General error checking
    if (outFolder.compare("")) {
        error = "Unspecified output folder";
        status = FAILED;
    }else if (inFolder.compare("")) {
        error = "Unspecified input folder";
        status = FAILED;
    }

    switch (m_ZoomType)
    {
    case Zoom::ZoomType::DeepZoom:
        // Deepzoom error checking
        if (quality == -1) {
            error = "Unspecified jpeg quality for DeepZoom";
            status = FAILED;
        }else if (overlap == -1) {
            error = "Unspecified overlap for DeepZoom";
            status = FAILED;
        } else if (tilesize == -1) {
            error = "Unspecified tile size for DeepZoom";
            status = FAILED;
        } else {
            // Launching deep zoom
            deepZoom(inFolder, outFolder, quality, overlap, tilesize);
        }
        break;
    case Zoom::ZoomType::Tarzoom:
        // Launghing tar zoom
        tarZoom(inFolder, outFolder);
        break;
    case Zoom::ZoomType::ITarzoom:
        // Launghing itar zoom
        tarZoom(inFolder, outFolder);
        break;
    case Zoom::ZoomType::None:
        break;
    }
}

void ZoomTask::deepZoom(QString inputFolder, QString output, uint32_t quality, uint32_t overlap, uint32_t tileSize)
{
    int nplanes = getNPlanes(output);

    // Deep zoom every plane
    for(int plane = 0; plane < nplanes; plane++)
    {
        // Load image, setup output folder for this plane
        QString fileName = (QStringList() << QString("%1/plane_%2").arg(output).arg(plane) << QString(".jpg")).join("");
        VipsImage* image = vips_image_new_from_file(fileName.toStdString().c_str(), NULL);
        if (image == NULL)
            LIB_VIPS_ERR
        QString folderName = QString("%1\\plane_%2").arg(output).arg(plane).toStdString().c_str();

        // Call dzsave and create the deepzoom tiles
        if (image == NULL || vips_dzsave(image, folderName.toStdString().c_str(),
            "overlap", overlap,
            "tile_size", tileSize,
            "layout", VIPS_FOREIGN_DZ_LAYOUT_DZ,
            "depth", VIPS_FOREIGN_DZ_DEPTH_ONETILE,
            "suffix", QString(".jpg[Q=%1]").arg(quality).toStdString().c_str(),
            NULL) != 0)
        {
            LIB_VIPS_ERR
        }

        // Update progress bar
        if(!progressed("Deepzoom:", 100*(plane+1)/nplanes))
            break;
    }
}

bool ZoomTask::progressed(std::string s, int percent)
{
    QString str(s.c_str());
    emit progress(str, percent);
    if(status == STOPPED)
        return false;
    return true;
}

void ZoomTask::tarZoom(QString inputFolder, QString outputFolder)
{

}

void ZoomTask::itarZoom(QString inputFolder, QString outputFolder)
{

}

int ZoomTask::getNPlanes(QString& output) {
    QDir destination(output);
    return destination.entryList(QStringList("plane_*.jpg"), QDir::Files).size();
}
