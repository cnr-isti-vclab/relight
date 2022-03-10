#ifndef LP_H
#define LP_H

class QString;

#include "relight_vector.h"
#include <vector>

void parseLP(QString sphere_path, std::vector<Vector3f> &lights, std::vector<QString> &filenames);

#endif // LP_H
