import glob
import rps
import psutil

import ast
import os
import sys

import json
import numpy as np



# methods
#    L2_SOLVER = 0   # Conventional least-squares
#    L1_SOLVER = 1   # L1 residual minimization
#    L1_SOLVER_MULTICORE = 2 # L1 residual minimization (multicore)
#    SBL_SOLVER = 3  # Sparse Bayesian Learning
#    SBL_SOLVER_MULTICORE = 4    # Sparse Bayesian Learning (multicore)
#    RPCA_SOLVER = 5    # Robust PCA 

if __name__ == '__main__':

	jsonfile = sys.argv[1]
	output = sys.argv[2]
	method = int(sys.argv[3])


	with open(jsonfile) as f:
		data = json.load(f)

	images = data['images']

	crop = None
	if 'crop' in data:
		crop = data['crop']

	print(crop)
	rps = rps.RPS()

	rps.L = np.transpose(np.array(data['lights']))
	rps.M, rps.height, rps.width = psutil.load_image_list(images, crop)

	print(rps.L.shape, rps.M.shape)
	rps.solve(method)

	psutil.save_normalmap_as_png(filename=output, normal=rps.N, height=rps.height, width=rps.width)
