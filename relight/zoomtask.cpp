#include <vips/vips.h>
#include <QDir>
#include "zoom.h"
#include "zoomtask.h"

void ZoomTask::run()
{
    // Parameter retrieval
    QString outFolder = hasParameter("output") ? (*this)["output"].value.toString() : "";
    QString inFolder = hasParameter("input") ? (*this)["input"].value.toString() : "";

    int quality = hasParameter("quality") ? (*this)["quality"].value.toInt() : -1;
    int overlap = hasParameter("overlap") ? (*this)["overlap"].value.toInt() : -1;
    int tilesize = hasParameter("tilesize") ? (*this)["tilesize"].value.toInt() : -1;

    std::function<bool(std::string s, int n)> callback = [this](std::string s, int n)->bool { return this->progressed(s, n); };

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
            const char* zoomError = deepZoom(inFolder, outFolder, quality, overlap, tilesize, callback);
            if (zoomError != NULL)
            {
                error = zoomError;
                status = FAILED;
                return;
            }
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
