#include <vips/vips.h>
#include <QDir>
#include "zoomtask.h"

void ZoomTask::run()
{
    // Parameter retrieval
    QString outFolder = hasParameter("output") ? (*this)["output"].value.toString() : "";
    QString inFolder = hasParameter("input") ? (*this)["input"].value.toString() : "";
    int quality = hasParameter("quality") ? (*this)["quality"].value.toInt() : -1;
    int overlap = hasParameter("overlap") ? (*this)["overlap"].value.toInt() : -1;
    int tilesize = hasParameter("tilesize") ? (*this)["tilesize"].value.toInt() : -1;

    // General error checking
    if (outFolder.compare("") == 0) {
        error = "Unspecified output folder";
        status = FAILED;
        return;
    }else if (inFolder.compare("") == 0) {
        error = "Unspecified input folder";
        status = FAILED;
        return;
    }

    switch (m_ZoomType)
    {
    case ZoomType::DeepZoom:
        // Deepzoom error checking
        if (quality == -1) {
            error = "Unspecified jpeg quality for DeepZoom";
            status = FAILED;
            return;
        }else if (overlap == -1) {
            error = "Unspecified overlap for DeepZoom";
            status = FAILED;
            return;
        } else if (tilesize == -1) {
            error = "Unspecified tile size for DeepZoom";
            status = FAILED;
            return;
        } else {
            // Launching deep zoom
            deepZoom(inFolder, outFolder, quality, overlap, tilesize);
        }
        break;
    case ZoomType::Tarzoom:
        // Launghing tar zoom
        tarZoom(inFolder, outFolder);
        break;
    case ZoomType::ITarzoom:
        // Launghing itar zoom
        tarZoom(inFolder, outFolder);
        break;
    case ZoomType::None:
        break;
    }

    status = DONE;
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
