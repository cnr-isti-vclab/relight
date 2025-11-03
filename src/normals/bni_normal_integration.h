#ifndef BNI_NORMAL_INTEGRATION_H
#define BNI_NORMAL_INTEGRATION_H

#include <QString>
#include <vector>
#include <functional>
#include <Eigen/Core>
/* Bilateral Normal Integration Xu Cao, Hiroaki Santo1, Boxin Shi, Fumio Okura1, and Yasuyuki Matsushita1
 *
 * https://github.com/xucao-42/bilateral_normal_integration
*/

void bni_integrate(std::function<bool(QString s, int n)> progressed,
								  int w, int h, std::vector<Eigen::Vector3f> &normalmap, std::vector<float> &heights,
								  double k = 2.0,
								  double tolerance = 1e-5,
								  double solver_tolerance = 1e-5,
								  int max_iterations = 10,
								  int max_solver_iterations = 500);

std::vector<float> bni_pyramid(std::function<bool(QString s, int n)> progressed,
								  int &w, int &h, std::vector<Eigen::Vector3f> &normalmap,
								  double k = 2.0,
								  double tolerance = 1e-5,
								  double solver_tolerance = 1e-5,
								  int max_iterations = 15,
								  int max_solver_iterations = 500,
								  int scale = 0);

bool savePly(const QString &filename, size_t w, size_t h, std::vector<float> &z);
bool saveTiff(const QString &filename, size_t w, size_t h, std::vector<float> &z, bool normalize = false);
bool saveDepthMap(const QString &filename, size_t w, size_t h, std::vector<float> &z);

void bilinear_interpolation3f(Eigen::Vector3f *data, uint32_t input_width,
							  uint32_t input_height, uint32_t output_width,
							  uint32_t output_height, Eigen::Vector3f *output);


#endif // BNI_NORMAL_INTEGRATION_H
