#ifndef FFT_NORMAL_INTEGRATION_H
#define FFT_NORMAL_INTEGRATION_H


#include <QString>
#include <vector>
#include <functional>
#include <Eigen/Core>
/* Robert T. Frankot and Rama Chellappa
 * A Method for Enforcing Integrability in Shape from Shading
 * IEEE PAMI Vol 10, No 4 July 1988. pp 439-451
 */

void fft_integrate(std::function<bool(QString s, int n)> progressed,
								  int w, int h, std::vector<Eigen::Vector3f> &normalmap, std::vector<float> &heights);



#endif // FFT_NORMAL_INTEGRATION_H
