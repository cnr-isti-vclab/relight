#ifndef DSTRETCH_H
#define DSTRETCH_H

#include <QFile>
#include "../src/imageset.h"
#include "../src/jpeg_decoder.h"
#include "../src/jpeg_encoder.h"
#include <Eigen/Eigen>
#include <Eigen/Eigenvalues>
#include <QString>
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

inline void dstretchImage(QString fileName, QString output, int minSamples, std::function<bool(QString s, int n)> progressed)
{
    // info.json
    QString infoFileName = output.mid(output.lastIndexOf("/") + 1, output.length()).split(".")[0];
    QFile info(output.mid(0, output.lastIndexOf("/")) + "/" + infoFileName + ".dstretch");
    QJsonObject infoObject;
    QJsonArray samplesJson;
    QJsonArray transformJson;
    QJsonDocument outputJson;
    // Vector used to temporarily store a line of pixels
    std::vector<uint8_t> pixels;
    // Vector used to store samples
    std::vector<Color3b> samples;

    // Final data to be saved
    std::vector<int> dstretched;
    std::vector<uint8_t> dstretchedBytes;
    // Jpeg encoder and decoder to handle output and input
    JpegDecoder decoder;
    JpegEncoder encoder;

    // Size of the image
    int width, height;
    // Covariance matrix
    Eigen::MatrixXd covariance(3, 3);

    // Max and min values for channels (used to rescale the output)
    int mins[] = {256, 256, 256};
    int maxs[] = {-1, -1, -1};

    // Initialization
    decoder.init(fileName.toStdString().c_str(), width, height);
    encoder.init(output.toStdString().c_str(), width, height);
    pixels.resize(width * 3);

    // Compute the distances between a sample and another one so that at least minSamples are taken
    uint32_t samplesHorizontal = std::ceil(std::sqrt(minSamples) * ((float)width / height));
    uint32_t samplesVertical =  std::ceil(std::sqrt(minSamples) * ((float)height / width));
    uint32_t rowSkip = std::max<uint32_t>(1, height / samplesVertical);
    uint32_t colSkip = std::max<uint32_t>(1, width / samplesHorizontal);

    // Sample each line
	for (int row=0; row<height; row++) {
        // Read the line
        decoder.readRows(1, pixels.data());

		if (row % rowSkip == 0) {
            // Getting the samples
			for (size_t col=0; col<pixels.size(); col+=colSkip*3) {
                Color3b color(pixels[col], pixels[col+1], pixels[col+2]);

                samples.push_back(color);
                samplesJson.push_back(QJsonArray({color.r, color.g, color.b}));
            }
        }

        if (row % 10 == 0)
            progressed("Sampling...", (row * 100) / width);
    }

    // Compute the sums needed to compute the covariance
    long sumChannel[]= {0,0,0};
    double sumX[][3] = {{0,0,0},{0,0,0},{0,0,0}};

	for (size_t k = 0; k < samples.size(); k++)
		for (int i = 0; i < 3; i++)
            sumChannel[i] += samples[k][i];

	for (int l = 0; l < 3; l++)
		for (int m = 0; m < 3; m++)
			for (size_t k = 0; k < samples.size(); k++)
                sumX[l][m] += samples[k][l] * samples[k][m];

    // Compute the covariance
    for (int l=0; l<3; l++)
        for (int m=0; m<3; m++)
            covariance(l,m) = ((double)(1.0f/(samples.size() - 1))) * (sumX[l][m] - ((double)1.0f/samples.size())*sumChannel[l]*sumChannel[m]);

    // Compute the rotation
    Eigen::EigenSolver<Eigen::MatrixXd> solver(covariance, true);
    Eigen::MatrixXd rotation = solver.eigenvectors().real();
    Eigen::MatrixXd eigenValues = solver.eigenvalues().real();

    Eigen::MatrixXd sigma = covariance.diagonal().asDiagonal();
    for (int i=0; i<3; i++)
        sigma(i, i) = std::sqrt(sigma(i,i));

    // Compute the stretching factor
    for (int i=0; i<3; i++)
        eigenValues(i) = 1.0f / std::sqrt(eigenValues(i) >= 0 ? eigenValues(i) : -eigenValues(i));

    // Compute the final transformation matrix
    Eigen::MatrixXd transformation = sigma * rotation * eigenValues.asDiagonal() * rotation.transpose();

    // Finally reposition the pixels with that offset
    decoder.init(fileName.toStdString().c_str(), width, height);
    Eigen::VectorXd currPixel(3);

	for (int i = 0; i < height; i++) {
        decoder.readRows(1, pixels.data());
		for (size_t k = 0; k < pixels.size(); k+=3) {
            for (int j=0; j<3; j++)
                currPixel(j) = pixels[k+j];

            currPixel = transformation * currPixel;

			for (int j = 0; j < 3; j++) {
                dstretched.push_back(currPixel[j]);
                mins[j] = std::min<int>(mins[j], currPixel[j]);
                maxs[j] = std::max<int>(maxs[j], currPixel[j]);
            }
        }

        if (i % 100 == 0)
            progressed("Transforming...", (i * 100) / height);
    }

	for (size_t k =0; k < dstretched.size(); k++)
    {
        uint32_t channelIdx = k % 3;
        dstretchedBytes.push_back(255 * ((float)(dstretched[k] - mins[channelIdx]) / (maxs[channelIdx] - mins[channelIdx])));

        if (k % 100 == 0)
            progressed("Scaling...", (k * 100) / dstretched.size());
    }

    progressed("Saving...", 50);
    encoder.writeRows(dstretchedBytes.data(), height);
    encoder.finish();

    // Prepare and save the info.json file
    info.open(QIODevice::WriteOnly);
    info.setPermissions(QFileDevice::WriteOther | QFileDevice::WriteOwner);
    infoObject["image_name"] = fileName.mid(fileName.lastIndexOf("/")+1, fileName.size());
    for (int i=0; i<3; i++)
    {
        for (int j=0; j<3; j++)
            transformJson.push_back(transformation(i,j));
        transformJson.push_back(0);
    }
    transformJson.push_back(0);
    transformJson.push_back(0);
    transformJson.push_back(0);
    transformJson.push_back(1);

    infoObject["transformation"] = transformJson;
    infoObject["samples"] = samplesJson;

    outputJson.setObject(infoObject);
    info.write(outputJson.toJson());
    info.close();
}

#endif
