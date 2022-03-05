#include "normalstask.h"
#include "jpeg_decoder.h"
#include "imageset.h"

#include <Eigen/Eigen>
#include <Eigen/Core>
#include <Eigen/Dense>

#include <QJsonArray>
#include <QJsonDocument>
#include <vector>
#include <iostream>

void NormalsTask::run()
{
    switch (m_Method)
    {
    // L2 solver
    case 0:
        solveL2();
        break;
    // SBL solver
    case 4:
        solveSBL();
        break;
    // RPCA solver
    case 5:
        solveRPCA();
        break;
    }
}

/**TODO:
 * - test
 * - support cropping
 */
void NormalsTask::solveL2()
{
    // ImageSet initialization
    ImageSet imageSet(m_InputDir.toStdString().c_str());
    std::function<bool(std::string stage, int percent)> callback = [this](std::string s, int n)->bool { return this->progressed(s, n); };
    imageSet.setCallback(&callback);

    // Pixel data
    PixelArray line;
    Eigen::MatrixXd mLights(imageSet.lights.size(), 3);
    Eigen::MatrixXd mPixel(imageSet.lights.size(), 3);
    Eigen::MatrixXd mNormals(imageSet.lights.size(), 3);

    // Fill the lights matrix
    for (int i=0; i<imageSet.lights.size(); i++)
        for (int j=0; j<3; j++)
            mLights(i, j) = imageSet.lights.at(i)[0];

    // Process images
    for (int r=0; r<imageSet.height; r++)
    {
        // Ran line by line atm
        imageSet.readLine(line);
        // For each pixel in the line solve the system
        for (int p=0; p<line.size(); p++)
        {
            // Get the color data of the pixel for all the nLights images
            std::vector<Color3f> color = line.pixel(p);
            // Fill the pixel vector
            for (int m=0; m<color.size(); m++)
            {
                Color3f currColor = color[m].clip();

                mPixel(m, 0) = currColor.r;
                mPixel(m, 1) = currColor.g;
                mPixel(m, 2) = currColor.b;
            }
            // Solve
            mNormals = (mLights.transpose() * mLights).ldlt().solve(mLights.transpose() * mPixel);
        }
    }
}

void NormalsTask::solveSBL()
{
}

void NormalsTask::solveRPCA()
{
}

void NormalsTask::pause()
{
    status = PAUSED;
}

void NormalsTask::resume()
{
    if(status == PAUSED)
        status = RUNNING;
}

void NormalsTask::stop()
{
    status = STOPPED;
}

bool NormalsTask::progressed(std::string s, int percent)
{
    QString str(s.c_str());
    emit progress(str, percent);
    if(status == STOPPED)
        return false;
    return true;
}
