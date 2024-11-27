#ifndef LP_H
#define LP_H

class QString;

#include <Eigen/Core>
#include <vector>

void parseLP(QString sphere_path, std::vector<Eigen::Vector3f> &lights, std::vector<QString> &filenames);

#endif // LP_H
