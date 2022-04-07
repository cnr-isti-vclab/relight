(function (global, factory) {
    typeof exports === 'object' && typeof module !== 'undefined' ? factory(exports) :
    typeof define === 'function' && define.amd ? define(['exports'], factory) :
    (global = typeof globalThis !== 'undefined' ? globalThis : global || self, factory(global.OpenLIME = global.OpenLIME || {}));
})(this, (function (exports) { 'use strict';

    class BoundingBox {
        constructor(options) {
            Object.assign(this, {
                xLow: 1e20,
                yLow: 1e20,
                xHigh: -1e20, 
                yHigh: -1e20 });
            Object.assign(this, options);
        }

        fromArray(x) {
            this.xLow = x[0];
            this.yLow = x[1]; 
            this.xHigh = x[2];
            this.yHigh  = x[3];
        }
        
        toEmpty() {
            this.xLow = 1e20;
            this.yLow = 1e20; 
            this.xHigh = -1e20;
            this.yHigh  = -1e20;
        }

        isEmpty() {
            return this.xLow > this.xHigh || this.yLow > this.yHigh;
        }

        toArray() {
            return [this.xLow, this.yLow, this.xHigh, this. yHigh];
        }

        toString() {
            return this.xLow.toString() + " " + this.yLow.toString() + " " + this.xHigh.toString() + " " + this.yHigh.toString();
        }

        mergeBox(box) {
    		if (box == null) {
                return this;
            } else {
                this.xLow = Math.min(this.xLow,  box.xLow);
                this.yLow = Math.min(this.yLow,  box.yLow);
                this.xHigh = Math.max(this.xHigh, box.xHigh);
                this.yHigh = Math.max(this.yHigh, box.yHigh);
            }
        }

        mergePoint(p) {
            this.xLow = Math.min(this.xLow, p.x);
            this.yLow = Math.min(this.yLow, p.y);
            this.xHigh = Math.max(this.xHigh, p.x);
            this.yHigh = Math.max(this.yHigh, p.y);
        }
        
        shift(dx, dy) {
            this.xLow += dx;
            this.yLow += dy;
            this.xHigh += dx;
            this.yHigh += dy;
        }

        quantize(side) {
            this.xLow =  Math.floor(this.xLow/side);
            this.yLow =  Math.floor(this.yLow/side);
            this.xHigh = Math.floor((this.xHigh-1)/side) + 1;
            this.yHigh = Math.floor((this.yHigh-1)/side) + 1;
        }

        width() {
            return this.xHigh - this.xLow;
        }
        
        height() {
            return this.yHigh - this.yLow;
        }

        center() {
            return [(this.xLow+this.xHigh)/2, (this.yLow+this.yHigh)/2];
        }

        corner(i) {
            // To avoid the switch
            let v = this.toArray();
            return [ v[0 + (i&0x1)<<1],  v[1 + (i&0x2)] ];
        }

        print() {
            console.log("BOX=" + this.xLow.toFixed(2) + ", " + this.yLow.toFixed(2) + ", " + this.xHigh.toFixed(2) + ", " + this.yHigh.toFixed(2));
        }

    }

    /**
     * 
     * @param {number} x position
     * @param {number} y position
     * @param {number} z scale
     * @param {number} a rotation in degrees
     * @param {number} t time
     *
     */

    class Transform {
    	constructor(options) {
    		Object.assign(this, { x:0, y:0, z:1, a:0, t:0 });

    		if(!this.t) this.t = performance.now();
    		
    		if(typeof(options) == 'object')
    			Object.assign(this, options);
    	}

    	copy() {
    		let transform = new Transform();
    		Object.assign(transform, this);
    		return transform;
    	}

    	apply(x, y) {
    		//TODO! ROTATE
    		let r = Transform.rotate(x, y, this.a);
    		return { 
    			x: r.x*this.z + this.x,
    			y: r.y*this.z + this.y
    		}
    	}

    	inverse() {
    		let r = Transform.rotate(this.x/this.z, this.y/this.z, -this.a);
    		return new Transform({x:-r.x, y:-r.y, z:1/this.z, a:-this.a, t:this.t});
    	}

    	static normalizeAngle(a) {
    		while(a > 360) a -= 360;
    		while(a < 0) a += 360;
    		return a;
    	}

    	static rotate(x, y, angle) {
    		angle = Math.PI*(angle/180);
    		let ex =  Math.cos(angle)*x + Math.sin(angle)*y;
    		let ey = -Math.sin(angle)*x + Math.cos(angle)*y;
    		return {x:ex, y:ey};
    	}

    	// first get applied this (a) then  transform (b).
    	compose(transform) {
    		let a = this.copy();
    		let b = transform;
    		a.z *= b.z;
    		a.a += b.a;
    		var r = Transform.rotate(a.x, a.y, b.a);
    		a.x = r.x*b.z + b.x;
    		a.y = r.y*b.z + b.y; 
    		return a;
    	}

    	/* transform the box (for example -w/2, -h/2 , w/2, h/2 in scene coords) */
    	transformBox(lbox) {
    		let box = new BoundingBox();
    		for(let i = 0; i < 4; i++) {
    			let c = lbox.corner(i);
    			let p = this.apply(c[0], c[1]);
    			box.mergePoint(p);
    		}
    		return box;
    	}

    /*  get the bounding box (in image coordinate sppace) of the vieport. 
        the viewport has y -> pointing up
    	the image and screen transform has y pointing down. hence the inversion.
     */
    	getInverseBox(viewport) {
    		let inverse = this.inverse();
    		let corners = [
    			{x:viewport.x,               y:viewport.y},
    			{x:viewport.x + viewport.dx, y:viewport.y},
    			{x:viewport.x,               y:viewport.y + viewport.dy},
    			{x:viewport.x + viewport.dx, y:viewport.y + viewport.dy}
    		];
    		let box = new BoundingBox();
    		for(let corner of corners) {
    			let p = inverse.apply(corner.x -viewport.w/2, -corner.y + viewport.h/2);
    			box.mergePoint(p);
    		}
    		return box;
    	}

        interpolate(source, target, time, easing) {
    		let t = (target.t - source.t);

    		this.t = time;
    		if(time < source.t) 
    			return Object.assign(this, source);
    		if(time > target.t || t < 0.0001) 
    			return Object.assign(this, target);		

    		let tt = (time - source.t)/t;
        		switch(easing) {
        			case 'ease-out': tt = 1 - Math.pow(1 - tt, 2); break;
        			case 'ease-in-out': tt = tt < 0.5 ? 2*tt*tt : 1 - Math.pow(-2 * tt + 2, 2) / 2; break;
        		}
        		let st = 1 -tt;
    		
    		for(let i of ['x', 'y', 'z', 'a'])
    			this[i] = (st*source[i] + tt*target[i]);
    	}



    /**
     *  Combines the transform with the viewport to the viewport with the transform
     * @param {Object} transform a {@link Transform} class.
     */
    	projectionMatrix(viewport) {
    		let z = this.z;

    		// In coords with 0 in lower left corner map x0 to -1, and x0+v.w to 1
    		// In coords with 0 at screen center and x0 at 0, map -v.w/2 -> -1, v.w/2 -> 1 
    		// With x0 != 0: x0 -> x0-v.w/2 -> -1, and x0+dx -> x0+v.dx-v.w/2 -> 1
    		// Where dx is viewport width, while w is window width
    		//0, 0 <-> viewport.x + viewport.dx/2 (if x, y =
    		
    		let zx = 2/viewport.dx;
    		let zy = 2/viewport.dy;

    		let dx =  zx * this.x + (2/viewport.dx)*(viewport.w/2-viewport.x)-1;
    		let dy = -zy * this.y + (2/viewport.dy)*(viewport.h/2-viewport.y)-1;

    		let a = Math.PI *this.a/180;
    		let matrix = [
    			 Math.cos(a)*zx*z, Math.sin(a)*zy*z,  0,  0, 
    			-Math.sin(a)*zx*z, Math.cos(a)*zy*z,  0,  0,
    			 0,  0,  1,  0,
    			dx, dy, 0,  1];
    		return matrix;
    	}

    /**
     * TODO (if needed)
     */ 
    	toMatrix() {
    		let z = this.z;
    		return [
    			z,   0,   0,   0,
    			0,   z,   0,   0, 
    			0,   0,   1,   0,
    			z*x, z*y, 0,   1,
    		];
    	}

        /**
    	 * Transform p from scene (0 at image center) to [0,wh] 
    	 * @param {*} viewport viewport(x,y,dx,dy,w,h)
    	 * @param {*} p point in scene: 0,0 at image center
    	 */ 
    	sceneToViewportCoords(viewport, p) {
            return [p[0] * this.z  + this.x - viewport.x + viewport.w/2, 
                    p[1] * this.z  - this.y + viewport.y + viewport.h/2 ];
        }

    	/**
         * Transform p from  [0,wh] to scene (0 at image center)
    	 * 
    	 * @param {*} viewport viewport(x,y,dx,dy,w,h)
    	 * @param {*} p point in range [0..w-1,0..h-1]
    	 */
        viewportToSceneCoords(viewport, p) {
            return [(p[0] + viewport.x - viewport.w/2 - this.x) / this.z,
                    (p[1] - viewport.y - viewport.h/2 + this.y) / this.z];

        }

    }

    function matMul(a, b) {
    	let r = new Array(16);
    	r[ 0] = a[0]*b[0] + a[4]*b[1] + a[8]*b[2] + a[12]*b[3];
    	r[ 1] = a[1]*b[0] + a[5]*b[1] + a[9]*b[2] + a[13]*b[3];
    	r[ 2] = a[2]*b[0] + a[6]*b[1] + a[10]*b[2] + a[14]*b[3];
    	r[ 3] = a[3]*b[0] + a[7]*b[1] + a[11]*b[2] + a[15]*b[3];

    	r[ 4] = a[0]*b[4] + a[4]*b[5] + a[8]*b[6] + a[12]*b[7];
    	r[ 5] = a[1]*b[4] + a[5]*b[5] + a[9]*b[6] + a[13]*b[7];
    	r[ 6] = a[2]*b[4] + a[6]*b[5] + a[10]*b[6] + a[14]*b[7];
    	r[ 7] = a[3]*b[4] + a[7]*b[5] + a[11]*b[6] + a[15]*b[7];

    	r[ 8] = a[0]*b[8] + a[4]*b[9] + a[8]*b[10] + a[12]*b[11];
    	r[ 9] = a[1]*b[8] + a[5]*b[9] + a[9]*b[10] + a[13]*b[11];
    	r[10] = a[2]*b[8] + a[6]*b[9] + a[10]*b[10] + a[14]*b[11];
    	r[11] = a[3]*b[8] + a[7]*b[9] + a[11]*b[10] + a[15]*b[11];

    	r[12] = a[0]*b[12] + a[4]*b[13] + a[8]*b[14] + a[12]*b[15];
    	r[13] = a[1]*b[12] + a[5]*b[13] + a[9]*b[14] + a[13]*b[15];
    	r[14] = a[2]*b[12] + a[6]*b[13] + a[10]*b[14] + a[14]*b[15];
    	r[15] = a[3]*b[12] + a[7]*b[13] + a[11]*b[14] + a[15]*b[15];
    	return r;
    }

    /**
     *  NOTICE TODO: the camera has the transform relative to the whole canvas NOT the viewport.
     * @param {object} options
     * * *bounded*: limit translation of the camera to the boundary of the scene.
     * * *maxZoom*: maximum zoom, 1:maxZoom is screen pixel to image pixel ratio.
     * * *minZoom*: minimum zoom,
     * * *minScreenFraction: the minimum portion of the screen to zoom in
     * * *maxFixedZoom: maximum pixel size
     * Signals:
     * Emits 'update' event when target is changed.
     */

    class Camera {

    	constructor(options) {
    		Object.assign(this, {
    			viewport: null,
    			bounded: true,
    			minScreenFraction: 1,
    			maxFixedZoom: 2,
    			maxZoom: 2,
    			minZoom: 1,
    			boundingBox: new BoundingBox,

    			signals: {'update':[]}
    		});
    		Object.assign(this, options);
    		this.target = new Transform(this.target);
    		this.source = this.target.copy();
        		this.easing = 'linear';
    	}

    	copy() {
    		let camera = new Camera();
    		Object.assign(camera, this);
    		return camera;
    	}

    	addEvent(event, callback) {
    		this.signals[event].push(callback);
    	}

    	emit(event) {
    		for(let r of this.signals[event])
    			r(this);
    	}

    /**
     *  Set the viewport and updates the camera for an as close as possible.
     */
    	setViewport(view) {
    		if(this.viewport) {
    			let rz = Math.sqrt((view.w/this.viewport.w)*(view.h/this.viewport.h));
    			this.viewport = view;
    			const {x, y, z, a } = this.target;
    			this.setPosition(0, x, y, z*rz, a);
    		} else {
    			this.viewport = view;
    		}
    	}

    	glViewport() {
    		let d = window.devicePixelRatio;
    		let viewport = {};
    		for (let i in this.viewport) 
    			viewport[i] = this.viewport[i]*d;
    		return viewport;
    	}
    /**
     *  Map coordinate relative to the canvas into scene coords. using the specified transform.
     * @returns [X, Y] in scene coordinates.
     */
    	mapToScene(x, y, transform) {
    		//compute coords relative to the center of the viewport.
    		x -= this.viewport.w/2;
    		y -= this.viewport.h/2;
    		x -= transform.x;
    		y -= transform.y;
    		x /= transform.z;
    		y /= transform.z;
    		let r = Transform.rotate(x, y, -transform.a);
    		return {x:r.x, y:r.y};
    	}
        	sceneToCanvas(x, y, transform) {
        		let r = Transform.rotate(x, y, transform.a);
        		x = r.x * transform.z;
        		y = t.y * transform.z;
        		x += transform.x;
        		y += transform.y;
        		x += this.viewport/2;
        		y += this.viewport/2;
        		return { x: x, y: y };
        	}

    	setPosition(dt, x, y, z, a, easing) {
    		// Discard events due to cursor outside window
    		//if (Math.abs(x) > 64000 || Math.abs(y) > 64000) return;
    		this.easing = easing || this.easing;

    		if (this.bounded) {
    			const sw = this.viewport.dx;
    			const sh = this.viewport.dy;

    			//
    			let xform = new Transform({x:x, y:y, z:z, a:a,t:0});
    			let tbox = xform.transformBox(this.boundingBox);
    			const bw = tbox.width();
    			const bh = tbox.height();

    			// Screen space offset between image boundary and screen boundary
    			// Do not let transform offet go beyond this limit.
    			// if (scaled-image-size < screen) it remains fully contained
    			// else the scaled-image boundary closest to the screen cannot enter the screen.
    			const dx = Math.abs(bw-sw)/2;
    			x = Math.min(Math.max(-dx, x), dx);

    			const dy = Math.abs(bh-sh)/2;
    			y = Math.min(Math.max(-dy, y), dy);
    		}

    		let now = performance.now();
    		this.source = this.getCurrentTransform(now);
    		//the angle needs to be interpolated in the shortest direction.
    		//target it is kept between 0 and +360, source is kept relative.
    		a = Transform.normalizeAngle(a);
    		this.source.a = Transform.normalizeAngle(this.source.a);
    		if(a - this.source.a > 180) this.source.a += 360;
    		if(this.source.a - a > 180) this.source.a -= 360;
    		Object.assign(this.target, { x: x, y:y, z:z, a:a, t:now + dt });
    		this.emit('update');
    	}
    	

    /*
     * Pan the camera 
     * @param {number} dx move the camera by dx pixels (positive means the image moves right).
     */
    	pan(dt, dx, dy) {
    		let now = performance.now();
    		let m = this.getCurrentTransform(now);
    		m.dx += dx;
    		m.dy += dy;
    	}

    /* zoom in or out at a specific point in canvas coords!
     * TODO: this is not quite right!
     */
    	zoom(dt, z, x, y) {
    		if(!x) x = 0;
    		if(!y) y = 0;

    		let now = performance.now();
    		let m = this.getCurrentTransform(now);

    		if (this.bounded) {
    			z = Math.min(Math.max(z, this.minZoom), this.maxZoom);
    		}

    		//x, an y should be the center of the zoom.
    		m.x += (m.x+x)*(m.z - z)/m.z;
    		m.y += (m.y+y)*(m.z - z)/m.z;

    		this.setPosition(dt, m.x, m.y, z, m.a);
    	}

    	rotate(dt, a) {

    		let now = performance.now();
    		let m = this.getCurrentTransform(now);

    		this.setPosition(dt, m.x, m.y, m.z, this.target.a + a);
    	}

    	deltaZoom(dt, dz, x, y) {
    		if(!x) x = 0;
    		if(!y) y = 0;

    		let now = performance.now();
    		let m = this.getCurrentTransform(now);


    		//rapid firing wheel event need to compound.
    		//but the x, y in input are relative to the current transform.
    		dz *= this.target.z/m.z;

    		if (this.bounded) {
    			if (m.z*dz < this.minZoom) dz = this.minZoom / m.z;
    			if (m.z*dz > this.maxZoom) dz = this.maxZoom / m.z;
    		}

    		//transform is x*z + dx = X , there x is positrion in scene, X on screen
    		//we want x*z*dz + dx1 = X (stay put, we need to find dx1.
    		let r = Transform.rotate(x, y, m.a);
    		m.x += r.x*m.z*(1 - dz);
    		m.y += r.y*m.z*(1 - dz);

    		
    		this.setPosition(dt, m.x, m.y, m.z*dz, m.a);
    	}


    	getCurrentTransform(time) {
    		let pos = new Transform();
    		if(time < this.source.t)
    			Object.assign(pos, this.source);
    		if(time >= this.target.t)
    			Object.assign(pos, this.target);
    		else 
    			pos.interpolate(this.source, this.target, time, this.easing);

    		pos.t = time;
    		return pos;
    	}

    	getGlCurrentTransform(time) {
    		const pos = this.getCurrentTransform(time);
    		pos.x *= window.devicePixelRatio;
    		pos.y *= window.devicePixelRatio;
    		pos.z *= window.devicePixelRatio;
    		return pos;
    	}
    /**
     * @param {Array} box fit the specified rectangle [minx, miny, maxx, maxy] in the canvas.
     * @param {number} dt animation duration in millisecond 
     * @param {string} size how to fit the image: <contain | cover> default is contain (and cover is not implemented
     */

    //TODO should fit keeping the same angle!
    	fit(box, dt, size) {
    		if (box.isEmpty()) return;
    		if(!dt) dt = 0;

    		//find if we align the topbottom borders or the leftright border.
    		let w = this.viewport.dx;
    		let h = this.viewport.dy;


    		let bw = box.width();
    		let bh = box.height();
    		let c = box.center();
    		let z = Math.min(w/bw, h/bh);

    		this.setPosition(dt, -c[0], -c[1], z, 0);
    	}

    	fitCameraBox(dt) {
    		this.fit(this.boundingBox, dt);
    	}

    	updateBounds(box, minScale) {
    		this.boundingBox = box;
    		const w = this.viewport.dx;
    		const h = this.viewport.dy;

    		let bw = this.boundingBox.width();
    		let bh = this.boundingBox.height();
    	
    		this.minZoom = Math.min(w/bw, h/bh) * this.minScreenFraction;
    		this.maxZoom = minScale > 0 ? this.maxFixedZoom / minScale : this.maxFixedZoom;
    		this.maxZoom = Math.max(this.minZoom, this.maxZoom);
    	}
    }

    /**
     * @param {string|Object} url URL of the image or the tiled config file, 
     * @param {string} type select one among: <image, {@link https://www.microimages.com/documentation/TechGuides/78googleMapsStruc.pdf google}, {@link https://docs.microsoft.com/en-us/previous-versions/windows/silverlight/dotnet-windows-silverlight/cc645077(v=vs.95)?redirectedfrom=MSDN deepzoom}, {@link http://www.zoomify.com/ZIFFileFormatSpecification.htm zoomify}, {@link https://iipimage.sourceforge.io/ iip}, {@link https://iiif.io/api/image/3.0/ iiif}>
     */
    class Layout {
    	constructor(url, type, options) {
    		Object.assign(this, {
    			type: type,
    			width: 0,
    			height: 0,
    			tilesize: 256,
    			overlap: 0, 
    			nlevels: 1,        //level 0 is the top, single tile level.
    			suffix: 'jpg',
    			qbox: [],          //array of bounding box in tiles, one for mipmap 
    			bbox: [],          //array of bounding box in pixels (w, h)
    			urls: [],
    			signals: { ready: [], updateSize: [] },          //callbacks when the layout is ready.
    			status: null,
    			subdomains: 'abc'
    		});
    		if(options)
    			Object.assign(this, options);

    		if(typeof(url) == 'string')
    			this.setUrls([url]);

    		if(typeof(url) == 'object')
    			Object.assign(this, url);

    	}

    	setUrls(urls) {
    		this.urls = urls;
    		(async () => {
    			switch(this.type) {
    				case 'image':    await this.initImage(); break; // No Url needed
    				case 'google':   await this.initGoogle(); break; // No Url needed

    				case 'deepzoom1px': await this.initDeepzoom(true); break; // urls[0] only needed
    				case 'deepzoom': await this.initDeepzoom(false); break; // urls[0] only needed
    				case 'zoomify':  await this.initZoomify(); break; // urls[0] only needed
    				case 'iiif':     await this.initIIIF(); break; // urls[0] only needed

    				case 'tarzoom':  await this.initTarzoom(); break; // all urls needed

    				case 'itarzoom':  await this.initITarzoom(); break; // actually it has just one url
    			}
    			this.initBoxes();
    			this.status = 'ready';
    			this.emit('ready');
    		})().catch(e => { console.log(e); this.status = e; });
    	}

    	addEvent(event, callback) {
    		this.signals[event].push(callback);
    	}

    	emit(event) {
    		for(let r of this.signals[event])
    			r(this);
    	}

    	isReady() {
    		return this.status == 'ready' && this.width && this.height;
    	}

    	boundingBox() {
    		if(!this.width) throw "Layout not initialized still";
    		return new BoundingBox({xLow:-this.width/2, yLow: -this.height/2, xHigh: this.width/2, yHigh: this.height/2});
    	}

    /**
     *  Each tile is assigned an unique number.
     */

    	index(level, x, y) {
    		let startindex = 0;
    		for(let i = 0; i < level; i++)
    			startindex += this.qbox[i].xHigh*this.qbox[i].yHigh;
    		return startindex + y*this.qbox[level].xHigh + x;
    	}

    /*
     * Compute all the bounding boxes (this.bbox and this.qbox).
     * @return number of tiles in the dataset
    */

    	initBoxes() {
    		this.qbox = []; //by level (0 is the bottom)
    		this.bbox = [];
    		var w = this.width;
    		var h = this.height;

    		if(this.type == 'image') {
    			this.qbox[0] = new BoundingBox({xLow:0, yLow: 0, xHigh: 1, yHigh: 1});
    			this.bbox[0] = new BoundingBox({xLow:0, yLow: 0, xHigh: w, yHigh: h}); 
    			// Acknowledge bbox change (useful for knowing scene extension (at canvas level))
    			this.emit('updateSize');
    			return 1;
    		}

    		for(let level = this.nlevels - 1; level >= 0; level--) {
    			this.qbox[level] = new BoundingBox({xLow:0, yLow: 0, xHigh: 0, yHigh: 0});
    			this.bbox[level] = new BoundingBox({xLow:0, yLow: 0, xHigh: w, yHigh: h}); 

    			this.qbox[level].yHigh = Math.ceil(h/this.tilesize);
    			this.qbox[level].xHigh = Math.ceil(w/this.tilesize);

    			// for(let y = 0; y*this.tilesize < h; y++) { // TODO replace with division
    			// 	this.qbox[level].yHigh = y+1;
    			// }
    			// for (let x = 0; x * this.tilesize < w; x++) {
    			// 	this.qbox[level].xHigh = x + 1;
    			// 	//					tiles.push({level:level, x:x, y:y});
    			// }

    			w >>>= 1;
    			h >>>= 1;
    		}
    		// Acknowledge bbox (useful for knowing scene extension (at canvas level))
    		this.emit('updateSize');
    	}

    /** Return the coordinates of the tile (in [0, 0, w h] image coordinate system) and the texture coords associated. 
     *
     */
    	tileCoords(level, x, y) {
    		let w = this.width;
    		let h = this.height;
    		//careful: here y is inverted due to textures not being flipped on load (Firefox fault!).
    		var tcoords = new Float32Array([0, 1,     0, 0,     1, 0,     1, 1]);

    		if(this.type == "image") {
    			return { 
    				coords: new Float32Array([-w/2, -h/2, 0,  -w/2, h/2, 0,  w/2, h/2, 0,  w/2, -h/2, 0]),
    				tcoords: tcoords 
    			};
    		}

    		let coords = new Float32Array([0, 0, 0,  0, 1, 0,  1, 1, 0,  1, 0, 0]);

    		let ilevel = this.nlevels - 1 - level;
    		let side =  this.tilesize*(1<<(ilevel)); //tile size in imagespace
    		let tx = side;
    		let ty = side;

    		if(side*(x+1) > this.width) {
    			tx = (this.width  - side*x);
    			if(this.type == 'google')
    				tcoords[4] = tcoords[6] = tx/side;
    		}

    		if(side*(y+1) > this.height) {
    			ty = (this.height - side*y);
    			if(this.type == 'google')
    				tcoords[1] = tcoords[7] = ty/side;
    		}

    		var lx  = this.qbox[level].xHigh-1; //last tile x pos, if so no overlap.
    		var ly  = this.qbox[level].yHigh-1;

    		var over = this.overlap;
    		if(over) {
    			let dtx = over / (tx/(1<<ilevel) + (x==0?0:over) + (x==lx?0:over));
    			let dty = over / (ty/(1<<ilevel) + (y==0?0:over) + (y==ly?0:over));

    			tcoords[0] = tcoords[2] = (x==0? 0: dtx);
    			tcoords[3] = tcoords[5] = (y==0? 0: dty);
    			tcoords[4] = tcoords[6] = (x==lx? 1: 1 - dtx);
    			tcoords[1] = tcoords[7] = (y==ly? 1: 1 - dty);
    		} 
    		//flip Y coordinates 
    		//TODO cleanup this mess!
    		let tmp = tcoords[1];
    		tcoords[1] = tcoords[7] = tcoords[3];
    		tcoords[3] = tcoords[5] = tmp;

    		for(let i = 0; i < coords.length; i+= 3) {
    			coords[i]   =  coords[i]  *tx + side*x - this.width/2;
    			coords[i+1] = -coords[i+1]*ty - side*y + this.height/2;
    		}

    		return { coords: coords, tcoords: tcoords }
    	}


    /**
     * Given a viewport and a transform computes the tiles needed for each level.
     * @param {array} viewport array with left, bottom, width, height
     * @param {border} border is radius (in tiles units) of prefetch
     * @returns {object} with level: the optimal level in the pyramid, pyramid: array of bounding boxes in tile units.
     */
    	neededBox(viewport, transform, border, bias) {
    		if(this.type == "image")
    			return { level:0, pyramid: [new BoundingBox({ xLow:0, yLow:0, xHigh:1, yHigh:1 })] };

    		//here we are computing with inverse levels; level 0 is the bottom!
    		let iminlevel = Math.max(0, Math.min(Math.floor(-Math.log2(transform.z) + bias), this.nlevels-1));
    		let minlevel = this.nlevels-1-iminlevel;
    		//
    		let bbox = transform.getInverseBox(viewport);
    		//find box in image coordinates where (0, 0) is in the upper left corner.
    		bbox.shift(this.width/2, this.height/2);

    		let pyramid = [];
    		for(let level = 0; level <= minlevel; level++) {
    			let ilevel = this.nlevels -1 -level;
    			let side = this.tilesize*Math.pow(2, ilevel);

    			let qbox = new BoundingBox(bbox);
    			qbox.quantize(side);

    			//clamp!
                qbox.xLow  = Math.max(qbox.xLow  - border, this.qbox[level].xLow);
                qbox.yLow  = Math.max(qbox.yLow  - border, this.qbox[level].yLow);
                qbox.xHigh = Math.min(qbox.xHigh + border, this.qbox[level].xHigh);
                qbox.yHigh = Math.min(qbox.yHigh + border, this.qbox[level].yHigh);
    			pyramid[level] = qbox;
    		}
    		return { level: minlevel, pyramid: pyramid };
    	}

    	getTileURL(id, tile) {
    		throw Error("Layout not defined or ready.");
    	}



    /*
     * Witdh and height can be recovered once the image is downloaded.
    */
    	async initImage() {
    		this.getTileURL = (rasterid, tile) => { return this.urls[rasterid]; };
    		this.nlevels = 1;
    		this.tilesize = 0;
    	}

    /**
     *  url points to the folder (without /)

     *  width and height must be defined
     */
    	async initGoogle(callback) {
    		if(!this.width || !this.height)
    			throw "Google rasters require to specify width and height";

    		this.tilesize = 256;
    		this.overlap = 0;

    		let max = Math.max(this.width, this.height)/this.tilesize;
    		this.nlevels = Math.ceil(Math.log(max) / Math.LN2) + 1;

    		if( this.urls[0].includes('{')) {
    			this.getTileURL = (rasterid, tile) => {
    				let  s = this.subdomains ? this.subdomains[Math.abs(tile.x + tile.y) % this.subdomains.length] : '';
    				let vars = {s, ...tile, z: tile.level};
    				return this.urls[rasterid].replace(/{(.+?)}/g,(match,p)=> vars[p]);
    			};
    		} else
    			this.getTileURL = (rasterid, tile) => {
    				return this.urls[rasterid] + "/" + tile.level + "/" + tile.y + "/" + tile.x + '.' + this.suffix;
    			};
    	}


    /**
     * Expects the url to point to .dzi config file
     */
    	async initDeepzoom(onepixel) {
    		let url = this.urls[0];
    		var response = await fetch(url);
    		if(!response.ok) {
    			this.status = "Failed loading " + url + ": " + response.statusText;
    			throw new Error(this.status);
    		}
    		let text = await response.text();
    		let xml = (new window.DOMParser()).parseFromString(text, "text/xml");

    		let doc = xml.documentElement;
    		this.suffix = doc.getAttribute('Format');
    		this.tilesize = parseInt(doc.getAttribute('TileSize'));
    		this.overlap = parseInt(doc.getAttribute('Overlap'));

    		let size = doc.querySelector('Size');
    		this.width = parseInt(size.getAttribute('Width'));
    		this.height = parseInt(size.getAttribute('Height'));

    		let max = Math.max(this.width, this.height)/this.tilesize;
    		this.nlevels = Math.ceil(Math.log(max) / Math.LN2) + 1;

    		this.urls = this.urls.map(url => url.substr(0, url.lastIndexOf(".")) + '_files/');
    		this.skiplevels = 0;
    		if(onepixel)
    			this.skiplevels = Math.ceil(Math.log(this.tilesize) / Math.LN2);

    		this.getTileURL = (rasterid, tile) => {
    			let url = this.urls[rasterid];
    			let level = tile.level + this.skiplevels;
    			return url + level + '/' + tile.x + '_' + tile.y + '.' + this.suffix;
    		}; 
    	}

    	async initTarzoom() {
    		this.tarzoom =[];	
    		for (let url of this.urls) {
    			var response = await fetch(url);
    			if (!response.ok) {
    				this.status = "Failed loading " + url + ": " + response.statusText;
    				throw new Error(this.status);
    			}
    			let json = await response.json();
    			json.url = url.substr(0, url.lastIndexOf(".")) + '.tzb';
    			Object.assign(this, json);
    			this.tarzoom.push(json);
    		}

    		this.getTileURL = (rasterid, tile) => {
    			const tar = this.tarzoom[rasterid];
    			tile.start = tar.offsets[tile.index];
    			tile.end = tar.offsets[tile.index+1];
    			return tar.url;
    		}; 
    	}


    	async initITarzoom() {
    		const url = this.urls[0];		
    		var response = await fetch(url);
    		if(!response.ok) {
    			this.status = "Failed loading " + url + ": " + response.statusText;
    			throw new Error(this.status);
    		}
    		let json = await response.json();
    		Object.assign(this, json); //suffix, tilesize, overlap, width, height, levels
    		this.url = url.substr(0, url.lastIndexOf(".")) + '.tzb';

    		this.getTileURL = (rasterid, tile) => {
    			let index = tile.index*this.stride;
    			tile.start = this.offsets[index];
    			tile.end = this.offsets[index+this.stride];
    			tile.offsets = [];
    			for(let i = 0; i < this.stride+1; i++)
    				tile.offsets.push(this.offsets[index + i] - tile.start);
    			return this.url;
    		}; 
    	}



    /**
     * Expects the url to point to ImageProperties.xml file.
     */
    	async initZoomify() {
    		const url = this.urls[0];
    		this.overlap = 0;
    		var response = await fetch(url);
    		if(!response.ok) {
    			this.status = "Failed loading " + url + ": " + response.statusText;
    			throw new Error(this.status);
    		}
    		let text = await response.text();
    		let xml = (new window.DOMParser()).parseFromString(text, "text/xml");
    		let doc = xml.documentElement;
    		this.tilesize = parseInt(doc.getAttribute('TILESIZE'));
    		this.width = parseInt(doc.getAttribute('WIDTH'));
    		this.height = parseInt(doc.getAttribute('HEIGHT'));
    		if(!this.tilesize || !this.height || !this.width)
    			throw "Missing parameter files for zoomify!";

    		let max = Math.max(this.width, this.height)/this.tilesize;
    		this.nlevels = Math.ceil(Math.log(max) / Math.LN2) + 1;

    		this.getTileURL = (rasterid, tile) => {
    			const tileUrl = this.urls[rasterid].substr(0, url.lastIndexOf("/"));
    			let group = tile.index >> 8;
    			return tileUrl + "/TileGroup" + group + "/" + tile.level + "-" + tile.x + "-" + tile.y + "." + this.suffix;
    		};
    	}

    	async initIIIF() {
    		const url = this.urls[0];
    		this.overlap = 0;

    		var response = await fetch(url);
    		if(!response.ok) {
    			this.status = "Failed loading " + url + ": " + response.statusText;
    			throw new Error(this.status);
    		}
    		let info = await response.json();
    		this.width = info.width;
    		this.height = info.height;
    		this.nlevels = info.tiles[0].scaleFactors.length;
    		this.tilesize = info.tiles[0].width;

    		this.getTileURL = (rasterid, tile) => {
    			const tileUrl = this.urls[rasterid].substr(0, url.lastIndexOf("/"));
    			let tw = this.tilesize;
    			parseInt(this.nlevels - 1 - tile.level);
    			let s = Math.pow(2, tile.level);

    			//region parameters
    			let xr = tile.x * tw * s;
    			let yr = tile.y * tw * s;
    			let wr = Math.min(tw * s, this.width - xr);
    			let hr = Math.min(tw * s, this.height - yr);

    			// pixel size parameters /ws,hs/
    			let ws = tw;
    			if (xr + tw*s > this.width)
    				ws = (this.width - xr + s - 1) / s;  
    			let hs = tw;
    			if (yr + tw*s > this.height)
    				hs = (this.height - yr + s - 1) / s;

    			return `${tileUrl}/${xr},${yr},${wr},${hr}/${ws},${hs}/0/default.jpg`;
    		};
    	}
    }

    /* Cache holds the images and the tile textures.
     *  Each tile has a priority 0 and above means it is visible, 
     *  negative depends on how far from the border and how more zoomed you need to go
     * 
     * * *capacity*: in bytes, max amount of GPU RAM used.
     * *size*: current size (read only!)
     * *maxRequest*: max number of concurrent HTTP requests
     * *requested*: current number of HTTP requests (read only!)
     * *maxPrefetch*: max number of tile prefetched (each tile might require multiple httprequests)
     * *prefetched*: current number of tiles requested (read only!)
    */

    class _Cache {
    	constructor(options) {
    		Object.assign(this, {
    			capacity: 512*(1<<20),  //256 MB total capacity available
    			size: 0,                //amount of GPU ram used

    			maxRequest: 4,          //max number of concurrent HTTP requests
    			requested: 0,
    			maxPrefetch: 8*(1<<20), //max amount of prefetched tiles.
    			prefetched: 0           //amount of currently prefetched GPU ram.
    		});

    		Object.assign(this, options);
    		this.layers = [];   //map on layer.
    	}

    /*  Queue is an ordered array of tiles needed by a layer.
     */
    	setCandidates(layer) {
    		if(!this.layers.includes(layer))
    			this.layers.push(layer);
    		setTimeout(() => { this.update(); }, 0); //ensure all the queues are set before updating.
    	}

    /* Look for best tile to load and schedule load from the web.
     */
    	update() {
    		if(this.requested > this.maxRequest)
    			return;

    		let best = this.findBestCandidate();
    		if(!best) return;
    		while(this.size > this.capacity) { //we need to make room.
    			let worst = this.findWorstTile();
    			if(!worst) {
    				console.log("BIG problem in the cache");
    				break;
    			}
    			if(worst.tile.time < best.tile.time)
    				this.dropTile(worst.layer, worst.tile);
    			else
    				return; 
    		}
    		this.loadTile(best.layer, best.tile);
    	}

    	findBestCandidate() {
    		let best = null;
    		for(let layer of this.layers) {
    			if(!layer.queue.length)
    				continue;
    			let tile = layer.queue.shift();
    			if(!best ||
    				tile.time > best.tile.time  + 1.0 ||  //old requests ignored
    				tile.priority > best.tile.priority)
    				best = { layer, tile };
    		}
    		return best;
    	}

    	findWorstTile() {
    		let worst = null;
    		for(let layer of this.layers) {
    			for(let tile of layer.tiles.values()) {
    				//TODO might be some are present when switching shaders.
    				if(tile.missing != 0) continue;
    				if(!worst || 
    				   tile.time < worst.tile.time || 
    				   (tile.time == worst.tile.time && tile.priority < worst.tile.priority)) {
    					worst = {layer, tile};
    				}
    			}
    		}
    		return worst;
    	}

    /* 
     */
    	loadTile(layer, tile) {
    		this.requested++;
    		(async () =>  { layer.loadTile(tile, (size) => { this.size += size; this.requested--; this.update(); } ); })();
    	}
    /*
     */
    	dropTile(layer, tile) {
    		this.size -= tile.size;
    		layer.dropTile(tile);
    	}
    /* Flush all memory
     */
    	flush() {
    	}

    /* Flush all tiles for a layer.
     */
    	flushLayer(layer) {
    		if(!this.layers.includes(layer))
    			return;
    		for(let tile of layer.tiles.values())
    			this.dropTile(layer, tile);
    	}

    /* 
     */
    }

    let Cache = new _Cache;

    /**
     * @param {string} id unique id for layer.
     * @param {object} options
     *  * *label*:
     *  * *transform*: relative coordinate [transformation](#transform) from layer to canvas
     *  * *visible*: where to render or not
     *  * *zindex*: stack ordering of the layer higher on top
     *  * *opacity*: from 0.0 to 1.0 (0.0 is fully transparent)
     *  * *rasters*: [rasters](#raster) used for rendering.
     *  * *controls*: shader parameters that can be modified (eg. light direction)
     *  * *shader*: [shader](#shader) used for rendering
     *  * *layout*: one of image, deepzoom, google, iiif, or zoomify
     *  * *mipmapBias*: default 0.4, when to switch between different levels of the mipmap, 0 means switch as early
     *                  as the tile would be enlarged on the screen, while 1.0 means switch when 1 pixel in tile is >= 2 pixels on screen
     *  * *prefetchBorder*: border tiles prefetch (default 1)
     *  * *maxRequest*: max number of simultaneous requests (should be GLOBAL not per layer!) default 4
     */

    class Layer {
    	constructor(options) {

    		//create from derived class if type specified
    		if (options.type) {
    			let type = options.type;
    			delete options.type;
    			if (type in this.types) {

    				return this.types[type](options);
    			}
    			throw "Layer type: " + type + "  module has not been loaded";
    		}

    		this.init(options);

    		/*
    		//create members from options.
    		this.rasters = this.rasters.map((raster) => new Raster(raster));

    		//layout needs to be the same for all rasters
    		if(this.rasters.length) {
    			if(typeof(this.layout) != 'object')
    				this.layout = new Layout(this.rasters[0].url, this.layout)
    			this.setLayout(this.layout)

    			if(this.rasters.length)
    				for(let raster in this.rasters)
    					raster.layout = this.layout;
    		}

    		if(this.shader)
    			this.shader = new Shader(this.shader);
    		*/
    	}

    	init(options) {
    		Object.assign(this, {
    			transform: new Transform(),
    			visible: true,
    			zindex: 0,
    			opacity: 1.0,
    			overlay: false, //in the GUI it won't affect the visibility of the other layers
    			rasters: [],
    			layers: [],
    			controls: {},
    			controllers: [],
    			shaders: {},
    			layout: 'image',
    			shader: null, //current shader.
    			gl: null,

    			prefetchBorder: 1,
    			mipmapBias: 0.4,
    			maxRequest: 4,

    			signals: { update: [], ready: [], updateSize: [] },  //update callbacks for a redraw, ready once layout is known.

    			//internal stuff, should not be passed as options.
    			tiles: new Map(),      //keep references to each texture (and status) indexed by level, x and y.
    			//each tile is tex: [.. one for raster ..], missing: 3 missing tex before tile is ready.
    			//only raster used by the shader will be loade.
    			queue: [],     //queue of tiles to be loaded.
    			requested: {},  //tiles requested.
    		});

    		Object.assign(this, options);

    		this.transform = new Transform(this.transform);

    		if (typeof (this.layout) == 'string') {
    			let size = { width: this.width || 0, height: this.height || 0 };
    			this.setLayout(new Layout(null, this.layout, size));
    		} else {
    			this.setLayout(this.layout);
    		}
    	}

    	addEvent(event, callback) {
    		this.signals[event].push(callback);
    	}

    	emit(event, ...parameters) {
    		for (let r of this.signals[event])
    			r(...parameters);
    	}

    	setLayout(layout) {
    		let callback = () => {
    			this.status = 'ready';
    			this.setupTiles(); //setup expect status to be ready!
    			this.emit('ready');
    			this.emit('update');
    		};
    		if (layout.status == 'ready') //layout already initialized.
    			callback();
    		else
    			layout.addEvent('ready', callback);
    		this.layout = layout;

    		// Set signal to acknowledge change of bbox when it is known. Let this signal go up to canvas
    		this.layout.addEvent('updateSize', () => { this.emit('updateSize'); });
    	}

    	setTransform(tx) {
    		this.transform = tx;
    		this.emit('updateSize');
    	}

    	setShader(id) {
    		if (!id in this.shaders)
    			throw "Unknown shader: " + id;
    		this.shader = this.shaders[id];
    		this.setupTiles();
    		this.shader.setEvent('update', () => { this.emit('update'); });
    	}

    	getMode() {
    		return this.shader.mode;
    	}

    	getModes() {
    		if (this.shader)
    			return this.shader.modes;
    		return [];
    	}

    	setMode(mode) {
    		this.shader.setMode(mode);
    		this.emit('update');
    	}

    	/**
    	 * @param {bool} visible
    	 */
    	setVisible(visible) {
    		this.visible = visible;
    		this.previouslyNeeded = null;
    		this.emit('update');
    	}

    	/**
    	 * @param {int} zindex
    	 */
    	setZindex(zindex) {
    		this.zindex = zindex;
    		this.emit('update');
    	}

    	static computeLayersMinScale(layers, discardHidden) {
    		if (layers == undefined || layers == null) {
    			console.log("ASKING SCALE INFO ON NO LAYERS");
    			return 1;
    		}
    		let layersScale = 1;
    		for (let layer of Object.values(layers)) {
    			if (!discardHidden || layer.visible) {
    				let s = layer.scale();
    				layersScale = Math.min(layersScale, s);
    			}
    		}
    		return layersScale;
    	}

    	scale() {
    		// FIXME: this do not consider children layers
    		return this.transform.z;
    	}

    	boundingBox() {
    		// FIXME: this do not consider children layers
    		// Take layout bbox
    		let result = this.layout.boundingBox();

    		// Apply layer transform to bbox
    		if (this.transform != null && this.transform != undefined) {
    			result = this.transform.transformBox(result);
    		}

    		return result;
    	}

    	static computeLayersBBox(layers, discardHidden) {
    		if (layers == undefined || layers == null) {
    			console.log("ASKING BBOX INFO ON NO LAYERS");
    			let emptyBox = new BoundingBox();
    			return emptyBox;
    		}
    		let layersBbox = new BoundingBox();
    		for (let layer of Object.values(layers)) {
    			if ((!discardHidden || layer.visible) && layer.layout.width) {
    				const bbox = layer.boundingBox();
    				layersBbox.mergeBox(bbox);
    			}
    		}
    		return layersBbox;
    	}

    	setControl(name, value, dt) {
    		let now = performance.now();
    		let control = this.controls[name];
    		this.interpolateControl(control, now);

    		control.source.value = [...control.current.value];
    		control.source.t = now;

    		control.target.value = [...value];
    		control.target.t = now + dt;

    		this.emit('update');
    	}

    	interpolateControls() {
    		let now = performance.now();
    		let done = true;
    		for (let control of Object.values(this.controls))
    			done = this.interpolateControl(control, now) && done;
    		return done;
    	}

    	interpolateControl(control, time) {
    		let source = control.source;
    		let target = control.target;
    		let current = control.current;

    		current.t = time;
    		if (time < source.t) {
    			current.value = [...source.value];
    			return false;
    		}

    		if (time > target.t - 0.0001) {
    			let done = current.value.every((e, i) => e === target.value[i]);
    			current.value = [...target.value];
    			return done;
    		}

    		let t = (target.t - source.t);
    		let tt = (time - source.t) / t;
    		let st = (target.t - time) / t;

    		current.value = [];
    		for (let i = 0; i < source.value.length; i++)
    			current.value[i] = (st * source.value[i] + tt * target.value[i]);
    		return false;
    	}

    	dropTile(tile) {
    		for(let i = 0; i < tile.tex.length; i++) {
    			if(tile.tex[i]) {
    				this.gl.deleteTexture(tile.tex[i]);
    			}
    		}
    		this.tiles.delete(tile.index);
    	}

    	clear() {
    		this.ibuffer = this.vbuffer = null;
    		Cache.flushLayer(this);
    		this.tiles = new Map(); //TODO We need to drop these tile textures before clearing Map
    		this.setupTiles();
    		this.queue = [];
    		this.previouslyNeeded = false;
    	}
    	/**
    	 *  render the 
    	 */
    	draw(transform, viewport) {
    		//exception for layout image where we still do not know the image size
    		//how linear or srgb should be specified here.
    		//		gl.pixelStorei(gl.UNPACK_COLORSPACE_CONVERSION_WEBGL, gl.NONE);
    		if (this.status != 'ready')// || this.tiles.size == 0)
    			return true;

    		if (!this.shader)
    			throw "Shader not specified!";

    		let done = this.interpolateControls();
    		this.prepareWebGL();

    		//		find which quads to draw and in case request for them
    		transform = this.transform.compose(transform);
    		let needed = this.layout.neededBox(viewport, transform, 0, this.mipmapBias);
    		let torender = this.toRender(needed);

    		let matrix = transform.projectionMatrix(viewport);
    		this.gl.uniformMatrix4fv(this.shader.matrixlocation, this.gl.FALSE, matrix);

    		for (let index in torender) {
    			torender[index];
    			//			if(tile.complete)
    			this.drawTile(torender[index]);
    		}

    		//		gl.uniform1f(t.opacitylocation, t.opacity);
    		return done;
    	}

    	drawTile(tile) {
    		let tiledata = this.tiles.get(tile.index);
    		if (tiledata.missing != 0)
    			throw "Attempt to draw tile still missing textures"

    		//TODO might want to change the function to oaccept tile as argument
    		let c = this.layout.tileCoords(tile.level, tile.x, tile.y);

    		//update coords and texture buffers
    		this.updateTileBuffers(c.coords, c.tcoords);

    		//bind textures
    		let gl = this.gl;
    		for (var i = 0; i < this.shader.samplers.length; i++) {
    			let id = this.shader.samplers[i].id;
    			gl.uniform1i(this.shader.samplers[i].location, i);
    			gl.activeTexture(gl.TEXTURE0 + i);
    			gl.bindTexture(gl.TEXTURE_2D, tiledata.tex[id]);
    		}
    		gl.drawElements(gl.TRIANGLES, 6, gl.UNSIGNED_SHORT, 0);
    	}

    	/* given the full pyramid of needed tiles for a certain bounding box, 
    	 *  starts from the preferred levels and goes up in the hierarchy if a tile is missing.
    	 *  complete is true if all of the 'brothers' in the hierarchy are loaded,
    	 *  drawing incomplete tiles enhance the resolution early at the cost of some overdrawing and problems with opacity.
    	 */

    	toRender(needed) {

    		let torender = {}; //array of minlevel, actual level, x, y (referred to minlevel)
    		let brothers = {};

    		let minlevel = needed.level;
    		let box = needed.pyramid[minlevel];

    		for (let y = box.yLow; y < box.yHigh; y++) {
    			for (let x = box.xLow; x < box.xHigh; x++) {
    				let level = minlevel;
    				while (level >= 0) {
    					let d = minlevel - level;
    					let index = this.layout.index(level, x >> d, y >> d);
    					if (this.tiles.has(index) && this.tiles.get(index).missing == 0) {
    						torender[index] = { index: index, level: level, x: x >> d, y: y >> d, complete: true };
    						break;
    					} else {
    						let sx = (x >> (d + 1)) << 1;
    						let sy = (y >> (d + 1)) << 1;
    						brothers[this.layout.index(level, sx, sy)] = 1;
    						brothers[this.layout.index(level, sx + 1, sy)] = 1;
    						brothers[this.layout.index(level, sx + 1, sy + 1)] = 1;
    						brothers[this.layout.index(level, sx, sy + 1)] = 1;
    					}
    					level--;
    				}
    			}
    		}
    		for (let index in brothers) {
    			if (index in torender)
    				torender[index].complete = false;
    		}
    		return torender;
    	}


    	updateTileBuffers(coords, tcoords) {
    		let gl = this.gl;
    		//TODO to reduce the number of calls (probably not needed) we can join buffers, and just make one call per draw! (except the bufferData, which is per node)
    		gl.bindBuffer(gl.ARRAY_BUFFER, this.vbuffer);
    		gl.bufferData(gl.ARRAY_BUFFER, coords, gl.STATIC_DRAW);

    		gl.vertexAttribPointer(this.shader.coordattrib, 3, gl.FLOAT, false, 0, 0);
    		gl.enableVertexAttribArray(this.shader.coordattrib);

    		gl.bindBuffer(gl.ARRAY_BUFFER, this.tbuffer);
    		gl.bufferData(gl.ARRAY_BUFFER, tcoords, gl.STATIC_DRAW);

    		gl.vertexAttribPointer(this.shader.texattrib, 2, gl.FLOAT, false, 0, 0);
    		gl.enableVertexAttribArray(this.shader.texattrib);
    	}



    	/**
    	 *  If layout is ready and shader is assigned, creates or update tiles to keep track of what is missing.
    	 */
    	setupTiles() {
    		if (!this.shader || !this.layout || this.layout.status != 'ready')
    			return;

    		// if(!this.tiles.size) {
    		// 	 this.tiles = JSON.parse(JSON.stringify(this.layout.tiles));
    		// 	 for(let tile of this.tiles) {
    		// 	 	tile.tex = new Array(this.shader.samplers.length);
    		// 	 	tile.missing = this.shader.samplers.length;
    		//  		tile.size = 0;
    		//  	}
    		//  	return;
    		// }

    		for (let tile of this.tiles) {
    			tile.missing = this.shader.samplers.length;			for (let sampler of this.shader.samplers) {
    				if (tile.tex[sampler.id])
    					tile.missing--;
    			}
    		}
    	}

    	prepareWebGL() {

    		let gl = this.gl;

    		if (!this.ibuffer) { //this part might go into another function.
    			this.ibuffer = gl.createBuffer();
    			gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, this.ibuffer);
    			gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, new Uint16Array([3, 2, 1, 3, 1, 0]), gl.STATIC_DRAW);

    			this.vbuffer = gl.createBuffer();
    			gl.bindBuffer(gl.ARRAY_BUFFER, this.vbuffer);
    			gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0]), gl.STATIC_DRAW);

    			this.tbuffer = gl.createBuffer();
    			gl.bindBuffer(gl.ARRAY_BUFFER, this.tbuffer);
    			gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([0, 0, 0, 1, 1, 1, 1, 0]), gl.STATIC_DRAW);
    		}

    		if (this.shader.needsUpdate)
    			this.shader.createProgram(gl);

    		gl.useProgram(this.shader.program);
    		this.shader.updateUniforms(gl, this.shader.program);


    	}

    	sameNeeded(a, b) {
    		if(a.level != b.level)
    			return false;

    		for(let p of ['xLow', 'xHigh', 'yLow', 'yHigh'])
    			if(a.pyramid[a.level][p] != b.pyramid[a.level][p])
    				return false;
    		
    		return true;
    	}
    	/**
    	*  @param {object] transform is the canvas coordinate transformation
    	*  @param {viewport} is the viewport for the rendering, note: for lens might be different! Where we change it? here layer should know!
    	*/
    	prefetch(transform, viewport) {
    		if (this.layers.length != 0) { //combine layers
    			for (let layer of this.layers)
    				layer.prefetch(transform, viewport);
    		}

    		if (this.rasters.length == 0)
    			return;

    		if (this.status != 'ready')
    			return;

    		if (typeof (this.layout) != 'object')
    			throw "AH!";

    		let needed = this.layout.neededBox(viewport, transform, this.prefetchBorder, this.mipmapBias);
    		if (this.previouslyNeeded && this.sameNeeded(this.previouslyNeeded, needed))
    			return;
    		this.previouslyNeeded = needed;

    		this.queue = [];
    		let now = performance.now();
    		//look for needed nodes and prefetched nodes (on the pos destination
    		let missing = this.shader.samplers.length;

    		for (let level = 0; level <= needed.level; level++) {
    			let box = needed.pyramid[level];
    			let tmp = [];
    			for (let y = box.yLow; y < box.yHigh; y++) {
    				for (let x = box.xLow; x < box.xHigh; x++) {
    					let index = this.layout.index(level, x, y);
    					let tile = this.tiles.get(index) || { index, x, y, missing, tex: [], level };
    					tile.time = now;
    					tile.priority = needed.level - level;
    					if (tile.missing != 0 && !this.requested[index])
    						tmp.push(tile);
    				}
    			}
    			let c = box.center();
    			//sort tiles by distance to the center TODO: check it's correct!
    			tmp.sort(function (a, b) { return Math.abs(a.x - c[0]) + Math.abs(a.y - c[1]) - Math.abs(b.x - c[0]) - Math.abs(b.y - c[1]); });
    			this.queue = this.queue.concat(tmp);
    		}
    		Cache.setCandidates(this);
    	}

    	async loadTile(tile, callback) {
    		if (this.tiles.has(tile.index))
    			throw "AAARRGGHHH double tile!";

    		if (this.requested[tile.index])
    			throw "AAARRGGHHH double request!";

    		this.tiles.set(tile.index, tile);
    		this.requested[tile.index] = true;

    		if (this.layout.type == 'itarzoom') {
    			tile.url = this.layout.getTileURL(null, tile);
    			let options = {};
    			if (tile.end)
    				options.headers = { range: `bytes=${tile.start}-${tile.end}`, 'Accept-Encoding': 'indentity' };

    			var response = await fetch(tile.url, options);
    			if (!response.ok) {
    				callback("Failed loading " + tile.url + ": " + response.statusText);
    				return;
    			}
    			let blob = await response.blob();

    			let i = 0;
    			for (let sampler of this.shader.samplers) {
    				let raster = this.rasters[sampler.id];
    				let imgblob = blob.slice(tile.offsets[i], tile.offsets[i + 1]);
    				const img = await raster.blobToImage(imgblob, this.gl);
    				let tex = raster.loadTexture(this.gl, img);
    				let size = img.width * img.height * 3;
    				tile.size += size;
    				tile.tex[sampler.id] = tex;
    				i++;
    			}
    			tile.missing = 0;
    			this.emit('update');
    			delete this.requested[tile.index];
    			if (callback) callback(tile.size);
    			return;
    		}

    		for (let sampler of this.shader.samplers) {

    			let raster = this.rasters[sampler.id];
    			tile.url = this.layout.getTileURL(sampler.id, tile);
    			const [tex, size] = await raster.loadImage(tile, this.gl);
    			if (this.layout.type == "image") {
    				this.layout.width = raster.width;
    				this.layout.height = raster.height;
    				this.layout.initBoxes();
    			}
    			tile.size += size;
    			tile.tex[sampler.id] = tex;
    			tile.missing--;
    			if (tile.missing <= 0) {
    				this.emit('update');
    				delete this.requested[tile.index];
    				if (callback) callback(size);
    			}
    		}
    	}

    }

    Layer.prototype.types = {};

    /**
     * @param {Element|String} canvas dom element or query selector for a <canvas> element.
     * @param {Element} overlay DIV containing annotations, TODO: at the moment it is just passed to the layers (might need refactoring)
     * @param {Camera} camera (see {@link Camera})
     * @param {Object} options
     * * *layers*: Object specifies layers (see. {@link Layer})
     * * *preserveDrawingBuffer* needed for screenshots (otherwise is just a performance penalty)
     * 
     * **Signals:**
     * Emits *"update"* event when a layer updates or is added or removed.
     * 
     */

    class Canvas {
    	constructor(canvas, overlay, camera, options) {
    		Object.assign(this, { 
    			canvasElement: null,
    			preserveDrawingBuffer: false, 
    			gl: null,
    			overlayElement: overlay,
    			camera: camera,
    			layers: {},
    			signals: {'update':[], 'updateSize':[], 'ready': []}
    		});
    		Object.assign(this, options);

    		this.init(canvas);
    			
    		for(let id in this.layers)
    		this.addLayer(id, new Layer(id, this.layers[id]));
    		this.camera.addEvent('update', () => this.emit('update'));
    	}

    	addEvent(event, callback) {
    		this.signals[event].push(callback);
    	}
    	
    	emit(event) {
    		for(let r of this.signals[event])
    			r(this);
    	}

    	//TODO move gl context to canvas!
    	init(canvas) {
    		if(!canvas)
    			throw "Missing element parameter"

    		if(typeof(canvas) == 'string') {
    			canvas = document.querySelector(canvas);
    			if(!canvas)
    				throw "Could not find dom element.";
    		}

    		if(!canvas.tagName)
    			throw "Element is not a DOM element"

    		if(canvas.tagName != "CANVAS")
    			throw "Element is not a canvas element";

    		this.canvasElement = canvas;

    		/* test context loss */
    		/* canvas = WebGLDebugUtils.makeLostContextSimulatingCanvas(canvas);
    		canvas.loseContextInNCalls(1000); */


    		let glopt = { antialias: false, depth: false, preserveDrawingBuffer: this.preserveDrawingBuffer };
    		this.gl = this.gl || 
    			canvas.getContext("webgl2", glopt) || 
    			canvas.getContext("webgl", glopt) || 
    			canvas.getContext("experimental-webgl", glopt) ;

    		if (!this.gl)
    			throw "Could not create a WebGL context";

    		canvas.addEventListener("webglcontextlost", (event) => { console.log("Context lost."); event.preventDefault(); }, false);
    		canvas.addEventListener("webglcontextrestored", ()  => { this.restoreWebGL(); }, false);
    		document.addEventListener("visibilitychange", (event) => { if(this.gl.isContextLost()) { this.restoreWebGL(); }});

    		/* DEBUG OpenGL calls */
    		/*function logGLCall(functionName, args) {   
    			console.log("gl." + functionName + "(" + 
    			WebGLDebugUtils.glFunctionArgsToString(functionName, args) + ")");   
    		} 
    		this.gl = WebGLDebugUtils.makeDebugContext(this.gl, undefined, logGLCall);  */


    	}

    	restoreWebGL() {
    		let glopt = { antialias: false, depth: false, preserveDrawingBuffer: this.preserveDrawingBuffer };
    		this.gl = this.gl || 
    			canvas.getContext("webgl2", glopt) || 
    			canvas.getContext("webgl", glopt) || 
    			canvas.getContext("experimental-webgl", glopt) ;

    		for(let layer of Object.values(this.layers)) {
    			layer.gl = this.gl;
    			layer.clear();
    			if(layer.shader)
    				layer.shader.restoreWebGL(this.gl);
    		}
    		this.prefetch();
    		this.emit('update');
    	}

    	addLayer(id, layer) {
    		layer.id = id;
    		layer.addEvent('ready', () => { 
    			if(Object.values(this.layers).every( l => l.status == 'ready'))
    				this.emit('ready');
    			this.prefetch();
    		});
    		layer.addEvent('update', () => { this.emit('update'); });
    		layer.addEvent('updateSize', () => { this.updateSize(); });
    		layer.gl = this.gl;
    		layer.overlayElement = this.overlayElement;
    		this.layers[id] = layer;
    		this.prefetch();
    	}

    	removeLayer(layer) {
    		layer.clear(); //order is important.

    		delete this.layers[layer.id];
    		delete Cache.layers[layer];
    		this.prefetch();
    	}

    	updateSize() {
    		const discardHidden = true;
    		let sceneBBox = Layer.computeLayersBBox(this.layers, discardHidden);
    		let minScale =  Layer.computeLayersMinScale(this.layers, discardHidden);
    		
    		if (sceneBBox != null) this.camera.updateBounds(sceneBBox, minScale);
    		this.emit('updateSize');
    	}

    	draw(time) {
    		let gl = this.gl;
    		let view = this.camera.glViewport();
    		gl.viewport(view.x, view.y, view.dx, view.dy);

    		var b = [0, 0, 0, 0];
    		gl.clearColor(b[0], b[1], b[2], b[3], b[4]);
    		gl.clear(gl.COLOR_BUFFER_BIT);

    		gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);
    		gl.enable(gl.BLEND);

    		//TODO: getCurren shoudl redurn {position, done}
    		let pos = this.camera.getGlCurrentTransform(time);
    		//todo we could actually prefetch toward the future a little bit
    		this.prefetch(pos);

    		//pos layers using zindex.
    		let ordered = Object.values(this.layers).sort( (a, b) => a.zindex - b.zindex);

    		//NOTICE: camera(pos) must be relative to the WHOLE canvas
    		let done = true;
    		for(let layer of ordered)
    			if(layer.visible)
    				done = layer.draw(pos, view) && done;

    		//TODO not really an elegant solution to tell if we have reached the target, the check should be in getCurrentTransform.
    		return done && pos.t >= this.camera.target.t;
    	}

    /**
     * This function have each layer to check which tiles are needed and schedule them for download.
     * @param {object} transform is the camera position (layer will combine with local transform).
     */
    	prefetch(transform) {
    		if(!transform)
    			transform = this.camera.getGlCurrentTransform(performance.now());
    		for(let id in this.layers) {
    			let layer = this.layers[id];
    			//console.log(layer);
    			//console.log(layer.layout.status);
    			if(layer.visible && layer.status == 'ready') {
    				layer.prefetch(transform, this.camera.glViewport());
    			}
    		}
    	}
    }

    /**
     * Raster is a providers of images and planes of coefficies.
     * It support all files format supported by browser and a set of tiled formats.
     *
     * Layout can be:
     * * image: a single image (.jpg, .png etc.)
     * * google: there is no config file, so layout, suffix is mandatory if not .jpg,  and url is the base folder of the tiles.
     * * deepzoom: requires only url, can be autodetected from the extension (.dzi)
     * * zoomify: requires url, can be autodetected, from the ImageProperties.xml, suffix is required if not .jpg
     * * iip: requires url, can be autodetected from the url
     * * iiif: layout is mandatory, url should point to base url {scheme}://{server}{/prefix}/{identifier}
     *
     * @param {string} id an unique id for each raster
     * @param {url} url of the content
     * @param {object} options 
     * * *type*: vec3 (default value) for standard images, vec4 when including alpha, vec2, float other purpouses.
     * * *attribute*: <coeff|kd|ks|gloss|normals|dem> meaning of the image.
     * * *colorSpace*: <linear|srgb> colorspace used for rendering.
     */

    class Raster {

    	constructor(options) {

    		Object.assign(this, { 
    			format: 'vec3', 
    			colorSpace: 'linear',
    			attribute: 'kd'
    		 });

    		Object.assign(this, options);
    	}


    	async loadImage(tile, gl) {
    		let img;
    		let cors = (new URL(tile.url, window.location.href)).origin !== window.location.origin;
    		if (tile.end || typeof createImageBitmap == 'undefined') {
    			let options = {};
    			options.headers = { range: `bytes=${tile.start}-${tile.end}`, 'Accept-Encoding': 'indentity', mode: cors? 'cors' : 'same-origin' };
    			let response = await fetch(tile.url, options);
    			if (!response.ok) {
    				callback("Failed loading " + tile.url + ": " + response.statusText);
    				return;
    			}

    			if (response.status != 206)
    				throw "The server doesn't support partial content requests (206).";

    			let blob = await response.blob();
    			img = await this.blobToImage(blob, gl);
    		} else {
    			img = document.createElement('img');
    			if (cors) img.crossOrigin="";
    			img.onerror = function (e) { console.log("Texture loading error!"); };
    			img.src = tile.url;
    			await new Promise((resolve, reject) => { img.onload = () => resolve(); });
    		}
    		let tex = this.loadTexture(gl, img);
    		//TODO 3 is not accurate for type of image, when changing from rgb to grayscale, fix this value.
    		let size = img.width * img.height * 3;
    		return [tex, size];	
    	}

    	async blobToImage(blob, gl) {
    		let img;
    		if(typeof createImageBitmap != 'undefined') {
    			var isFirefox = typeof InstallTrigger !== 'undefined';
    			//firefox does not support options for this call, BUT the image is automatically flipped.
    			if(isFirefox)
    				img = await createImageBitmap(blob); 
    			else
    				img = await createImageBitmap(blob, { imageOrientation1: 'flipY' });

    		} else { //fallback for IOS
    			let urlCreator = window.URL || window.webkitURL;
    			img = document.createElement('img');
    			img.onerror = function(e) { console.log("Texture loading error!"); };
    			img.src = urlCreator.createObjectURL(blob);

    			await new Promise((resolve, reject) => { img.onload = () => resolve(); });
    			urlCreator.revokeObjectURL(img.src);
    			
    		}
    		return img;		
    	}
    /*
     * @param {function} callback as function(tex, sizeinBytes)
     */

    	loadTexture(gl, img) {
    		this.width = img.width;  //this will be useful for layout image.
    		this.height = img.height;

    		var tex = gl.createTexture();
    		gl.bindTexture(gl.TEXTURE_2D, tex);
    		gl.texParameterf(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
    		gl.texParameterf(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR); //_MIPMAP_LINEAR);
    		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    		let glFormat = gl.RGBA;
    		switch(this.format) {
    			case 'vec3':
    				glFormat = gl.RGB;
    				break;
    			case 'vec4':
    				glFormat = gl.RGBA;
    				break;
    			case 'float':
    				glFormat = gl.LUMINANCE;
    				break;
    		} 
    		gl.texImage2D(gl.TEXTURE_2D, 0, glFormat, glFormat, gl.UNSIGNED_BYTE, img);
    		return tex;
    	}
    }

    /**
     *  @param {object} options
     * *label*: used for menu
     * *samplers*: array of rasters {id:, type: } color, normals, etc.
     * *uniforms*: type = <vec4|vec3|vec2|float|int>, needsUpdate controls when updated in gl, size is unused, value is and array or a float, 
     *             we also want to support interpolation: source (value is the target), start, end are the timing (same as camera interpolation)
     * *body*: code actually performing the rendering, needs to return a vec4
     * *name*: name of the body function
     */

    class Shader {
    	constructor(options) {
    		Object.assign(this, {
    			version: 100,   //check for webglversion.
    			samplers: [],
    			uniforms: {},
    			name: "",
    			program: null,      //webgl program
    			modes: [],
    			needsUpdate: true,
    			signals: { 'update':[] }
    		});

    		Object.assign(this, options);
    	}

    	setEvent(event, callback) {
    		this.signals[event] = [callback];
    	}

    	emit(event) {
    		for(let r of this.signals[event])
    			r(this);
    	}

    	restoreWebGL(gl) {
    		this.createProgram(gl);
    	}

    	setUniform(name, value) {
    		let u = this.uniforms[name];
    		if(typeof(value) == "number" && u.value == value) 
    			return;
    		if(Array.isArray(value) && Array.isArray(u.value) && value.length == u.value.length) {
    			let equal = true;
    			for(let i = 0; i < value.length; i++)
    				if(value[i] != u.value[i]) {
    					equal = false;
    					break;
    				}
    			if(equal)
    				return;
    		}

    		u.value = value;
    		u.needsUpdate = true;
    		this.emit('update');
    	}

    	createProgram(gl) {

    		let vert = gl.createShader(gl.VERTEX_SHADER);
    		gl.shaderSource(vert, this.vertShaderSrc(gl));

    		gl.compileShader(vert);
    		let compiled = gl.getShaderParameter(vert, gl.COMPILE_STATUS);
    		if(!compiled) {
    			console.log(gl.getShaderInfoLog(vert));
    			throw Error("Failed vertex shader compilation: see console log and ask for support.");
    		}

    		let frag = gl.createShader(gl.FRAGMENT_SHADER);
    		gl.shaderSource(frag, this.fragShaderSrc(gl));
    		gl.compileShader(frag);

    		if(this.program)
    			gl.deleteProgram(this.program);

    		let program = gl.createProgram();

    		gl.getShaderParameter(frag, gl.COMPILE_STATUS);
    		compiled = gl.getShaderParameter(frag, gl.COMPILE_STATUS);
    		if(!compiled) {
    			console.log(this.fragShaderSrc());
    			console.log(gl.getShaderInfoLog(frag));
    			throw Error("Failed fragment shader compilation: see console log and ask for support.");
    		}

    		gl.attachShader(program, vert);
    		gl.attachShader(program, frag);
    		gl.linkProgram(program);

    		if ( !gl.getProgramParameter( program, gl.LINK_STATUS) ) {
    			var info = gl.getProgramInfoLog(program);
    			throw new Error('Could not compile WebGL program. \n\n' + info);
    		}

    		//sampler units;
    		for(let sampler of this.samplers)
    			sampler.location = gl.getUniformLocation(program, sampler.name);

    		this.coordattrib = gl.getAttribLocation(program, "a_position");
    		gl.vertexAttribPointer(this.coordattrib, 3, gl.FLOAT, false, 0, 0);
    		gl.enableVertexAttribArray(this.coordattrib);

    		this.texattrib = gl.getAttribLocation(program, "a_texcoord");
    		gl.vertexAttribPointer(this.texattrib, 2, gl.FLOAT, false, 0, 0);
    		gl.enableVertexAttribArray(this.texattrib);

    		this.matrixlocation = gl.getUniformLocation(program, "u_matrix");

    		this.program = program;
    		this.needsUpdate = false;

    		for(let uniform of Object.values(this.uniforms)) {
    			uniform.location = null;
    			uniform.needsUpdate = true;
    		}
    	}

    	updateUniforms(gl, program) {
    		performance.now();
    		for(const [name, uniform] of Object.entries(this.uniforms)) {
    			if(!uniform.location)
    				uniform.location = gl.getUniformLocation(program, name);

    			if(!uniform.location)  //uniform not used in program
    				continue; 

    			if(uniform.needsUpdate) {
    				let value = uniform.value;
    				switch(uniform.type) {
    					case 'vec4':  gl.uniform4fv(uniform.location, value); break;
    					case 'vec3':  gl.uniform3fv(uniform.location, value); break;
    					case 'vec2':  gl.uniform2fv(uniform.location, value); break;
    					case 'float': gl.uniform1f(uniform.location, value); break;
    					case 'int':   gl.uniform1i (uniform.location, value); break;
    					default: throw Error('Unknown uniform type: ' + u.type);
    				}
    			}
    		}
    	}

    	vertShaderSrc(gl) {
    		let gl2 = !(gl instanceof WebGLRenderingContext);
    		return `${gl2? '#version 300 es':''}

precision highp float; 
precision highp int; 

uniform mat4 u_matrix;
${gl2? 'in' : 'attribute'} vec4 a_position;
${gl2? 'in' : 'attribute'} vec2 a_texcoord;

${gl2? 'out' : 'varying'} vec2 v_texcoord;

void main() {
	gl_Position = u_matrix * a_position;
	v_texcoord = a_texcoord;
}`;
    	}

    	fragShaderSrc(gl) {
    		throw 'Unimplemented!'
    	}
    }

    /**
     * Display a simple image. Extends {@link Layer}.
     * @param {options} options Same as {@link Layer}, but url and layout are required.
     */

    class LayerImage extends Layer {
    	constructor(options) {
    		super(options);

    		if(Object.keys(this.rasters).length != 0)
    			throw "Rasters options should be empty!";

    		if(!this.url)
    			throw "Url option is required";

    		this.layout.setUrls([this.url]);
    		const rasterFormat = this.format != null ? this.format : 'vec4';
    		let raster = new Raster({ format: rasterFormat, attribute: 'kd', colorspace: 'sRGB' });

    		this.rasters.push(raster);
    		

    		let shader = new Shader({
    			'label': 'Rgb',
    			'samplers': [{ id:0, name:'kd', type: rasterFormat }]
    		});
    		
    		shader.fragShaderSrc = function(gl) {

    			let gl2 = !(gl instanceof WebGLRenderingContext);
    			let str = `${gl2? '#version 300 es' : ''}

precision highp float;
precision highp int;

uniform sampler2D kd;

${gl2? 'in' : 'varying'} vec2 v_texcoord;
${gl2? 'out' : ''} vec4 color;


void main() {
	color = texture${gl2?'':'2D'}(kd, v_texcoord);
	${gl2? '':'gl_FragColor = color;'}
}
`;
    			return str;

    		};

    		this.shaders = {'standard': shader };
    		this.setShader('standard');
    	}
    }

    Layer.prototype.types['image'] = (options) => { return new LayerImage(options); };

    /**
     * Combines other layers (using a framebuffer) using a shader. Lens is an example. Extends {@link Layer}.
     * @param {options} options Same as {@link Layer}, but url and layout are required.
     */
    class LayerCombiner extends Layer {
    	constructor(options) {
    		super(options);

    		if(Object.keys(this.rasters).length != 0)
    			throw "Rasters options should be empty!";

    /*		let shader = new ShaderCombiner({
    			'label': 'Combiner',
    			'samplers': [{ id:0, name:'source1', type:'vec3' }, { id:1, name:'source2', type:'vec3' }],
    		});

    		this.shaders = {'standard': shader };
    		this.setShader('standard'); */

    //todo if layers check for importjson

    		this.textures = [];
    		this.framebuffers = [];
    		this.status = 'ready';
    	}


    	draw(transform, viewport) {
    		for(let layer of this.layers)
    			if(layer.status != 'ready')
    				return;

    		if(!this.shader)
    			throw "Shader not specified!";

    		let w = viewport.dx;
    		let h = viewport.dy;

    		if(!this.framebuffers.length || this.layout.width != w || this.layout.height != h) {
    			this.deleteFramebuffers();
    			this.layout.width = w;
    			this.layout.height = h;
    			this.createFramebuffers();
    		}

    		let gl = this.gl;
    		var b = [0, 0, 0, 0];
    		gl.clearColor(b[0], b[1], b[2], b[3]);


    //TODO optimize: render to texture ONLY if some parameters change!
    //provider di textures... max memory and reference counting.

    		for(let i = 0; i < this.layers.length; i++) { 
    			gl.bindFramebuffer(gl.FRAMEBUFFER, this.framebuffers[i]);
    			gl.clear(gl.COLOR_BUFFER_BIT);
    			this.layers[i].draw(transform, {x:0, y:0, dx:w, dy:h, w:w, h:h});
    			gl.bindFramebuffer(gl.FRAMEBUFFER, null);
    		}


    		this.prepareWebGL();

    		for(let i = 0; i < this.layers.length; i++) {
    			gl.uniform1i(this.shader.samplers[i].location, i);
    			gl.activeTexture(gl.TEXTURE0 + i);
    			gl.bindTexture(gl.TEXTURE_2D, this.textures[i]);
    		}



    		this.updateTileBuffers(
    			new Float32Array([-1, -1, 0,  -1, 1, 0,  1, 1, 0,  1, -1, 0]), 
    			new Float32Array([ 0,  0,      0, 1,     1, 1,     1,  0]));
    		gl.drawElements(gl.TRIANGLES, 6, gl.UNSIGNED_SHORT,0);
    	}

    	
    	createFramebuffers() {
    		let gl = this.gl;
    		for(let i = 0; i < this.layers.length; i++) {
    			//TODO for thing like lens, we might want to create SMALLER textures for some layers.
    			const texture = gl.createTexture();

    			gl.bindTexture(gl.TEXTURE_2D, texture);

    			const level = 0;
    			const internalFormat = gl.RGBA;
    			const border = 0;
    			const format = gl.RGBA;
    			const type = gl.UNSIGNED_BYTE;
    			gl.texImage2D(gl.TEXTURE_2D, level, internalFormat,
    				this.layout.width, this.layout.height, border, format, type, null);

    			gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
    			gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    			gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);

    			const framebuffer = gl.createFramebuffer();
    			gl.bindFramebuffer(gl.FRAMEBUFFER, framebuffer);
    			gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0, gl.TEXTURE_2D, texture, 0);
    			gl.bindFramebuffer(gl.FRAMEBUFFER, null);

    			this.textures[i] = texture;
    			this.framebuffers[i] = framebuffer;
    		}
    	}
    	//TODO release textures and framebuffers
    	deleteFramebuffers() {
    	}

    	boundingBox() {
    		// Combiner ask the combination of all its children boxes
    		// keeping the hidden, because they could be hidden, but revealed by the combiner
    		const discardHidden = false;
    		let result = Layer.computeLayersBBox(this.layers, discardHidden);
    		if (this.transform != null && this.transform != undefined) {
    			result = this.transform.transformBox(result);
    		}
    		return result;
    	}
    	
    	scale() {
    		//Combiner ask the scale of all its children
    		//keeping the hidden, because they could be hidden, but revealed by the combiner
    		const discardHidden = false;
    		let scale = Layer.computeLayersMinScale(this.layers, discardHidden);
    		scale *= this.transform.z;
    		return scale;
    	}
    }

    Layer.prototype.types['combiner'] = (options) => { return new LayerCombiner(options); };

    /**
     *  @param {object} options
     * *compose*: compose operation: add, subtract, multiply, etc.
     */

    class ShaderCombiner extends Shader {
    	constructor(options) {
    		super(options);

    		this.mode = 'mean', //Lighten Darken Contrast Inversion HSV components LCh components
    		this.samplers = [
    			{ id:0, name:'source1', type:'vec3' },
    			{ id:1, name:'source2', type:'vec3' }
    		];

    		this.modes = ['first','second','mean','diff'];
    		this.operations = {
    			'first': 'color = c1;',
    			'second': 'color = c2;',
    			'mean': 'color = (c1 + c2)/2.0;',
    			'diff': 'color = vec4(c2.rgb - c1.rgb, c1.a);'
    		};
    	}

    	setMode(mode) {
    //		if(!(mode in this.modes))
    		if(this.modes.indexOf(mode)==-1)
    			throw Error("Unknown mode: " + mode);
    		this.mode = mode;
    		this.needsUpdate = true;
    	}

    	fragShaderSrc(gl) {
    		let gl2 = !(gl instanceof WebGLRenderingContext);
    		let operation = this.operations[this.mode];
    		return `${gl2? '#version 300 es' : ''}

precision highp float; 
precision highp int; 

${gl2? 'in' : 'varying'} vec2 v_texcoord;

uniform sampler2D source1;
uniform sampler2D source2;

${gl2? 'out' : ''} vec4 color;

void main() {
	vec4 c1 = texture(source1, v_texcoord);
	vec4 c2 = texture(source2, v_texcoord);
	${operation};
	${gl2?'':'gl_FragColor = color;'}
}
`;
    	}

    	vertShaderSrc(gl) {
    		let gl2 = !(gl instanceof WebGLRenderingContext);
    		return `${gl2? '#version 300 es':''}

precision highp float; 
precision highp int; 

${gl2? 'in' : 'attribute'} vec4 a_position;
${gl2? 'in' : 'attribute'} vec2 a_texcoord;

${gl2? 'out' : 'varying'} vec2 v_texcoord;

void main() {
	gl_Position = a_position;
	v_texcoord = a_texcoord;
}`;
    	}
    }

    /**
     * Virtual nase class for controllers: classes that handle mouse and touch events and links to pan, zoom, etc.
     * Callbacks supporte are:
     * * *panStart(event)* calling event.preventDefault() will capture the panning gestire
     * * *panMove(event)*
     * * *panEnd(event)*
     * * *pinchStart(event)* calling event.preventDefault() will capture the pinch gestire
     * * *pinchMove(event)*
     * * *pinchEnd(event)*
     * * *wheelDelta(event)*
     * * *singleTap(event)*
     * * *wheelDelta(event)*
     * * *doubleTap(event)*
     * * *resize(event)*
     * 
     * In general event.preventDefault() will capture the event and wont be propagated to other controllers.

     * 
     * @param {options} options 
     * * *panDelay* inertia of the movement in ms for panning movements (default 100)
     * * *zoomDelay* a zoom event is smoothed over this delay in ms (default 200)
     * * *priority* higher priority controllers are invoked in advance.
     */

    class Controller {
    	constructor(options) {

    /*	For some reason can't define these variables static, for the moment just use the numeric value.
    	static NoModifiers = 0;
    	static CrtlModifier = 1;
    	static ShiftModifier = 2;
    	static AltModifier = 4; */

    		Object.assign(this, {
    			active: true,
    			debug: false,
    			panDelay: 50,
    			zoomDelay: 200,
    			priority: 0,
    			activeModifiers: [0]
    		});

    		Object.assign(this, options);

    	}

    	modifierState(e) {
    		let state = 0;
    		if(e.ctrlKey) state += 1;
    		if(e.shiftKey) state += 2;
    		if(e.altKey) state += 4;
    		
    		return state;
    	}

    	captureEvents() {
    		this.capture = true; //TODO should actually specify WHAT it is capturing: which touch etc.
    	}

    	releaseEvents() {
    		this.capture = false;
    	}

    /* Implement these functions to interacts with mouse/touch/resize events. */

    }

    /*
     * Controller that turn the position of the mouse on the screen to a [0,1]x[0,1] parameter
     * @param {Function} callback 
     * Options: relative (
     */

    function clamp(value, min, max) {
    	return Math.max(min, Math.min(max, value));
    }

    class Controller2D extends Controller {

    	constructor(callback, options) {
    		super(options);
    		Object.assign(this, { relative: false, speed: 2.0, start_x: 0, start_y: 0, current_x: 0, current_y: 0 }, options);

    		//By default the controller is active only with no modifiers.
    		//you can select which subsets of the modifiers are active.
    		this.callback = callback;
    		
    		if(!this.box) {
    			this.box = new BoundingBox({xLow:-0.99, yLow: -0.99, xHigh: 0.99, yHigh: 0.99});
    		}

    		this.panning = false;
    	}
    	//TODO find a decent name for functions and variables!
    	setPosition(x, y) {
    		this.current_x = x;
    		this.current_y = y;
    		this.callback(x, y);
    	}

    	project(e) {
    		let rect = e.target.getBoundingClientRect();
    		let x = 2*e.offsetX/rect.width - 1;
    		let y = 2*(1 - e.offsetY/rect.height) -1;
    		return [x, y]
    	}

    	update(e) {
    		let [x, y] = this.project(e);

    		if(this.relative) {
    			x = clamp(this.speed*(x - this.start_x) + this.current_x, -1, 1);
    			y = clamp(this.speed*(y - this.start_y) + this.current_y, -1, 1);
    		}

    		this.callback(x, y);
    	}

    	panStart(e) {
    		if(!this.active || !this.activeModifiers.includes(this.modifierState(e)))
    			return;

    		if(this.relative) {
    			let [x, y] = this.project(e);
    			this.start_x = x;
    			this.start_y = y;
    		}
    		
    		this.update(e);
    		this.panning = true;
    		e.preventDefault();
    	}

    	panMove(e) {
    		if(!this.panning)
    			return false;
    		this.update(e);
    	}

    	panEnd(e) {
    		if(!this.panning)
    			return false;
    		this.panning = false;
    		if(this.relative) {
    			let [x, y] = this.project(e);
    			this.current_x = clamp(this.speed*(x - this.start_x) + this.current_x, -1, 1);
    			this.current_y = clamp(this.speed*(y - this.start_y) + this.current_y, -1, 1);
    		}
    	}

    	fingerSingleTap(e) {
    		if(!this.active || !this.activeModifiers.includes(this.modifierState(e)))
    			return;
    		if(this.relative)
    			return;
    		this.update(e);
    		e.preventDefault();
    	}

    }

    class ControllerPanZoom extends Controller {

    	constructor(camera, options) {
    		super(options);

    		this.camera = camera;
    		this.zoomAmount = 1.2;          //for wheel or double tap event
    		
    		
    		this.panning = false;           //true if in the middle of a pan
    		this.initialTransform = null;
    		this.startMouse = null;

    		this.zooming = false;           //true if in the middle of a pinch
    		this.initialDistance = 0.0;
    	}

    	panStart(e) {
    		if(!this.active || this.panning || !this.activeModifiers.includes(this.modifierState(e)))
    			return;
    		this.panning = true;

    		this.startMouse = { x: e.offsetX, y: e.offsetY };

    		let now = performance.now();
    		this.initialTransform = this.camera.getCurrentTransform(now);
    		this.camera.target = this.initialTransform.copy(); //stop animation.
    		e.preventDefault();
    	}

    	panMove(e) {
    		if (!this.panning)
    			return;

    		let m = this.initialTransform;
    		let dx = (e.offsetX - this.startMouse.x);
    		let dy = (e.offsetY - this.startMouse.y);
    		
    		this.camera.setPosition(this.panDelay, m.x + dx, m.y + dy, m.z, m.a);
    	}

    	panEnd(e) {
    		this.panning = false;
    	}

    	distance(e1, e2) {
    		return Math.sqrt(Math.pow(e1.x - e2.x, 2) + Math.pow(e1.y - e2.y, 2));
    	}

    	pinchStart(e1, e2) {
    		this.zooming = true;
    		this.initialDistance = Math.max(30, this.distance(e1, e2));
    		e1.preventDefault();
    		//e2.preventDefault(); //TODO this is optional?
    	}

    	pinchMove(e1, e2) {
    		if (!this.zooming)
    			return;
    		let rect1 = e1.target.getBoundingClientRect();
    		let offsetX1 = e1.clientX - rect1.left;
    		let offsetY1 = e1.clientY - rect1.top;
    		let rect2 = e2.target.getBoundingClientRect();
    		let offsetX2 = e2.clientX - rect2.left;
    		let offsetY2 = e2.clientY - rect2.top;
    		const scale = this.distance(e1, e2);
    		const pos = this.camera.mapToScene((offsetX1 + offsetX2)/2, (offsetY1 + offsetY2)/2, this.camera.getCurrentTransform(performance.now()));
    		const dz = scale/this.initialDistance;
    		this.camera.deltaZoom(this.zoomDelay, dz, pos.x, pos.y);
    		this.initialDistance = scale;
    		e1.preventDefault();
    	}

    	pinchEnd(e, x, y, scale) {
    		this.zooming = false;
    		e.preventDefault();
    	}

    	mouseWheel(e) {
    		let delta = -e.deltaY/53;
    		const pos = this.camera.mapToScene(e.offsetX, e.offsetY, this.camera.getCurrentTransform(performance.now()));
    		const dz = Math.pow(this.zoomAmount, delta);		
    		this.camera.deltaZoom(this.zoomDelay, dz, pos.x, pos.y);
    		e.preventDefault();
    	}

    	fingerDoubleTap(e) {
    		if(!this.active || !this.activeModifiers.includes(this.modifierState(e)))
    			return;
    		const pos = this.camera.mapToScene(e.offsetX, e.offsetY, this.camera.getCurrentTransform(performance.now()));
    		const dz = this.zoomAmount;
    		this.camera.deltaZoom(this.zoomDelay, dz, pos.x, pos.y);
    	}

    }

    /**
     * Manages handles simultaneous events from a target. 
     * how do I write more substantial documentation.
     *
     * @param {div} target is the DOM element from which the events are generated
     * @param {object} options is a JSON describing the options
     *  * **diagonal**: default *27*, the screen diagonal in inch
     */
    class PointerManager {
        constructor(target, options) {

            this.target = target;

            Object.assign(this, {
                diagonal: 27,                // Standard monitor 27"
                pinchMaxInterval: 200        // in ms, fingerDown event max distance in time to trigger a pinch.
            });

            if (options)
                Object.assign(this, options);

            this.currentPointers = [];
            this.eventObservers = new Map();
            this.ppmm = PointerManager.getPPMM(this.diagonal);

            this.target.style.touchAction = "none";
            this.target.addEventListener('pointerdown', (e) => this.handleEvent(e), false);
            this.target.addEventListener('pointermove', (e) => this.handleEvent(e), false);
            this.target.addEventListener('pointerup', (e) => this.handleEvent(e), false);
            this.target.addEventListener('pointercancel', (e) => this.handleEvent(e), false);
            this.target.addEventListener('wheel', (e) => this.handleEvent(e), false);
        }

        ///////////////////////////////////////////////////////////
        /// Constants
        static get ANYPOINTER() { return -1; }

        ///////////////////////////////////////////////////////////
        /// Utilities

        static splitStr(str) {
            return str.trim().split(/\s+/g);
        }

        static getPPMM(diagonal) {
            // sqrt(w^2 + h^2) / diagonal / 1in
            return Math.round(Math.sqrt(screen.width **2  + screen.height **2) / diagonal / 25.4);
        }

        ///////////////////////////////////////////////////////////
        /// Class interface

        // register pointer handlers.
        on(eventTypes, obj, idx = PointerManager.ANYPOINTER) {
            eventTypes = PointerManager.splitStr(eventTypes);

            if (typeof (obj) == 'function') {
                obj = Object.fromEntries(eventTypes.map(e => [e, obj]));
                obj.priority = -1000;
            }

            eventTypes.forEach(eventType => {
                if (idx == PointerManager.ANYPOINTER) {
                    this.broadcastOn(eventType, obj);
                } else {
                    const p = this.currentPointers[idx];
                    if (!p) {
                        throw new Error("Bad Index");
                    }
                    p.on(eventType, obj);
                }
            });
            return obj;
        }

        // unregister pointer handlers
        off(eventTypes, callback, idx = PointerManager.ANYPOINTER) {
            if (idx == PointerManager.ANYPOINTER) {
                this.broadcastOff(eventTypes, callback);
            } else {
                PointerManager.splitStr(eventTypes).forEach(eventType => {
                    const p = this.currentPointers[idx];
                    if (!p) {
                        throw new Error("Bad Index");
                    }
                    p.off(eventType, callback);
                });
            }
        }

        onEvent(handler) {
            const cb_properties = ['fingerHover', 'fingerSingleTap', 'fingerDoubleTap', 'fingerHold', 'mouseWheel'];
            if (!handler.hasOwnProperty('priority'))
                throw new Error("Event handler has not priority property");

            if (!cb_properties.some((e) => typeof (handler[e]) == 'function'))
                throw new Error("Event handler properties are wrong or missing");

            for (let e of cb_properties)
                if (typeof (handler[e]) == 'function') {
                    this.on(e, handler);
                }
            if(handler.panStart)
                this.onPan(handler);
            if(handler.pinchStart)
                this.onPinch(handler);
        }

        onPan(handler) {
            const cb_properties = ['panStart', 'panMove', 'panEnd'];
            if (!handler.hasOwnProperty('priority'))
                throw new Error("Event handler has not priority property");

            if (!cb_properties.every((e) => typeof (handler[e]) == 'function'))
                throw new Error("Pan handler is missing one of this functions: panStart, panMove or panEnd");

            handler.fingerMovingStart = (e) => {
                handler.panStart(e);
                if (!e.defaultPrevented) return;
                 this.on('fingerMoving', (e1) => {
                    handler.panMove(e1);
                }, e.idx);
                this.on('fingerMovingEnd', (e2) => {
                    handler.panEnd(e2);
                }, e.idx);
            };
            this.on('fingerMovingStart', handler);
        }

        onPinch(handler) {
            const cb_properties = ['pinchStart', 'pinchMove', 'pinchEnd'];
            if (!handler.hasOwnProperty('priority'))
                throw new Error("Event handler has not priority property");

            if (!cb_properties.every((e) => typeof (handler[e]) == 'function'))
                throw new Error("Pinch handler is missing one of this functions: pinchStart, pinchMove or pinchEnd");

            handler.fingerDown = (e1) => {
                //find other pointers not in moving status
                const filtered = this.currentPointers.filter(cp => cp && cp.idx != e1.idx && cp.status == cp.stateEnum.DETECT);
                if (filtered.length == 0) return;

                //for each pointer search for the last fingerDown event.
                const fingerDownEvents = [];
                for (let cp of filtered) {
                    let down = null;
                    for (let e of cp.eventHistory.toArray())
                        if (e.fingerType == 'fingerDown')
                            down = e;
                    if (down)
                        fingerDownEvents.push(down);
                }
                //we start from the closest one
                //TODO maybe we should sort by distance instead.
                fingerDownEvents.sort((a, b) => b.timeStamp - a.timeStamp);
                for (let e2 of fingerDownEvents) {
                    if (e1.timeStamp - e2.timeStamp > this.pinchInterval) break; 

                    handler.pinchStart(e1, e2);
                    if (!e1.defaultPrevented) break;

                    clearTimeout(this.currentPointers[e1.idx].timeout);
                    clearTimeout(this.currentPointers[e2.idx].timeout);

                    this.on('fingerMovingStart', (e) => e.preventDefault(), e1.idx); //we need to capture this event (pan conflict)
                    this.on('fingerMovingStart', (e) => e.preventDefault(), e2.idx);
                    this.on('fingerMoving',      (e) => e2 && handler.pinchMove(e1 = e, e2), e1.idx); //we need to assign e1 and e2, to keep last position.
                    this.on('fingerMoving',      (e) => e1 && handler.pinchMove(e1, e2 = e), e2.idx);

                    this.on('fingerMovingEnd', (e) => {
                        if (e2)
                            handler.pinchEnd(e, e2);
                        e1 = e2 = null;
                    }, e1.idx);
                    this.on('fingerMovingEnd', (e) => {
                        if (e1)
                            handler.pinchEnd(e1, e);
                        e1 = e2 = null;
                    }, e2.idx);

                    break;
                }
            };
            this.on('fingerDown', handler);
        }
        ///////////////////////////////////////////////////////////
        /// Implementation stuff

        // register broadcast handlers
        broadcastOn(eventType, obj) {
            const handlers = this.eventObservers.get(eventType);
            if (handlers)
                handlers.push(obj);
            else
                this.eventObservers.set(eventType, [obj]);
        }

        // unregister broadcast handlers
        broadcastOff(eventTypes, obj) {
            PointerManager.splitStr(eventTypes).forEach(eventType => {
                if (this.eventObservers.has(eventType)) {
                    if (!obj) {
                        this.eventObservers.delete(eventType);
                    } else {
                        const handlers = this.eventObservers.get(eventType);
                        const index = handlers.indexOf(obj);
                        if (index > -1) {
                            handlers.splice(index, 1);
                        }
                        if (handlers.length == 0) {
                            this.eventObservers.delete(eventType);
                        }
                    }
                }
            });
        }

        // emit broadcast events
        broadcast(e) {
            if (!this.eventObservers.has(e.fingerType)) return;
            this.eventObservers.get(e.fingerType)
                .sort((a, b) => b.priority - a.priority)
                .every(obj => {
                    obj[e.fingerType](e);
                    return !e.defaultPrevented;
                });  // the first obj returning a defaultPrevented event breaks the every loop
        }

        addCurrPointer(cp) {
            let result = -1;
            for (let i = 0; i < this.currentPointers.length && result < 0; i++) {
                if (this.currentPointers[i] == null) {
                    result = i;
                }
            }
            if (result < 0) {
                this.currentPointers.push(cp);
                result = this.currentPointers.length - 1;
            } else {
                this.currentPointers[result] = cp;
            }

            return result;
        }

        removeCurrPointer(index) {
            this.currentPointers[index] = null;
            while ((this.currentPointers.length > 0) && (this.currentPointers[this.currentPointers.length - 1] == null)) {
                this.currentPointers.pop();
            }
        }

        handleEvent(e) {
            if (e.type == 'pointerdown') this.target.setPointerCapture(e.pointerId);
            if (e.type == 'pointercancel') console.log(e);

            let handled = false;
            for (let i = 0; i < this.currentPointers.length && !handled; i++) {
                const cp = this.currentPointers[i];
                if (cp) {
                    handled = cp.handleEvent(e);
                    if (cp.isDone())
                        this.removeCurrPointer(i);
                }
            }
            if (!handled) {
                const cp = new SinglePointerHandler(this, e.pointerId, { ppmm: this.ppmm });
                handled = cp.handleEvent(e);
            }
            //e.preventDefault();
        }

    }


    class SinglePointerHandler {
        constructor(parent, pointerId, options) {

            this.parent = parent;
            this.pointerId = pointerId;

            Object.assign(this, {
                ppmm: 3, // 27in screen 1920x1080 = 3 ppmm
            });
            if (options)
                Object.assign(this, options);

            this.eventHistory = new CircularBuffer(10);
            this.isActive = false;
            this.startTap = 0;
            this.threshold = 15; // 15mm

            this.eventObservers = new Map();
            this.isDown = false;
            this.done = false;

            this.stateEnum = {
                IDLE: 0,
                DETECT: 1,
                HOVER: 2,
                MOVING_START: 3,
                MOVING: 4,
                MOVING_END: 5,
                HOLD: 6,
                TAPS_DETECT: 7,
                SINGLE_TAP: 8,
                DOUBLE_TAP_DETECT: 9,
                DOUBLE_TAP: 10,
            };
            this.status = this.stateEnum.IDLE;
            this.timeout = null;
            this.holdTimeoutThreshold = 600;
            this.tapTimeoutThreshold = 100;
            this.oldDownPos = { clientX: 0, clientY: 0 };
            this.movingThreshold = 1; // 1mm
            this.idx = this.parent.addCurrPointer(this);
        }

        ///////////////////////////////////////////////////////////
        /// Utilities

        static distance(x0, y0, x1, y1) {
            return Math.sqrt((x1 - x0) ** 2 + (y1 - y0) ** 2);
        }

        distanceMM(x0, y0, x1, y1) {
            return SinglePointerHandler.distance(x0, y0, x1, y1) / this.ppmm;
        }

        ///////////////////////////////////////////////////////////
        /// Class interface

        on(eventType, obj) {
            this.eventObservers.set(eventType, obj);
        }

        off(eventType) {
            if (this.eventObservers.has(eventType)) {
                this.eventObservers.delete(eventType);
            }
        }

        ///////////////////////////////////////////////////////////
        /// Implementation stuff

        addToHistory(e) {
            this.eventHistory.push(e);
        }

        prevPointerEvent() {
            return this.eventHistory.last();
        }

        handlePointerDown(e) {
            this.startTap = e.timeStamp;
        }

        handlePointerUp(e) {
            e.timeStamp - this.startTap;
        }

        isLikelySamePointer(e) {
            let result = this.pointerId == e.pointerId;
            if (!result && !this.isDown && e.type == "pointerdown") {
                const prevP = this.prevPointerEvent();
                if (prevP) {
                    result = (e.pointerType == prevP.pointerType) && this.distanceMM(e.clientX, e.clientY, prevP.clientX, prevP.clientY) < this.threshold;
                }
            }
            return result;
        }

        // emit+broadcast
        emit(e) {
            if (this.eventObservers.has(e.fingerType)) {
                this.eventObservers.get(e.fingerType)[e.fingerType](e);
                if (e.defaultPrevented) return;
            }
            this.parent.broadcast(e);
        }

        // output Event, speed is computed only on pointermove
        createOutputEvent(e, type) {
            const result = e;
            result.fingerType = type;
            result.originSrc = this.originSrc;
            result.speedX = 0;
            result.speedY = 0;
            result.idx = this.idx;
            const prevP = this.prevPointerEvent();
            if (prevP && (e.type == 'pointermove')) {
                const dt = result.timeStamp - prevP.timeStamp;
                if (dt > 0) {
                    result.speedX = (result.clientX - prevP.clientX) / dt * 1000.0;  // px/s
                    result.speedY = (result.clientY - prevP.clientY) / dt * 1000.0;  // px/s
                }
            }
            return result;
        }

        // Finite State Machine
        processEvent(e) {
            let distance = 0;
            if (e.type == "pointerdown") {
                this.oldDownPos.clientX = e.clientX;
                this.oldDownPos.clientY = e.clientY;
                this.isDown = true;
            }
            if (e.type == "pointerup" || e.type == "pointercancel") this.isDown = false;
            if (e.type == "pointermove" && this.isDown) {
                distance = this.distanceMM(e.clientX, e.clientY, this.oldDownPos.clientX, this.oldDownPos.clientY);
            }

            if (e.type == "wheel") {
                this.emit(this.createOutputEvent(e, 'mouseWheel'));
                return;
            }

            switch (this.status) {
                case this.stateEnum.HOVER:
                case this.stateEnum.IDLE:
                    if (e.type == 'pointermove') {
                        this.emit(this.createOutputEvent(e, 'fingerHover'));
                        this.status = this.stateEnum.HOVER;
                        this.originSrc = e.composedPath()[0];
                    } else if (e.type == 'pointerdown') {
                        this.status = this.stateEnum.DETECT;
                        this.emit(this.createOutputEvent(e, 'fingerDown'));
                        if (e.defaultPrevented) { // An observer captured the fingerDown event
                            this.status = this.stateEnum.MOVING;
                            break;
                        }
                        this.originSrc = e.composedPath()[0];
                        this.timeout = setTimeout(() => {
                            this.emit(this.createOutputEvent(e, 'fingerHold'));
                            if(e.defaultPrevented) this.status = this.stateEnum.IDLE;
                        }, this.holdTimeoutThreshold);
                    }
                    break;
                case this.stateEnum.DETECT:
                    if (e.type == 'pointercancel') { /// For Firefox
                        clearTimeout(this.timeout);
                        this.status = this.stateEnum.IDLE;
                        this.emit(this.createOutputEvent(e, 'fingerHold'));
                    } else if (e.type == 'pointermove' && distance > this.movingThreshold) {
                        clearTimeout(this.timeout);
                        this.status = this.stateEnum.MOVING;
                        this.emit(this.createOutputEvent(e, 'fingerMovingStart'));
                    } else if (e.type == 'pointerup') {
                        clearTimeout(this.timeout);
                        this.status = this.stateEnum.TAPS_DETECT;
                        this.timeout = setTimeout(() => {
                            this.status = this.stateEnum.IDLE;
                            this.emit(this.createOutputEvent(e, 'fingerSingleTap'));
                        }, this.tapTimeoutThreshold);
                    }
                    break;
                case this.stateEnum.TAPS_DETECT:
                    if (e.type == 'pointerdown') {
                        clearTimeout(this.timeout);
                        this.status = this.stateEnum.DOUBLE_TAP_DETECT;
                        this.timeout = setTimeout(() => {
                            this.emit(this.createOutputEvent(e, 'fingerHold'));
                            if(e.defaultPrevented) this.status = this.stateEnum.IDLE;
                        }, this.tapTimeoutThreshold);
                    } else if (e.type == 'pointermove' && distance > this.movingThreshold) {
                        clearTimeout(this.timeout);
                        this.status = this.stateEnum.IDLE;
                        this.emit(this.createOutputEvent(e, 'fingerHover'));
                    }
                    break;
                case this.stateEnum.DOUBLE_TAP_DETECT:
                    if (e.type == 'pointerup' || e.type == 'pointercancel') {
                        clearTimeout(this.timeout);
                        this.status = this.stateEnum.IDLE;
                        this.emit(this.createOutputEvent(e, 'fingerDoubleTap'));
                    }
                    break;
                case this.stateEnum.DOUBLE_TAP_DETECT:
                    if (e.type == 'pointermove' && distance > this.movingThreshold) {
                        this.status = this.stateEnum.MOVING;
                        this.emit(this.createOutputEvent(e, 'fingerMovingStart'));
                    }
                    break;
                case this.stateEnum.MOVING:
                    if (e.type == 'pointermove') {
                        // Remain MOVING
                        this.emit(this.createOutputEvent(e, 'fingerMoving'));
                    } else if (e.type == 'pointerup' || e.type == 'pointercancel') {
                        this.status = this.stateEnum.IDLE;
                        this.emit(this.createOutputEvent(e, 'fingerMovingEnd'));
                    }
                    break;
                default:
                    console.log("ERROR " + this.status);
                    console.log(e);
                    break;
            }

            this.addToHistory(e);
        }

        handleEvent(e) {
            let result = false;
            if (this.isLikelySamePointer(e)) {
                this.pointerId = e.pointerId; //it's mine
                this.processEvent(e);
                result = true;
            }
            return result;
        }

        isDone() {
            return this.status == this.stateEnum.IDLE;
        }

    }


    class CircularBuffer {
        constructor(capacity) {
            if (typeof capacity != "number" || !Number.isInteger(capacity) || capacity < 1)
                throw new TypeError("Invalid capacity");
            this.buffer = new Array(capacity);
            this.capacity = capacity;
            this.first = 0;
            this.size = 0;
        }

        clear() {
            this.first = 0;
            this.size = 0;
        }

        empty() {
            return this.size == 0;
        }

        size() {
            return this.size;
        }

        capacity() {
            return this.capacity;
        }

        first() {
            let result = null;
            if (this.size > 0) result = this.buffer[this.first];
            return result;
        }

        last() {
            let result = null;
            if (this.size > 0) result = this.buffer[(this.first + this.size - 1) % this.capacity];
            return result;
        }

        enqueue(v) {
            this.first = (this.first > 0) ? this.first - 1 : this.first = this.capacity - 1;
            this.buffer[this.first] = v;
            if (this.size < this.capacity) this.size++;
        }

        push(v) {
            if (this.size == this.capacity) {
                this.buffer[this.first] = v;
                this.first = (this.first + 1) % this.capacity;
            } else {
                this.buffer[(this.first + this.size) % this.capacity] = v;
                this.size++;
            }
        }

        dequeue() {
            if (this.size == 0) throw new RangeError("Dequeue on empty buffer");
            const v = this.buffer[(this.first + this.size - 1) % this.capacity];
            this.size--;
            return v;
        }

        pop() {
            return this.dequeue();
        }

        shift() {
            if (this.size == 0) throw new RangeError("Shift on empty buffer");
            const v = this.buffer[this.first];
            if (this.first == this.capacity - 1) this.first = 0; else this.first++;
            this.size--;
            return v;
        }

        get(start, end) {
            if (this.size == 0 && start == 0 && (end == undefined || end == 0)) return [];
            if (typeof start != "number" || !Number.isInteger(start) || start < 0) throw new TypeError("Invalid start value");
            if (start >= this.size) throw new RangeError("Start index past end of buffer: " + start);

            if (end == undefined) return this.buffer[(this.first + start) % this.capacity];

            if (typeof end != "number" || !Number.isInteger(end) || end < 0) throw new TypeError("Invalid end value");
            if (end >= this.size) throw new RangeError("End index past end of buffer: " + end);

            if (this.first + start >= this.capacity) {
                start -= this.capacity;
                end -= this.capacity;
            }
            if (this.first + end < this.capacity)
                return this.buffer.slice(this.first + start, this.first + end + 1);
            else
                return this.buffer.slice(this.first + start, this.capacity).concat(this.buffer.slice(0, this.first + end + 1 - this.capacity));
        }

        toArray() {
            if (this.size == 0) return [];
            return this.get(0, this.size - 1);
        }

    }

    /**
     * Manages an OpenLIME viewer functionality on a canvas
     * how do I write more substantial documentation.
     *
     * @param {div} div of the DOM or selector (es. '#canvasid'), or a canvas.
     * @param {string} options is a url to a JSON describing the viewer content
     * @param {object} options is a JSON describing the viewer content
     *  * **animate**: default *true*, calls requestAnimation() and manages refresh.
     *  * **background**: css style for background (overwrites css if present)
     * 
     * @example
     * const lime = new OpenLIME.OpenLIME('.openlime');
     * // .openlime is the class of a DIV element in the DOM.
     */

    class Viewer {

        constructor(div, options) {

            Object.assign(this, {
                background: null,
                canvas: {},
                controllers: [],
                camera: new Camera()
            });


            if (typeof (div) == 'string')
                div = document.querySelector(div);

            if (!div)
                throw "Missing element parameter";

            Object.assign(this, options);
            if (this.background)
                div.style.background = this.background;

            this.containerElement = div;
            this.canvasElement = div.querySelector('canvas');
            if (!this.canvasElement) {
                this.canvasElement = document.createElement('canvas');
                div.prepend(this.canvasElement);
            }

            this.overlayElement = document.createElement('div');
            this.overlayElement.classList.add('openlime-overlay');
            this.containerElement.appendChild(this.overlayElement);


            this.canvas = new Canvas(this.canvasElement, this.overlayElement, this.camera, this.canvas);
            this.canvas.addEvent('update', () => { this.redraw(); });

            this.pointerManager = new PointerManager(this.overlayElement);

            this.canvasElement.addEventListener('contextmenu', (e) => {
                e.preventDefault();
                return false;
            });

            var resizeobserver = new ResizeObserver(entries => {
                for (let entry of entries) {
                    this.resize(entry.contentRect.width, entry.contentRect.height);
                }
            });
            resizeobserver.observe(this.canvasElement);

            this.resize(this.canvasElement.clientWidth, this.canvasElement.clientHeight);
        }


        /* Convenience function, it actually passes to Canvas
        */
        addLayer(id, layer) {
            canvas.addLayer(id, layer);
        }

        /**
        * Resize the canvas (and the overlay) and triggers a redraw.
        */

        resize(width, height) {
            // Test with retina display!
            this.canvasElement.width = width * window.devicePixelRatio;
            this.canvasElement.height = height * window.devicePixelRatio;

            this.camera.setViewport({ x: 0, y: 0, dx: width, dy: height, w: width, h: height });
            this.canvas.prefetch();
            this.redraw();
        }

        /**
        *
        * Schedule a drawing.
        */
        redraw() {
            if (this.animaterequest) return;
            this.animaterequest = requestAnimationFrame((time) => { this.draw(time); });
        }

        /**
        * Do not call this if OpenLIME is animating, use redraw()
        * @param {time} time as in performance.now()
        */
        draw(time) {
            if (!time) time = performance.now();
            this.animaterequest = null;

            this.camera.viewport;
            this.camera.getCurrentTransform(time);

            let done = this.canvas.draw(time);
            if (!done)
                this.redraw();
        }

    }

    let url = 'skin/skin.svg';
    let svg = null;
    let pad = 5;

    class Skin {

    	static setUrl(u) { url = u; }
    	static async loadSvg() {
    		var response = await fetch(url);
    		if (!response.ok) {
    			throw Error("Failed loading " + url + ": " + response.statusText);
    		}

    		let text = await response.text();
    		let parser = new DOMParser();
    		svg = parser.parseFromString(text, "image/svg+xml").documentElement;
    	}
    	static async getElement(selector) { 
    		if(!svg)
    		await Skin.loadSvg();
    		return svg.querySelector(selector).cloneNode(true);
    	}

    	//can't return just the svg, because it needs the container BEFORE computing the box.
    	static async appendIcon(container, selector) {
    		let element = await Skin.getElement(selector);

    		let icon = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
    		container.appendChild(icon);
    		icon.appendChild(element);
    		
    		let box = element.getBBox();

    		let tlist = element.transform.baseVal;
    		if (tlist.numberOfItems == 0)
    			tlist.appendItem(icon.createSVGTransform());
    		tlist.getItem(0).setTranslate(-box.x, -box.y);

    		icon.setAttribute('viewBox', `${-pad} ${-pad} ${box.width + 2*pad} ${box.height + 2*pad}`);
    		icon.setAttribute('preserveAspectRatio', 'xMidYMid meet');
    		return icon;
    	}

    	// static appendIcon(container, selector) {		
    	// 	let icon = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
    	// 	container.appendChild(icon);

    	// 	(async () => {
    	// 		let element = await Skin.getElement(selector);
    	// 		icon.appendChild(element);
    		
    	// 		let box = element.getBBox();

    	// 		let tlist = element.transform.baseVal;
    	// 		if (tlist.numberOfItems == 0)
    	// 			tlist.appendItem(icon.createSVGTransform());
    	// 		tlist.getItem(0).setTranslate(-box.x, -box.y);

    	// 		icon.setAttribute('viewBox', `${-pad} ${-pad} ${box.width + 2*pad} ${box.height + 2*pad}`);
    	// 		icon.setAttribute('preserveAspectRatio', 'xMidYMid meet');
    	// 	})();
    	// 	return icon;
    	// }
    	
    }

    /* Basic viewer for a single layer.
     *  we support actions through buttons: each button style is controlled by classes (trigger), active (if support status)
     *  and custom.
     * actions supported are:
     *  home: reset the camera
     *  zoomin, zoomout
     *  fullscreen
     *  rotate (45/90 deg rotation option.
     *  light: turn on light changing.
     *  switch layer(s)
     *  lens.
     * 
     * How the menu works:
     * Each entry eg: { title: 'Coin 16' }
     * title: large title
     * section: smaller title
     * html: whatever html
     * button: visually a button, attributes: group, layer, mode
     * slider: callback(percent)
     * list: an array of entries.
     * 
     * Additional attributes:
     * onclick: a function(event) {}
     * group: a group of entries where at most one is active
     * layer: a layer id: will be active if layer is visible
     * mode: a layer visualization mode, active if it's the current mode.
     * layer + mode: if both are specified, both must be current for an active.
     */

    class UIBasic {
    	constructor(lime, options) {
    		//we need to know the size of the scene but the layers are not ready.
    		let camera = lime.camera;
    		Object.assign(this, {
    			lime: lime,
    			camera: lime.camera,
    			skin: 'skin/skin.svg',
    			autoFit: true,
    			//skinCSS: 'skin.css', // TODO: probably not useful
    			actions: {
    				home: { title: 'Home', display: true, key: 'Home', task: (event) => { if (camera.boundingBox) camera.fitCameraBox(250); } },
    				fullscreen: { title: 'Fullscreen', display: true, key: 'f', task: (event) => { this.toggleFullscreen(); } },
    				layers: { title: 'Layers', display: true, key: 'Escape', task: (event) => { this.toggleLayers(event); } },
    				zoomin: { title: 'Zoom in', display: false, key: '+', task: (event) => { camera.deltaZoom(250, 1.25, 0, 0); } },
    				zoomout: { title: 'Zoom out', display: false, key: '-', task: (event) => { camera.deltaZoom(250, 1 / 1.25, 0, 0); } },
    				rotate: { title: 'Rotate', display: false, key: 'r', task: (event) => { camera.rotate(250, -45); } },
    				light: { title: 'Light', display: 'auto', key: 'l', task: (event) => { this.toggleLightController(); } },
    				ruler: { title: 'Ruler', display: false, task: (event) => { this.startRuler(); } },
    				help: { title: 'Help', display: false, key: '?', task: (event) => { this.toggleHelp(this.actions.help); }, html: '<p>Help here!</p>' },
    				snapshot: { title: 'Snapshot', display: false, task: (event) => { this.snapshot(); } },
    			},
    			viewport: [0, 0, 0, 0], //in scene coordinates
    			scale: null,
    			unit: null,
    			attribution: null,     //image attribution
    			lightcontroller: null,
    		});

    		Object.assign(this, options);
    		if (this.autoFit)
    			this.lime.canvas.addEvent('updateSize', () => this.lime.camera.fitCameraBox(0));

    		this.panzoom = new ControllerPanZoom(this.lime.camera, { priority: -1000 });

    		this.menu = [];

    		/*let element = entry.element;
    		let group = element.getAttribute('data-group');
    		let layer = element.getAttribute('data-layer');
    		let mode = element.getAttribute('data-mode');
    		let active = (layer && this.lime.canvas.layers[layer].visible) &&
    			(!mode || this.lime.canvas.layers[layer].getMode() == mode);
    		entry.element.classList.toggle('active', active); */

    		this.menu.push({ section: "Layers" });
    		for (let [id, layer] of Object.entries(this.lime.canvas.layers)) {
    			let modes = [];
    			for (let m of layer.getModes()) {
    				let mode = {
    					button: m,
    					mode: m,
    					layer: id,
    					onclick: () => { layer.setMode(m); this.updateMenu(); },
    					status: () => layer.getMode() == m ? 'active' : '',
    				};
    				if (m == 'specular' && layer.shader.setSpecularExp)
    					mode.list = [{ slider: '', oninput: (e) => { layer.shader.setSpecularExp(e.target.value); } }];
    				modes.push(mode);
    			}
    			let layerEntry = {
    				button: layer.label || id,
    				onclick: () => { this.setLayer(layer); },
    				status: () => layer.visible ? 'active' : '',
    				list: modes,
    				layer: id
    			};
    			if (layer.annotations) {
    				layerEntry.list.push(layer.annotationsEntry());
    				//TODO: this could be a convenience, creating an editor which can be
    				//customized later using layer.editor.
    				//if(layer.editable) 
    				//	layer.editor = this.editor;
    			}
    			this.menu.push(layerEntry);
    		}

    		let controller = new Controller2D((x, y) => {
    			for (let layer of lightLayers)
    				layer.setLight([x, y], 0);
    			if(this.showLightDirections)
    				this.updateLightDirections(x, y);

    			}, { 
    				active: false, 
        			activeModifiers: [2, 4], 
        			control: 'light', 
        			onPanStart: this.showLightDirections ? () => {
        				this.info.fade(true);
        				Object.values(this.lime.canvas.layers).filter(l => l.annotations != null).forEach(l => l.setVisible(false) );
        				this.enableLightDirections(true); } : null,
        			onPanEnd: this.showLightDirections ? () => { 
        				this.info.fade(false);
        				Object.values(this.lime.canvas.layers).filter(l => l.annotations != null).forEach(l => l.setVisible(true) );
        				this.enableLightDirections(false); } : null,
        			relative: true 
    			});

    		controller.priority = 0;
    		this.lime.pointerManager.onEvent(controller);
    		this.lightcontroller = controller;


    		let lightLayers = [];
    		for (let [id, layer] of Object.entries(this.lime.canvas.layers))
    			if (layer.controls.light) lightLayers.push(layer);

    		if (lightLayers.length) {
    			this.createLightDirections();
    			for (let layer of lightLayers) {
    				controller.setPosition(0.5, 0.5);
    				//layer.setLight([0.5, 0.5], 0);
    				layer.controllers.push(controller);
    			}
    		}

    		if (queueMicrotask) queueMicrotask(() => { this.init(); }); //allows modification of actions and layers before init.
    		else setTimeout(() => { this.init(); }, 0);
    	}

    	createLightDirections() {
    		this.lightDirections = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
    		this.lightDirections.setAttribute('viewBox', '-100, -100, 200 200');
    		this.lightDirections.setAttribute('preserveAspectRatio', 'xMidYMid meet');
    		this.lightDirections.style.display = 'none';
    		this.lightDirections.classList.add('openlime-lightdir');
    		for(let x = -1; x <= 1; x++) {
    			for(let y = -1; y <= 1; y++) {
    				let line = document.createElementNS('http://www.w3.org/2000/svg', 'line');
    				line.pos = [x*35, y*35];
    				//line.setAttribute('data-start', `${x} ${y}`);
    				this.lightDirections.appendChild(line);
    			}
    		}
    		this.lime.containerElement.appendChild(this.lightDirections);
    	}
    	
    	updateLightDirections(lx, ly) {
    		let lines = [...this.lightDirections.children];
    		for(let line of lines) {
    			let x = line.pos[0];
    			let y = line.pos[1];
    			
    			line.setAttribute('x1', 0.6*x -25*0*lx);
    			line.setAttribute('y1', 0.6*y +25*0*ly);
    			line.setAttribute('x2', x/0.6 + 60*lx);
    			line.setAttribute('y2', y/0.6 - 60*ly);
    		}
    	}
    	enableLightDirections(show) {
    		this.lightDirections.style.display = show? 'block' : 'none';
    	}
    	

    	init() {
    		(async () => {

    			document.addEventListener('keydown', (e) => this.keyDown(e), false);
    			document.addEventListener('keyup', (e) => this.keyUp(e), false);

    			let panzoom = this.panzoom = new ControllerPanZoom(this.lime.camera, {
    				priority: -1000,
    				activeModifiers: [0, 1]
    			});
    			this.lime.pointerManager.onEvent(panzoom); //register wheel, doubleclick, pan and pinch
    			// this.lime.pointerManager.on("fingerSingleTap", { "fingerSingleTap": (e) => { this.showInfo(e); }, priority: 10000 });

    			this.createMenu();
    			this.updateMenu();

    			if (this.actions.light && this.actions.light.display === 'auto')
    				this.actions.light.display = true;


    			if (this.skin)
    				await this.loadSkin();
    			/* TODO: this is probably not needed
    			if(this.skinCSS)
    				await this.loadSkinCSS();
    			*/

    			this.setupActions();
    			this.setupScale();
    			if(this.attribution) {
    				var p = document.createElement('p');
    				p.classList.add('openlime-attribution');
    				p.innerHTML = this.attribution;
    				this.lime.containerElement.appendChild(p);
    			}

    			

    			for (let l of Object.values(this.lime.canvas.layers)) {
    				this.setLayer(l);
    				break;
    			}

    			if (this.actions.light.active == true)
    				this.toggleLightController();

    		})().catch(e => { console.log(e); throw Error("Something failed") });
    	}

    	keyDown(e) {
    	}

    	keyUp(e) {
    		if (e.target != document.body && e.target.closest('input, textarea') != null)
    			return;

    		if (e.defaultPrevented) return;

    		for (const a of Object.values(this.actions)) {
    			if ('key' in a && a.key == e.key) {
    				e.preventDefault();
    				a.task(e);
    				return;
    			}
    		}
    	}
    	
    	async loadSkin() {
    		let toolbar = document.createElement('div');
    		toolbar.classList.add('openlime-toolbar');
    		this.lime.containerElement.appendChild(toolbar);


    		//toolbar manually created with parameters (padding, etc) + css for toolbar positioning and size.
    		{
    			for (let [name, action] of Object.entries(this.actions)) {

    				if (action.display !== true)
    					continue;

    				await Skin.appendIcon(toolbar, '.openlime-' + name);
    			}

    		}
    	}

    	setupActions() {
    		for (let [name, action] of Object.entries(this.actions)) {
    			let element = this.lime.containerElement.querySelector('.openlime-' + name);
    			if (!element)
    				continue;
    			// let pointerManager = new PointerManager(element);
    			// pointerManager.onEvent({ fingerSingleTap: action.task, priority: -2000 });
    			element.addEventListener('click', (e) => {
    				action.task(e);
    				e.preventDefault();
    			});
    		}
    		let items = document.querySelectorAll('.openlime-layers-button');
    		for (let item of items) {
    			let id = item.getAttribute('data-layer');
    			if (!id) continue;
    			item.addEventListener('click', () => {
    				this.setLayer(this.lime.layers[id]);
    			});
    		}
    	}
    	//find best length for scale from min -> max
    	//zoom 2 means a pixel in image is now 2 pixel on screen, scale is
    	bestScaleLength(min, max, scale, zoom) {
    		scale /= zoom;
    		//closest power of 10:
    		let label10 = Math.pow(10, Math.floor(Math.log(max * scale) / Math.log(10)));
    		let length10 = label10 / scale;
    		if (length10 > min) return { length: length10, label: label10 };

    		let label20 = label10 * 2;
    		let length20 = length10 * 2;
    		if (length20 > min) return { length: length20, label: label20 };

    		let label50 = label10 * 5;
    		let length50 = length10 * 5;

    		if (length50 > min) return { length: length50, label: label50 };
    		return { length: 0, label: 0 }
    	}

    	updateScale(line, text) {
    		//let zoom = this.lime.camera.getCurrentTransform(performance.now()).z;
    		let zoom = this.lime.camera.target.z;
    		if (zoom == this.lastScaleZoom)
    			return;
    		this.lastScaleZoom = zoom;
    		let s = this.bestScaleLength(100, 200, this.scale, zoom);
    		//let line = document.querySelector('.openlime-scale > line');
    		let margin = 200 - 10 - s.length;
    		line.setAttribute('x1', margin / 2);
    		line.setAttribute('x2', 200 - margin / 2);
    		//let text = document.querySelector('.openlime-scale > text');
    		text.textContent = s.label + "mm";


    	}

    	//scale is length of a pixel in mm
    	setupScale() {
    		if (!this.scale) return;
    		this.scales = { 'mm': 1, 'cm': 10, 'm': 1000, 'km': 1000000 };


    		let svg = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
    		svg.setAttribute('viewBox', `0 0 200 40`);
    		svg.classList.add('openlime-scale');
    		let line = document.createElementNS('http://www.w3.org/2000/svg', 'line');
    		line.setAttribute('x1', 5);
    		line.setAttribute('y1', 26.5);
    		line.setAttribute('x2', 195);
    		line.setAttribute('y2', 26.5);
    		let text = document.createElementNS('http://www.w3.org/2000/svg', 'text');
    		text.setAttribute('x', '50%');
    		text.setAttribute('y', '16px');
    		text.setAttribute('dominant-baseline', 'middle');
    		text.setAttribute('text-anchor', 'middle');
    		text.textContent = "10mm";
    		//var label = document.createTextNode("10mm");
    		//text.appendChild(label);


    		svg.appendChild(line);
    		svg.appendChild(text);
    		this.lime.containerElement.appendChild(svg);
    		this.lime.camera.addEvent('update', () => { this.updateScale(line, text); });
    	}

    	//we need the concept of active layer! so we an turn on and off light.
    	toggleLightController() {
    		let div = this.lime.containerElement;
    		let active = div.classList.toggle('openlime-light-active');
    		this.lightActive = active;

    		for (let layer of Object.values(this.lime.canvas.layers))
    			for (let c of layer.controllers)
    				if (c.control == 'light') {
    					c.active = true;
    					c.activeModifiers = active ? [0, 2, 4] : [2, 4];  //nothing, shift and alt
    				}
    	}

    	toggleFullscreen() {
    		let canvas = this.lime.canvasElement;
    		let div = this.lime.containerElement;
    		let active = div.classList.toggle('openlime-fullscreen-active');

    		if (!active) {
    			var request = document.exitFullscreen || document.webkitExitFullscreen ||
    				document.mozCancelFullScreen || document.msExitFullscreen;
    			request.call(document); document.querySelector('.openlime-scale > line');

    			this.lime.resize(canvas.offsetWidth, canvas.offsetHeight);
    		} else {
    			var request = div.requestFullscreen || div.webkitRequestFullscreen ||
    				div.mozRequestFullScreen || div.msRequestFullscreen;
    			request.call(div);
    		}
    		this.lime.resize(canvas.offsetWidth, canvas.offsetHeight);
    	}

    	startRuler() {
    	}

    	endRuler() {
    	}

    	toggleHelp(help, on) {
    		if(!help.dialog) {
    			help.dialog = new UIDialog(this.lime.containerElement, { modal: true, class: 'openlime-help-dialog' });
    			help.dialog.setContent(help.html);
    		} else
    			help.dialog.toggle(on);		
    	}

    	snapshot() {
    		var e = document.createElement('a');
    		e.setAttribute('href', this.lime.canvas.canvasElement.toDataURL());
    		e.setAttribute('download', 'snapshot.png');
    		e.style.display = 'none';
    		document.body.appendChild(e);
    		e.click();
    		document.body.removeChild(e);
    	}

    	/* Layer management */

    	createEntry(entry) {
    		if (!('id' in entry))
    			entry.id = 'entry_' + (this.entry_count++);

    		let id = `id="${entry.id}"`;
    		let tooltip = 'tooltip' in entry ? `title="${entry.tooltip}"` : '';
    		let classes = 'classes' in entry ? entry.classes : '';
    		let html = '';
    		if ('title' in entry) {
    			html += `<h2 ${id} class="openlime-title ${classes}" ${tooltip}>${entry.title}</h2>`;

    		} else if ('section' in entry) {
    			html += `<h3 ${id} class="openlime-section ${classes}" ${tooltip}>${entry.section}</h3>`;

    		} else if ('html' in entry) {
    			html += `<div ${id} class="${classes}">${entry.html}</div>`;

    		} else if ('button' in entry) {
    			let group = 'group' in entry ? `data-group="${entry.group}"` : '';
    			let layer = 'layer' in entry ? `data-layer="${entry.layer}"` : '';
    			let mode = 'mode' in entry ? `data-mode="${entry.mode}"` : '';
    			html += `<a href="#" ${id} ${group} ${layer} ${mode} ${tooltip} class="openlime-entry ${classes}">${entry.button}</a>`;
    		} else if ('slider' in entry) {
    			html += `<input type="range" min="1" max="100" value="50" class="openlime-slider ${classes}" ${id}>`;
    		}

    		if ('list' in entry) {
    			let ul = `<div class="openlime-list ${classes}">`;
    			for (let li of entry.list)
    				ul += this.createEntry(li);
    			ul += '</div>';
    			html += ul;
    		}
    		return html;
    	}

    	addEntryCallbacks(entry) {
    		entry.element = this.layerMenu.querySelector('#' + entry.id);
    		if (entry.onclick)
    			entry.element.addEventListener('click', (e) => {
    				entry.onclick();
    				//this.updateMenu();
    			});
    		if (entry.oninput)
    			entry.element.addEventListener('input', entry.oninput);
    		if (entry.oncreate)
    			entry.oncreate();

    		if ('list' in entry)
    			for (let e of entry.list)
    				this.addEntryCallbacks(e);
    	}

    	updateEntry(entry) {
    		let status = entry.status ? entry.status() : '';
    		entry.element.classList.toggle('active', status == 'active');

    		if ('list' in entry)
    			for (let e of entry.list)
    				this.updateEntry(e);
    	}

    	updateMenu() {
    		for (let entry of this.menu)
    			this.updateEntry(entry);
    	}

    	createMenu() {
    		this.entry_count = 0;
    		let html = `<div class="openlime-layers-menu">`;
    		for (let entry of this.menu) {
    			html += this.createEntry(entry);
    		}
    		html += '</div>';


    		let template = document.createElement('template');
    		template.innerHTML = html.trim();
    		this.layerMenu = template.content.firstChild;
    		this.lime.containerElement.appendChild(this.layerMenu);

    		for (let entry of this.menu) {
    			this.addEntryCallbacks(entry);
    		}


    		/*		for(let li of document.querySelectorAll('[data-layer]'))
    					li.addEventListener('click', (e) => {
    						this.setLayer(this.lime.canvas.layers[li.getAttribute('data-layer')]);
    					}); */
    	}

    	toggleLayers(event) {
    		this.layerMenu.classList.toggle('open');
    	}

    	setLayer(layer_on) {
    		if (typeof layer_on == 'string')
    			layer_on = this.lime.canvas.layers[layer_on];

    		if (layer_on.overlay) { //just toggle
    			layer_on.setVisible(!layer_on.visible);

    		} else {
    			for (let layer of Object.values(this.lime.canvas.layers)) {
    				if (layer.overlay)
    					continue;

    				layer.setVisible(layer == layer_on);
    				for (let c of layer.controllers) {
    					if (c.control == 'light')
    						c.active = this.lightActive && layer == layer_on;
    				}
    			}
    		}
    		this.updateMenu();
    		this.lime.redraw();
    	}

    	closeLayersMenu() {
    		this.layerMenu.style.display = 'none';
    	}
    }

    class UIDialog {
    	constructor(container, options) {
    		Object.assign(this, {
    			dialog: null,
    			content: null,
    			container: container,
    			modal: false,
    			signals: { 'closed': [] },
    			class: null,
    		}, options);
    		this.create();
    	}

    	//TODO make QObject style events dependency
    	addEvent(event, callback) {
    		this.signals[event].push(callback);
    	}
    	
    	emit(event, ...parameters) {
    		for (let r of this.signals[event])
    			r(...parameters);
    	}

    	create() {
    		let background = document.createElement('div');
    		background.classList.add('openlime-dialog-background');

    		let dialog = document.createElement('div');
    		dialog.classList.add('openlime-dialog');
    		if (this.class)
    			dialog.classList.add(this.class);

    		(async () => {
    			let close = await Skin.appendIcon(dialog, '.openlime-close');
    			close.classList.add('openlime-close');
    			close.addEventListener('click', () => this.hide());
    			//content.appendChild(close);
    		})();


    		// let close = Skin.appendIcon(dialog, '.openlime-close');
    		// close.classList.add('openlime-close');
    		// close.addEventListener('click', () => this.hide());

    		let content = document.createElement('div');
    		content.classList.add('openlime-dialog-content');
    		dialog.append(content);

    		if (this.modal) {
    			background.addEventListener('click', (e) => { if (e.target == background) this.hide(); });
    			background.appendChild(dialog);
    			this.container.appendChild(background);
    			this.element = background;

    		} else {

    			this.container.appendChild(dialog);
    			this.element = dialog;
    		}

    		this.dialog = dialog;
    		this.content = content;
    	}
    	
    	setContent(html) {
    		if (typeof (html) == 'string')
    			this.content.innerHTML = html;
    		else
    			this.content.replaceChildren(html);
    	}
    	
    	show() {
    		this.element.classList.remove('hidden');
    	}
    	
    	hide() {
    		this.element.classList.add('hidden');
    		this.emit('closed');
    	}
    	
    	fade(on) {
    		this.element.classList.toggle('fading');
    	}

    	toggle(on) {
    		this.element.classList.toggle('hidden', on);
    	}
    }

    /**
     *  @param {object} options
     * *compose*: compose operation: add, subtract, multiply, etc.
     */

    class ShaderRTI extends Shader {
    	constructor(options) {
    		super({});

    		Object.assign(this, {
    			modes: ['light', 'normals', 'diffuse', 'specular'],
    			mode: 'normal',
    			type:        ['ptm', 'hsh',  'sh', 'rbf', 'bln'],
    			colorspaces: ['lrgb', 'rgb', 'mrgb', 'mycc'],

    			nplanes: null,     //number of coefficient planes
    			yccplanes: null,     //number of luminance planes for mycc color space
    			njpegs: null,      //number of textures needed (ceil(nplanes/3))
    			material: null,    //material parameters
    			lights: null,      //light directions (needed for rbf interpolation)
    			sigma: null,       //rbf interpolation parameter
    			ndimensions: null, //PCA dimension space (for rbf and bln)

    			scale: null,      //factor and bias are used to dequantize coefficient planes.
    			bias: null,

    			basis: null,       //PCA basis for rbf and bln
    			lweights: null    //light direction dependent coefficients to be used with coefficient planes
    		});
    		Object.assign(this, options);

    		if(this.relight)
    			this.init(this.relight);

    		this.setMode('light');
    	}

    	setMode(mode) {
    		if(!(this.modes.includes(mode)))
    			throw Error("Unknown mode: " + mode);
    		this.mode = mode;

    		if( mode != 'light') {
    			this.lightWeights([ 0.612,  0.354, 0.707], 'base');
    			this.lightWeights([-0.612,  0.354, 0.707], 'base1');
    			this.lightWeights([     0, -0.707, 0.707], 'base2');
    		}
    		this.needsUpdate = true;
    	}

    	setLight(light) {
    		if(!this.uniforms.light) 
    			throw "Shader not initialized, wait on layer ready event for setLight."

    		let x = light[0];
    		let y = light[1];

    		//map the square to the circle.
    		let r = Math.sqrt(x*x + y*y);
    		if(r > 1) {
    			x /= r;
    			y /= r;
    		}
    		let z = Math.sqrt(Math.max(0, 1 - x*x - y*y));
    		light = [x, y, z];

    		if(this.mode == 'light')
    			this.lightWeights(light, 'base');
    		this.setUniform('light', light);
    	}
    	setSpecularExp(value) {
    		this.setUniform('specular_exp', value);
    	}

    	init(relight) {
    		Object.assign(this, relight);
    		if(this.colorspace == 'mycc')
    			this.nplanes = this.yccplanes[0] + this.yccplanes[1] + this.yccplanes[2];
    		else 
    			this.yccplanes = [0, 0, 0];


    		this.planes = [];
    		this.njpegs = 0;
    		while(this.njpegs*3 < this.nplanes)
    			this.njpegs++;

    		for(let i = 0; i < this.njpegs; i++)
    			this.samplers.push({ id:i, name:'plane'+i, type:'vec3' });
    		if(this.normals)
    			this.samplers.push({id:this.njpegs, name:'normals', type:'vec3' });

    		if(this.normals)
    			this.samplers.push({ id:this.njpegs, name:'normals', type:'vec3'});

    		this.material = this.materials[0];

    		if(this.lights)
    			this.lights + new Float32Array(this.lights);

    		if(this.type == "rbf")
    			this.ndimensions = this.lights.length/3;


    		if(this.type == "bilinear") {
    			this.ndimensions = this.resolution*this.resolution;
    			this.type = "bln";
    		}

    		this.scale = this.material.scale;
    		this.bias = this.material.bias;


    		if(['mrgb', 'mycc'].includes(this.colorspace))
    			this.loadBasis(this.basis);


    		this.uniforms = {
    			light: { type: 'vec3', needsUpdate: true, size: 3,              value: [0.0, 0.0, 1] },
    			specular_exp: { type: 'float', needsUpdate: false, size: 1, value: 10 },
    			bias:  { type: 'vec3', needsUpdate: true, size: this.nplanes/3, value: this.bias },
    			scale: { type: 'vec3', needsUpdate: true, size: this.nplanes/3, value: this.scale },
    			base:  { type: 'vec3', needsUpdate: true, size: this.nplanes },
    			base1: { type: 'vec3', needsUpdate: false, size: this.nplanes },
    			base2: { type: 'vec3', needsUpdate: false, size: this.nplanes }
    		};

    		this.lightWeights([0, 0, 1], 'base');
    	}

    	lightWeights(light, basename, time) {
    		let value;
    		switch(this.type) {
    			case 'ptm': value = PTM.lightWeights(light); break;
    			case 'hsh': value = HSH.lightWeights(light); break;
    			case 'sh' : value = SH.lightWeights(light); break;
    			case 'rbf': value = RBF.lightWeights(light, this); break;
    			case 'bln': value = BLN.lightWeights(light, this); break;
    		}
    		this.setUniform(basename, value, time);
    	}

    	baseLightOffset(p, l, k) {
    		return (p*this.ndimensions + l)*3 + k;
    	}

    	basePixelOffset(p, x, y, k) {
    		return (p*this.resolution*this.resolution + (x + y*this.resolution))*3 + k;
    	}

    	loadBasis(data) {
    		let tmp = new Uint8Array(data);
    		this.basis = new Float32Array(data.length);

    		new Float32Array(tmp.length);
    		for(let plane = 0; plane < this.nplanes+1; plane++) {
    			for(let c = 0; c < this.ndimensions; c++) {
    				for(let k = 0; k < 3; k++) {
    					let o = this.baseLightOffset(plane, c, k);
    					if(plane == 0)
    						this.basis[o] = tmp[o]/255;
    					else
    						this.basis[o] = ((tmp[o] - 127)/this.material.range[plane-1]);
    				}
    			}
    		}
    	}

    	fragShaderSrc(gl) {
    		
    		let basetype = 'vec3'; //(this.colorspace == 'mrgb' || this.colorspace == 'mycc')?'vec3':'float';
    		let gl2 = !(gl instanceof WebGLRenderingContext);
    		let str = `${gl2? '#version 300 es' : ''}

precision highp float; 
precision highp int; 

#define np1 ${this.nplanes + 1}

${gl2? 'in' : 'varying'} vec2 v_texcoord;
${gl2? 'out' : ''} vec4 color;

const mat3 T = mat3(8.1650e-01, 4.7140e-01, 4.7140e-01,
	-8.1650e-01, 4.7140e-01,  4.7140e-01,
	-1.6222e-08, -9.4281e-01, 4.7140e-01);

uniform vec3 light;
uniform float specular_exp;
uniform vec3 bias[np1];
uniform vec3 scale[np1];

uniform ${basetype} base[np1];
uniform ${basetype} base1[np1];
uniform ${basetype} base2[np1];
`;

    		for(let n = 0; n < this.njpegs; n++) 
    			str += `
uniform sampler2D plane${n};
`;

    		if(this.normals)
    			str += `
uniform sampler2D normals;
`;

    		if(this.colorspace == 'mycc')
    			str +=
`

const int ny0 = ${this.yccplanes[0]};
const int ny1 = ${this.yccplanes[1]};
`;

    		switch(this.colorspace) {
    			case 'rgb':  str +=  RGB.render(this.njpegs, gl2); break;
    			case 'mrgb': str += MRGB.render(this.njpegs, gl2); break;
    			case 'mycc': str += MYCC.render(this.njpegs, this.yccplanes[0], gl2); break;
    		}

    		str += `

void main(void) {

`;
    		if(this.mode == 'light') {
    			str += `
	color = render(base);
`;
    		} else  {
    			if(this.normals)
    				str += `
	vec3 normal = (texture${gl2?'':'2D'}(normals, v_texcoord).zyx *2.0) - 1.0;
	normal.z = sqrt(1.0 - normal.x*normal.x - normal.y*normal.y);
`;
    			else
    				str += `
	vec3 normal;
	normal.x = dot(render(base ).xyz, vec3(1));
	normal.y = dot(render(base1).xyz, vec3(1));
	normal.z = dot(render(base2).xyz, vec3(1));
	normal = normalize(T * normal);
`; 
    			switch(this.mode) {
    			case 'normals':  str += `
	normal = (normal + 1.0)/2.0;
	color = vec4(0.0, normal.xy, 1);
`;
    			break;

    			case 'diffuse': str += `
	color = vec4(vec3(dot(light, normal)), 1);
`;
    			break;

    			case 'specular': 
    			default: str += `
	float s = pow(dot(light, normal), specular_exp);
	//color = vec4(render(base).xyz*s, 1.0);
	color = vec4(s, s, s, 1.0);
`;
    			break;
    			}
    		}

    		str += `
	${gl2?'':'gl_FragColor = color;'}
}`;
    		return str;
    	}
    }



    class RGB {
    	static render(njpegs, gl2) {
    		let str = `
vec4 render(vec3 base[np1]) {
	vec4 rgb = vec4(0, 0, 0, 1);`;

    		for(let j = 0; j < njpegs; j++) {
    			str += `
	{
		vec4 c = texture${gl2?'':'2D'}(plane${j}, v_texcoord);
		rgb.x += base[${j}].x*(c.x - bias[${j}].x)*scale[${j}].x;
		rgb.y += base[${j}].y*(c.y - bias[${j}].y)*scale[${j}].y;
		rgb.z += base[${j}].z*(c.z - bias[${j}].z)*scale[${j}].z;
	}
`;
    		}
    		str += `
	return rgb;
}
`;
    		return str;
    	}
    }

    class MRGB {
    	static render(njpegs, gl2) {
    		let str = `
vec4 render(vec3 base[np1]) {
	vec3 rgb = base[0];
	vec4 c;
	vec3 r;
`;
    		for(let j = 0; j < njpegs; j++) {
    			str +=
`	c = texture${gl2?'':'2D'}(plane${j}, v_texcoord);
	r = (c.xyz - bias[${j}])* scale[${j}];

	rgb += base[${j}*3+1]*r.x;
	rgb += base[${j}*3+2]*r.y;
	rgb += base[${j}*3+3]*r.z;
`    ;
    		}
    		str += `
	return vec4(rgb, 1);
}
`;
    		return str;
    	}
    }

    class MYCC {

    	static render(njpegs, ny1, gl2) {
    		let str = `
vec3 toRgb(vec3 ycc) {
 	vec3 rgb;
	rgb.g = ycc.r + ycc.b/2.0;
	rgb.b = ycc.r - ycc.b/2.0 - ycc.g/2.0;
	rgb.r = rgb.b + ycc.g;
	return rgb;
}

vec4 render(vec3 base[np1]) {
	vec3 rgb = base[0];
	vec4 c;
	vec3 r;
`;
    		for(let j = 0; j < njpegs; j++) {
    			str += `

	c = texture${gl2?'':'2D'}(plane${j}, v_texcoord);

	r = (c.xyz - bias[${j}])* scale[${j}];
`;

    			if(j < ny1) {
    				str += `
	rgb.x += base[${j}*3+1].x*r.x;
	rgb.y += base[${j}*3+2].y*r.y;
	rgb.z += base[${j}*3+3].z*r.z;
`;
    			} else {
    				str += `
	rgb.x += base[${j}*3+1].x*r.x;
	rgb.x += base[${j}*3+2].x*r.y;
	rgb.x += base[${j}*3+3].x*r.z;
`;
    			}
    		}
    		str += `	
	return vec4(toRgb(rgb), 1);
}
`;
    		return str;
    	}
    }




    /* PTM utility functions 
     */
    class PTM {
    	/* @param {Array} v expects light direction as [x, y, z]
    	*/
    	static lightWeights(v) {
    		let b = [1.0, v[0], v[1], v[0]*v[0], v[0]*v[1], v[1]*v[1]];
    		let base = new Float32Array(18);
    		for(let i = 0; i < 18; i++)
    			base[3*i] = base[3*i+1] = base[3*i+2] = b[i];
    		return base;
    	}
    }


    /* HSH utility functions 
     */
    class HSH {
    	/* @param {Array} v expects light direction as [x, y, z]
    	*/
    	static lightWeights(v) {
    		let PI = 3.1415;
    		let phi = Math.atan2(v[1], v[0]);
    		if (phi < 0)
    			phi = 2 * PI + phi;
    		let theta = Math.min(Math.acos(v[2]), PI / 2 - 0.1);

    		let cosP = Math.cos(phi);
    		let cosT = Math.cos(theta);
    		let cosT2 = cosT * cosT;

    		let b = [
    			1.0 / Math.sqrt(2 * PI),

    			Math.sqrt(6 / PI) * (cosP * Math.sqrt(cosT-cosT2)),
    			Math.sqrt(3 / (2 * PI)) * (-1 + 2*cosT),
    			Math.sqrt(6 / PI) * (Math.sqrt(cosT - cosT2) * Math.sin(phi)),

    			Math.sqrt(30 / PI) * (Math.cos(2 * phi) * (-cosT + cosT2)),
    			Math.sqrt(30 / PI) * (cosP*(-1 + 2 * cosT) * Math.sqrt(cosT - cosT2)),
    			Math.sqrt(5  / (2 * PI)) * (1 - 6 * cosT + 6 * cosT2),
    			Math.sqrt(30 / PI) * ((-1 + 2 * cosT) * Math.sqrt(cosT - cosT2) * Math.sin(phi)),
    			Math.sqrt(30 / PI) * ((-cosT + cosT2) * Math.sin(2*phi))
    		];
    		let base = new Float32Array(27);
    		for(let i = 0; i < 27; i++)
    			base[3*i] = base[3*i+1] = base[3*i+2] = b[i];
    		return base;
    	}
    }

    class SH {
    	/* @param {Array} v expects light direction as [x, y, z]
    	*/
    	static lightWeights(v) {
    		let PI = 3.1415;
    		let A = 0.5*Math.sqrt(3.0/PI);
    		let B = 0.5*Math.sqrt(15/PI);
    		let b = [
    			0.5/Math.sqrt(PI),
    			A*v[0],
    			A*v[2],
    			A*v[1],
    			B*v[0]*v[1],
    			B*v[0]*v[2],
    			0.5*Math.sqrt(5/PI)*(3*v[2]*v[2] - 1),
    			B*v[1]*v[2],
    			0.5*B*(v[1]*v[1] - v[0]*v[0])
    		];

    		let base = new Float32Array(27);
    		for(let i = 0; i < 27; i++)
    			base[3*i] = base[3*i+1] = base[3*i+2] = b[i];
    		return base;
    	}
    }


    class RBF {
    	/* @param {Array} v expects light direction as [x, y, z]
    	*/
    	static lightWeights(lpos, shader) {

    		let weights = RBF.rbf(lpos, shader);

    		let np = shader.nplanes;
    		let lweights = new Float32Array((np + 1) * 3);

    		for(let p = 0; p < np+1; p++) {
    			for(let k = 0; k < 3; k++) {
    				for(let l = 0; l < weights.length; l++) {
    					let o = shader.baseLightOffset(p, weights[l][0], k);
    					lweights[3*p + k] += weights[l][1]*shader.basis[o];
    				}
    			}
    		}
    		return lweights;
    	}

    	static rbf(lpos, shader) {
    		let radius = 1/(shader.sigma*shader.sigma);
    		let weights = new Array(shader.ndimensions);

    		//compute rbf weights
    		let totw = 0.0;
    		for(let i = 0; i < weights.length; i++) {
    			let dx = shader.lights[i*3+0] - lpos[0];
    			let dy = shader.lights[i*3+1] - lpos[1];
    			let dz = shader.lights[i*3+2] - lpos[2];

    			let d2 = dx*dx + dy*dy + dz*dz;
    			let w = Math.exp(-radius * d2);

    			weights[i] = [i, w];
    			totw += w;
    		}
    		for(let i = 0; i < weights.length; i++)
    			weights[i][1] /= totw;


    		//pick only most significant and renormalize
    		let count = 0;
    		totw = 0.0;
    		for(let i = 0; i < weights.length; i++) {
    			if(weights[i][1] > 0.001) {
    				weights[count++] =  weights[i];
    				totw += weights[i][1];
    			}
    		}

    		weights = weights.slice(0, count); 
    		for(let i = 0; i < weights.length; i++)
    			weights[i][1] /= totw;

    		return weights;
    	}
    }

    class BLN {
    	static lightWeights(lpos, shader) {
    		let np = shader.nplanes;
    		let s = Math.abs(lpos[0]) + Math.abs(lpos[1]) + Math.abs(lpos[2]);

    		//rotate 45 deg.
    		let x = (lpos[0] + lpos[1])/s;
    		let y = (lpos[1] - lpos[0])/s;
    		x = (x + 1.0)/2.0;
    		y = (y + 1.0)/2.0;
    		x = x*(shader.resolution - 1.0);
    		y = y*(shader.resolution - 1.0);

    		let sx = Math.min(shader.resolution-2, Math.max(0, Math.floor(x)));
    		let sy = Math.min(shader.resolution-2, Math.max(0, Math.floor(y)));
    		let dx = x - sx;
    		let dy = y - sy;

    		//bilinear interpolation coefficients.
    		let s00 = (1 - dx)*(1 - dy);
    		let s10 =      dx *(1 - dy);
    		let s01 = (1 - dx)* dy;
    		let s11 =      dx * dy;

    		let lweights = new Float32Array((np + 1) * 3);

    		//TODO optimize away basePixel

    		for(let p = 0; p < np+1; p++) {
    			for(let k = 0; k < 3; k++) {
    				let o00 = shader.basePixelOffset(p, sx, sy, k);
    				let o10 = shader.basePixelOffset(p, sx+1, sy, k);
    				let o01 = shader.basePixelOffset(p, sx, sy+1, k);
    				let o11 = shader.basePixelOffset(p, sx+1, sy+1, k);

    				lweights[3*p + k] = 
    					s00*shader.basis[o00] + 
    					s10*shader.basis[o10] +
    					s01*shader.basis[o01] +
    					s11*shader.basis[o11];

    			}
    		}
    		return lweights;
    	}
    }

    /**
     * Extends {@link Layer}.
     * @param {options} options Same as {@link Layer}, but url and layout are required.
     * **url**: points to a relight format .json
     * **plane**: url for the first coefficient (plane_0), needed for IIIF and IIP (without /info.json)
     */

    class LayerRTI extends Layer {
    	constructor(options) {
    		super(options);

    		if(Object.keys(this.rasters).length != 0)
    			throw "Rasters options should be empty!";

    		if(!this.url)
    			throw "Url option is required";

    		// if(!this.layout)
    		// 	this.layout = 'image';

    		// this.layout.setUrl(this.url);
    		// this.setLayout(this.layout);

    		this.shaders['rti'] = new ShaderRTI({ normals: this.normals });
    		this.setShader('rti');

    		let now = performance.now();
    		this.controls['light'] = { source:{ value: [0, 0], t: now }, target:{ value:[0, 0], t:now }, current:{ value:[0, 0], t:now } };
    		this.worldRotation = 0; //if the canvas or the layer rotate, light direction neeeds to be rotated too.
    		if(this.url)
    			this.loadJson(this.url);
    	}

    	imageUrl(url, plane) {
    		let path = this.url.substring(0, this.url.lastIndexOf('/')+1);
    		switch(this.layout.type) {
    			case 'image':    return path + plane + '.jpg';			case 'google':   return path + plane;			case 'deepzoom': return path + plane + '.dzi';			case 'tarzoom':  return path + plane + '.tzi';			case 'itarzoom':  return path + 'planes.tzi';			case 'zoomify':  return path + plane + '/ImageProperties.xml';			//case 'iip':      return this.plane.throw Error("Unimplemented");
    			case 'iiif': throw Error("Unimplemented");
    			default:     throw Error("Unknown layout: " + layout.type);
    		}
    	}

    /*
     * Alias for setControl
     * @param {Array} light light direction as an array [x, y]
     * @param {number} dt delay
     */
    	setLight(light, dt) {
    		this.setControl('light', light, dt);
    	}

    	loadJson(url) {
    		(async () => {
    			var response = await fetch(this.url);
    			if(!response.ok) {
    				this.status = "Failed loading " + this.url + ": " + response.statusText;
    				return;
    			}
    			let json = await response.json();
    			this.shader.init(json);
    			let urls = [];
    			for(let p = 0; p < this.shader.njpegs; p++) {
    				let url = this.imageUrl(this.url, 'plane_' + p);
    				urls.push(url);
    				let raster = new Raster({ format: 'vec3', attribute: 'coeff', colorspace: 'linear' });
    				this.rasters.push(raster);
    			}
    			if(this.normals) { // ITARZOOM must include normals and currently has a limitation: loads the entire tile 
    				let url = this.imageUrl(this.url, 'normals');
    				urls.push(url);
    				let raster = new Raster({ format: 'vec3', attribute: 'coeff', colorspace: 'linear' });
    				this.rasters.push(raster);				
    			}			
    			this.layout.setUrls(urls);

    		})().catch(e => { console.log(e); this.status = e; });
    	}

    /*
     *  Internal function: light control maps to light direction in the shader.
     */
    	interpolateControls() {
    		let done = super.interpolateControls();
    		if(!done) {
    			let light = this.controls['light'].current.value;
    			//this.shader.setLight(light);
    			let rotated = Transform.rotate(light[0], light[1], this.worldRotation*Math.PI);
    			this.shader.setLight([rotated.x, rotated.y]);
    		}
    		return done;
    	}
    	draw(transform, viewport) {
    		this.worldRotation = transform.a + this.transform.a;
    		return super.draw(transform, viewport);
    	}
    }

    Layer.prototype.types['rti'] = (options) => { return new LayerRTI(options); };

    /**
     *  @param {object} options
     *   mode: default is ward, can be [ward, diffuse, specular, normals]
     */

    class ShaderBRDF extends Shader {
    	constructor(options) {
    		super({});
    		this.modes = ['ward', 'diffuse', 'specular', 'normals'];
    		this.mode = 'ward';

    		Object.assign(this, options);
    		
    		const kdCS = this.colorspaces['kd'] == 'linear' ? 0 : 1;
    		const ksCS = this.colorspaces['ks'] == 'linear' ? 0 : 1;

    		const brightness = options.brightness ? options.brightness : 1.0;
    		const gamma = options.gamma ? options.gamma : 2.2;
    		const alphaLimits = options.alphaLimits ? options.alphaLimits : [0.01, 0.5];

    		this.uniforms = {
    			uLightInfo:          { type: 'vec4', needsUpdate: true, size: 4, value: [0.1, 0.1, 0.9, 0] },
    			uAlphaLimits:        { type: 'vec2', needsUpdate: true, size: 2, value: alphaLimits },
    			uBrightnessGamma:    { type: 'vec2', needsUpdate: true, size: 2, value: [brightness, gamma] },		
    			uInputColorSpaceKd:  { type: 'int', needsUpdate: true, size: 1, value: kdCS },
    			uInputColorSpaceKs:  { type: 'int', needsUpdate: true, size: 1, value: ksCS },
    		};

    		this.innerCode = '';
    		this.setMode(this.mode);
    	}

    	setLight(light) {
    		// Light with 4 components (Spot: 4th==1, Dir: 4th==0)
    		this.setUniform('uLightInfo', light);
    	}

    	setMode(mode) {
    		this.mode = mode;
    		switch(mode) {
    			case 'ward':
    				this.innerCode = 
    				`vec3 linearColor = (kd + ks * spec) * NdotL;
				linearColor += kd * 0.02; // HACK! adding just a bit of ambient`;
    			break;
    			case 'diffuse':
    				this.innerCode = 
    				`vec3 linearColor = kd;`;
    			break;
    			case 'specular':
    				this.innerCode = 
    				`vec3 linearColor = clamp((ks * spec) * NdotL, 0.0, 1.0);`;
    			break;
    			case 'normals':
    				this.innerCode = 
    				`vec3 linearColor = (N+vec3(1.))/2.;
				applyGamma = false;`;
    			break;
    			default:
    				console.log("ShaderBRDF: Unknown mode: " + mode);
    				throw Error("ShaderBRDF: Unknown mode: " + mode);
    		}
    		this.needsUpdate = true;
    	}

    	fragShaderSrc(gl) {
    		let gl2 = !(gl instanceof WebGLRenderingContext);
    		let hasGloss = this.samplers.findIndex( s => s.name == 'uTexGloss') != -1;
    		let hasKs = this.samplers.findIndex( s => s.name == 'uTexKs') != -1;	
    		let str = `${gl2? '#version 300 es' : ''}
precision highp float; 
precision highp int; 

#define NULL_NORMAL vec3(0,0,0)
#define SQR(x) ((x)*(x))
#define PI (3.14159265359)
#define ISO_WARD_EXPONENT (4.0)

${gl2? 'in' : 'varying'} vec2 v_texcoord;
uniform sampler2D uTexKd;
uniform sampler2D uTexKs;
uniform sampler2D uTexNormals;
uniform sampler2D uTexGloss;

uniform vec4 uLightInfo; // [x,y,z,w] (if .w==0 => Directional, if w==1 => Spot)
uniform vec2 uAlphaLimits;
uniform vec2 uBrightnessGamma;

uniform int uInputColorSpaceKd; // 0: Linear; 1: sRGB
uniform int uInputColorSpaceKs; // 0: Linear; 1: sRGB

${gl2? 'out' : ''}  vec4 color;

vec3 getNormal(const in vec2 texCoord) {
	vec3 n = texture(uTexNormals, texCoord).xyz;
	n = 2. * n - vec3(1.);
	float norm = length(n);
	if(norm < 0.5) return NULL_NORMAL;
	else return n/norm;
}

vec3 linear2sRGB(vec3 linearRGB) {
    bvec3 cutoff = lessThan(linearRGB, vec3(0.0031308));
    vec3 higher = vec3(1.055)*pow(linearRGB, vec3(1.0/2.4)) - vec3(0.055);
    vec3 lower = linearRGB * vec3(12.92);
    return mix(higher, lower, cutoff);
}

vec3 sRGB2Linear(vec3 sRGB) {
    bvec3 cutoff = lessThan(sRGB, vec3(0.04045));
    vec3 higher = pow((sRGB + vec3(0.055))/vec3(1.055), vec3(2.4));
    vec3 lower = sRGB/vec3(12.92);
    return mix(higher, lower, cutoff);
}

float ward(in vec3 V, in vec3 L, in vec3 N, in vec3 X, in vec3 Y, in float alpha) {

	vec3 H = normalize(V + L);

	float H_dot_N = dot(H, N);
	float sqr_alpha_H_dot_N = SQR(alpha * H_dot_N);

	if(sqr_alpha_H_dot_N < 0.00001) return 0.0;

	float L_dot_N_mult_N_dot_V = dot(L,N) * dot(N,V);
	if(L_dot_N_mult_N_dot_V <= 0.0) return 0.0;

	float spec = 1.0 / (4.0 * PI * alpha * alpha * sqrt(L_dot_N_mult_N_dot_V));
	
	//float exponent = -(SQR(dot(H,X)) + SQR(dot(H,Y))) / sqr_alpha_H_dot_N; // Anisotropic
	float exponent = -SQR(tan(acos(H_dot_N))) / SQR(alpha); // Isotropic
	
	spec *= exp( exponent );

	return spec;
}


void main() {
	vec3 N = getNormal(v_texcoord);
	if(N == NULL_NORMAL) {
		color = vec4(0.0);
		return;
	}

	vec3 L = (uLightInfo.w == 0.0) ? normalize(uLightInfo.xyz) : normalize(uLightInfo.xyz - gl_FragCoord.xyz);
	vec3 V = vec3(0.0,0.0,1.0);
    vec3 H = normalize(L + V);
	float NdotL = max(dot(N,L),0.0);

	vec3 kd = texture(uTexKd, v_texcoord).xyz;
	vec3 ks = ${hasKs ? 'texture(uTexKs, v_texcoord).xyz' : 'vec3(0.0, 0.0, 0.0)'};
	if(uInputColorSpaceKd == 1) {
		kd = sRGB2Linear(kd);
	}
	if(uInputColorSpaceKs == 1) {
		ks = sRGB2Linear(ks);
	}
	kd /= PI;

	float gloss = ${hasGloss ? 'texture(uTexGloss, v_texcoord).x' : '0.0'};
	float minGloss = 1.0 - pow(uAlphaLimits[1], 1.0 / ISO_WARD_EXPONENT);
	float maxGloss = 1.0 - pow(uAlphaLimits[0], 1.0 / ISO_WARD_EXPONENT);

	float alpha = pow(1.0 - gloss * (maxGloss - minGloss) - minGloss, ISO_WARD_EXPONENT);
	
	
	vec3 e = vec3(0.0,0.0,1.0);
	vec3 T = normalize(cross(N,e));
	vec3 B = normalize(cross(N,T));
	float spec = ward(V, L, N, T, B, alpha);
	
	bool applyGamma = true;

	${this.innerCode}

	vec3 finalColor = applyGamma ? pow(linearColor * uBrightnessGamma[0], vec3(1.0/uBrightnessGamma[1])) : linearColor;
	color = vec4(finalColor, 1.0);
	${gl2?'':'gl_FragColor = color;'}
}
`;
    	return str;
    	}

    }

    /**
     * Extends {@link Layer}.
     * @param {options} options Same as {@link Layer}, but channels(ks,kd,normals,gloss) are required.
     */

    class LayerBRDF extends Layer {
    	constructor(options) {
    		super(options);

    		if(Object.keys(this.rasters).length != 0)
    			throw "Rasters options should be empty!";

    		if(!this.channels)
    			throw "channels option is required";

    		if(!this.channels.kd || !this.channels.normals)
    			throw "kd and normals channels are required";
    	
    		if(!this.colorspaces) {
    			console.log("LayerBRDF: missing colorspaces: force both to linear");
    			this.colorspaces['kd'] = 'linear';
    			this.colorspaces['ks'] = 'linear';
    		}

    		let id = 0;
    		let urls = [];
    		let samplers = [];
    		for (let c in this.channels) {
    			let url = this.channels[c];
    			switch (c) {
    				case 'kd':
    					this.rasters.push(new Raster({ format: 'vec3', attribute: 'kd', colorspace: this.colorspaces['kd'] }));
    					samplers.push({ 'id': id, 'name': 'uTexKd' });
    					break;
    				case 'ks':
    					this.rasters.push(new Raster({ format: 'vec3',  attribute: 'ks',      colorspace: this.colorspaces['ks'] }));
    					samplers.push({ 'id': id, 'name': 'uTexKs' });
    					break;
    				case 'normals':
    					this.rasters.push(new Raster({ format: 'vec3',  attribute: 'normals', colorspace: 'linear' }));
    					samplers.push({ 'id': id, 'name': 'uTexNormals' });
    					break;
    				case 'gloss':
    					this.rasters.push(new Raster({ format: 'float', attribute: 'gloss',   colorspace: 'linear' }));
    					samplers.push({ 'id': id, 'name': 'uTexGloss' });
    					break;
    			}
    			urls[id] = url;
    			id++;
    		}
    		this.layout.setUrls(urls);
    		
    		let now = performance.now();
    		this.controls['light'] = { source:{ value: [0, 0], t: now }, target:{ value:[0, 0], t:now }, current:{ value:[0, 0], t:now } };
    		const brightness = options.brightness ? options.brightness : 1.0;
    		const gamma = options.gamma ? options.gamma : 2.2;
    		const alphaLimits = options.alphaLimits ? options.alphaLimits : [0.01, 0.5];

    		let shader = new ShaderBRDF({
    			'label': 'Rgb',
    			'samplers': samplers,
    			'colorspaces': this.colorspaces,
    			'brightness': brightness,
    			'gamma': gamma,
    			'alphaLimits': alphaLimits
    		});

    		this.shaders['brdf'] = shader;
    		this.setShader('brdf');
    	}

    	setLight(light, dt) {
    		let r2 =  light[0]*light[0] + light[1]*light[1];
    		if (r2 > 1.0) {
    			let r = Math.sqrt(r2);
    			light[0] /= r;
    			light[1] /= r;
    			r2 = 1.0;
    		}
    		light[2] = Math.sqrt(1-r2);
    		this.setControl('light', light, dt);
    	}

    	interpolateControls() {
    		let done = super.interpolateControls();
    		if(!done) {
    			let light = this.controls['light'].current.value;
    			let r2 =  light[0]*light[0] + light[1]*light[1];
    			if (r2 > 1.0) {
    				light[0] /= r2;
    				light[1] /= r2;
    				r2 = 1.0;
    			}
    			light[2] = Math.sqrt(1-r2);
    	

    			//let z = Math.sqrt(1 - light[0]*light[0] - light[1]*light[1]);
    			this.shader.setLight([light[0], light[1], light[2], 0]);
    		}
    		return done;
    	}
    }


    Layer.prototype.types['brdf'] = (options) => { return new LayerBRDF(options); };

    class ShaderLens extends Shader {
        constructor(options) {
            super(options);
            
            this.samplers = [
    			{ id:0, name:'source0' }, { id:1, name:'source1' }
    		];
            
            this.uniforms = {
                u_lens: { type: 'vec4', needsUpdate: true, size: 4, value: [0,0,100,10] },
                u_width_height: { type: 'vec2', needsUpdate: true, size: 2, value: [1,1]}
            };
            this.label = "ShaderLens";
            this.needsUpdate = true;
            this.secondLayerEnabled = false;
        }

        setSecondLayerEnabled(x) {
            this.secondLayerEnabled = x;
            this.needsUpdate = true;
        }

        setLensUniforms(lensViewportCoords, windowWH) {
            this.setUniform('u_lens', lensViewportCoords);
            this.setUniform('u_width_height', windowWH);
        }
        
    	fragShaderSrc(gl) {
    		let gl2 = !(gl instanceof WebGLRenderingContext);

            let samplerDeclaration = `uniform sampler2D ` + this.samplers[0].name + `;`;
            let secondSamplerCode = "";

            if (this.secondLayerEnabled) {
                samplerDeclaration += `uniform sampler2D ` + this.samplers[1].name + `;`;

                secondSamplerCode =  
                `vec4 c1 = texture(source1, v_texcoord);
            if (centerDist2 > lensR2) {
                float k = (c1.r + c1.g + c1.b) / 3.0;
                c1 = vec4(k, k, k, c1.a);
            }
            color = color * (1.0 - c1.a) + c1 * c1.a; `;
            }

            console.log("Shader code 0 " + samplerDeclaration);
            console.log("Shader code 1 " + secondSamplerCode);

    		return `${gl2? '#version 300 es':''}

        precision highp float; 
        precision highp int; 

        ${samplerDeclaration}
        uniform vec4 u_lens;
        uniform vec2 u_width_height; // Keep wh to map to pixels. TexCoords cannot be integer unless using texture_rectangle
        ${gl2? 'in' : 'varying'} vec2 v_texcoord;
        ${gl2? 'out' : ''} vec4 color;

        void main() {
            float lensR2 = u_lens.z * u_lens.z;
            float innerBorderR2 = (u_lens.z - u_lens.w) * (u_lens.z - u_lens.w);
            float dx = v_texcoord.x * u_width_height.x - u_lens.x;
            float dy = v_texcoord.y * u_width_height.y - u_lens.y;
            float centerDist2 = dx*dx+dy*dy;

            color = vec4(0.0, 0.0, 0.0, 0.0);
            if (centerDist2 < innerBorderR2) {
                color = texture(source0, v_texcoord);
            } else if (centerDist2 < lensR2) {
                const float k = 0.8;
                color = vec4(k,k,k,1.0);
            }
            ${secondSamplerCode}
            ${gl2?'':'gl_FragColor = color;'}

        }
        `
        }

        vertShaderSrc(gl) {
    		let gl2 = !(gl instanceof WebGLRenderingContext);
    		return `${gl2? '#version 300 es':''}

precision highp float; 
precision highp int; 

${gl2? 'in' : 'attribute'} vec4 a_position;
${gl2? 'in' : 'attribute'} vec2 a_texcoord;

${gl2? 'out' : 'varying'} vec2 v_texcoord;
void main() {
	gl_Position = a_position;
    v_texcoord = a_texcoord;
}`;
    	}
    }

    /**
     * options must contain one layer and lens = {x:, y:, r:, border: }
     */
    class LayerLens extends LayerCombiner {
    	constructor(options) {
    		options = Object.assign({
    			overlay: true
    		}, options);
    		super(options);
    		
    		// Shader lens currently handles up to 2 layers
    		let shader = new ShaderLens();
    		if (this.layers.length == 2) shader.setSecondLayerEnabled(true);
    		this.shaders['lens'] = shader;
    		this.setShader('lens');

    		this.startPos = [0, 0];
    		this.border = 2;

    		let now = performance.now();
    		this.controls['center'] = { source:{ value: [0, 0],    t: now }, target:{ value:[0, 0],    t:now }, current:{ value:[0, 0],    t:now } };
    		this.controls['radius'] = { source:{ value: [0, 0],    t: now }, target:{ value:[0, 0],    t:now }, current:{ value:[0, 0],    t:now } };
    		this.setLens(0,0,this.radius,this.border);
    		this.signals.draw = [];
    	}

    	setSecondLayerEnabled(x) {
    		if (this.layers.length == 2) {
    			// With two layers set visible or not the second layer and set the property in the shader
    			this.layers[1].setVisible(x);
    			this.shader.setSecondLayerEnabled(x);
    		} else if (!x) {
    			// With a single layer, just tell the shader to use only the first layer 
    			this.shader.setSecondLayerEnabled(x);
    		}

    	}

    	setLens(x = 0, y = 0, r = 100, border = 10) {
    		this.border = border;
    		this.setCenter(x, y);
    		this.setRadius(r);
    	}
    	
    	setRadius(r, delayms = 100) {
    		this.setControl('radius', [r, 0], delayms);
    	}

    	getRadius() {
    		return this.controls['radius'].current.value[0];
    	}

    	setCenter(x, y, delayms = 100) {
    		this.setControl('center', [x, y], delayms);
    	}

    	getCurrentCenter() {
    		return this.controls['center'].current.value;
    	}

    	getTargetCenter() {
    		return this.controls['center'].target.value;
    	}

    	draw(transform, viewport) {
    		let done = this.interpolateControls();
    		const vlens = this.getLensInViewportCoords(transform, viewport);
    		this.shader.setLensUniforms(vlens, [viewport.w, viewport.h]);
    		this.emit('draw');

    		super.draw(transform, viewport);

    		return done;
    	}

    	getLensViewport(transform, viewport) {
    		const lensC = this.getCurrentCenter();
    		const l = transform.sceneToViewportCoords(viewport, lensC);
    		const r = this.getRadius() * transform.z;
    		return {x: Math.floor(l[0]-r), y: Math.floor(l[1]-r), dx: Math.ceil(2*r), dy: Math.ceil(2*r), w:viewport.w, h:viewport.h};
    	}

    	getLensInViewportCoords(transform, viewport) {
    		const lensC = this.getCurrentCenter();
    		const c = transform.sceneToViewportCoords(viewport, lensC);
    		const r = this.getRadius();
    		return [c[0],  c[1], r * transform.z, this.border];
    	}

    }

    Layer.prototype.types['lens'] = (options) => { return new LayerLens(options); };

    class ControllerLens extends Controller {
    	constructor(options) {

    		super(options);

            if (!options.lensLayer) {
                console.log("ControllerLens lensLayer option required");
                throw "ControllerLens lensLayer option required";
            }
     
            if (!options.camera) {
                console.log("ControllerLens camera option required");
                throw "ControllerLens camera option required";
            }

            this.panning = false;
            this.zooming = false;
            this.initialDistance = 0;
            this.startPos = [0, 0];
        }

    	panStart(e) {
            if (!this.active)
                return;

            const p = this.getScenePosition(e);
            this.panning = false;

            if (this.isInsideLens(p)) {
                this.panning = true;
                e.preventDefault();
            }
    	}

    	panMove(e) {
            // Discard events due to cursor outside window
            if (Math.abs(e.offsetX) > 64000 || Math.abs(e.offsetY) > 64000) return;
            if(this.panning) {
                const p = this.getScenePosition(e);
                const dx = p[0]-this.startPos[0];
                const dy = p[1]-this.startPos[1];
                const c = this.lensLayer.getTargetCenter();
        
                this.lensLayer.setCenter(c[0] + dx, c[1] + dy);
                this.startPos = p;
                e.preventDefault();
            }
    	}

    	panEnd(e) {
    		if(!this.panning)
    			return;
    		this.panning = false;
    	}

    	pinchStart(e1, e2) {
            if (!this.active)
                return;

            const p0 = this.getScenePosition(e1);
            const p1 = this.getScenePosition(e2);
            const pc = [(p0[0]+ p1[0]) * 0.5, (p0[1] + p1[1]) * 0.5];

            if (this.isInsideLens(pc)) {
                this.zooming = true;
                this.initialDistance = this.distance(e1, e2);
                this.initialRadius = this.lensLayer.getRadius();

                e1.preventDefault();
            } 
    	}

    	pinchMove(e1, e2) {
    		if (!this.zooming)
                return;
            const d = this.distance(e1, e2);
    		const scale = d / (this.initialDistance + 0.00001);
            const newRadius = scale * this.initialRadius;
            this.lensLayer.setRadius(newRadius);
    	}

    	pinchEnd(e, x, y, scale) {
    		this.zooming = false;
        }
        
        mouseWheel(e) {
            const p = this.getScenePosition(e);
            let result = false;
            if (this.isInsideLens(p)) {
                const delta = e.deltaY > 0 ? 1 : -1;
                const factor = delta > 0 ? 1.2 : 1/1.2;
                const r = this.lensLayer.getRadius();
                this.lensLayer.setRadius(r*factor);

                result = true;
                e.preventDefault();
            } 
            
            return result;
        }

    	getScenePosition(e, t = null) {
            let x = e.offsetX;
            let y = e.offsetY;
            let rect = e.target.getBoundingClientRect();

            // Transform canvas p to scene coords
            if (t == null) {
                let now = performance.now();
                t = this.camera.getCurrentTransform(now);
            }
            const p = t.viewportToSceneCoords(this.camera.viewport, [x, rect.height- y]);
            
            return p;
        }

    	distance(e1, e2) {
    		return Math.sqrt(Math.pow(e1.x - e2.x, 2) + Math.pow(e1.y - e2.y, 2));
    	}

        isInsideLens(p) {
            const c = this.lensLayer.getCurrentCenter();
            const dx = p[0] - c[0];
            const dy = p[1] - c[1];
            const d2 = dx*dx + dy*dy;
            const r = this.lensLayer.getRadius();
            const res = d2 < r * r;
            if (res) { this.startPos = p;}
            return res;
        }
    }

    class ControllerFocusContext extends ControllerLens {
        static callUpdate(param) {
            param.update();
        }
        
        constructor(options) {
            super(options);
            if (!options.lensLayer) {
                console.log("ControllerFocusContext lensLayer option required");
                throw "ControllerFocusContext lensLayer option required";
            }
     
            if (!options.camera) {
                console.log("ControllerFocusContext camera option required");
                throw "ControllerFocusContext camera option required";
            }

            if (!options.canvas) {
                console.log("ControllerFocusContext canvas option required");
                throw "ControllerFocusContext canvas option required";
            }

            let callback = () => {
                const bbox = this.camera.boundingBox;
                this.maxDatasetSize = Math.max(bbox.width(), bbox.height());
                this.minDatasetSize = Math.min(bbox.width(), bbox.height());
                this.setDatasetDimensions(bbox.width(), bbox.height());
    		};
            this.canvas.addEvent('updateSize', callback);

           
            this.updateTimeInterval = 10;
            this.updateDelay = 200; 
            this.zoomDelay = 200;
            this.zoomAmount = 1.2;
            this.radiusFactorFromBoundary = 1.5; // Distance Lens Center Canvas Border in radii
            this.maxMinRadiusRatio = 3;

            this.image_width = 1;
            this.image_height = 1;
            
            this.FocusContextEnabled = true;

            this.centerToClickOffset = [0, 0];
            this.previousClickPos = [0, 0];
            this.currentClickPos = [0, 0];

            this.insideLens = false;
            
            // this.touchZoom = false;
            // this.touchZoomDist = 0;
            // this.previousTouchZoomDist = 0;
            // this.lastDeltaTouchZoomDist = 0;
        }

    	panStart(e) {
            if (!this.active)
                return;
                
            const t = this.camera.getCurrentTransform(performance.now());
            const p = this.getScenePosition(e, t);
            this.panning = false;
            this.insideLens = this.isInsideLens(p);

            if (this.insideLens) {
                e.preventDefault();
                const startPos = this.getPixelPosition(e); 
                this.panning = true;

                const lc = this.getScreenPosition(this.getFocus().position, t);
                this.centerToClickOffset = [startPos[0] - lc[0], startPos[1] - lc[1]];
                this.currentClickPos = [startPos[0], startPos[1]];
            } 

            // Activate a timeout to call update() in order to update position also when mouse is clicked but steady
            // Stop the time out on panEnd
            this.timeOut = setInterval(this.update.bind(this), 20);
    	}

        panMove(e) {
            if (Math.abs(e.offsetX) > 64000 || Math.abs(e.offsetY) > 64000) return;
            if(this.panning) {
                this.currentClickPos = this.getPixelPosition(e);
            }  
        }

        mouseWheel(e) {
            const p = this.getScenePosition(e);
            this.insideLens = this.isInsideLens(p);
            let focus = this.getFocus();
            const now = performance.now();
            let context = this.camera.getCurrentTransform(now);

            if (this.insideLens) {
                const dz = e.deltaY  > 0 ? this.zoomAmount : 1/this.zoomAmount;
                this.scaleLensContext(focus, context, dz);
            } else {
                const pos = this.camera.mapToScene(e.offsetX, e.offsetY, this.camera.getCurrentTransform(now));
                let dz =  e.deltaY < 0 ? this.zoomAmount : 1/this.zoomAmount;

                // Clamp to zoom limits
                const maxDeltaZoom = this.camera.maxZoom / context.z;
                const minDeltaZoom = this.camera.minZoom / context.z;
                dz = Math.min(maxDeltaZoom, Math.max(minDeltaZoom, dz));
                
                // Zoom aroun cursor position
                this.camera.deltaZoom(this.updateDelay, dz, pos.x, pos.y);
                context = this.camera.getCurrentTransform(performance.now());
            }  

            e.preventDefault();
            return true;
        }

        scaleLensContext(focus, context, dz) {
            const zoomAmountMax = 1.5;
            const zoomAmountMin = 1.3;

            const minRadius = Math.min(this.canvas.gl.canvas.clientWidth,  this.canvas.gl.canvas.clientHeight) * 0.1;
            const maxRadius = minRadius * this.maxMinRadiusRatio;
            const r = focus.radius * context.z;

            // Distribute lens scale between radius scale and context scale
            // When radius is going outside radius boundary, scale of the inverse amounts radius and zoom scale | screen size constant
            // When radius is changing from boundary condition to a valid one change radius of maxamount, and no change to zoom scale.
            // In between interpolate.
            const t = Math.max(0, Math.min(1, (r - minRadius) / (maxRadius - minRadius)));
            let zoomScaleAmount = dz > 1 ? 1 * (1-t) + (1 / zoomAmountMin) * t       : (1-t) * zoomAmountMin + 1 * t;
            let radiusScaleAmount = dz > 1 ? zoomAmountMax * (1-t) + zoomAmountMin * t : (1-t) / zoomAmountMin + 1 /zoomAmountMax * t;
            const newR = r * radiusScaleAmount;

            // Clamp radius
            if (newR < minRadius) {
                radiusScaleAmount = minRadius / r;
            } else if (newR > maxRadius) {
                radiusScaleAmount = maxRadius / r;
            }
            // Clamp scale
            if (context.z * zoomScaleAmount < this.camera.minZoom) {
                zoomScaleAmount = this.camera.minZoom / context.z;
            } else if (context.z * zoomScaleAmount > this.camera.maxZoom) {
                zoomScaleAmount = this.camera.maxZoom / context.z;
            }

            // Scale around lens center
            context.x += focus.position[0]*context.z*(1 - zoomScaleAmount);
            context.y -= focus.position[1]*context.z*(1 - zoomScaleAmount);
            context.z = context.z * zoomScaleAmount;  
            focus.radius *= radiusScaleAmount;

            // Bring the lens within the focus&context condition after a zoom operation
            if (dz != 1) {
                const delta = this.getCanvasBorder(focus, context);
                let box = this.getShrinkedBox(delta);
                const screenP = context.sceneToViewportCoords(this.camera.viewport, focus.position);
                for(let i = 0; i < 2; ++i) {
                    const deltaMin = Math.max(0, (box.min[i] - screenP[i]));
                    const deltaMax = Math.min(0, (box.max[i] - screenP[i]));
                    let delta = deltaMin != 0 ? deltaMin : deltaMax;
                    if (i == 0) {
                        context.x += delta;
                    } else {
                        context.y -= delta;
                    }
                }
            }

            // Apply scales
            this.camera.setPosition(this.zoomDelay, context.x, context.y, context.z, context.a);
            this.lensLayer.setRadius(focus.radius, this.zoomDelay);
        }

        panEnd() {
            this.panning = false;
            this.zooming = false;
            clearTimeout(this.timeOut);
        }

         update() {
            if (this.panning) {
                const t = this.camera.getCurrentTransform(performance.now());
                let lensDeltaPosition = this.lastInteractionDelta(t);
                lensDeltaPosition[0] /= t.z;
                lensDeltaPosition[1] /= t.z;
                this.panLens(lensDeltaPosition);
                this.previousClickPos = [this.currentClickPos[0], this.currentClickPos[1]];
            } 
        }

        lastInteractionDelta(t) {
            let result = [0, 0];
            // Compute delta with respect to previous position
            if (this.panning && this.insideLens) {
                // For lens pan Compute delta wrt previous lens position
                const lc = this.getScreenPosition(this.getFocus().position, t);
                result =
                    [this.currentClickPos[0] - lc[0] - this.centerToClickOffset[0],
                     this.currentClickPos[1] - lc[1] - this.centerToClickOffset[1]];
            } else {
                // For camera pan Compute delta wrt previous click position
                result = 
                    [this.currentClickPos[0] - this.previousClickPos[0],
                     this.currentClickPos[1] - this.previousClickPos[1]];
            }
          
            return result;
        }

        panLens(delta) {
            let context = this.camera.getCurrentTransform(performance.now());
            let focus = this.getFocus();

            if (this.FocusContextEnabled) { 
                // adjust camera to maintain the focus and context condition
                let txy = this.getAmountOfFocusContext(this.camera.viewport, focus, context, delta);
                // When t is 1: already in focus&context, move only the lens.
                // When t is 0.5: border situation, move both focus & context to keep the lens steady on screen.
                // In this case the context should be moved of deltaFocus*scale to achieve steadyness.
                // Thus interpolate deltaContext between 0 and deltaFocus*s (with t ranging from 1 to 0.5)
                const deltaFocus = [delta[0] * txy[0], delta[1] * txy[1]];
                const deltaContext = [-deltaFocus[0] * context.z * 2 * (1-txy[0]), 
                                       deltaFocus[1] * context.z * 2 * (1-txy[1])];
                context.x += deltaContext[0];
                context.y += deltaContext[1];

                focus.position[0] += deltaFocus[0];
                focus.position[1] += deltaFocus[1];

                // Clamp lens position on dataset boundaries
                if (Math.abs(focus.position[0]) > this.image_width/2) {
                    focus.position[0] = this.image_width/2 * Math.sign(focus.position[0]);
                }

                if (Math.abs(focus.position[1]) > this.image_height/2) {
                    focus.position[1] = this.image_height/2 * Math.sign(focus.position[1]);
                } 

                // Apply changes to camera and lens
                this.camera.setPosition(this.updateDelay, context.x, context.y, context.z, context.a);
                this.lensLayer.setCenter(focus.position[0], focus.position[1], this.updateDelay);
                this.lensLayer.setRadius(focus.radius);
            } else {
                this.lensLayer.setCenter(focus.position[0] + delta[0], focus.position[1] + delta[1], this.updateDelay);
            }
        }

        getFocus() {
            const p = this.lensLayer.getCurrentCenter();
            const r = this.lensLayer.getRadius();
            return  {position: p, radius: r}
        }
        
        setDatasetDimensions(width, height) {
            this.image_width = width;
            this.image_height = height;
        }

        initLens() {
            const t = this.camera.getCurrentTransform(performance.now());
            const imageRadius = 100 / t.z;
            this.lensLayer.setRadius(imageRadius);
            this.lensLayer.setCenter(this.image_width * 0.5, this.image_height);
        }
        
        getPixelPosition(e) {
            let x = e.offsetX;
            let y = e.offsetY;
            let rect = e.target.getBoundingClientRect();
            return [x, rect.height - y];
        }

        getScreenPosition(p, t) {
            // Transform from p expressed wrt world center (at dataset center is 0,0)
            // to Viewport coords 0,w 0,h
            const c = t.sceneToViewportCoords(this.camera.viewport, p);
            return c;
        }

        isInsideLens(p) {
            const c = this.lensLayer.getTargetCenter();
            const dx = p[0] - c[0];
            const dy = p[1] - c[1];
            const d = Math.sqrt(dx*dx + dy*dy);
            const r = this.lensLayer.getRadius();
            const within = d < r;
            //const onBorder = within && d >= r-this.lensLayer.border;
            return within;
        }
        
        getAmountOfFocusContext(viewport, focus, context, panDir) {
            // Return a value among 0.5 and 1. 1 is full focus and context,
            // 0.5 is borderline focus and context. 
            const delta = this.getCanvasBorder(focus, context);
            const box = this.getShrinkedBox(delta);
            const p = context.sceneToViewportCoords(viewport, focus.position); 

            const halfCanvasW = this.canvas.gl.canvas.clientWidth / 2 - delta;
            const halfCanvasH = this.canvas.gl.canvas.clientHeight / 2 - delta;
        
            let xDistance = (panDir[0] > 0 ?
              Math.max(0, Math.min(halfCanvasW, box.max[0] - p[0])) / (halfCanvasW) :
              Math.max(0, Math.min(halfCanvasW, p[0] - box.min[0])) / (halfCanvasW));
            xDistance = this.smoothstep(xDistance, 0, 0.75);
        
            let yDistance = (panDir[1] > 0 ?
              Math.max(0, Math.min(halfCanvasH, box.max[1] - p[1])) / (halfCanvasH) :
              Math.max(0, Math.min(halfCanvasH, p[1] - box.min[1])) / (halfCanvasH));
            yDistance = this.smoothstep(yDistance, 0, 0.75);
            
            // Use d/2+05, because when d = 0.5 camera movement = lens movement 
            // with the effect of the lens not moving from its canvas position.
            const txy =  [xDistance / 2 + 0.5, yDistance / 2 + 0.5];
            return txy;
        }

        getCanvasBorder(focus, context) {
            return context.z * focus.radius * this.radiusFactorFromBoundary; // Distance Lens Center Canvas Border
        }
          
        getShrinkedBox(delta) {
            const width = this.canvas.gl.canvas.clientWidth;
            const height = this.canvas.gl.canvas.clientHeight;
            const box = {
            min: [delta, delta],
            max: [width - delta, height - delta]
            };
            return box;
        }

        smoothstep(x, x0, x1) {
            if (x < x0) {
                return 0;
            } else if (x > x1) {
                return 1;
            } else {
                const t = (x - x0) / (x1 - x0);
                return t * t * (-2 * t + 3);
            }
        }

        // pinchStart(e0, e1) {
        //     this.touchZoom = true;
        //     this.isInteractionActive = true;

        //     const p0 = this.getScenePosition(e1);
        //     const p1 = this.getScenePosition(e2);
        //     const pc = [(p0[0]+ p1[0]) * 0.5, (p0[1] + p1[1]) * 0.5];

        //     // const d01 = [p0[0] - p1[0], p0[1] - p1[1]];
        //     // this.touchZoomDist = Math.sqrt(d01[0] * d01[0] + d01[1] * d01[1]);
        //     // this.previousTouchZoomDist = this.touchZoomDist;
        //     // this.startPos = [pc[0], pc[1]];
        
        //     if (this.isInsideLens(pc)) {
        //         this.interactionType = InteractionType.TOUCHZOOM;
        //         this.initialDistance = this.distance(e1, e2);
        //         this.initialRadius = this.lensLayer.getRadius();

        //         e1.preventDefault();
        //     }

        //     this.lastUpdateTimeEvent = new Date();
        // }

    	// pinchMove(e1, e2) {
        //     if (this.zooming) {
            //     const d = this.distance(e1, e2);
            //     const scale = d / (this.initialDistance + 0.00001);
            //     const newRadius = scale * this.initialRadius;
            //     this.lensLayer.setRadius(newRadius);
        //     }
        // }

        // pinchEnd(e, x, y, scale) {
    	// 	this.zooming = false;
        //  this.touchZoom = false;
        // }

    }

    /** coordinates for annotations are relative to the top left corner!!!!
     */

    class Annotation {
    	constructor(options) {
    		Object.assign(
    			this, 
    			{
    				id: Annotation.UUID(),
    				code: null,
    				label: null,
    				description: null,
    				class: null,
    				target: null,
    				svg: null,
    				data: {},
    				style: null,
    				bbox: null,
    				visible: true,

    				ready: false, //already: convertted to svg
    				needsUpdate: true,
    				editing: false,
    			}, 
    			options);
    			//TODO label as null is problematic, sort this issue.
    			if(!this.label) this.label = ''; 
    			this.elements = []; //assign options is not recursive!!!
    	}

    	static UUID() {
    		return 'axxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function(c) {
    			var r = Math.random() * 16 | 0, v = c == 'x' ? r : (r & 0x3 | 0x8);
    			return v.toString(16);
    		});
    	}

    	getBBoxFromElements() {
    		let box = { x: 0, y: 0, width: 0, height: 0 };
    		if(!this.elements.length)
    			return box;
    		let { x, y, width, height } = this.elements[0].getBBox();
    		for(let shape of this.elements) {
    				const { sx, sy, swidth, sheight } = shape.getBBox();
    				x = Math.min(x, sx);
    				y = Math.min(x, sy);
    				width = Math.max(width + x, sx + swidth) - x; 
    				height = Math.max(height + y, sy + sheight) - y; 
    		}
    		return { x, y, width, height };
    	}

    	static fromJsonLd(entry) {
    		if(entry.type != 'Annotation')
    			throw "Not a jsonld annotation.";
    		let options = {id: entry.id};

    		let rename = { 'identifying': 'code', 'identifying': 'label', 'describing': 'description', 'classifying':'class' };
    		for(let item of entry.body) {
    			let field = rename[item.purpose];
    			if(field)
    				options[field] = item.value;
    		}
    		let selector = entry.target && entry.target.selector;
    		if(selector) {
    			switch(selector.type) {
    			case 'SvgSelector':
    				options.svg = selector.value;
    				options.elements = [];
    				break;
    			default:
    				throw "Unsupported selector: " + selector.type;
    			}
    		}
    		return new Annotation(options);
    	}
    	toJsonLd() {
    		let body = [];
    		if(this.code !== null)
    			body.push( { type: 'TextualBody', value: this.code, purpose: 'indentifying' });
    		if(this.class !== null)
    			body.push( { type: 'TextualBody', value: this.class, purpose: 'classifying' });
    		if(this.description !== null)
    			body.push( { type: 'TextualBody', value: this.description, purpose: 'describing' });

    		({
    			"@context": "http://www.w3.org/ns/anno.jsonld",
    			id: this.id,
    			type: "Annotation",
    			body: body,
    			target: { selector: {} }
    		});
    		if(this.target)
    			target.selector.source = this.target;


    		if(this.element) {
    			var s = new XMLSerializer();
    			s.serializeToString(this.element);
    		}
    	}
    }

    /**
     * SVG or OpenGL polygons/lines/points annotation layer
     * @param {object} options
     * * *svgURL*: url for the svg containing the annotations
     * * *svgXML*: svg string containing the annotatiosn
     * * *geometry*: TODO: should contain the areas/lines/points for OpenGL rendering
     * * *style*: css style for the annotation elements (shadow dom allows css to apply only to this layer)
     * * *annotations*: collection of annotations info: each annotations is id: { label, svg (optional), data (custom data) (TODO)
     */


    class LayerAnnotation extends Layer {
    	constructor(options) {
    		options = Object.assign({
    			// geometry: null,  //unused, might want to store here the quads/shapes for opengl rendering
    			style: null,    //straightforward for svg annotations, to be defined oro opengl rendering
    			annotations: [],
    			hoverable: false, //display info about annotation on mousehover.
    			selected: new Set,
    			overlay: true,
    			annotationsListEntry: null, //TODO: horrible name for the interface list of annotations
    		}, options);
    		super(options);

    		this.signals.selected = [];
    		this.signals.loaded = [];

    		if (typeof (this.annotations) == "string") { //assumes it is an URL
    			(async () => { await this.loadAnnotations(this.annotations); })();
    		}
    	}

    	async loadAnnotations(url) {
    		var response = await fetch(url);
    		if(!response.ok) {
    			this.status = "Failed loading " + this.url + ": " + response.statusText;
    			return;
    		}
    		this.annotations = await response.json();
    		if(this.annotations.status == 'error') {
    			alert("Failed to load annotations: " + this.annotations.msg);
    			return;
    		}
    		//this.annotations = this.annotations.map(a => '@context' in a ? Annotation.fromJsonLd(a): a);
    		this.annotations = this.annotations.map(a => new Annotation(a));
    		for(let a of this.annotations)
    			if(a.publish != 1)
    				a.visible = false;
    		this.annotations.sort((a, b) => a.label.localeCompare(b.label));
    		if(this.annotationsListEntry)
    			this.createAnnotationsList();
    		
    		this.emit('update');
    		this.emit('ready');
    		this.emit('loaded');
    	}


    	newAnnotation(annotation, selected = true) {
    		if(!annotation)
    			annotation = new Annotation();

    		this.annotations.push(annotation);
    		let html = this.createAnnotationEntry(annotation);
    		let template = document.createElement('template');
    		template.innerHTML = html.trim();
    		
    		let list =  this.annotationsListEntry.element.parentElement.querySelector('.openlime-list');
    		list.appendChild(template.content.firstChild);
    		
    		this.clearSelected();
    		this.setSelected(annotation);

    		return annotation;
    	}


    	annotationsEntry() {
    		return this.annotationsListEntry =  {
    			html: '',
    			list: [], //will be filled later.
    			classes: 'openlime-annotations',
    			status: () => 'active',
    			oncreate: () => { 
    				if(Array.isArray(this.annotations))
    					this.createAnnotationsList();
    			}
    		}
    	}

    	createAnnotationsList() {
    		let html ='';
    		for(let a of this.annotations) {
    			html += this.createAnnotationEntry(a);
    		}

    		let list =  this.annotationsListEntry.element.parentElement.querySelector('.openlime-list');
    		list.innerHTML = html;
    		list.addEventListener('click', (e) =>  { 
    			let svg = e.srcElement.closest('svg');
    			if(svg) {
    				let entry = svg.closest('[data-annotation]');
    				entry.classList.toggle('hidden');
    				let id = entry.getAttribute('data-annotation');
    				let anno = this.getAnnotationById(id);
    				anno.visible = !anno.visible;
    				anno.needsUpdate = true;
    				this.emit('update');
    			}

    			let id = e.srcElement.getAttribute('data-annotation');
    			if(id) {
    				this.clearSelected();
    				let anno = this.getAnnotationById(id);
    				this.setSelected(anno, true);
    			}
    		});
    	}

    	createAnnotationEntry(a) {
    		return `<a href="#" data-annotation="${a.id}" class="openlime-entry ${a.visible == 0? 'hidden':''}">${a.label || ''}
			<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="openlime-eye"><path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path><circle cx="12" cy="12" r="3"></circle></svg>
			<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="openlime-eye-off"><path d="M17.94 17.94A10.07 10.07 0 0 1 12 20c-7 0-11-8-11-8a18.45 18.45 0 0 1 5.06-5.94M9.9 4.24A9.12 9.12 0 0 1 12 4c7 0 11 8 11 8a18.5 18.5 0 0 1-2.16 3.19m-6.72-1.07a3 3 0 1 1-4.24-4.24"></path><line x1="1" y1="1" x2="23" y2="23"></line></svg>
			</a>`;
    	}

    	getAnnotationById(id) {
    		for(const anno of this.annotations)
    			if(anno.id == id)
    				return anno;
    		return null;
    	}

    	clearSelected() {
    		this.annotationsListEntry.element.parentElement.querySelectorAll(`[data-annotation]`).forEach((e) => e.classList.remove('selected'));
    		this.selected.clear();
    	}
    	//set selected class for annotation
    	setSelected(anno, on = true) {
    		this.annotationsListEntry.element.parentElement.querySelector(`[data-annotation="${anno.id}"]`).classList.toggle('selected', on);
    		if(on)
    			this.selected.add(anno.id);
    		else
    			this.selected.delete(anno.id);
    		this.emit('selected', anno);
    	}
    }

    /**
     * SVG or OpenGL polygons/lines/points annotation layer
     * @param {object} options
     * * *svgURL*: url for the svg containing the annotations
     * * *svgXML*: svg string containing the annotatiosn
     * * *style*: css style for the annotation elements (shadow dom allows css to apply only to this layer)
     */

    class LayerSvgAnnotation extends LayerAnnotation {

    	constructor(options) {
    		options = Object.assign({
    			svgURL: null,
    			svgXML: null,
    			overlayElement: null,    //reference to canvas overlayElement. TODO: check if really needed.
    			shadow: true,            //svg attached as shadow node (so style apply
    			svgElement: null, //the svg layer
    			svgGroup: null,
    			classes: {
    				'': { stroke: '#000', label: '' },
    			}
    		}, options);
    		super(options);
    		this.style += Object.entries(this.classes).map((g) => `[data-class=${g[0]}] { stroke:${g[1].stroke}; }`).join('\n');
    		//this.createSVGElement();
    		//this.setLayout(this.layout);
    	}

    	createSVGElement() {
    		this.svgElement = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
    		this.svgElement.classList.add('openlime-svgoverlay');
    		this.svgGroup = document.createElementNS('http://www.w3.org/2000/svg', 'g');
    		this.svgElement.append(this.svgGroup);

    		let root = this.overlayElement;
    		if (this.shadow)
    			root = this.overlayElement.attachShadow({ mode: "open" });

    		if (this.style) {
    			const style = document.createElement('style');
    			style.textContent = this.style;
    			root.append(style);
    		}
    		root.appendChild(this.svgElement);
    	}
    	/*  unused for the moment!!! 
    		async loadSVG(url) {
    			var response = await fetch(url);
    			if (!response.ok) {
    				this.status = "Failed loading " + this.url + ": " + response.statusText;
    				return;
    			}
    			let text = await response.text();
    			let parser = new DOMParser();
    			this.svgXML = parser.parseFromString(text, "image/svg+xml").documentElement;
    			throw "if viewbox is set in svgURL should it overwrite options.viewbox or viceversa?"
    		}
    	*/

    	setVisible(visible) {
    		if (this.svgElement)
    			this.svgElement.style.display = visible ? 'block' : 'none';
    		super.setVisible(visible);
    	}

    	clearSelected() {
    		if (!this.svgElement) this.createSVGElement();
    		//		return;
    		this.svgGroup.querySelectorAll('[data-annotation]').forEach((e) => e.classList.remove('selected'));
    		super.clearSelected();
    	}

    	setSelected(anno, on = true) {
    		for (let a of this.svgElement.querySelectorAll(`[data-annotation="${anno.id}"]`))
    			a.classList.toggle('selected', on);

    		super.setSelected(anno, on);
    	}


    	newAnnotation(annotation, selected = true) {
    		let svg = createElement$1('svg');
    		if (!annotation)
    			annotation = new Annotation({ element: svg, selector_type: 'SvgSelector' });
    		return super.newAnnotation(annotation, selected)
    	}

    	draw(transform, viewport) {
    		if (!this.svgElement)
    			return true;
    		let t = this.transform.compose(transform);
    		this.svgElement.setAttribute('viewBox', `${-viewport.w / 2} ${-viewport.h / 2} ${viewport.w} ${viewport.h}`);
    		let c = this.boundingBox().corner(0);
    		this.svgGroup.setAttribute("transform",
    			`translate(${t.x} ${t.y}) rotate(${-t.a} 0 0) scale(${t.z} ${t.z}) translate(${c[0]} ${c[1]})`);
    		return true;
    	}

    	prefetch(transform) {
    		if (!this.svgElement)
    			this.createSVGElement();

    		if (!this.visible) return;
    		if (this.status != 'ready')
    			return;

    		const bBox = this.boundingBox();
    		this.svgElement.setAttribute('viewBox', `${bBox.xLow} ${bBox.yLow} ${bBox.xHigh - bBox.xLow} ${bBox.yHigh - bBox.yLow}`);

    		//find which annotations needs to be added to the ccanvas, some 
    		//indexing whould be used, for the moment we just iterate all of them.

    		for (let anno of this.annotations) {

    			//TODO check for class visibility and bbox culling (or maybe should go to prefetch?)
    			if (!anno.ready && typeof anno.svg == 'string') {
    				let parser = new DOMParser();
    				let element = parser.parseFromString(anno.svg, "image/svg+xml").documentElement;
    				anno.elements = [...element.children];
    				anno.ready = true;

    				/*				} else if(this.svgXML) {
    									a.svgElement = this.svgXML.querySelector(`#${a.id}`);
    									if(!a.svgElement)
    										throw Error(`Could not find element with id: ${id} in svg`);
    								} */
    			}

    			if (!anno.needsUpdate)
    				continue;

    			anno.needsUpdate = false;

    			for (let e of this.svgGroup.querySelectorAll(`[data-annotation="${anno.id}"]`))
    				e.remove();

    			if (!anno.visible)
    				continue;

    			//second time will be 0 elements, but we need to 
    			//store somewhere knowledge of which items in the scene and which still not.
    			for (let child of anno.elements) {
    				let c = child; //.cloneNode(true);
    				c.setAttribute('data-annotation', anno.id);
    				c.setAttribute('data-class', anno.class);

    				//c.setAttribute('data-layer', this.id);
    				c.classList.add('openlime-annotation');
    				if (this.selected.has(anno.id))
    					c.classList.add('selected');
    				this.svgGroup.appendChild(c);
    				c.onpointerdown = (e) => {
    					if (e.button == 0) {
    						e.preventDefault();
    						e.stopPropagation();
    						if (this.onClick && this.onClick(anno))
    							return;
    						if (this.selected.has(anno.id))
    							return;
    						this.clearSelected();
    						this.setSelected(anno, true);
    					}
    				};


    				//utils

    				/*				let parser = new DOMParser();
    								let use = createElement('use', { 'xlink:href': '#' + a.id,  'stroke-width': 10,  'pointer-events': 'stroke' });
    								//let use = parser.parseFromString(`<use xlink:href="${a.id}" stroke-width="10" pointer-events="stroke"/>`, "image/svg+xml");
    								this.svgGroup.appendChild(use);  */
    			}
    		}
    	}
    }

    function createElement$1(tag, attributes) {
    	let e = document.createElementNS('http://www.w3.org/2000/svg', tag);
    	if (attributes)
    		for (const [key, value] of Object.entries(attributes))
    			e.setAttribute(key, value);
    	return e;
    }

    Layer.prototype.types['svg_annotations'] = (options) => { return new LayerSvgAnnotation(options); };

    /* FROM: https://stackoverflow.com/questions/40650306/how-to-draw-a-smooth-continuous-line-with-mouse-using-html-canvas-and-javascript */


    function simplify(points, length) {
    	let length2 = Math.pow(length, 2);

        var simplify1 = function(start, end) { // recursize simplifies points from start to end
            var index, i, xx , yy, dx, dy, ddx, ddy,  t, dist, dist1;
            let p1 = points[start];
            let p2 = points[end];   
            xx = p1.x;
            yy = p1.y;
            ddx = p2.x - xx;
            ddy = p2.y - yy;
            dist1 = ddx * ddx + ddy * ddy;
            let maxDist = length2;
            for (var i = start + 1; i < end; i++) {
                let p = points[i];
                if (ddx !== 0 || ddy !== 0) {
                    t = ((p.x - xx) * ddx + (p.y - yy) * ddy) / dist1;
                    if (t > 1) {
                        dx = p.x - p2.x;
                        dy = p.y - p2.y;
                    } else 
                    if (t > 0) {
                        dx = p.x - (xx + ddx * t);
                        dy = p.y - (yy + ddy * t);
                    } else {
                        dx = p.x - xx;
                        dy = p.y - yy;
                    }
                } else {
                    dx = p.x - xx;
                    dy = p.y - yy;
                }
                dist = dx * dx + dy * dy; 
                if (dist > maxDist) {
                    index = i;
                    maxDist = dist;
                }
            }

            if (maxDist > length2) { 
                if (index - start > 1){
                    simplify1(start, index);
                }
                newLine.push(points[index]);
                if (end - index > 1){
                    simplify1(index, end);
                }
            }
        };    
        var end = points.length - 1;
        var newLine = [points[0]];
        simplify1(0, end);
        newLine.push(points[end]);
        return newLine;
    }



    function smooth(points, cornerThres, match) {
    	cornerThres *= 3.1415/180;
    	let newPoints = []; // array for new points

    	if(points.length <= 2)
    		return points.map((p) => [p.x, p.y]);

    	let nx1, ny1, nx2, ny2, dist1, dist2;

    	function dot(x, y, xx, yy) {  // get do product
    		// dist1,dist2,nx1,nx2,ny1,ny2 are the length and  normals and used outside function
    		// normalise both vectors
    		
    		dist1 = Math.sqrt(x * x + y * y); // get length
    		if (dist1  > 0) {  // normalise
    			nx1 = x / dist1 ;
    			ny1 = y / dist1 ;
    		} else {
    			nx1 = 1;  // need to have something so this will do as good as anything
    			ny1 = 0;
    		}
    		dist2  = Math.sqrt(xx * xx + yy * yy);
    		if (dist2  > 0) {
    			nx2 = xx / dist2;
    			ny2 = yy / dist2;
    		} else {
    			nx2 = 1;
    			ny2 = 0;
    		}
    		return Math.acos(nx1 * nx2 + ny1 * ny2 ); // dot product
    	}

    	let p1 = points[0];
    	let endP = points[points.length-1];
    	let i = 0;  // start from second poitn if line not closed
    	let closed = false;
    	let len = Math.hypot(p1.x- endP.x, p1.y-endP.y);
    	
    	if(len < Math.SQRT2){  // end points are the same. Join them in coordinate space
    		endP =  p1;
    		i = 0;			 // start from first point if line closed
    		p1 = points[points.length-2];
    		closed = true;
    	}	   
    	newPoints.push([points[i].x,points[i].y]);
    	for(; i < points.length-1; i++){
    		let p2 = points[i];
    		let p3 = points[i + 1];
    		let angle = Math.abs(dot(p2.x - p1.x, p2.y - p1.y, p3.x - p2.x, p3.y - p2.y));
    		if(dist1 !== 0){  // dist1 and dist2 come from dot function
    			if( angle < cornerThres){ // bend it if angle between lines is small
    				  if(match){
    					  dist1 = Math.min(dist1,dist2);
    					  dist2 = dist1;
    				  }
    				  // use the two normalized vectors along the lines to create the tangent vector
    				  let x = (nx1 + nx2) / 2;  
    				  let y = (ny1 + ny2) / 2;
    				  len = Math.sqrt(x * x + y * y);  // normalise the tangent
    				  if(len === 0){
    					  newPoints.push([p2.x,p2.y]);								  
    				  } else {
    					  x /= len;
    					  y /= len;
    					  if(newPoints.length > 0){
    						  var np = newPoints[newPoints.length-1];
    						  np.push(p2.x-x*dist1*0.25);
    						  np.push(p2.y-y*dist1*0.25);
    					  }
    					  newPoints.push([  // create the new point with the new bezier control points.
    							p2.x,
    							p2.y,
    							p2.x+x*dist2*0.25,
    							p2.y+y*dist2*0.25
    					  ]);
    				  }
    			} else {
    				newPoints.push([p2.x,p2.y]);			
    			}
    		}
    		p1 = p2;
    	}  
    	if(closed){ // if closed then copy first point to last.
    		p1 = [];
    		for(i = 0; i < newPoints[0].length; i++){
    			p1.push(newPoints[0][i]);
    		}
    		newPoints.push(p1);
    	}else {
    		newPoints.push([points[points.length-1].x,points[points.length-1].y]);	  
    	}
    	return newPoints;	
    }

    function smoothToPath(smoothed) {
    	let p = smoothed[0];
    	let d = [`M${p[0].toFixed(1)} ${p[1].toFixed(1)}`];


    	let p1;
    	for(let i = 0; i < smoothed.length-1; i++) {
    		p = smoothed[i];
    		p1 = smoothed[i+1];
    	
    		
    		if(p.length == 2)
    			d.push(`l${(p1[0]-p[0]).toFixed(1)} ${(p1[1]-p[1]).toFixed(1)}`);
    		else if(p.length == 4) 
    			d.push(`q${(p[2]-p[0]).toFixed(1)} ${(p[3]-p[1]).toFixed(1)} ${(p1[0]-p[0]).toFixed(1)} ${(p1[1]-p[1]).toFixed(1)}`);
    		else
    			d.push(`c${(p[2]-p[0]).toFixed(1)} ${(p[3]-p[1]).toFixed(1)} ${(p[4]-p[0]).toFixed(1)} ${(p[5]-p[1]).toFixed(1)} ${(p1[0]-p[0]).toFixed(1)} ${(p1[1]-p[1]).toFixed(1)}`);
    	}
    	return d.join(' ');
    }

    class EditorSvgAnnotation {
    	constructor(lime, layer, options) {
    		this.layer = layer;
    		Object.assign(this, {
    			lime: lime,
    			panning: false,
    			tool: null, //doing nothing, could: ['line', 'polygon', 'point', 'box', 'circle']
    			startPoint: null, //starting point for box and  circle
    			currentLine: [],
    			annotation: null,
    			priority: 20000,
    			classes: {
    				'': { stroke: '#000', label: '' },
    				'class1': { stroke: '#770', label: '' },
    				'class2': { stroke: '#707', label: '' },
    				'class3': { stroke: '#777', label: '' },
    				'class4': { stroke: '#070', label: '' },
    				'class5': { stroke: '#007', label: '' },
    				'class6': { stroke: '#077', label: '' },
    			},
    			tools: {
    				point: {
    					img: '<svg width=24 height=24><circle cx=12 cy=12 r=3 fill="red" stroke="gray"/></svg>',
    					tooltip: 'New point',
    					tool: Point,
    				},
    				pen: {
    					img: '<svg width=24 height=24><circle cx=12 cy=12 r=3 fill="red" stroke="gray"/></svg>',
    					tooltip: 'New polyline',
    					tool: Pen,
    				},
    				line: {
    					img: `<svg width=24 height=24>
						<path d="m 4.7,4.5 c 0.5,4.8 0.8,8.5 3.1,11 2.4,2.6 4.2,-4.8 6.3,-5 2.7,-0.3 5.1,9.3 5.1,9.3" stroke-width="3" fill="none" stroke="grey"/>
						<path d="m 4.7,4.5 c 0.5,4.8 0.8,8.5 3.1,11 2.4,2.6 4.2,-4.8 6.3,-5 2.7,-0.3 5.1,9.3 5.1,9.3" stroke-width="1" fill="none" stroke="red"/></svg>`,
    					tooltip: 'New line',
    					tool: Line,
    				},
    				erase: {
    					img: '',
    					tooltip: 'Erase lines',
    					tool: Erase,
    				},
    				box: {
    					img: '<svg width=24 height=24><rect x=5 y=5 width=14 height=14 fill="red" stroke="gray"/></svg>',
    					tooltip: 'New box',
    					tool: Box,
    				},
    				circle: {
    					img: '<svg width=24 height=24><circle cx=12 cy=12 r=7 fill="red" stroke="gray"/></svg>',
    					tooltip: 'New circle',
    					tool: Circle,
    				},
    				/*				colorpick: {
    									img: '',
    									tooltip: 'Pick a color',
    									tool: Colorpick,
    								} */
    			},
    			annotation: null, //not null only when editWidget is shown.
    			editWidget: null,
    			createCallback: null, //callbacks for backend
    			updateCallback: null,
    			deleteCallback: null
    		}, options);

    		layer.style += Object.entries(this.classes).map((g) => `[data-class=${g[0]}] { stroke:${g[1].stroke}; }`).join('\n');
    		//at the moment is not really possible to unregister the events registered here.
    		lime.pointerManager.onEvent(this);
    		document.addEventListener('keyup', (e) => this.keyUp(e), false);
    		layer.addEvent('selected', (anno) => {
    			if (!anno || anno == this.annotation)
    				return;
    			this.showEditWidget(anno);
    		});

    		layer.annotationsEntry = () => {

    			let entry = {
    				html: `<div class="openlime-tools"></div>`,
    				list: [], //will be filled later.
    				classes: 'openlime-annotations',
    				status: () => 'active',
    				oncreate: () => {
    					if (Array.isArray(layer.annotations))
    						layer.createAnnotationsList();

    					let tools = {
    						'add': { action: () => { this.createAnnotation(); }, title: "New annotation" },
    						'edit': { action: () => { this.toggleEditWidget(); }, title: "Edit annotations" },
    						'export': { action: () => { this.exportAnnotations(); }, title: "Export annotations" },
    						'trash': { action: () => { this.deleteSelected(); }, title: "Delete selected annotations" },
    					};
    					(async () => {

    						for (const [label, tool] of Object.entries(tools)) {
    							let icon = await Skin.appendIcon(entry.element.firstChild, '.openlime-' + label); // TODO pass entry.element.firstChild as parameter in onCreate
    							icon.setAttribute('title', tool.title);
    							icon.addEventListener('click', tool.action);
    						}
    					})();
    				}
    			};
    			layer.annotationsListEntry = entry;
    			return entry;
    		};
    	}

    	createAnnotation() {
    		let anno = this.layer.newAnnotation();
    		anno.publish = 1;
    		anno.label = anno.description = anno.class = '';
    		let post = { id: anno.id, label: anno.label, description: anno.description, 'class': anno.class, svg: null, publish: anno.publish };
    		if (this.createCallback) {
    			let result = this.createCallback(post);
    			if (!result)
    				alert("Failed to create annotation!");
    		}
    		this.showEditWidget(anno);
    		this.layer.setSelected(anno);
    	}


    	toggleEditWidget() {
    		if (this.annotation)
    			return this.hideEditWidget();

    		let id = this.layer.selected.values().next().value;
    		if (!id)
    			return;

    		let anno = this.layer.getAnnotationById(id);
    		this.showEditWidget(anno);
    	}

    	updateEditWidget() {
    		let anno = this.annotation;
    		let edit = this.editWidget;
    		if (!anno.class)
    			anno.class = '';
    		edit.querySelector('[name=label]').value = anno.label || '';
    		edit.querySelector('[name=description]').value = anno.description || '';
    		edit.querySelector('[name=classes]').value = anno.class;
    		edit.querySelector('[name=publish]').checked = anno.publish == 1;
    		edit.classList.remove('hidden');
    		let button = edit.querySelector('.openlime-select-button');
    		button.textContent = this.classes[anno.class].label;
    		button.style.background = this.classes[anno.class].stroke;
    	}

    	showEditWidget(anno) {
    		this.annotation = anno;
    		this.setTool(null);
    		this.setActiveTool();
    		this.layer.annotationsListEntry.element.querySelector('.openlime-edit').classList.add('active');
    		(async () => {
    			await this.createEditWidget();
    			this.updateEditWidget();
    		})();
    	}

    	hideEditWidget() {
    		this.annotation = null;
    		this.setTool(null);
    		this.editWidget.classList.add('hidden');
    		this.layer.annotationsListEntry.element.querySelector('.openlime-edit').classList.remove('active');
    	}


    	//TODO this should actually be in the html.
    	async createEditWidget() {
    		if (this.editWidget)
    			return;

    		let html = `
				<div class="openlime-annotation-edit">
					<span>Title:</span> <input name="label" type="text">
					<span>Description:</span> <input name="description" type="text">
					

					<span>Class:</span> 
					<div class="openlime-select">
						<input type="hidden" name="classes" value=""/>
						<div class="openlime-select-button"></div>
						<ul class="openlime-select-menu">
						${Object.entries(this.classes).map((c) =>
			`<li data-class="${c[0]}" style="background:${c[1].stroke};">${c[1].label}</li>`).join('\n')}
						</ul>
					</div>
					<span><input type="checkbox" name="publish" value=""> Publish</span>
					<div class="openlime-annotation-edit-tools"></div>
				</div>`;
    		let template = document.createElement('template');
    		template.innerHTML = html.trim();
    		let edit = template.content.firstChild;

    		let select = edit.querySelector('.openlime-select');
    		let button = edit.querySelector('.openlime-select-button');
    		let ul = edit.querySelector('ul');
    		let options = edit.querySelectorAll('li');
    		let input = edit.querySelector('[name=classes]');

    		button.addEventListener('click', (e) => {
    			e.stopPropagation();
    			for (let o of options)
    				o.classList.remove('selected');
    			select.classList.toggle('active');

    		});

    		ul.addEventListener('click', (e) => {
    			e.stopPropagation();

    			input.value = e.srcElement.getAttribute('data-class');
    			input.dispatchEvent(new Event('change'));
    			button.style.background = this.classes[input.value].stroke;
    			button.textContent = e.srcElement.textContent;

    			select.classList.toggle('active');
    		});

    		document.addEventListener('click', (e) => {
    			select.classList.remove('active');
    		});

    		document.querySelector('.openlime-layers-menu').appendChild(edit);

    		let tools = edit.querySelector('.openlime-annotation-edit-tools');



    		let draw = await Skin.appendIcon(tools, '.openlime-draw');
    		draw.addEventListener('click', (e) => { this.setTool('line'); this.setActiveTool(draw); });

    		//		let pen = await Skin.appendIcon(tools, '.openlime-pen'); 
    		//		pen.addEventListener('click', (e) => { this.setTool('pen'); setActive(pen); });

    		let erase = await Skin.appendIcon(tools, '.openlime-erase');
    		erase.addEventListener('click', (e) => { this.setTool('erase'); this.setActiveTool(erase); });

    		let undo = await Skin.appendIcon(tools, '.openlime-undo');
    		undo.addEventListener('click', (e) => { this.undo(); });

    		let redo = await Skin.appendIcon(tools, '.openlime-redo');
    		redo.addEventListener('click', (e) => { this.redo(); });

    		/*		let colorpick = await Skin.appendIcon(tools, '.openlime-colorpick'); 
    				undo.addEventListener('click', (e) => { this.pickColor(); }); */

    		let label = edit.querySelector('[name=label]');
    		label.addEventListener('blur', (e) => { if (this.annotation.label != label.value) this.saveCurrent(); this.saveAnnotation(); });

    		let descr = edit.querySelector('[name=description]');
    		descr.addEventListener('blur', (e) => { if (this.annotation.description != descr.value) this.saveCurrent(); this.saveAnnotation(); });

    		let classes = edit.querySelector('[name=classes]');
    		classes.addEventListener('change', (e) => { if (this.annotation.class != classes.value) this.saveCurrent(); this.saveAnnotation(); });

    		let publish = edit.querySelector('[name=publish]');
    		publish.addEventListener('change', (e) => { if (this.annotation.publish != publish.value) this.saveCurrent(); this.saveAnnotation(); });

    		edit.classList.add('hidden');
    		this.editWidget = edit;
    	}


    	saveAnnotation() {
    		let edit = this.editWidget;
    		let anno = this.annotation;

    		anno.label = edit.querySelector('[name=label]').value || '';
    		anno.description = edit.querySelector('[name=description]').value || '';
    		anno.publish = edit.querySelector('[name=publish]').checked ? 1 : 0;
    		let select = edit.querySelector('[name=classes]');
    		anno.class = select.value || '';

    		let button = edit.querySelector('.openlime-select-button');
    		button.style.background = this.classes[anno.class].stroke;

    		for (let e of this.annotation.elements)
    			e.setAttribute('data-class', anno.class);

    		let post = { id: anno.id, label: anno.label, description: anno.description, class: anno.class, publish: anno.publish };
    		//anno.bbox = anno.getBBoxFromElements();
    		let serializer = new XMLSerializer();
    		post.svg = `<svg xmlns="http://www.w3.org/2000/svg">
				${anno.elements.map((s) => { s.classList.remove('selected'); return serializer.serializeToString(s) }).join("\n")}  
				</svg>`;

    		if (this.updateCallback) {
    			let result = this.updateCallback(post);
    			if (!result) {
    				alert("Failed to update annotation");
    				return;
    			}
    		}				//for (let c of element.children)
    		//		a.elements.push(c);

    		//update the entry
    		let template = document.createElement('template');
    		template.innerHTML = this.layer.createAnnotationEntry(anno);
    		let entry = template.content.firstChild;
    		//TODO find a better way to locate the entry!
    		this.layer.annotationsListEntry.element.parentElement.querySelector(`[data-annotation="${anno.id}"]`).replaceWith(entry);
    		this.layer.setSelected(anno);
    	}

    	deleteSelected() {
    		let id = this.layer.selected.values().next().value;
    		if (id)
    			this.deleteAnnotation(id);
    	}

    	deleteAnnotation(id) {
    		let anno = this.layer.getAnnotationById(id);
    		if (this.deleteCallback) {
    			if (!confirm(`Deleting annotation ${anno.label}, are you sure?`))
    				return;
    			let result = this.deleteCallback(anno);
    			if (!result) {
    				alert("Failed to delete this annotation.");
    				return;
    			}
    		}
    		//remove svg elements from the canvas
    		this.layer.svgGroup.querySelectorAll(`[data-annotation="${anno.id}"]`).forEach(e => e.remove());

    		//remove entry from the list
    		let list = this.layer.annotationsListEntry.element.parentElement.querySelector('.openlime-list');
    		list.querySelectorAll(`[data-annotation="${anno.id}"]`).forEach(e => e.remove());

    		this.layer.annotations = this.layer.annotations.filter(a => a !== anno);
    		this.layer.clearSelected();
    		this.hideEditWidget();
    	}

    	exportAnnotations() {
    		let svgElement = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
    		const bBox = this.layer.boundingBox();
    		svgElement.setAttribute('viewBox', `0 0 ${bBox.xHigh-bBox.xLow} ${bBox.yHigh-bBox.yLow}`);
    		let style = createElement('style');
    		style.textContent = this.layer.style;
    		svgElement.appendChild(style);
    		let serializer = new XMLSerializer();
    		//let svg = `<svg xmlns="http://www.w3.org/2000/svg">
    		for (let anno of this.layer.annotations) {
    			for (let e of anno.elements) {
    				if (e.tagName == 'path') {
    					//Inkscape nitpicks on the commas in svg path.
    					let d = e.getAttribute('d');
    					e.setAttribute('d', d.replaceAll(',', ' '));
    				}
    				svgElement.appendChild(e.cloneNode());
    			}
    		}
    		let svg = serializer.serializeToString(svgElement);
    		/*(${this.layer.annotations.map(anno => {
    			return `<group id="${anno.id}" title="${anno.label}" data-description="${anno.description}">
    				${anno.elements.map((s) => { 
    					s.classList.remove('selected'); 
    					return serializer.serializeToString(s) 
    				}).join("\n")}
    				</group>`;
    		})}
    		</svg>`; */

    		///console.log(svg);

    		var e = document.createElement('a');
    		e.setAttribute('href', 'data:text/plain;charset=utf-8,' + encodeURIComponent(svg));
    		e.setAttribute('download', 'annotations.svg');
    		e.style.display = 'none';
    		document.body.appendChild(e);
    		e.click();
    		document.body.removeChild(e);
    	}

    	setActiveTool(e) {
    		if (!this.editWidget) return;
    		let tools = this.editWidget.querySelector('.openlime-annotation-edit-tools');
    		tools.querySelectorAll('svg').forEach(a =>
    			a.classList.remove('active'));
    		if (e)
    			e.classList.add('active');
    	}

    	setTool(tool) {
    		this.tool = tool;
    		if (this.factory && this.factory.quit)
    			this.factory.quit();
    		if (tool) {
    			if (!tool in this.tools)
    				throw "Unknown editor tool: " + tool;

    			this.factory = new this.tools[tool].tool();
    			this.factory.annotation = this.annotation;
    			this.factory.layer = this.layer;
    		}
    		document.querySelector('.openlime-overlay').classList.toggle('erase', tool == 'erase');
    		document.querySelector('.openlime-overlay').classList.toggle('crosshair', tool && tool != 'erase');
    	}

    	// UNDO STUFF	

    	undo() {
    		let anno = this.annotation; //current annotation.
    		if (!anno)
    			return;
    		if (this.factory && this.factory.undo && this.factory.undo()) {
    			anno.needsUpdate = true;
    			this.lime.redraw();
    			return;
    		}

    		if (anno.history && anno.history.length) {
    			//TODO history will be more complicated if it has to manage multiple tools.
    			anno.future.push(this.annoToData(anno));

    			let data = anno.history.pop();
    			this.dataToAnno(data, anno);

    			anno.needsUpdate = true;
    			this.lime.redraw();
    			this.updateEditWidget();
    		}
    	}

    	redo() {
    		let anno = this.annotation; //current annotation.
    		if (!anno)
    			return;
    		if (this.factory && this.factory.redo && this.factory.redo()) {
    			anno.needsUpdate = true;
    			this.lime.redraw();
    			return;
    		}
    		if (anno.future && anno.future.length) {
    			anno.history.push(this.annoToData(anno));

    			let data = anno.future.pop();
    			this.dataToAnno(data, anno);

    			anno.needsUpdate = true;
    			this.lime.redraw();
    			this.updateEditWidget();
    		}
    	}

    	saveCurrent() {
    		console.log('save current');
    		let anno = this.annotation; //current annotation.
    		if (!anno.history)
    			anno.history = [];

    		anno.history.push(this.annoToData(anno));
    		anno.future = [];
    	}

    	annoToData(anno) {
    		let data = {};
    		for (let i of ['id', 'label', 'description', 'class', 'publish'])
    			data[i] = `${anno[i] || ''}`;
    		data.elements = anno.elements.map(e => { let n = e.cloneNode(); n.points = e.points; return n; });
    		return data;
    	}

    	dataToAnno(data, anno) {
    		for (let i of ['id', 'label', 'description', 'class', 'publish'])
    			anno[i] = `${data[i]}`;
    		anno.elements = data.elements.map(e => { let n = e.cloneNode(); n.points = e.points; return n; });
    	}

    	// TOOLS STUFF

    	keyUp(e) {
    		if (e.defaultPrevented) return;
    		switch (e.key) {
    			case 'Escape':
    				if (this.tool) {
    					this.setActiveTool();
    					this.setTool(null);
    					e.preventDefault();
    				}
    				break;
    			case 'Delete':
    				this.deleteSelected();
    				break;
    			case 'Backspace':
    				break;
    			case 'z':
    				if (e.ctrlKey)
    					this.undo();
    				break;
    			case 'Z':
    				if (e.ctrlKey)
    					this.redo();
    				break;
    		}
    	}

    	panStart(e) {
    		if (e.buttons != 1 || e.ctrlKey || e.altKey || e.shiftKey || e.metaKey)
    			return;
    		if (!['line', 'erase', 'box', 'circle'].includes(this.tool))
    			return;
    		this.panning = true;
    		e.preventDefault();

    		this.saveCurrent();

    		const pos = this.mapToSvg(e);
    		this.factory.create(pos, e);

    		this.annotation.needsUpdate = true;

    		this.lime.redraw();
    	}

    	panMove(e) {
    		if (!this.panning)
    			return false;

    		const pos = this.mapToSvg(e);
    		this.factory.adjust(pos, e);
    	}

    	panEnd(e) {
    		if (!this.panning)
    			return false;
    		this.panning = false;

    		const pos = this.mapToSvg(e);
    		let changed = this.factory.finish(pos, e);
    		if (!changed) //nothing changed no need to keep current situation in history.
    			this.annotation.history.pop();
    		else
    			this.saveAnnotation();
    		this.annotation.needsUpdate = true;
    		this.lime.redraw();
    	}

    	fingerHover(e) {
    		if (this.tool != 'line')
    			return;
    		e.preventDefault();
    		const pos = this.mapToSvg(e);
    		this.factory.hover(pos, e);
    		this.annotation.needsUpdate = true;
    		this.lime.redraw();
    	}

    	fingerSingleTap(e) {
    		if (!['point', 'line', 'erase'].includes(this.tool))
    			return;
    		e.preventDefault();

    		this.saveCurrent();

    		const pos = this.mapToSvg(e);
    		let changed = this.factory.tap(pos, e);
    		if (!changed) //nothing changed no need to keep current situation in history.
    			this.annotation.history.pop();
    		else
    			this.saveAnnotation();
    		this.annotation.needsUpdate = true;

    		this.lime.redraw();
    	}

    	fingerDoubleTap(e) {
    		if (!['line'].includes(this.tool))
    			return;
    		e.preventDefault();

    		this.saveCurrent();

    		const pos = this.mapToSvg(e);
    		let changed = this.factory.doubleTap(pos, e);
    		if (!changed) //nothing changed no need to keep current situation in history.
    			this.annotation.history.pop();
    		else
    			this.saveAnnotation();
    		this.annotation.needsUpdate = true;

    		this.lime.redraw();
    	}


    	mapToSvg(e) {
    		let camera = this.lime.camera;
    		let transform = camera.getCurrentTransform(performance.now());
    		let pos = camera.mapToScene(e.offsetX, e.offsetY, transform);
    		const topLeft = this.layer.boundingBox().corner(0);
    		pos.x -= topLeft[0]; 
    		pos.y -= topLeft[1];
    		pos.z = transform.z;
    		return pos;
    	}
    }

    class Point {
    	tap(pos) {
    		//const pos = this.mapToSvg(e);
    		let point = createElement('circle', { cx: pos.x, cy: pos.y, r: 10, class: 'point' });
    		//point.classList.add('selected');
    		return point;
    	}
    }

    class Pen {
    	constructor() {
    		//TODO Use this.path.points as in line, instead.
    		this.points = [];
    	}
    	create(pos) {
    		this.points.push(pos);
    		if (this.points.length == 1) {
    			saveCurrent;

    			this.path = createElement('path', { d: `M${pos.x} ${pos.y}`, class: 'line' });
    			return this.path;
    		}
    		let p = this.path.getAttribute('d');
    		this.path.setAttribute('d', p + ` L${pos.x} ${pos.y}`);
    		this.path.points = this.points;
    	}
    	undo() {
    		if (!this.points.length)
    			return;
    		this.points.pop();
    		let d = this.points.map((p, i) => `${i == 0 ? 'M' : 'L'}${p.x} ${p.y}`).join(' ');
    		this.path.setAttribute('d', d);

    		if (this.points.length < 2) {
    			this.points = [];
    			this.annotation.elements = this.annotation.elements.filter((e) => e != this.path);
    		}
    	}
    }


    class Box {
    	constructor() {
    		this.origin = null;
    		this.box = null;
    	}

    	create(pos) {
    		this.origin = pos;
    		this.box = createElement('rect', { x: pos.x, y: pos.y, width: 0, height: 0, class: 'rect' });
    		return this.box;
    	}

    	adjust(pos) {
    		let p = this.origin;

    		this.box.setAttribute('x', Math.min(p.x, pos.x));
    		this.box.setAttribute('width', Math.abs(pos.x - p.x));
    		this.box.setAttribute('y', Math.min(p.y, pos.y));
    		this.box.setAttribute('height', Math.abs(pos.y - p.y));
    	}

    	finish(pos) {
    		return this.box;
    	}
    }

    class Circle {
    	constructor() {
    		this.origin = null;
    		this.circle = null;
    	}
    	create(pos) {
    		this.origin = pos;
    		this.circle = createElement('circle', { cx: pos.x, cy: pos.y, r: 0, class: 'circle' });
    		return this.circle;
    	}
    	adjust(pos) {
    		let p = this.origin;
    		let r = Math.hypot(pos.x - p.x, pos.y - p.y);
    		this.circle.setAttribute('r', r);
    	}
    	finish() {
    		return this.circle;
    	}
    }

    class Line {

    	constructor() {
    		this.history = [];
    	}
    	create(pos) {
    		/*if(this.segment) {
    			this.layer.svgGroup.removeChild(this.segment);
    			this.segment = null;
    		}*/
    		for (let e of this.annotation.elements) {
    			if (!e.points || e.points.length < 2)
    				continue;
    			if (Line.distance(e.points[0], pos) * pos.z < 5) {
    				e.points.reverse();
    				this.path = e;
    				this.path.setAttribute('d', Line.svgPath(e.points));
    				//reverse points!
    				this.history = [this.path.points.length];
    				return;
    			}
    			if (Line.distanceToLast(e.points, pos) < 5) {
    				this.path = e;
    				this.adjust(pos);
    				this.history = [this.path.points.length];
    				return;
    			}
    		}
    		this.path = createElement('path', { d: `M${pos.x} ${pos.y}`, class: 'line' });
    		this.path.points = [pos];
    		this.history = [this.path.points.length];
    		this.annotation.elements.push(this.path);
    	}

    	tap(pos) {
    		if (!this.path) {
    			this.create(pos);
    			return false;
    		} else {
    			if (this.adjust(pos))
    				this.history = [this.path.points.length - 1];
    			return true;
    		}
    	}
    	doubleTap(pos) {
    		if (!this.path)
    			return false;
    		if (this.adjust(pos)) {
    			this.history = [this.path.points.length - 1];
    			this.path = null;
    		}
    		return false;
    	}

    	hover(pos, event) {
    		return;
    	}
    	quit() {
    		return;
    	}

    	adjust(pos) {
    		let gap = Line.distanceToLast(this.path.points, pos);
    		if (gap * pos.z < 4) return false;

    		this.path.points.push(pos);

    		this.path.getAttribute('d');
    		this.path.setAttribute('d', Line.svgPath(this.path.points));//d + `L${pos.x} ${pos.y}`);
    		return true;
    	}

    	finish() {
    		this.path.setAttribute('d', Line.svgPath(this.path.points));
    		return true; //some changes where made!
    	}

    	undo() {
    		if (!this.path || !this.history.length)
    			return false;
    		this.path.points = this.path.points.slice(0, this.history.pop());
    		this.path.setAttribute('d', Line.svgPath(this.path.points));
    		return true;
    	}
    	redo() {
    		return false;
    	}
    	//TODO: smooth should be STABLE, if possible.
    	static svgPath(points) {
    		//return points.map((p, i) =>  `${(i == 0? "M" : "L")}${p.x} ${p.y}`).join(' '); 

    		let tolerance = 1.5 / points[0].z;
    		let tmp = simplify(points, tolerance);

    		let smoothed = smooth(tmp, 90, true);
    		return smoothToPath(smoothed);
    		
    	}
    	static distanceToLast(line, point) {
    		let last = line[line.length - 1];
    		return Line.distance(last, point);
    	}
    	static distance(a, b) {
    		let dx = a.x - b.x;
    		let dy = a.y - b.y;
    		return Math.sqrt(dx * dx + dy * dy);
    	}
    }

    class Erase {
    	create(pos, event) { this.erased = false; this.erase(pos, event); }
    	adjust(pos, event) { this.erase(pos, event); }
    	finish(pos, event) { return this.erase(pos, event); } //true if some points where removed.
    	tap(pos, event) { return this.erase(pos, event); }
    	erase(pos, event) {
    		for (let e of this.annotation.elements) {
    			if (e == event.originSrc) {
    				e.points = [];
    				this.erased = true;
    				continue;
    			}

    			let points = e.points;
    			if (!points || !points.length)
    				continue;

    			if (Line.distanceToLast(points, pos) < 10)
    				this.erased = true, points.pop();
    			else if (Line.distance(points[0], pos) < 10)
    				this.erased = true, points.shift();
    			else
    				continue;

    			if (points.length <= 2) {
    				e.points = [];
    				e.setAttribute('d', '');
    				this.annotation.needsUpdate = true;
    				this.erased = true;
    				continue;
    			}

    			e.setAttribute('d', Line.svgPath(points));
    		}
    		this.annotation.elements = this.annotation.elements.filter(e => { return !e.points || e.points.length > 2; });
    		return this.erased;
    	}
    }

    //utils
    function createElement(tag, attributes) {
    	let e = document.createElementNS('http://www.w3.org/2000/svg', tag);
    	if (attributes)
    		for (const [key, value] of Object.entries(attributes))
    			e.setAttribute(key, value);
    	return e;
    }

    exports.Camera = Camera;
    exports.Canvas = Canvas;
    exports.Controller = Controller;
    exports.Controller2D = Controller2D;
    exports.ControllerFocusContext = ControllerFocusContext;
    exports.ControllerLens = ControllerLens;
    exports.ControllerPanZoom = ControllerPanZoom;
    exports.EditorSvgAnnotation = EditorSvgAnnotation;
    exports.Layer = Layer;
    exports.LayerAnnotation = LayerAnnotation;
    exports.LayerBRDF = LayerBRDF;
    exports.LayerCombiner = LayerCombiner;
    exports.LayerImage = LayerImage;
    exports.LayerLens = LayerLens;
    exports.LayerRTI = LayerRTI;
    exports.LayerSvgAnnotation = LayerSvgAnnotation;
    exports.Layout = Layout;
    exports.PointerManager = PointerManager;
    exports.Raster = Raster;
    exports.Shader = Shader;
    exports.ShaderCombiner = ShaderCombiner;
    exports.Skin = Skin;
    exports.Transform = Transform;
    exports.UIBasic = UIBasic;
    exports.UIDialog = UIDialog;
    exports.Viewer = Viewer;
    exports.matMul = matMul;

    Object.defineProperty(exports, '__esModule', { value: true });

}));
