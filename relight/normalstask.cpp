#include "normalstask.h"
#include "jpeg_decoder.h"
#include "jpeg_encoder.h"
#include "imageset.h"

#include <Eigen/Eigen>
#include <Eigen/Core>
#include <Eigen/Dense>

#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>
#include <vector>
#include <iostream>
#include <time.h>

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
 * - support cropping
 */
void NormalsTask::solveL2()
{
    int start = clock();
    // Pixel data
    Eigen::MatrixXd mLights(m_Lights.size(), 3);
    Eigen::MatrixXd mPixel(m_Lights.size(), 1);
    Eigen::MatrixXd mNormals;

    unsigned int normalIdx = 0;

    // Fill the lights matrix
    for (int i=0; i<m_Lights.size(); i++)
        for (int j=0; j<3; j++)
            mLights(i, j) = m_Lights[i][j];

    // For each pixel in the line solve the system
    for (int p=0; p<m_Row.size(); p++)
    {
        // Fill the pixel vector
        for (int m=0; m<m_Lights.size(); m++)
            mPixel(m, 0) = m_Row[p][m].mean();

        // Solve
        mNormals = (mLights.transpose() * mLights).ldlt().solve(mLights.transpose() * mPixel);
        mNormals.col(0).normalize();

        // Save
        m_Normals[normalIdx] = floor(((mNormals(0, 0) + 1.0f) / 2.0f) * 255);
        m_Normals[normalIdx+1] = floor(((mNormals(1, 0) + 1.0f) / 2.0f) * 255);
        m_Normals[normalIdx+2] = floor(((mNormals(2, 0) + 1.0f) / 2.0f) * 255);

        normalIdx += 3;
    }

    int end = clock();
    qDebug() << "TIME: " << ((float)(end - start) / CLOCKS_PER_SEC);
}

void NormalsTask::solveSBL()
{
}

void NormalsTask::solveRPCA()
{
}

bool NormalsTask::progressed(std::string s, int percent)
{
    QString str(s.c_str());
    emit progress(str, percent);
    if(status == STOPPED)
        return false;
    return true;
}
