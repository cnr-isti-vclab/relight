/* accepts the same options as Relight, but if a layers options is present
each one of the layers becomes a Relight, otherwise just create one layer 

Canvas variables:
item,
gl
pos,
fit,
width, height (of the canvas, if more than one layer its the bounding box.

Layer: 
id, 
label, 
Relight ones.
position
rotation
scale


Coordinate system:
We deal with 3 coordinate systems:

1) the screen coordinates: the origin is in the center of the screen.
2) the canvas (virtual) coordinates: the origin is in the center.
   pos defined by:
    x, y: che position of the centter of the canvas in screen coordinates
    z: the log2 scal
    a: the rotation (degrees).
    to know the screen coords of a point p:
    rot(p*z, a) + (x, y);

3) the layer coordinates: the origin is again in the center
	rot(p*z, a) + (x, y) (and the composed.

A canvas is defined like an image: with, height.
  x and y are the coords of the center
  z is the zoom level with 0 being canvas width and height
  a is the rotation angle (counterclockwise) in degrees

The transform is: translate first to the center, then scale and rotate.

The position of the layers in the canvas is defined in the same way: translate then scale and rotate. default is 0.0.0. */

RelightCanvas = function(item, options) {
	var t = this;
	t.options = Object.assign({
		pos: { x: 0, y:0, z:0, a: 0, t:0 },
		background: [0, 0, 0, 0],
		bounded: true,
		zbounded: true,
		maxzoom: -1,
		minzoom: 100,
		border: 1,                   //prefetching tiles out of view
		maxRequested: 4,
		fit: true,                   //scale on load.
		preserveDrawingBuffer: false,
	}, options);

	for(var i in t.options)
		t[i] = t.options[i];

	if(!item)
		return null;

	if(typeof(item) == 'string')
		item = document.querySelector(item);
	if(item.tagName != "CANVAS")
		return null;
	t.canvas = item;

	var glopt = { antialias: false, depth: false, preserveDrawingBuffer: t.options.preserveDrawingBuffer };
	t.gl = options.gl || t.canvas.getContext("webgl2", glopt) || 
			t.canvas.getContext("webgl", glopt) || 
			t.canvas.getContext("experimental-webgl", glopt) ;
	if (!t.gl) return null;


	if(t.options.rotation)
		t.pos.a = t.options.rotation;
	t.previous = { x:0, y:0, z:0, a:0, t:0 };

	if(t.options.layers) {
		t.layers = t.options.layers.map( (layer) => {
			return new Relight(t.gl, layer);
		});
	} else {
		t.layers = [ new Relight(t.gl, t.options) ];
	}
	
	//TODO not properly elegant.
	t.layers.forEach((layer) => { 
		layer.canvas = t.canvas;
		layer.onLoad(() => { t.ready() });
		layer.redraw = function() { t.redraw(); };
	});
	t.initGL();

	t._onready = [];
	t._onposchange = [];
	t._onlightchange = [];
}


RelightCanvas.prototype = {

initGL: function() {
	var gl = this.gl;
	gl.pixelStorei(gl.UNPACK_COLORSPACE_CONVERSION_WEBGL, gl.NONE);
	var b = this.options.background;
	gl.clearColor(b[0], b[1], b[2], b[3], b[4]);
	gl.disable(gl.DEPTH_TEST);
	gl.clear(gl.COLOR_BUFFER_BIT);
	gl.viewport(0, 0, this.canvas.width, this.canvas.height);
},

redraw: function() {
	var t = this;
	if(t.animaterequest) return;
	t.animaterequest = requestAnimationFrame(function (time) { t.draw(time); });
},

rot: function(dx, dy, a) {
	var a = Math.PI*(a/180);
	var x =  Math.cos(a)*dx + Math.sin(a)*dy;
	var y = -Math.sin(a)*dx + Math.cos(a)*dy;
	return [x, y];
},

//computes the position of the layer starting from the canvas pos + relative layer pos.
project: function(layer, pos) {
	var lz = Math.pow(2, layer.scale);
	var z = Math.pow(2, pos.z);
	var p = this.rot(layer.position[0], layer.position[1], -pos.a);
	var lpos = {
		x: pos.x*lz - p[0],
		y: pos.y*lz - p[1],
		z: pos.z + layer.scale, 
		a: pos.a + layer.rotation
	};
	return lpos;
},

//return the current bounding box in canvas coordinates (but takes scale into account.
boundingBox: function() {
	var t = this;
	var box = [1e20, 1e20, -1e20, -1e20];
	t.layers.forEach((layer) => {

		var pos = t.project(layer, t.pos); 
		var b = layer.getBox(pos);
		box[0] = Math.min(b[0], box[0]);
		box[1] = Math.min(b[1], box[1]);
		box[2] = Math.max(b[2], box[2]);
		box[3] = Math.max(b[3], box[3]);

	});
	return box;
},


draw: function(timestamp) {
	var t = this;
	var gl = t.gl;
	t.animaterequest = null;

	t.gl.viewport(0, 0, t.canvas.width, t.canvas.height);
	var b = this.options.background;
	gl.clearColor(b[0], b[1], b[2], b[3], b[4]);
	gl.clear(gl.COLOR_BUFFER_BIT);


	var pos = t.getCurrent(performance.now());

	t.layers.forEach((layer) =>  {
		var lpos = t.project(layer, pos);
		layer.draw(lpos); 
	});

	if(timestamp < t.pos.t)
		t.redraw();
},

prefetch: function() {
	this.layers.forEach((layer) => { layer.prefetch(); });
},

ready: function() {
	var t = this;
	//wait for all layers headers
	for(var i = 0; i < t.layers.length; i++)
		if(t.layers[i].waiting) return;

	//we are looking for width and height at 0 zoom, but zoom could be currently different
	var z = t.pos.z;
	t.pos.z = 0;
	var box = t.boundingBox();
	t.pos.z = z;
	t.width = (box[2] - box[0]);
	t.height = (box[3] - box[1]);

	if(t.fit)
		t.centerAndScale();
	else
		t.layers.forEach((layer) => { layer.pos = t.project(layer, t.pos); }); //needed for light recomputation.

	for(var i = 0; i < t._onready.length; i++)
		t._onready[i]();

	t.layers.forEach((layer) => { layer.prefetch(); });
	t.redraw();
},

onReady: function(f) {
	this._onready.push(f);
},

onPosChange: function(f) {
	this._onposchange.push(f);
},

onLightChange: function(f) {
	this._onlightchange.push(f);
},

resize: function(width, height) {
	this.canvas.width = width;
	this.canvas.height = height;

	this.layers.forEach((layer) => { layer.prefetch(); });
	this.redraw();
},

zoom: function(dz, dt) {
	var p = this.pos;
	this.setPosition(dt, p.x, p.y, p.z+dz, p.a);
},

center: function(dt) {
	var p = this.pos;
	this.setPosition(dt, this.width/2, this.height/2, p.z, p.a);
},

centerAndScale: function(dt) {
	var t = this;
	var box = t.boundingBox();
	var zoom = Math.pow(2, t.pos.z);
	var scale = Math.max(zoom*(box[2]-box[0])/t.canvas.width, zoom*(box[3]-box[1])/t.canvas.height);
	var z = Math.log(scale)/Math.LN2;
	t.setPosition(dt, (box[2] + box[0])/2, (box[3] + box[1])/2, z, t.pos.a);
},

pan: function(dt, dx, dy) { //dx and dy expressed as pixels in the current size!
	var p = this.pos;
	//size of a rendering pixel in original image pixels.
	this.setPosition(dt, p.x - dx, p.y - dy, p.z, p.a);
},

rotate: function(dt, angle) {
	var p = this.pos;
	var a = p.a + angle;
	while(a > 360) a -= 360;
	while(a <   0) a += 360;
	this.setPosition(dt, p.x, p.y, p.z, a);
},

setPosition: function(dt, x, y, z, a) {

	var t = this;


	if(t.zbounded && t.width) {
		var zx = Math.log(t.width/t.canvas.width)/Math.log(2.0);
		var zy = Math.log(t.height/t.canvas.height)/Math.log(2.0);
		var maxz = Math.max(zx, zy);
		if(z > maxz) z = maxz;
		if(z <= t.maxzoom) z = t.maxzoom;
		if(z >= t.minzoom) z = t.minzoom;
	}
	if(t.bounded && t.width) {
		var scale = Math.pow(2, z);
		var boundx = Math.abs((t.width - scale*t.canvas.width)/2);
		x = Math.max(-boundx, Math.min(boundx, x));
		var boundy = Math.abs((t.height - scale*t.canvas.height)/2);
		y = Math.max(-boundy, Math.min(boundy, y));
	}

	if(!dt) dt = 0;
	var time = performance.now();
	t.previous = t.getCurrent(time);

	if(x == t.previous.x && y == t.previous.y && z == t.previous.z && a == t.previous.a)
		return;

	t.pos = { x:x, y:y, z:z, a:a, t:time + dt };
	t.layers.forEach((layer) => {
		layer.pos = t.project(layer, t.pos); 
		if(a != t.previous.a)
			layer.computeLightWeights(layer.light);
	});

	t.prefetch();
	t.redraw();

	for(var i = 0; i < t._onposchange.length; i++)
		t._onposchange[i]();
},

setLight: function(x, y, z) {
	this.layers.forEach((layer) => { layer.setLight(x, y, z); });
	this._onlightchange.forEach((f) => { f(); });
},

setNormals: function(on) {
	this.layers.forEach((layer) => { layer.setNormals(on); });
	this.redraw();
},

getCurrent: function(time) {
	var t = this;

	if(!t.pos.t || time > t.pos.t)
		return { x: t.pos.x, y: t.pos.y, z: t.pos.z, a: t.pos.a, t: time };

	var dt = t.pos.t - t.previous.t;
	if(dt < 1) return t.pos;

	var dt = (t.pos.t - time)/(t.pos.t - t.previous.t); //how much is missing to pos
	var ft = 1 - dt;

	var z = t.pos.z*ft + t.previous.z*dt;
	var x = t.pos.x*ft + t.previous.x*dt;
	var y = t.pos.y*ft + t.previous.y*dt;

	return { 
		x:x, 
		y:y, 
		z:z, 
		a:t.pos.a*ft + t.previous.a*dt,
		t:time };
}

};

