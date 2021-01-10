Relight.prototype.headFrag = function() {
	var t = this;

	var basetype = (t.colorspace == 'mrgb' || t.colorspace == 'mycc')?'vec3':'float';

	var str = 
`
#ifdef GL_ES
precision highp float;
#endif

const int np1 = ${t.nplanes + 1};
const int nj = ${t.njpegs};
`;

	if(t.colorspace == 'mycc')
		str +=
`
const int ny0 = ${t.yccplanes[0]};
const int ny1 = ${t.yccplanes[1]};
`

	if(t.normals) {

		if(t.normals == 2)
			str += 'uniform vec3 light;\n';

		str += 
`
const mat3 T = mat3(8.1650e-01, 4.7140e-01, 4.7140e-01,
	-8.1650e-01, 4.7140e-01,  4.7140e-01,
	-1.6222e-08, -9.4281e-01, 4.7140e-01);
`;
		if(t.colorspace == 'lrgb')
			str += `
			uniform ${basetype} base0[np1];
			uniform ${basetype} base1[np1];
			uniform ${basetype} base2[np1];`;
		else
			str += `
			uniform ${basetype} base0[nj];
			uniform ${basetype} base1[nj];
			uniform ${basetype} base2[nj];`;

	}

	str +=
`
uniform ${basetype} base[np1];
uniform float bias[np1];
uniform float scale[np1];
uniform float opacity;

uniform sampler2D planes[nj];

varying vec2 v_texcoord;
`;

	return str;
}

Relight.prototype.fragTail = function() {
	var str = '';
	switch(this.normals) {
	case 1: str = '	color = (normalize(T * color) + 1.0)/2.0;\n'; break;
	case 2: str = 
`	color = normalize(T * color);
	float c = dot(light, color);
	color = vec3(c, c, c);
`;
	break;
	}

	str +=
`	gl_FragColor = vec4(color, opacity);
}`;

	return str;
}


/*    MRGB    */

Relight.prototype.mrgbFrag = function() {
	let t = this;

	let src = t.headFrag();

/* Unrolling the loop to investigate a glsl probable bug */
	if(!t.normals) {
		src +=

`void main(void) {
	vec3 color = base[0];
	vec4 c;
`;
	for(let j = 0; j < t.njpegs; j++) {
		src +=
`	c = texture2D(planes[${j}], v_texcoord);
	color += base[${j}*3+1]*(c.x - bias[${j}*3+1])*scale[${j}*3+1];
	color += base[${j}*3+2]*(c.y - bias[${j}*3+2])*scale[${j}*3+2];
	color += base[${j}*3+3]*(c.z - bias[${j}*3+3])*scale[${j}*3+3];
`;
	}


	let old_src = 

`void main(void) {
	vec3 color = base[0];

	for(int j = 0; j < nj; j++) {
		vec4 c = texture2D(planes[j], v_texcoord);
		color += base[j*3+1]*(c.x - bias[j*3+1])*scale[j*3+1];
		color += base[j*3+2]*(c.y - bias[j*3+2])*scale[j*3+2];
		color += base[j*3+3]*(c.z - bias[j*3+3])*scale[j*3+3];
	}
`;

	} else {
		src +=
`void main(void) {
	vec3 one = vec3(1.0 ,1.0, 1.0);
	float b = dot(base[0], one);
	vec3 color = vec3(b);

	for(int j = 0; j < 1; j++) {
		vec4 c = texture2D(planes[j], v_texcoord);

		vec3 b0 = vec3(dot(base0[j*3+1],one), dot(base0[j*3+2],one), dot(base0[j*3+3],one));
		vec3 b1 = vec3(dot(base1[j*3+1],one), dot(base1[j*3+2],one), dot(base1[j*3+3],one));
		vec3 b2 = vec3(dot(base2[j*3+1],one), dot(base2[j*3+2],one), dot(base2[j*3+3],one));

		vec3 r = vec3(
			(c.x - bias[j*3+1])*scale[j*3+1],
			(c.y - bias[j*3+2])*scale[j*3+2],
			(c.z - bias[j*3+3])*scale[j*3+3]);

		color.x += dot(b0, r);
		color.y += dot(b1, r);
		color.z += dot(b2, r);
	}
`;
	}

	src += t.fragTail();
	return src;
}



/*  MYCC */

Relight.prototype.myccFrag = function() {
	var t = this;
	var src = t.headFrag();

	src += `
vec3 toRgb(vec3 ycc) {
 	vec3 rgb;
	rgb.g = ycc.r + ycc.b/2.0;
	rgb.b = ycc.r - ycc.b/2.0 - ycc.g/2.0;
	rgb.r = rgb.b + ycc.g;
	return rgb;
}
`;

	if(!t.normals) {
		src +=

`
void main(void) { 
	vec3 color = base[0];

	for(int j = 0; j < nj; j++) {
		vec4 c = texture2D(planes[j], v_texcoord);

		vec3 r = vec3(
			(c.x - bias[j*3+1])*scale[j*3+1],
			(c.y - bias[j*3+2])*scale[j*3+2],
			(c.z - bias[j*3+3])*scale[j*3+3]);

		if(j < ny1) {
			color.x += base[j*3+1].x*r.x;
			color.y += base[j*3+2].y*r.y;
			color.z += base[j*3+3].z*r.z;
		} else {
			color.x += base[j*3+1].x*r.x;
			color.x += base[j*3+2].x*r.y;
			color.x += base[j*3+3].x*r.z;
		}
	}
	color = toRgb(color);
`;

	} else {
		src += `

void main(void) { 
	vec3 color0 = base0[0];
	vec3 color1 = base1[0];
	vec3 color2 = base2[0];

	for(int j = 0; j < nj; j++) {
		vec4 c = texture2D(planes[j], v_texcoord);
		vec3 r = vec3(
			(c.x - bias[j*3+1])*scale[j*3+1],
			(c.y - bias[j*3+2])*scale[j*3+2],
			(c.z - bias[j*3+3])*scale[j*3+3]);

		if(j < ny1) {
			color0.x += base0[j*3+1].x*r.x;
			color0.y += base0[j*3+2].y*r.y;
			color0.z += base0[j*3+3].z*r.z;

			color1.x += base1[j*3+1].x*r.x;
			color1.y += base1[j*3+2].y*r.y;
			color1.z += base1[j*3+3].z*r.z;

			color2.x += base2[j*3+1].x*r.x;
			color2.y += base2[j*3+2].y*r.y;
			color2.z += base2[j*3+3].z*r.z;

		} else {
			color0.x += base0[j*3+1].x*r.x;
			color0.x += base0[j*3+2].x*r.y;
			color0.x += base0[j*3+3].x*r.z;

			color1.x += base1[j*3+1].x*r.x;
			color1.x += base1[j*3+2].x*r.y;
			color1.x += base1[j*3+3].x*r.z;

			color2.x += base2[j*3+1].x*r.x;
			color2.x += base2[j*3+2].x*r.y;
			color2.x += base2[j*3+3].x*r.z;
		}
	}

	color0 = toRgb(color0);
	color1 = toRgb(color1);
	color2 = toRgb(color2);

	vec3 color = vec3(color0.x + color0.y + color0.z, color1.x + color1.y + color1.z, color2.x + color2.y + color2.z);
`;
	}

	src += t.fragTail();
	return src;
}


/* RGB */

Relight.prototype.rgbFrag = function() {
	var t = this;
	var src = t.headFrag() + 
`
void main(void) {
	vec3 color = vec3(0);
	for(int j = 0; j < nj; j++) {
		vec4 c = texture2D(planes[j], v_texcoord);
`;
	if(!t.normals) 
		src +=
`		color.x += base[j]*(c.x - bias[j*3+1])*scale[j*3+1];
		color.y += base[j]*(c.y - bias[j*3+2])*scale[j*3+2];
		color.z += base[j]*(c.z - bias[j*3+3])*scale[j*3+3];
	}
`;
	else
		src +=
`		float r = 
			(c.x - bias[j*3+1])*scale[j*3+1] + 
			(c.y - bias[j*3+2])*scale[j*3+2] +
			(c.z - bias[j*3+3])*scale[j*3+3];

		color.x += base0[j]*r;
		color.y += base1[j]*r;
		color.z += base2[j]*r;
	}
`;

	src += t.fragTail();
	return src;
}




/* YCC */

Relight.prototype.yccFrag = function() {
	var t = this;
	var src = t.headFrag() + 
`
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
`;
	src += t.fragTail();
}


/* LRGB */

Relight.prototype.lrgbFrag = function() {
	var t= this;
	var src = t.headFrag();
	if(!t.normals)
		src +=
`
void main(void) {
	vec4 rgb = texture2D(planes[0], v_texcoord);
	float l = 0.0;
	for(int j = 1; j < nj; j++) {
		vec4 c = texture2D(planes[j], v_texcoord);
		l += base[j*3-3]*(c.x - bias[j*3+1])*scale[j*3+1];
		l += base[j*3-2]*(c.y - bias[j*3+2])*scale[j*3+2];
		l += base[j*3-1]*(c.z - bias[j*3+3])*scale[j*3+3];
	}

	vec3 color = vec3(rgb.x*l, rgb.y*l, rgb.z*l);
`;

	else
		src += 
`
void main(void) {
	vec3 color = vec3(0.0, 0.0, 0.0);

	for(int j = 1; j < nj; j++) {
		vec4 c = texture2D(planes[j], v_texcoord);
		vec3 r = vec3(
			(c.x - bias[j*3+1])*scale[j*3+1],
			(c.y - bias[j*3+2])*scale[j*3+2],
			(c.z - bias[j*3+3])*scale[j*3+3]);

		color.x += base0[j*3-3]*r.x + base0[j*3-2]*r.y + base0[j*3-1]*r.z;
		color.y += base1[j*3-3]*r.x + base1[j*3-2]*r.y + base1[j*3-1]*r.z;
		color.z += base2[j*3-3]*r.x + base2[j*3-2]*r.y + base2[j*3-1]*r.z;
	}
`;

	src += t.fragTail();
	return src;
}


/* IMG */

Relight.prototype.imgFrag = function() {

 	var src =  
`#ifdef GL_ES
precision highp float;
#endif

uniform sampler2D planes[1];      //0 is segments
uniform float opacity;

varying vec2 v_texcoord;

void main(void) {
	vec4 c = texture2D(planes[0], v_texcoord);
	gl_FragColor = vec4(c.rgb, opacity);
}`;

	return src;
}

/* DEM */

Relight.prototype.demFrag = function() {

 	var src =  
`#ifdef GL_ES
precision highp float;
#endif

uniform vec3 light;
uniform sampler2D planes[1];      //0 is segments
uniform float opacity;
//uniform float azimuth;

varying vec2 v_texcoord;

void main(void) {
	vec4 a = texture2D(planes[0], v_texcoord + vec2(-0.001, -0.001));
	vec4 b = texture2D(planes[0], v_texcoord + vec2( 0.000, -0.001));
	vec4 c = texture2D(planes[0], v_texcoord + vec2(+0.001, -0.001));
	vec4 d = texture2D(planes[0], v_texcoord + vec2(-0.001,  0.000));
	vec4 e = texture2D(planes[0], v_texcoord + vec2( 0.000,  0.000));
	vec4 f = texture2D(planes[0], v_texcoord + vec2(+0.001,  0.000));
	vec4 g = texture2D(planes[0], v_texcoord + vec2(-0.001, +0.001));
	vec4 h = texture2D(planes[0], v_texcoord + vec2( 0.000, +0.001));
	vec4 i = texture2D(planes[0], v_texcoord + vec2(+0.001, +0.001));

	a.r += a.g/256.0;
	b.r += b.g/256.0;
	c.r += c.g/256.0;
	d.r += d.g/256.0;
	e.r += e.g/256.0;
	f.r += f.g/256.0;
	g.r += g.g/256.0;
	h.r += h.g/256.0;
	i.r += i.g/256.0;

	float cellsize = 0.05;

	float dx = -((c.r + (2.0*f.r) + i.r) - (a.r + (2.0*d.r) + g.r)) / (8.0 * cellsize);
	float dy = ((g.r + (2.0*h.r) + i.r) - (a.r + (2.0*b.r) + c.r)) / (8.0 * cellsize);

	vec3 normal = vec3(-dx, -dy, 1.0);
	normal /= sqrt(1.0 + dx*dx + dy*dy);

	float hillshade = dot(normal, light);
	gl_FragColor = vec4(hillshade, hillshade, hillshade, opacity);
	gl_FragColor = vec4(hillshade, hillshade, hillshade, opacity);
}`;

	return src;
}



Relight.prototype.setupShaders = function() {

	var t = this;

	t.vertCode =
`uniform mat4 u_matrix;
attribute vec4 a_position;
attribute vec2 a_texcoord;

varying vec2 v_texcoord;

void main() {
	gl_Position = a_position; 
	v_texcoord = a_texcoord;
}`;



	var frag;
	switch(t.type) {
	case 'img': frag = t.imgFrag();  break;
	case 'dem': frag = t.demFrag(); break;
	default:
		switch(t.colorspace) {
		case 'mrgb': frag = t.mrgbFrag(); break;
		case 'mycc': frag = t.myccFrag(); break;
		case  'ycc': frag = t.yccFrag();  break;
		case  'rgb': frag = t.rgbFrag();  break;
	case 'lrgb': frag = t.lrgbFrag(); break;
		}
	}
	t.fragCode = frag;
}
