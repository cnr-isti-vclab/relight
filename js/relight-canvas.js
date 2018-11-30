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
A canvas is defined like an image: with, height.
  x and y are the coords of the center of the screen the canvas. (0, 0 beingg in top left of image)
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
		border: 1,                   //prefetching tiles out of view
		maxRequested: 4,
		fit: true,                   //scale on load.
		preserveDrawingBuffer: false,
	}, options);

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


	

	t.pos = t.options.pos;

	if(t.options.rotation)
		t.pos.a = t.options.rotation;
	t.previous = { x:0, y:0, z:0, t:0 };

	if(t.options.layers) {
		t.layers = t.options.layers.map( (layer) => {
			return new Relight(t.gl, layer);
		});
	} else {
		t.layers = [new Relight(t.gl, t.options)];
	}
	t.initGL();

	t._onposchange = [];
	t._onlightchange = [];
}

RelightCanvas.prototype = Relight.prototype;


RelightCanvas.prototype.initGL = function() {
	var gl = this.gl;
	gl.pixelStorei(gl.UNPACK_COLORSPACE_CONVERSION_WEBGL, gl.NONE);
	var b = this.options.background;
	gl.clearColor(b[0], b[1], b[2], b[3], b[4]);
	gl.disable(gl.DEPTH_TEST);
	gl.clear(gl.COLOR_BUFFER_BIT);
	gl.viewport(0, 0, this.canvas.width, this.canvas.height);
};

RelightCanvas.prototype.redraw = function() {
	var t = this;
	if(t.animaterequest) return;
	t.animaterequest = requestAnimationFrame(function (time) { t.draw(time); });
};

RelightCanvas.prototype.onPosChange = function(f) {
	this._onposchange.push(f);
};

RelightCanvas.prototype.onLightChange = function(f) {
	this._onlightchange.push(f);
};

RelightCanvas.prototype.resize = function(width, height) {
	this.canvas.width = width;
	this.canvas.height = height;
	this.prefetch();
	this.redraw();
};

RelightCanvas.prototype.project = function(pos, x, y) { //convert image coords to canvas coords.
	var t = this;
	var r = t.rot(x - pos.x,  y - pos.y, pos.a);
	var z = Math.pow(2, pos.z);
	return [r[0]/z + t.canvas.width/2, r[1]/z + t.canvas.height/2]
},

RelightCanvas.prototype.iproject = function(pos, x, y) {
	var t = this;
	var z = Math.pow(2, pos.z);
	x = (x - t.canvas.width/2)*z;
	y = (y - t.canvas.height/2)*z;
	[x, y] = t.rot(x, y, -pos.a);
	return [x + pos.x, y + pos.y];
};

RelightCanvas.prototype.zoom = function(dz, dt) {
	var p = this.pos;
	this.setPosition(dt, p.x, p.y, p.z+dz, p.a);
};

RelightCanvas.prototype.center = function(dt) {
	var p = this.pos;
	this.setPosition(dt, this.width/2, this.height/2, p.z, p.a);
};

RelightCanvas.prototype.centerAndScale = function(dt) {
	var t = this;
	t.pos.x = t.width/2;
	t.pos.y = t.height/2;
	t.pos.z = 0;
	var box = t.getBox(t.pos);
	var scale = Math.max((box[2]-box[0])/t.canvas.width, (box[3]-box[1])/t.canvas.height);
	var z = Math.log(scale)/Math.LN2;
	this.setPosition(dt, t.pos.x, t.pos.y, z, t.pos.a);
};

RelightCanvas.prototype.pan = function(dt, dx, dy) { //dx and dy expressed as pixels in the current size!
	var p = this.pos;
	//size of a rendering pixel in original image pixels.
	var scale = Math.pow(2, p.z);
	var r = this.rot(dx, dy, p.a);
	this.setPosition(dt, p.x - r[0]*scale, p.y - r[1]*scale, p.z, p.a);
};

RelightCanvas.prototype.rotate = function(dt, angle) {
	var p = this.pos;
	var a = p.a + angle;
	while(a > 360) a -= 360;
	while(a <   0) a += 360;
	this.setPosition(dt, p.x, p.y, p.z, a);
};

RelightCanvas.prototype.setPosition = function(dt, x, y, z, a) {

	var t = this;
	var scale = Math.pow(2, z);

	if(0 && t.bounded && t.width) {
		var zx = Math.min(z, Math.log(t.width/t.canvas.width)/Math.log(2.0));
		var zy = Math.min(z, Math.log(t.height/t.canvas.height)/Math.log(2.0));
		z = Math.max(zx, zy);
		scale = Math.pow(2, z);

		var ix = [scale*t.canvas.width/2, t.width - scale*t.canvas.width/2].sort(function(a, b) { return a-b; });
		x = Math.max(ix[0], Math.min(ix[1], x));
		var iy = [scale*t.canvas.height/2, t.height - scale*t.canvas.height/2].sort(function(a, b) { return a-b; });
		y = Math.max(iy[0], Math.min(iy[1], y));

		if(z <= -1) z = -1;
	}

	if(!dt) dt = 0;
	var time = performance.now();
	t.previous = t.getCurrent(time);

	if(x == t.previous.x && y == t.previous.y && z == t.previous.z && a == t.previous.a)
		return;

	t.pos = { x:x, y:y, z:z, a:a, t:time + dt };

	t.prefetch();
	t.redraw();

	for(var i = 0; i < t._onposchange.length; i++)
		t._onposchange[i]();
};

RelightCanvas.prototype.getCurrent = function(time) {
	var t = this;

	if(time > t.pos.t)
		return { x: t.pos.x, y: t.pos.y, z: t.pos.z, a: t.pos.a, t: time };

	var dt = t.pos.t - t.previous.t;
	if(dt < 1) return t.pos;

	var dt = (t.pos.t - time)/(t.pos.t - t.previous.t); //how much is missing to pos
	var ft = 1 - dt;

	return { 
		x:t.pos.x*ft + t.previous.x*dt, 
		y:t.pos.y*ft + t.previous.y*dt, 
		z:t.pos.z*ft + t.previous.z*dt, 
		a:t.pos.a*ft + t.previous.a*dt,
		t:time };
};

