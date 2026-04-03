#ifndef NORMALSTASK_H
#define NORMALSTASK_H

#include "../task.h"
#include "../project.h"
#include "../imageset.h"
#include "normals_parameters.h"
#include "normalsworker.h"

#include <QJsonObject>
#include <QMutex>
#include <QRect>
#include <QRunnable>
#include <QString>

#include <Eigen/Core>

#include <functional>


class NormalsTask :  public Task {
public:
	NormalsParameters parameters;

	ImageSet imageset;
	Lens lens;
	float z_threshold =0.001;

	virtual void run() override;
	virtual QJsonObject info() const override;

	void setParameters(NormalsParameters &param);
	void initFromProject(Project &project);
	void initFromFolder(const char *folder, Dome &dome, const Crop &crop);

	void assm(QString filename, std::vector<Eigen::Vector3f> &normals, int width, int height, float precision,
			  std::function<bool(QString, int)> *_callback);
	void fixNormal(Eigen::Vector3f &n); //check for nan, and z< threshold
};

#endif // NORMALSTASK_H
