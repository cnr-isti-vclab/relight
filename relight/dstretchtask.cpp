#include "dstretchtask.h"
#include "../src/relight_vector.h"
#include "../src/jpeg_encoder.h"
#include "../src/jpeg_decoder.h"
#include "dstretch.h"
#include <cmath>
#include <Eigen/Eigen>
#include <Eigen/Eigenvalues>
#include <QDebug>

/** TODO
 *      - Crop image to specify a subsection for sampling
 */

void DStretchTask::run()
{
    uint32_t minSamples;
    std::function<bool(std::string s, int n)> callback = [this](std::string s, int n)->bool { return this->progressed(s, n); };
    status = RUNNING;

    // Get sample rate
    if (hasParameter("min_samples"))
        minSamples = (*this)["min_samples"].value.toInt();
    else {
        error = "Unspecified sample rate";
        status = FAILED;
        return;
    }

    dstretchImage(input_folder, output, minSamples, callback);
    status = DONE;
}

bool DStretchTask::progressed(std::string s, int percent)
{
    if(status == PAUSED) {
        mutex.lock();
        mutex.unlock();
    }
    if(status == STOPPED)
        return false;

    QString str(s.c_str());
    emit progress(str, percent);
    if(status == STOPPED)
        return false;
    return true;
}
