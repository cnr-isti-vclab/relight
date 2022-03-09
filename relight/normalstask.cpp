#include "normalstask.h"
#include "jpeg_decoder.h"
#include "jpeg_encoder.h"
#include "imageset.h"
#include "threadpool.h"

#include <Eigen/Eigen>
#include <Eigen/Core>
#include <Eigen/Dense>

#include <QJsonArray>
#include <QJsonDocument>
#include <QDebug>
#include <vector>
#include <iostream>
#include <time.h>

//////////////////////////////////////////////////////// NORMALS TASK //////////////////////////////////////////////////////////
/// \brief NormalsTask: Takes care of creating the normals from the images given in a folder (inputFolder) and saves the file
///         in the outputFolder. After applying the crop described in the QRect passed as an argument to the constructor,
///         the NormalsTask creates a NormalsWorker for each line in the final image.
///         That NormalsWorker fills a vector with the colors of the normals in that line.
///

void NormalsTask::run()
{
    // Save the normal as jpg file
    JpegEncoder encoder;
    // ImageSet initialization
    ImageSet imageSet(m_InputFolder.toStdString().c_str());
    // Normals vector
    std::vector<uint8_t> normals;

    int start = clock();
    // Init
    normals.resize(imageSet.width * imageSet.height * 3);
    imageSet.setCallback(nullptr);

    // Set the crop
    if(!m_Crop.isValid()) {
        m_Crop.setLeft(0);
        m_Crop.setWidth(imageSet.width);
        m_Crop.setTop(0);
        m_Crop.setHeight(imageSet.height);
    }
    imageSet.crop(m_Crop.left(), m_Crop.top(), m_Crop.width(), m_Crop.height());

    // Thread pool used to handle the processors
    ThreadPool pool;
    // Line in the imageset to be processed
    PixelArray line;
    // Saving the workers in order to delete them at the end of the task
    NormalsWorker** workers = new NormalsWorker*[imageSet.height];

    pool.start(QThread::idealThreadCount());

    for (int i=0; i<imageSet.height; i++)
    {
        // Read a line
        imageSet.readLine(line);

        // Create the normal task and get the run lambda
        uint32_t idx = i * 3 * imageSet.width;
        NormalsWorker* task = new NormalsWorker(m_Method, line, normals.data() + idx, imageSet.lights);
        std::function<void(void)> run = [task](void)->void { return task->run();};

        // Save the task to delete it later
        workers[i] = task;

        // Launch the task
        pool.queue(run);
        pool.waitForSpace();

        progressed("Computing normals...", ((float)i / imageSet.height) * 100);
    }

    // Wait for the end of all the threads
    pool.finish();
    // Save the final result
    encoder.encode(normals.data(), imageSet.width, imageSet.height, m_OutputFolder.toStdString().c_str());

    progressed("Cleaning up...", 99);

    // Avoid leaks
    for (int i=0; i<imageSet.height; i++)
        delete workers[i];
    delete[] workers;

    int end = clock();
    qDebug() << "Time: " << ((double)(end - start) / CLOCKS_PER_SEC);
    progressed("Finished", 100);
}

bool NormalsTask::progressed(std::string s, int percent)
{
    QString str(s.c_str());
    emit progress(str, percent);
    if(status == STOPPED)
        return false;
    return true;
}

//////////////////////////////////////////////////////// NORMALS WORKER //////////////////////////////////////////////////////////
/// \brief NormalsWorker: generates the normals for a given line in the image set, depending on the method specified when
///         creating the Worker.
///


void NormalsWorker::run()
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
void NormalsWorker::solveL2()
{
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
}

void NormalsWorker::solveSBL()
{
}

void NormalsWorker::solveRPCA()
{
}

