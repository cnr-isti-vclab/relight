#ifndef BNI_NORMAL_INTEGRATION_H
#define BNI_NORMAL_INTEGRATION_H

#include <QString>
#include <vector>
#include <functional>
/* Bilateral Normal Integration Xu Cao, Hiroaki Santo1, Boxin Shi, Fumio Okura1, and Yasuyuki Matsushita1
 *
 * https://github.com/xucao-42/bilateral_normal_integration
*/

void bni_integrate(std::function<bool(QString s, int n)> progressed,
								  int w, int h, std::vector<float> &normalmap, std::vector<float> &heights,
								  double k = 2.0,
								  double tolerance = 1e-5,
								  double solver_tolerance = 1e-5,
								  int max_iterations = 10,
								  int max_solver_iterations = 5000);

std::vector<float> bni_pyramid(std::function<bool(QString s, int n)> progressed,
								  int &w, int &h, std::vector<float> &normalmap,
								  double k = 2.0,
								  double tolerance = 1e-5,
								  double solver_tolerance = 1e-5,
								  int max_iterations = 150,
								  int max_solver_iterations = 5000,
								  int scale = 0);

bool savePly(const QString &filename, size_t w, size_t h, std::vector<float> &z);
bool saveTiff(const QString &filename, size_t w, size_t h, std::vector<float> &z);
bool saveDepthMap(const QString &filename, size_t w, size_t h, std::vector<float> &z);

#endif // BNI_NORMAL_INTEGRATION_H
