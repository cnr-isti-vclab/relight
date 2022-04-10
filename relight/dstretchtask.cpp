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
    // Correlation matrix
    Eigen::MatrixXd correlation(3, 3);
    // Channel means
    Eigen::VectorXd means(3);
    means.fill(0);

    // TODO: make sure at least nSamples are sampled
    int samplesPerLine = 5;
    // Sample each line
    for (int row=0; row<height; row++)
    {
        // Read the line
        decoder.readRows(1, pixels.data());

        // Getting the samples
        for (int col=0; col<pixels.size(); col+=samplesPerLine*3)
        {
            for (int i=0; i<samplesPerLine; i++)
            {
                means(0) += pixels[col+i];
                means(1) += pixels[col+i +1];
                means(2) += pixels[col+i +2];
            }
            samples.push_back({pixels[col], pixels[col+1], pixels[col+2]});
        }

        progressed("Sampling...", (row * 100) / width);
    }

    // Compute the covariance matrix
    for (int l=0; l<3; l++)
    {
        for (int m=0; m<3; m++)
        {
            unsigned long long sumX = 0, sumM = 0, sumL = 0;
            for (int k=0; k<samples.size(); k++)
            {
                sumL += samples[k][l];
                if (m <= l)
                    sumM = sumL;
                sumX += sumL * samples[k][m];
            }
            covariance(l,m) = (1.0f/(samples.size()-1)) * (sumX - (1.0f/samples.size()) * sumL * sumM);
            progressed("Computing covariance...", (((l+1)*(m+1)+m+1) * 100) / 9);
        }
    }

    // Compute the correlation matrix
    for (int l=0; l<3; l++)
    {
        for (int m=0; m<3; m++)
        {
            correlation(l,m) = covariance(l,m) / std::sqrt(covariance(l,l) * covariance(m,m));
            progressed("Computing correlation...", (((l+1)*(m+1)+m+1) * 100) / 9);
        }
    }

    // Compute the rotation
    Eigen::EigenSolver<Eigen::MatrixXd> solver(correlation, true);
    Eigen::MatrixXd rotation = solver.eigenvectors().real();
    Eigen::MatrixXd eigenValues = solver.eigenvalues().real();

    // Compute the stretching factor
    eigenValues.unaryExpr([](float val){return 50 / std::sqrt(val);});

    // Compute the final transformation matrix
    Eigen::MatrixXd transformation = rotation.transpose() * eigenValues.asDiagonal() * rotation;
    // Compute the mean of the channels
    for (int i=0; i<3; i++)
        means(i) /= (width*height);
    // Apply the transformation to the mean
    Eigen::VectorXd offset = transformation * means;

    // Finally reposition the pixels with that offset
    decoder.init(input_folder.toStdString().c_str(), width, height);
    for (int i=0; i<height; i++)
    {
        decoder.readRows(1, pixels.data());
        for (int k=0; k<pixels.size(); k+=3)
        {
            dstretched.push_back(pixels[k] + offset(0));
            dstretched.push_back(pixels[k+1] + offset(1));
            dstretched.push_back(pixels[k+2] + offset(2));
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
