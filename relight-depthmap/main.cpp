#include "camera.h"
#include "orthodepthmap.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>
#include <QStringList>

#include <iostream>

using namespace std;

int main(int argc, char *argv[]) {
	if (argc > 2 || (argc == 2 && (QString::fromLocal8Bit(argv[1]) == "-h" ||
	                               QString::fromLocal8Bit(argv[1]) == "--help"))) {
		cerr << "Usage: " << argv[0] << " [project_directory]" << endl;
		cerr << "The project directory contains photogrammetry/ and depthmaps/." << endl;
		return argc > 2 ? 1 : 0;
	}

	QCoreApplication app(argc, argv);
	QDir baseDir(argc == 2 ? QString::fromLocal8Bit(argv[1]) : QDir::currentPath());
	if (!baseDir.exists()) {
		cerr << "Project directory does not exist: " << qPrintable(baseDir.path()) << endl;
		return 1;
	}
	baseDir = QDir(baseDir.absolutePath());

	try {
		const QDir photogrammetryDir(baseDir.filePath("photogrammetry"));
		const QDir depthmapsDir(baseDir.filePath("depthmaps"));
		const QDir xmlDir(photogrammetryDir.filePath("Ori-Relative"));

		const QString depthmapPath = photogrammetryDir.filePath("Malt/Z_Num7_DeZoom4_STD-MALT.tif");
		const QString maskPath = photogrammetryDir.filePath("Malt/Masq_STD-MALT_DeZoom4.tif");
		const QString correlationPath = photogrammetryDir.filePath("Malt/Correl_STD-MALT_Num_6.tif");
		const QString pointCloudPath = photogrammetryDir.filePath("AperiCloud_Relative.ply");
		const QString depthXmlPath = photogrammetryDir.filePath("Malt/Z_Num7_DeZoom4_STD-MALT.xml");
		const QString outputPointsPath = baseDir.filePath("points_h.txt");
		const QString depthmapBackupPath = photogrammetryDir.filePath("Malt/Z_Num7_DeZoom4_STD-MALT_backup.tif");
		const QString maskBackupPath = photogrammetryDir.filePath("Malt/Masq_STD-MALT_DeZoom4_backup.tif");

		const QFileInfoList depthmapFiles =
			depthmapsDir.entryInfoList({"*.tiff"}, QDir::Files, QDir::Name);
		if (depthmapFiles.isEmpty())
			throw QString("No .tiff camera depth maps found in %1").arg(depthmapsDir.absolutePath());

		if (xmlDir.entryList({"*.xml"}, QDir::Files).isEmpty())
			throw QString("No orientation XML files found in %1").arg(xmlDir.absolutePath());

		OrthoDepthmap ortho;
		if (!ortho.loadDepth(qPrintable(depthmapBackupPath)))
			throw QString("Failed to load orthographic depth map: %1").arg(depthmapBackupPath);
		if (!ortho.loadMask(qPrintable(maskBackupPath)))
			throw QString("Failed to load orthographic mask: %1").arg(maskBackupPath);
		if (!ortho.loadCorrelation(qPrintable(correlationPath)))
			throw QString("Failed to load correlation map: %1").arg(correlationPath);
		if (!ortho.loadXml(qPrintable(depthXmlPath)))
			throw QString("Failed to load depth-map metadata: %1").arg(depthXmlPath);
		ortho.loadPointCloud(qPrintable(pointCloudPath));

		ortho.beginIntegration();

		const QStringList orientationExtensions = {
			".tiff", ".tif", ".jpg", ".jpeg"
		};
		for (const QFileInfo &depthmapFile : depthmapFiles) {
			const QString cameraName = depthmapFile.completeBaseName();
			QString orientationXmlPath;
			for (const QString &extension : orientationExtensions) {
				const QString candidate = xmlDir.filePath(
					"Orientation-" + cameraName + extension + ".xml");
				if (QFile::exists(candidate)) {
					orientationXmlPath = candidate;
					break;
				}
			}
			if (orientationXmlPath.isEmpty())
				throw QString("Missing camera parameter XML for image: %1").arg(cameraName);

			CameraDepthmap cameraDepthmap;
			if (!cameraDepthmap.camera.loadXml(orientationXmlPath))
				throw QString("Missing or invalid camera XML: %1").arg(orientationXmlPath);
			if (!cameraDepthmap.loadDepth(qPrintable(depthmapFile.absoluteFilePath())))
				throw QString("Unable to load camera depth map: %1").arg(depthmapFile.fileName());

			if (cameraDepthmap.width != cameraDepthmap.camera.width ||
			    cameraDepthmap.height != cameraDepthmap.camera.height) {
				qDebug() << "Resizing camera parameters for depth map:"
				         << depthmapFile.fileName();
				cameraDepthmap.camera.scale(cameraDepthmap.width, cameraDepthmap.height);
			}

			cout << "Processed: " << depthmapFile.fileName().toStdString() << endl;
			ortho.anchorCamera(cameraDepthmap, qPrintable(outputPointsPath));
		}

		ortho.endIntegration();
		ortho.saveDepth(qPrintable(depthmapPath));
		ortho.saveMask(qPrintable(maskPath));
		ortho.saveObj(qPrintable(photogrammetryDir.filePath("depthmap.obj")));

		const auto restoreTimestamp = [](const QString &path, const QString &backupPath) {
			if (!QFile::exists(backupPath))
				throw QString("Backup file is missing: %1").arg(backupPath);

			QFile file(path);
			if (!file.open(QIODevice::Append))
				throw QString("Unable to open file to restore its timestamp: %1").arg(path);
			if (!file.setFileTime(QFileInfo(backupPath).lastModified(),
			                      QFileDevice::FileModificationTime))
				throw QString("Unable to restore timestamp for: %1").arg(path);
		};
		restoreTimestamp(depthmapPath, depthmapBackupPath);
		restoreTimestamp(maskPath, maskBackupPath);
	} catch (const QString &error) {
		cerr << "Depth map processing failed: " << qPrintable(error) << endl;
		return 1;
	}

	return 0;
}
