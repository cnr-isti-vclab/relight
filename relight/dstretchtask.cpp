#include "dstretchtask.h"
#include "relight_vector.h"
#include "jpeg_encoder.h"
#include "jpeg_decoder.h"
#include <cmath>
#include <Eigen/Eigen>
#include <Eigen/Eigenvalues>
#include <QDebug>

/** TODO
 *      - Crop image to specify a subsection for sampling
 */


void DStretchTask::run()
{
    status = RUNNING;
    // Get sample rate
    if (hasParameter("sample_rate"))
        m_SampleRate = (*this)["sample_rate"].value.toInt();
    else {
        error = "Unspecified sample rate";
        status = FAILED;
        return;
    }

    // Vector used to temporarily store a line of pixels
    std::vector<uint8_t> pixels;
    // Vector used to store samples
    std::vector<Color3b> samples;
    // Final data to be saved
    std::vector<uint8_t> dstretched;
    // Size of the image
    int width, height;
    // Jpeg encoder and decoder to handle output and input
    JpegDecoder decoder;
    JpegEncoder encoder;

    decoder.init(input_folder.toStdString().c_str(), width, height);
    encoder.init(output.toStdString().c_str(), width, height);
    pixels.resize(width * 3);

    // Covariance matrix
    Eigen::MatrixXd covariance(3, 3);
    // Channel means
    Eigen::VectorXd means(3);
    means.fill(0);

    // TODO: make sure at least nSamples are sampled
    uint32_t samplesHorizontal = std::ceil(std::sqrt(1000) * ((float)width / height));
    uint32_t samplesVertical = std::ceil(std::sqrt(1000) * ((float)height / width));
    uint32_t rowSkip = height / samplesVertical;
    uint32_t colSkip = width / samplesHorizontal;

    // Sample each line
    for (int row=0; row<height; row++)
    {
        // Read the line
        decoder.readRows(1, pixels.data());

        if (row % rowSkip == 0)
        {
            qDebug() << "Row: " << row;
            // Getting the samples
            for (int col=0; col<pixels.size(); col+=colSkip*3)
            {
                means(0) += pixels[col];
                means(1) += pixels[col +1];
                means(2) += pixels[col +2];

                Color3b color(pixels[col], pixels[col+1], pixels[col+2]);
                samples.push_back(color);
            }
        }

        progressed("Sampling...", (row * 100) / width);
    }

    // Compute the mean of the channels
    for (int i=0; i<3; i++)
        means(i) /= samples.size();

    long long sumChannel[]= {0,0,0};
    long double sumX[][3] = {{0,0,0},{0,0,0},{0,0,0}};

    for (int k=0; k<samples.size(); k++)
        for (int i=0; i<3; i++)
            sumChannel[i] += samples[k][i];

    for (int l=0; l<3; l++)
        for (int m=0; m<3; m++)
            for (int k=0; k<samples.size(); k++)
                sumX[l][m] += (samples[k][l] * samples[k][m]) / (samples.size()-1);

    double reducer = ((long double)1.0f/((samples.size()-1) * samples.size()));
    for (int l=0; l<3; l++)
    {
        for (int m=0; m<3; m++)
        {
            long double channelLreduced = reducer * sumChannel[l];
            long double bothChannels = channelLreduced * sumChannel[m];
            covariance(l, m) = sumX[l][m] - bothChannels;
        }
    }

    // Compute the rotation
    Eigen::EigenSolver<Eigen::MatrixXd> solver(covariance, true);
    Eigen::MatrixXd rotation = solver.eigenvectors().real();
    Eigen::MatrixXd eigenValues = solver.eigenvalues().real();

    Eigen::MatrixXd sigma = covariance.diagonal().asDiagonal();
    for (int i=0; i<3; i++)
        sigma(i, i) = std::sqrt(sigma(i,i));

    qDebug() << eigenValues(0) << "," << eigenValues(1) << "," << eigenValues(2) << "," ;
    // Compute the stretching factor
    for (int i=0; i<3; i++)
        eigenValues(i) = 1.0f / std::sqrt(eigenValues(i) >= 0 ? eigenValues(i) : -eigenValues(i));
    qDebug() << eigenValues(0) << "," << eigenValues(1) << "," << eigenValues(2) << "," ;

    // Compute the final transformation matrix
    Eigen::MatrixXd transformation = sigma * rotation * eigenValues.asDiagonal() * rotation.transpose();
    // Apply the transformation to the mean
    Eigen::VectorXd offset = means - transformation * means;

    // Finally reposition the pixels with that offset
    decoder.init(input_folder.toStdString().c_str(), width, height);
    Eigen::VectorXd currPixel(3);

    for (int i=0; i<height; i++)
    {
        decoder.readRows(1, pixels.data());
        for (int k=0; k<pixels.size(); k+=3)
        {
            currPixel(0) = pixels[k] - means(0);
            currPixel(1) = pixels[k+1] - means(1);
            currPixel(2) = pixels[k+2] - means(2);

            currPixel = transformation * currPixel + means;

            dstretched.push_back(currPixel(0));
            dstretched.push_back(currPixel(1));
            dstretched.push_back(currPixel(2));
        }
    }

    encoder.writeRows(dstretched.data(), height);
    encoder.finish();

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
