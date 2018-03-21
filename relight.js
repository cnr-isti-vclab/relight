function LdrRti(canvas, _options) {
	canvas = $(canvas);
	if(!canvas) return null;
	this.canvas = canvas[0];

	this.options = Object.assign({}, _options);
	var glopt = { antialias: false, depth: false };
	this.gl = this.options.gl || this.canvas.getContext("webgl2", glopt) || 
			canvas.getContext("webgl", glopt) || canvas.getContext("experimental-webgl", glopt) ;
	if (!this.gl) return null;

	this.light = [0, 0, 1];
	this.lweights = [];
	this.waiting = 1;
	this.update = true;

	this.initGL();

	return this;
}

LdrRti.prototype = {

get: function(url, type, callback) {
	var r=new XMLHttpRequest();
	r.open('GET', url);
	if(type != 'xml')
		r.responseType = type;
	r.onload = function (e) {
		if (r.readyState === 4) {
			if (r.status === 200) {
				if(type == 'xml')
					callback(r.responseXML);
				else
					callback(r.response);
			} else {
				console.error(r.statusText);
			}
		}
	};
	r.send();
},

setUrl: function(url) {
	var t = this;
	t.url = url;
	t.ready = false;

	t.waiting = 1;
	t.get(url + '/info.json', 'json', function(d) { t.loadInfo(d); });


},

loadInfo: function(info) {
	var t = this;
	t.waiting--;
	t.type = info.type;
	t.colorspace = info.colorspace;
	t.nmaterials = info.materials.length;
	t.nplanes = info.nplanes;
	t.materials = info.materials;
	if(info.lights) {
		t.lights = new Float32Array(info.lights.length);
		for(var i = 0; i < t.lights.length; i++)
			t.lights[i] = info.lights[i];
	} 
	if(t.type == "rbf") {
		t.sigma = info.sigma;
		t.ndimensions = t.lights.length/3;
	}

	if(t.type == "bilinear") {
		t.resolution = info.resolution;
		t.ndimensions = t.resolution*t.resolution;
	}

	if(t.nmaterials > 1) {
		t.waiting++;
		t.segments = new Image;
		t.segments.src = t.url + '/segments.png';
		t.segments.onload = function() { t.loadSegments(); }
	}

	if(t.colorspace == 'mycc') {
		t.yccplanes = info.yccplanes;
		t.nplanes = t.yccplanes[0] + t.yccplanes[1] + t.yccplanes[2];
	} else
		t.yccplanes = [0, 0, 0];

	t.width = info.width;
	t.height = info.height;
	t.planes = [];
	t.scale = new Float32Array((t.nplanes+1)*t.nmaterials);
	t.bias = new Float32Array((t.nplanes+1)*t.nmaterials);

	for(var m = 0;  m < t.nmaterials; m++) {
		for(var p = 1; p < t.nplanes+1; p++) {
			t.scale[m*(t.nplanes+1) + p] = t.materials[m].scale[p-1];
			t.bias [m*(t.nplanes+1) + p] = t.materials[m].bias [p-1];
		}
	}

	if(t.colorspace == 'mrgb' || t.colorspace == 'mycc') {
		t.get(t.url + '/materials.bin', 'arraybuffer', function(d) { t.loadBasis(d); });
		t.waiting++;
	}

	t.njpegs = 0;
	while(t.njpegs*3 < t.nplanes)
		t.njpegs++;
	t.waiting += t.njpegs;

	for(var i = 0; i < t.njpegs; i++) {
		t.planes[i] = new Image;
		t.planes[i].src = t.url + '/plane_' + i + '.jpg';
		t.planes[i].onload = function(k) { return function() { t.loadPlane(k); } }(i);
	}
	t.loadProgram();
},

// p from 0 to nplanes,
basePixelOffset(m, p, x, y, k) {
	var t = this;
	return ((m*(t.nplanes+1) + p)*t.resolution*t.resolution + (x + y*t.resolution))*3 + k;
},

baseLightOffset(m, p, l, k) {
	var t = this;
	return ((m*(t.nplanes+1) + p)*t.ndimensions + l)*3 + k;
},


loadBasis: function(data) {
	var t = this;
	var tmp = new Uint8Array(data);

	//apply offset and scale of the basis
	t.basis = new Float32Array(tmp.length);
	for(var m = 0; m < t.nmaterials; m++) {
		for(var p = 0; p < t.nplanes+1; p++) {
			for(var c = 0; c < t.ndimensions; c++) {
				for(var k = 0; k < 3; k++) {
					var o = t.baseLightOffset(m, p, c, k);
					if(p == 0)
						t.basis[o] = tmp[o]/255; //range from 0 to 1.
					else
						t.basis[o] = ((tmp[o] - 127)/t.materials[m].range[p-1]);
				}
			}
		}
	}
	t.updateWaiting(-1);
},

loadSegments: function() {
	var gl = this.gl;
	var tex = gl.createTexture();
	gl.bindTexture(gl.TEXTURE_2D, tex);

	if(gl instanceof WebGL2RenderingContext)
		gl.texImage2D(gl.TEXTURE_2D, 0, gl.R8, gl.RED, gl.UNSIGNED_BYTE, this.segments);
	else
		gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGB, gl.RGB, gl.UNSIGNED_BYTE, this.segments);

//	gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
//	gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
	gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
	gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
	gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
	gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
	this.segments.tex = tex;
	this.updateWaiting(-1);
},

loadPlane: function(i) {
	var gl = this.gl;
	var tex = gl.createTexture();
	gl.bindTexture(gl.TEXTURE_2D, tex);

	gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGB, gl.RGB, gl.UNSIGNED_BYTE, this.planes[i]);

	gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
	gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
//	gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
//	gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
	gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
	gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
	this.planes[i].tex = tex;
	this.updateWaiting(-1);
},

initGL: function() {
	var gl = this.gl;
	gl.pixelStorei(gl.UNPACK_COLORSPACE_CONVERSION_WEBGL, gl.NONE);
	gl.clearColor(0.0, 0.0, 0.0, 1.0);
	gl.disable(gl.DEPTH_TEST);
	gl.clear(gl.COLOR_BUFFER_BIT);
	gl.viewport(0, 0, this.canvas.width, this.canvas.height);
},

computeLightWeights: function(lpos) {
	var t = this;
	if(t.waiting) return;
	switch(t.type) {
	case 'rbf':      t.computeLightWeightsRbf(lpos);  break;
	case 'bilinear': t.computeLightWeightsOcta(lpos); break;
	case 'ptm':      t.computeLightWeightsPtm(lpos);  break;
	case 'hsh':      t.computeLightWeightsHsh(lpos);  break;
	default: console.log("Unknown basis", t.type);
	}

	if(t.baseLocation) {
		if(t.colorspace != 'mrgb' && t.colorspace != 'mycc')
			t.gl.uniform1fv(t.baseLocation, t.lweights);
		else
			t.gl.uniform3fv(t.baseLocation, t.lweights);
	}
},

closestLight: function(lpos) {
	var t = this;
	var best = 0;
	var bestd = 1e20;
	for(var i = 0; i < t.lights.length; i += 3) {
		var dx = t.lights[i+0] - lpos[0];
		var dy = t.lights[i+1] - lpos[1];
		var dz = t.lights[i+2] - lpos[2];

		var d2 = dx*dx + dy*dy + dz*dz;
		if(d2 < bestd) {
			best = i;
			bestd = d2;
		}
	}
	return best/3;
},

computeLightWeightsPtm: function(v) {
	var t = this;
	var w = [1.0, v[0], v[1], v[0]*v[0], v[0]*v[1], v[1]*v[1]];


		t.lweights = new Float32Array(t.nplanes);
		for(var p = 0; p < w.length; p++)
		t.lweights[p] = w[p];
},

computeLightWeightsHsh: function(v) {
	var t = this;
	var M_PI = 3.1415;
	var phi = Math.atan2(v[1], v[0]);
	if (phi < 0.0)
		phi = 2.0 * M_PI + phi;
	var theta = Math.min(Math.acos(v[2]), M_PI / 2.0 - 0.5);

	var cosP = Math.cos(phi);
	var cosT = Math.cos(theta);
	var cosT2 = cosT * cosT;

	var w = new Float32Array(9);
	w[0] = 1.0 / Math.sqrt(2.0 * M_PI);

	w[1] = Math.sqrt(6.0 / M_PI) * (cosP * Math.sqrt(cosT-cosT2));
	w[2] = Math.sqrt(3.0 / (2.0 * M_PI)) * (-1.0 + 2.0*cosT);
	w[3] = Math.sqrt(6.0 / M_PI) * (Math.sqrt(cosT - cosT2) * Math.sin(phi));

	w[4] = Math.sqrt(30.0 / M_PI) * (Math.cos(2.0 * phi) * (-cosT + cosT2));
	w[5] = Math.sqrt(30.0 / M_PI) * (cosP*(-1.0 + 2.0 * cosT) * Math.sqrt(cosT - cosT2));
	w[6] = Math.sqrt(5.0 / (2.0 * M_PI)) * (1.0 - 6.0 * cosT + 6.0 * cosT2);
	w[7] = Math.sqrt(30.0 / M_PI) * ((-1.0 + 2.0 * cosT) * Math.sqrt(cosT - cosT2) * Math.sin(phi));
	w[8] = Math.sqrt(30.0 / M_PI) * ((-cosT + cosT2) * Math.sin(2.0*phi));

	t.lweights = w;
},

computeLightWeightsRbf: function(lpos) {
	var t = this;
	var nm = t.nmaterials;
	var np = t.nplanes;
	var radius = 1/(t.sigma*t.sigma);

	var weights = new Array(t.ndimensions);

	//compute rbf weights
	var totw = 0.0;
	for(var i = 0; i < weights.length; i++) {
		var dx = t.lights[i*3+0] - lpos[0];
		var dy = t.lights[i*3+1] - lpos[1];
		var dz = t.lights[i*3+2] - lpos[2];

		var d2 = dx*dx + dy*dy + dz*dz;
		var w = Math.exp(-radius * d2);

		weights[i] = [i, w];
		totw += w;
	}
	for(var i = 0; i < weights.length; i++)
		weights[i][1] /= totw;


	//pick only most significant and renormalize

	var count = 0;
	totw = 0.0;
	for(var i = 0; i < weights.length; i++)
		if(weights[i][1] > 0.001) {
			weights[count++] =  weights[i];
			totw += weights[i][1];
		}

	weights = weights.slice(0, count); 

	for(var i = 0; i < weights.length; i++)
		weights[i][1] /= totw;


	//now iterate basis:
	t.lweights = new Float32Array(nm * (np + 1) * 3);

	for(var m = 0; m < nm; m++) {
		for(var p = 0; p < np+1; p++) {
			for(var k = 0; k < 3; k++) {
				for(var l = 0; l < weights.length; l++) {
					var o = t.baseLightOffset(m, p, weights[l][0], k);
					t.lweights[3*(m*(np+1) + p) + k] += weights[l][1]*t.basis[o];
				}
			}
		}
	}
},

computeLightWeightsOcta: function(lpos) {
	var t = this;
	var nm = t.nmaterials;
	var np = t.nplanes;
	var s = Math.abs(lpos[0]) + Math.abs(lpos[1]) + Math.abs(lpos[2]);
//rotate 45 deg.
	var x = (lpos[0] + lpos[1])/s;
	var y = (lpos[1] - lpos[0])/s;
	x = (x + 1.0)/2.0;
	y = (y + 1.0)/2.0;
	x = x*(t.resolution - 1.0);
	y = y*(t.resolution - 1.0);

	var sx = Math.min(t.resolution-2, Math.max(0, Math.floor(x)));
	var sy = Math.min(t.resolution-2, Math.max(0, Math.floor(y)));
	var dx = x - sx;
	var dy = y - sy;

	var s00 = (1 - dx)*(1 - dy);
	var s10 =      dx *(1 - dy);
	var s01 = (1 - dx)* dy;
	var s11 =      dx * dy;

	t.lweights = new Float32Array(nm * (np + 1) * 3);


//TODO optimize away basePixel
	for(var m = 0; m < nm; m++) {
		for(var p = 0; p < np+1; p++) {
			for(var k = 0; k < 3; k++) {
				var o00 = t.basePixelOffset(m, p, sx, sy, k);
				t.lweights[3*(m*(np+1) + p) + k] += s00*t.basis[o00];

				var o10 = t.basePixelOffset(m, p, sx+1, sy, k);
				t.lweights[3*(m*(np+1) + p) + k] += s10*t.basis[o10];

				var o01 = t.basePixelOffset(m, p, sx, sy+1, k);
				t.lweights[3*(m*(np+1) + p) + k] += s01*t.basis[o01];

				var o11 = t.basePixelOffset(m, p, sx+1, sy+1, k);
				t.lweights[3*(m*(np+1) + p) + k] += s11*t.basis[o11];
			}
		}
	}

},

setLight: function(x, y, z) {
	var t = this;
	var r = Math.sqrt(x*x + y*y + z*z);
	t.light = [x/r, y/r, z/r];
	t.computeLightWeights(t.light);
	if(t.ready)
		this.redraw();
},

loadProgram: function() {

	var t = this;

	t.vertCode = `
uniform mat4 u_matrix;

attribute vec4 a_position;
varying vec2 v_texcoord;

void main() {
  gl_Position = u_matrix * a_position;
  v_texcoord.x = 0.5 + (a_position.x -0.5);
  v_texcoord.y = 0.5 + (a_position.y -0.5);
}
`;

	var mrgbCode = `
#ifdef GL_ES
precision highp float;
#endif

const int np1 = ` + (t.nplanes + 1) + `;
const int nj1 = ` + (t.njpegs + 1) + `;
const int nm = ` + t.nmaterials + `;
const int nmp1 = np1*nm;
uniform sampler2D planes[nj1];      //0 is segments
uniform vec3 base[nmp1];
uniform float bias[nmp1];
uniform float scale[nmp1];

varying vec2 v_texcoord;
void main(void) { 
	vec3 color = vec3(0);
`;

	if(t.nmaterials == 1) {
		mrgbCode += `

	color += base[0];
	for(int j = 1; j < nj1; j++) {
		vec4 c = texture2D(planes[j-1], v_texcoord);
		color += base[j*3-2]*(c.x - bias[j*3-2])*scale[j*3-2]; 
		color += base[j*3-1]*(c.y - bias[j*3-1])*scale[j*3-1]; 
		color += base[j*3-0]*(c.z - bias[j*3-0])*scale[j*3-0]; 
	};
`;

	} else {
		mrgbCode += `

	int mat = int(texture2D(planes[0], v_texcoord).x*256.0)/8;
	for(int m = 0; m < nm; m++)
		if(m == mat) {
			color += base[m*np1];
			for(int j = 1; j < nj1; j++) {
				vec4 c = texture2D(planes[j], v_texcoord);
				color += base[m*np1 + j*3-2]*(c.x - bias[m*np1 + j*3-2])*scale[m*np1 + j*3-2]; 
				color += base[m*np1 + j*3-1]*(c.y - bias[m*np1 + j*3-1])*scale[m*np1 + j*3-1]; 
				color += base[m*np1 + j*3-0]*(c.z - bias[m*np1 + j*3-0])*scale[m*np1 + j*3-0]; 
			}
		}`;
	}

	mrgbCode +=
`	/* gamma fix
	color.x *= color.x;
	color.y *= color.y;
	color.z *= color.z; */
	gl_FragColor = vec4(color, 1.0);
}`;


	var myccCode = `
#ifdef GL_ES
precision highp float;
#endif

const int ny0 = ` + (t.yccplanes[0]) + `;
const int ny1 = ` + (t.yccplanes[1]) + `;

const int np1 = ` + (t.nplanes + 1) + `;
const int nj1 = ` + (t.njpegs  + 1) + `;
const int nm  = ` + t.nmaterials + `;
const int nmp1 = np1*nm;
uniform sampler2D planes[nj1];      //0 is segments
uniform vec3  base[nmp1];
uniform float bias[nmp1];
uniform float scale[nmp1];


varying vec2 v_texcoord;
void main(void) { 
	vec3 color = vec3(0);
`;

	if(t.nmaterials == 1) {
		myccCode += `

	color += base[0];
	for(int j = 1; j < nj1; j++) {
		vec4 c = texture2D(planes[j-1], v_texcoord);

		if(j-1 < ny1) {
			color.x += base[j*3-2].x*(c.x - bias[j*3-2])*scale[j*3-2]; 
			color.y += base[j*3-1].y*(c.y - bias[j*3-1])*scale[j*3-1]; 
			color.z += base[j*3-0].z*(c.z - bias[j*3-0])*scale[j*3-0];
		} else {
			color.x += base[j*3-2].x*(c.x - bias[j*3-2])*scale[j*3-2]; 
			color.x += base[j*3-1].x*(c.y - bias[j*3-1])*scale[j*3-1]; 
			color.x += base[j*3-0].x*(c.z - bias[j*3-0])*scale[j*3-0]; 
		}
	};

	float tmp = color.r - color.b/2.0;
	vec3 rgb;
	rgb.g = color.b + tmp;
	rgb.b = tmp - color.g/2.0;
	rgb.r = rgb.b + color.g;
	color = rgb;
`;

	} else {
		myccCode += `

	int mat = int(texture2D(planes[0], v_texcoord).x*256.0)/8;
	for(int m = 0; m < nm; m++)
		if(m == mat) {
			color += base[m*np1];
			for(int j = 1; j < nj1; j++) {
				vec4 c = texture2D(planes[j], v_texcoord);
				color += base[m*np1 + j*3-2]*(c.x - bias[m*np1 + j*3-2])*scale[m*np1 + j*3-2]; 
				color += base[m*np1 + j*3-1]*(c.y - bias[m*np1 + j*3-1])*scale[m*np1 + j*3-1]; 
				color += base[m*np1 + j*3-0]*(c.z - bias[m*np1 + j*3-0])*scale[m*np1 + j*3-0]; 
			}
		}`;
	}

	myccCode +=
`	gl_FragColor = vec4(color, 1.0);
}`;


	var rgbCode = `
#ifdef GL_ES
precision highp float;
#endif

const int np1 = ` + (t.nplanes + 1) + `;
const int nj = ` + (t.njpegs) + `;
uniform sampler2D planes[nj];      //0 is segments
uniform float base[np1-3];
uniform float bias[np1];
uniform float scale[np1];

varying vec2 v_texcoord;

void main(void) {
	vec3 color = vec3(0);
	for(int j = 0; j < nj; j++) {
		vec4 c = texture2D(planes[j], v_texcoord);
		color.x += base[j]*(c.x - bias[j*3+1])*scale[j*3+1]; 
		color.y += base[j]*(c.y - bias[j*3+2])*scale[j*3+2]; 
		color.z += base[j]*(c.z - bias[j*3+3])*scale[j*3+3]; 
	}
	gl_FragColor = vec4(color, 1.0);
}`;


	var yccCode = `
#ifdef GL_ES
precision highp float;
#endif

const int np1 = ` + (t.nplanes + 1) + `;
const int nj = ` + (t.njpegs) + `;
uniform sampler2D planes[nj];      //0 is segments
uniform float base[np1-3];
uniform float bias[np1];
uniform float scale[np1];

varying vec2 v_texcoord;

vec3 toYcc(vec4 rgb) {
	vec3 c;
	c.x =       0.299   * rgb.x + 0.587   * rgb.y + 0.114   * rgb.z;
	c.y = 0.5 - 0.16874 * rgb.x - 0.33126 * rgb.y + 0.50000 * rgb.z;
	c.z = 0.5 + 0.50000 * rgb.x - 0.41869 * rgb.y - 0.08131 * rgb.z;
	return c;
}

vec3 toRgb(vec4 ycc) {
	ycc.y -= 0.5;
	ycc.z -= 0.5;
	vec3 c;
	c.x = ycc.x +                   1.402   *ycc.z;
	c.y = ycc.x + -0.344136*ycc.y - 0.714136*ycc.z; 
	c.z = ycc.x +  1.772   *ycc.y;
	return c;
}

void main(void) {
	vec3 color = vec3(0);
	for(int j = 0; j < nj; j++) {
		vec4 c = texture2D(planes[j], v_texcoord);
		color.x += base[j]*(c.x - bias[j*3+1])*scale[j*3+1];

		if(j == 0) {
			color.y = (c.y - bias[j*3+2])*scale[j*3+2];
			color.z = (c.z - bias[j*3+3])*scale[j*3+3]; 
		}
	}

	color = toRgb(vec4(color, 1.0));
	gl_FragColor = vec4(color, 1.0);
}`;



	var lrgbCode = `
#ifdef GL_ES
precision highp float;
#endif

const int np1 = ` + (t.nplanes+1) + `;
const int nj = ` + (t.njpegs) + `;
uniform sampler2D planes[nj];      //0 is segments
uniform float base[np1]; //lrgb ptm the weights starts from 0 (while bias and scale start from 3
uniform float bias[np1];
uniform float scale[np1];

varying vec2 v_texcoord;

void main(void) {
	vec4 color = texture2D(planes[0], v_texcoord);
	float l = 0.0;
	for(int j = 1; j < nj; j++) {
		vec4 c = texture2D(planes[j], v_texcoord);
		l += base[j*3-3]*(c.x - bias[j*3+1])*scale[j*3+1]; 
		l += base[j*3-2]*(c.y - bias[j*3+2])*scale[j*3+2]; 
		l += base[j*3-1]*(c.z - bias[j*3+3])*scale[j*3+3]; 
	}

	gl_FragColor = vec4(color.x*l, color.y*l, color.z*l, 1.0);
}`;


	switch(t.colorspace) {
	case 'ycc': t.fragCode = yccCode; break;
	case 'mycc':  t.fragCode = myccCode; break;
	case 'mrgb': t.fragCode = mrgbCode; break;
	case 'rgb':  t.fragCode = rgbCode; break;
	case 'lrgb': t.fragCode = lrgbCode; break;
	}

	var gl = this.gl;
	var vertShader = gl.createShader(gl.VERTEX_SHADER);
	gl.shaderSource(vertShader, this.vertCode);
	gl.compileShader(vertShader);
	console.log(gl.getShaderInfoLog(vertShader));

	var fragShader = gl.createShader(gl.FRAGMENT_SHADER);
	gl.shaderSource(fragShader, t.fragCode);
	gl.compileShader(fragShader);
	t.program = gl.createProgram();
	var compiled = gl.getShaderParameter(fragShader, gl.COMPILE_STATUS);
	if(!compiled) {
		console.log(t.fragCode);
		console.log(gl.getShaderInfoLog(fragShader));
	}

	gl.attachShader(this.program, vertShader);
	gl.attachShader(this.program, fragShader);
	gl.linkProgram(this.program);
	gl.useProgram(this.program);

//BUFFERS

	var vbuffer = gl.createBuffer();
	gl.bindBuffer(gl.ARRAY_BUFFER, vbuffer);
	gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([0, 0, 0,  0, 1, 0,  1, 1, 0,  1, 0, 0]), gl.STATIC_DRAW);

	var ibuffer = gl.createBuffer();
	gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, ibuffer);
	gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, new Uint16Array([3,2,1,3,1,0]), gl.STATIC_DRAW);

	var coord = gl.getAttribLocation(t.program, "a_position");
	gl.vertexAttribPointer(coord, 3, gl.FLOAT, false, 0, 0);
	gl.enableVertexAttribArray(coord);

	t.matrixLocation = gl.getUniformLocation(t.program, "u_matrix");
	t.baseLocation = gl.getUniformLocation(t.program, "base");
	t.planesLocations = gl.getUniformLocation(t.program, "planes");

	gl.uniform1fv(gl.getUniformLocation(t.program, "scale"), t.scale);
	gl.uniform1fv(gl.getUniformLocation(t.program, "bias"), t.bias);

	var sampler = gl.getUniformLocation(t.program, "planes");
	var samplerArray = new Int32Array(t.njpegs + (t.nmaterials > 1? 1: 0));
	var len = samplerArray.length;
	while (len--)
		samplerArray[len] = len;

	gl.uniform1iv(sampler, samplerArray);
},

//in case it's more efficient than using uniform
createBaseBuffer: function() {
	if(t.weighttex) return;
	//gl.RGB! gl.RGB32F or 16F?
	t.weightbuff = new Uint8Array((t.nplanes+1)*t.nmaterials*3);
	var gl = t.gl;
	var tex = gl.createTexture();
	gl.bindTexture(gl.TEXTURE_2D, tex);
	gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGB, t.nplanes+1, t.nmaterials, 0, gl.RGB, gl.UNSIGNED_BYTE, t.weightbuff);
	gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
	gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
	gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
	gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
	t.weighttex = tex;
},

updateWaiting: function(n) {
	this.waiting += n;
	if(this.waiting != 0)
		return;
	if(this.onLoad)
		this.onLoad(this);
},

draw: function(zoom) {
	if(!zoom) zoom = 1.0;
	var t = this;

//	console.log("Drawing", t);
	t.gl.clearColor(1.0, 0.0, 0.0, 1.0);
	t.gl.clear(t.gl.COLOR_BUFFER_BIT);

	if(t.waiting)
		return;

	var nside = t.width;

	var sx = 2*zoom;
	var sy = 2*zoom;
	var dx = -1;
	var dy = -1;
	var matrix = [sx, 0,  0,  0,  0, -sy,  0,  0,  0,  0,  1,  0,  dx,  -dy,  0,  1];

	var gl = this.gl;
	var texn = gl.TEXTURE0;
	if(t.nmaterials > 1) {
		gl.activeTexture(texn++);
		gl.bindTexture(gl.TEXTURE_2D, this.segments.tex);
	}

	for(var i = 0; i < t.njpegs; i++) {
		gl.activeTexture(texn++);
		gl.bindTexture(gl.TEXTURE_2D, this.planes[i].tex);
	}
	gl.uniformMatrix4fv(this.matrixLocation, false, matrix);
	gl.drawElements(gl.TRIANGLES, 6, gl.UNSIGNED_SHORT,0);
}, 

cleanup: function() {
	var t = this;
	for(var i = 0; i < t.planes.lenght; i++)
		gl.deleteTexture(t.planes[i].tex);
}

} //prototype


