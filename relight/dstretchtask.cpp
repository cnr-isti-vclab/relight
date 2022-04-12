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
    uint32_t minSamples;

    status = RUNNING;
    // Get sample rate
    if (hasParameter("min_samples"))
        minSamples = (*this)["min_samples"].value.toInt();
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
    std::vector<int> dstretched;
    std::vector<uint8_t> dstretchedBytes;
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
    // Max and min values for channels (used to rescale the output)
    int mins[] = {256, 256, 256};
    int maxs[] = {-1, -1, -1};

    // TODO: make sure at least nSamples are sampled
    uint32_t samplesHorizontal = std::ceil(std::sqrt(minSamples) * ((float)width / height));
    uint32_t samplesVertical = std::ceil(std::sqrt(minSamples) * ((float)height / width));
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
                for (int i=0; i<3; i++)
                    means(i) += pixels[i];

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
            for (int j=0; j<3; j++)
                currPixel(j) = pixels[k+j];

            currPixel -= means;
            currPixel = transformation * currPixel + means + offset;

            for (int j=0; j<3; j++)
            {
                dstretched.push_back(currPixel[j]);
                mins[j] = std::min<int>(mins[j], currPixel[j]);
                maxs[j] = std::max<int>(maxs[j], currPixel[j]);
            }
        }

        progressed("Transforming...", (i * 100) / height);
    }

    for (int k=0; k<dstretched.size(); k++)
    {
        uint32_t channelIdx = k % 3;
        dstretchedBytes.push_back(255 * ((float)(dstretched[k] - mins[channelIdx]) / (maxs[channelIdx] - mins[channelIdx])));
        progressed("Scaling...", (k * 100) / dstretched.size());
    }

    encoder.writeRows(dstretchedBytes.data(), height);
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
