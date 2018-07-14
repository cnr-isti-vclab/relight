
function RelightViewer(div, options) {

	var t = this;
	if(typeof(div) == "string")
		div = document.querySelector(div);

	div.innerHTML = 
'	<canvas id="rticanvas"></canvas>\n' +
'	<div class="relight-toolbox">\n' +
'		<div class="relight-home" title="Home"></div>\n' +
'		<div class="relight-zoomin" title="Zoom In"></div>\n' +
'		<div class="relight-zoomout" title="Zoom Out"></div>\n' +
'		<div class="relight-light" title="Light On"></div>\n' +
'		<div class="relight-full" title="Fullscreen"></div>\n' +
'		<div class="relight-help" title="Help"></div>\n' +
'	</div>\n';
'	<div class="relight-help-dialog"></div>\n';

	var canvas = document.querySelector('#rticanvas');

	Relight.call(this, canvas, options);

	var support = "onwheel" in document.createElement("div") ? "wheel" : // Modern browsers 
		document.onmousewheel !== undefined ? "mousewheel" : // Webkit and IE support at least "mousewheel"
		"DOMMouseScroll"; // older Firefox
	t.canvas.addEventListener(support,   function(e) { t.mousewheel(e); },   false);
	window.addEventListener('resize', function(e) { t.resize(div.offsetWidth, div.offsetHeight); });

	var mc = new Hammer.Manager(t.canvas);
	mc.add( new Hammer.Pan({ direction: Hammer.DIRECTION_ALL, threshold: 0 }) );
	mc.on('panstart',         function(ev) { t.mousedown(ev); });
	mc.on('panmove',          function(ev) { t.mousemove(ev); });
	mc.on('panend pancancel', function(ev) { t.mouseup(ev); });

	mc.add( new Hammer.Pinch() );
	mc.on('pinchstart',           function(ev) { t.mousedown(ev); });
	mc.on('pinchmove',            function(ev) { t.mousemove(ev); });
	mc.on('pinchend pinchcancel', function(ev) { t.mouseup(ev); });

	mc.add( new Hammer.Tap({taps:2}) );
	mc.on('tap', function(ev) { t.zoom(-2*t.nav.zoomstep, t.nav.zoomdelay); });

	t.nav = { action:null, lighting: false, pandelay: 50, zoomdelay:200, zoomstep: 0.25, lightsize:200, pointers: {}, support: support, fullscreen:false };

	for(var i in t.nav)
		if(t.options[i])
			t.nav[i] = t.options[i];


	this.addAction('.relight-home',        function(event) { t.centerAndScale(t.nav.zoomdelay); });

	this.addAction('.relight-zoomin',      function(event) { t.zoom(-t.nav.zoomstep, t.nav.zoomdelay); });
	this.addAction('.relight-zoomout',     function(event) { t.zoom(+t.nav.zoomstep, t.nav.zoomdelay); });
	this.addAction('.relight-light',       function(event) {
		if(t.nav.lighting)
			event.target.classList.remove('relight-light_on');
		else
			event.target.classList.add('relight-light_on');

		t.nav.lighting = !t.nav.lighting; 
	});
	this.addAction('.relight-full', function(event) {
		if(t.nav.fullscreen) {
			var request = document.exitFullscreen || document.webkitExitFullscreen ||
				document.mozCancelFullScreen || document.msExitFullscreen;
			request.call(document);
			event.target.classList.remove('relight-full_on');

			div.style.height = "100%";
			t.resize(div.offsetWidth, div.offsetHeight);
		} else {
			var request = div.requestFullscreen || div.webkitRequestFullscreen ||
				div.mozRequestFullScreen || div.msRequestFullscreen;
			request.call(div);
			event.target.classList.add('relight-full_on');
		}
		div.style.height = window.offsetHeight + "px";
		t.resize(div.offsetWidth, div.offsetHeight);

		t.nav.fullscreen = !t.nav.fullscreen; 
	});
	this.addAction('.relight-help', function(event) { t.showHelp(); });

	t.resize(div.offsetWidth, div.offsetHeight);

	//remember size (add exif into json!)
}

RelightViewer.prototype = Relight.prototype;

RelightViewer.prototype.addAction = function(selector, action) {
	var tap = new Hammer.Manager(document.querySelector(selector));
	tap.add(new Hammer.Tap());
	tap.on('tap', action);
}

RelightViewer.prototype.mousedown = function(event) {
	var t = this;
	var src = event.srcEvent
	if(event.type == 'pinchstart')
		t.nav.action = 'zoom';
	else if(t.nav.lighting || src.shiftKey || src.ctrlKey || src.button > 0)
		t.nav.action = 'light';
	else
		t.nav.action = 'pan';

	t.nav.pos = this.pos;
	t.nav.light = this.light;
}

RelightViewer.prototype.mousemove = function(event) {
	var t = this;
	if(!t.nav.action) return;

	var p = t.nav.pos;
	var x = event.deltaX;
	var y = event.deltaY;

	switch(t.nav.action) {
	case 'pan':
		var scale = Math.pow(2, p.z);
		x *= scale;
		y *= scale;
		t.setPosition(p.x - x, p.y - y, p.z, t.nav.pandelay);
		break;

	case 'zoom':
		var z = p.z/Math.pow(event.scale, 1.5);
		t.setPosition(p.x, p.y, z, t.nav.zoomdelay);
		break;

	case 'light':
		var dx = x/t.nav.lightsize;
		var dy = y/t.nav.lightsize;
		var x = t.nav.light[0] + dx;
		var y = t.nav.light[1] + dy;

		var r = Math.sqrt(x*x + y*y);
		if(r > 1.0) {
			x /= r;
			y /= r;
			r = 1.0;
		}
		var z = Math.sqrt(1 - r);

		t.setLight(-x, y, z);
		break;
	}
}

RelightViewer.prototype.mouseup = function(event) {
	if(this.nav.action) {
		this.nav.action = null;
		event.preventDefault();
	}
}

RelightViewer.prototype.mousewheel = function(event) {
	if ( this.nav.support == "mousewheel" ) {
		event.deltaY = - 1/40 * event.wheelDelta;
	} else {
		event.deltaY = event.deltaY || event.detail;
	}

	var dz = event.deltaY > 0? this.nav.zoomstep : -this.nav.zoomstep;
	this.zoom(-dz, this.nav.zoomdelay);
	event.preventDefault();
}
