/*

Position is given with x, y, z, t

where 
  x and y are the coords of the center of the screen in full scale image.
  z is the zoom level with 0 being the 1 tile level.
  t is for interpolation
*/

function MRti(canvas, o) {
	canvas = $(canvas);
	if(!canvas)
		return null;
	canvas = canvas[0];


	this.options = Object.assign({}, o);

	var glopt = { antialias: false, depth: false };
	this.gl = this.options.gl || this.canvas.getContext("webgl2", glopt) || 
			canvas.getContext("webgl", glopt) || canvas.getContext("experimental-webgl", glopt) ;
	if (!this.gl) return null;

	this.gl = gl;
	this.canvas = canvas;
	this.visible = true;
	this.light = [0, 0, 1];
	this.lweights = [];
	this.pos = { x: 0, y:0, z:0, t:0 };         //t is in the future.
	this.previous = { x:0, y:0, z:0, t:0 };

	this.update = true;
	this.nodes = [];

	this.cache = {};           //priority [index, level, x, y] //is really needed? probably not.
	this.queued = [];          //array of things to load
	this.requested = {};       //things being actually requested
	this.requestedCount = 0;
	this.maxPrefetched = o.maxPrefetched ? o.maxPrefetched : 4;

	this.previousbox = [1, 1, -1, -1];
	this.animaterequest = null;

	if(o.width)  canvas.width  = o.width;
	if(o.height) canvas.height = o.height;
	this.suffix = o.suffix? o.suffix : ".jpg";

	this.initGL();

	return this;
}

//coordinate system: we have origin 0,0 at the center of the image x goes from 0 to 1.(level z = 0). in the original size scale. Z = 0 highest level
//animation on this stuff is separated from position

MRti.prototype = {

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

	t.get(url + '/info.json', 'json', function(d) { t.loadInfo(d); });
},

loadInfo: function(info) {
	var t = this;
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

	if(t.colorspace == 'mycc') {
		t.yccplanes = info.yccplanes;
		t.nplanes = t.yccplanes[0] + t.yccplanes[1] + t.yccplanes[2];
	} else
		t.yccplanes = [0, 0, 0];

	t.width = parseInt(info.width);
	t.height = parseInt(info.height);
	t.tilesize = info.tilesize ? info.tilesize : 0;
	t.overlap = info.overlap ? info.overlap : 0;
	t.layout = info.layout ? info.layout : "image";

	t.planes = [];
	t.scale = new Float32Array((t.nplanes+1)*t.nmaterials);
	t.bias = new Float32Array((t.nplanes+1)*t.nmaterials);

	for(var m = 0;  m < t.nmaterials; m++) {
		for(var p = 1; p < t.nplanes+1; p++) {
			t.scale[m*(t.nplanes+1) + p] = t.materials[m].scale[p-1];
			t.bias [m*(t.nplanes+1) + p] = t.materials[m].bias [p-1];
		}
	}

	t.njpegs = 0;
	while(t.njpegs*3 < t.nplanes)
		t.njpegs++;

	t.initTree();
	t.loadProgram();

	function loaded() {
		if(t.onLoad)
			t.onLoad(t);
		t.prefetch();
		t.preload();
	}

	if(t.colorspace == 'mrgb' || t.colorspace == 'mycc') {
		t.get(t.url + '/materials.bin', 'arraybuffer', function(d) { t.loadBasis(d); loaded(); });
	} else {
		loaded();
	}
},

initTree: function() {

	var t = this;
	t.nodes = [];

	switch(t.layout) {
		case "image":
			t.resolutions = 1;
			t.qbbox = [0, 0, 1, 1];
			t.bbox = [0, 0, t.width, t.height];
			return;
			break;
		case "deepzoom":
			var max = Math.max(t.width, t.height)/t.tilesize;
			t.resolutions = Math.ceil(Math.log(max) / Math.LN2) + 1;
			t.suffix = ".jpeg";
			t.getTileURL = function (image, x, y, resolution) {
				var prefix = image.substr(0, image.lastIndexOf("."));
				var base = t.url + '/' + prefix + '_files/';
				return base + (resolution + 0) + '/' + x + '_' + y + t.suffix;
			};
			break;
		default:
			console.log("OOOPPpppps");
	}
	if(!t.tilesize) {
		console.log("TILESIZE!", t.tilesize);
		return;
	}

	t.qbox = [];
	t.bbox = [];
	var w = t.width;
	var h = t.height;
	for(var i = t.resolutions - 1; i >= 0; i--) {
		t.qbox[i] = [0, 0, 0, 0]; //TODO replace with correct formula
		t.bbox[i] = [0, 0, w, h];
		for(var y = 0; y*t.tilesize < h; y++) {
			t.qbox[i][3] = y+1;
			for(var x = 0; x*t.tilesize < w; x ++) {
				var index = t.index(i, x, y);
				t.nodes[index] = { tex: [], missing: t.njpegs };
				t.qbox[i][2] = x+1;
			}
		}
		w >>>= 1;
		h >>>= 1;
	}
	console.log(t.qbox);
	console.log(t.bbox);
},


resize: function(width, height) {
	this.canvas.width = width;
	this.canvas.height = height;
	gl.viewport(0, 0, width, height);
	this.redraw();
},

zoom: function(dz, dt) {
	var p = this.pos;
	this.setPosition(p.x, p.y, p.z+dz, dt);
},

pan: function(dx, dy, dt) { //dx and dy expressed as pixels in the current size!
	//remap pixels to [0,1] space.
	var p = this.pos;
	var scale = Math.pow(2, p.z);
	var side = this.tilesize*scale/2; //this is the side of the whole picture in pixels
	this.setPosition(p.x - dx/side, p.y - dy/side, p.z, dt);
},

setPosition: function(x, y, z, dt) {
	if(z >= this.levels) z = this.levels;
	if(z <= 0) z = 0;
	if(!dt) dt = 0;
	var t = performance.now();
	this.previous = this.pos;
	this.previous.t = t;
	this.pos = { x: x, y:y, z:z, t:t + dt };
	this.update = true;
	this.prefetch();
	this.redraw();
},

getCurrent: function(time) {
	var t = this;
	if(time > t.pos.t) 
		return t.pos;
	var dt = t.pos.t - t.previous.t;
	if(dt < 1) return t.pos;

	var dt = (t.pos.t - time)/(t.pos.t - t.previous.t); //how much is missing to pos
	var ft = 1 - dt;
	return { 
		x:t.pos.x*ft + t.previous.x*dt, 
		y:t.pos.y*ft + t.previous.y*dt, 
		z:t.pos.z*ft + t.previous.z*dt, 
		t:time };
},


initGL: function() {
	var gl = this.gl;
	gl.pixelStorei(gl.UNPACK_COLORSPACE_CONVERSION_WEBGL, gl.NONE);
	gl.clearColor(0.0, 0.0, 0.0, 1.0);
	gl.disable(gl.DEPTH_TEST);
	gl.clear(gl.COLOR_BUFFER_BIT);
	gl.viewport(0, 0, this.canvas.width, this.canvas.height);
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
},

index: function(resolution, x, y) {
	var startindex = ((1<<(resolution*2))-1)/3;
	var side = 1<<resolution;
	return startindex + y*side + x;
},

loadTile: function(resolution, x, y) {
	var t = this;
	var index = t.index(resolution, x, y);

	if(t.requested[index]) {
		console.log("AAARRGGHHH double request!");
		return;
	}
	t.requested[index] = true;
	t.requestedCount++;

	for(var p = 0; p < t.njpegs; p++)
		t.loadComponent(p, index, resolution, x, y);
},



loadComponent: function(plane, index, resolution, x, y) {
	var t = this;
	var gl = t.gl;
	var name = "plane_" + plane + ".jpg";

	//TODO use a cache of images to avoid memory allocation waste
	var image = new Image();
	image.crossOrigin = "Anonymous";
	image.src = t.getTileURL(name, resolution, x, y);
	image.addEventListener('load', function() {
		var tex = gl.createTexture();
		gl.bindTexture(gl.TEXTURE_2D, tex);
		gl.texParameterf(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
		gl.texParameterf(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
		gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGB, gl.RGB, gl.UNSIGNED_BYTE, image);

		t.nodes[index].tex[plane] = tex;
		t.nodes[index].missing--;
		if(t.nodes[index].missing == 0) {
			delete t.requested[index];
			t.requestedCount--;
			t.preload();
			t.redraw();
		}
	});
	image.onerror = function() {
		t.nodes[index].missing = -1;
		delete t.requested[index];
		t.requestedCount--;
		t.preload();
	}
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


draw: function(timestamp) {
	var t = this;
	t.animaterequest = null;

	t.gl.clearColor(0.0, 0.0, 0.0, 1.0);
	t.gl.clear(t.gl.COLOR_BUFFER_BIT);
	t.gl.enable(t.gl.SCISSOR_TEST);

	if(!t.visible)
		return;

	var pos = this.getCurrent(performance.now());

	//compute image bbox
	//we start from
	var mag = Math.pow(2, pos.z);
	var side = Math.pow(2, t.levels-1)*t.tilesize;
	var mx = t.img_width/side;
	var my = t.img_height/side;
	var w = Math.floor(mx*mag*t.tilesize);
	var h = Math.floor(my*mag*t.tilesize);
	var x = Math.ceil(t.canvas.width/2 - w/2 - pos.x*t.tilesize*mag/2) + 1;
	var y = Math.ceil(t.canvas.height/2 - h/2 + pos.y*t.tilesize*mag/2) + 1;
	t.gl.scissor(x, y, w-2, h-2);

	//compute position using current and pos

	var needed = this.neededBox(pos);
	var level = Math.floor(pos.z);
	var startindex = ((1<<(level*2))-1)/3;
	var side = Math.pow(2, level);
	var backup = {};
	for(var y = needed[1]; y < needed[3]; y++) {
		for(var x = needed[0]; x < needed[2]; x++) {
			var index = startindex + y*side + x;

			//TODO if parent is present let's these checks pass.
			if(this.nodes[index].missing == -1) {
				continue; //missing image
			}
			if(this.nodes[index].missing != 0) {
				continue;
			}
			var l = level;
			var s = side;
			var px =x;
			var py = y;
			//TODO: This doesn't work? why?
			while(l >= 1 && this.nodes[index].missing != 0) {
				l--;
				s /= 2;
				py = py>>1;
				px = px>>1;
				index = ((1<<(l*2))-1)/3 + py*s + px;
			}
			this.renderNode(index, pos, px, py, l);
		}
	}
	if(timestamp < this.pos.t)
		this.redraw();

	t.gl.disable(t.gl.SCISSOR_TEST);
}, 

redraw: function() {
	var t = this;
	if(t.animaterequest) return;
	t.animaterequest = requestAnimationFrame(function (time) { t.draw(time); });
},

renderNode: function(index, pos, x, y, level) {
	if(this.nodes[index].missing != 0) return;

	//we need to determine how big is a tile in ortho coordinates.
	//vert coord [0-1] needs to be scaled to 2*tilesize/canvas.width (ortho coords goes from [-1,1]). is
	var mag = Math.pow(2, pos.z - level);
	var nside = Math.pow(2, level);
	var sx = mag*2*this.tilesize/this.canvas.width;
	var sy = mag*2*this.tilesize/this.canvas.height;
	var dx = sx*((x - nside/2) - (nside/2)*pos.x);
	var dy = sy*((y - nside/2) - (nside/2)*pos.y);
	var matrix = [sx, 0,  0,  0,  0, -sy,  0,  0,  0,  0,  1,  0,  dx,  -dy,  0,  1];

	var gl = this.gl;
	for(var i = 0; i < this.ordlen; i++) {
		gl.activeTexture(gl.TEXTURE0 + i);
		gl.bindTexture(gl.TEXTURE_2D, this.nodes[index].tex[i]);
	}
	gl.uniformMatrix4fv(this.matrixLocation, false, matrix);
	gl.drawElements(gl.TRIANGLES, 6, gl.UNSIGNED_SHORT,0);
},

prefetch: function() {
	var t = this;
	if(!t.visible)
		return;
	var needed = t.neededBox(t.pos);
	var level = Math.floor(t.pos.z);
	//TODO check level also (unlikely, but let's be exact)
	if(t.previouslevel == level && needed[0] == t.previousbox[0] && needed[1] == t.previousbox[1] &&
		needed[2] == t.previousbox[2] && needed[3] == t.previousbox[3])
		return;

	t.previouslevel = level;
	t.previousbox = needed;
	t.queued = [];

	//look for needed nodes and prefetched nodes (on the pos destination

	for(var y = needed[1]; y <= needed[3]; y++) {
		for(var x = needed[0]; x <= needed[2]; x++) {
			var index = t.index(level, x, y);
			if(t.nodes[index].missing != 0 && !t.requested[index])
				t.queued.push([level, x, y]);
		}
	}
	t.preload();
},

preload: function() {
	while(this.requestedCount < this.maxPrefetched && this.queued.length > 0) {
		var tile = this.queued.shift();
		this.loadTile(tile[0], tile[1], tile[2]);
	}
},

neededBox: function(pos) {
	var t = this;
	var resolution = Math.floor(pos.z);
	//size of a rendering pixel in original image pixels.
	var scale = Math.pow(2, t.resolutions-1 - pos.z);
	//find coordinates in original size image
	var bbox = [
		pos.x - scale*canvas.width()/2,
		pos.y - scale*canvas.height()/2,
		pos.x + scale*canvas.width()/2,
		pos.y + scale*canvas.height()/2,
	];
	var side = t.tilesize*Math.pow(2, (t.resolutions-1) - resolution);
	//quantized bbox
	var qbox = [
		Math.floor((bbox[0])/side),
		Math.floor((bbox[1])/side),
		Math.floor((bbox[2]-1)/side) + 1,
		Math.floor((bbox[3]-1)/side) + 1];

	//clamp!
	qbox[0] = Math.max(qbox[0], t.qbox[resolution][0]);
	qbox[1] = Math.max(qbox[1], t.qbox[resolution][1]);
	qbox[2] = Math.min(qbox[2], t.qbox[resolution][2]);
	qbox[3] = Math.min(qbox[3], t.qbox[resolution][3]);
	return qbox;
}

};


