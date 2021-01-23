function formatMm(val) {
	if(val < 20)
		return val.toFixed(1) + " mm";
	if(val < 500)
		return (val/10).toFixed(1) + " cm";
	if(val < 100000)
		return (val/1000).toFixed(1) + " m";
	else
		return (val/1000000).toFixed(2) + " km";
}
function RelightViewer(div, options) {

	var t = this;
	if(typeof(div) == "string")
		div = document.querySelector(div);
	t.div = div;

	t.nav = {
		action:null, lighting: true, fullscreen:false, 
		pandelay: 50, zoomdelay:200, zoomstep: 0.25, lightsize:0.8,
		pointers: {}, 
		support: support,
//		pagemap: { size: 200, autohide: 1000 },
		pagemap: false,
		normals: 0,
		actions: {
			home:    { title: 'Home',       task: function(event) { t.centerAndScale(t.nav.zoomdelay); }        },
			zoomin:  { title: 'Zoom In',    task: function(event) { t.zoom(-t.nav.zoomstep, t.nav.zoomdelay); } },
			zoomout: { title: 'Zoom Out',   task: function(event) { t.zoom(+t.nav.zoomstep, t.nav.zoomdelay); } },
			rotate:  { title: 'Rotate',     task: function(event) { t.rotate(t.nav.zoomstep, 45); } },
			light:   { title: 'Light',      task: function(event) { t.toggleLight(event); }                     },
			normals: { title: 'Normals',     task: function(event) { t.toggleNormals(event); } },
			full:    { title: 'Fullscreen', task: function(event) { t.toggleFullscreen(event); }                },
			info:    { title: 'info',       task: function(event) { t.showInfo(); }                             }
		},
		scale: 0                     //size of a pixel in mm.
	};

	if(options.hasOwnProperty('notool'))
		for(var i = 0; i < options.notool.length; i++)
			delete t.nav.actions[options.notool[i]];


	for(var i in t.nav)
		if(options.hasOwnProperty(i))
			t.nav[i] = options[i];

	var info = document.querySelector(".relight-info-content");
	if(info)
		info.remove();
	else
		delete t.nav.actions['info'];

	var html = 
'	<canvas></canvas>\n' +
'	<div class="relight-toolbox">\n';
	for(var i in t.nav.actions) {
		var action = t.nav.actions[i];
		if(i == 'light' && t.nav.lighting)
			i += ' relight-light_on';
		if(i == 'normal' && t.nav.normals)
			i += ' relight-normals_on';
		html += '		<div class="relight-' + i + '" title="' + action.title + '"></div>\n';
	}

	html += '	</div>\n';
	if(t.nav.scale)
		html += '	<div class="relight-scale"><hr/><span></span></div>\n';

	if(t.nav.pagemap) {
		html += '	<div';
		if(t.nav.pagemap.thumb)
			html += ' style="background-image:url(' + options.url + '/' + t.nav.pagemap.thumb + '); background-size:cover"';
		html += ' class="relight-pagemap"><div class="relight-pagemap-area"></div></div>\n';
	}

	html += '	<div class="relight-info-dialog"></div>\n';




	div.innerHTML = html;

	t.dialog = div.querySelector(".relight-info-dialog");

	if(info) {
		t.dialog.appendChild(info);
		info.style.display = 'block';
		t.addAction(div, ".relight-info-dialog", function() { t.hideInfo(); });
	}

	if(t.nav.pagemap) {
		t.nav.pagemap.div  = div.querySelector(".relight-pagemap");
		t.nav.pagemap.area = div.querySelector(".relight-pagemap-area");
	}

	for(var i in t.nav.actions)
		t.addAction(div, '.relight-' + i, t.nav.actions[i].task);

	var canvas = div.querySelector('canvas');

	RelightCanvas.call(this, canvas, options);

	var support = "onwheel" in document.createElement("div") ? "wheel" : // Modern browsers 
		document.onmousewheel !== undefined ? "mousewheel" : // Webkit and IE support at least "mousewheel"
		"DOMMouseScroll"; // older Firefox
	t.canvas.addEventListener(support,   function(e) { t.mousewheel(e); },   false);
	window.addEventListener('resize', function(e) { t.resize(canvas.offsetWidth, canvas.offsetHeight); if(options.scale) t.updateScale(); t.updatePagemap(); });
	t.canvas.addEventListener('contextmenu', function(e) { e.preventDefault(); return false; });


	var mc = new Hammer.Manager(t.canvas);
	mc.add( new Hammer.Pan({ pointers:1, direction: Hammer.DIRECTION_ALL, threshold: 0 }) );
	mc.on('panstart',         function(ev) { t.mousedown(ev); });
	mc.on('panmove',          function(ev) { t.mousemove(ev); });
	mc.on('panend pancancel', function(ev) { t.mouseup(ev);   });

	mc.add( new Hammer.Pinch() );
	mc.on('pinchstart',           function(ev) { t.mousedown(ev); });
	mc.on('pinchmove',            function(ev) { t.mousemove(ev); });
	mc.on('pinchend pinchcancel', function(ev) { t.mouseup(ev); });

	mc.add( new Hammer.Tap({ taps:2 }) );
	mc.on('tap', function(ev) { t.zoom(-2*t.nav.zoomstep, t.nav.zoomdelay); });

	t.resize(canvas.offsetWidth, canvas.offsetHeight);
	if(options.scale)
		t.onPosChange(function() { t.updateScale(); });
	//remember size (add exif into json!)
	if(t.nav.pagemap) {
		t.onReady(function() { t.initPagemap(); });
		t.onPosChange(function() { t.updatePagemap(); });
	}
}


RelightViewer.prototype = RelightCanvas.prototype;

RelightViewer.prototype.addAction = function(div, selector, action) {
	var tap = new Hammer.Manager(div.querySelector(selector));
	tap.add(new Hammer.Tap());
	tap.on('tap', action);
};

RelightViewer.prototype.toggleLight = function(event) {
	var t = this;
	if(t.nav.lighting)
		event.target.classList.remove('relight-light_on');
	else
		event.target.classList.add('relight-light_on');

	t.nav.lighting = !t.nav.lighting; 
};

RelightViewer.prototype.toggleNormals = function(event) {
	var t = this;

	t.nav.normals = (t.nav.normals + 1)%3;
	t.setNormals(t.nav.normals);

	if(!t.nav.normals)
		event.target.classList.remove('relight-normals_on');
	else
		event.target.classList.add('relight-normals_on');
};

RelightViewer.prototype.toggleFullscreen = function(event) {
	var t = this;
	var div = t.div;
	if(t.nav.fullscreen) {
		var request = document.exitFullscreen || document.webkitExitFullscreen ||
			document.mozCancelFullScreen || document.msExitFullscreen;
		request.call(document);
		event.target.classList.remove('relight-full_on');

		div.style.height = "100%";
		t.resize(t.canvas.offsetWidth, t.canvas.offsetHeight);
	} else {
		var request = div.requestFullscreen || div.webkitRequestFullscreen ||
			div.mozRequestFullScreen || div.msRequestFullscreen;
		request.call(div);
		event.target.classList.add('relight-full_on');
	}
	div.style.height = window.offsetHeight + "px";
	t.resize(t.canvas.offsetWidth, t.canvas.offsetHeight);

	t.nav.fullscreen = !t.nav.fullscreen; 
};

RelightViewer.prototype.updateScale = function() {
	var t = this;
	var span = t.div.querySelector('.relight-scale > span');
	var hr = t.div.querySelector('.relight-scale > hr');
	var scale = Math.pow(2, t.pos.z);
	var scalesize = t.options.scale*hr.offsetWidth*scale; //size of a pixel
	span.innerHTML = formatMm(scalesize); 

	var box = t.div.querySelector('.relight-scale');
	box.style.opacity = 1.0;

	if(t.nav.scaletimeout)
		clearTimeout(t.nav.scaletimeout);
	t.nav.scaletimeout = setTimeout(
		function() {
			t.nav.scaletimeout = null;
			box.style.opacity = 0.1;
		}, 
		1000);
};
	
RelightViewer.prototype.initPagemap = function() {
	var t = this;
	var page = t.nav.pagemap;
	var size = page.size || page.div.offsetWidth;
	var w = t.width;
	var h = t.height;
	if(w > h) {
		page.w = size;
		page.h = size * h/w;
	} else {
		page.w = size * w/h
		page.h = size;
	}
	page.div.style.width =  page.w+  'px';
	page.div.style.height = page.h + 'px';
	
	page.area.style.width =  page.w/2+  'px';
	page.area.style.height = page.h/2 + 'px';
	t.updatePagemap();
}

RelightViewer.prototype.updatePagemap = function() {
	var t = this;
	var page = t.nav.pagemap;
	var a = page.area;
	if(!page.w) return;

	var w = t.canvas.width;
	var h = t.canvas.height;

	var box = t.boundingBox();
	var offset = [(box[0] + box[2])/2, (box[1] + box[3])/2];
	var scale = Math.pow(2, t.pos.z);

	var center = [-offset[0]/scale/t.canvas.width  + 0.5, -offset[1]/scale/t.canvas.height  + 0.5];
	var width  = t.canvas.width*scale /t.width;
	var height = t.canvas.height*scale/t.height;
	var bbox = [
		Math.max(0,      parseInt(page.w*(center[0] - width /2))),
		Math.max(0,      parseInt(page.h*(center[1] - height/2))),
		Math.min(page.w, parseInt(page.w*(center[0] + width /2))),
		Math.min(page.h, parseInt(page.h*(center[1] + height/2)))
	];

	page.area.style.left = bbox[0] + 'px';
	page.area.style.top =  bbox[1] + 'px';
	page.area.style.width = (bbox[2] - bbox[0]) + 'px';
	page.area.style.height = (bbox[3] - bbox[1]) + 'px';

	page.div.style.opacity = 1.0;

	if(page.autohide) {

		if(page.timeout)
			clearTimeout(page.timeout);
		page.timeout = setTimeout(
			function() {
				page.timeout = null;
				page.div.style.opacity = 0.1;
			}, 
			page.autohide
		);
	}
}


RelightViewer.prototype.mousedown = function(event) {
	var t = this;

	var src = event.srcEvent
	//src.buttons is a mask 1 -> left, 2 -> right, 4 -> center
	if(event.type == 'pinchstart') {
		t.nav.action = 'zoom';
	} else if(!t.nav.lighting || src.shiftKey || src.ctrlKey || (src.buttons & 0x2)) {
		t.nav.action = 'pan';
	} else {
		t.nav.action = 'light';
		t.lightDirection(event);
	}

	//save initial position.
	t.nav.pos = this.pos;
	t.nav.light = this.light;
};

RelightViewer.prototype.lightDirection = function(event) {
	var t = this;
	var e = event.srcEvent;
	var w = t.nav.lightsize*t.canvas.width/2;
	var h = t.nav.lightsize*t.canvas.height/2;

	var x = (e.offsetX - t.canvas.width/2)/w;
	var y = (e.offsetY - t.canvas.height/2)/h;

	var r = Math.sqrt(x*x + y*y);
	if(r > 1.0) {
		x /= r;
		y /= r;
		r = 1.0;
	}
	var z = Math.sqrt(1 - r);
	t.setLight(x, -y, z);
}

RelightViewer.prototype.mousemove = function(event) {
	var t = this;
	if(!t.nav.action) return;

	var p = t.nav.pos;
	var x = event.deltaX;
	var y = event.deltaY;
	var scale = Math.pow(2, p.z);

	switch(t.nav.action) {
	case 'pan':
		t.setPosition(t.nav.pandelay, p.x - x*scale, p.y - y*scale, p.z, p.a);
		break;

	case 'zoom':
		z = p.z - Math.log(event.scale)/Math.log(2)
		t.setPosition(t.nav.zoomdelay, p.x, p.y, z, p.a);
		break;

	case 'light':
		t.lightDirection(event);
		break;
	}
};

RelightViewer.prototype.mouseup = function(event) {
	if(this.nav.action) {
		this.nav.action = null;
		event.preventDefault();
	}
};

RelightViewer.prototype.mousewheel = function(event) {
	if ( this.nav.support == "mousewheel" ) {
		event.deltaY = - 1/40 * event.wheelDelta;
	} else {
		event.deltaY = event.deltaY || event.detail;
	}

	var dz = event.deltaY > 0? this.nav.zoomstep : -this.nav.zoomstep;
	this.zoom(-dz, this.nav.zoomdelay);
	event.preventDefault();
};


RelightViewer.prototype.setInfo = function(info) {
	if(typeof(info) == "string")
		this.dialog.innerHTML = info;
	else
		this.dialog.append(info);
};

RelightViewer.prototype.showInfo = function() {
	this.dialog.style.display = 'block';
};

RelightViewer.prototype.hideInfo = function() {
	this.dialog.style.display = 'none';
};


