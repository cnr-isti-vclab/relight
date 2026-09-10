function [XYZ,N,rho,Phi,mask,tab_nrj] = near_ps(data,calib,params)
%NEAR_PS solves photometric stereo under nearby point light sources
%
%	=== USAGE ===	
%
%	XYZ = NEAR_PS(DATA,CALIB) estimates an
%	NROWS x NCOLS x 3 gridded point cloud, using the images in DATA and
%	the calibration parameters in CALIB
%
%	XYZ = NEAR_PS(DATA,CALIB,PARAMS) uses the algorithm parameters in
%	set in PARAMS
%
%	[XYZ,N] = NEAR_PS(...) also provides an NROWS x NCOLS x 3 normal
%	map
%
%	[XYZ,N,rho] = NEAR_PS(...) also provides an 
%	NROWS x NCOLS x NCHANNELS albedo map, where NCHANNELS = 1 if the
%	images are graylevel images, and NCHANNELS = 3 if they are RGB
%
%	[XYZ,N,rho,Phi] = NEAR_PS(...) also provides an NIMGS x NCHANNELS
%	matrix containing lighting intensities
%
%	[XYZ,N,rho,mask] = NEAR_PS(...) also provides the binary mask
%
%	[XYZ,N,rho,mask,tab_nrj] = NEAR_PS(...) also provides a vector tab_nrj
%	containing the energy at each iteration
%
%	=== DATA ===
%	
%	- Required fields:
%		- DATA.I:	NROWS x NCOLS x NIMGS (graylevel images) or
%				NROWS x NCOLS x NCHANNELS x NIMGS (color images)
%	- Optional fields:
%		- DATA.mask:	M x N binary mask (default: ones(NROWS,NCOLS))
%
%	=== CALIB ===
%	- Required fields:
%		- CALIB.S:	NIMGS x 3 light sources locations
%		- CALIB.K:	3 x 3 intrinsics matrix
%
%	- Optional fields:
%		- CALIB.Phi:	NIMGS x NCHANNELS intensities 
%				(default: ones(NIMGS,NCHANNELS)
%		- CALIB.mu:	NIMGS x 1 anisotropy factor 
%				(default: zeros(NIMGS,1))
%		- CALIB.Dir:	NIMGS x 3 orientations 
%			(default: [zeros(NIMGS,1);zeros(NIMGS,1);ones(NIMGS,1)])
%
%	=== PARAMETERS ===
%	- Optional fields:
%		- PARAMS.semicalibrated: binary variable indicating if
%		intensities are refined or not
%			(default: 0)
%		- PARAMS.z0: NROWS x NCOLS initial depth map
%			(default: 700*ones(NROWS,NCOLS))
%		- PARAMS.estimator: string indicating which estimator is used:
%		estimator = 'LS' uses least-squares
%		estimator = 'Cauchy' uses Cauchy's M-estimator
%		estimator = 'GM' uses Geman and McClure's M-estimator
%		estimator = 'Tukey' uses Tukey's biweight M-estimator
%		estimator = 'Welsh' uses Welsh's M-estimator
%			(default: 'LS')
%		- PARAMS.lambda: parameter of the robust estimator
%			(default: 0, since LS estimator requires no estimator)
%			(we recommend e.g. 0.1 for Cauchy)
%		- PARAMS.zeta: hyper-parameter controlling how close we stay from z0
%			(default: 0)
%		- PARAMS.self_shadows: binary variable indicating if
%		self-shadows are included in the model or not	
%			(default: 1)
%		- PARAMS.used:	NROWS x NCOLS x NIMGS (graylevel) or
%				NROWS x NCOLS x NCHANNELS x NIMGS (color) 
%						(binary map indicating whether data is used or not)
%		- PARAMS.maxit: max number of iterations
%			(default: 100)
%		- PARAMS.tol: relative stopping criterion on the energy
%			(default: 1e-3)
%		- PARAMS.tol_normals: stopping criterion on median angular difference between two normal maps
%			(default: 0.1)
%		- PARAMS.ratio: integer downsampling factor 
%			(default: 1)
%		- PARAMS.display: binary variable indicating if current shape
%		and albedo are displayed at each iteration 
%			(default: 0)
%		- PARAMS.precond: preconditioner used
%		precond = 'cmg' uses Koutis conjugate multigrid preconditioner
%		precond = 'ichol' uses incomplete Cholesky 
%		precond = 'gs' uses Gauss-Seidel 
%		precond = 'jacobi' uses Jacobi 
%			(default: 'jacobi', but 'cmg' is strongly recommended)
%		- PARAMS.tol_pcg: relative stopping criterion for inner CG iter
%			(default: 1e-6)
%		- PARAMS.maxit_pcg: max iterations for inner CG iter
%			(default: 25)
%
%		- PARAMS.scales: the number of scales in the multi-scale approach
%			(default: 1)
%
%	=== CREDITS ===
%
%	This is an implementation of the method described in Section 4 of:
%	[1]	"LED-based Photometric Stereo: Modeling, Calibration and Numerical Solution"
%		Quéau et al.
%		2017 
%
%	It was initially presented in:
%	[2]	"Semi-calibrated Near-Light Photometric Stereo" 
%		Quéau et al. 
%		Proc. SSVM 2017, Kolding, Denmark

	disp(' ');
	
	if(nargin<2)
		disp('ERROR: must provide data and calibration');
		return;
	end

	%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	%%% Check DATA
	% Check images
	if(~isfield(data,'I'))
		disp('ERROR: images not provided in DATA.I')
		return;
	end
	I = double(data.I); clear data.I;
	if(size(I,4)==1) % Graylevel images
		nchannels = 1;
		[nrows,ncols,nimgs] = size(I);
	else % Color images
		[nrows,ncols,nchannels,nimgs] = size(I);
	end
	% Check mask
	if(~isfield(data,'mask'))
		disp('WARNING: mask not provided in DATA.mask, using full 2D grid')
		data.mask = ones(nrows,ncols);
	end
	mask = double(data.mask)>0; clear data.mask;
	if(size(mask,1)~=nrows | size(mask,2)~=ncols | ndims(mask)>2)
		disp(sprintf('ERROR: mask should be %d x %d',nrows,ncols));
		return;
	end
	clear data

	%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	%%% Check CALIB	
	% Check locations
	if(~isfield(calib,'S'))
		disp('ERROR: sources locations not provided in DATA.S')
		return;
	end
	S = double(calib.S); clear calib.S;
	if(size(S,1)~=nimgs | size(S,2)~= 3 | ndims(S)>2) 
		disp(sprintf('ERROR: S should be %d x 3'),nimgs);
		return;
	end
	% Check intrinsics
	if(~isfield(calib,'K'))
		disp('ERROR: intrinsics not provided in CALIB.K')
		return;
	end
	K = double(calib.K); clear calib.K;
	if(size(K,1)~=3 | size(K,2)~= 3 | ndims(K)>2) 
		disp('ERROR: K should be 3 x 3');
		return;
	end
	if(K(1,3)==0)
		K = K';
	end
	if(K(1,1)==0 | K(2,2)==0 | K(1,3)==0 | K(2,3)==0 | K(3,3)~=1 )
		disp('ERROR: intrinsics matrix not in a standard format');
		return;
	end
	% Check intensities
	if(~isfield(calib,'Phi'))
		disp('WARNING: intensities not provided in CALIB.Phi, using default values')
		calib.Phi = ones(nimgs,nchannels);
	end
	Phi = double(calib.Phi); clear calib.Phi;
	if(size(Phi,1)~=nimgs | size(Phi,2)~=nchannels | ndims(Phi)>2)
		disp(sprintf('ERROR: Phi should be %d x %d',nimgs,nchannels));
		return;
	end
	% Check anisotropy
	if(~isfield(calib,'mu'))
		disp('WARNING: anisotropy not provided in CALIB.mu, using default values')
		calib.mu = zeros(nimgs,1);
	end
	mu = double(calib.mu); clear calib.mu;
	if(size(mu,1)~=nimgs | size(mu,2) >1 | ndims(mu) >2)
		disp(sprintf('ERROR: mu should be %d x 1',nimgs));
		return;
	end
	% Check orientations
	if(~isfield(calib,'Dir'))
		disp('WARNING: orientations not provided in CALIB.Dir, using default values')
		calib.Dir = [zeros(nimgs,1),zeros(nimgs,1),ones(nimgs,1)];
	end
	Dir = double(calib.Dir); clear calib.Dir;
	if(size(Dir,1)~=nimgs | size(Dir,2)~=3 | ndims(Dir) >2)
		disp(sprintf('ERROR: Dir should be %d x 3',nimgs));
		return;
	end
	%~ if(sum(sum(Dir.^2,2))~=nimgs)
		%~ disp('ERROR: each row of Dir should have unit-length');
		%~ return;
	%~ end
	clear calib

	%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	%%% Check PARAMS
	if(~exist('params','var')|isempty(params)) params=[]; end;
	% Check initial depth
	if(~isfield(params,'z0'))
		disp('WARNING: initial depth not provided in PARAMS.z0, using default values')
		params.z0 = 700*ones(nrows,ncols);
	end
	z0 = params.z0; clear params.z0;
	if(size(z0,1)~=nrows | size(z0,2) ~= ncols | ndims(z0) > 2)
		disp(sprintf('ERROR: z0 should be %d x %d',nrows,ncols));
	end
	% Check semi_calibrated
	if(~isfield(params,'semi_calibrated'))
		disp('WARNING: semi_calibrated parameter not provided in PARAMS.semi_calibrated, using default values')
		params.semi_calibrated = 0;
	end
	semi_calibrated = params.semi_calibrated; clear params.semi_calibrated;
	if(semi_calibrated ~= 1 & semi_calibrated ~= 0)
		disp('ERROR: semi_calibrated should be binary');
	end
	% Check estimator
	if(~isfield(params,'estimator'))
		disp('WARNING: estimator not provided in PARAMS.estimator, using default values')
		params.estimator = 'LS';
	end
	estimator = params.estimator; clear params.estimator;
	% Check estimator's parameter
	if(~isfield(params,'lambda'))
		params.lambda = 0;
	end
	lambda = params.lambda; clear params.lambda;
	if(~strcmp(estimator,'LS') & lambda == 0)
		disp('ERROR: a strictly positive estimator parameter must be set in PARAMS.lambda for robust M-estimation');
	end
	% Check fidelity's parameter
	if(~isfield(params,'zeta'))
		params.zeta = 0;
	end
	zeta = params.zeta; clear params.zeta;
	if(zeta < 0)
		disp('ERROR: a positive fidelity must be set in PARAMS.zeta');
	end	
	% Check self shadows
	if(~isfield(params,'self_shadows'))
		disp('WARNING: self_shadows parameter not provided in PARAMS.self_shadows, using default values')
		params.self_shadows = 1;
	end
	self_shadows = params.self_shadows; clear params.self_shadows;
	if(self_shadows ~= 1 & self_shadows ~= 0)
		disp('ERROR: self_shadows should be binary');
		return;
	end
	% Check used
	if(~isfield(params,'used'))
		disp('WARNING: used not provided in PARAMS.used, using all data')
		params.used = ones(size(I));
	end
	used = double(params.used); clear params.used; 

	% Check scales
	if(~isfield(params,'scales'))
		disp('WARNING: scales not provided in PARAMS.scales, using default values')
		params.scales = 1;
	end
	scales = params.scales; clear params.scales;
	if(scales<0)
		disp(sprintf('ERROR: scales should be positive'));
	end
	% Check max iterations
	if(~isfield(params,'maxit'))
		disp('WARNING: max number of iterations not set in PARAMS.maxit, using default values')
		params.maxit = 100;
	end
	maxit = params.maxit; clear params.maxit;
	if(maxit < 0)
		disp('ERROR: max number of iterations should be positive');
		return;
	end
	% Check tolerance
	if(~isfield(params,'tol'))
		disp('WARNING: tolerance not set in PARAMS.tol, using default values')
		params.tol = 1e-3;
	end
	tol = params.tol; clear params.tol;
	if(tol < 0)
		disp('ERROR: tolerance should be positive');
		return;
	end
	% Check tolerance normals
	if(~isfield(params,'tol_normals'))
		disp('WARNING: tolerance on normaps not set in PARAMS.tol_normals, using default values')
		params.tol_normals = 0.1;
	end
	tol_normals = params.tol_normals; clear params.tol_normals;
	if(tol_normals < 0)
		disp('ERROR: tol_normals should be positive');
		return;
	end	
	
	% Check ratio
	if(~isfield(params,'ratio'))
		disp('WARNING: downsampling factor not set in PARAMS.ratio, using default values')
		params.ratio = 1;
	end
	ratio = params.ratio; clear params.ratio;
	if(ratio < 0)
		disp('ERROR: ratio should be a positive integer');
		return;
	end
	if(ratio>1)
		K(1:2,:) = K(1:2,:)./ratio;
		mask = imresize(mask,1./ratio);
		z0 = imresize(z0,1./ratio);
		disp('rescaling images');
		if(size(I,4)>1)
			I_resized = imresize(I(:,:,:,1),1./ratio);
			used_resized = imresize(used(:,:,:,1),1./ratio);
			for im = 1:size(I,4)
				I_resized(:,:,:,im) = imresize(I(:,:,:,im),1./ratio); 
				used_resized(:,:,:,im) = imresize(used(:,:,:,im),1./ratio); 
			end
			I = I_resized;
			clear I_resized
			used = used_resized;
			clear used_resized
		else
			I_resized = imresize(I(:,:,1),1./ratio);
			used_resized = imresize(used(:,:,1),1./ratio);
			for im = 1:size(I,3)
				I_resized(:,:,im) = imresize(I(:,:,im),1./ratio); 
				used_resized(:,:,im) = imresize(used(:,:,im),1./ratio); 
			end
			I = I_resized;
			used = used_resized;
			clear I_resized
			clear used_resized
		end
		[nrows,ncols] = size(mask);
	end
	clear ratio
	% Check preconditioner
	if(~isfield(params,'precond'))
		disp('WARNING: preconditioner not provided in PARAMS.precond, using default values')
		params.precond = 'ichol';
	end
	precond = params.precond; clear params.precond;	
	
	% Check display
	if(~isfield(params,'display'))
		disp('WARNING: display parameter not provided in PARAMS.display, using default values')
		params.display = 0;
	end
	display = params.display; clear params.display;
	if(display ~= 1 & display ~= 0)
		disp('ERROR: display should be binary');
		return;
	end
	if(display)
		hfig1 = figure();
		hfig2 = figure();
		hfig3 = figure();
		hfig4 = figure();
	end
	% Check PCG tolerance
	if(~isfield(params,'tol_pcg'))
		disp('WARNING: PCG tolerance not set in PARAMS.tol_pcg, using default values')
		params.tol_pcg = 1e-6;
	end
	tol_pcg = params.tol_pcg; clear params.tol_pcg;
	if(tol_pcg < 0)
		disp('ERROR: PCG tolerance should be positive');
		return;
	end
	% Check PCG max iterations
	if(~isfield(params,'maxit_pcg'))
		disp('WARNING: max number of iterations for PCG not set in PARAMS.maxit_pcg, using default values')
		params.maxit_pcg = 25;
	end
	maxit_pcg = params.maxit_pcg; clear params.maxit_pcg;
	if(maxit_pcg < 0)
		disp('ERROR: max number of iterations for PCG should be positive');
		return;
	end
	clear params
	disp(' ');

	
	%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	%%% Prepare data
	% Intrinsics
	fx = K(1,1);
	fy = K(2,2);
	x0 = K(1,3);
	y0 = K(2,3);
	clear K
	
	% Estimator phi(x) and weight function w(x) = phi'(x)/x
	if(strcmp(estimator,'LS'))
		phi_fcn = @(x,lambda) 0.5*x.^2;	
		w_fcn = @(x,lambda) ones(size(x));
	elseif(strcmp(estimator,'Cauchy'))
		phi_fcn = @(x,lambda) 0.5*log(1+x.^2/lambda^2);	
	    w_fcn = @(x,lambda) 1./(lambda^2+x.^2);
	elseif(strcmp(estimator,'Lp'))
		thr_norm = 1e-2;
		phi_fcn = @(x,lambda) ((abs(x)).^lambda) ./ (lambda*(thr_norm^(lambda-2)));
		w_fcn = @(x,lambda) lambda.*(max(thr_norm,abs(x))).^(lambda-2) ./ (lambda*(thr_norm^(lambda-2))) ;  
	elseif(strcmp(estimator,'GM'))
		phi_fcn = @(x,lambda) 0.5*(lambda^2)*x.^2./(x.^2+lambda^2);
		w_fcn = @(x,lambda) lambda^4./((x.^2+lambda^2).^2);	
	elseif(strcmp(estimator,'Welsh'))
		phi_fcn = @(x,lambda) 0.5*lambda^2.*(1-exp(-x.^2./(lambda^2)));
		w_fcn = @(x,lambda) exp(-x.^2./(lambda^2));
	elseif(strcmp(estimator,'Tukey'))
		phi_fcn = @(x,lambda) (1/6)*(abs(x)<=lambda).*(1-(1-x.^2./(lambda^2)).^3).*lambda^2+(abs(x)<lambda).*lambda^2;
		w_fcn = @(x,lambda) (abs(x)<=lambda).*((1-x.^2./(lambda^2)).^2);
	end
	
	% Self shadow function psi(x) and derivative psi'(x)
	if(self_shadows)
		psi_fcn = @(x) max(x,0);		% \{.\}_+ operator in [1]
		chi_fcn = @(x) double(x>=0); 	
	else
		psi_fcn = @(x) x;   
		chi_fcn = @(x) 1*ones(size(x));	
	end
	% Normalize images to [0,1]
	max_I = max(I(:));
	I = I./max_I;
	if(nchannels>1)
		if(size(Phi,2)==1)
			Phi = repmat(Phi,[1 nchannels]);
		end
	end
	Phi = Phi./max_I;
	
	%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	% Store the initial intrinsics
	x0_ref = x0;
	y0_ref = y0;
	fx_ref = fx;
	fy_ref = fy;
	% Store the initial images and masks
	I_ref = I;
	mask_ref = mask;
	used_ref = used; 
	nrows_ref = size(I_ref,1);
	ncols_ref = size(I_ref,2);
	lambda_ref = lambda;
	% Store initials priors
	z0_ref = z0;
	% Store output
	z_final = z0;
	rho_final = zeros(nrows,ncols,nchannels);
	% Set the scales
	scales = 2.^(scales-1:-1:0);
	%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
	
	for ratio=scales
		% Scale inputs
		fx = fx_ref./ratio;
		fy = fy_ref./ratio;
		x0 = x0_ref./ratio;
		y0 = y0_ref./ratio;
		mask = imresize(mask_ref,1./ratio)==1;
		z0 = imresize(z0_ref,1./ratio);
		disp('rescaling images');
		if(size(I_ref,4)>1)
			I_resized = imresize(I_ref(:,:,:,1),1./ratio);
			used_resized = imresize(used_ref(:,:,:,1),1./ratio)>0;
			for im = 1:size(I_ref,4)
				I_resized(:,:,:,im) = imresize(I_ref(:,:,:,im),1./ratio); 
				used_resized(:,:,:,im) = imresize(used_ref(:,:,:,im),1./ratio); 
			end
			I = I_resized;
			clear I_resized
			used = used_resized;
			clear used_resized
		else
			I_resized = imresize(I_ref(:,:,1),1./ratio);
			used_resized = imresize(used_ref(:,:,1),1./ratio)>0;
			for im = 1:size(I_ref,3)
				I_resized(:,:,im) = imresize(I_ref(:,:,im),1./ratio); 
				used_resized(:,:,im) = imresize(used_ref(:,:,im),1./ratio); 
			end
			I = I_resized;
			used = used_resized;
			clear I_resized
			clear used_resized
		end
		[nrows,ncols] = size(mask);
		[uu,vv] = meshgrid(1:ncols,1:nrows);
		u_tilde = (uu - x0); 
		v_tilde = (vv - y0);
		clear uu vv x0 y0
		% Mask
		imask = find(mask>0);
		npix = length(imask);
		% Some useful change
		if(nchannels>1)
			I = permute(I,[1 2 4 3]); 
		end
		% Some useful variables
		px_rep = repmat(u_tilde(imask),[1 nimgs]);
		py_rep = repmat(v_tilde(imask),[1 nimgs]);
		% Vectorize images
		I = reshape(I,nrows*ncols,nimgs,nchannels);
		I = I(imask,:,:);
		% Vectorize binary mask of used data
		W_idx = reshape(used,nrows*ncols,nimgs,nchannels);
		W_idx = W_idx(imask,:,:);
		clear used
		
		% Scale parameters
		if(~strcmp(estimator,'Lp'))
			lambda = lambda_ref*median(abs(I(:)-median(I(:))));
		end
		
		%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
		%%% Initialize variables at current scale
		z = imresize(z_final,1./ratio);
		z(mask==0) = NaN;
		rho = imresize(rho_final,1./ratio);
		
		%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
		%%% Auxiliary variables at current scale		
		% Gradient operator on the mask
		G = make_gradient(mask);
		Dx =  G(1:2:end,:);
		Dy = G(2:2:end,:);
		
		Dx_rep = repmat(Dx,[nimgs 1]);
		Dy_rep = repmat(Dy,[nimgs 1]);
		
		clear G;
		z0 = log(z0(imask));
		z_tilde = log(z(imask));
		XYZ = cat(3,z.*u_tilde./fx,z.*v_tilde./fy,z);
		zx = Dx*z_tilde;
		zy = Dy*z_tilde;
		Nx = zeros(nrows,ncols);
		Ny = zeros(nrows,ncols);
		Nz = zeros(nrows,ncols);
		Nx(imask) = fx*zx;
		Ny(imask) = fy*zy;
		Nz(imask) = -u_tilde(imask).*zx-v_tilde(imask).*zy-1;
		dz = sqrt(Nx.^2+Ny.^2+Nz.^2);
		N = cat(3,Nx./dz,Ny./dz,Nz./dz);
		rho_tilde = reshape(bsxfun(@rdivide,rho,dz),nrows*ncols,nchannels);
		rho_tilde = rho_tilde(imask,:);
		tab_nrj = [];
		
		disp('==============================================================');
		disp(sprintf('Starting algorithm at scale %d with %d pixels, %d images, %d channels, lambda = %.6f',ratio,npix,nimgs,nchannels,lambda));
		disp(' ')
	
	
		%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
		%%% Initial energy
		shading_fcn = @(z,tz) (spdiags(reshape(fx*tz(:,:,1)-px_rep.*tz(:,:,3),npix*nimgs,1),0,npix*nimgs,npix*nimgs)*Dx_rep+spdiags(reshape(fy*tz(:,:,2)-py_rep.*tz(:,:,3),npix*nimgs,1),0,npix*nimgs,npix*nimgs)*Dy_rep)*z-reshape(tz(:,:,3),npix*nimgs,1);	% zeta in [1] (Eq. 4.6)
		r_fcn = @(rho,shadz,II,phi,W_idx) W_idx.*(reshape(rho*phi',[nimgs*npix 1]).*psi_fcn(shadz)-II); % residual
		J_fcn = @(rho,shadz,II,phi,W_idx) sum(phi_fcn(r_fcn(rho,shadz,II,phi,W_idx),lambda)); % energy
		[Tz,grad_Tz] = t_fcn(z_tilde,S,Dir,mu,u_tilde(imask)./fx,v_tilde(imask)./fy); % Compute t field (Eq. (3.14))
		psi = reshape(shading_fcn(z_tilde,Tz),npix,nimgs); % \{ \chi \}_+ in [1]
		energy = 0;
		for ch = 1:nchannels
			phich = Phi(:,ch);
			Ich = I(:,:,ch);
			Wch = W_idx(:,:,ch);
			energy = energy+J_fcn(rho_tilde(:,ch),psi(:),Ich(:),phich,Wch(:));
		end
		energy = energy./(npix*nimgs*nchannels);
		energy = energy + zeta*sum((z_tilde(:)-z0(:)).^2)/npix; 
		clear Ich psich
		disp(sprintf('== it. 0 - energy : %.20f',energy));
		disp(' ');
		tab_nrj(1) = energy;
		
		% Display Initial result
		if(display)
			figure(hfig1);
			surfl(-XYZ(:,:,1),-XYZ(:,:,2),-XYZ(:,:,3),[0 90]);
			shading flat;
			colormap gray
			view(-220,30);
			axis ij
			axis image
			title('Shape')

			figure(hfig2)
			semilogy(0:0,tab_nrj(1:1),'Linewidth',4)
			title('Energy')


			figure(hfig3)
			imagesc(rho./max(rho(:)))
			if(nchannels==1)
				colormap gray
				colorbar
			end
			axis image
			axis off
			title('Albedo')

			figure(hfig4)
			Ndisp = N;
			Ndisp(:,:,3) = -Ndisp(:,:,3);
			Ndisp = 0.5*(Ndisp+1);
			imagesc(Ndisp)
			axis image
			axis off
			title('Normals')
			drawnow
		end		

		for it = 1:maxit
			w = zeros(npix,nimgs,nchannels);
			chi = chi_fcn(psi);
			phi_chi = psi.*chi;

			
			% Pseudo-albedo update
			for ch = 1:nchannels
				Ich = I(:,:,ch);
				phich = Phi(:,ch);
				phi_chich = bsxfun(@times,psi.*chi,transpose(Phi(:,ch)));
				w(:,:,ch) = Wch.*w_fcn(reshape(r_fcn(rho_tilde(:,ch),psi(:),Ich(:),phich,Wch(:)),npix,nimgs),lambda);
				denom = (sum(w(:,:,ch).*(phi_chich).^2,2));
				idx_ok = find(denom>0);
				if(length(idx_ok>0))
					rho_tilde(idx_ok,ch) = (sum(w(idx_ok,:,ch).*(I(idx_ok,:,ch)).*phi_chich(idx_ok,:),2))./denom(idx_ok);
				end
			end

			% Log-depth update
			rho_rep = zeros(npix,nimgs*nchannels);
			for ch = 1:nchannels
				Ich = I(:,:,ch);
				Wch = W_idx(:,:,ch);
				phich = Phi(:,ch);
				w(:,:,ch) = Wch.*w_fcn(reshape(r_fcn(rho_tilde(:,ch),psi(:),Ich(:),phich,Wch(:)),npix,nimgs),lambda);
				rho_rep(:,(ch-1)*nimgs+1:ch*nimgs) = rho_tilde(:,ch)*transpose(phich);
			end
			D = repmat(chi,[1 nchannels]).*(rho_rep.^2).*reshape(w,npix,nimgs*nchannels);
				
			A = spdiags(reshape(fx*Tz(:,:,1)-px_rep.*Tz(:,:,3),npix*nimgs,1),0,npix*nimgs,npix*nimgs)*Dx_rep+spdiags(reshape(fy*Tz(:,:,2)-py_rep.*Tz(:,:,3),npix*nimgs,1),0,npix*nimgs,npix*nimgs)*Dy_rep;
			A = A+sparse(1:npix*nimgs,repmat(1:npix,[1 nimgs]),transpose((spdiags(reshape(fx*grad_Tz(:,:,1)-px_rep.*grad_Tz(:,:,3),npix*nimgs,1),0,npix*nimgs,npix*nimgs)*Dx_rep+spdiags(reshape(fy*grad_Tz(:,:,2)-py_rep.*grad_Tz(:,:,3),npix*nimgs,1),0,npix*nimgs,npix*nimgs)*Dy_rep)*z_tilde-reshape(grad_Tz(:,:,3),npix*nimgs,1)));
			A = repmat(A,[nchannels 1]);
			At = transpose(A);
			M = At*spdiags(D(:),0,nimgs*npix*nchannels,nimgs*npix*nchannels)*A;
			M = M/(nimgs*npix*nchannels);
			M = M+zeta*speye(npix)/npix;
			
			rhs = repmat(chi,[1 nchannels]).*(rho_rep).*(rho_rep.*(-repmat(psi,[1 nchannels]))+reshape(I,npix,nimgs*nchannels)).*reshape(w,npix,nimgs*nchannels);
			rhs = At*rhs(:)/(nimgs*npix*nchannels);
			rhs = rhs+zeta*(z0(:)-z_tilde(:))/npix;
			clear rho_rep
			
			% Recompute the preconditioner every 5 iterations
			if(mod(it,5)==1)
				disp('Preconditioner recomputed')
				disp(' ');
				if(strcmp(precond,'cmg'))
					precond_L = cmg_sdd(M);
					precond_R = [];
				elseif(strcmp(precond,'ichol'))
					precond_L = ichol(M);
					precond_R = precond_L';
				elseif(strcmp(precond,'gs'))
					D = spdiags(spdiags(M,0),0,size(M,1),size(M,2));
					L = tril(M);
					precond_L = (D+L)*inv(D)*(D+L');
					precond_R = [];							
				elseif(strcmp(precond,'jacobi'))
					precond_L = spdiags(1./spdiags(M,0),0,size(M,1),size(M,2));
					precond_R = [];
				else
					precond_L = [];
					precond_R = [];
				end
			end
			z_tilde = z_tilde + pcg(M,rhs,tol_pcg,maxit_pcg,precond_L,precond_R);

			zx = Dx*z_tilde;
			zy = Dy*z_tilde;
			Nx = zeros(nrows,ncols);
			Ny = zeros(nrows,ncols);
			Nz = zeros(nrows,ncols);
			Nx(imask) = fx*zx;
			Ny(imask) = fy*zy;
			Nz(imask) = -u_tilde(imask).*zx-v_tilde(imask).*zy-1;
			dz = sqrt(Nx.^2+Ny.^2+Nz.^2);
			N_old = N; % Store for convergence test
			N = cat(3,Nx./dz,Ny./dz,Nz./dz);
			z(imask) = exp(z_tilde);
			
			% Intensities update
			if(semi_calibrated)
				% Phi update
				for ch = 1:nchannels
					Ich = I(:,:,ch);
					Wch = W_idx(:,:,ch);
					w(:,:,ch) = Wch.*max(0,w_fcn(reshape(r_fcn(rho_tilde(:,ch),psi(:),Ich(:),phich,Wch(:)),npix,nimgs),lambda));
					rho_psi_chi = psi.*chi.*repmat(rho_tilde(:,ch),[1 nimgs]);
					Phi(:,ch) = transpose(((sum(w(:,:,ch).*I(:,:,ch).*rho_psi_chi,1)))./(sum(w(:,:,ch).*(rho_psi_chi).^2,1)));
				end
			end
			
			for ch = 1:nchannels
				rhoch = rho(:,:,ch);
				rhoch(imask) = rho_tilde(:,ch).*dz(imask);
				Ich = I(:,:,ch);
				max_val = median(rhoch(imask))+8*std(rhoch(imask));
				rhoch(rhoch>max_val) = max_val;
				rhoch(rhoch<0) = 0;
				rho(:,:,ch) = rhoch;
			end
			XYZ = cat(3,z.*u_tilde./fx,z.*v_tilde./fy,z);

			% Convergence test
			energy_new = 0;
			[Tz,grad_Tz] = t_fcn(z_tilde,S,Dir,mu,u_tilde(imask)./fx,v_tilde(imask)./fy);
			psi = reshape(shading_fcn(z_tilde,Tz),npix,nimgs);
			for ch = 1:nchannels
				Ich = I(:,:,ch);
				Wch = W_idx(:,:,ch);
				energy_new = energy_new+J_fcn(rho_tilde(:,ch),psi(:),Ich(:),Phi(:,ch),Wch(:));
			end
			energy_new = energy_new./(npix*nimgs*nchannels);
			relative_diff = abs(energy_new-energy)./energy_new;
			
			normal_diff = rad2deg(real(acos(sum(N_old.*N,3))));
			normal_residual = median(normal_diff(imask));
			
			diverged = energy_new>energy;
			
			disp(sprintf('== it. %d - energy : %.6f - energy relative diff : %.6f - normal median variation: %.2f',it,energy_new,relative_diff,normal_residual));
			energy = energy_new;
			tab_nrj(it+1) = energy;
			
			% Display current result
			if(display)
				figure(hfig1);
				surfl(-XYZ(:,:,1),-XYZ(:,:,2),-XYZ(:,:,3),[0 90]);
				shading flat;
				colormap gray
				view(-220,30);
				axis ij
				axis image
				title('Shape')

				figure(hfig2)
				semilogy(0:it,tab_nrj(1:it+1),'Linewidth',4)
				title('Energy')

				figure(hfig3)
				rho_disp = rho./max(rho(:));
				imagesc(uint8(255*rho_disp))
				if(nchannels==1)
					colormap gray
					colorbar
				end
				axis image
				axis off
				title('Albedo')

				figure(hfig4)
				Ndisp = N;
				Ndisp(:,:,3) = -Ndisp(:,:,3);
				Ndisp = 0.5*(Ndisp+1);
				imagesc(Ndisp)
				axis image
				axis off
				title('Normals')
							
				drawnow	
			end
			if((it>1) & (relative_diff<tol))
				disp('CONVERGENCE REACHED: ENERGY EVOLUTION IS LOW ENOUGH');
				break;
			end
			if((it>1) & (normal_residual<tol_normals))
				disp('CONVERGENCE REACHED: SHAPE EVOLUTION IS LOW ENOUGH');
				break;
			end
			if((it>1) & diverged)
				disp('STOPPED: DIVERGENT BEHAVIOR DETECTED');
				break;
			end
			if((it>1) & diverged)
				disp('STOPPED: DIVERGENT BEHAVIOR DETECTED');
				break;
			end
			if(it==maxit)
				disp('STOPPED: MAX NUMBER OF ITERATIONS REACHED');
				break;
			end
		end
		%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
		% Update final result by interpolation
		if(ratio>1)
			disp('upscaling for the next level in the pyramid...');
			z(mask==0) = NaN;
			z=inpaint_nans(z);
			z_final = imresize(z,[nrows_ref ncols_ref]);
			rho_final = imresize(rho,[nrows_ref ncols_ref]);
			end
	end
	if(display)
		close(hfig1);
		close(hfig2);
		close(hfig3);
		close(hfig4);
	end
	% Back-scale albedo for consistency with the original images
	Phi = Phi.*max_I;
	% Fill non-estimated values with NaNs
	NaN_mask = NaN*mask;
	NaN_mask(mask>0) = 1;
	XYZ = bsxfun(@times,NaN_mask,XYZ);
	N = bsxfun(@times,NaN_mask,N);
	rho = bsxfun(@times,NaN_mask,rho);
	
	disp(' ');
end	


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%	
	

% Functions for computing the gradient operator on non-rectangular domains
function [M,Dyp,Dym,Dxp,Dxm] = make_gradient(mask)

	% Compute forward (Dxp and Dyp) and backward (Dxm and Dym) operators
	[Dyp,Dym,Dxp,Dxm,Sup,Sum,Svp,Svm,Omega,index_matrix,imask] = gradient_operators(mask);
	[nrows,ncols] = size(mask);

	% When there is no bottom neighbor, replace by backward (or by 0 if no top)
	Dy = Dyp;
	no_bottom = find(~Omega(:,:,1));
	no_bottom = nonzeros(index_matrix(no_bottom));
	Dy(no_bottom,:) = Dym(no_bottom,:);

	% Same for the x direction (right / left)
	Dx = Dxp;
	no_right = find(~Omega(:,:,3));
	no_right = nonzeros(index_matrix(no_right));
	Dx(no_right,:) = Dxm(no_right,:);

	M = sparse([],[],[],2*size(Dx,1),size(Dx,2),2*length(imask));
	M(1:2:end-1,:) = Dx;
	M(2:2:end,:) = Dy;
end

function [Dup,Dum,Dvp,Dvm,Sup,Sum,Svp,Svm,Omega,index_matrix,imask] = gradient_operators(mask)

	[nrows,ncols] = size(mask);
	Omega_padded = padarray(mask,[1 1],0);

	% Pixels who have bottom neighbor in mask
	Omega(:,:,1) = mask.*Omega_padded(3:end,2:end-1);
	% Pixels who have top neighbor in mask
	Omega(:,:,2) = mask.*Omega_padded(1:end-2,2:end-1);
	% Pixels who have right neighbor in mask
	Omega(:,:,3) = mask.*Omega_padded(2:end-1,3:end);
	% Pixels who have left neighbor in mask
	Omega(:,:,4) = mask.*Omega_padded(2:end-1,1:end-2);
	

	imask = find(mask>0);
	index_matrix = zeros(nrows,ncols);
	index_matrix(imask) = 1:length(imask);

	% Dv matrix
	% When there is a neighbor on the right : forward differences
	idx_c = find(Omega(:,:,3)>0);
	[xc,yc] = ind2sub(size(mask),idx_c);
	indices_centre = index_matrix(idx_c);	
	indices_right = index_matrix(sub2ind(size(mask),xc,yc+1));
	indices_right = indices_right(:);
	II = indices_centre;
	JJ = indices_right;
	KK = ones(length(indices_centre),1);
	II = [II;indices_centre];
	JJ = [JJ;indices_centre];
	KK = [KK;-ones(length(indices_centre),1)];
	
	Dvp = sparse(II,JJ,KK,length(imask),length(imask));
	Svp = speye(length(imask));
	Svp = Svp(index_matrix(idx_c),:);

	% When there is a neighbor on the left : backward differences
	idx_c = find(Omega(:,:,4)>0);
	[xc,yc] = ind2sub(size(mask),idx_c);
	indices_centre = index_matrix(idx_c);	
	indices_right = index_matrix(sub2ind(size(mask),xc,yc-1));
	indices_right = indices_right(:);
	II = [indices_centre];
	JJ = [indices_right];
	KK = [-ones(length(indices_centre),1)];
	II = [II;indices_centre];
	JJ = [JJ;indices_centre];
	KK = [KK;ones(length(indices_centre),1)];
	
	Dvm = sparse(II,JJ,KK,length(imask),length(imask));
	Svm = speye(length(imask));
	Svm = Svm(index_matrix(idx_c),:);



	% Du matrix
	% When there is a neighbor on the bottom : forward differences
	idx_c = find(Omega(:,:,1)>0);
	[xc,yc] = ind2sub(size(mask),idx_c);
	indices_centre = index_matrix(idx_c);	
	indices_right = index_matrix(sub2ind(size(mask),xc+1,yc));
	indices_right = indices_right(:);
	II = indices_centre;
	JJ = indices_right;
	KK = ones(length(indices_centre),1);
	II = [II;indices_centre];
	JJ = [JJ;indices_centre];
	KK = [KK;-ones(length(indices_centre),1)];
	
	Dup = sparse(II,JJ,KK,length(imask),length(imask));
	Sup = speye(length(imask));
	Sup = Sup(index_matrix(idx_c),:);

	% When there is a neighbor on the top : backward differences
	idx_c = find(Omega(:,:,2)>0);
	[xc,yc] = ind2sub(size(mask),idx_c);
	indices_centre = index_matrix(idx_c);	
	indices_right = index_matrix(sub2ind(size(mask),xc-1,yc));
	indices_right = indices_right(:);
	II = [indices_centre];
	JJ = [indices_right];
	KK = [-ones(length(indices_centre),1)];
	II = [II;indices_centre];
	JJ = [JJ;indices_centre];
	KK = [KK;ones(length(indices_centre),1)];
	
	Dum = sparse(II,JJ,KK,length(imask),length(imask));
	Sum = speye(length(imask));
	Sum = Sum(index_matrix(idx_c),:);	

end

function [sortedA,sortIndex,sortIndex_orig] = sort_linear_index(A,sortDim,sortOrder)
%#SORT_LINEAR_INDEX   Just like SORT, but returns linear indices

  sizeA = size(A);  %# Get the matrix size
  if nargin < 2
    sortDim = find(sizeA > 1,1);  %# Define sortDim, if necessary
  end
  if nargin < 3
    sortOrder = 'ascend';  %# Define sortOrder, if necessary
  end
  [sortedA,sortIndex] = sort(A,sortDim,sortOrder);  %# Sort the matrix
  sortIndex_orig = sortIndex;
  [subIndex{1:numel(sizeA)}] = ...  %# Create a set of matrix subscripts
     ind2sub(sizeA,reshape(1:prod(sizeA),sizeA));
  subIndex{sortDim} = sortIndex;  %# Overwrite part of the subscripts with
                                  %#   the sort indices
  sortIndex = sub2ind(sizeA,subIndex{:});  %# Find the linear indices

end

function [T_field,grad_t] = t_fcn(z,Xs,Dir,mu,u_tilde,v_tilde) 
	npix = length(z);
	nimgs = size(Xs,1);
	
	%%% Current mesh
	exp_z = exp(z);
	XYZ = cat(2,exp_z.*u_tilde,exp_z.*v_tilde,exp_z);

	%%% T field
	T_field = zeros(npix,nimgs,3);
	a_field = zeros(npix,nimgs);
	if(nargout>1)
		da_field = zeros(npix,nimgs);
	end
	for i = 1:nimgs
		% Unit lighting field
		T_field(:,i,1) = Xs(i,1)-XYZ(:,1);
		T_field(:,i,2) = Xs(i,2)-XYZ(:,2);
		T_field(:,i,3) = Xs(i,3)-XYZ(:,3);
		normS_i = sqrt(T_field(:,i,1).^2+T_field(:,i,2).^2+T_field(:,i,3).^2);
		% Attenuation = anisotropy / squared distance
		scal_prod = -T_field(:,i,1).*Dir(i,1)-T_field(:,i,2).*Dir(i,2)-T_field(:,i,3).*Dir(i,3);
		a_field(:,i) = (scal_prod.^mu(i))./(normS_i.^(3+mu(i)));
		da_field(:,i) = (mu(i).*scal_prod.^(mu(i)-1).*(XYZ(:,1).*Dir(i,1)+XYZ(:,2).*Dir(i,2)+XYZ(:,3).*Dir(i,3)))./(normS_i.^(mu(i)+3))-(mu(i)+3).*(scal_prod.^mu(i)).*(-T_field(:,i,1).*XYZ(:,1)-T_field(:,i,2).*XYZ(:,2)-T_field(:,i,3).*XYZ(:,3))./(normS_i.^(mu(i)+5));
		% Final lighting field
		T_field(:,i,1) = T_field(:,i,1).*a_field(:,i);
		T_field(:,i,2) = T_field(:,i,2).*a_field(:,i);
		T_field(:,i,3) = T_field(:,i,3).*a_field(:,i);
	end
	if(nargout>1)
		grad_t = zeros(npix,nimgs,3);
		grad_t(:,:,1) = bsxfun(@times,-exp_z.*u_tilde,a_field+da_field)+bsxfun(@times,da_field,Xs(:,1)');		
		grad_t(:,:,2) = bsxfun(@times,-exp_z.*v_tilde,a_field+da_field)+bsxfun(@times,da_field,Xs(:,2)');	
		grad_t(:,:,3) = bsxfun(@times,-exp_z,a_field+da_field)+bsxfun(@times,da_field,Xs(:,3)');
	end
end
