/*

Relight

Position is given with x, y, z, a, t

where 
  x and y are the coords of the center of the screen in full scale image. (0, 0 beingg in top left of image)
  z is the zoom level with 0 being the original image. (in the code level)
  a is the rotation angle (counterclockwise) in degrees
  t is for interpolation

Tiles follow deepzoom convention:

* the corner is top-left
* "ilevel" has 0 as the single tile level
* "level" has 0 as the original resolution in this code.

Tile size divides the original image.
Overlay enlarges the tiles but not the side on the border of the image (need to know when rendering!)


Options:

	gl: if provided will use an already created context
	url: path of the directory where the json (and the images) resides
	path: for iip absolute path of the same directory
	layout: pick between image, deepzoom, google, iip, iiif, zoomify
	visible: rendering active or not, (default: true)
	light: initial light (default [0, 0, 1]
	pos: initial view (default { x:0, y:0, z:0, t:0 })
	border: amount of prefetching around borders (default 1)
	maxRequested: max amount of requested tiles *tiles* (default 4)
	fit: scale and center the model (default true)

Methods:
*/

function Relight(gl, options) {
	var t = this;
	if(!gl)
		return null;
	t.gl = gl;

	t.options = Object.assign({
		visible: true,
		opacity: 1,
		layout: "image",

		suffix: ".jpg",
		position: [ 0, 0],
		scale: 1,
		rotation: 0,

		light: [0, 0, 1],
		normals: 0,

		border: 1,                   //prefetching tiles out of view
		mipmapbias: 0.5,
		maxRequested: 4

	}, options);

	t.pos = { x: 0, y:0, z: 0, a: 0 };
	t.normals = t.options.normals;

	if(t.url && t.url.endsWidth("/"))
		t.url = r.url.slice(0, -1);

	for(var i in t.options)
		t[i] = t.options[i];


	t.previousbox = [1, 1, -1, -1];

	t.nodes = [];
	t.lweights = [];
	t.cache = {};           //priority [index, level, x, y] //is really needed? probably not.
	t.queued = [];          //array of things to load
	t.requested = {};       //things being actually requested
	t.requestedCount = 0;
	t.animaterequest = null;
	t.waiting = 0;

//events
	t._onload = [];

	if(t.img) { //this meas we are loading an image
		t.loadInfo({type: 'img', colorspace: null, width: 0, height: 0, nplanes: 3 });
	} else if(t.dem) { //this meas we are loading an image
		t.loadInfo({type: 'dem', colorspace: null, width: 0, height: 0, nplanes: 3 });
	} else if(t.url !== null) {
		t.setUrl(t.url);
	}
	return this;
}

Relight.prototype = {

get: function(url, type, callback) {
	if(!url) throw "Missing url!";
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
	t.img = 'plane_0';

	t.waiting = 1;
	t.get(url + '/info.json', 'json', function(d) { t.waiting--; t.loadInfo(d); });
},

loadInfo: function(info) {
	var t = this;

	t.type = info.type;
	t.colorspace = info.colorspace;

	t.width = parseInt(info.width);
	t.height = parseInt(info.height);

	if(t.colorspace == 'mycc') {
		t.yccplanes = info.yccplanes;
		t.nplanes = t.yccplanes[0] + t.yccplanes[1] + t.yccplanes[2];
	} else {
		t.yccplanes = [0, 0, 0];
		t.nplanes = info.nplanes;
	}

	t.planes = [];
	t.njpegs = 0;
	while(t.njpegs*3 < t.nplanes)
		t.njpegs++;

	//TODO: is this the right position?
	if(t.type == 'img' || t.type == 'dem') {
		t.initTree();
		t.loadProgram();
		t.loaded();
		return;
	}

	t.nmaterials = info.materials.length;
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

//	t.tilesize = 'tilesize' in info ? info.tilesize : 0;
//	t.overlap  = 'overlap'  in info ? info.overlap  : 0;
//	t.layout   = 'layout'   in info ? info.layout   : "image";

	t.factor = new Float32Array((t.nplanes+1)*t.nmaterials);
	t.bias = new Float32Array((t.nplanes+1)*t.nmaterials);

	for(var m = 0;  m < t.nmaterials; m++) {
		for(var p = 1; p < t.nplanes+1; p++) {
			t.factor[m*(t.nplanes+1) + p] = t.materials[m].scale[p-1];
			t.bias [m*(t.nplanes+1) + p] = t.materials[m].bias [p-1];
		}
	}

	t.initTree();
	t.loadProgram();

	if(t.colorspace == 'mrgb' || t.colorspace == 'mycc') {
		if(info.basis) {
			t.loadBasis(info.basis);
		} else { //backward compatible with binary materials
			t.waiting++;
			t.get(t.url + '/materials.bin', 'arraybuffer', function(d) { t.waiting--; t.loadBasis(d); t.loaded(); });
		}
	}
	t.loaded();
},


onLoad: function(f) {
	this._onload.push(f);
},


loaded: function() {
	var t = this;
	if(t.waiting) return;


//	t.prefetch(); //TODO PROBLEM! thees need to be called everytime a node has been loaded (it will do by itself) or position has changed
	t._onload.forEach( (f) => { f(); });

	//this needs to know it's orientation.
	t.computeLightWeights(t.light);
},

initTree: function() {

	var t = this;

	if(t.imgCache) {
		for(var i = 0; i < t.imgCache.length; i++)
			t.imgCache[i].src = '';
	} else {
		t.imgCache = [];
		for(var i = 0; i < t.maxRequested*t.njpegs; i++) {
			var image = new Image();
			image.crossOrigin = "";
			t.imgCache[i] = image;
		}
	}
	t.currImgCache = 0;

	t.flush();
	t.nodes = [];

	switch(t.layout) {
		case "image":
			t.nlevels = 1;
			t.tilesize = 0;
			t.qbox = [[0, 0, 1, 1]];
			t.bbox = [[0, 0, t.width, t.height]];

			t.getTileURL = function(image, x, y, level) {
				if(t.url)
					return t.url + '/' + image;
				else
					return image;
			};
			t.nodes[0] = { tex: [], missing:t.njpegs };
			return;

		case "google":
			t.tilesize = 256;
			t.overlap = 0;
			var max = Math.max(t.width, t.height)/t.tilesize;
			t.nlevels = Math.ceil(Math.log(max) / Math.LN2) + 1;

			t.getTileURL = function(image, x, y, level) {
				var prefix = image.substr(0, image.lastIndexOf("."));
				var base = t.url + '/' + prefix;
				var ilevel = parseInt(t.nlevels - 1 - level);
				return base + "/" + ilevel + "/" + y + "/" + x + t.suffix;
			};
			break;

		case "deepzoom":
			t.metaDataURL = t.url + "/" + t.img + ".dzi";
			t.getTileURL = function (image, x, y, level) {
				var prefix = image.substr(0, image.lastIndexOf("."));
				var base = t.url + '/' + prefix + '_files/';
				var ilevel = parseInt(t.nlevels - 1 - level);
				return base + ilevel + '/' + x + '_' + y + t.suffix;
			};

			t.parseMetaData = function (response) {
				t.suffix = "." + /Format="(\w+)/.exec(response)[1];
				t.tilesize = parseInt(/TileSize="(\d+)/.exec(response)[1]);
				t.overlap = parseInt(/Overlap="(\d+)/.exec(response)[1]);

				if(!t.width) t.width = parseInt(/Width="(\d+)/.exec(response)[1]);
				if(!t.height) t.height = parseInt(/Height="(\d+)/.exec(response)[1]);

				var max = Math.max(t.width, t.height)/t.tilesize;
				t.nlevels = Math.ceil(Math.log(max) / Math.LN2) + 1;
			};

			break;

		case "zoomify":
			t.overlap = 0; //overlap is not specified!
			t.metaDataURL = t.url + "/" + t.img + "/ImageProperties.xml";
			t.getTileURL = function(image, x, y, level) {
				var prefix = image.substr(0, image.lastIndexOf("."));
				var base = t.url + '/' + prefix;
				var ilevel = parseInt(t.nlevels - 1 - level);
				var index = t.index(level, x, y)>>>0;
				var group = index >> 8;
				return base + "/TileGroup" + group + "/" + ilevel + "-" + x + "-" + y + t.suffix;
			};

			t.parseMetaData = function(response) {
				var tmp = response.split('"');
				t.tilesize = parseInt(tmp[11]);

				var max = Math.max(t.width, t.height)/t.tilesize;
				t.nlevels = Math.ceil(Math.log(max) / Math.LN2) + 1;
			}
			break;


		case "iip":
			t.suffix = ".tif";
			t.overlap = 0;
			t.metaDataURL = t.server + "?FIF=" + t.path + "/" + t.img + t.suffix + "&obj=IIP,1.0&obj=Max-size&obj=Tile-size&obj=Resolution-number";

			t.parseMetaData = function(response) {
				var tmp = response.split( "Tile-size:" );
				if(!tmp[1]) return null;
				t.tilesize = parseInt(tmp[1].split(" ")[0]);
				tmp = response.split( "Max-size:" );
				if(!tmp[1]) return null;
				tmp = tmp[1].split('\n')[0].split(' ');
				t.width = parseInt(tmp[0]);
				t.height= parseInt(tmp[1]);
				t.nlevels  = parseInt(response.split( "Resolution-number:" )[1]);
			}

			t.getTileURL = function(image, x, y, level) {
				var prefix = image.substr(0, image.lastIndexOf("."));
				var img = t.path + "/" + prefix + t.suffix;
				var index = y*t.qbox[level][2] + x;
				var ilevel = parseInt(t.nlevels - 1 - level);
				return t.server+"?FIF=" + img + "&JTL=" + ilevel + "," + index;
			};
			break;

		case "iiif":
			t.metaDataURL = t.server + "?IIIF=" + t.path + "/" + t.img + "/info.json";
			t.parseMetaData = function(response) {
				var info = JSON.parse(response);
				t.width = info.width;
				t.height = info.height;
				t.nlevels = info.tiles[0].scaleFactors.length;
				t.tilesize = info.tiles[0].width;
			}
			t.getTileURL = function(image, x, y, level) {
				let tw = t.tilesize;
				let ilevel = parseInt(t.nlevels - 1 - level);
				let s = Math.pow(2, level);
				//region parameters
				let xr = x * tw * s;
				let yr = y * tw * s;
				let wr = Math.min(tw * s, t.width - xr)
				let hr = Math.min(tw * s, t.height - yr);

				// pixel size parameters /ws,hs/
				let ws = tw
				if (xr + tw*s > t.width)
					ws = (t.width - xr + s - 1) / s  
				let hs = tw
				if (yr + tw*s > t.height)
					hs = (t.height - yr + s - 1) / s

				return t.server + `?IIIF=${t.path}/${t.img}/${xr},${yr},${wr},${hr}/${ws},${hs}/0/default.jpg`;
			};
			break;
		default:
			console.log("OOOPPpppps");
	}

	if(t.metaDataURL) {
		t.waiting++;
		t.get(t.metaDataURL, 'text', function(r) { t.waiting--; t.parseMetaData(r); initBoxes(); t.loaded(); });
	} else 
		initBoxes();

	function initBoxes() {
		t.qbox = []; //by level (0 is the bottom)
		t.bbox = [];
		var w = t.width;
		var h = t.height;
		var count = 0;
		for(var level = t.nlevels - 1; level >= 0; level--) {
			var ilevel = t.nlevels -1 - level;
			t.qbox[ilevel] = [0, 0, 0, 0];
			t.bbox[ilevel] = [0, 0, w, h];
			for(var y = 0; y*t.tilesize < h; y++) {
				t.qbox[ilevel][3] = y+1;
				for(var x = 0; x*t.tilesize < w; x ++) {
					t.nodes[count++] = { tex: [], missing: t.njpegs };
					t.qbox[ilevel][2] = x+1;
				}
			}
			w >>>= 1;
			h >>>= 1;
		}
	}
},

rot: function(dx, dy, a) {
	var a = Math.PI*(a/180);
	var x =  Math.cos(a)*dx + Math.sin(a)*dy;
	var y = -Math.sin(a)*dx + Math.cos(a)*dy;
	return [x, y];
},


//convert image coords to canvas coords
project: function(pos, x, y) {
	var t = this;
	var z = Math.pow(2, pos.z);

	var r = t.rot(x - t.width/2,  y - t.height/2, pos.a);
	r[0] = (r[0] - pos.x)/z;
	r[1] = (r[1] - pos.y)/z;

	return r;
},

//TODO fully support rect
iproject: function(pos, x, y) {
	var t = this;
	var z = Math.pow(2, pos.z);
	var r = t.rot(x*z + pos.x, y*z + pos.y, pos.a);
	r[0] += t.width/2;
	r[1] += t.height/2;
	return r;
},

//return the box of the image in the canvas coords (remember the center at 0,0)
getBox: function(pos) {
	var t = this;
	var corners = [0, 0,  0, 1,  1, 1,  1, 0];
	var box = [ 1e20, 1e20, -1e20, -1e20];
	for(var i = 0; i < 8; i+= 2) {
		var p = t.project(pos, corners[i]*t.width, corners[i+1]*t.height);
		box[0] = Math.min(p[0], box[0]);
		box[1] = Math.min(p[1], box[1]);
		box[2] = Math.max(p[0], box[2]);
		box[3] = Math.max(p[1], box[3]);
	}
	return box;
},

//sreturn the coordinates of the canvas in image space
getIBox: function(pos) {
	var t = this;
	var corners = [-0.5, -0.5,  -0.5, 0.5,  0.5, 0.5,  0.5, -0.5];
	var box = [ 1e20, 1e20, -1e20, -1e20];
	for(var i = 0; i < 8; i+= 2) {
		var p = t.iproject(pos, corners[i]*t.canvas.width, corners[i+1]*t.canvas.height);
		box[0] = Math.min(p[0], box[0]);
		box[1] = Math.min(p[1], box[1]);
		box[2] = Math.max(p[0], box[2]);
		box[3] = Math.max(p[1], box[3]);
	}
	return box;
},

// p from 0 to nplan
basePixelOffset: function(m, p, x, y, k) {
	var t = this;
	return ((m*(t.nplanes+1) + p)*t.resolution*t.resolution + (x + y*t.resolution))*3 + k;
},

baseLightOffset: function(m, p, l, k) {
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

index: function(level, x, y) {
	var t = this;
	var startindex = 0;
	for(var i = t.nlevels-1; i > level; i--) {
		startindex += t.qbox[i][2]*t.qbox[i][3];
	}
	return startindex + y*t.qbox[level][2] + x;

/*	var startindex = ((1<<(ilevel*2))-1)/3;
	var side = 1<<ilevel;
	return startindex + y*side + x; */
},

loadTile: function(level, x, y) {
	var t = this;
	var index = t.index(level, x, y);

	if(t.requested[index]) {
		console.log("AAARRGGHHH double request!");
		return;
	}
	t.requested[index] = true;
	t.requestedCount++;

	for(var p = 0; p < t.njpegs; p++) 
		t.loadComponent(p, index, level, x, y);
},

loadComponent: function(plane, index, level, x, y) {
	var t = this;
	var gl = t.gl;
	if(t.type == 'img')
		var name = t.img;
	else if(t.type == 'dem')
		var name = t.dem;
	else
		var name = "plane_" + plane + ".jpg";

	//TODO use a cache of images to avoid memory allocation waste
	var image = t.imgCache[t.currImgCache++]; //new Image();
	if(t.currImgCache >= t.imgCache.length)
		t.currImgCache = 0;
//	image.crossOrigin = "Anonymous";
	image.src = t.getTileURL(name, x, y, level);
//removeEventListener
//	image.addEventListener('load', function() {
	image.onload = function() {
		var tex = gl.createTexture();
		gl.bindTexture(gl.TEXTURE_2D, tex);
		gl.texParameterf(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
		gl.texParameterf(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR); //_MIPMAP_LINEAR);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
		gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGB, gl.RGB, gl.UNSIGNED_BYTE, image);
//		gl.generateMipmap(gl.TEXTURE_2D);

		t.nodes[index].tex[plane] = tex;
		t.nodes[index].missing--;
		if(t.nodes[index].missing == 0) {
			delete t.requested[index];
			t.requestedCount--;
			t.preload();
			t.redraw();
		}
		if(t.layout == 'image') {
			t.width = image.width;
			t.height = image.height;
			t.loaded();
		}
	};
	image.onerror = function() {
		t.nodes[index].missing = -1;
		delete t.requested[index];
		t.requestedCount--;
		t.preload();
	}
},

flush: function() {
	var t = this;
	if(!t.nodes) return;
	for(var i = 0; i < t.nodes.length; i++) {
		var node = t.nodes[i];
		//abort calls TODO
		for(var j = 0; j < node.tex.length; j++)
			t.gl.deleteTexture(node.tex[j]);
	}
	//clean up cache events
	t.previouslevel = null;
	t.previousbox = [1, 1, -1, -1];
	for(var i = 0; i < t.imgCache.length; i++) {
		var img = t.imgCache[i];
		img.onload = null;
		img.onerror = null;
	}
	t.requested = {};
	t.requestedCount = 0;
},

setNormals: function(on) {
	var t = this;
	if(on === undefined)
		t.normals = (t.normals + 1)%3;
	else
		t.normals = Number(on);
	t.loadProgram();
	t.computeLightWeights(t.light);
	t.redraw();
},

computeLightWeights: function(lpos) {
	var t = this;
	var l = t.rot(lpos[0], lpos[1], -t.pos.a);
	l[2] = lpos[2];

	if(t.waiting) return;

	var lightFun;
	switch(t.type) {
	case 'img':                                     return;
	case 'dem':      t.gl.uniform3f(t.lightLocation, l[0], l[1], l[2]); return;
	case 'rbf':      lightFun = t.computeLightWeightsRbf;  break;
	case 'bilinear': lightFun = t.computeLightWeightsOcta; break;
	case 'ptm':      lightFun = t.computeLightWeightsPtm;  break;
	case 'hsh':      lightFun = t.computeLightWeightsHsh;  break;
	default: console.log("Unknown basis", t.type);
	}
	lightFun.call(this, l);

	var uniformer = (t.colorspace == 'mrgb' || t.colorspace == 'mycc') ? t.gl.uniform3fv : t.gl.uniform1fv;

	t.gl.useProgram(t.program);

	if(t.baseLocation0) {
		lightFun.call(this, [0.612,  0.354, 0.707]);
		uniformer.call(t.gl, t.baseLocation0, t.lweights);
	}

	if(t.baseLocation1) {
		lightFun.call(this, [-0.612,  0.354, 0.707]);
		uniformer.call(t.gl, t.baseLocation1, t.lweights);
	}

	if(t.baseLocation2) {
		lightFun.call(this, [     0, -0.707, 0.707]);
		uniformer.call(t.gl, t.baseLocation2, t.lweights);
	}


	if(t.baseLocation) {
		uniformer.call(t.gl, t.baseLocation, t.lweights);
	}

	if(t.lightLocation) {
		t.gl.uniform3fv(t.lightLocation, l);
	}
},

computeLightWeightsPtm: function(v) {
	var t = this;
	var w = [1.0, v[0], v[1], v[0]*v[0], v[0]*v[1], v[1]*v[1], 0, 0, 0];

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
	this.redraw();
},

loadProgram: function() {
	var t = this;
	t.setupShaders();

	var gl = t.gl;
	t.vertShader = gl.createShader(gl.VERTEX_SHADER);
	gl.shaderSource(t.vertShader, t.vertCode);
	gl.compileShader(t.vertShader);
	compiled = gl.getShaderParameter(t.vertShader, gl.COMPILE_STATUS);
	if(!compiled) {
		alert("Failed vertex shader compilation: see console log and ask for support.");
		console.log(t.vertShader);
		console.log(gl.getShaderInfoLog(t.vertShader));
	}

	t.fragShader = gl.createShader(gl.FRAGMENT_SHADER);
	gl.shaderSource(t.fragShader, t.fragCode);
	gl.compileShader(t.fragShader);

	compiled = gl.getShaderParameter(t.fragShader, gl.COMPILE_STATUS);
	if(!compiled) {
		alert("Failed fragment shader compilation: see console log and ask for support.");
		console.log(t.fragCode);
		console.log(gl.getShaderInfoLog(t.fragShader));
	}

	t.program = gl.createProgram();
	gl.attachShader(t.program, t.vertShader);
	gl.attachShader(t.program, t.fragShader);
	gl.linkProgram(t.program);
	gl.useProgram(t.program);

	if(t.colorspace) {
		//used for normal viewing.
		t.baseLocation0 = gl.getUniformLocation(t.program, "base0");
		t.baseLocation1 = gl.getUniformLocation(t.program, "base1");
		t.baseLocation2 = gl.getUniformLocation(t.program, "base2");

		t.baseLocation = gl.getUniformLocation(t.program, "base");
		t.planesLocations = gl.getUniformLocation(t.program, "planes");

		gl.uniform1fv(gl.getUniformLocation(t.program, "scale"), t.factor);
		gl.uniform1fv(gl.getUniformLocation(t.program, "bias"), t.bias);
	}

//BUFFERS

	t.ibuffer = gl.createBuffer();
	gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, t.ibuffer);
	gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, new Uint16Array([3,2,1,3,1,0]), gl.STATIC_DRAW);

	t.vbuffer = gl.createBuffer();
	gl.bindBuffer(gl.ARRAY_BUFFER, t.vbuffer);
	gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([0, 0, 0,  0, 1, 0,  1, 1, 0,  1, 0, 0]), gl.STATIC_DRAW);

	t.coordattrib = gl.getAttribLocation(t.program, "a_position");
	gl.vertexAttribPointer(t.coordattrib, 3, gl.FLOAT, false, 0, 0);
	gl.enableVertexAttribArray(t.coordattrib);

	t.tbuffer = gl.createBuffer();
	gl.bindBuffer(gl.ARRAY_BUFFER, t.tbuffer);
	gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([0, 0,  0, 1,  1, 1,  1, 0]), gl.STATIC_DRAW);

	t.texattrib = gl.getAttribLocation(t.program, "a_texcoord");
	gl.vertexAttribPointer(t.texattrib, 2, gl.FLOAT, false, 0, 0);
	gl.enableVertexAttribArray(t.texattrib);

	t.lightLocation = gl.getUniformLocation(t.program, "light");
	t.opacitylocation = gl.getUniformLocation(t.program, "opacity");

//	t.matrixLocation = gl.getUniformLocation(t.program, "u_matrix");
  var sampler = gl.getUniformLocation(t.program, "planes");
	var samplerArray = new Int32Array(t.njpegs);
	var len = samplerArray.length;
	while (len--)
		samplerArray[len] = len;

	gl.uniform1iv(sampler, samplerArray);
},

//minlevel is the level of the tiles at the current zoom, level is the (level, x, y) is the tile to be rendered

drawNode: function(pos, minlevel, level, x, y) {
	var t = this;
	var index = t.index(level, x, y);

	//TODO if parent is present let's these checks pass.
	if(this.nodes[index].missing != 0)
		return; //missing image

	//compute coords of the corners
	var z = Math.pow(2, pos.z);
	var a = Math.PI*pos.a/180;
	var c = Math.cos(a);
	var s = Math.sin(a);

//TODO use a static buffer
	var coords = new Float32Array([0, 0, 0,  0, 1, 0,   1, 1, 0,   1,  0, 0]);
	var tcoords = new Float32Array([0, 0,   0, 1,    1, 1,    1, 0]);


	var sx = 2.0/t.canvas.width;
	var sy = 2.0/t.canvas.height;

	if(t.layout == "image") {
		for(var i = 0; i < coords.length; i += 3) {
			var r = t.rot(coords[i]*t.width - t.width/2, -coords[i+1]*t.height + t.height/2, pos.a);
			coords[i]   = (r[0] - pos.x)*sx/z;
			coords[i+1] = (r[1] + pos.y)*sy/z;

		}

	} else {
		var side = tilesizeinimgspace = t.tilesize*(1<<(level));

		var tx = side;
		var ty = side;
		if(t.layout != "google") { //google does not clip images.
			if(side*(x+1) > t.width) {
				tx = (t.width  - side*x);
			}
			if(side*(y+1) > t.height) {
				ty = (t.height - side*y);
			} //in imagespace
		}

		var over = t.overlap;
		var lx  = t.qbox[level][2]-1;
		var ly  = t.qbox[level][3]-1;

		if(over) {
			var dtx = over / (tx/(1<<level) + (x==0?0:over) + (x==lx?0:over));
			var dty = over / (ty/(1<<level) + (y==0?0:over) + (y==ly?0:over));

			tcoords[0] = tcoords[2] = (x==0? 0: dtx);
			tcoords[1] = tcoords[7] = (y==0? 0: dty);

			tcoords[4] = tcoords[6] = (x==lx? 1: 1 - dtx);
			tcoords[3] = tcoords[5] = (y==ly? 1: 1 - dty);
		}

		for(var i = 0; i < coords.length; i+=3) {
			var r = t.rot(coords[i]*tx + side*x - t.width/2,  -coords[i+1]*ty - side*y + t.height/2, pos.a);
			coords[i]   = (r[0] - pos.x)*sx/z;
			coords[i+1] = (r[1] + pos.y)*sy/z;
		}
	}

//0.0 is in the center of the screen, 
	var gl = t.gl;
	//TODO join buffers, and just make one call per draw! (except the bufferData, which is per node)
	gl.bindBuffer(gl.ARRAY_BUFFER, t.vbuffer);
	gl.bufferData(gl.ARRAY_BUFFER, coords, gl.STATIC_DRAW);

	gl.vertexAttribPointer(t.coordattrib, 3, gl.FLOAT, false, 0, 0);
	gl.enableVertexAttribArray(t.coordattrib);

	gl.bindBuffer(gl.ARRAY_BUFFER, t.tbuffer);
	gl.bufferData(gl.ARRAY_BUFFER, tcoords, gl.STATIC_DRAW);

	gl.vertexAttribPointer(t.texattrib, 2, gl.FLOAT, false, 0, 0);
	gl.enableVertexAttribArray(t.texattrib);

	for(var i = 0; i < t.njpegs; i++) {
		gl.activeTexture(gl.TEXTURE0 + i);
		gl.bindTexture(gl.TEXTURE_2D, t.nodes[index].tex[i]);
	}
	gl.drawElements(gl.TRIANGLES, 6, gl.UNSIGNED_SHORT,0);
},

draw: function(pos) {
	var t = this;
	var gl = t.gl;

	if(t.waiting || !t.visible)
		return;

	var needed = t.neededBox(pos, 0);

	var minlevel = needed.level; //this is the minimum level;
	var ilevel = t.nlevels - 1 - minlevel;
	//size of a rendering pixel in original image pixels.
	var scale = Math.pow(2, pos.z);

	//find coordinates of the image in the canvas
	var box = [
		t.canvas.width/2 - pos.x,
		t.canvas.height/2 - (t.height/scale - pos.y),
		t.width/scale,
		t.height/scale
	];
	if(t.layout == "google") {
		box[0] += 1; box[1] += 1; box[2] -= 2; box[3] -= 2;
	}
	//gl.enable(gl.SCISSOR_TEST);
	//gl.scissor(box[0], box[1], box[2], box[3]);

	gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);
	gl.enable(gl.BLEND);

	gl.useProgram(t.program);
	gl.uniform1f(t.opacitylocation, t.opacity);

	var torender = {}; //array of minlevel, actual level, x, y (referred to minlevel)
	var brothers = {};
	var box = needed.box[minlevel];
	for(var y = box[1]; y < box[3]; y++) {
		for(var x = box[0]; x < box[2]; x++) {
			var level = minlevel;
			while(level < t.nlevels) {
				var d = level -minlevel;
				var index = t.index(level, x>>d, y>>d);
				if(t.nodes[index].missing == 0) {
					torender[index] = [level, x>>d, y>>d];
					break;
				} else {
					var sx = (x>>(d+1))<<1;
					var sy = (y>>(d+1))<<1;
					brothers[t.index(level, sx, sy)] = 1;
					brothers[t.index(level, sx+1, sy)] = 1;
					brothers[t.index(level, sx+1, sy+1)] = 1;
					brothers[t.index(level, sx, sy+1)] = 1;
				}
				level++;
			}
		}
	}

	for(var index in torender) {
		var id = torender[index];
		if(t.opacity != 1.0 && brothers[index]) continue;

		var level = id[0];
		var x = id[1];
		var y = id[2];
		t.drawNode(pos, minlevel, level, x, y);
	}

	//gl.disable(gl.SCISSOR_TEST);
	gl.disable(gl.BLEND);
}, 

redraw: function() {}, //placeholder for other to replace the draw code.

prefetch: function() {
	var t = this;
	if(t.waiting || !t.visible)
		return;
	var needed = t.neededBox(t.pos, t.border);
	var minlevel = needed.level;

	//TODO check level also (unlikely, but let's be exact)
	var box = needed.box[minlevel];
	if(t.previouslevel == minlevel && box[0] == t.previousbox[0] && box[1] == t.previousbox[1] &&
		box[2] == t.previousbox[2] && box[3] == t.previousbox[3])
		return;

	t.previouslevel = minlevel;
	t.previousbox = box;
	t.queued = [];

	//look for needed nodes and prefetched nodes (on the pos destination
	for(var level = t.nlevels-1; level >= minlevel; level--) {
		var box = needed.box[level];
		var tmp = [];
		for(var y = box[1]; y < box[3]; y++) {
			for(var x = box[0]; x < box[2]; x++) {
				var index = t.index(level, x, y);
				if(t.nodes[index].missing != 0 && !t.requested[index])
					tmp.push({level:level, x:x, y:y});
			}
		}
		var cx = (box[0] + box[2]-1)/2;
		var cy = (box[1] + box[3]-1)/2;
		tmp.sort(function(a, b) { return Math.abs(a.x - cx) + Math.abs(a.y - cy) - Math.abs(b.x - cx) - Math.abs(b.y - cy); });
		t.queued = t.queued.concat(tmp);
	}
	//sort queued by level and distance from center
	t.preload();
},

preload: function() {
	while(this.requestedCount < this.maxRequested && this.queued.length > 0) {
		var tile = this.queued.shift();
		this.loadTile(tile.level, tile.x, tile.y);
	}
},

neededBox: function(pos, border, canvas) {
	var t = this;
	if(t.layout == "image") {
		return { level:0, box: [[0, 0, 1, 1]] };
	}
	var w = this.canvas.width;
	var h = this.canvas.height;
	var minlevel = Math.max(0, Math.min(Math.floor(pos.z + t.mipmapbias), t.nlevels-1));

	//size of a rendering pixel in original image pixels.
	var scale = Math.pow(2, pos.z);
	var box = [];
	for(var level = t.nlevels-1; level >= minlevel; level--) {
		//find coordinates in original size image
//		var bbox = [
//			pos.x - scale*w/2,
//			pos.y - scale*h/2,
//			pos.x + scale*w/2,
//			pos.y + scale*h/2
//		];
		var bbox = t.getIBox(pos); //thats the reverse.
		var side = t.tilesize*Math.pow(2, level);
		//quantized bbox
		var qbox = [
			Math.floor((bbox[0])/side),
			Math.floor((bbox[1])/side),
			Math.floor((bbox[2]-1)/side) + 1,
			Math.floor((bbox[3]-1)/side) + 1];
	
		//clamp!
		qbox[0] = Math.max(qbox[0]-border, t.qbox[level][0]);
		qbox[1] = Math.max(qbox[1]-border, t.qbox[level][1]);
		qbox[2] = Math.min(qbox[2]+border, t.qbox[level][2]);
		qbox[3] = Math.min(qbox[3]+border, t.qbox[level][3]);
		box[level] = qbox;
	}
	return { level:minlevel, box: box };
}

};


