#include "material.h"

using namespace std;
vector<float> MaterialBuilder::toPrincipal(float *v) {
	size_t dim = mean.size();
	size_t nplanes = proj.size()/dim;


	vector<float> res(nplanes, 0.0f);
/*	if(colorspace == LRGB) {
		float col[3] = {0, 0, 0};
		for(size_t k = 0; k < dim; k += 3) {
			res[0] += v[k+0];
			res[1] += v[k+1];
			res[2] += v[k+2];
		}
		//saturate:
		float top = std::max(res[0], std::max(res[1], res[2]));
		res[0] /= top;
		res[1] /= top;
		res[2] /= top;
		float luma = (res[0] + res[1] + res[2])/3;

		for(size_t p = 3; p < nplanes; p++) {
			for(size_t k = 0; k < dim; k++)
				res[p] += v[k] * proj[k + p*dim];
		}


	} else { //RGB, MRGB
		vector<float> col(dim);

		for(size_t k = 0; k < dim; k++)
			col[k] = v[k] - mean[k];

		for(size_t p = 0; p < nplanes; p++) {
			for(size_t k = 0; k < dim; k++) {
				res[p] += col[k] * proj[k + p*dim];
			}
		}
	} */
	return res;
}

vector<float> MaterialBuilder::toVariable(const vector<float> &v) {
	/*	arma::Col<double> column(arma::trans(v * proj.t()));
		column += mean;
		vector<float> res(column.size());
		for(int i = 0; i < res.size(); i++)
			res[i] = column[i];
		return res; */
	return vector<float>();
}
