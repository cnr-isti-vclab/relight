
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

uniform sampler2D planes[nj];

varying vec2 v_texcoord;
`;

	return str;
}


/*    MRGB    */

Relight.prototype.mrgbFrag = function() {
	var t = this;

	var src = t.headFrag();

	if(!t.normals)
		src +=

`void main(void) {
	vec3 color = base[0];

	for(int j = 0; j < nj; j++) {
		vec4 c = texture2D(planes[j], v_texcoord);
		color += base[j*3+1]*(c.x - bias[j*3+1])*scale[j*3+1];
		color += base[j*3+2]*(c.y - bias[j*3+2])*scale[j*3+2];
		color += base[j*3+3]*(c.z - bias[j*3+3])*scale[j*3+3];
	}
	gl_FragColor = vec4(color, 1.0);
}
`;

	else
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
	color = (normalize(T * color) + 1.0)/2.0;
	gl_FragColor = vec4(color, 1.0);
}
`;
	return src;
}



/*  MYCC */

Relight.prototype.myccFrag = function() {
	var t = this;
	var src = t.headFrag();

	if(!t.normals)
		src +=

`
void main(void) { 
	vec3 color = base[0];

	for(int j = 0; j < nj; j++) {
		vec4 c = texture2D(planes[j], v_texcoord);

		if(j < ny1) {
			color.x += base[j*3+1].x*(c.x - bias[j*3+1])*scale[j*3+1];
			color.y += base[j*3+2].y*(c.y - bias[j*3+2])*scale[j*3+2];
			color.z += base[j*3+3].z*(c.z - bias[j*3+3])*scale[j*3+3];
		} else {
			color.x += base[j*3+1].x*(c.x - bias[j*3+1])*scale[j*3+1];
			color.x += base[j*3+2].x*(c.y - bias[j*3+2])*scale[j*3+2];
			color.x += base[j*3+3].x*(c.z - bias[j*3+3])*scale[j*3+3];
		}
	}
	float tmp = color.r - color.b/2.0;
	vec3 rgb;
	rgb.g = color.b + tmp;
	rgb.b = tmp - color.g/2.0;
	rgb.r = rgb.b + color.g;

	gl_FragColor = vec4(rgb, 1.0);
}
`;

	else
		src +=
`
void main(void) { 
	vec3 color0 = base0[0];
	vec3 color1 = base1[0];
	vec3 color2 = base2[0];

	for(int j = 0; j < ny1; j++) {
		vec4 c = texture2D(planes[j], v_texcoord);
		vec3 r = vec3(
			(c.x - bias[j*3+1])*scale[j*3+1],
			(c.y - bias[j*3+2])*scale[j*3+2],
			(c.z - bias[j*3+3])*scale[j*3+3]);
		if(j < ny1) {
			color0.x += base0[j*3+1].x*r.x;
			color1.x += base1[j*3+1].x*r.x;
			color2.x += base2[j*3+1].x*r.x;
		} else {
			color0.x += base0[j*3+1].x*r.x;
			color0.x += base0[j*3+2].y*r.y;
			color0.x += base0[j*3+3].z*r.z;

			color1.x += base1[j*3+1].x*r.x;
			color1.x += base1[j*3+2].y*r.y;
			color1.x += base1[j*3+3].z*r.z;

			color2.x += base2[j*3+1].x*r.x;
			color2.x += base2[j*3+2].y*r.y;
			color2.x += base2[j*3+3].z*r.z;
		}
	}
	vec3 color = vec3(color0.r, color1.r, color2.r);
	color = (normalize(T * color) + 1.0)/2.0;

	gl_FragColor = vec4(color, 1.0);
}
`;

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
	color = (normalize(T * color) + 1.0)/2.0;
`;

	src +=
`
	gl_FragColor = vec4(color, 1.0);
}`;

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
	gl_FragColor = vec4(color, 1.0);
}`;

}


/* LRGB */

Relight.prototype.lrgbFrag = function() {
	var t= this;
	var src = t.headFrag();
	if(!t.normals)
		src +=
`
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

	color = (normalize(T * color) + 1.0)/2.0;
	gl_FragColor = vec4(color, 1.0);
}`;

	return src;
}


/* IMG */

Relight.prototype.imgFrag = function() {

 	var src =  
`#ifdef GL_ES
precision highp float;
#endif

uniform sampler2D planes[1];      //0 is segments

varying vec2 v_texcoord;

void main(void) {
	gl_FragColor = texture2D(planes[0], v_texcoord);
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
	switch(t.colorspace) {
	case 'mrgb': frag = t.mrgbFrag(); break;
	case 'mycc': frag = t.myccFrag(); break;
	case  'ycc': frag = t.yccFrag();  break;
	case  'rgb': frag = t.rgbFrag();  break;
	case 'lrgb': frag = t.lrgbFrag(); break;
	default:     frag = t.imgFrag();  break;
	}
	t.fragCode = frag;
	console.log(frag);
}

/* RGB normals
	t.fragCode = `

#ifdef GL_ES\n
precision highp float;
#endif

const int np1 = ${12 + 1};
const int nj = ${t.njpegs};

//transposed matrix see code from relight.cpp getNormalsThreeLights()
const mat3 T = mat3(8.1650e-01, 4.7140e-01, 4.7140e-01,
	-8.1650e-01, 4.7140e-01,  4.7140e-01,
	-1.6222e-08, -9.4281e-01, 4.7140e-01);

uniform sampler2D planes[nj];
uniform float base0[np1-3];
uniform float base1[np1-3];
uniform float base2[np1-3];

uniform float bias[np1];
uniform float scale[np1];

varying vec2 v_texcoord;

void main(void) {
	vec3 color = vec3(0);
	for(int j = 0; j < nj; j++) {

		vec4 c = texture2D(planes[j], v_texcoord);

		//TODO remove scale, it's useless.
		float r = 
			(c.x - bias[j*3+1])*scale[j*3+1] + 
			(c.y - bias[j*3+2])*scale[j*3+2] +
			(c.z - bias[j*3+3])*scale[j*3+3];

		color.y += base0[j]*r;
		color.z += base1[j]*r;
		color.x += base2[j]*r;
	}

	color = (normalize(T * color) + 1.0)/2.0;
	gl_FragColor = vec4(color, 1.0);
}
`;

 */
