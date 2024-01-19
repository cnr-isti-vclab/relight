#ifndef BNI_NORMAL_INTEGRATION_H
#define BNI_NORMAL_INTEGRATION_H

#include <QString>
#include <vector>
/* Bilateral Normal Integration Xu Cao, Hiroaki Santo1, Boxin Shi, Fumio Okura1, and Yasuyuki Matsushita1
 *
 * https://github.com/xucao-42/bilateral_normal_integration
*/

std::vector<double> bni_integrate(int w, int h, std::vector<float> &normalmap,
                                double k = 2.0,
                                double tolerance = 1e-4,
                                int max_iterations = 150,
                                int max_solver_iterations = 1000);

bool savePly(const QString &filename, int w, int h, std::vector<double> &z);
bool saveDepthMap(const QString &filename, int w, int h, std::vector<float> &z);

#endif // BNI_NORMAL_INTEGRATION_H
