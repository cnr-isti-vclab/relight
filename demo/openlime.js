(function (global, factory) {
    typeof exports === 'object' && typeof module !== 'undefined' ? factory(exports) :
    typeof define === 'function' && define.amd ? define(['exports'], factory) :
    (global = typeof globalThis !== 'undefined' ? globalThis : global || self, factory(global.OpenLIME = global.OpenLIME || {}));
})(this, (function (exports) { 'use strict';

    // HELPERS
    window.structuredClone = typeof (structuredClone) == "function" ? structuredClone : function (value) { return JSON.parse(JSON.stringify(value)); };

    /**
     * Utility class providing various helper functions for OpenLIME.
     * Includes methods for SVG manipulation, file loading, image processing, and string handling.
     * 
     * 
     * @static
     */
    class Util {

        /**
         * Pads a number with leading zeros
         * @param {number} num - Number to pad
         * @param {number} size - Desired string length
         * @returns {string} Zero-padded number string
         * 
         * @example
         * ```javascript
         * Util.padZeros(42, 5); // Returns "00042"
         * ```
         */
        static padZeros(num, size) {
            num = num.toString();
            while (num.length < size) num = "0" + num;
            return num;
        }

        /**
         * Prints source code with line numbers
         * Useful for shader debugging
         * @param {string} str - Source code to print
         * @private
         */
        static printSrcCode(str) {
            let i = 1;
            let result = '';
            for (let l of str.split(/\r\n|\r|\n/)) {
                const nline = Util.padZeros(i, 5);
                result += nline + '   ' + l + '\n';
                i++;
            }
            console.log(result);
        }

        /**
         * Creates an SVG element with optional attributes
         * @param {string} tag - SVG element tag name
         * @param {Object} [attributes] - Key-value pairs of attributes
         * @returns {SVGElement} Created SVG element
         * 
         * @example
         * ```javascript
         * const circle = Util.createSVGElement('circle', {
         *     cx: '50',
         *     cy: '50',
         *     r: '40'
         * });
         * ```
         */
        static createSVGElement(tag, attributes) {
            let e = document.createElementNS('http://www.w3.org/2000/svg', tag);
            if (attributes)
                for (const [key, value] of Object.entries(attributes))
                    e.setAttribute(key, value);
            return e;
        }

        /**
         * Parses SVG string into DOM element
         * @param {string} text - SVG content string
         * @returns {SVGElement} Parsed SVG element
         * @throws {Error} If parsing fails
         */
        static SVGFromString(text) {
            const parser = new DOMParser();
            return parser.parseFromString(text, "image/svg+xml").documentElement;
        }

        /**
         * Loads SVG file from URL
         * @param {string} url - URL to SVG file
         * @returns {Promise<SVGElement>} Loaded and parsed SVG
         * @throws {Error} If fetch fails or content isn't SVG
         * 
         * @example
         * ```javascript
         * const svg = await Util.loadSVG('icons/icon.svg');
         * document.body.appendChild(svg);
         * ```
         */
        static async loadSVG(url) {
            let response = await fetch(url);
            if (!response.ok) {
                const message = `An error has occured: ${response.status}`;
                throw new Error(message);
            }
            let data = await response.text();
            let result = null;
            if (Util.isSVGString(data)) {
                result = Util.SVGFromString(data);
            } else {
                const message = `${url} is not an SVG file`;
                throw new Error(message);
            }
            return result;
        };

        /**
         * Loads HTML content from URL
         * @param {string} url - URL to HTML file
         * @returns {Promise<string>} HTML content
         * @throws {Error} If fetch fails
         */
        static async loadHTML(url) {
            let response = await fetch(url);
            if (!response.ok) {
                const message = `An error has occured: ${response.status}`;
                throw new Error(message);
            }
            let data = await response.text();
            return data;
        };

        /**
         * Loads and parses JSON from URL
         * @param {string} url - URL to JSON file
         * @returns {Promise<Object>} Parsed JSON data
         * @throws {Error} If fetch or parsing fails
         */
        static async loadJSON(url) {
            let response = await fetch(url);
            if (!response.ok) {
                const message = `An error has occured: ${response.status}`;
                throw new Error(message);
            }
            let data = await response.json();
            return data;
        }

        /**
         * Loads image from URL
         * @param {string} url - Image URL
         * @returns {Promise<HTMLImageElement>} Loaded image
         * @throws {Error} If image loading fails
         */
        static async loadImage(url) {
            return new Promise((resolve, reject) => {
                const img = new Image();
                img.addEventListener('load', () => resolve(img));
                img.addEventListener('error', (err) => reject(err));
                img.src = url;
            });
        }

        /**
         * Appends loaded image to container
         * @param {HTMLElement} container - Target container
         * @param {string} url - Image URL
         * @param {string} [imgClass] - Optional CSS class
         * @returns {Promise<void>}
         */
        static async appendImg(container, url, imgClass = null) {
            const img = await Util.loadImage(url);
            if (imgClass) img.classList.add(imgClass);
            container.appendChild(img);
            return img;
        }

        /**
          * Appends multiple images to container
          * @param {HTMLElement} container - Target container
          * @param {string[]} urls - Array of image URLs
          * @param {string} [imgClass] - Optional CSS class
          * @returns {Promise<void>}
          */
        static async appendImgs(container, urls, imgClass = null) {
            for (const u of urls) {
                const img = await Util.loadImage(u);
                if (imgClass) img.classList.add(imgClass);
                container.appendChild(img);
            }
        }

        /**
         * Tests if string is valid SVG content
         * @param {string} input - String to test
         * @returns {boolean} True if string is valid SVG
         */
        static isSVGString(input) {
            const regex = /^\s*(?:<\?xml[^>]*>\s*)?(?:<!doctype svg[^>]*\s*(?:\[?(?:\s*<![^>]*>\s*)*\]?)*[^>]*>\s*)?(?:<svg[^>]*>[^]*<\/svg>|<svg[^/>]*\/\s*>)\s*$/i;
            if (input == undefined || input == null)
                return false;
            input = input.toString().replace(/\s*<!Entity\s+\S*\s*(?:"|')[^"]+(?:"|')\s*>/img, '');
            input = input.replace(/<!--([\s\S]*?)-->/g, '');
            return Boolean(input) && regex.test(input);
        }

        /**
         * Computes Signed Distance Field from image data
         * Implementation based on Felzenszwalb & Huttenlocher algorithm
         * 
         * @param {Uint8Array} buffer - Input image data
         * @param {number} w - Image width
         * @param {number} h - Image height
         * @param {number} [cutoff=0.25] - Distance field cutoff
         * @param {number} [radius=8] - Maximum distance to compute
         * @returns {Float32Array|Array} Computed distance field
         * 
         * Technical Details:
         * - Uses 2D Euclidean distance transform
         * - Separate inner/outer distance fields
         * - Optimized grid computation
         * - Sub-pixel accuracy
         */
        static computeSDF(buffer, w, h, cutoff = 0.25, radius = 8) {

            // 2D Euclidean distance transform by Felzenszwalb & Huttenlocher https://cs.brown.edu/~pff/dt/
            function edt(data, width, height, f, d, v, z) {
                for (let x = 0; x < width; x++) {
                    for (let y = 0; y < height; y++) {
                        f[y] = data[y * width + x];
                    }
                    edt1d(f, d, v, z, height);
                    for (let y = 0; y < height; y++) {
                        data[y * width + x] = d[y];
                    }
                }
                for (let y = 0; y < height; y++) {
                    for (let x = 0; x < width; x++) {
                        f[x] = data[y * width + x];
                    }
                    edt1d(f, d, v, z, width);
                    for (let x = 0; x < width; x++) {
                        data[y * width + x] = Math.sqrt(d[x]);
                    }
                }
            }

            // 1D squared distance transform
            function edt1d(f, d, v, z, n) {
                v[0] = 0;
                z[0] = -INF;
                z[1] = +INF;

                for (let q = 1, k = 0; q < n; q++) {
                    var s = ((f[q] + q * q) - (f[v[k]] + v[k] * v[k])) / (2 * q - 2 * v[k]);
                    while (s <= z[k]) {
                        k--;
                        s = ((f[q] + q * q) - (f[v[k]] + v[k] * v[k])) / (2 * q - 2 * v[k]);
                    }
                    k++;
                    v[k] = q;
                    z[k] = s;
                    z[k + 1] = +INF;
                }

                for (let q = 0, k = 0; q < n; q++) {
                    while (z[k + 1] < q) k++;
                    d[q] = (q - v[k]) * (q - v[k]) + f[v[k]];
                }
            }

            var data = new Uint8ClampedArray(buffer);
            const INF = 1e20;
            const size = Math.max(w, h);

            // temporary arrays for the distance transform
            const gridOuter = Array(w * h);
            const gridInner = Array(w * h);
            const f = Array(size);
            const d = Array(size);
            const z = Array(size + 1);
            const v = Array(size);

            for (let i = 0; i < w * h; i++) {
                var a = data[i] / 255.0;
                gridOuter[i] = a === 1 ? 0 : a === 0 ? INF : Math.pow(Math.max(0, 0.5 - a), 2);
                gridInner[i] = a === 1 ? INF : a === 0 ? 0 : Math.pow(Math.max(0, a - 0.5), 2);
            }

            edt(gridOuter, w, h, f, d, v, z);
            edt(gridInner, w, h, f, d, v, z);

            const dist = window.Float32Array ? new Float32Array(w * h) : new Array(w * h);

            for (let i = 0; i < w * h; i++) {
                dist[i] = Math.min(Math.max(1 - ((gridOuter[i] - gridInner[i]) / radius + cutoff), 0), 1);
            }
            return dist;
        }

        /**
         * Rasterizes SVG to ImageData
         * @param {string} url - SVG URL
         * @param {number[]} [size=[64,64]] - Output dimensions [width, height]
         * @returns {Promise<ImageData>} Rasterized image data
         * 
         * Processing steps:
         * 1. Loads SVG file
         * 2. Sets up canvas context
         * 3. Handles aspect ratio
         * 4. Centers image
         * 5. Renders to ImageData
         * 
         * @example
         * ```javascript
         * const imageData = await Util.rasterizeSVG('icon.svg', [128, 128]);
         * context.putImageData(imageData, 0, 0);
         * ```
         */
        static async rasterizeSVG(url, size = [64, 64]) {
            const svg = await Util.loadSVG(url);
            const svgWidth = svg.getAttribute('width');
            const svgHeight = svg.getAttribute('height');

            const canvas = document.createElement("canvas");
            canvas.width = size[0];
            canvas.height = size[1];

            svg.setAttributeNS(null, 'width', `100%`);
            svg.setAttributeNS(null, 'height', `100%`);

            const ctx = canvas.getContext("2d");
            const data = (new XMLSerializer()).serializeToString(svg);
            const DOMURL = window.URL || window.webkitURL || window;

            const img = new Image();
            const svgBlob = new Blob([data], { type: 'image/svg+xml;charset=utf-8' });
            const svgurl = DOMURL.createObjectURL(svgBlob);
            img.src = svgurl;

            return new Promise((resolve, reject) => {
                img.onload = () => {
                    const aCanvas = size[0] / size[1];
                    const aSvg = svgWidth / svgHeight;
                    let wSvg = 0;
                    let hSvg = 0;
                    if (aSvg < aCanvas) {
                        hSvg = size[1];
                        wSvg = hSvg * aSvg;
                    } else {
                        wSvg = size[0];
                        hSvg = wSvg / aSvg;
                    }

                    let dy = (size[1] - hSvg) * 0.5;
                    let dx = (size[0] - wSvg) * 0.5;

                    ctx.translate(dx, dy);
                    ctx.drawImage(img, 0, 0);

                    DOMURL.revokeObjectURL(svgurl);

                    const imageData = ctx.getImageData(0, 0, size[0], size[1]);

                    // const imgURI = canvas
                    //     .toDataURL('image/png')
                    //     .replace('image/png', 'image/octet-stream');

                    // console.log(imgURI);

                    resolve(imageData);
                };
                img.onerror = (e) => reject(e);
            });
        }

    }

    /**
     * Represents an axis-aligned rectangular bounding box that can be wrapped tightly around geometric elements.
     * The box is defined by two opposite vertices (low and high corners) and provides a comprehensive set of
     * utility methods for manipulating and analyzing bounding boxes.
     */
    class BoundingBox {
        /**
         * Creates a new BoundingBox instance.
         * @param {Object} [options] - Configuration options for the bounding box
         * @param {number} [options.xLow=1e20] - X coordinate of the lower corner
         * @param {number} [options.yLow=1e20] - Y coordinate of the lower corner
         * @param {number} [options.xHigh=-1e20] - X coordinate of the upper corner
         * @param {number} [options.yHigh=-1e20] - Y coordinate of the upper corner
         */
        constructor(options) {
            Object.assign(this, {
                xLow: 1e20,
                yLow: 1e20,
                xHigh: -1e20,
                yHigh: -1e20
            });
            Object.assign(this, options);
        }

        /**
         * Initializes the bounding box from an array of coordinates.
         * @param {number[]} x - Array containing coordinates in order [xLow, yLow, xHigh, yHigh]
         */
        fromArray(x) {
            this.xLow = x[0];
            this.yLow = x[1];
            this.xHigh = x[2];
            this.yHigh = x[3];
        }

        /**
         * Resets the bounding box to an empty state by setting coordinates to extreme values.
         */
        toEmpty() {
            this.xLow = 1e20;
            this.yLow = 1e20;
            this.xHigh = -1e20;
            this.yHigh = -1e20;
        }

        /**
         * Checks if the bounding box is empty (has no valid area).
         * A box is considered empty if its low corner coordinates are greater than its high corner coordinates.
         * @returns {boolean} True if the box is empty, false otherwise
         */
        isEmpty() {
            return this.xLow > this.xHigh || this.yLow > this.yHigh;
        }

        /**
         * Converts the bounding box coordinates to an array.
         * @returns {number[]} Array of coordinates in order [xLow, yLow, xHigh, yHigh]
         */
        toArray() {
            return [this.xLow, this.yLow, this.xHigh, this.yHigh];
        }

        /**
          * Creates a space-separated string representation of the bounding box coordinates.
          * @returns {string} String in format "xLow yLow xHigh yHigh"
          */
        toString() {
            return this.xLow.toString() + " " + this.yLow.toString() + " " + this.xHigh.toString() + " " + this.yHigh.toString();
        }

        /**
         * Enlarges this bounding box to include another bounding box.
         * If this box is empty, it will adopt the dimensions of the input box.
         * If the input box is null, no changes are made.
         * @param {BoundingBox|null} box - The bounding box to merge with this one
         */
        mergeBox(box) {
            if (box == null)
                return;

            if (this.isEmpty())
                Object.assign(this, box);
            else {
                this.xLow = Math.min(this.xLow, box.xLow);
                this.yLow = Math.min(this.yLow, box.yLow);
                this.xHigh = Math.max(this.xHigh, box.xHigh);
                this.yHigh = Math.max(this.yHigh, box.yHigh);
            }
        }

        /**
         * Enlarges this bounding box to include a point.
         * @param {{x: number, y: number}} p - The point to include in the bounding box
         */
        mergePoint(p) {
            this.xLow = Math.min(this.xLow, p.x);
            this.yLow = Math.min(this.yLow, p.y);
            this.xHigh = Math.max(this.xHigh, p.x);
            this.yHigh = Math.max(this.yHigh, p.y);
        }

        /**
         * Moves the bounding box by the specified displacement.
         * @param {number} dx - Displacement along the x-axis
         * @param {number} dy - Displacement along the y-axis
         */
        shift(dx, dy) {
            this.xLow += dx;
            this.yLow += dy;
            this.xHigh += dx;
            this.yHigh += dy;
        }

        /**
         * Quantizes the bounding box coordinates by dividing by a specified value and rounding down.
         * This creates a grid-aligned bounding box.
         * @param {number} side - The value to divide coordinates by
         */
        quantize(side) {
            this.xLow = Math.floor(this.xLow / side);
            this.yLow = Math.floor(this.yLow / side);
            this.xHigh = Math.floor((this.xHigh - 1) / side) + 1;
            this.yHigh = Math.floor((this.yHigh - 1) / side) + 1;
        }

        /**
         * Calculates the width of the bounding box.
         * @returns {number} The difference between xHigh and xLow
         */
        width() {
            return this.xHigh - this.xLow;
        }

        /**
         * Calculates the height of the bounding box.
         * @returns {number} The difference between yHigh and yLow
         */
        height() {
            return this.yHigh - this.yLow;
        }

        /**
         * Calculates the center point of the bounding box.
         * @returns {{x: number, y: number}} The coordinates of the center point
         */
        center() {
            return { x: (this.xLow + this.xHigh) / 2, y: (this.yLow + this.yHigh) / 2 };
        }

        /**
         * Gets the coordinates of a specific corner of the bounding box.
         * @param {number} i - Corner index (0: bottom-left, 1: bottom-right, 2: top-left, 3: top-right)
         * @returns {{x: number, y: number}} The coordinates of the specified corner
         */
        corner(i) {
            // To avoid the switch
            let v = this.toArray();
            return { x: v[0 + (i & 0x1) << 1], y: v[1 + (i & 0x2)] };
        }

        /**
         * Checks if this bounding box intersects with another bounding box.
         * @param {BoundingBox} box - The other bounding box to check intersection with
         * @returns {boolean} True if the boxes intersect, false otherwise
         */
        intersects(box) {
            return xLow <= box.xHigh && xHigh >= box.xLow && yLow <= box.yHigh && yHigh >= box.yLow;
        }

        /**
         * Prints the bounding box coordinates to the console in a formatted string.
         * Output format: "BOX=xLow, yLow, xHigh, yHigh" with values rounded to 2 decimal places
         */
        print() {
            console.log("BOX=" + this.xLow.toFixed(2) + ", " + this.yLow.toFixed(2) + ", " + this.xHigh.toFixed(2) + ", " + this.yHigh.toFixed(2));
        }

    }

    /**
     * @typedef {Array<number>} APoint
     * A tuple of [x, y] representing a 2D point.
     * @property {number} 0 - X coordinate
     * @property {number} 1 - Y coordinate
     * 
     * @example
     * ```javascript
     * const point: APoint = [10, 20]; // [x, y]
     * const x = point[0];  // x coordinate
     * const y = point[1];  // y coordinate
     * ```
     */

    /**
     * @typedef {Object} Point
     * Object representation of a 2D point
     * @property {number} x - X coordinate
     * @property {number} y - Y coordinate
     */

    /**
     * @typedef {Object} TransformParameters
     * @property {number} [x=0] - X translation component
     * @property {number} [y=0] - Y translation component
     * @property {number} [a=0] - Rotation angle in degrees
     * @property {number} [z=1] - Scale factor
     * @property {number} [t=0] - Timestamp for animations
     */

    /**
     * @typedef {'linear'|'ease-out'|'ease-in-out'} EasingFunction
     * Animation easing function type
     */

    /**
     * 
     * Implements a 2D affine transformation system for coordinate manipulation.
     * Provides a complete set of operations for coordinate system conversions,
     * camera transformations, and animation support.
     * 
     * Mathematical Model:
     * Transformation of point P to P' follows the equation:
     *
     * P' = z * R(a) * P + T
     *
     * where:
     * - z: scale factor
     * - R(a): rotation matrix for angle 'a'
     * - T: translation vector (x,y)
     * 
     * Key Features:
     * - Full affine transformation support
     * - Camera positioning utilities
     * - Animation interpolation
     * - Viewport projection
     * - Coordinate system conversions
     * - Bounding box transformations
     * 
     *
     * Coordinate Systems and Transformations:
     * 
     * 1. Scene Space:
     * - Origin at image center
     * - Y-axis points up
     * - Unit scale
     * 
     * 2. Viewport Space:
     * - Origin at top-left
     * - Y-axis points down
     * - Pixel units [0..w-1, 0..h-1]
     * 
     * 3. WebGL Space:
     * - Origin at center
     * - Y-axis points up
     * - Range [-1..1, -1..1]
     * 
     * Transform Pipeline:
     * ```
     * Scene -> Transform -> Viewport -> WebGL
     * ```
     * 
     * Animation System:
     * - Time-based interpolation
     * - Multiple easing functions
     * - Smooth transitions
     * 
     * Performance Considerations:
     * - Matrix operations optimized for 2D
     * - Cached transformation results
     * - Efficient composition
     */
    class Transform { //FIXME Add translation to P?
    	/**
    	 * Creates a new Transform instance
    	 * @param {TransformParameters} [options] - Transform configuration
    	 * 
    	 * @example
    	 * ```javascript
    	 * // Create identity transform
    	 * const t1 = new Transform();
    	 * 
    	 * // Create custom transform
    	 * const t2 = new Transform({
    	 *     x: 100,    // Translate 100 units in x
    	 *     y: 50,     // Translate 50 units in y
    	 *     a: 45,     // Rotate 45 degrees
    	 *     z: 2       // Scale by factor of 2
    	 * });
    	 * ```
    	 */
    	constructor(options) {
    		Object.assign(this, { x: 0, y: 0, z: 1, a: 0, t: 0 });

    		if (!this.t) this.t = performance.now();

    		if (typeof (options) == 'object')
    			Object.assign(this, options);
    	}

    	/**
    	 * Creates a deep copy of the transform
    	 * @returns {Transform} New transform with identical parameters
    	 */
    	copy() {
    		let transform = new Transform();
    		Object.assign(transform, this);
    		return transform;
    	}

    	/**
    	 * Applies transform to a point (x,y)
    	 * Performs full affine transformation: scale, rotate, translate
    	 * 
    	 * @param {number} x - X coordinate to transform
    	 * @param {number} y - Y coordinate to transform
    	 * @returns {Point} Transformed point
    	 * 
    	 * @example
    	 * ```javascript
    	 * const transform = new Transform({x: 10, y: 20, a: 45, z: 2});
    	 * const result = transform.apply(5, 5);
    	 * // Returns rotated, scaled, and translated point
    	 * ```
    	 */
    	apply(x, y) {
    		//TODO! ROTATE
    		let r = Transform.rotate(x, y, this.a);
    		return {
    			x: r.x * this.z + this.x,
    			y: r.y * this.z + this.y
    		}
    	}

    	/**
    	 * Computes inverse transformation
    	 * Creates transform that undoes this transform's effects
    	 * @returns {Transform} Inverse transform
    	 */
    	inverse() {
    		let r = Transform.rotate(this.x / this.z, this.y / this.z, -this.a);
    		return new Transform({ x: -r.x, y: -r.y, z: 1 / this.z, a: -this.a, t: this.t });
    	}

    	/**
    	 * Normalizes angle to range [0, 360]
    	 * @param {number} a - Angle in degrees
    	 * @returns {number} Normalized angle
    	 * @static
    	 */
    	static normalizeAngle(a) {
    		while (a > 360) a -= 360;
    		while (a < 0) a += 360;
    		return a;
    	}

    	/**
    	 * Rotates point (x,y) by angle a around Z axis
    	 * @param {number} x - X coordinate to rotate
    	 * @param {number} y - Y coordinate to rotate
    	 * @param {number} a - Rotation angle in degrees
    	 * @returns {Point} Rotated point
    	 * @static
    	 */
    	static rotate(x, y, a) {
    		a = Math.PI * (a / 180);
    		let ex = Math.cos(a) * x - Math.sin(a) * y;
    		let ey = Math.sin(a) * x + Math.cos(a) * y;
    		return { x: ex, y: ey };
    	}

    	/**
    	 * Composes two transforms: this * transform
    	 * Applies this transform first, then the provided transform
    	 * 
    	 * @param {Transform} transform - Transform to compose with
    	 * @returns {Transform} Combined transformation
    	 * 
    	 * @example
    	 * ```javascript
    	 * const t1 = new Transform({x: 10, a: 45});
    	 * const t2 = new Transform({z: 2});
    	 * const combined = t1.compose(t2);
    	 * // Results in rotation, then scale, then translation
    	 * ```
    	 */
    	compose(transform) {
    		let a = this.copy();
    		let b = transform;
    		a.z *= b.z;
    		a.a += b.a;
    		var r = Transform.rotate(a.x, a.y, b.a);
    		a.x = r.x * b.z + b.x;
    		a.y = r.y * b.z + b.y;
    		return a;
    	}

    	/**
    	 * Transforms a bounding box through this transform
    	 * @param {BoundingBox} box - Box to transform
    	 * @returns {BoundingBox} Transformed bounding box
    	 */
    	transformBox(lbox) {
    		let box = new BoundingBox();
    		for (let i = 0; i < 4; i++) {
    			let c = lbox.corner(i);
    			let p = this.apply(c.x, c.y);
    			box.mergePoint(p);
    		}
    		return box;
    	}

    	/**
    	 * Computes viewport bounds in image space
    	 * Accounts for coordinate system differences between viewport and image
    	 * 
    	 * @param {Viewport} viewport - Current viewport
    	 * @returns {BoundingBox} Bounds in image space
    	 */
    	getInverseBox(viewport) {
    		let inverse = this.inverse();
    		let corners = [
    			{ x: viewport.x, y: viewport.y },
    			{ x: viewport.x + viewport.dx, y: viewport.y },
    			{ x: viewport.x, y: viewport.y + viewport.dy },
    			{ x: viewport.x + viewport.dx, y: viewport.y + viewport.dy }
    		];
    		let box = new BoundingBox();
    		for (let corner of corners) {
    			let p = inverse.apply(corner.x - viewport.w / 2, -corner.y + viewport.h / 2);
    			box.mergePoint(p);
    		}
    		return box;
    	}

    	/**
    	 * Interpolates between two transforms
    	 * @param {Transform} source - Starting transform
    	 * @param {Transform} target - Ending transform
    	 * @param {number} time - Current time for interpolation
    	 * @param {EasingFunction} easing - Easing function type
    	 * @returns {Transform} Interpolated transform
    	 * @static
    	 * 
    	 * @example
    	 * ```javascript
    	 * const start = new Transform({x: 0, y: 0});
    	 * const end = new Transform({x: 100, y: 100});
    	 * const mid = Transform.interpolate(start, end, 500, 'ease-out');
    	 * ```
    	 */
    	static interpolate(source, target, time, easing) { //FIXME STATIC
    		console.assert(!isNaN(source.x));
    		console.assert(!isNaN(target.x));
    		const pos = new Transform();
    		let dt = (target.t - source.t);
    		if (time < source.t) {
    			Object.assign(pos, source);
    		} else if (time > target.t || dt < 0.001) {
    			Object.assign(pos, target);
    		} else {
    			let tt = (time - source.t) / dt;
    			switch (easing) {
    				case 'ease-out': tt = 1 - Math.pow(1 - tt, 2); break;
    				case 'ease-in-out': tt = tt < 0.5 ? 2 * tt * tt : 1 - Math.pow(-2 * tt + 2, 2) / 2; break;
    			}
    			let st = 1 - tt;
    			for (let i of ['x', 'y', 'z', 'a'])
    				pos[i] = (st * source[i] + tt * target[i]);
    		}
    		pos.t = time;
    		return pos;
    	}

    	/**
    	 * Generates WebGL projection matrix
    	 * Combines transform with viewport for rendering
    	 * 
    	 * @param {Viewport} viewport - Current viewport
    	 * @returns {number[]} 4x4 projection matrix in column-major order
    	 */
    	projectionMatrix(viewport) {
    		let z = this.z;

    		// In coords with 0 in lower left corner map x0 to -1, and x0+v.w to 1
    		// In coords with 0 at screen center and x0 at 0, map -v.w/2 -> -1, v.w/2 -> 1 
    		// With x0 != 0: x0 -> x0-v.w/2 -> -1, and x0+dx -> x0+v.dx-v.w/2 -> 1
    		// Where dx is viewport width, while w is window width
    		//0, 0 <-> viewport.x + viewport.dx/2 (if x, y =

    		let zx = 2 / viewport.dx;
    		let zy = 2 / viewport.dy;

    		let dx = zx * this.x + (2 / viewport.dx) * (viewport.w / 2 - viewport.x) - 1;
    		let dy = zy * this.y + (2 / viewport.dy) * (viewport.h / 2 - viewport.y) - 1;

    		let a = Math.PI * this.a / 180;
    		let matrix = [
    			Math.cos(a) * zx * z, Math.sin(a) * zy * z, 0, 0,
    			-Math.sin(a) * zx * z, Math.cos(a) * zy * z, 0, 0,
    			0, 0, 1, 0,
    			dx, dy, 0, 1];
    		return matrix;
    	}

    	/**
    	 * Converts scene coordinates to viewport coordinates
    	 * @param {Viewport} viewport - Current viewport
    	 * @param {APoint} p - Point in scene space
    	 * @returns {APoint} Point in viewport space [0..w-1, 0..h-1]
    	 */
    	sceneToViewportCoords(viewport, p) { //FIXME Point is an array, but in other places it is an Object...
    		return [p[0] * this.z + this.x - viewport.x + viewport.w / 2,
    		p[1] * this.z - this.y + viewport.y + viewport.h / 2];
    	}

    	/**
    	 * Converts viewport coordinates to scene coordinates
    	 * @param {Viewport} viewport - Current viewport
    	 * @param {APoint} p - Point in viewport space [0..w-1, 0..h-1]
    	 * @returns {APoint} Point in scene space
    	 */
    	viewportToSceneCoords(viewport, p) {
    		return [(p[0] + viewport.x - viewport.w / 2 - this.x) / this.z,
    		(p[1] - viewport.y - viewport.h / 2 + this.y) / this.z];
    	}

    	/**
    	 * Prints transform parameters for debugging
    	 * @param {string} [str=""] - Prefix string
    	 * @param {number} [precision=0] - Decimal precision
    	 */
    	print(str = "", precision = 0) {
    		const p = precision;
    		console.log(str + " x:" + this.x.toFixed(p) + ", y:" + this.y.toFixed(p) + ", z:" + this.z.toFixed(p) + ", a:" + this.a.toFixed(p) + ", t:" + this.t.toFixed(p));
    	}
    }

    /**
     * @typedef {Object} SignalHandler
     * @property {Object.<string, Function[]>} signals - Map of event names to arrays of callback functions
     * @property {string[]} allSignals - List of all registered signal names
     */
    /**
     * Adds event handling capabilities to a prototype.
     * Creates a simple event system that allows objects to emit and listen to events.
     * 
     * The function modifies the prototype by adding:
     * - Event registration methods
     * - Event emission methods
     * - Signal initialization
     * - Signal storage
     * 
     *
     * Implementation Details
     * 
     * The signal system works by:
     * 1. Extending the prototype with signal tracking properties
     * 2. Maintaining arrays of callbacks for each signal type
     * 3. Providing methods to register and trigger callbacks
     * 
     * Signal Storage Structure:
     * ```javascript
     * {
     *     signals: {
     *         'eventName1': [callback1, callback2, ...],
     *         'eventName2': [callback3, callback4, ...]
     *     },
     *     allSignals: ['eventName1', 'eventName2', ...]
     * }
     * ```
     * 
     * Performance Considerations:
     * - Callbacks are stored in arrays for fast iteration
     * - Signals are initialized lazily on first use
     * - Direct property access for quick event emission
     * 
     * Usage Notes:
     * - Events must be registered before they can be used
     * - Multiple callbacks can be registered for the same event
     * - Callbacks are executed synchronously
     * - Parameters are passed through to callbacks unchanged
     *
     * @function
     * @param {Object} proto - The prototype to enhance with signal capabilities
     * @param {...string} signals - Names of signals to register
     * 
     * @example
     * ```javascript
     * // Add events to a class
     * class MyClass {}
     * addSignals(MyClass, 'update', 'change');
     * 
     * // Use events
     * const obj = new MyClass();
     * obj.addEvent('update', () => console.log('Updated!'));
     * obj.emit('update');
     * ```
     * 
     * @example
     * ```javascript
     * // Multiple signals
     * class DataHandler {}
     * addSignals(DataHandler, 
     *     'dataLoaded',
     *     'dataProcessed',
     *     'error'
     * );
     * 
     * const handler = new DataHandler();
     * handler.addEvent('dataLoaded', (data) => {
     *     console.log('Data loaded:', data);
     * });
     * ```
     */
    function addSignals(proto, ...signals) {
    	if (!proto.prototype.allSignals)
    			proto.prototype.allSignals = [];
    	proto.prototype.allSignals = [...proto.prototype.allSignals, ...signals];

    	/**
    	 * Methods added to the prototype
    	 */
    	/**
    	 * Initializes the signals system for an instance.
    	 * Creates the signals storage object and populates it with empty arrays
    	 * for each registered signal type.
    	 * 
    	 * @memberof SignalHandler
    	 * @instance
    	 * @private
    	 */
    	proto.prototype.initSignals = function () {
    			this.signals = Object.fromEntries(this.allSignals.map(s => [s, []]));
    	};

    	/**
    	 * Registers a callback function for a specific event.
    	 * 
    	 * @memberof SignalHandler
    	 * @instance
    	 * @param {string} event - The event name to listen for
    	 * @param {Function} callback - Function to be called when event is emitted
    	 * @throws {Error} Implicitly if event doesn't exist
    	 * 
    	 * @example
    	 * ```javascript
    	 * obj.addEvent('update', (param1, param2) => {
    	 *     console.log('Update occurred with:', param1, param2);
    	 * });
    	 * ```
    	 */
    	proto.prototype.addEvent = function (event, callback) {
    			if (!this.signals)
    					this.initSignals();
    			this.signals[event].push(callback);
    	};

    	/**
    	 * Adds a one-time event listener that will be automatically removed after first execution.
    	 * Once the event is emitted, the listener is automatically removed before the callback
    	 * is executed.
    	 * 
    	 * @memberof SignalHandler
    	 * @instance
    	 * @param {string} event - The event name to listen for once
    	 * @param {Function} callback - Function to be called once when event is emitted
    	 * @throws {Error} Implicitly if event doesn't exist or callback is not a function
    	 * 
    	 * @example
    	 * ```javascript
    	 * obj.once('update', (param) => {
    	 *     console.log('This will only run once:', param);
    	 * });
    	 * ```
    	 */
    	proto.prototype.once = function (event, callback) {
    			if (!callback || typeof callback !== 'function') {
    					console.error('Callback must be a function');
    					return;
    			}

    			const wrappedCallback = (...args) => {
    					// Remove the listener before calling the callback
    					// to prevent recursion if the callback emits the same event
    					this.removeEvent(event, wrappedCallback);
    					callback.apply(this, args);
    			};

    			this.addEvent(event, wrappedCallback);
    	};

    	/**
    	 * Removes an event callback or all callbacks for a specific event.
    	 * If no callback is provided, all callbacks for the event are removed.
    	 * If a callback is provided, only that specific callback is removed.
    	 * 
    	 * @memberof SignalHandler
    	 * @instance
    	 * @param {string} event - The event name to remove callback(s) from
    	 * @param {Function} [callback] - Optional specific callback function to remove
    	 * @returns {boolean} True if callback(s) were removed, false if event or callback not found
    	 * @throws {Error} Implicitly if event doesn't exist
    	 * 
    	 * @example
    	 * ```javascript
    	 * // Remove specific callback
    	 * const callback = (data) => console.log(data);
    	 * obj.addEvent('update', callback);
    	 * obj.removeEvent('update', callback);
    	 * 
    	 * // Remove all callbacks for an event
    	 * obj.removeEvent('update');
    	 * ```
    	 */
    	proto.prototype.removeEvent = function (event, callback) {
    			if (!this.signals) {
    					this.initSignals();
    					return false;
    			}

    			if (!this.signals[event]) {
    					return false;
    			}

    			if (callback === undefined) {
    					// Remove all callbacks for this event
    					const hadCallbacks = this.signals[event].length > 0;
    					this.signals[event] = [];
    					return hadCallbacks;
    			}

    			// Find and remove specific callback
    			const initialLength = this.signals[event].length;
    			this.signals[event] = this.signals[event].filter(cb => cb !== callback);
    			return initialLength > this.signals[event].length;
    	};

    	/**
    	 * Emits an event, triggering all registered callbacks.
    	 * Callbacks are executed in the order they were registered.
    	 * Creates a copy of the callbacks array before iteration to prevent
    	 * issues if callbacks modify the listeners during emission.
    	 * 
    	 * @memberof SignalHandler
    	 * @instance
    	 * @param {string} event - The event name to emit
    	 * @param {...*} parameters - Parameters to pass to the callback functions
    	 * 
    	 * @example
    	 * ```javascript
    	 * obj.emit('update', 'param1', 42);
    	 * ```
    	 */
    	proto.prototype.emit = function (event, ...parameters) {
    			if (!this.signals)
    					this.initSignals();
    			// Create a copy of the callbacks array to safely iterate even if
    			// callbacks modify the listeners
    			const callbacks = [...this.signals[event]];
    			for (let r of callbacks)
    					r(...parameters);
    	};
    }

    /**
     * Defines a rectangular viewing region inside a canvas area.
     * @typedef {Object} Viewport
     * @property {number} x - X-coordinate of the lower-left corner
     * @property {number} y - Y-coordinate of the lower-left corner
     * @property {number} dx - Width of the viewport
     * @property {number} dy - Height of the viewport
     * @property {number} w - Total canvas width
     * @property {number} h - Total canvas height
     */

    /**
     * Camera class that manages viewport parameters and camera transformations.
     * Acts as a container for parameters needed to define the viewport and camera position,
     * supporting smooth animations between positions using source and target transforms.
     * 
     * The camera maintains two Transform objects:
     * - source: represents current position
     * - target: represents destination position
     * 
     * Animation between positions is handled automatically by the OpenLIME system
     * unless manually interrupted by user input.
     */
    class Camera {
    	/**
    	 * Creates a new Camera instance.
    	 * @param {Object} [options] - Configuration options
    	 * @param {boolean} [options.bounded=true] - Whether to limit camera translation to scene boundaries
    	 * @param {number} [options.maxFixedZoom=2] - Maximum allowed pixel size
    	 * @param {number} [options.minScreenFraction=1] - Minimum portion of screen to show when zoomed in
    	 * @param {Transform} [options.target] - Initial target transform
    	 * @fires Camera#update
    	 */
    	constructor(options) {
    		Object.assign(this, {
    			viewport: null,
    			bounded: true,
    			minScreenFraction: 1,
    			maxFixedZoom: 2,
    			maxZoom: 2,
    			minZoom: 1,
    			boundingBox: new BoundingBox,
    		});
    		Object.assign(this, options);
    		this.target = new Transform(this.target);
    		this.source = this.target.copy();
    		this.easing = 'linear';
    	}

    	/**
    	 * Creates a deep copy of the camera instance.
    	 * @returns {Camera} A new Camera instance with copied properties
    	 */
    	copy() {
    		let camera = new Camera();
    		Object.assign(camera, this);
    		return camera;
    	}

    	/**
    	* Updates the viewport while maintaining the camera position as close as possible to the previous one.
    	* @param {Viewport} view - The new viewport in CSS coordinates
    	*/
    	setViewport(view) {
    		if (this.viewport) {
    			let rz = Math.sqrt((view.w / this.viewport.w) * (view.h / this.viewport.h));
    			this.viewport = view;
    			const { x, y, z, a } = this.target;
    			this.setPosition(0, x, y, z * rz, a);
    		} else {
    			this.viewport = view;
    		}
    	}

    	/**
    	 * Returns the current viewport in device coordinates (accounting for device pixel ratio).
    	 * @returns {Viewport} The current viewport scaled for device pixels
    	 */
    	glViewport() {
    		let d = window.devicePixelRatio;
    		let viewport = {};
    		for (let i in this.viewport)
    			viewport[i] = this.viewport[i] * d;
    		return viewport;
    	}

    	/**
    	 * Converts canvas coordinates to scene coordinates using the specified transform.
    	 * @param {number} x - X coordinate relative to canvas
    	 * @param {number} y - Y coordinate relative to canvas
    	 * @param {Transform} transform - Transform to use for conversion
    	 * @returns {{x: number, y: number}} Coordinates in scene space relative to viewport center
    	 */
    	mapToScene(x, y, transform) {
    		//compute coords relative to the center of the viewport.
    		x -= this.viewport.w / 2;
    		y -= this.viewport.h / 2;
    		x -= transform.x;
    		y -= transform.y;
    		x /= transform.z;
    		y /= transform.z;
    		let r = Transform.rotate(x, y, -transform.a);
    		return { x: r.x, y: r.y };
    	}

    	/**
    	 * Converts scene coordinates to canvas coordinates using the specified transform.
    	 * @param {number} x - X coordinate in scene space
    	 * @param {number} y - Y coordinate in scene space
    	 * @param {Transform} transform - Transform to use for conversion
    	 * @returns {{x: number, y: number}} Coordinates in canvas space
    	 */
    	sceneToCanvas(x, y, transform) {
    		let r = Transform.rotate(x, y, transform.a);
    		x = r.x * transform.z + transform.x - this.viewport.x + this.viewport.w / 2;
    		y = r.y * transform.z - transform.y + this.viewport.y + this.viewport.h / 2;
    		return { x: x, y: y };
    	}

    	/**
    	 * Sets the camera target parameters for a new position.
    	 * @param {number} dt - Animation duration in milliseconds
    	 * @param {number} x - X component of translation
    	 * @param {number} y - Y component of translation
    	 * @param {number} z - Zoom factor
    	 * @param {number} a - Rotation angle in degrees
    	 * @param {string} [easing] - Easing function name for animation
    	 * @fires Camera#update
    	 */
    	setPosition(dt, x, y, z, a, easing) {
    		/**
    		* The event is fired when the camera target is changed.
    		* @event Camera#update
    		*/

    		// Discard events due to cursor outside window
    		//if (Math.abs(x) > 64000 || Math.abs(y) > 64000) return;
    		this.easing = easing || this.easing;

    		if (this.bounded) {
    			const sw = this.viewport.dx;
    			const sh = this.viewport.dy;

    			//
    			let xform = new Transform({ x: x, y: y, z: z, a: a, t: 0 });
    			let tbox = xform.transformBox(this.boundingBox);
    			const bw = tbox.width();
    			const bh = tbox.height();

    			// Screen space offset between image boundary and screen boundary
    			// Do not let transform offet go beyond this limit.
    			// if (scaled-image-size < screen) it remains fully contained
    			// else the scaled-image boundary closest to the screen cannot enter the screen.
    			const dx = Math.abs(bw - sw) / 2;// + this.boundingBox.center().x- tbox.center().x;
    			x = Math.min(Math.max(-dx, x), dx);

    			const dy = Math.abs(bh - sh) / 2;// + this.boundingBox.center().y - tbox.center().y;
    			y = Math.min(Math.max(-dy, y), dy);
    		}

    		let now = performance.now();
    		this.source = this.getCurrentTransform(now);
    		//the angle needs to be interpolated in the shortest direction.
    		//target it is kept between 0 and +360, source is kept relative.
    		a = Transform.normalizeAngle(a);
    		this.source.a = Transform.normalizeAngle(this.source.a);
    		if (a - this.source.a > 180) this.source.a += 360;
    		if (this.source.a - a > 180) this.source.a -= 360;
    		Object.assign(this.target, { x: x, y: y, z: z, a: a, t: now + dt });
    		console.assert(!isNaN(this.target.x));
    		this.emit('update');
    	}

    	/**
    	 * Pans the camera by a specified amount in canvas coordinates.
    	 * @param {number} dt - Animation duration in milliseconds
    	 * @param {number} dx - Horizontal displacement
    	 * @param {number} dy - Vertical displacement
    	 */
    	pan(dt, dx, dy) {
    		let now = performance.now();
    		let m = this.getCurrentTransform(now);
    		m.x += dx;
    		m.y += dy;
    		this.setPosition(dt, m.x, m.y, m.z, m.a);
    	}

    	/**
    	 * Zooms the camera to a specific point in canvas coordinates.
    	 * @param {number} dt - Animation duration in milliseconds
    	 * @param {number} z - Target zoom level
    	 * @param {number} [x=0] - X coordinate to zoom towards
    	 * @param {number} [y=0] - Y coordinate to zoom towards
    	 */
    	zoom(dt, z, x, y) {
    		if (!x) x = 0;
    		if (!y) y = 0;

    		let now = performance.now();
    		let m = this.getCurrentTransform(now);

    		if (this.bounded) {
    			z = Math.min(Math.max(z, this.minZoom), this.maxZoom);
    		}

    		//x, an y should be the center of the zoom.
    		m.x += (m.x + x) * (m.z - z) / m.z;
    		m.y += (m.y + y) * (m.z - z) / m.z;

    		this.setPosition(dt, m.x, m.y, z, m.a);
    	}

    	/**
    	 * Rotates the camera around its z-axis.
    	 * @param {number} dt - Animation duration in milliseconds
    	 * @param {number} a - Rotation angle in degrees
    	 */
    	rotate(dt, a) {

    		let now = performance.now();
    		let m = this.getCurrentTransform(now);

    		this.setPosition(dt, m.x, m.y, m.z, this.target.a + a);
    	}

    	/**
    	 * Applies a relative zoom change at a specific point.
    	 * @param {number} dt - Animation duration in milliseconds
    	 * @param {number} dz - Relative zoom change factor
    	 * @param {number} [x=0] - X coordinate to zoom around
    	 * @param {number} [y=0] - Y coordinate to zoom around
    	 */
    	deltaZoom(dt, dz, x = 0, y = 0) {

    		let now = performance.now();
    		let m = this.getCurrentTransform(now);

    		//rapid firing wheel event need to compound.
    		//but the x, y in input are relative to the current transform.
    		dz *= this.target.z / m.z;

    		if (this.bounded) {
    			if (m.z * dz < this.minZoom) dz = this.minZoom / m.z;
    			if (m.z * dz > this.maxZoom) dz = this.maxZoom / m.z;
    		}

    		//transform is x*z + dx = X , there x is positrion in scene, X on screen
    		//we want x*z*dz + dx1 = X (stay put, we need to find dx1.
    		let r = Transform.rotate(x, y, m.a);
    		m.x += r.x * m.z * (1 - dz);
    		m.y += r.y * m.z * (1 - dz);

    		this.setPosition(dt, m.x, m.y, m.z * dz, m.a);
    	}

    	/**
    	 * Gets the camera transform at a specific time.
    	 * @param {number} time - Current time in milliseconds (from performance.now())
    	 * @returns {Transform} The interpolated transform at the specified time
    	 */
    	getCurrentTransform(time) {
    		if (time > this.target.t) this.easing = 'linear';
    		return Transform.interpolate(this.source, this.target, time, this.easing);
    	}

    	/**
    	 * Gets the camera transform at a specific time in device coordinates.
    	 * @param {number} time - Current time in milliseconds (from performance.now())
    	 * @returns {Transform} The interpolated transform scaled for device pixels
    	 */
    	getGlCurrentTransform(time) {
    		const pos = this.getCurrentTransform(time);
    		pos.x *= window.devicePixelRatio;
    		pos.y *= window.devicePixelRatio;
    		pos.z *= window.devicePixelRatio;
    		return pos;
    	}

    	/**
    	 * Adjusts the camera to frame a specified bounding box.
    	 * @param {BoundingBox} box - The box to frame in canvas coordinates
    	 * @param {number} [dt=0] - Animation duration in milliseconds
    	 */
    	fit(box, dt) {
    		if (box.isEmpty()) return;
    		if (!dt) dt = 0;

    		//find if we align the topbottom borders or the leftright border.
    		let w = this.viewport.dx;
    		let h = this.viewport.dy;

    		let bw = box.width();
    		let bh = box.height();
    		let c = box.center();
    		let z = Math.min(w / bw, h / bh);

    		this.setPosition(dt, -c.x * z, -c.y * z, z, 0);
    	}

    	/**
    	 * Resets the camera to show the entire scene.
    	 * @param {number} dt - Animation duration in milliseconds
    	 */
    	fitCameraBox(dt) {
    		this.fit(this.boundingBox, dt);
    	}

    	/**
    	 * Updates the camera's boundary constraints and zoom limits.
    	 * @private
    	 * @param {BoundingBox} box - New bounding box for constraints
    	 * @param {number} minScale - Minimum scale factor
    	 */
    	updateBounds(box, minScale) {
    		this.boundingBox = box;
    		const w = this.viewport.dx;
    		const h = this.viewport.dy;

    		let bw = this.boundingBox.width();
    		let bh = this.boundingBox.height();

    		this.minZoom = Math.min(w / bw, h / bh) * this.minScreenFraction;
    		this.maxZoom = minScale > 0 ? this.maxFixedZoom / minScale : this.maxFixedZoom;
    		this.maxZoom = Math.max(this.minZoom, this.maxZoom);
    	}
    }

    addSignals(Camera, 'update');

    // Tile level x y  index ----- tex missing() start/end (tarzoom) ----- time, priority size(byte)

    /**
     * @typedef {Object} TileProperties
     * @property {number} index - Unique identifier for the tile
     * @property {number[]} bbox - Bounding box coordinates [minX, minY, maxX, maxY]
     * @property {number} level - Zoom level in the pyramid (for tiled layouts)
     * @property {number} x - Horizontal grid position
     * @property {number} y - Vertical grid position
     * @property {number} w - Tile width (for image layouts)
     * @property {number} h - Tile height (for image layouts)
     * @property {number} start - Starting byte position in dataset (for tar-based formats)
     * @property {number} end - Ending byte position in dataset (for tar-based formats)
     * @property {WebGLTexture[]} tex - Array of WebGL textures (one per channel)
     * @property {number} missing - Count of pending channel data requests
     * @property {number} time - Creation timestamp for cache management
     * @property {number} priority - Loading priority for cache management
     * @property {number} size - Total size in bytes for cache management
     */

    /**
     * 
     * Represents a single tile in an image tiling system.
     * Tiles are fundamental units used to manage large images through regular grid subdivision.
     * Supports both traditional pyramid tiling and specialized formats like RTI/BRDF.
     * 
     * Features:
     * - Multi-channel texture support
     * - Cache management properties
     * - Format-specific byte positioning
     * - Flexible layout compatibility
     * - Priority-based loading
     * 
     * Usage Contexts:
     * 1. Tiled Layouts:
     *    - Part of zoom level pyramid
     *    - Grid-based positioning (x, y, level)
     * 
     * 2. Image Layouts:
     *    - Direct image subdivision
     *    - Dimensional specification (w, h)
     * 
     * 3. Specialized Formats:
     *    - RTI (Reflectance Transformation Imaging)
     *    - BRDF (Bidirectional Reflectance Distribution Function)
     *    - TAR-based formats (tarzoom, itarzoom)
     * 
     *
     * Implementation Details
     * 
     * Property Categories:
     * 
     * 1. Identification:
     * ```javascript
     * {
     *     index: number,    // Unique tile ID
     *     bbox: number[],   // Spatial bounds
     * }
     * ```
     * 
     * 2. Positioning:
     * ```javascript
     * {
     *     // Tiled Layout Properties
     *     level: number,    // Zoom level
     *     x: number,        // Grid X
     *     y: number,        // Grid Y
     *     
     *     // Image Layout Properties
     *     w: number,        // Width
     *     h: number,        // Height
     * }
     * ```
     * 
     * 3. Data Access:
     * ```javascript
     * {
     *     start: number,    // Byte start
     *     end: number,      // Byte end
     *     tex: WebGLTexture[], // Channel textures
     *     missing: number,  // Pending channels
     * }
     * ```
     * 
     * 4. Cache Management:
     * ```javascript
     * {
     *     time: number,     // Creation time
     *     priority: number, // Load priority
     *     size: number      // Memory size
     * }
     * ```
     * 
     * Format-Specific Considerations:
     * 
     * 1. Standard Tiling:
     * - Uses level, x, y for pyramid positioning
     * - Single texture per tile
     * 
     * 2. RTI/BRDF:
     * - Multiple textures per tile (channels)
     * - Missing counter tracks channel loading
     * 
     * 3. TAR Formats:
     * - Uses start/end for byte positioning
     * - Enables direct data access in archives
     * 
     * Cache Management:
     * - time: Used for LRU (Least Recently Used) calculations
     * - priority: Influences loading order
     * - size: Helps manage memory constraints
     */
    class Tile {
        /**
         * Creates a new Tile instance with default properties
         * 
         * @example
         * ```javascript
         * // Create a basic tile
         * const tile = new Tile();
         * tile.level = 2;
         * tile.x = 3;
         * tile.y = 4;
         * tile.priority = 1;
         * ```
         * 
         * @example
         * ```javascript
         * // Create a multi-channel tile
         * const tile = new Tile();
         * tile.tex = new Array(3); // For RGB channels
         * tile.missing = 3; // Waiting for all channels
         * ```
         */
        constructor() {
            Object.assign(this, {
                index: null,
                bbox: null,

                level: null, //used only in LayoutTiles
                x: null,
                y: null,
                w: null, // used only in LayoutImages
                h: null, // used only in LayoutImages

                start: null,
                end: null,

                tex: [],
                missing: null,
                time: null,
                priority: null,
                size: null
            });
        }
    }

    /**
     * Contain functions to pass between different coordinate system.
     * Here described the coordinate system in sequence
     * - CanvasHTML: Html coordinates: 0,0 left,top to width height at bottom right (y Down)
     * - CanvasContext: Same as Html, but scaled by devicePixelRatio (y Down) (required for WebGL, not for SVG)
     * - Viewport: 0,0 left,bottom to (width,height) at top right (y Up)
     * - Center: 0,0 at viewport center (y Up)
     * - Scene: 0,0 at dataset center (y Up). The dataset is placed here through the camera transform 
     * - Layer: 0,0 at Layer center (y Up). Layer is placed over the dataset by the layer transform
     * - Image: 0,0 at left,top (y Down)
     * - Layout: 0,0 at left,top (y Down). Depends on layout
     */
    class CoordinateSystem {
        
        /**
         * Transform point from Viewport to CanvasHTML
         * @param {*} p point in Viewport: 0,0 at left,bottom
         * @param {Camera} camera Camera which contains viewport information
         * @param {bool} useGL True to work with WebGL, false for SVG. When true, it uses devPixelRatio scale
         * @returns  point in CanvasHtml: 0,0 left,top
         */
         static fromViewportToCanvasHtml(p, camera, useGL) {
            const viewport = this.getViewport(camera, useGL);
            let result = this.invertY(p, viewport);
            return useGL ? this.scale(result, 1/window.devicePixelRatio) : result;
        }

        /**
         * Transform point from CanvasHTML to GLViewport
         * @param {*} p point in CanvasHtml: 0,0 left,top y Down
         * @param {Camera} camera Camera
         * @param {bool} useGL True to work with WebGL, false for SVG. When true, it uses devPixelRatio scale
         * @returns  point in GLViewport: 0,0 left,bottom, scaled by devicePixelRatio
         */
         static fromCanvasHtmlToViewport(p, camera, useGL) {
            let result = useGL ? this.scale(p, window.devicePixelRatio) : p;
            const viewport = this.getViewport(camera, useGL);
            return this.invertY(result, viewport);
        }

        
        /**
         * Transform a point from Viewport to Layer coordinates
         * @param {*} p point {x,y} in Viewport (0,0 left,bottom, y Up)
         * @param {Camera} camera camera
         * @param {Transform} layerT layer transform
         * @param {bool} useGL True to work with WebGL, false for SVG. When true, it uses devPixelRatio scale
         * @returns point in Layer coordinates (0, 0 at layer center, y Up)
         */
         static fromViewportToLayer(p, camera, layerT, useGL) {
           // M = InvLayerT * InvCameraT  * Tr(-Vw/2, -Vh/2)
           const cameraT = this.getCurrentTransform(camera, useGL);
           const invCameraT = cameraT.inverse();
           const invLayerT = layerT.inverse();
           const v2c = this.getFromViewportToCenterTransform(camera, useGL);
           const M = v2c.compose(invCameraT.compose(invLayerT)); // First apply v2c, then invCamera, then invLayer
            
           return M.apply(p.x, p.y);
        }

        /**
         * Transform a point from Layer to Viewport coordinates
         * @param {*} p point {x,y} Layer (0,0 at Layer center y Up)
         * @param {Camera} camera 
         * @param {Transform} layerT layer transform
         * @param {bool} useGL True to work with WebGL, false for SVG. When true, it uses devPixelRatio scale
         * @returns point in viewport coordinates (0,0 at left,bottom y Up)
         */
         static fromLayerToViewport(p, camera, layerT, useGL) {
            const M = this.getFromLayerToViewportTransform(camera, layerT, useGL);
            return M.apply(p.x, p.y);
         }

        /**
         * Transform a point from Layer to Center 
         * @param {*} p point {x,y} in Layer coordinates (0,0 at Layer center)
         * @param {Camera} camera camera
         * @param {Transform} layerT layer transform
         * @returns point in Center (0, 0 at glViewport center) coordinates.
         */
         static fromLayerToCenter(p, camera, layerT, useGL) {
            // M = cameraT * layerT
            const cameraT = this.getCurrentTransform(camera, useGL);
            const M = layerT.compose(cameraT);

            return  M.apply(p.x, p.y);
        }

        ////////////// CHECKED UP TO HERE ////////////////////

        /**
         * Transform a point from Layer to Image coordinates
         * @param {*} p point {x, y} Layer coordinates (0,0 at Layer center)
         * @param {*} layerSize {w, h} Size in pixel of the Layer
         * @returns  Point in Image coordinates (0,0 at left,top, y Down)
         */
         static fromLayerToImage(p, layerSize) {
            // InvertY * Tr(Lw/2, Lh/2)
            let result  = {x: p.x + layerSize.w/2, y: p.y + layerSize.h/2};
            return this.invertY(result, layerSize);
        }
        
        /**
         * Transform a point from CanvasHtml to Scene
         * @param {*} p point {x, y} in CanvasHtml (0,0 left,top, y Down)
         * @param {Camera} camera camera
         * @param {bool} useGL True to work with WebGL, false for SVG. When true, it uses devPixelRatio scale
         * @returns Point in Scene coordinates (0,0 at scene center, y Up)
         */
         static fromCanvasHtmlToScene(p, camera, useGL) {
            // invCameraT * Tr(-Vw/2, -Vh/2) * InvertY  * [Scale(devPixRatio)]
            let result = this.fromCanvasHtmlToViewport(p, camera, useGL);
            const v2c = this.getFromViewportToCenterTransform(camera, useGL);
            const invCameraT = this.getCurrentTransform(camera, useGL).inverse();
            const M = v2c.compose(invCameraT);

            return  M.apply(result.x, result.y);
        }

        /**
         * Transform a point from Scene to CanvasHtml
         * @param {*} p point {x, y} Scene coordinates (0,0 at scene center, y Up)
         * @param {Camera} camera camera
         * @param {bool} useGL True to work with WebGL, false for SVG. When true, it uses devPixelRatio scale
         * @returns Point in CanvasHtml (0,0 left,top, y Down)
         */
        static fromSceneToCanvasHtml(p, camera, useGL) {
            // invCameraT * Tr(-Vw/2, -Vh/2) * InvertY  * [Scale(devPixRatio)]
            let result = this.fromSceneToViewport(p, camera, useGL);
            return this.fromViewportToCanvasHtml(result, camera, useGL);
        }

        /**
         * Transform a point from Scene to Viewport
         * @param {*} p point {x, y} Scene coordinates (0,0 at scene center, y Up)
         * @param {Camera} camera camera
         * @param {bool} useGL True to work with WebGL, false for SVG. When true, it uses devPixelRatio scale
         * @returns Point in Viewport (0,0 left,bottom, y Up)
         */
        static fromSceneToViewport(p, camera, useGL) {
            // FromCenterToViewport * CamT
            const c2v = this.getFromViewportToCenterTransform(camera, useGL).inverse();
            const CameraT = this.getCurrentTransform(camera, useGL);
            const M = CameraT.compose(c2v);

            return  M.apply(p.x, p.y);
        }
        
        /**
         * Transform a point from Scene to Viewport, using given transform and viewport
         * @param {*} p point {x, y} Scene coordinates (0,0 at scene center, y Up)
         * @param {Transform} cameraT camera transform
         * @param {*} viewport viewport {x,y,dx,dy,w,h}
         * @returns Point in Viewport (0,0 left,bottom, y Up)
         */
        static fromSceneToViewportNoCamera(p, cameraT, viewport) {
            // invCameraT * Tr(-Vw/2, -Vh/2) * InvertY  * [Scale(devPixRatio)]
            const c2v = this.getFromViewportToCenterTransformNoCamera(viewport).inverse();
            const M = cameraT.compose(c2v);

            return  M.apply(p.x, p.y);
        }
            
        /**
         * Transform a point from Viewport to Scene.
         * @param {*} p point {x, y} Viewport coordinates (0,0 at left,bottom, y Up)
         * @param {Camera} camera camera
         * @param {bool} useGL True to work with WebGL, false for SVG. When true, it uses devPixelRatio scale
         * @returns Point in Viewport (0,0 at scene center, y Up)
         */
         static fromViewportToScene(p, camera, useGL) {
            // invCamT * FromViewportToCenter 
            const v2c = this.getFromViewportToCenterTransform(camera, useGL);
            const invCameraT = this.getCurrentTransform(camera, useGL).inverse();
            const M = v2c.compose(invCameraT);

            return  M.apply(p.x, p.y);
        }

        /**
         * Transform a point from Viewport to Scene, using given transform and viewport
         * @param {*} p point {x, y} Viewport coordinates (0,0 at left,bottom, y Up)
         * @param {Transform} cameraT camera transform
         * @param {*} viewport viewport {x,y,dx,dy,w,h}
         * @returns Point in Viewport (0,0 at scene center, y Up)
         */
        static fromViewportToSceneNoCamera(p, cameraT, viewport) {
            // invCamT * FromViewportToCenter 
            const v2c = this.getFromViewportToCenterTransformNoCamera(viewport);
            const invCameraT = cameraT.inverse();
            const M = v2c.compose(invCameraT);

            return  M.apply(p.x, p.y);
        }
        
        /**
         * Transform a point from CanvasHtml to Image
         * @param {*} p  point {x, y} in CanvasHtml (0,0 left,top, y Down)
         * @param {Camera} camera camera 
         * @param {Transform} layerT layer transform 
         * @param {*} layerSize  {w, h} Size in pixel of the Layer
         * @param {bool} applyGLScale if true apply devPixelRatio scale. Keep it false when working with SVG
         * @returns Point in Image space (0,0 left,top of the image, y Down)
         */
         static fromCanvasHtmlToImage(p, camera, layerT, layerSize, useGL) {
            // Translate(Lw/2, Lh/2) * InvLayerT * InvCameraT *  Translate(-Vw/2, -Vh/2) * invertY * [Scale(devicePixelRatio)]
            // in other words... fromLayerToImage * invLayerT * fromCanvasHtmlToScene
            let result = this.fromCanvasHtmlToScene(p, camera, useGL);
            const invLayerT = layerT.inverse();
            result = invLayerT.apply(result.x, result.y);
            result = this.fromLayerToImage(result, layerSize);

            return result;
        }

        /**
         * Transform a box from Viewport to Image coordinates
         * @param {BoundingBox} box in Viewport coordinates (0,0 at left,bottom, y Up)
         * @param {Transform} cameraT camera Transform
         * @param {*} viewport {x,y,dx,dy,w,h}
         * @param {Transform} layerT layer transform
         * @param {*} layerSize {w,h} layer pixel size
         * @returns box in Image coordinates (0,0 left,top, y Dowm)
         */
         static fromViewportBoxToImageBox(box, cameraT, viewport, layerT, layerSize) {
            // InvertYonImage * T(Lw/2, Lh/2) * InvL * InvCam * T(-Vw/2,-Vh/2) 
            let V2C = new Transform({x:-viewport.w/2, y:-viewport.h/2});
            let C2S = cameraT.inverse();
            let S2L = layerT.inverse();
            let L2I = new Transform({x:layerSize.w/2, y:layerSize.h/2});
            let M = V2C.compose(C2S.compose(S2L.compose(L2I)));
            let resultBox = new BoundingBox();
    		for(let i = 0; i < 4; ++i) {
                let p = box.corner(i);
                p = M.apply(p.x, p.y);
                p = CoordinateSystem.invertY(p, layerSize);
    			resultBox.mergePoint(p);
    		}
            return resultBox;
        }

        /**
         * Transform a box from Layer to Scene 
         * @param {BoundingBox} box  box in Layer coordinates (0,0 at layer center)
         * @param {Transform} layerT layer transform
         * @returns box in Scene coordinates (0,0 at scene center)
         */
         static fromLayerBoxToSceneBox(box, layerT) {
             return layerT.transformBox(box); 
        }
      
        /**
         * Transform a box from Scene to Layer 
         * @param {BoundingBox} box  box in Layer coordinates (0,0 at layer center)
         * @param {Transform} layerT layer transform
         * @returns box in Scene coordinates (0,0 at scene center)
         */
         static fromSceneBoxToLayerBox(box, layerT) {
            return layerT.inverse().transformBox(box); 
       }

        /**
         * Transform a box from Layer to Viewport coordinates
         * @param {BoundingBox} box box in Layer coordinates (0,0 at Layer center y Up)
         * @param {Camera} camera 
         * @param {Transform} layerT layer transform
         * @param {bool} useGL True to work with WebGL, false for SVG. When true, it uses devPixelRatio scale
         * @returns Box in Viewport coordinates (0,0 at left, bottom y Up)
         */
         static fromLayerBoxToViewportBox(box, camera, layerT, useGL) {
            const M = this.getFromLayerToViewportTransform(camera, layerT, useGL);
            return M.transformBox(box);  
        }

        /**
         * Transform a box from Layer to Viewport coordinates
         * @param {BoundingBox} box box in Layer coordinates (0,0 at Layer center y Up)
         * @param {Camera} camera 
         * @param {Transform} layerT layer transform
         * @param {bool} useGL True to work with WebGL, false for SVG. When true, it uses devPixelRatio scale
         * @returns Box in Viewport coordinates (0,0 at left, bottom y Up)
         */
         static fromViewportBoxToLayerBox(box, camera, layerT, useGL) {
            const M = this.getFromLayerToViewportTransform(camera, layerT, useGL).inverse();
            return M.transformBox(box);  
        }

        /**
         * Get a transform to go from viewport 0,0 at left, bottom y Up, to Center 0,0 at viewport center
         * @param {Camera} camera camera
         * @param {bool} useGL True to work with WebGL, false for SVG. When true, it uses devPixelRatio scale
         * @returns transform from Viewport to Center
         */
         static getFromViewportToCenterTransform(camera, useGL) {
            const viewport = this.getViewport(camera, useGL);
            return this.getFromViewportToCenterTransformNoCamera(viewport);
        }

        /**
         * Get a transform to go from viewport 0,0 at left, bottom y Up, to Center 0,0 at viewport center
         * from explicit viewport param. (Not using camera parameter here)
         * @param {*} viewport viewport
         * @returns transform from Viewport to Center
         */
        static getFromViewportToCenterTransformNoCamera(viewport) {
            return new Transform({x:viewport.x-viewport.w/2, y:viewport.y-viewport.h/2, z:1, a:0, t:0});
        }

        /**
         * Return transform with y reflected wrt origin (y=-y)
         * @param {Transform} t  
         * @returns {Transform} transform, with y reflected (around 0)
         */
        static reflectY(t) {
            return new Transform({x:t.x, y:-t.y, z:t.z, a:t.a, t:t.t});
        }

        /**
         * Get a transform to go from Layer (0,0 at Layer center y Up) to Viewport (0,0 at left,bottom y Up)
         * @param {Camera} camera 
         * @param {Transform} layerT layer transform
         * @param {bool} useGL True to work with WebGL, false for SVG. When true, it uses devPixelRatio scale
         * @returns transform from Layer to Viewport
         */
         static getFromLayerToViewportTransform(camera, layerT, useGL) {
            // M =  Center2Viewport * CameraT  * LayerT
            const cameraT = this.getCurrentTransform(camera, useGL);
            const c2v = this.getFromViewportToCenterTransform(camera, useGL).inverse();
            const M = layerT.compose(cameraT.compose(c2v));
            return M;
        }

        /**
         * Get a transform to go from Layer (0,0 at Layer center y Up) to Viewport (0,0 at left,bottom y Up)
         * @param {Transform} CameraT camera transform
         * @param {viewport} viewport {x,y,dx,dy,w,h} viewport
         * @param {Transform} layerT layer transform
         * @returns transform from Layer to Viewport
         */
        static getFromLayerToViewportTransformNoCamera(cameraT, viewport, layerT) {
            // M =  Center2Viewport * CameraT  * LayerT
            const c2v =  this.getFromViewportToCenterTransformNoCamera(viewport).inverse();
            const M = layerT.compose(cameraT.compose(c2v));
            return M;
        }
        

        /**
         * Scale x applying f scale factor
         * @param {*} p Point to be scaled
         * @param {Number} f Scale factor
         * @returns Point in CanvasContext (Scaled by devicePixelRation)
         */
        static scale(p, f) {
            return { x:p.x * f, y:p.y * f};
        }

        /**
         * Invert y with respect to viewport.h
         * @param {*} p Point to be transformed 
         * @param {*} viewport current viewport
         * @returns Point with y inverted with respect to viewport.h
         */
        static invertY(p, viewport) {
            return {x:p.x, y:viewport.h - p.y};
        }

        /**
         * Return the camera viewport: scaled by devicePixelRatio if useGL is true.
         * @param {bool} useGL True to work with WebGL, false for SVG. When true viewport scaled by devPixelRatio 
         * @returns Viewport 
         */
        static getViewport(camera, useGL) {
            return useGL ? camera.glViewport() : camera.viewport;
        }

        static getCurrentTransform(camera, useGL) {
            let cameraT = useGL ?
                            camera.getGlCurrentTransform(performance.now()) :
                            camera.getCurrentTransform(performance.now());
           
            return cameraT;
        }
    }

    // Tile level x y  index ----- tex missing() start/end (tarzoom) ----- time, priority size(byte)

    /**
     * @typedef {Object} TileObj
     * @property {number} level - Zoom level in the image pyramid
     * @property {number} x - Horizontal position in tile grid
     * @property {number} y - Vertical position in tile grid
     * @property {number} index - Unique tile identifier
     * @property {number} [start] - Starting byte position in dataset (for tar formats)
     * @property {number} [end] - Ending byte position in dataset (for tar formats)
     * @property {number} missing - Number of pending channel data requests
     * @property {WebGLTexture[]} tex - Array of textures (one per channel)
     * @property {number} time - Tile creation timestamp for cache management
     * @property {number} priority - Loading priority for cache management
     * @property {number} size - Total tile size in bytes
     */

    /**
     * @typedef {'image'|'deepzoom'|'deepzoom1px'|'google'|'zoomify'|'iiif'|'tarzoom'|'itarzoom'} LayoutType
     * @description Supported image format types:
     * - image: Single-resolution web images (jpg, png, etc.)
     * - deepzoom: Microsoft Deep Zoom with root tile > 1px
     * - deepzoom1px: Microsoft Deep Zoom with 1px root tile
     * - google: Google Maps tiling scheme
     * - zoomify: Zoomify format
     * - iiif: International Image Interoperability Framework
     * - tarzoom: OpenLIME tar-based tiling
     * - itarzoom: OpenLIME indexed tar-based tiling
     */

    /**
     * @typedef {Object} LayoutOptions
     * @property {number} [width] - Image width (required for google layout)
     * @property {number} [height] - Image height (required for google layout)
     * @property {string} [suffix='jpg'] - Tile file extension
     * @property {string} [subdomains='abc'] - Available subdomains for URL templates
     */

    /**
     * Layout manages image formats and tiling schemes in OpenLIME.
     * 
     * This class is responsible for:
     * - Managing different image formats
     * - Handling tiling schemes
     * - Coordinating tile loading
     * - Converting between coordinate systems
     * - Managing tile priorities
     * 
     * Format Support:
     * 1. Single-resolution images:
     * - Direct URL to image file
     * - Supports all standard web formats (jpg, png, etc)
     * 
     * 2. Tiled formats:
     * - DeepZoom (Microsoft): Uses .dzi config file
     * - Google Maps: Direct directory structure
     * - Zoomify: Uses ImageProperties.xml
     * - IIIF: Standard server interface
     * - TarZoom: OpenLIME's optimized format
     * 
     * @fires Layout#ready - When layout is initialized and ready for use
     * @fires Layout#updateSize - When layout dimensions change
     * 
     * @example
     * ```javascript
     * // Single image
     * const imageLayout = new Layout('image.jpg', 'image');
     * 
     * // Deep Zoom
     * const dzLayout = new Layout('tiles.dzi', 'deepzoom');
     * 
     * // Google Maps format
     * const googleLayout = new Layout('tiles/', 'google', {
     *   width: 2000,
     *   height: 1500
     * });
     * ```
     */
    class Layout {
    	/**
    	 * Creates a new Layout instance
    	 * @param {string} url - URL to image or configuration file
    	 * @param {LayoutType} type - Type of image layout
    	 * @param {LayoutOptions} [options] - Additional configuration
    	 * @throws {Error} If layout type is unknown or module not loaded
    	 */
    	constructor(url, type, options) {


    		if (type == 'image') {
    			this.setDefaults(type);
    			this.init(url, type, options);

    		} else if (type in this.types)
    			return this.types[type](url, type, options);

    		else if (type == null)
    			return;

    		else
    			throw "Layout type: " + type + " unknown, or module not loaded";
    	}

    	/**
    	 * Gets tile dimensions
    	 * @returns {number[]} [width, height] of tiles
    	 */
    	getTileSize() {
    		return [this.width, this.height];
    	}

    	/**
    	 * Sets default layout properties
    	 * @param {LayoutType} type - Layout type
    	 * @private
    	 */
    	setDefaults(type) {
    		Object.assign(this, {
    			type: type,
    			width: 0,
    			height: 0,
    			suffix: 'jpg',
    			urls: [],
    			status: null,
    			subdomains: 'abc'
    		});
    	}

    	/**
    	 * Initializes layout configuration
    	 * @param {string} url - Resource URL
    	 * @param {LayoutType} type - Layout type
    	 * @param {LayoutOptions} options - Configuration options
    	 * @private
    	 */
    	init(url, type, options) {
    		if (options)
    			Object.assign(this, options);

    		if (typeof (url) == 'string')
    			this.setUrls([url]);
    		if (this.width && this.height)
    			this.status = 'ready';
    	}

    	/**
    	 * Sets URLs for layout resources
    	 * @param {string[]} urls - Array of resource URLs
    	 * @fires Layout#ready
    	 * @private
    	 */
    	setUrls(urls) {
    		/**
    		* The event is fired when a layout is ready to be drawn(the single-resolution image is downloaded or the multi-resolution structure has been initialized).
    		* @event Layout#ready
    		*/
    		this.urls = urls;
    		this.getTileURL = (rasterid, tile) => { return this.urls[rasterid]; };
    		this.status = 'ready';
    		this.emit('ready');
    	}

    	/**
    	 * Constructs URL for specific image plane
    	 * @param {string} url - Base URL
    	 * @param {string} plane - Plane identifier
    	 * @returns {string} Complete URL
    	 */
    	imageUrl(url, plane) {
    		let path = url.substring(0, url.lastIndexOf('/') + 1);
    		return path + plane + '.jpg';
    	}

    	/**
    	 * Gets URL for specific tile
    	 * @param {number} id - Channel identifier
    	 * @param {TileObj} tile - Tile object
    	 * @returns {string} Tile URL
    	 * @abstract
    	 */
    	getTileURL(id, tile) {
    		throw Error("Layout not defined or ready.");
    	}

    	/**
    	 * Gets layout bounds
    	 * @returns {BoundingBox} Layout boundaries
    	 */
    	boundingBox() {
    		//if(!this.width) throw "Layout not initialized still";
    		return new BoundingBox({ xLow: -this.width / 2, yLow: -this.height / 2, xHigh: this.width / 2, yHigh: this.height / 2 });
    	}

    	/**
    	 * Calculates tile coordinates
    	 * @param Obj} tile - Tile to calculate coordinates for
    	 * @returns {{coords: Float32Array, tcoords: Float32Array}} Image and texture coordinates
    	 */
    	tileCoords(tile) {
    		let w = this.width;
    		let h = this.height;
    		//careful: here y is inverted due to textures not being flipped on load (Firefox fault!).
    		var tcoords = new Float32Array([0, 1, 0, 0, 1, 0, 1, 1]);

    		return {
    			coords: new Float32Array([-w / 2, -h / 2, 0, -w / 2, h / 2, 0, w / 2, h / 2, 0, w / 2, -h / 2, 0]),
    			tcoords: tcoords
    		};
    	}

    	/**
    	 * Creates new tile instance
    	 * @param {number} index - Tile identifier
    	 * @returns {TileObj} New tile object
    	 * @private
    	 */
    	newTile(index) {
    		let tile = new Tile();
    		tile.index = index;
    		return tile;
    	}

    	/**
    	 * Determines required tiles for rendering
    	 * @param {Object} viewport - Current viewport
    	 * @param {Transform} transform - Current transform
    	 * @param {Transform} layerTransform - Layer transform
    	 * @param {number} border - Border size
    	 * @param {number} bias - Mipmap bias
    	 * @param {Map} tiles - Existing tiles
    	 * @param {number} [maxtiles=8] - Maximum tiles to return
    	 * @returns {TileObj[]} Array of needed tiles
    	 */
    	needed(viewport, transform, layerTransform, border, bias, tiles, maxtiles = 8) {
    		//FIXME should check if image is withing the viewport (+ border)
    		let tile = tiles.get(0) || this.newTile(0); //{ index, x, y, missing, tex: [], level };
    		tile.time = performance.now();
    		tile.priority = 10;

    		if (tile.missing === null) // || tile.missing != 0 && !this.requested[index])
    			return [tile];
    		return [];
    	}

    	/**
    	 * Gets tiles available for rendering
    	 * @param {Object} viewport - Current viewport
    	 * @param {Transform} transform - Current transform
    	 * @param {Transform} layerTransform - Layer transform
    	 * @param {number} border - Border size
    	 * @param {number} bias - Mipmap bias
    	 * @param {Map} tiles - Existing tiles
    	 * @returns {Object.<number, Tile>} Map of available tiles
    	 */
    	available(viewport, transform, layerTransform, border, bias, tiles) {
    		//FIXME should check if image is withing the viewport (+ border)
    		let torender = {};

    		if (tiles.has(0) && tiles.get(0).missing == 0)
    			torender[0] = tiles.get(0); //{ index: index, level: level, x: x >> d, y: y >> d, complete: true };
    		return torender;
    	}

    	/**
    	 * Calculates viewport bounding box
    	 * @param {Object} viewport - Viewport parameters
    	 * @param {Transform} transform - Current transform
    	 * @param {Transform} layerT - Layer transform
    	 * @returns {BoundingBox} Viewport bounds in image space
    	 */
    	getViewportBox(viewport, transform, layerT) {
    		const boxViewport = new BoundingBox({ xLow: viewport.x, yLow: viewport.y, xHigh: viewport.x + viewport.dx, yHigh: viewport.y + viewport.dy });
    		return CoordinateSystem.fromViewportBoxToImageBox(boxViewport, transform, viewport, layerT, { w: this.width, h: this.height });
    	}
    }

    Layout.prototype.types = {};

    addSignals(Layout, 'ready', 'updateSize');

    /**
     * Implements a singleton cache system for efficient tile management and retrieval in layers.
     * Handles tile loading, prefetching, and memory management with rate limiting capabilities.
     * @private
     */
    class _Cache {
    	/**
    	 * Creates a new Cache instance.
    	 * @param {Object} [options] - Configuration options for the cache
    	 * @param {number} [options.capacity=536870912] - Total cache capacity in bytes (default: 512MB)
    	 * @param {number} [options.maxRequest=6] - Maximum concurrent HTTP requests
    	 * @param {number} [options.maxRequestsRate=0] - Maximum requests per second (0 for unlimited)
    	 * @param {number} [options.maxPrefetch=8388608] - Maximum prefetch size in bytes (default: 8MB)
    	 */
    	constructor(options) {
    		Object.assign(this, {
    			capacity: 512 * (1 << 20),  //256 MB total capacity available
    			size: 0,                //amount of GPU ram used

    			maxRequest: 6,          //max number of concurrent HTTP requests
    			requested: 0,
    			maxRequestsRate: 0,     //max number of requests per second, 0 means no rate.
    			requestRateTimeout: null, //calls update when a new slot is available due to request rate.
    			lastRequestTimestamp: performance.now(),           //holdls last requests timestamps.
    			maxPrefetch: 8 * (1 << 20), //max amount of prefetched tiles.
    			prefetched: 0           //amount of currently prefetched GPU ram.
    		});

    		Object.assign(this, options);
    		this.layers = [];   //map on layer.
    	}

    	/**
    	 * Registers a layer's tiles as candidates for downloading and initiates the update process.
    	 * @param {Layer} layer - The layer whose tiles should be considered for caching
    	 */
    	setCandidates(layer) {
    		if (!this.layers.includes(layer))
    			this.layers.push(layer);
    		setTimeout(() => { this.update(); }, 0); //ensure all the queues are set before updating.
    	}

    	/**
    		 * Checks if the cache is currently rate limited based on request count and timing.
    	 * @private
    		 * @returns {boolean} True if rate limited, false otherwise
    		 */
    	rateLimited() {
    		if (this.requested > this.maxRequest)
    			return true;

    		if (this.maxRequestsRate == 0)
    			return false;

    		let now = performance.now();
    		let period = 1000 / this.maxRequestsRate;
    		let diff = now - this.lastRequestTimestamp;
    		if (diff > period)
    			return false;


    		if (!this.requestRateTimeout) {
    			this.requestRateTimeout = setTimeout(() => {
    				this.requestRateTimeout = null;
    				this.update();
    			}, period - diff + 10);
    		}
    		return true;
    	}

    	/**
    	 * Updates the cache state by processing the download queue while respecting capacity and rate limits.
    	 * @private
    	 */
    	update() {
    		if (this.rateLimited())
    			return;


    		let best = this.findBestCandidate();
    		if (!best) return;
    		while (this.size > this.capacity) { //we need to make room.
    			let worst = this.findWorstTile();
    			if (!worst) {
    				console.log("BIG problem in the cache");
    				break;
    			}
    			if (worst.tile.time < best.tile.time)
    				this.dropTile(worst.layer, worst.tile);
    			else
    				return;
    		}
    		console.assert(best != best.layer.queue[0]);
    		best.layer.queue.shift();
    		this.lastRequestTimestamp = performance.now();
    		this.loadTile(best.layer, best.tile);
    	}

    	/**
    	 * Identifies the highest priority tile that should be downloaded next.
    	 * @private
    	 * @returns {Object|null} Object containing the best candidate layer and tile, or null if none found
    	 */
    	findBestCandidate() {
    		let best = null;
    		for (let layer of this.layers) {
    			while (layer.queue.length > 0 && layer.tiles.has(layer.queue[0].index)) {
    				layer.queue.shift();
    			}
    			if (!layer.queue.length)
    				continue;
    			let tile = layer.queue[0];
    			if (!best ||
    				tile.time > best.tile.time + 1.0 ||  //old requests ignored
    				tile.priority > best.tile.priority)
    				best = { layer, tile };
    		}
    		return best;
    	}

    	/**
    	 * Identifies the lowest priority tile that should be removed from cache if space is needed.
    	 * @private
    	 * @returns {Object|null} Object containing the worst candidate layer and tile, or null if none found
    	 */
    	findWorstTile() {
    		let worst = null;
    		for (let layer of this.layers) {
    			for (let tile of layer.tiles.values()) {
    				//TODO might be some are present when switching shaders.
    				if (tile.missing != 0) continue;
    				if (!worst ||
    					tile.time < worst.tile.time ||
    					(tile.time == worst.tile.time && tile.priority < worst.tile.priority)) {
    					worst = { layer, tile };
    				}
    			}
    		}
    		return worst;
    	}

    	/**
    	 * Initiates the loading of a tile for a specific layer.
    	 * @private
    	 * @param {Layer} layer - The layer the tile belongs to
    	 * @param {Object} tile - The tile to be loaded
    	 */
    	loadTile(layer, tile) {
    		this.requested++;
    		(async () => { layer.loadTile(tile, (size) => { this.size += size; this.requested--; this.update(); }); })();
    	}

    	/**
    	 * Removes a tile from the cache and updates the cache size.
    	 * @private
    	 * @param {Layer} layer - The layer the tile belongs to
    	 * @param {Object} tile - The tile to be removed
    	 */
    	dropTile(layer, tile) {
    		this.size -= tile.size;
    		layer.dropTile(tile);
    	}

    	/**
    	 * Removes all tiles associated with a specific layer from the cache.
    	 * @param {Layer} layer - The layer whose tiles should be flushed
    	 */
    	flushLayer(layer) {
    		if (!this.layers.includes(layer))
    			return;
    		for (let tile of layer.tiles.values())
    			this.dropTile(layer, tile);
    	}
    }

    /**
     * Singleton cache instance for managing tile loading and caching across layers.
     * Provides efficient tile retrieval and memory management with configurable capacity
     * and request rate limiting.
     * 
     * @namespace
     * @property {number} capacity - Total cache capacity in bytes (default: 512MB)
     * @property {number} size - Current amount of GPU RAM used
     * @property {number} maxRequest - Maximum concurrent HTTP requests (default: 6)
     * @property {number} requested - Current number of active requests
     * @property {number} maxRequestsRate - Maximum requests per second (0 for unlimited)
     * @property {number} maxPrefetch - Maximum size of prefetched tiles in bytes
     * @property {number} prefetched - Current amount of prefetched GPU RAM
     * @property {Layer[]} layers - Array of layers being managed by the cache
     */
    const Cache = new _Cache;

    /**
     * @typedef {Object} LayerOptions
     * @property {string|Layout} [layout='image'] - Layout/format of input raster images
     * @property {string} [type] - Identifier for specific derived layer class
     * @property {string} [id] - Unique layer identifier
     * @property {string} [label] - Display label for UI (defaults to id)
     * @property {Transform} [transform] - Transform from layer to canvas coordinates
     * @property {boolean} [visible=true] - Whether layer should be rendered
     * @property {number} [zindex=0] - Stack order for rendering (higher = on top)
     * @property {boolean} [overlay=false] - Whether layer renders in overlay mode
     * @property {number} [prefetchBorder=1] - Tile prefetch threshold in tile units
     * @property {number} [mipmapBias=0.4] - Texture resolution selection bias (0=highest, 1=lowest)
     * @property {Object.<string, Shader>} [shaders] - Map of available shaders
     * @property {Controller[]} [controllers] - Array of active UI controllers
     * @property {Layer} [sourceLayer] - Layer to share tiles with
     * @property {number} [pixelSize=0.0] - Physical size of a pixel in mm
     */

    /**
     * Layer is the core class for rendering content in OpenLIME.
     * It manages raster data display, tile loading, and shader-based rendering.
     * 
     * Features:
     * - Tile-based rendering with level-of-detail
     * - Shader-based visualization effects
     * - Automatic tile prefetching and caching
     * - Coordinate system transformations
     * - Animation and interpolation of shader parameters
     * - Support for multiple visualization modes
     * - Integration with layout systems for different data formats
     * 
     * Layers can be used directly or serve as a base class for specialized layer types.
     * The class uses a registration system where derived classes register themselves,
     * allowing instantiation through the 'type' option.
     * 
     * @fires Layer#ready - Fired when layer is initialized
     * @fires Layer#update - Fired when redraw is needed
     * @fires Layer#loaded - Fired when all tiles are loaded
     * @fires Layer#updateSize - Fired when layer size changes
     * 
     * @example
     * ```javascript
     * // Create a basic image layer
     * const layer = new OpenLIME.Layer({
     *   layout: 'deepzoom',
     *   type: 'image',
     *   url: 'path/to/image.dzi',
     *   label: 'Main Image'
     * });
     * 
     * // Add to viewer
     * viewer.addLayer('main', layer);
     * 
     * // Listen for events
     * layer.addEvent('ready', () => {
     *   console.log('Layer initialized');
     * });
     * ```
     */
    class Layer {
    	/**
    	* Creates a Layer. Additionally, an object literal with Layer `options` can be specified.
    	* Signals are triggered when:
    	* ready: the size and layout of the layer is known
    	* update: some new tile is available, or some visualization parameters has changed
    	* loaded: is fired when all the images needed have been downloaded
    	* @param {Object} [options]
    	* @param {(string|Layout)} options.layout='image' The layout (the format of the input raster images).
    	* @param {string} options.type A string identifier to select the specific derived layer class to instantiate.
    	* @param {string} options.id The layer unique identifier.
    	* @param {string} options.label A string with a more comprehensive definition of the layer. If it exists, it is used in the UI layer menu, otherwise the `id` value is taken.
    	* @param {Transform} options.transform The relative coords from layer to canvas.
    	* @param {bool} options.visible=true Whether to render the layer.
    	* @param {number} options.zindex Stack ordering value for the rendering of layers (higher zindex on top).
    	* @param {bool} options.overlay=false  Whether the layer must be rendered in overlay mode.
    	* @param {number} options.prefetchBorder=1 The threshold (in tile units) around the current camera position for which to prefetch tiles.
    	* @param {number} options.mipmapBias=0.2 Determine which texture is used when scale is not a power of 2. 0: use always the highest resulution, 1 the lowest, 0.5 switch halfway.
    	* @param {Object} options.shaders A map (shadersId, shader) of the shaders usable for the layer rendering. See @link {Shader}.
    	* @param {Controller[]} options.controllers An array of UI device controllers active on the layer.
    	* @param {Layer} options.sourceLayer The layer from which to take the tiles (in order to avoid tile duplication).
    	*/
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
    	}

    	/** @ignore */
    	init(options) {
    		Object.assign(this, {
    			transform: new Transform(),
    			viewport: null,
    			debug: false,
    			visible: true,
    			zindex: 0,
    			overlay: false, //in the GUI it won't affect the visibility of the other layers
    			rasters: [],
    			layers: [],
    			controls: {},
    			controllers: [],
    			shaders: {},
    			layout: 'image',
    			shader: null, //current shader.
    			gl: null,
    			width: 0,
    			height: 0,
    			prefetchBorder: 1,
    			mipmapBias: 0.4,
    			pixelSize: 0.0,

    			//signals: { update: [], ready: [], updateSize: [] },  //update callbacks for a redraw, ready once layout is known.

    			//internal stuff, should not be passed as options.
    			tiles: new Map(),      //keep references to each texture (and status) indexed by level, x and y.
    			//each tile is tex: [.. one for raster ..], missing: 3 missing tex before tile is ready.
    			//only raster used by the shader will be loade.
    			queue: [],     //queue of tiles to be loaded.
    			requested: new Map,  //tiles requested.
    		});

    		Object.assign(this, options);
    		if (this.sourceLayer) this.tiles = this.sourceLayer.tiles; //FIXME avoid tiles duplication

    		this.transform = new Transform(this.transform);

    		if (typeof (this.layout) == 'string') {
    			let size = { width: this.width, height: this.height };
    			if (this.server) size.server = this.server;
    			this.setLayout(new Layout(null, this.layout, size));
    		} else {
    			this.setLayout(this.layout);
    		}
    	}

    	/**
    	 * Sets the layer's viewport
    	 * @param {Object} view - Viewport specification
    	 * @param {number} view.x - X position
    	 * @param {number} view.y - Y position
    	 * @param {number} view.dx - Width 
    	 * @param {number} view.dy - Height
    	 * @fires Layer#update
    	 */
    	setViewport(view) {
    		this.viewport = view;
    		this.emit('update');
    	}

    	/**
    	 * Adds a filter to the current shader
    	 * @param {Object} filter - Filter specification
    	 * @throws {Error} If no shader is set
    	 */
    	addShaderFilter(f) {
    		if (!this.shader) throw "Shader not implemented";
    		this.shader.addFilter(f);
    	}

    	/**
    	 * Removes a filter from the current shader
    	 * @param {Object} name - Filter name
    	 * @throws {Error} If no shader is set
    	 */
    	removeShaderFilter(name) {
    		if (!this.shader) throw "Shader not implemented";
    		this.shader.removeFilter(name);
    	}

    	/**
    	 * Removes all filters from the current shader
    	 * @param {Object} name - Filter name
    	 * @throws {Error} If no shader is set
    	 */
    	clearShaderFilters() {
    		if (!this.shader) throw "Shader not implemented";
    		this.shader.clearFilters();
    	}

    	/**
    	 * Sets the layer state with optional animation
    	 * @param {Object} state - State object with controls and mode
    	 * @param {number} [dt] - Animation duration in ms
    	 * @param {string} [easing='linear'] - Easing function ('linear'|'ease-out'|'ease-in-out')
    	 */
    	setState(state, dt, easing = 'linear') {
    		if ('controls' in state)
    			for (const [key, v] of Object.entries(state.controls)) {
    				this.setControl(key, v, dt, easing);
    			}
    		if ('mode' in state && state.mode) {
    			this.setMode(state.mode);
    		}
    	}

    	/**
    	 * Gets the current layer state
    	 * @param {Object} [stateMask] - Optional mask to filter returned state properties
    	 * @returns {Object} Current state object
    	 */
    	getState(stateMask = null) {
    		const state = {};
    		state.controls = {};
    		for (const [key, v] of Object.entries(this.controls)) {
    			if (!stateMask || ('controls' in stateMask && key in stateMask.controls))
    				state.controls[key] = v.current.value;
    		}
    		if (!stateMask || 'mode' in stateMask)
    			if (this.getMode())
    				state.mode = this.getMode();
    		return state;
    	}

    	/** @ignore */
    	setLayout(layout) {
    		/**
    		* The event is fired when a layer is initialized.
    		* @event Layer#ready
    		*/
    		/**
    		* The event is fired if a redraw is needed.
    		* @event Layer#update
    		*/
    		this.layout = layout;

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

    		// Set signal to acknowledge change of bbox when it is known. Let this signal go up to canvas
    		this.layout.addEvent('updateSize', () => {
    			if (this.shader)
    				this.shader.setTileSize(this.layout.getTileSize());
    			this.emit('updateSize');
    		});
    	}

    	/**
    	 * Sets the layer's transform
    	 * @param {Transform} tx - New transform
    	 * @fires Layer#updateSize
    	 */
    	setTransform(tx) { //FIXME
    		this.transform = tx;
    		this.emit('updateSize');
    	}

    	/**
    	 * Sets the active shader
    	 * @param {string} id - Shader identifier from registered shaders
    	 * @throws {Error} If shader ID is not found
    	 * @fires Layer#update
    	 */
    	setShader(id) {
    		if (!id in this.shaders)
    			throw "Unknown shader: " + id;
    		this.shader = this.shaders[id];
    		this.setupTiles();
    		this.shader.addEvent('update', () => { this.emit('update'); });
    	}

    	/**
    	 * Gets the current shader visualization mode
    	 * @returns {string|null} Current mode or null if no shader
    	 */
    	getMode() {
    		if (this.shader)
    			return this.shader.mode;
    		return null;
    	}

    	/**
    	 * Gets available shader modes
    	 * @returns {string[]} Array of available modes
    	 */
    	getModes() {
    		if (this.shader)
    			return this.shader.modes;
    		return [];
    	}

    	/**
    	 * Sets shader visualization mode
    	 * @param {string} mode - Mode to set
    	 * @fires Layer#update
    	 */
    	setMode(mode) {
    		this.shader.setMode(mode);
    		this.emit('update');
    	}

    	/**
    	 * Sets layer visibility
    	 * @param {boolean} visible - Whether layer should be visible
    	 * @fires Layer#update
    	 */
    	setVisible(visible) {
    		this.visible = visible;
    		this.previouslyNeeded = null;
    		this.emit('update');
    	}

    	/**
    	 * Sets layer rendering order
    	 * @param {number} zindex - Stack order value
    	 * @fires Layer#update
    	 */
    	setZindex(zindex) {
    		this.zindex = zindex;
    		this.emit('update');
    	}

    	/**
    	 * Computes minimum scale across layers
    	 * @param {Object.<string, Layer>} layers - Map of layers
    	 * @param {boolean} discardHidden - Whether to ignore hidden layers
    	 * @returns {number} Minimum scale value
    	 * @static
    	 */
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

    	/**
    	 * Gets layer scale
    	 * @returns {number} Current scale value
    	 */
    	scale() {
    		// FIXME: this do not consider children layers
    		return this.transform.z;
    	}


    	/**
    	 * Gets pixel size in millimeters
    	 * @returns {number} Size of one pixel in mm
    	 */
    	pixelSizePerMM() {
    		return this.pixelSize * this.transform.z;
    	}


    	/**
    	 * Gets layer bounding box in scene coordinates
    	 * @returns {BoundingBox} Bounding box
    	 */
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

    	/**
    	 * Computes combined bounding box of multiple layers
    	 * @param {Object.<string, Layer>} layers - Map of layers
    	 * @param {boolean} discardHidden - Whether to ignore hidden layers
    	 * @returns {BoundingBox} Combined bounding box
    	 * @static
    	 */
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

    	/**
    	 * Gets the shader parameter control corresponding to `name`
    	 * @param {*} name The name of the control.
    	 * return {*} The control
    	 */
    	getControl(name) {
    		let control = this.controls[name] ? this.controls[name] : null;
    		if (control) {
    			let now = performance.now();
    			this.interpolateControl(control, now);
    		}
    		return control;
    	}

    	/**
    	 * Adds a shader parameter control
    	 * @param {string} name - Control identifier
    	 * @param {*} value - Initial value
    	 * @throws {Error} If control already exists
    	 */
    	addControl(name, value) {
    		if (this.controls[name])
    			throw new Error(`Control "$name" already exist!`);
    		let now = performance.now();
    		this.controls[name] = { 'source': { 'value': value, 't': now }, 'target': { 'value': value, 't': now }, 'current': { 'value': value, 't': now }, 'easing': 'linear' };
    	}

    	/**
    	 * Sets a shader control value with optional animation
    	 * @param {string} name - Control identifier
    	 * @param {*} value - New value
    	 * @param {number} [dt] - Animation duration in ms
    	 * @param {string} [easing='linear'] - Easing function
    	 * @fires Layer#update
    	 */
    	setControl(name, value, dt, easing = 'linear') { //When are created?
    		let now = performance.now();
    		let control = this.controls[name];
    		this.interpolateControl(control, now);

    		control.source.value = [...control.current.value];
    		control.source.t = now;

    		control.target.value = [...value];
    		control.target.t = now + dt;

    		control.easing = easing;

    		this.emit('update');
    	}

    	/**
    	 * Updates control interpolation
    	 * @returns {boolean} Whether all interpolations are complete
    	 */
    	interpolateControls() {
    		let now = performance.now();
    		let done = true;
    		for (let control of Object.values(this.controls))
    			done = this.interpolateControl(control, now) && done;
    		return done;
    	}

    	/** @ignore */
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

    		let dt = (target.t - source.t);
    		let tt = (time - source.t) / dt;
    		switch (control.easing) {
    			case 'ease-out': tt = 1 - Math.pow(1 - tt, 2); break;
    			case 'ease-in-out': tt = tt < 0.5 ? 2 * tt * tt : 1 - Math.pow(-2 * tt + 2, 2) / 2; break;
    		}
    		let st = 1 - tt;

    		current.value = [];
    		for (let i = 0; i < source.value.length; i++)
    			current.value[i] = (st * source.value[i] + tt * target.value[i]);
    		return false;
    	}

    	/////////////
    	/// CACHE HANDLING & RENDERING

    	/** @ignore */
    	dropTile(tile) {
    		for (let i = 0; i < tile.tex.length; i++) {
    			if (tile.tex[i]) {
    				this.gl.deleteTexture(tile.tex[i]);
    			}
    		}
    		this.tiles.delete(tile.index);
    	}

    	/**
    	 * Clears layer resources and resets state
    	 * @private
    	 */
    	clear() {
    		this.ibuffer = this.vbuffer = null;
    		Cache.flushLayer(this);
    		this.tiles = new Map(); //TODO We need to drop these tile textures before clearing Map
    		this.setupTiles();
    		this.queue = [];
    		this.previouslyNeeded = false;
    	}

    	/*
    	 * Renders the layer
    	 */
    	/** @ignore */
    	draw(transform, viewport) {
    		//exception for layout image where we still do not know the image size
    		//how linear or srgb should be specified here.
    		//		gl.pixelStorei(gl.UNPACK_COLORSPACE_CONVERSION_WEBGL, gl.NONE);
    		if (this.status != 'ready')// || this.tiles.size == 0)
    			return true;

    		if (!this.shader)
    			throw "Shader not specified!";

    		let done = this.interpolateControls();

    		let parent_viewport = viewport;
    		if (this.viewport) {
    			viewport = this.viewport;
    			this.gl.viewport(viewport.x, viewport.y, viewport.dx, viewport.dy);
    		}


    		this.prepareWebGL();

    		//		find which quads to draw and in case request for them
    		let available = this.layout.available(viewport, transform, this.transform, 0, this.mipmapBias, this.tiles);

    		transform = this.transform.compose(transform);
    		let matrix = transform.projectionMatrix(viewport);
    		this.gl.uniformMatrix4fv(this.shader.matrixlocation, this.gl.FALSE, matrix);

    		this.updateAllTileBuffers(available);

    		// bind filter textures
    		let iSampler = this.shader.samplers.length;
    		for (const f of this.shader.filters) {
    			for (let i = 0; i < f.samplers.length; i++) {
    				this.gl.uniform1i(f.samplers[i].location, iSampler);
    				this.gl.activeTexture(this.gl.TEXTURE0 + iSampler);
    				this.gl.bindTexture(this.gl.TEXTURE_2D, f.samplers[i].tex);
    				iSampler++;
    			}
    		}

    		let i = 0;
    		for (let tile of Object.values(available)) {
    			//			if(tile.complete)
    			this.drawTile(tile, i);
    			++i;
    		}
    		if (this.vieport)
    			this.gl.viewport(parent_viewport.x, parent_viewport.y, parent_viewport.dx, parent_viewport.dy);

    		return done;
    	}

    	/** @ignore */
    	drawTile(tile, index) {
    		//let tiledata = this.tiles.get(tile.index);
    		if (tile.missing != 0)
    			throw "Attempt to draw tile still missing textures"

    		//coords and texture buffers updated once for all tiles from main draw() call

    		//bind textures
    		let gl = this.gl;
    		for (var i = 0; i < this.shader.samplers.length; i++) {
    			let id = this.shader.samplers[i].id;
    			gl.uniform1i(this.shader.samplers[i].location, i);
    			gl.activeTexture(gl.TEXTURE0 + i);
    			gl.bindTexture(gl.TEXTURE_2D, tile.tex[id]);
    		}

    		// for (var i = 0; i < this.shader.samplers.length; i++) {
    		// 	let id = this.shader.samplers[i].id;
    		// 	gl.uniform1i(this.shader.samplers[i].location, i);
    		// 	gl.activeTexture(gl.TEXTURE0 + i);
    		// 	gl.bindTexture(gl.TEXTURE_2D, tile.tex[id]);
    		// } // FIXME - TO BE REMOVED?

    		const byteOffset = this.getTileByteOffset(index);
    		gl.drawElements(gl.TRIANGLES, 6, gl.UNSIGNED_SHORT, byteOffset);
    	}

    	getTileByteOffset(index) {
    		return index * 6 * 2;
    	}

    	/* given the full pyramid of needed tiles for a certain bounding box, 
    	 *  starts from the preferred levels and goes up in the hierarchy if a tile is missing.
    	 *  complete is true if all of the 'brothers' in the hierarchy are loaded,
    	 *  drawing incomplete tiles enhance the resolution early at the cost of some overdrawing and problems with opacity.
    	 */
    	/** @ignore */
    	/*toRender(needed) {

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
    						torender[index] = this.tiles.get(index); //{ index: index, level: level, x: x >> d, y: y >> d, complete: true };
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
    	}*/

    	/** @ignore */
    	// Update tile vertex and texture coords.
    	// Currently called by derived classes 
    	updateTileBuffers(coords, tcoords) {
    		let gl = this.gl;
    		//TODO to reduce the number of calls (probably not needed) we can join buffers, and just make one call per draw! (except the bufferData, which is per node)
    		gl.bindBuffer(gl.ARRAY_BUFFER, this.vbuffer);
    		gl.bufferData(gl.ARRAY_BUFFER, coords, gl.STATIC_DRAW);
    		//FIXME this is not needed every time.
    		gl.vertexAttribPointer(this.shader.coordattrib, 3, gl.FLOAT, false, 0, 0);
    		gl.enableVertexAttribArray(this.shader.coordattrib);

    		gl.bindBuffer(gl.ARRAY_BUFFER, this.tbuffer);
    		gl.bufferData(gl.ARRAY_BUFFER, tcoords, gl.STATIC_DRAW);

    		gl.vertexAttribPointer(this.shader.texattrib, 2, gl.FLOAT, false, 0, 0);
    		gl.enableVertexAttribArray(this.shader.texattrib);
    	}


    	/** @ignore */
    	// Update tile vertex and texture coords of all the tiles in a single VBO
    	updateAllTileBuffers(tiles) {
    		let gl = this.gl;

    		//use this.tiles instead.
    		let N = Object.values(tiles).length;
    		if (N == 0) return;

    		const szV = 12;
    		const szT = 8;
    		const szI = 6;
    		const iBuffer = new Uint16Array(szI * N);
    		const vBuffer = new Float32Array(szV * N);
    		const tBuffer = new Float32Array(szT * N);
    		let i = 0;
    		for (let tile of Object.values(tiles)) {
    			let c = this.layout.tileCoords(tile);
    			vBuffer.set(c.coords, i * szV);
    			tBuffer.set(c.tcoords, i * szT);

    			const off = i * 4;
    			tile.indexBufferByteOffset = 2 * i * szI;
    			iBuffer.set([off + 3, off + 2, off + 1, off + 3, off + 1, off + 0], i * szI);
    			++i;
    		}
    		gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, this.ibuffer);
    		gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, iBuffer, gl.STATIC_DRAW);

    		gl.bindBuffer(gl.ARRAY_BUFFER, this.vbuffer);
    		gl.bufferData(gl.ARRAY_BUFFER, vBuffer, gl.STATIC_DRAW);

    		gl.vertexAttribPointer(this.shader.coordattrib, 3, gl.FLOAT, false, 0, 0);
    		gl.enableVertexAttribArray(this.shader.coordattrib);

    		gl.bindBuffer(gl.ARRAY_BUFFER, this.tbuffer);
    		gl.bufferData(gl.ARRAY_BUFFER, tBuffer, gl.STATIC_DRAW);

    		gl.vertexAttribPointer(this.shader.texattrib, 2, gl.FLOAT, false, 0, 0);
    		gl.enableVertexAttribArray(this.shader.texattrib);

    	}

    	/*
    	 *  If layout is ready and shader is assigned, creates or update tiles to keep track of what is missing.
    	 */
    	/** @ignore */
    	setupTiles() {
    		if (!this.shader || !this.layout || this.layout.status != 'ready')
    			return;

    		for (let tile of this.tiles) {
    			tile.missing = this.shader.samplers.length;
    			for (let sampler of this.shader.samplers) {
    				if (tile.tex[sampler.id])
    					tile.missing--;
    			}
    		}
    	}

    	/** @ignore */
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

    		if (this.shader.needsUpdate) {
    			this.shader.debug = this.debug;
    			this.shader.createProgram(gl);
    		}

    		gl.useProgram(this.shader.program);
    		this.shader.updateUniforms(gl);
    	}

    	/** @ignore */
    	sameNeeded(a, b) {
    		if (a.level != b.level)
    			return false;

    		for (let p of ['xLow', 'xHigh', 'yLow', 'yHigh'])
    			if (a.pyramid[a.level][p] != b.pyramid[a.level][p])
    				return false;

    		return true;
    	}

    	/**
    	 * Initiates tile prefetching based on viewport
    	 * @param {Transform} transform - Current view transform
    	 * @param {Object} viewport - Current viewport
    	 * @private
    	 */
    	prefetch(transform, viewport) {
    		if (this.viewport)
    			viewport = this.viewport;

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

    		/*let needed = this.layout.needed(viewport, transform, this.prefetchBorder, this.mipmapBias, this.tiles);


    		this.queue = [];
    		let now = performance.now();
    		let missing = this.shader.samplers.length;


    		for(let tile of needed) {
    			if(tile.missing === null)
    				tile.missing = missing;
    			if (tile.missing != 0 && !this.requested[index])
    				tmp.push(tile);
    		} */
    		this.queue = this.layout.needed(viewport, transform, this.transform, this.prefetchBorder, this.mipmapBias, this.tiles);
    		/*		let needed = this.layout.neededBox(viewport, transform, this.prefetchBorder, this.mipmapBias);
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
    				}*/
    		Cache.setCandidates(this);
    	}

    	/**
    	 * Loads a specific tile
    	 * @param {Object} tile - Tile specification
    	 * @param {Function} callback - Completion callback
    	 * @returns {Promise<void>}
    	 * @private
    	 */
    	async loadTile(tile, callback) {
    		if (this.tiles.has(tile.index))
    			throw "AAARRGGHHH double tile!";

    		if (this.requested.has(tile.index)) {
    			console.log("Warning: double request!");
    			callback("Double tile request");
    			return;
    		}

    		this.tiles.set(tile.index, tile);
    		this.requested.set(tile.index, true);

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
    				tile.w = img.width;
    				tile.h = img.height;
    				i++;
    			}
    			tile.missing = 0;
    			this.emit('update');
    			this.requested.delete(tile.index);
    			if (callback) callback(tile.size);
    			return;
    		}
    		tile.missing = this.shader.samplers.length;
    		for (let sampler of this.shader.samplers) {

    			let raster = this.rasters[sampler.id];
    			tile.url = this.layout.getTileURL(sampler.id, tile);
    			const [tex, size] = await raster.loadImage(tile, this.gl); // TODO Parallelize request and url must be a parameter (implement request ques per url)
    			if (this.layout.type == "image") {
    				this.layout.width = raster.width;
    				this.layout.height = raster.height;
    				this.layout.emit('updateSize');
    			}
    			tile.size += size;
    			tile.tex[sampler.id] = tex;
    			tile.missing--;
    			if (tile.missing <= 0) {
    				this.emit('update');
    				this.requested.delete(tile.index);
    				if (this.requested.size == 0)
    					this.emit('loaded');
    				if (callback) callback(size);
    			}
    		}
    	}
    }

    Layer.prototype.types = {};
    addSignals(Layer, 'ready', 'update', 'loaded', 'updateSize');

    //// HELPERS

    window.structuredClone = typeof (structuredClone) == "function" ? structuredClone : function (value) { return JSON.parse(JSON.stringify(value)); };


    /**
     * Canvas class that manages WebGL context, layers, and scene rendering.
     * Handles layer management, WebGL context creation/restoration, and render timing.
     */
    class Canvas {
    	/**
    	 * Creates a new Canvas instance with WebGL context and overlay support.
    	 * @param {HTMLCanvasElement|string} canvas - Canvas DOM element or selector
    	 * @param {HTMLElement|string} overlay - Overlay DOM element or selector for decorations (annotations, glyphs)
    	 * @param {Camera} camera - Scene camera instance
    	 * @param {Object} [options] - Configuration options
    	 * @param {Object} [options.layers] - Layer configurations mapping layer IDs to Layer instances
    	 * @param {boolean} [options.preserveDrawingBuffer=false] - Whether to preserve WebGL buffers until manually cleared
    	 * @param {number} [options.targetfps=30] - Target frames per second for rendering
    	 * @fires Canvas#update
    	 * @fires Canvas#updateSize
    	 * @fires Canvas#ready
    	 */
    	constructor(canvas, overlay, camera, options) {
    		Object.assign(this, {
    			canvasElement: null,
    			preserveDrawingBuffer: false,
    			gl: null,
    			overlayElement: null,
    			camera: camera,
    			layers: {},
    			targetfps: 30,
    			fps: 0,
    			timing: [16], //records last 30 frames time from request to next draw, rolling, primed to avoid /0
    			timingLength: 5, //max number of timings.
    			overBudget: 0, //fraction of frames that took too long to render.

    			signals: { 'update': [], 'updateSize': [], 'ready': [] }
    		});
    		Object.assign(this, options);

    		this.init(canvas, overlay);

    		for (let id in this.layers)
    			this.addLayer(id, new Layer(this.layers[id]));
    		this.camera.addEvent('update', () => this.emit('update'));
    	}

    	/**
    	 * Records render timing information and updates FPS statistics.
    	 * @param {number} elapsed - Time elapsed since last frame in milliseconds
    	 * @private
    	 */
    	addRenderTiming(elapsed) {
    		this.timing.push(elapsed);
    		while (this.timing.length > this.timingLength)
    			this.timing.shift();
    		this.overBudget = this.timing.filter(t => t > 1000 / this.targetfps).length / this.timingLength;
    		this.fps = 1000 / (this.timing.reduce((sum, a) => sum + a, 0) / this.timing.length);
    	}

    	/**
    	 * Initializes WebGL context and sets up event listeners.
    	 * @param {HTMLCanvasElement|string} canvas - Canvas element or selector
    	 * @param {HTMLElement|string} overlay - Overlay element or selector
    	 * @throws {Error} If canvas or overlay elements cannot be found or initialized
    	 * @private
    	 */
    	init(canvas, overlay) {
    		if (!canvas)
    			throw "Missing element parameter"

    		if (typeof (canvas) == 'string') {
    			canvas = document.querySelector(canvas);
    			if (!canvas)
    				throw "Could not find dom element.";
    		}

    		if (!overlay)
    			throw "Missing element parameter"

    		if (typeof (overlay) == 'string') {
    			overlay = document.querySelector(overlay);
    			if (!overlay)
    				throw "Could not find dom element.";
    		}

    		if (!canvas.tagName)
    			throw "Element is not a DOM element"

    		if (canvas.tagName != "CANVAS")
    			throw "Element is not a canvas element";

    		this.canvasElement = canvas;
    		this.overlayElement = overlay;

    		/* test context loss */
    		/* canvas = WebGLDebugUtils.makeLostContextSimulatingCanvas(canvas);
    		canvas.loseContextInNCalls(1000); */


    		let glopt = { antialias: false, depth: false, preserveDrawingBuffer: this.preserveDrawingBuffer };
    		this.gl = this.gl ||
    			canvas.getContext("webgl2", glopt) ||
    			canvas.getContext("webgl", glopt) ||
    			canvas.getContext("experimental-webgl", glopt);

    		if (!this.gl)
    			throw "Could not create a WebGL context";

    		canvas.addEventListener("webglcontextlost", (event) => { console.log("Context lost."); event.preventDefault(); }, false);
    		canvas.addEventListener("webglcontextrestored", () => { this.restoreWebGL(); }, false);
    		document.addEventListener("visibilitychange", (event) => { if (this.gl.isContextLost()) { this.restoreWebGL(); } });
    	}

    	/**
    	* Updates the state of the canvas and its components.
    	* @param {Object} state - State object containing updates
    	* @param {Object} [state.camera] - Camera state updates
    	* @param {Object} [state.layers] - Layer state updates
    	* @param {number} dt - Animation duration in milliseconds
    	* @param {string} [easing='linear'] - Easing function for animations
    	*/
    	setState(state, dt, easing = 'linear') {
    		if ('camera' in state) {
    			const m = state.camera;
    			this.camera.setPosition(dt, m.x, m.y, m.z, m.a, easing);
    		}
    		if ('layers' in state)
    			for (const [k, layerState] of Object.entries(state.layers))
    				if (k in this.layers) {
    					const layer = this.layers[k];
    					layer.setState(layerState, dt, easing);
    				}
    	}

    	/**
    	* Retrieves current state of the canvas and its components.
    	* @param {Object} [stateMask=null] - Optional mask to filter returned state properties
    	* @returns {Object} Current state object
    	*/
    	getState(stateMask = null) {
    		let state = {};
    		if (!stateMask || stateMask.camera) {
    			let now = performance.now();
    			let m = this.camera.getCurrentTransform(now);
    			state.camera = { 'x': m.x, 'y': m.y, 'z': m.z, 'a': m.a };
    		}
    		state.layers = {};
    		for (let layer of Object.values(this.layers)) {
    			const layerMask = window.structuredClone(stateMask);
    			if (stateMask && stateMask.layers) Object.assign(layerMask, stateMask.layers[layer.id]);
    			state.layers[layer.id] = layer.getState(layerMask);
    		}
    		return state;
    	}

    	/**
     * Restores WebGL context after loss.
     * Reinitializes shaders and textures for all layers.
     * @private
     */
    	restoreWebGL() {
    		let glopt = { antialias: false, depth: false, preserveDrawingBuffer: this.preserveDrawingBuffer };
    		this.gl = this.gl ||
    			this.canvasElement.getContext("webgl2", glopt) ||
    			this.canvasElement.getContext("webgl", glopt) ||
    			this.canvasElement.getContext("experimental-webgl", glopt);

    		for (let layer of Object.values(this.layers)) {
    			layer.gl = this.gl;
    			layer.clear();
    			if (layer.shader)
    				layer.shader.restoreWebGL(this.gl);
    		}
    		this.prefetch();
    		this.emit('update');
    	}

    	/**
    	 * Adds a layer to the canvas.
    	 * @param {string} id - Unique identifier for the layer
    	 * @param {Layer} layer - Layer instance to add
    	 * @fires Canvas#update
    	 * @fires Canvas#ready
    	 * @throws {Error} If layer ID already exists
    	 */
    	addLayer(id, layer) {

    		console.assert(!(id in this.layers), "Duplicated layer id");

    		layer.id = id;
    		layer.addEvent('ready', () => {
    			if (Object.values(this.layers).every(l => l.status == 'ready'))
    				this.emit('ready');
    			this.prefetch();
    		});
    		layer.addEvent('update', () => { this.emit('update'); });
    		layer.addEvent('updateSize', () => { this.updateSize(); });
    		layer.gl = this.gl;
    		layer.canvas = this;
    		layer.overlayElement = this.overlayElement;
    		this.layers[id] = layer;
    		this.prefetch();
    	}

    	/**
     * Removes a layer from the canvas.
     * @param {Layer} layer - Layer instance to remove
     * @example
     * const layer = new Layer(options);
     * canvas.addLayer('map', layer);
     * // ... later ...
     * canvas.removeLayer(layer);
     */
    	removeLayer(layer) {
    		layer.clear(); //order is important.

    		delete this.layers[layer.id];
    		delete Cache.layers[layer];
    		this.prefetch();
    	}

    	/**
    	 * Updates canvas size and camera bounds based on layers.
    	 * @fires Canvas#updateSize
    	 * @private
    	 */
    	updateSize() {
    		const discardHidden = false;
    		let sceneBBox = Layer.computeLayersBBox(this.layers, discardHidden);
    		let minScale = Layer.computeLayersMinScale(this.layers, discardHidden);

    		if (sceneBBox != null && this.camera.viewport)
    			this.camera.updateBounds(sceneBBox, minScale);
    		this.emit('updateSize');
    	}

    	/**
     * Renders a frame at the specified time.
     * @param {number} time - Current time in milliseconds
     * @returns {boolean} True if all animations are complete
     * @private
     */
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
    		let ordered = Object.values(this.layers).sort((a, b) => a.zindex - b.zindex);

    		//NOTICE: camera(pos) must be relative to the WHOLE canvas
    		let done = true;
    		for (let layer of ordered) {
    			if (layer.visible)
    				done = layer.draw(pos, view) && done;
    		}

    		//TODO not really an elegant solution to tell if we have reached the target, the check should be in getCurrentTransform.
    		return done && pos.t >= this.camera.target.t;
    	}

    	/**
     * Schedules tile downloads based on current view.
     * @param {Object} [transform] - Optional transform override, defaults to current camera transform
     * @private
     */
    	prefetch(transform) {
    		if (!transform)
    			transform = this.camera.getGlCurrentTransform(performance.now());
    		for (let id in this.layers) {
    			let layer = this.layers[id];
    			//console.log(layer);
    			//console.log(layer.layout.status);
    			if (layer.visible && layer.status == 'ready') {
    				layer.prefetch(transform, this.camera.glViewport());
    			}
    		}
    	}
    }

    /**
     * Fired when canvas content is updated (layer changes, camera moves).
     * @event Canvas#update
     */

    /**
     * Fired when canvas or layout size changes.
     * @event Canvas#updateSize
     */

    /**
     * Fired when all layers are initialized and ready to display.
     * @event Canvas#ready
     */

    addSignals(Canvas, 'update', 'updateSize', 'ready');

    /**
     * Represents a cubic spline interpolation for smooth color transitions.
     * @private
     */
    class Spline {
    	/**
    	 * Creates a new Spline instance.
    	 * @param {number[]} xs - X coordinates array
    	 * @param {number[]} ys - Y coordinates array
    	 */
    	constructor(xs, ys) {
    		this.xs = xs;
    		this.ys = ys;
    		this.ks = this.getNaturalKs(new Float64Array(this.xs.length));
    	}

    	getNaturalKs(ks) {
    		const n = this.xs.length - 1;
    		const A = Spline.zerosMat(n + 1, n + 2);
    		for (let i = 1; i < n; i++ // rows
    		) {
    			A[i][i - 1] = 1 / (this.xs[i] - this.xs[i - 1]);
    			A[i][i] =
    				2 *
    				(1 / (this.xs[i] - this.xs[i - 1]) + 1 / (this.xs[i + 1] - this.xs[i]));
    			A[i][i + 1] = 1 / (this.xs[i + 1] - this.xs[i]);
    			A[i][n + 1] =
    				3 *
    				((this.ys[i] - this.ys[i - 1]) /
    					((this.xs[i] - this.xs[i - 1]) * (this.xs[i] - this.xs[i - 1])) +
    					(this.ys[i + 1] - this.ys[i]) /
    					((this.xs[i + 1] - this.xs[i]) * (this.xs[i + 1] - this.xs[i])));
    		}
    		A[0][0] = 2 / (this.xs[1] - this.xs[0]);
    		A[0][1] = 1 / (this.xs[1] - this.xs[0]);
    		A[0][n + 1] =
    			(3 * (this.ys[1] - this.ys[0])) /
    			((this.xs[1] - this.xs[0]) * (this.xs[1] - this.xs[0]));
    		A[n][n - 1] = 1 / (this.xs[n] - this.xs[n - 1]);
    		A[n][n] = 2 / (this.xs[n] - this.xs[n - 1]);
    		A[n][n + 1] =
    			(3 * (this.ys[n] - this.ys[n - 1])) /
    			((this.xs[n] - this.xs[n - 1]) * (this.xs[n] - this.xs[n - 1]));
    		return Spline.solve(A, ks);
    	}

    	/**
     * Finds index of the point before the target value using binary search.
     * Inspired by https://stackoverflow.com/a/40850313/4417327
     * @param {number} target - Value to search for
     * @returns {number} Index of the point before target
     * @private
     */
    	getIndexBefore(target) {
    		let low = 0;
    		let high = this.xs.length;
    		let mid = 0;
    		while (low < high) {
    			mid = Math.floor((low + high) / 2);
    			if (this.xs[mid] < target && mid !== low) {
    				low = mid;
    			}
    			else if (this.xs[mid] >= target && mid !== high) {
    				high = mid;
    			}
    			else {
    				high = low;
    			}
    		}
    		if (low === this.xs.length - 1) {
    			return this.xs.length - 1;
    		}
    		return low + 1;
    	}

    	/**
     * Calculates interpolated value at given point.
     * @param {number} x - Point to interpolate at
     * @returns {number} Interpolated value
     */
    	at(x) {
    		let i = this.getIndexBefore(x);
    		const t = (x - this.xs[i - 1]) / (this.xs[i] - this.xs[i - 1]);
    		const a = this.ks[i - 1] * (this.xs[i] - this.xs[i - 1]) -
    			(this.ys[i] - this.ys[i - 1]);
    		const b = -this.ks[i] * (this.xs[i] - this.xs[i - 1]) +
    			(this.ys[i] - this.ys[i - 1]);
    		const q = (1 - t) * this.ys[i - 1] +
    			t * this.ys[i] +
    			t * (1 - t) * (a * (1 - t) + b * t);
    		return q;
    	}

    	// Utilities 

    	static solve(A, ks) {
    		const m = A.length;
    		let h = 0;
    		let k = 0;
    		while (h < m && k <= m) {
    			let i_max = 0;
    			let max = -Infinity;
    			for (let i = h; i < m; i++) {
    				const v = Math.abs(A[i][k]);
    				if (v > max) {
    					i_max = i;
    					max = v;
    				}
    			}
    			if (A[i_max][k] === 0) {
    				k++;
    			}
    			else {
    				Spline.swapRows(A, h, i_max);
    				for (let i = h + 1; i < m; i++) {
    					const f = A[i][k] / A[h][k];
    					A[i][k] = 0;
    					for (let j = k + 1; j <= m; j++)
    						A[i][j] -= A[h][j] * f;
    				}
    				h++;
    				k++;
    			}
    		}
    		for (let i = m - 1; i >= 0; i-- // rows = columns
    		) {
    			var v = 0;
    			if (A[i][i]) {
    				v = A[i][m] / A[i][i];
    			}
    			ks[i] = v;
    			for (let j = i - 1; j >= 0; j-- // rows
    			) {
    				A[j][m] -= A[j][i] * v;
    				A[j][i] = 0;
    			}
    		}
    		return ks;
    	}

    	static zerosMat(r, c) {
    		const A = [];
    		for (let i = 0; i < r; i++)
    			A.push(new Float64Array(c));
    		return A;
    	}

    	static swapRows(m, k, l) {
    		let p = m[k];
    		m[k] = m[l];
    		m[l] = p;
    	}
    }


    /**
     * Represents a color in RGBA format with values normalized between 0 and 1.
     */
    class Color {
    	/**
    	 * Creates a new Color instance.
    	 * @param {number|string} r - Red component [0.0, 1.0] or color string ('#RGB', '#RGBA', '#RRGGBB', '#RRGGBBAA', 'rgb()', 'rgba()')
    	 * @param {number} [g] - Green component [0.0, 1.0]
    	 * @param {number} [b] - Blue component [0.0, 1.0]
    	 * @param {number} [a] - Alpha component [0.0, 1.0]
    	 * @throws {Error} If string value is not a valid color format
    	 */
    	constructor(r, g = undefined, b = undefined, a = undefined) {
    		if (typeof (r) == 'string') {
    			if (/^#([A-Fa-f0-9]{3}){1,2}$/.test(r)) {
    				let c = r.substring(1).split('');
    				if (c.length == 3) {
    					c = [c[0], c[0], c[1], c[1], c[2], c[2]];
    				}
    				c = '0x' + c.join('') + 'FF';
    				r = Color.normalizedRGBA(c >> 24);
    				g = Color.normalizedRGBA(c >> 16);
    				b = Color.normalizedRGBA(c >> 8);
    				a = Color.normalizedRGBA(c);
    			} else if (/^#([A-Fa-f0-9]{4}){1,2}$/.test(r)) {
    				let c = r.substring(1).split('');
    				c = '0x' + c.join('');
    				r = Color.normalizedRGBA(c >> 24);
    				g = Color.normalizedRGBA(c >> 16);
    				b = Color.normalizedRGBA(c >> 8);
    				a = Color.normalizedRGBA(c);
    			} else if (/^rgb\(/.test(r)) {
    				let c = r.split("(")[1].split(")")[0];
    				c = c.split(',');
    				r = Color.clamp(c[0] / 255);
    				g = Color.clamp(c[1] / 255);
    				b = Color.clamp(c[2] / 255);
    				a = 1.0;
    			} else if (/^rgba\(/.test(r)) {
    				let c = r.split("(")[1].split(")")[0];
    				c = c.split(',');
    				r = Color.clamp(c[0] / 255);
    				g = Color.clamp(c[1] / 255);
    				b = Color.clamp(c[2] / 255);
    				a = Color.clamp(c[3] / 255);
    			} else {
    				throw Error("Value is not a color");
    			}
    		}
    		this.r = r;
    		this.g = g;
    		this.b = b;
    		this.a = a;
    	}

    	static clamp = (num, min = 0.0, max = 1.0) => Math.min(Math.max(num, min), max);

    	static hex(c) {
    		var hex = c.toString(16).toUpperCase();
    		return hex.length == 1 ? "0" + hex : hex;
    	}

    	static normalizedRGBA(c) {
    		return Color.clamp((c & 255) / 255);
    	}

    	static rgbToHex(r, g, b) {
    		const rgb = b | (g << 8) | (r << 16);
    		return '#' + ((0x1000000 | rgb).toString(16).substring(1)).toUpperCase();
    	}

    	static rgbToHexa(r, g, b, a) {
    		return '#' + Color.hex(r) + Color.hex(g) + Color.hex(b) + Color.hex(a);
    	}

    	/**
    	 * Gets color components as an array.
    	 * @returns {number[]} Array of [r, g, b, a] values
    	 */

    	value() {
    		return [this.r, this.g, this.b, this.a];
    	}

    	/**
    	 * Converts color to RGB values [0, 255].
    	 * @returns {number[]} Array of [r, g, b] values
    	 */
    	toRGB() {
    		const rgb = [this.r * 255, this.g * 255, this.b * 255];
    		rgb.forEach((e, idx, arr) => {
    			arr[idx] = Color.clamp(Math.round(e), 0, 255);
    		});
    		return rgb;
    	}

    	/**
    	 * Converts color to hexadecimal string.
    	 * @returns {string} Color in '#RRGGBB' format
    	 */
    	toHex() {
    		const rgb = this.toRGB();
    		return Color.rgbToHex(rgb[0], rgb[1], rgb[2]);
    	}

    	/**
    	 * Converts color to hexadecimal string with alpha.
    	 * @returns {string} Color in '#RRGGBBAA' format
    	 */
    	toHexa() {
    		const rgba = this.toRGBA();
    		return Color.rgbToHexa(rgba[0], rgba[1], rgba[2], rgba[3]);
    	}

    	/**
    	 * Converts color to RGBA values [0-255].
    	 * @returns {number[]} Array of [r, g, b, a] values
    	 */
    	toRGBA() {
    		const rgba = [this.r * 255, this.g * 255, this.b * 255, this.a * 255];
    		rgba.forEach((e, idx, arr) => {
    			arr[idx] = Color.clamp(Math.round(e), 0, 255);
    		});
    		return rgba;
    	}
    }

    /**
     * Creates a colormap for mapping numerical values to colors.
     * Supports linear, spline, and bar interpolation between colors.
     */
    class Colormap {
    	/**
    	 * Creates a new Colormap instance.
    	 * @param {Color[]} [colors=[black, white]] - Array of colors to interpolate between
    	 * @param {Object} [options] - Configuration options
    	 * @param {number[]} [options.domain=[0,1]] - Domain range for mapping
    	 * @param {Color} [options.lowColor] - Color for values below domain (defaults to first color)
    	 * @param {Color} [options.highColor] - Color for values above domain (defaults to last color)
    	 * @param {string} [options.description=''] - Description of the colormap
    	 * @param {('linear'|'spline'|'bar')} [options.type='linear'] - Interpolation type
    	 * @throws {Error} If colors/domain format is invalid
    	 */
    	constructor(colors = [new Color(0, 0, 0, 1), new Color(1, 1, 1, 1)], options = '') {
    		options = Object.assign({
    			domain: [0.0, 1.0],
    			lowColor: null,
    			highColor: null,
    			description: '',
    			type: 'linear'
    		}, options);
    		Object.assign(this, options);
    		const nval = colors.length;

    		if (!this.lowColor) this.lowColor = colors[0];
    		if (!this.highColor) this.highColor = colors[nval - 1];

    		const nd = this.domain.length;
    		if (nval < 2 && nd != 2 && this.nval != nd && this.domain[nd - 1] <= this.domain[0]) {
    			throw Error("Colormap colors/domain bad format");
    		}

    		const delta = (this.domain[nd - 1] - this.domain[0]) / (nval - 1);
    		this.xarr = [];
    		this.rarr = [];
    		this.garr = [];
    		this.barr = [];
    		this.aarr = [];
    		for (let i = 0; i < nval; i++) {
    			if (nd == 2)
    				this.xarr.push(this.domain[0] + i * delta);
    			else
    				this.xarr.push(this.domain[i]);
    			this.rarr.push(colors[i].r);
    			this.garr.push(colors[i].g);
    			this.barr.push(colors[i].b);
    			this.aarr.push(colors[i].a);
    		}
    		this.rspline = new Spline(this.xarr, this.rarr);
    		this.gspline = new Spline(this.xarr, this.garr);
    		this.bspline = new Spline(this.xarr, this.barr);
    		this.aspline = new Spline(this.xarr, this.aarr);
    	}

    	static clamp = (num, min, max) => Math.min(Math.max(num, min), max);

    	/**
    	 * Gets the domain range of the colormap.
    	 * @returns {number[]} Array containing [min, max] of domain
    	 */
    	rangeDomain() {
    		return [this.domain[0], this.domain[this.domain.length - 1]];
    	}

    	/**
    	 * Gets color for a value using bar interpolation.
    	 * @param {number} x - Value to get color for
    	 * @returns {Color} Corresponding color
    	 * @private
    	 */
    	bar(x) {
    		if (x < this.xarr[0]) return this.lowColor;
    		if (x > this.xarr[this.xarr.length - 1]) return this.highColor;
    		const c = new Color(this.rarr[0], this.garr[0], this.barr[0], this.aarr[0]);
    		for (let i = 0; i < this.xarr.length - 1; i++) {
    			if (x > this.xarr[i] && x <= this.xarr[i + 1]) {
    				c.r = this.rarr[i];
    				c.g = this.garr[i];
    				c.b = this.barr[i];
    				c.a = this.aarr[i];
    			}
    		}
    		return c;
    	}

    	/**
    	 * Gets color for a value using linear interpolation.
    	 * @param {number} x - Value to get color for
    	 * @returns {Color} Corresponding color
    	 * @private
    	 */
    	linear(x) {
    		if (x < this.xarr[0]) return this.lowColor;
    		if (x > this.xarr[this.xarr.length - 1]) return this.highColor;
    		const c = new Color(this.rarr[0], this.garr[0], this.barr[0], this.aarr[0]);
    		for (let i = 0; i < this.xarr.length - 1; i++) {
    			if (x > this.xarr[i] && x <= this.xarr[i + 1]) {
    				c.r = (this.rarr[i + 1] - this.rarr[i]) * (x - this.xarr[i]) / (this.xarr[i + 1] - this.xarr[i]) + this.rarr[i];
    				c.g = (this.garr[i + 1] - this.garr[i]) * (x - this.xarr[i]) / (this.xarr[i + 1] - this.xarr[i]) + this.garr[i];
    				c.b = (this.barr[i + 1] - this.barr[i]) * (x - this.xarr[i]) / (this.xarr[i + 1] - this.xarr[i]) + this.barr[i];
    				c.a = (this.aarr[i + 1] - this.aarr[i]) * (x - this.xarr[i]) / (this.xarr[i + 1] - this.xarr[i]) + this.aarr[i];
    			}
    		}
    		return c;
    	}

    	/**
    	 * Gets color for a value using spline interpolation.
    	 * @param {number} x - Value to get color for
    	 * @returns {Color} Corresponding color
    	 * @private
    	 */
    	spline(x) {
    		if (x < this.xarr[0]) return this.lowColor;
    		if (x > this.xarr[this.xarr.length - 1]) return this.highColor;
    		return new Color(this.rspline.at(x), this.gspline.at(x), this.bspline.at(x), this.aspline.at(x));
    	}

    	/**
    	 * Gets color for a value using configured interpolation type.
    	 * @param {number} x - Value to get color for
    	 * @returns {Color} Corresponding color
    	 * @throws {Error} If interpolation type is invalid
    	 */
    	at(x) {
    		let result = null;
    		switch (this.type) {
    			case 'linear':
    				result = this.linear(x);
    				break;
    			case 'spline':
    				result = this.spline(x);
    				break;
    			case 'bar':
    				result = this.bar(x);
    				break;
    			default:
    				throw Error("Interpolant type not exist");
    		}
    		return result;
    	}

    	/**
    	 * Samples the colormap into a buffer.
    	 * @param {number} maxSteps - Number of samples to generate
    	 * @returns {{min: number, max: number, buffer: Uint8Array}} Sample data and buffer
    	 */
    	sample(maxSteps) {
    		let min = this.xarr[0];
    		let max = this.xarr[this.xarr.length - 1];
    		//if (this.domain.length == 2) maxSteps = this.xarr.length;
    		let buffer = new Uint8Array(maxSteps * 4);
    		let delta = (max - min) / maxSteps;
    		for (let i = 0; i < maxSteps; i++) {
    			let c = this.at(min + i * delta).toRGBA();
    			buffer[i * 4 + 0] = c[0];
    			buffer[i * 4 + 1] = c[1];
    			buffer[i * 4 + 2] = c[2];
    			buffer[i * 4 + 3] = c[3];
    		}
    		return { min, max, buffer };
    	}
    }

    /**
     * Creates a visual legend for a colormap.
     */
    class ColormapLegend {
    	/**
    	 * Creates a new ColormapLegend instance.
    	 * @param {Object} viewer - Viewer instance to attach legend to
    	 * @param {Colormap} colorscale - Colormap to create legend for
    	 * @param {Object} [options] - Configuration options
    	 * @param {number} [options.nticks=6] - Number of ticks/divisions in legend
    	 * @param {number} [options.legendWidth=25] - Width of legend as percentage
    	 * @param {string} [options.textColor='#fff'] - Color of text labels
    	 * @param {string} [options.class='openlime-legend'] - CSS class for legend container
    	 */
    	constructor(viewer, colorscale, options) {
    		options = Object.assign({
    			nticks: 6,
    			legendWidth: 25,
    			textColor: '#fff',
    			class: 'openlime-legend'
    		}, options);
    		Object.assign(this, options);
    		this.viewer = viewer;
    		this.colorscale = colorscale;

    		this.container = document.querySelector(`.${this.class}`);
    		if (!this.container) {
    			this.container = document.createElement('div');
    			this.container.classList.add(this.class);
    		}

    		this.scale = document.createElement('div');
    		this.scale.style = `display: flex; border-radius: 20px; height: 22px; color: ${this.textColor}; 
		font-weight: bold; overflow: hidden; margin: 0px 2px 4px 0px; background-color: #7c7c7c; 
		font-family: Arial,Helvetica,sans-serif; font-size:12px;
		border: 1px solid #000;`;
    		this.container.appendChild(this.scale);
    		this.viewer.containerElement.appendChild(this.container);

    		const domain = colorscale.rangeDomain();
    		const legend = document.createElement('div');
    		legend.style = `display: flex; align-items: center; justify-content: center; 
		background: ${colorscale.linear(domain[0]).toHex()}; width: ${this.legendWidth}%; margin: 0`;
    		legend.textContent = colorscale.description;
    		this.scale.appendChild(legend);

    		if (this.colorscale.type == 'linear') this.legendLinear();
    		if (this.colorscale.type == 'bar') this.legendBar();
    	}

    	/**
    	 * Creates legend for linear interpolation.
    	 * @private
    	 */
    	legendLinear() {
    		const domain = this.colorscale.rangeDomain();
    		const delta = (domain[1] - domain[0]) / this.nticks;
    		const deltaWidth = (100 - this.legendWidth) / this.nticks;
    		let vl = domain[0];
    		for (let i = 0; i < this.nticks; i++) {
    			let v = domain[0] + delta * i;
    			let vr = i < (this.nticks - 1) ? domain[0] + delta * (i + 0.5) : v;
    			const c = this.colorscale.at(v);
    			const cl = this.colorscale.at(vl);
    			const cr = this.colorscale.at(vr);
    			const value = document.createElement('div');
    			const bkg = `background: linear-gradient(to right, ${cl.toHex()}, ${c.toHex()}, ${cr.toHex()})`;
    			value.style = `display: flex; align-items: center; justify-content: center; 
			${bkg};	width: ${deltaWidth}%; margin: 0`;
    			value.textContent = v.toFixed(1);
    			this.scale.appendChild(value);
    			vl = vr;
    		}
    	}

    	/**
    	 * Creates legend for bar interpolation.
    	 * @private
    	 */
    	legendBar() {
    		const deltaWidth = (100 - this.legendWidth) / this.colorscale.domain.length;
    		for (let i = 0; i < this.colorscale.xarr.length; i++) {
    			const c = new Color(this.colorscale.rarr[i], this.colorscale.garr[i], this.colorscale.barr[i], this.colorscale.aarr[i]);
    			const v = this.colorscale.xarr[i];
    			const value = document.createElement('div');
    			const bkg = `background: ${c.toHex()}`;
    			value.style = `display: flex; align-items: center; justify-content: center; 
			${bkg};	width: ${deltaWidth}%; margin: 0`;
    			value.textContent = v.toFixed(1);
    			this.scale.appendChild(value);
    		}
    	}

    }

    /*
    * @fileoverview 
    * Raster module provides functionality for loading and managing image data in various formats.
    * Supports multiple color formats and handles both local and remote image loading with CORS support.
    */

    /**
    * @typedef {('vec3'|'vec4'|'float')} Raster#Format
    * Defines the color format for image data storage in textures and renderbuffers.
    * @property {'vec3'} vec3 - RGB format (3 components without alpha)
    * @property {'vec4'} vec4 - RGBA format (4 components with alpha)
    * @property {'float'} float - Single-channel format for coefficient data
    */

    /**
    * Raster class handles image loading and texture creation for OpenLIME.
    * Provides functionality for:
    * - Loading images from URLs or blobs
    * - Converting images to WebGL textures
    * - Handling different color formats
    * - Supporting partial content requests
    * - Managing CORS requests
    * - Creating mipmaps for large textures
    */
    class Raster {
    	/**
    	 * Creates a new Raster instance.
    	 * @param {Object} [options] - Configuration options
    	 * @param {Raster#Format} [options.format='vec3'] - Color format for image data:
    	 *   - 'vec3' for RGB images
    	 *   - 'vec4' for RGBA images
    	 *   - 'float' for coefficient data
    	 */
    	constructor(options) {

    		Object.assign(this, {
    			format: 'vec3',
    		});

    		Object.assign(this, options);
    	}

    	/**
    	 * Loads an image tile and converts it to a WebGL texture.
    	 * Supports both full and partial content requests.
    	 * @async
    	 * @param {Object} tile - The tile to load
    	 * @param {string} tile.url - URL of the image
    	 * @param {number} [tile.start] - Start byte for partial requests
    	 * @param {number} [tile.end] - End byte for partial requests
    	 * @param {WebGLRenderingContext} gl - The WebGL rendering context
    	 * @returns {Promise<Array>} Promise resolving to [texture, size]:
    	 *   - texture: WebGLTexture object
    	 *   - size: Size of the image in bytes (width * height * components)
    	 * @throws {Error} If server doesn't support partial content requests when required
    	 */
    	async loadImage(tile, gl) {
    		let img;
    		let cors = (new URL(tile.url, window.location.href)).origin !== window.location.origin;
    		if (tile.end || typeof createImageBitmap == 'undefined') {
    			let options = {};
    			options.headers = { range: `bytes=${tile.start}-${tile.end}`, 'Accept-Encoding': 'indentity', mode: cors ? 'cors' : 'same-origin' };
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
    			if (cors) img.crossOrigin = "";
    			img.onerror = function (e) { console.log("Texture loading error!"); };
    			img.src = tile.url;
    			await new Promise((resolve, reject) => {
    				img.onload = () => { resolve(); };
    			});
    		}
    		let tex = this.loadTexture(gl, img);
    		//TODO 3 is not accurate for type of image, when changing from rgb to grayscale, fix this value.
    		let size = img.width * img.height * 3;
    		return [tex, size];
    	}

    	/**
    	 * Converts a Blob to an Image or ImageBitmap.
    	 * Handles browser-specific differences in image orientation.
    	 * @private
    	 * @async
    	 * @param {Blob} blob - Image data as Blob
    	 * @param {WebGLRenderingContext} gl - The WebGL rendering context
    	 * @returns {Promise<HTMLImageElement|ImageBitmap>} Promise resolving to the image
    	 */
    	async blobToImage(blob, gl) {
    		let img;
    		if (typeof createImageBitmap != 'undefined') {
    			var isFirefox = typeof InstallTrigger !== 'undefined';
    			//firefox does not support options for this call, BUT the image is automatically flipped.
    			if (isFirefox)
    				img = await createImageBitmap(blob);
    			else
    				img = await createImageBitmap(blob, { imageOrientation1: 'flipY' });

    		} else { //fallback for IOS
    			let urlCreator = window.URL || window.webkitURL;
    			img = document.createElement('img');
    			img.onerror = function (e) { console.log("Texture loading error!"); };
    			img.src = urlCreator.createObjectURL(blob);

    			await new Promise((resolve, reject) => { img.onload = () => resolve(); });
    			urlCreator.revokeObjectURL(img.src);

    		}
    		return img;
    	}

    	/**
    	 * Creates a WebGL texture from an image.
    	 * Handles different color formats and automatically creates mipmaps for large textures.
    	 * @private
    	 * @param {WebGLRenderingContext} gl - The WebGL rendering context
    	 * @param {HTMLImageElement|ImageBitmap} img - The source image
    	 * @returns {WebGLTexture} The created texture
    	 * 
    	 * @property {number} width - Width of the loaded image (set after loading)
    	 * @property {number} height - Height of the loaded image (set after loading)
    	 */
    	loadTexture(gl, img) {
    		this.width = img.width;  //this will be useful for layout image.
    		this.height = img.height;

    		var tex = gl.createTexture();
    		gl.bindTexture(gl.TEXTURE_2D, tex);

    		let glFormat = gl.RGBA;
    		switch (this.format) {
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
    		gl.texParameterf(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);

    		//build mipmap for large images.
    		if (this.width > 1024 || this.height > 1024) {
    			gl.generateMipmap(gl.TEXTURE_2D);
    			gl.texParameterf(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR_MIPMAP_LINEAR);
    		} else {
    			gl.texParameterf(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
    		}

    		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    		return tex;
    	}
    }

    /**
    * @typedef {Object} Shader~Sampler
    * A reference to a 2D texture used in the shader.
    * @property {number} id - Unique identifier for the sampler
    * @property {string} name - Sampler variable name in shader program (e.g., "kd" for diffuse texture)
    * @property {string} label - Display label for UI/menus
    * @property {Array<Object>} samplers - Array of raster definitions
    * @property {number} samplers[].id - Raster identifier
    * @property {string} samplers[].type - Raster type (e.g., 'color', 'normal')
    * @property {boolean} [samplers[].bind=true] - Whether sampler should be bound in prepareGL
    * @property {boolean} [samplers[].load=true] - Whether sampler should load from raster
    * @property {Array<Object>} uniforms - Shader uniform variables
    * @property {string} uniforms[].type - Data type ('vec4'|'vec3'|'vec2'|'float'|'int')
    * @property {boolean} uniforms[].needsUpdate - Whether uniform needs GPU update
    * @property {number} uniforms[].value - Uniform value or array of values
    */

    /**
     * Shader module provides WebGL shader program management for OpenLIME.
     * Supports both WebGL 1.0 and 2.0/3.0 GLSL specifications with automatic version detection.
     * 
     * Shader class manages WebGL shader programs.
     * Features:
     * - GLSL/ES 2.0 and 3.0 support
     * - Automatic uniform management
     * - Multiple shader modes
     * - Filter pipeline
     * - Automatic version selection
     */
    class Shader {
    	/**
    	 * Creates a new Shader instance.
    	 * @param {Object} [options] - Configuration options
    	 * @param {Array<Shader~Sampler>} [options.samplers=[]] - Texture sampler definitions
    	 * @param {Object.<string,Object>} [options.uniforms={}] - Shader uniform variables
    	 * @param {string} [options.label=null] - Display label for the shader
    	 * @param {Array<string>} [options.modes=[]] - Available shader modes
    	 * @param {number} [options.version=100] - GLSL version (100 for WebGL1, 300 for WebGL2)
    	 * @param {boolean} [options.debug=false] - Enable debug output
    	 * @fires Shader#update
    	 */
    	constructor(options) {
    		Object.assign(this, {
    			version: 100,   //check for webglversion. 
    			debug: false,
    			samplers: [],
    			uniforms: {},
    			label: null,
    			program: null,      //webgl program
    			modes: [],
    			mode: null, // The current mode
    			needsUpdate: true,
    			tileSize: [0, 0]
    		});
    		addSignals(Shader, 'update');
    		Object.assign(this, options);
    		this.filters = [];
    	}

    	/**
    	 * Clears all filters from the shader pipeline.
    	 * @fires Shader#update
    	 */
    	clearFilters() {
    		this.filters = [];
    		this.needsUpdate = true;
    		this.emit('update');
    	}

    	/**
    	 * Adds a filter to the shader pipeline.
    	 * @param {Object} filter - Filter to add
    	 * @fires Shader#update
    	 */
    	addFilter(f) {
    		f.shader = this;
    		this.filters.push(f);
    		this.needsUpdate = true;
    		f.needsUpdate = true;
    		this.emit('update');
    	}

    	/**
    	 * Removes a filter from the pipeline by name.
    	 * @param {string} name - Name of filter to remove
    	 * @fires Shader#update
    	 */
    	removeFilter(name) {
    		this.filters = this.filters.filter((v) => {
    			return v.name != name;
    		});
    		this.needsUpdate = true;
    		this.emit('update');
    	}

    	/**
    	 * Sets the current shader mode.
    	 * @param {string} mode - Mode identifier (must be in options.modes)
    	 * @throws {Error} If mode is not recognized
    	 */
    	setMode(mode) {
    		if (this.modes.indexOf(mode) == -1)
    			throw Error("Unknown mode: " + mode);
    		this.mode = mode;
    		this.needsUpdate = true;
    	}

    	/**
    	 * Restores WebGL state after context loss.
    	 * @param {WebGLRenderingContext} gl - WebGL context
    	 * @private
    	 */
    	restoreWebGL(gl) {
    		this.createProgram(gl);
    	}

    	/**
    	 * Sets tile dimensions for shader calculations.
    	 * @param {number[]} size - [width, height] of tile
    	 */
    	setTileSize(sz) {
    		this.tileSize = sz;
    		this.needsUpdate = true;
    	}

    	/**
    	 * Sets a uniform variable value.
    	 * @param {string} name - Uniform variable name
    	 * @param {number|boolean|Array} value - Value to set
    	 * @throws {Error} If uniform doesn't exist
    	 * @fires Shader#update
    	 */
    	setUniform(name, value) {
    		/**
    		* The event is fired when a uniform shader variable is changed.
    		* @event Camera#update
    		*/
    		let u = this.getUniform(name);
    		if (!u)
    			throw new Error(`Unknown '${name}'. It is not a registered uniform.`);
    		if ((typeof (value) == "number" || typeof (value) == "boolean") && u.value == value)
    			return;
    		if (Array.isArray(value) && Array.isArray(u.value) && value.length == u.value.length) {
    			let equal = true;
    			for (let i = 0; i < value.length; i++)
    				if (value[i] != u.value[i]) {
    					equal = false;
    					break;
    				}
    			if (equal)
    				return;
    		}

    		u.value = value;
    		u.needsUpdate = true;
    		this.emit('update');
    	}

    	/** @ignore */
    	completeFragShaderSrc(gl) {
    		let gl2 = !(gl instanceof WebGLRenderingContext);

    		let src = `${gl2 ? '#version 300 es' : ''}\n`;
    		src += `precision highp float;\n`;
    		src += `precision highp int;\n`;
    		src += `const vec2 tileSize = vec2(${this.tileSize[0]}.0, ${this.tileSize[1]}.0);\n`;
    		src += this.fragShaderSrc() + '\n';

    		for (let f of this.filters) {
    			src += `		// Filter: ${f.name}\n`;
    			src += f.fragModeSrc() + '\n';
    			src += f.fragSamplerSrc() + '\n';
    			src += f.fragUniformSrc() + '\n';
    			src += f.fragDataSrc() + '\n\n';
    		}

    		src += `
		${gl2 ? 'out' : ''} vec4 color;
		void main() { 
			color = data();
			`;
    		for (let f of this.filters) {
    			src += `color=${f.functionName()}(color);\n`;
    		}
    		src += `${gl2 ? '' : 'gl_FragColor = color;'}
		}`;
    		return src;
    	}

    	/**
    	 * Creates the WebGL shader program.
    	 * @param {WebGLRenderingContext} gl - WebGL context
    	 * @private
    	 * @throws {Error} If shader compilation or linking fails
    	 */
    	createProgram(gl) {

    		let vert = gl.createShader(gl.VERTEX_SHADER);
    		gl.shaderSource(vert, this.vertShaderSrc(gl));

    		gl.compileShader(vert);
    		let compiled = gl.getShaderParameter(vert, gl.COMPILE_STATUS);
    		if (!compiled) {
    			Util.printSrcCode(this.vertShaderSrc(gl));
    			console.log(gl.getShaderInfoLog(vert));
    			throw Error("Failed vertex shader compilation: see console log and ask for support.");
    		} else if (this.debug) {
    			Util.printSrcCode(this.vertShaderSrc(gl));
    		}

    		let frag = gl.createShader(gl.FRAGMENT_SHADER);
    		gl.shaderSource(frag, this.completeFragShaderSrc(gl));
    		gl.compileShader(frag);

    		if (this.program)
    			gl.deleteProgram(this.program);

    		let program = gl.createProgram();

    		gl.getShaderParameter(frag, gl.COMPILE_STATUS);
    		compiled = gl.getShaderParameter(frag, gl.COMPILE_STATUS);
    		if (!compiled) {
    			Util.printSrcCode(this.completeFragShaderSrc(gl));
    			console.log(gl.getShaderInfoLog(frag));
    			throw Error("Failed fragment shader compilation: see console log and ask for support.");
    		} else if (this.debug) {
    			Util.printSrcCode(this.completeFragShaderSrc(gl));
    		}
    		gl.attachShader(program, vert);
    		gl.attachShader(program, frag);
    		gl.linkProgram(program);

    		if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
    			var info = gl.getProgramInfoLog(program);
    			throw new Error('Could not compile WebGL program. \n\n' + info);
    		}

    		//sampler units;
    		for (let sampler of this.samplers)
    			sampler.location = gl.getUniformLocation(program, sampler.name);

    		// filter samplers
    		for (let f of this.filters)
    			for (let sampler of f.samplers)
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

    		for (let uniform of Object.values(this.allUniforms())) {
    			uniform.location = null;
    			uniform.needsUpdate = true;
    		}

    		for (let f of this.filters)
    			f.prepare(gl);

    	}

    	/**
    	 * Gets a uniform variable by name.
    	 * Searches both shader uniforms and filter uniforms.
    	 * @param {string} name - Uniform variable name
    	 * @returns {Object|undefined} Uniform object if found
    	 */
    	getUniform(name) {
    		let u = this.uniforms[name];
    		if (u) return u;
    		for (let f of this.filters) {
    			u = f.uniforms[name];
    			if (u) return u;
    		}
    		return u;
    	}

    	/**
    	 * Returns all uniform variables associated with the shader and its filters.
    	 * Combines uniforms from both the base shader and all active filters into a single object.
    	 * @returns {Object.<string, Object>} Combined uniform variables
    	 */
    	allUniforms() {
    		const result = this.uniforms;
    		for (let f of this.filters) {
    			Object.assign(result, f.uniforms);
    		}
    		return result;
    	}

    	/**
    	 * Updates all uniform values in the GPU.
    	 * @param {WebGLRenderingContext} gl - WebGL context
    	 * @private
    	 */
    	updateUniforms(gl) {
    		for (const [name, uniform] of Object.entries(this.allUniforms())) {
    			if (!uniform.location)
    				uniform.location = gl.getUniformLocation(this.program, name);

    			if (!uniform.location)  //uniform not used in program
    				continue;

    			if (uniform.needsUpdate) {
    				let value = uniform.value;
    				switch (uniform.type) {
    					case 'vec4': gl.uniform4fv(uniform.location, value); break;
    					case 'vec3': gl.uniform3fv(uniform.location, value); break;
    					case 'vec2': gl.uniform2fv(uniform.location, value); break;
    					case 'float': gl.uniform1f(uniform.location, value); break;
    					case 'int': gl.uniform1i(uniform.location, value); break;
    					case 'bool': gl.uniform1i(uniform.location, value); break;
    					case 'mat3': gl.uniformMatrix3fv(uniform.location, false, value); break;
    					case 'mat4': gl.uniformMatrix4fv(uniform.location, false, value); break;
    					default: throw Error('Unknown uniform type: ' + u.type);
    				}
    				uniform.needsUpdate = false;
    			}
    		}
    	}

    	/**
    	 * Gets vertex shader source code.
    	 * Default implementation provides basic vertex transformation and texture coordinate passing.
    	 * @param {WebGLRenderingContext} gl - WebGL context
    	 * @returns {string} Vertex shader source code
    	 */
    	vertShaderSrc(gl) {
    		let gl2 = !(gl instanceof WebGLRenderingContext);
    		return `${gl2 ? '#version 300 es' : ''}

precision highp float; 
precision highp int; 

uniform mat4 u_matrix;
${gl2 ? 'in' : 'attribute'} vec4 a_position;
${gl2 ? 'in' : 'attribute'} vec2 a_texcoord;

${gl2 ? 'out' : 'varying'} vec2 v_texcoord;

			void main() {
				gl_Position = u_matrix * a_position;
				v_texcoord = a_texcoord;
			} `;
    	}

    	/**
    	 * Gets fragment shader source code.
    	 * Must be overridden in derived classes for custom shading.
    	 * @param {WebGLRenderingContext} gl - WebGL context
    	 * @returns {string} Fragment shader source code
    	 * @virtual
    	 */
    	fragShaderSrc(gl) {
    		let gl2 = !(gl instanceof WebGLRenderingContext);
    		let str = `

uniform sampler2D kd;

${gl2 ? 'in' : 'varying'} vec2 v_texcoord;

vec4 data() {
	return texture${gl2 ? '' : '2D'}(kd, v_texcoord);
}
`;
    		return str;
    	}
    }

    /**
     * @typedef {Object} LayerImageOptions
     * @property {string} url - URL of the image to display (required)
     * @property {string|Layout} [layout='image'] - Layout format for image display
     * @property {string} [format='vec4'] - Image data format for WebGL processing
     * @property {string} [type='image'] - Must be 'image' when using Layer factory
     * @extends LayerOptions
     */

    /**
     * LayerImage provides fundamental image rendering capabilities in OpenLIME.
     * It serves as both a standalone layer for basic image display and a foundation
     * for more complex image-based layers.
     * 
     * Features:
     * - Single image rendering
     * - WebGL-based display
     * - Automatic format handling
     * - Layout system integration
     * - Shader-based processing
     * 
     * Technical Details:
     * - Uses WebGL textures for image data
     * - Supports various color formats (vec3, vec4)
     * - Integrates with OpenLIME layout system
     * - Manages raster data automatically
     * - Provides standard RGB shader by default
     * 
     * @extends Layer
     * 
     * @example
     * ```javascript
     * // Direct instantiation
     * const imageLayer = new OpenLIME.LayerImage({
     *   url: 'image.jpg',
     *   layout: 'image',
     *   format: 'vec4'
     * });
     * viewer.addLayer('main', imageLayer);
     * 
     * // Using Layer factory
     * const factoryLayer = new OpenLIME.Layer({
     *   type: 'image',
     *   url: 'image.jpg',
     *   layout: 'image'
     * });
     * viewer.addLayer('factory', factoryLayer);
     * ```
     */
    class LayerImage extends Layer {
    	/**
    	 * Creates a new LayerImage instance
    	 * @param {LayerImageOptions} options - Configuration options
    	 * @throws {Error} If rasters options is not empty (should be auto-configured)
    	 * @throws {Error} If no URL is provided and layout has no URLs
    	 */
    	constructor(options) {
    		super(options);

    		if (Object.keys(this.rasters).length != 0)
    			throw "Rasters options should be empty!";

    		if (this.url)
    			this.layout.setUrls([this.url]);
    		else if (this.layout.urls.length == 0)
    			throw "Missing options.url parameter";

    		const rasterFormat = this.format != null ? this.format : 'vec4';
    		let raster = new Raster({ format: rasterFormat }); //FIXME select format for GEO stuff

    		this.rasters.push(raster);


    		let shader = new Shader({
    			'label': 'Rgb',
    			'samplers': [{ id: 0, name: 'kd', type: rasterFormat }]
    		});

    		this.shaders = { 'standard': shader };
    		this.setShader('standard');
    	}
    }

    /**
     * Register this layer type with the Layer factory
     * @type {Function}
     * @private
     */
    Layer.prototype.types['image'] = (options) => { return new LayerImage(options); };

    /**
     * @typedef {Object} LayerCombinerOptions
     * @property {Layer[]} layers - Array of layers to be combined (required)
     * @property {Object.<string, Shader>} [shaders] - Map of available shaders
     * @property {string} [type='combiner'] - Must be 'combiner' when using Layer factory
     * @property {boolean} [visible=true] - Whether combined output is visible
     * @extends LayerOptions
     */

    /**
     * LayerCombiner provides functionality to combine multiple layers using framebuffer operations
     * and custom shaders. It enables complex visual effects by compositing layers in real-time
     * using GPU-accelerated rendering.
     * 
     * Features:
     * - Real-time layer composition
     * - Custom shader-based effects
     * - Framebuffer management
     * - Dynamic texture allocation
     * - Resolution-independent rendering
     * - GPU-accelerated compositing
     * 
     * Use Cases:
     * - Layer blending and mixing
     * - Image comparison tools
     * - Lens effects (see {@link LayerLens})
     * - Custom visual effects
     * - Multi-layer compositing
     * 
     * Technical Details:
     * - Creates framebuffers for each input layer
     * - Manages WebGL textures and resources
     * - Supports dynamic viewport resizing
     * - Handles shader-based composition
     * - Maintains proper resource cleanup
     * 
     * @extends Layer
     * 
     * @example
     * ```javascript
     * // Create two base layers
     * const baseLayer = new OpenLIME.Layer({
     *   type: 'image',
     *   url: 'base.jpg'
     * });
     * 
     * const overlayLayer = new OpenLIME.Layer({
     *   type: 'image',
     *   url: 'overlay.jpg'
     * });
     * 
     * // Create combiner with custom shader
     * const combiner = new OpenLIME.Layer({
     *   type: 'combiner',
     *   layers: [baseLayer, overlayLayer],
     *   visible: true
     * });
     * 
     * // Set up blend shader
     * const shader = new OpenLIME.ShaderCombiner();
     * shader.mode = 'blend';
     * combiner.shaders = { 'standard': shader };
     * combiner.setShader('standard');
     * 
     * // Add to viewer
     * viewer.addLayer('combined', combiner);
     * ```
     */
    class LayerCombiner extends Layer {
    	/**
    	 * Creates a new LayerCombiner instance
    	 * @param {LayerCombinerOptions} options - Configuration options
    	 * @throws {Error} If rasters option is not empty (rasters should be defined in source layers)
    	 */
    	constructor(options) {
    		super(options);

    		if (Object.keys(this.rasters).length != 0)
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

    	/**
    	 * Renders the combined layers using framebuffer operations
    	 * Handles framebuffer creation, layer rendering, and final composition
    	 * @param {Transform} transform - Current view transform
    	 * @param {Object} viewport - Current viewport parameters
    	 * @param {number} viewport.x - Viewport X position
    	 * @param {number} viewport.y - Viewport Y position
    	 * @param {number} viewport.dx - Viewport width
    	 * @param {number} viewport.dy - Viewport height
    	 * @param {number} viewport.w - Total width
    	 * @param {number} viewport.h - Total height
    	 * @throws {Error} If shader is not specified
    	 * @private
    	 */
    	draw(transform, viewport) {
    		for (let layer of this.layers)
    			if (layer.status != 'ready')
    				return;

    		if (!this.shader)
    			throw "Shader not specified!";

    		let w = viewport.dx;
    		let h = viewport.dy;

    		if (!this.framebuffers.length || this.layout.width != w || this.layout.height != h) {
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

    		for (let i = 0; i < this.layers.length; i++) {
    			gl.bindFramebuffer(gl.FRAMEBUFFER, this.framebuffers[i]);
    			gl.clear(gl.COLOR_BUFFER_BIT);
    			this.layers[i].draw(transform, { x: 0, y: 0, dx: w, dy: h, w: w, h: h });
    			gl.bindFramebuffer(gl.FRAMEBUFFER, null);
    		}

    		this.prepareWebGL();

    		for (let i = 0; i < this.layers.length; i++) {
    			gl.uniform1i(this.shader.samplers[i].location, i);
    			gl.activeTexture(gl.TEXTURE0 + i);
    			gl.bindTexture(gl.TEXTURE_2D, this.textures[i]);
    		}

    		this.updateTileBuffers(
    			new Float32Array([-1, -1, 0, -1, 1, 0, 1, 1, 0, 1, -1, 0]),
    			new Float32Array([0, 0, 0, 1, 1, 1, 1, 0]));
    		gl.drawElements(gl.TRIANGLES, 6, gl.UNSIGNED_SHORT, 0);
    	}

    	/**
    	 * Creates framebuffers and textures for layer composition
    	 * Initializes WebGL resources for each input layer
    	 * @private
    	 */
    	createFramebuffers() {
    		let gl = this.gl;
    		for (let i = 0; i < this.layers.length; i++) {
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
    	/**
    	 * Cleans up framebuffer and texture resources
    	 * Should be called when resizing or destroying the layer
    	 * @private
    	 */
    	deleteFramebuffers() {
    	}

    	/**
    	 * Computes combined bounding box of all input layers
    	 * @returns {BoundingBox} Combined bounding box
    	 * @override
    	 * @private
    	 */
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

    	/**
    	 * Computes minimum scale across all input layers
    	 * @returns {number} Combined scale factor
    	 * @override
    	 * @private
    	 */
    	scale() {
    		//Combiner ask the scale of all its children
    		//keeping the hidden, because they could be hidden, but revealed by the combiner
    		const discardHidden = false;
    		let scale = Layer.computeLayersMinScale(this.layers, discardHidden);
    		scale *= this.transform.z;
    		return scale;
    	}
    }

    /**
     * Register this layer type with the Layer factory
     * @type {Function}
     * @private
     */
    Layer.prototype.types['combiner'] = (options) => { return new LayerCombiner(options); };

    /**
     * Represents an annotation that can be drawn as an overlay on a canvas.
     * An annotation is a decoration (text, graphics element, glyph) that provides
     * additional information for interpreting underlying drawings.
     * Each annotation has a unique identifier and can contain various properties
     * such as description, category, drawing style, labels, etc.
     * 
     */
    class Annotation {
    	/**
    		* Creates a new Annotation instance.
    		* @param {Object} [options] - Configuration options for the annotation.
    		* @param {string} [options.id] - Unique identifier for the annotation. If not provided, a UUID will be generated.
    		* @param {string} [options.code] - A code identifier for the annotation.
    		* @param {string} [options.label=''] - Display label for the annotation.
    		* @param {string} [options.description] - HTML text containing a comprehensive description.
    		* @param {string} [options.class] - Category or classification of the annotation.
    		* @param {string} [options.target] - Target element or area this annotation refers to.
    		* @param {string} [options.svg] - SVG content for the annotation.
    		* @param {Object} [options.image] - Image data associated with the annotation.
    		* @param {Object} [options.region] - Region coordinates {x, y, w, h} for the annotation.
    		* @param {Object} [options.data={}] - Additional custom data for the annotation.
    		* @param {Object} [options.style] - Style configuration for rendering.
    		* @param {BoundingBox} [options.bbox] - Bounding box of the annotation.
    		* @param {boolean} [options.visible=true] - Visibility state of the annotation.
    		* @param {Object} [options.state] - State variables for the annotation.
    		* @param {boolean} [options.ready=false] - Indicates if SVG conversion is complete.
    		* @param {boolean} [options.needsUpdate=true] - Indicates if annotation needs updating.
    		* @param {boolean} [options.editing=false] - Indicates if annotation is being edited.
    		* @class
    		*/
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
    				image: null,
    				region: null,
    				data: {},
    				style: null,
    				bbox: null,
    				visible: true,
    				state: null,
    				ready: false, //already: converted to svg
    				needsUpdate: true,
    				editing: false,
    			},
    			options);
    		//TODO label as null is problematic, sort this issue.
    		if (!this.label) this.label = '';
    		this.elements = []; //assign options is not recursive!!!
    	}

    	/**
    		* Generates a UUID (Universally Unique Identifier) for annotation instances.
    		* @returns {string} A newly generated UUID.
    		* @private
    		*/
    	static UUID() {
    		return 'axxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function (c) {
    			var r = Math.random() * 16 | 0, v = c == 'x' ? r : (r & 0x3 | 0x8);
    			return v.toString(16);
    		});
    	}

    	/**
    	 * Calculates and returns the bounding box of the annotation based on its elements or region.
    	 * The coordinates are always relative to the top-left corner of the canvas.
    	 * @returns {BoundingBox} The calculated bounding box of the annotation.
    	 * If the annotation has no elements and no region, returns an empty bounding box.
    	 * If the annotation has a region but no elements, returns a bounding box based on the region.
    	 * If the annotation has elements, calculates the bounding box that encompasses all elements.
    	 */
    	getBBoxFromElements() {
    		let box = null;
    		if (!this.elements.length) {
    			if (this.region == null) {
    				box = new BoundingBox();
    			} else {
    				const r = this.region;
    				box = new BoundingBox({ xLow: r.x, yLow: r.y, xHigh: r.x + r.w, yHigh: r.y + r.h });
    			}
    		} else {
    			let { x, y, width, height } = this.elements[0].getBBox();
    			for (let shape of this.elements) {
    				const { sx, sy, swidth, sheight } = shape.getBBox();
    				x = Math.min(x, sx);
    				y = Math.min(x, sy);
    				width = Math.max(width + x, sx + swidth) - x;
    				height = Math.max(height + y, sy + sheight) - y;
    			}
    			box = new BoundingBox({ xLow: x, yLow: y, xHigh: x + width, yHigh: y + width });
    		}
    		return box;
    	}

    	/////////////////////////////////
    	/**
    	 * Creates an Annotation instance from a JSON-LD format string.
    	 * @param {Object} entry - The JSON-LD object representing an annotation.
    	 * @returns {Annotation} A new Annotation instance.
    	 * @throws {Error} If the entry is not a valid JSON-LD annotation or contains unsupported selectors.
    	 */
    	static fromJsonLd(entry) {
    		if (entry.type != 'Annotation')
    			throw "Not a jsonld annotation.";
    		let options = { id: entry.id };

    		let rename = { 'identifying': 'code', 'identifying': 'label', 'describing': 'description', 'classifying': 'class' };
    		for (let item of entry.body) {
    			let field = rename[item.purpose];
    			if (field)
    				options[field] = item.value;
    		}
    		let selector = entry.target && entry.target.selector;
    		if (selector) {
    			switch (selector.type) {
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

    	/**
    	* Converts the annotation to a JSON-LD format object.
     	* @returns {Object} A JSON-LD representation of the annotation including context, 
    	* id, type, body (with code, class, and description), and target selector information.
     	*/
    	toJsonLd() {
    		let body = [];
    		if (this.code !== null)
    			body.push({ type: 'TextualBody', value: this.code, purpose: 'indentifying' });
    		if (this.class !== null)
    			body.push({ type: 'TextualBody', value: this.class, purpose: 'classifying' });
    		if (this.description !== null)
    			body.push({ type: 'TextualBody', value: this.description, purpose: 'describing' });

    		({
    			"@context": "http://www.w3.org/ns/anno.jsonld",
    			id: this.id,
    			type: "Annotation",
    			body: body,
    			target: { selector: {} }
    		});
    		if (this.target)
    			target.selector.source = this.target;


    		if (this.element) {
    			var s = new XMLSerializer();
    			s.serializeToString(this.element);
    		}
    	}
    }

    /**
     * @typedef {Object} LayerAnnotationOptions
     * @property {string} [style] - CSS styles for annotation rendering
     * @property {string|Annotation[]} [annotations=[]] - URL of JSON annotation data or array of annotations
     * @property {boolean} [overlay=true] - Whether annotations render as overlay
     * @property {Set<string>} [selected=new Set()] - Set of selected annotation IDs
     * @property {Object} [annotationsListEntry=null] - UI entry for annotations list
     * @extends LayerOptions
     */

    /**
     * LayerAnnotation provides functionality for displaying and managing annotations overlaid on other layers.
     * It supports both local and remote annotation data, selection management, and UI integration.
     * 
     * Features:
     * - Display of text, graphics, and glyph annotations
     * - Remote annotation loading via JSON/HTTP
     * - Selection management
     * - Visibility toggling per annotation
     * - UI integration with annotation list
     * - Annotation event handling
     * 
     * The layer automatically handles:
     * - Annotation data loading and parsing
     * - UI synchronization
     * - Visibility states
     * - Selection states
     * - Event propagation
     * 
     * @extends Layer
     * @fires LayerAnnotation#selected - Fired when annotation selection changes, with selected annotation as parameter
     * @fires LayerAnnotation#loaded - Fired when annotations are loaded
     * @fires Layer#update - Inherited from Layer, fired when redraw needed
     * @fires Layer#ready - Inherited from Layer, fired when layer is ready
     * 
     * @example
     * ```javascript
     * // Create annotation layer from remote JSON
     * const annoLayer = new OpenLIME.LayerAnnotation({
     *   annotations: 'https://example.com/annotations.json',
     *   style: '.annotation { color: red; }',
     *   overlay: true
     * });
     * 
     * // Listen for selection changes
     * annoLayer.addEvent('selected', (annotation) => {
     *   console.log('Selected annotation:', annotation.label);
     * });
     * 
     * // Add to viewer
     * viewer.addLayer('annotations', annoLayer);
     * ```
     */
    class LayerAnnotation extends Layer { //FIXME CustomData Object template {name: { label: defaultValue: type:number,enum,string,boolean min: max: enum:[] }}
    	/**
    	 * Instantiates a LayerAnnotation object.
    	 * @param {Object} [options] An object literal with options that inherits from {@link Layer}.
    	 * @param {string} options.style Properties to style annotations.
    		 * @param {(string|Array)} options.annotations The URL of the annotation data (JSON file or HTTP GET Request to an annotation server) or an array of annotations.
    	 */
    	constructor(options) {
    		options = Object.assign({
    			// geometry: null,  //unused, might want to store here the quads/shapes for opengl rendering
    			style: null,    //straightforward for svg annotations, to be defined or opengl rendering
    			annotations: [],
    			selected: new Set,
    			overlay: true,
    			annotationsListEntry: null, //TODO: horrible name for the interface list of annotations
    		}, options);
    		super(options);

    		if (typeof (this.annotations) == "string") { //assumes it is an URL
    			(async () => { await this.loadAnnotations(this.annotations); })();
    		}
    	}

    	/**
    	 * Loads annotations from a URL
    	 * @param {string} url - URL to fetch annotations from (JSON format)
    	 * @fires LayerAnnotation#loaded
    	 * @fires Layer#update
    	 * @fires Layer#ready
    	 * @private
    	 * @async
    	 */
    	async loadAnnotations(url) {
    		const headers = new Headers();
    		headers.append('pragma', 'no-cache');
    		headers.append('cache-control', 'no-cache');
    		var response = await fetch(url, {
    			method: 'GET',
    			headers: headers,
    		});
    		if (!response.ok) {
    			this.status = "Failed loading " + this.url + ": " + response.statusText;
    			return;
    		}
    		this.annotations = await response.json();
    		if (this.annotations.status == 'error') {
    			alert("Failed to load annotations: " + this.annotations.msg);
    			return;
    		}
    		//this.annotations = this.annotations.map(a => '@context' in a ? Annotation.fromJsonLd(a): a);
    		this.annotations = this.annotations.map(a => new Annotation(a));
    		for (let a of this.annotations)
    			if (a.publish != 1)
    				a.visible = false;
    		//this.annotations.sort((a, b) => a.label.localeCompare(b.label));
    		if (this.annotationsListEntry)
    			this.createAnnotationsList();

    		this.emit('update');
    		this.status = 'ready';
    		this.emit('ready');
    		this.emit('loaded');
    	}

    	/**
    	 * Creates a new annotation and adds it to the layer
    	 * @param {Annotation} [annotation] - Optional pre-configured annotation
    	 * @returns {Annotation} The newly created annotation
    	 * @private
    	 */
    	newAnnotation(annotation) {
    		if (!annotation)
    			annotation = new Annotation();

    		this.annotations.push(annotation);
    		let html = this.createAnnotationEntry(annotation);
    		let template = document.createElement('template');
    		template.innerHTML = html.trim();

    		let list = this.annotationsListEntry.element.parentElement.querySelector('.openlime-list');
    		list.appendChild(template.content.firstChild);

    		this.clearSelected();
    		//this.setSelected(annotation);
    		return annotation;
    	}

    	/**
    	 * Creates the UI entry for the annotations list
    	 * @returns {Object} Configuration object for annotations list UI
    	 * @private
    	 */
    	annotationsEntry() {
    		return this.annotationsListEntry = {
    			html: '',
    			list: [], //will be filled later.
    			classes: 'openlime-annotations',
    			status: () => 'active',
    			oncreate: () => {
    				if (Array.isArray(this.annotations))
    					this.createAnnotationsList();
    			}
    		}
    	}

    	/**
    	 * Creates the complete annotations list UI
    	 * @private
    	 */
    	createAnnotationsList() {
    		let html = '';
    		for (let a of this.annotations) {
    			html += this.createAnnotationEntry(a);
    		}

    		let list = this.annotationsListEntry.element.parentElement.querySelector('.openlime-list');
    		list.innerHTML = html;
    		list.addEventListener('click', (e) => {
    			let svg = e.srcElement.closest('svg');
    			if (svg) {
    				let entry = svg.closest('[data-annotation]');
    				entry.classList.toggle('hidden');
    				let id = entry.getAttribute('data-annotation');
    				let anno = this.getAnnotationById(id);
    				anno.visible = !anno.visible;
    				anno.needsUpdate = true;
    				this.emit('update');
    			}

    			let id = e.srcElement.getAttribute('data-annotation');
    			if (id) {
    				this.clearSelected();
    				let anno = this.getAnnotationById(id);
    				this.setSelected(anno, true);
    			}
    		});
    	}

    	/**
    	 * Creates a single annotation entry for the UI
    	 * @param {Annotation} annotation - The annotation to create an entry for
    	 * @returns {string} HTML string for the annotation entry
    	 * @private
    	 */
    	createAnnotationEntry(a) {
    		return `<a href="#" data-annotation="${a.id}" class="openlime-entry ${a.visible == 0 ? 'hidden' : ''}">${a.label || ''}
			<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="openlime-eye"><path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path><circle cx="12" cy="12" r="3"></circle></svg>
			<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="openlime-eye-off"><path d="M17.94 17.94A10.07 10.07 0 0 1 12 20c-7 0-11-8-11-8a18.45 18.45 0 0 1 5.06-5.94M9.9 4.24A9.12 9.12 0 0 1 12 4c7 0 11 8 11 8a18.5 18.5 0 0 1-2.16 3.19m-6.72-1.07a3 3 0 1 1-4.24-4.24"></path><line x1="1" y1="1" x2="23" y2="23"></line></svg>
			</a>`;
    	}

    	/**
    	 * Retrieves an annotation by its ID
    	 * @param {string} id - Annotation identifier
    	 * @returns {Annotation|null} The found annotation or null if not found
    	 */
    	getAnnotationById(id) {
    		for (const anno of this.annotations)
    			if (anno.id == id)
    				return anno;
    		return null;
    	}

    	/**
    	 * Clears all annotation selections
    	 * @private
    	 */
    	clearSelected() {
    		this.annotationsListEntry.element.parentElement.querySelectorAll(`[data-annotation]`).forEach((e) => e.classList.remove('selected'));
    		this.selected.clear();
    	}

    	/**
    	 * Sets the selection state of an annotation
    	 * @param {Annotation} anno - The annotation to select/deselect
    	 * @param {boolean} [on=true] - Whether to select (true) or deselect (false)
    	 * @fires LayerAnnotation#selected
    	 */
    	setSelected(anno, on = true) {
    		this.annotationsListEntry.element.parentElement.querySelector(`[data-annotation="${anno.id}"]`).classList.toggle('selected', on);
    		if (on)
    			this.selected.add(anno.id);
    		else
    			this.selected.delete(anno.id);
    		this.emit('selected', anno);
    	}
    }

    addSignals(LayerAnnotation, 'selected', 'loaded');

    /*
     * @fileoverview
     * LayoutTileImages module provides management for collections of image tiles with associated regions.
     * This layout type is specialized for handling multiple independent image tiles, each with their own
     * position and dimensions, rather than a regular grid of tiles like LayoutTiles.
     */

    /*
     * @typedef {Object} TileDescriptor
     * Properties expected in tile descriptors:
     * @property {boolean} visible - Whether the tile should be rendered
     * @property {Object} region - Position and dimensions of the tile
     * @property {number} region.x - X coordinate of tile's top-left corner
     * @property {number} region.y - Y coordinate of tile's top-left corner
     * @property {number} region.w - Width of the tile
     * @property {number} region.h - Height of the tile
     * @property {string} image - URL or path to the tile's image
     * @property {number} [publish] - Publication status (1 = published)
     */

    /**
     * LayoutTileImages class manages collections of image tiles with associated regions.
     * Each tile represents an independent image with its own position and dimensions in the layout space.
     * Tiles can be individually shown or hidden and are loaded from annotation files or external descriptors.
     * @extends Layout
     */
    class LayoutTileImages extends Layout {
    	/**
    	 * Creates a new LayoutTileImages instance.
    	 * @param {string|null} url - URL to the annotation file containing tile descriptors, or null if descriptors will be set later
    	 * @param {string} type - The layout type (should be 'tile_images')
    	 * @param {Object} [options] - Configuration options inherited from Layout
    	 */
    	constructor(url, type, options) {
    		super(url, null, options);
    		this.setDefaults(type);
    		this.init(url, type, options);

    		// Contain array of records with at least visible,region,image (url of the image). 
    		// Can be also a pointer to annotation array set from outside with setTileDescriptors()
    		this.tileDescriptors = [];
    		this.box = new BoundingBox();

    		if (url != null) {
    			// Read data from annotation file
    			this.loadDescriptors(url);
    		}
    	}

    	/**
    	 * Gets the tile size. For this layout, tiles don't have a fixed size.
    	 * @returns {number[]} Returns [0, 0] as tiles have variable sizes
    	 */
    	getTileSize() {
    		return [0, 0];
    	}

    	/**
    	 * Loads tile descriptors from an annotation file.
    	 * @private
    	 * @async
    	 * @param {string} url - URL of the annotation file
    	 * @fires Layout#ready - When descriptors are loaded and processed
    	 * @fires Layout#updateSize - When bounding box is computed
    	 */
    	async loadDescriptors(url) {
    		// Load tile descriptors from annotation file
    		let response = await fetch(url);
    		if (!response.ok) {
    			this.status = "Failed loading " + url + ": " + response.statusText;
    			return;
    		}
    		this.tileDescriptors = await response.json();
    		if (this.tileDescriptors.status == 'error') {
    			alert("Failed to load annotations: " + this.tileDescriptors.msg);
    			return;
    		}
    		//this.annotations = this.annotations.map(a => '@context' in a ? Annotation.fromJsonLd(a): a);
    		this.tileDescriptors = this.tileDescriptors.map(a => new Annotation(a));
    		for (let a of this.tileDescriptors) {
    			if (a.publish != 1)
    				a.visible = false;
    		}
    		this.computeBoundingBox();
    		this.emit('updateSize');

    		if (this.path == null) {
    			this.setPathFromUrl(url);
    		}

    		this.status = 'ready';
    		this.emit('ready');
    	}

    	/**
    	 * Computes the bounding box containing all tile regions.
    	 * Updates the layout's box property to encompass all tile regions.
    	 * @private
    	 */
    	computeBoundingBox() {
    		this.box = new BoundingBox();
    		for (let a of this.tileDescriptors) {
    			let r = a.region;
    			let b = new BoundingBox({ xLow: r.x, yLow: r.y, xHigh: r.x + r.w, yHigh: r.y + r.h });
    			this.box.mergeBox(b);
    		}
    	}

    	/**
    	 * Gets the layout's bounding box.
    	 * @returns {BoundingBox} The bounding box containing all tile regions
    	 */
    	boundingBox() {
    		return this.box;
    	}

    	/**
    	 * Sets the base path for tile URLs based on the annotation file location.
    	 * @private
    	 * @param {string} url - URL of the annotation file
    	 */
    	setPathFromUrl(url) {
    		// Assume annotations in dir of annotation.json + /annot/
    		const myArray = url.split("/");
    		const N = myArray.length;
    		this.path = "";
    		for (let i = 0; i < N - 1; ++i) {
    			this.path += myArray[i] + "/";
    		}
    		this.getTileURL = (id, tile) => {
    			const url = this.path + '/' + this.tileDescriptors[tile.index].image;
    			return url;
    		};
    		//this.path += "/annot/";
    	}

    	/**
    	 * Sets tile descriptors programmatically instead of loading from a file.
    	 * @param {Annotation[]} tileDescriptors - Array of tile descriptors
    	 * @fires Layout#ready
    	 */
    	setTileDescriptors(tileDescriptors) {
    		this.tileDescriptors = tileDescriptors;

    		this.status = 'ready';
    		this.emit('ready');
    	}

    	/**
    	 * Gets the URL for a specific tile.
    	 * @param {number} id - Channel/raster ID
    	 * @param {TileObj} tile - Tile object
    	 * @returns {string} URL to fetch tile image
    	 */
    	getTileURL(id, tile) {
    		const url = this.path + '/' + this.tileDescriptors[id].image;
    		return url;
    	}

    	/**
    	 * Sets visibility for a specific tile.
    	 * @param {number} index - Index of the tile
    	 * @param {boolean} visible - Visibility state to set
    	 */
    	setTileVisible(index, visible) {
    		this.tileDescriptors[index].visible = visible;
    	}

    	/**
    	 * Sets visibility for all tiles.
    	 * @param {boolean} visible - Visibility state to set for all tiles
    	 */
    	setAllTilesVisible(visible) {
    		const N = this.tileCount();

    		for (let i = 0; i < N; ++i) {
    			this.tileDescriptors[i].visible = visible;
    		}
    	}

    	/**
    	 * Maps tile coordinates to a linear index.
    	 * In this layout, x directly maps to the index as tiles are stored in a flat list.
    	 * @param {number} level - Zoom level (unused in this layout)
    	 * @param {number} x - X coordinate (used as index)
    	 * @param {number} y - Y coordinate (unused in this layout)
    	 * @returns {number} Linear index of the tile
    	 */
    	index(level, x, y) {
    		// Map x to index (flat list)
    		return x;
    	}

    	/**
    	 * Gets coordinates for a tile in both image space and texture space.
    	 * @param Obj} tile - The tile to get coordinates for
    	 * @returns {Object} Coordinate data
    	 * @returns {Float32Array} .coords - Image space coordinates [x,y,z, x,y,z, x,y,z, x,y,z]
    	 * @returns {Float32Array} .tcoords - Texture coordinates [u,v, u,v, u,v, u,v]
    	 */
    	tileCoords(tile) {
    		const r = this.tileDescriptors[tile.index].region;
    		const x0 = r.x;
    		const y0 = r.y;
    		const x1 = x0 + r.w;
    		const y1 = y0 + r.h;

    		return {
    			coords: new Float32Array([x0, y0, 0, x0, y1, 0, x1, y1, 0, x1, y0, 0]),

    			//careful: here y is inverted due to textures not being flipped on load (Firefox fault!).
    			tcoords: new Float32Array([0, 1, 0, 0, 1, 0, 1, 1])
    		};
    	}

    	/**
    	 * Determines which tiles are needed for the current view.
    	 * @param {Viewport} viewport - Current viewport
    	 * @param {Transform} transform - Current transform
    	 * @param {Transform} layerTransform - Layer-specific transform
    	 * @param {number} border - Border size in viewport units
    	 * @param {number} bias - Resolution bias (unused in this layout)
    	 * @param {Map<number,Tile>} tiles - Currently available tiles
    	 * @param {number} [maxtiles=8] - Maximum number of tiles to return
    	 * @returns {TileObj[]} Array of needed tiles sorted by distance to viewport center
    	 */
    	needed(viewport, transform, layerTransform, border, bias, tiles, maxtiles = 8) {
    		//look for needed nodes and prefetched nodes (on the pos destination
    		const box = this.getViewportBox(viewport, transform, layerTransform);

    		let needed = [];
    		let now = performance.now();

    		// Linear scan of all the potential tiles
    		const N = this.tileCount();
    		const flipY = true;
    		for (let x = 0; x < N; x++) {
    			let index = this.index(0, x, 0);
    			let tile = tiles.get(index) || this.newTile(index);

    			if (this.intersects(box, index, flipY)) {
    				tile.time = now;
    				tile.priority = this.tileDescriptors[index].visible ? 10 : 1;
    				if (tile.missing === null)
    					needed.push(tile);
    			}
    		}
    		let c = box.center();
    		//sort tiles by distance to the center TODO: check it's correct!
    		needed.sort(function (a, b) { return Math.abs(a.x - c[0]) + Math.abs(a.y - c[1]) - Math.abs(b.x - c[0]) - Math.abs(b.y - c[1]); });

    		return needed;
    	}

    	/**
    	 * Gets tiles currently available for rendering.
    	 * @param {Viewport} viewport - Current viewport
    	 * @param {Transform} transform - Current transform
    	 * @param {Transform} layerTransform - Layer-specific transform
    	 * @param {number} border - Border size in viewport units
    	 * @param {number} bias - Resolution bias (unused in this layout)
    	 * @param {Map<number,Tile>} tiles - Available tiles
    	 * @returns {Object.<number,Tile>} Map of tile index to tile object for visible, loaded tiles
    	 */
    	available(viewport, transform, layerTransform, border, bias, tiles) {
    		//find box in image coordinates where (0, 0) is in the upper left corner.
    		const box = this.getViewportBox(viewport, transform, layerTransform);

    		let torender = [];

    		// Linear scan of all the potential tiles
    		const N = this.tileCount();
    		const flipY = true;
    		for (let x = 0; x < N; x++) {
    			let index = this.index(0, x, 0);

    			if (this.tileDescriptors[index].visible && this.intersects(box, index, flipY)) {
    				if (tiles.has(index)) {
    					let tile = tiles.get(index);
    					if (tile.missing == 0) {
    						torender[index] = tile;
    					}
    				}
    			}
    		}

    		return torender;
    	}

    	/**
    	 * Creates a new tile instance with properties from its descriptor.
    	 * @param {number} index - Index of the tile descriptor
    	 * @returns {TileObj} New tile instance with region and image properties
    	 */
    	newTile(index) {
    		let tile = new Tile();
    		tile.index = index;

    		let descriptor = this.tileDescriptors[index];
    		tile.image = descriptor.image;
    		Object.assign(tile, descriptor.region);
    		return tile;
    	}

    	/**
    	 * Checks if a tile's region intersects with a given box.
    	 * @private
    	 * @param {BoundingBox} box - Box to check intersection with
    	 * @param {number} index - Index of the tile to check
    	 * @param {boolean} [flipY=true] - Whether to flip Y coordinates for texture coordinate space
    	 * @returns {boolean} True if the tile intersects the box
    	 */
    	intersects(box, index, flipY = true) {
    		const r = this.tileDescriptors[index].region;
    		const xLow = r.x;
    		const yLow = r.y;
    		const xHigh = xLow + r.w;
    		const yHigh = yLow + r.h;
    		const boxYLow = flipY ? -box.yHigh : box.yLow;
    		const boxYHigh = flipY ? -box.yLow : box.yHigh;

    		return xLow < box.xHigh && yLow < boxYHigh && xHigh > box.xLow && yHigh > boxYLow;
    	}

    	/**
    	 * Gets the total number of tiles in the layout.
    	 * @returns {number} Number of tile descriptors
    	 */
    	tileCount() {
    		return this.tileDescriptors.length;
    	}

    }
    /**
     * @event Layout#ready
     * Fired when the layout is ready for rendering.
     * This occurs when:
     * - Tile descriptors are loaded from annotation file
     * - Tile descriptors are set programmatically
     */

    /**
     * @event Layout#updateSize
     * Fired when the layout size changes and scene extension needs updating.
     * This occurs when:
     * - Tile descriptors are loaded and bounding box is computed
     */

    // Register the tile_images layout type
    Layout.prototype.types['tile_images'] = (url, type, options) => { return new LayoutTileImages(url, type, options); };

    /**
     * @typedef {Object} LayerAnnotationImageOptions
     * @property {string} [url] - URL to the annotations JSON file
     * @property {string} [path] - Base path for annotation image files
     * @property {string} [format='vec4'] - Raster format for image data
     * @extends LayerAnnotationOptions
     */

    /**
     * LayerAnnotationImage extends LayerAnnotation to provide support for image-based annotations.
     * Each annotation corresponds to a single tile in the layer, with customizable visibility
     * and shader-based rendering.
     * 
     * Features:
     * - Image-based annotation rendering
     * - Per-annotation visibility control
     * - Custom shader support for image processing
     * - Automatic texture management
     * - WebGL/WebGL2 compatibility
     * - Multi-format raster support
     * 
     * The class handles:
     * - Image loading and caching
     * - Texture creation and binding
     * - Shader setup and compilation
     * - Tile visibility management
     * - WebGL state management
     * 
     * @extends LayerAnnotation
     * 
     * @example
     * ```javascript
     * // Create image annotation layer
     * const imageAnnoLayer = new OpenLIME.LayerAnnotationImage({
     *   url: 'annotations.json',
     *   path: './annotation-images/',
     *   format: 'vec4'
     * });
     * 
     * // Configure visibility
     * imageAnnoLayer.setAllTilesVisible(true);
     * imageAnnoLayer.setTileVisible(0, false); // Hide first annotation
     * 
     * // Add to viewer
     * viewer.addLayer('imageAnnotations', imageAnnoLayer);
     * ```
     */
    class LayerAnnotationImage extends LayerAnnotation {
        /**
     * Creates a new LayerAnnotationImage instance
     * @param {LayerAnnotationImageOptions} options - Configuration options
     * @throws {Error} If path is not specified (warns in console)
     */
        constructor(options) {
            const url = options.url;
            if (options.path == null) {
                console.log("WARNING MISSING ANNOTATION PATH, SET TO ./annot/");
            }
            super(options);
            const rasterFormat = this.format != null ? this.format : 'vec4';

            let initCallback = () => {
                // Set Annotation Urls path
                if (options.path) {
                    this.layout.path = options.path;
                } else if (url != null) {
                    // Extract path from annotation.json path
                    this.layout.setPathFromUrl(path);
                }

                for (let a of this.annotations) {
                    let raster = new Raster({ format: rasterFormat });
                    this.rasters.push(raster);
                }
                console.log("Set " + this.annotations.length + " annotations into layout");
                this.setupShader(rasterFormat);
                this.layout.setTileDescriptors(this.annotations);
            };
            this.addEvent('loaded', initCallback);
        }

        /**
         * Gets the number of annotations in the layer
         * @returns {number} Number of annotations
         */
        length() {
            return this.annotations.length;
        }

        /**
         * Sets visibility for a specific annotation/tile
         * @param {number} index - Index of the annotation
         * @param {boolean} visible - Whether the annotation should be visible
         */
        setTileVisible(index, visible) {
            this.layout.setTileVisible(index, visible);
            //this.annotations[index].needsUpdate = true;
            //this.emit('update');
        }

        /**
         * Sets visibility for all annotations/tiles
         * @param {boolean} visible - Whether all annotations should be visible
         */
        setAllTilesVisible(visible) {
            this.layout.setAllTilesVisible(visible);
            // for(let a of this.annotations) {
            //     a.needsUpdate = true;
            // }
            //this.emit('update');
        }

        /**
         * Renders a specific tile/annotation
         * @param {Object} tile - Tile object containing texture information
         * @param {number} index - Index of the tile
         * @throws {Error} If tile is missing textures
         * @private
         */
        drawTile(tile, index) {
            if (tile.missing != 0)
                throw "Attempt to draw tile still missing textures"

            const idx = tile.index;

            //coords and texture buffers updated once for all tiles from main draw() call

            //bind texture of this tile only (each tile corresponds to an image)
            let gl = this.gl;
            let id = this.shader.samplers[idx].id;
            gl.uniform1i(this.shader.samplers[idx].location, idx);
            gl.activeTexture(gl.TEXTURE0 + idx);
            gl.bindTexture(gl.TEXTURE_2D, tile.tex[id]);

            const byteOffset = this.getTileByteOffset(index);
            gl.drawElements(gl.TRIANGLES, 6, gl.UNSIGNED_SHORT, byteOffset);
        }

        /**
         * Configures the shader for rendering annotations
         * @param {string} rasterFormat - Format of the raster data ('vec4', etc)
         * @private
         */
        setupShader(rasterFormat) {
            let samplers = [];
            let N = this.rasters.length;
            for (let i = 0; i < N; ++i) {
                samplers.push({ id: i, name: 'kd', type: rasterFormat });
            }
            let shader = new Shader({
                'label': 'Rgb',
                'samplers': samplers //[{ id:0, name:'kd', type: rasterFormat }]
            });

            shader.fragShaderSrc = function (gl) {

                let gl2 = !(gl instanceof WebGLRenderingContext);
                let str = `

uniform sampler2D kd;

${gl2 ? 'in' : 'varying'} vec2 v_texcoord;

vec4 data() {
	return texture${gl2 ? '' : '2D'}(kd, v_texcoord);
}
`;
                return str;

            };

            this.shaders = { 'standard': shader };
            this.setShader('standard');
        }

    }

    Layer.prototype.types['annotation_image'] = (options) => { return new LayerAnnotationImage(options); };

    /**
     * @typedef {Object} LayerMaskedImageOptions
     * @property {string} url - URL of the masked image to display (required)
     * @property {string} [format='vec4'] - Image data format
     * @property {string} [type='maskedimage'] - Must be 'maskedimage' when using Layer factory
     * @extends LayerOptions
     */

    /**
     * LayerMaskedImage provides specialized handling for masked scalar images with bilinear interpolation.
     * It implements custom texture sampling and masking operations through WebGL shaders.
     * 
     * Features:
     * - Custom scalar image handling
     * - Bilinear interpolation with masking
     * - WebGL shader-based processing
     * - Support for both WebGL 1 and 2
     * - Nearest-neighbor texture filtering
     * - Masked value visualization
     * 
     * Technical Details:
     * - Uses LUMINANCE format for single-channel data
     * - Implements custom bilinear sampling in shader
     * - Handles mask values through alpha channel
     * - Supports value rescaling (255.0/254.0 scale with -1.0/254.0 bias)
     * - Uses custom texture parameters for proper sampling
     * 
     * Shader Implementation:
     * - Performs bilinear interpolation in shader
     * - Handles masked values (0 = masked)
     * - Implements value rescaling
     * - Provides visualization of masked areas (in red)
     * - Uses texelFetch for precise sampling
     * 
     * @extends Layer
     * 
     * @example
     * ```javascript
     * // Create masked image layer
     * const maskedLayer = new OpenLIME.Layer({
     *   type: 'maskedimage',
     *   url: 'masked-data.png',
     *   format: 'vec4'
     * });
     * 
     * // Add to viewer
     * viewer.addLayer('masked', maskedLayer);
     * ```
     */
    class LayerMaskedImage extends Layer {
    	/**
    	 * Creates a new LayerMaskedImage instance
    	 * @param {LayerMaskedImageOptions} options - Configuration options
    	 * @throws {Error} If rasters options is not empty
    	 * @throws {Error} If url is not provided and layout has no URLs
    	 */
    	constructor(options) {
    		super(options);

    		if (Object.keys(this.rasters).length != 0)
    			throw "Rasters options should be empty!";

    		if (this.url) {
    			this.layout.setUrls([this.url]);
    		} else if (this.layout.urls.length == 0)
    			throw "Missing options.url parameter";

    		const rasterFormat = this.format != null ? this.format : 'vec4';
    		let raster = new Raster({ format: rasterFormat }); //FIXME select format for GEO stuff

    		this.rasters.push(raster);

    		let shader = new Shader({
    			'label': 'Rgb',
    			'samplers': [{ id: 0, name: 'kd', type: rasterFormat }]
    		});

    		shader.fragShaderSrc = function (gl) {

    			let gl2 = !(gl instanceof WebGLRenderingContext);
    			let str = `
		
		uniform sampler2D kd;

		${gl2 ? 'in' : 'varying'} vec2 v_texcoord;

		vec2 bilinear_masked_scalar(sampler2D field, vec2 uv) {
			vec2 px = uv*tileSize;
			ivec2 iuv = ivec2(floor( px ));
			vec2 fuv = fract(px);
			int i0 = iuv.x;
			int j0 = iuv.y;
			int i1 = i0+1>=int(tileSize.x) ? i0 : i0+1;
			int j1 = j0+1>=int(tileSize.y) ? j0 : j0+1;
		  
			float f00 = texelFetch(field, ivec2(i0, j0), 0).r;
			float f10 = texelFetch(field, ivec2(i1, j0), 0).r;
			float f01 = texelFetch(field, ivec2(i0, j1), 0).r;
			float f11 = texelFetch(field, ivec2(i1, j1), 0).r;

			// FIXME Compute weights of valid
		  
			vec2 result_masked_scalar;
			result_masked_scalar.y = f00*f01*f10*f11;
			result_masked_scalar.y = result_masked_scalar.y > 0.0 ? 1.0 : 0.0;

			const float scale = 255.0/254.0;
			const float bias  = -1.0/254.0;
			result_masked_scalar.x = mix(mix(f00, f10, fuv.x), mix(f01, f11, fuv.x), fuv.y);
			result_masked_scalar.x = result_masked_scalar.y * (scale * result_masked_scalar.x + bias);		  
			return result_masked_scalar;
		  }
		  
		  vec4 data() { 
			vec2  masked_scalar = bilinear_masked_scalar(kd, v_texcoord);
			return masked_scalar.y > 0.0 ?  vec4(masked_scalar.x, masked_scalar.x, masked_scalar.x, masked_scalar.y) :  vec4(1.0, 0.0, 0.0, masked_scalar.y);
		  }
		`;
    			return str;

    		};

    		this.shaders = { 'scalarimage': shader };
    		this.setShader('scalarimage');

    		this.rasters[0].loadTexture = this.loadTexture.bind(this);
    		//this.layout.setUrls([this.url]);
    	}

    	/**
    	 * Renders the masked image
    	 * @param {Transform} transform - Current view transform
    	 * @param {Object} viewport - Current viewport
    	 * @returns {boolean} Whether render completed successfully
    	 * @override
    	 * @private
    	 */
    	draw(transform, viewport) {
    		return super.draw(transform, viewport);
    	}

    	/**
    	 * Custom texture loader for masked images
    	 * Sets up proper texture parameters for scalar data
    	 * 
    	 * @param {WebGLRenderingContext|WebGL2RenderingContext} gl - WebGL context
    	 * @param {HTMLImageElement} img - Source image
    	 * @returns {WebGLTexture} Created texture
    	 * @private
    	 */
    	loadTexture(gl, img) {
    		this.rasters[0].width = img.width;
    		this.rasters[0].height = img.height;

    		var tex = gl.createTexture();
    		gl.bindTexture(gl.TEXTURE_2D, tex);
    		gl.texParameterf(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
    		gl.texParameterf(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST); //_MIPMAP_LINEAR);
    		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    		gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);

    		// gl.texImage2D(gl.TEXTURE_2D, 0, gl.R16UI, gl.R16UI, gl.UNSIGNED_SHORT, img);
    		gl.texImage2D(gl.TEXTURE_2D, 0, gl.LUMINANCE, gl.LUMINANCE, gl.UNSIGNED_BYTE, img);
    		return tex;
    	}
    }

    /**
     * Register this layer type with the Layer factory
     * @type {Function}
     * @private
     */
    Layer.prototype.types['maskedimage'] = (options) => { return new LayerMaskedImage(options); };

    // Tile level x y  index ----- tex missing() start/end (tarzoom) ----- time, priority size(byte)

    /**
     * @typedef {'image'|'deepzoom'|'deepzoom1px'|'google'|'zoomify'|'iiif'|'iip'|'tarzoom'|'itarzoom'} Layout#Type
     * Supported image layout types including both single-resolution and multi-resolution formats.
     * - image: Standard web image formats (jpg, png, gif, etc.)
     * - deepzoom: Microsoft Deep Zoom format with root tile > 1px
     * - deepzoom1px: Microsoft Deep Zoom format with 1px root tile
     * - google: Google Maps tiling scheme
     * - zoomify: Zoomify tiling format
     * - iiif: International Image Interoperability Framework
     * - iip: Internet Imaging Protocol
     * - tarzoom: OpenLIME custom format (single TAR of DeepZoom pyramid)
     * - itarzoom: OpenLIME custom interleaved TAR format
     */

    /**
     * The Layout class is responsible for specifying the data formats (images) managed by OpenLIME.
     * All web single-resolution image types (*jpg*, *png*, *gif*, etc...) are supported as well as the most common 
     * tiled image formats (*deepzoom*, *zoomify*, *IIIF*, *google maps*), which are suitable for large images.
     * #### Single-resolution images
     * The URL is the address of the file (for instance, 'https://my.example/image.jpg').
     * #### Tiled images
     * They can be specified in a variety of ways depending on the format chosen.
     * * **deepzoom** - The root tile of the image pyramid has a size > 1px (typical value is 254px). It is defined by the URL of the *.dzi* file 
     * (for instance, 'https://my.example/image.dzi'). See: {@link https://docs.microsoft.com/en-us/previous-versions/windows/silverlight/dotnet-windows-silverlight/cc645077(v=vs.95)?redirectedfrom=MSDN DeepZoom}
     * * **deepzoom1px** - The root tile of the image pyramid has a size = 1px. It is defined by the URL of the *.dzi* file 
     * (for instance, 'https://my.example/image.dzi'). See: {@link https://docs.microsoft.com/en-us/previous-versions/windows/silverlight/dotnet-windows-silverlight/cc645077(v=vs.95)?redirectedfrom=MSDN DeepZoom}
     * * **google** - The URL points directly to the directory containing the pyramid of images (for instance, 'https://my.example/image'). 
     * The standard does not require any configuration file, so it is mandatory to indicate in the `options` the 
     * width and height in pixels of the original image. See: {@link https://www.microimages.com/documentation/TechGuides/78googleMapsStruc.pdf Google Maps}
     * * **zoomify** - The URL indicates the location of Zoomify configuration file (for instance, 'https://my.example/image/ImageProperties.xml').
     * See: {@link http://www.zoomify.com/ZIFFileFormatSpecification.htm Zoomify}
     * * **iip** - The server parameter (optional) indicates the URL of the IIPImage endpoint (for example '/fcgi-bin/iipsrv.fcgi').
     * The URL parameter indicates either just the name of the path and image file (for instance 'image.tif') if the server parameter has been set or the full IIP URL if not
     * (for instance '/fcgi-bin/iipsrv.fcgi?FIF=image.tif' or 'https://you.server//fcgi-bin/iipsrv.fcgi?FIF=image.tif' if image is hosted elsewhere)
     * See: {@link https://iipimage.sourceforge.io/ IIPImage Server}
     * * **iiif** - According to the standard, the URL is the address of a IIIF server (for instance, 'https://myiiifserver.example/').
     * See: {@link https://iiif.io/api/image/3.0/ IIIF }
     * * **tarzoom** and **itarzoom** - This is a custom format of the OpenLIME framework. It can be described as the TAR of a DeepZoom (all the DeepZoom image pyramid is stored in a single file).
     * It takes advantage of the fact that current web servers are able to handle partial-content HTTP requests. Tarzoom facilitates
     * the work of the server, which is not penalised by having to manage a file system with many small files. The URL is the address of the *.tzi* file 
     * (for instance, 'https://my.example/image.tzi'). Warning: tarzoom|itarzoom may not work on older web servers.
     * 
     * @extends Layout
     * 
     * @example
     * ```javascript
     * // DeepZoom layout
     * const dzLayout = new LayoutTiles('image.dzi', 'deepzoom', {
     *   cachelevels: 8
     * });
     * 
     * // Google Maps layout
     * const googleLayout = new LayoutTiles('tiles/', 'google', {
     *   width: 4096,
     *   height: 3072,
     *   suffix: 'png'
     * });
     * 
     * // IIIF layout
     * const iiifLayout = new LayoutTiles('https://server/image', 'iiif');
     * ```
     * 
     * @fires Layout#ready - When layout initialization is complete
     * @fires Layout#updateSize - When layout dimensions change 
    */
    class LayoutTiles extends Layout {
    	/**
    	 * Creates a new LayoutTiles instance.
    	 * @param {string} url - URL to the image or tile configuration
    	 * @param {Layout#Type} type - The type of image layout
    	 * @param {Object} [options] - Configuration options
    	 * @param {number} [options.width] - Width of original image (required for 'google' type)
    	 * @param {number} [options.height] - Height of original image (required for 'google' type)
    	 * @param {string} [options.suffix='jpg'] - Tile file extension
    	 * @param {string} [options.subdomains='abc'] - Available subdomains for Google URL template
    	 * @param {number} [options.cachelevels=10] - Number of levels above current to cache
    	 * @param {string} [options.server] - IIP server URL (for IIP type only)
    	 * @fires Layout#ready
    	 * @fires Layout#updateSize
    	 */
    	constructor(url, type, options) {
    		super(url, null, options);
    		this.setDefaults(type);
    		this.init(url, type, options);
    	}

    	/**
    	 * Sets default values for the layout.
    	 * @private
    	 * @param {Layout#Type} type - The layout type
    	 */
    	setDefaults(type) {
    		super.setDefaults(type);
    		Object.assign(this, {
    			tilesize: 256,
    			overlap: 0,
    			nlevels: 1,        //level 0 is the top, single tile level.
    			qbox: [],          //array of bounding box in tiles, one for mipmap 
    			bbox: [],          //array of bounding box in pixels (w, h)
    			urls: [],
    			cachelevels: 10,
    		});
    	}

    	/**
    	 * Configures URLs and initializes the layout based on type.
    	 * @private
    	 * @param {string[]} urls - Array of URLs to configure
    	 * @fires Layout#ready
    	 * @fires Layout#updateSize
    	 */
    	setUrls(urls) {
    		/**
    		* The event is fired when a layout is ready to be drawn(the single-resolution image is downloaded or the multi-resolution structure has been initialized).
    		* @event Layout#ready
    		*/
    		this.urls = urls;
    		(async () => {
    			switch (this.type) {
    				case 'google': await this.initGoogle(); break;        // No Url needed
    				case 'deepzoom1px': await this.initDeepzoom(true); break;  // urls[0] only needed
    				case 'deepzoom': await this.initDeepzoom(false); break; // urls[0] only needed
    				case 'zoomify': await this.initZoomify(); break;       // urls[0] only needed
    				case 'iiif': await this.initIIIF(); break;          // urls[0] only needed
    				case 'iip': await this.initIIP(); break;           // urls[0] only needed
    				case 'tarzoom': await this.initTarzoom(); break;       // all urls needed
    				case 'itarzoom': await this.initITarzoom(); break;      // actually it has just one url
    			}
    			this.initBoxes();
    			this.status = 'ready';
    			this.emit('ready');
    		})().catch(e => { console.log(e); this.status = e; });
    	}

    	/**
    	 * Constructs image URL for a specific plane/channel.
    	 * @private
    	 * @param {string} url - Base URL
    	 * @param {string} plane - Plane/channel identifier
    	 * @returns {string} Constructed URL
    	 * @throws {Error} For unknown layout types
    	 */
    	imageUrl(url, plane) {
    		let path = url.substring(0, url.lastIndexOf('/') + 1);
    		switch (this.type) {
    			case 'image': return path + plane + '.jpg';			case 'google': return path + plane;			case 'deepzoom': return path + plane + '.dzi';			case 'tarzoom': return path + plane + '.tzi';			case 'itarzoom': return path + 'planes.tzi';			case 'zoomify': return path + plane + '/ImageProperties.xml';			case 'iip': return url + "&SDS=" + plane.substring(plane.lastIndexOf('_') + 1, plane.length);			case 'iiif': throw Error("Unimplemented");
    			default: throw Error("Unknown layout: " + this.type);
    		}
    	}

    	/**
    	 * Gets the tile size for the layout.
    	 * @returns {number[]} Array containing [width, height] of tiles
    	 */
    	getTileSize() {
    		return [this.tilesize, this.tilesize];
    	}

    	/**
    	 * Generates unique index for a tile based on its position and level.
    	 * @param {number} level - Zoom level
    	 * @param {number} x - X coordinate
    	 * @param {number} y - Y coordinate
    	 * @returns {number} Unique tile index
    	 */
    	index(level, x, y) {
    		let startindex = 0;
    		for (let i = 0; i < level; i++)
    			startindex += this.qbox[i].xHigh * this.qbox[i].yHigh;
    		return startindex + y * this.qbox[level].xHigh + x;
    	}

    	/**
    	 * Converts tile index back to level, x, y coordinates.
    	 * @param {number} index - Tile index
    	 * @returns {Object} Position object containing level, x, y
    	 */
    	reverseIndex(index) {
    		let originalindex = index;
    		let level = 0;
    		for (let i = 0; i < this.qbox.length; i++) {
    			let size = this.qbox[i].xHigh * this.qbox[i].yHigh;
    			if (index - size < 0)
    				break;
    			index -= size;
    			level++;
    		}
    		let width = this.qbox[level].xHigh;
    		let y = Math.floor(index / width);
    		let x = index % width;
    		console.assert(this.index(level, x, y) == originalindex);
    		return { level, x, y };
    	}

    	/**
    	 * Initializes bounding boxes for all pyramid levels.
    	 * @private
    	 * @fires Layout#updateSize
    	 * @returns {number} Total number of tiles
    	 */
    	initBoxes() {
    		/**
    		* The event is fired when a layout size is modified (and the scene extension must be recomputed at canvas level).
    		* @event Layout#updateSize
    		*/

    		this.qbox = []; //by level (0 is the bottom)
    		this.bbox = [];
    		var w = this.width;
    		var h = this.height;

    		if (this.type == 'image') {
    			this.qbox[0] = new BoundingBox({ xLow: 0, yLow: 0, xHigh: 1, yHigh: 1 });
    			this.bbox[0] = new BoundingBox({ xLow: 0, yLow: 0, xHigh: w, yHigh: h });
    			// Acknowledge bbox change (useful for knowing scene extension (at canvas level))
    			this.emit('updateSize');
    			return 1;
    		}

    		for (let level = this.nlevels - 1; level >= 0; level--) {
    			this.qbox[level] = new BoundingBox({ xLow: 0, yLow: 0, xHigh: 0, yHigh: 0 });
    			this.bbox[level] = new BoundingBox({ xLow: 0, yLow: 0, xHigh: w, yHigh: h });

    			this.qbox[level].yHigh = Math.ceil(h / this.tilesize);
    			this.qbox[level].xHigh = Math.ceil(w / this.tilesize);

    			w >>>= 1;
    			h >>>= 1;
    		}
    		// Acknowledge bbox (useful for knowing scene extension (at canvas level))
    		this.emit('updateSize');
    	}

    	/**
    	 * Gets coordinates for a tile in both image space and texture space.
    	 * @param {TileObj} tile - The tile to get coordinates for
    	 * @returns {Object} Coordinate data
    	 * @returns {Float32Array} .coords - Image space coordinates [x,y,z, x,y,z, x,y,z, x,y,z]
    	 * @returns {Float32Array} .tcoords - Texture coordinates [u,v, u,v, u,v, u,v]
    	 */
    	tileCoords(tile) {
    		let { level, x, y } = tile;
    		this.width;
    		this.height;
    		//careful: here y is inverted due to textures not being flipped on load (Firefox fault!).
    		var tcoords = new Float32Array([0, 1, 0, 0, 1, 0, 1, 1]);
    		let coords = new Float32Array([0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0]); // FIXME 32 bit and errors

    		let ilevel = this.nlevels - 1 - level;
    		let side = this.tilesize * (1 << (ilevel)); //tile size in imagespace
    		let tx = side;
    		let ty = side;

    		if (side * (x + 1) > this.width) {
    			tx = (this.width - side * x);
    			if (this.type == 'google')
    				tcoords[4] = tcoords[6] = tx / side;
    		}

    		if (side * (y + 1) > this.height) {
    			ty = (this.height - side * y);
    			if (this.type == 'google')
    				tcoords[1] = tcoords[7] = ty / side;
    		}

    		var lx = this.qbox[level].xHigh - 1; //last tile x pos, if so no overlap.
    		var ly = this.qbox[level].yHigh - 1;

    		var over = this.overlap;
    		if (over) {
    			let dtx = over / (tx / (1 << ilevel) + (x == 0 ? 0 : over) + (x == lx ? 0 : over));
    			let dty = over / (ty / (1 << ilevel) + (y == 0 ? 0 : over) + (y == ly ? 0 : over));

    			tcoords[0] = tcoords[2] = (x == 0 ? 0 : dtx);
    			tcoords[3] = tcoords[5] = (y == 0 ? 0 : dty);
    			tcoords[4] = tcoords[6] = (x == lx ? 1 : 1 - dtx);
    			tcoords[1] = tcoords[7] = (y == ly ? 1 : 1 - dty);
    		}
    		//flip Y coordinates 
    		//TODO cleanup this mess!
    		let tmp = tcoords[1];
    		tcoords[1] = tcoords[7] = tcoords[3];
    		tcoords[3] = tcoords[5] = tmp;

    		for (let i = 0; i < coords.length; i += 3) {
    			coords[i] = coords[i] * tx + side * x - this.width / 2;
    			coords[i + 1] = -coords[i + 1] * ty - side * y + this.height / 2;
    		}

    		return { coords: coords, tcoords: tcoords }
    	}

    	/**
    	 * Creates a new tile instance with computed properties.
    	 * @param {number} index - Unique tile identifier
    	 * @returns {TileObj} New tile instance
    	 */
    	newTile(index) {
    		let tile = super.newTile(index);
    		tile.index = index;
    		Object.assign(tile, this.reverseIndex(index));
    		return tile;
    	}

    	/**
    	 * Determines which tiles are needed for the current view.
    	 * @param {Viewport} viewport - Current viewport
    	 * @param {Transform} transform - Current transform
    	 * @param {Transform} layerTransform - Layer-specific transform
    	 * @param {number} border - Border size in tile units for prefetching
    	 * @param {number} bias - Resolution bias (0-1, affects mipmap level selection)
    	 * @param {Map<number,Tile>} tiles - Currently available tiles
    	 * @param {number} [maxtiles=8] - Maximum number of tiles to return
    	 * @returns {TileObj[]} Array of needed tiles sorted by priority
    	 */
    	needed(viewport, transform, layerTransform, border, bias, tiles, maxtiles = 8) {
    		let neededBox = this.neededBox(viewport, transform, layerTransform, 0, bias);

    		//if (this.previouslyNeeded && this.sameNeeded(this.previouslyNeeded, neededBox))
    		//		return;
    		//this.previouslyNeeded = neededBox;

    		let needed = [];
    		let now = performance.now();
    		//look for needed nodes and prefetched nodes (on the pos destination
    		//let missing = this.shader.samplers.length;

    		for (let level = 0; level <= neededBox.level; level++) {
    			let box = neededBox.pyramid[level];
    			let tmp = [];
    			for (let y = box.yLow; y < box.yHigh; y++) {
    				for (let x = box.xLow; x < box.xHigh; x++) {
    					let index = this.index(level, x, y);
    					let tile = tiles.get(index) || this.newTile(index); //{ index, x, y, missing, tex: [], level };
    					tile.time = now;
    					tile.priority = neededBox.level - level;
    					if (tile.priority > this.cachelevels) continue;
    					if (tile.missing === null) // || tile.missing != 0 && !this.requested[index])
    						tmp.push(tile);
    				}
    			}
    			let c = box.center();
    			//sort tiles by distance to the center TODO: check it's correct!
    			tmp.sort(function (a, b) { return Math.abs(a.x - c.x) + Math.abs(a.y - c.y) - Math.abs(b.x - c.x) - Math.abs(b.y - c.y); });
    			needed = needed.concat(tmp);
    		}
    		return needed;
    	}

    	/**
    	 * Gets tiles currently available for rendering.
    	 * @param {Viewport} viewport - Current viewport
    	 * @param {Transform} transform - Current transform
    	 * @param {Transform} layerTransform - Layer-specific transform
    	 * @param {number} border - Border size in tile units
    	 * @param {number} bias - Resolution bias
    	 * @param {Map<number,Tile>} tiles - Available tiles
    	 * @returns {Object.<number,Tile>} Map of tile index to tile object with additional 'complete' property
    	 */
    	available(viewport, transform, layerTransform, border, bias, tiles) {
    		let needed = this.neededBox(viewport, transform, layerTransform, 0, bias);
    		let torender = {}; //array of minlevel, actual level, x, y (referred to minlevel)
    		let brothers = {};

    		let minlevel = needed.level;
    		let box = needed.pyramid[minlevel];

    		for (let y = box.yLow; y < box.yHigh; y++) {
    			for (let x = box.xLow; x < box.xHigh; x++) {
    				let level = minlevel;
    				while (level >= 0) {
    					let d = minlevel - level;
    					let index = this.index(level, x >> d, y >> d);
    					if (tiles.has(index) && tiles.get(index).missing == 0) {
    						torender[index] = tiles.get(index); //{ index: index, level: level, x: x >> d, y: y >> d, complete: true };
    						break;
    					} else {
    						let sx = (x >> (d + 1)) << 1;
    						let sy = (y >> (d + 1)) << 1;
    						brothers[this.index(level, sx, sy)] = 1;
    						brothers[this.index(level, sx + 1, sy)] = 1;
    						brothers[this.index(level, sx + 1, sy + 1)] = 1;
    						brothers[this.index(level, sx, sy + 1)] = 1;
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

    	/**
    	 * Computes required tiles at each pyramid level for the current view.
    	 * @param {Viewport} viewport - Current viewport
    	 * @param {Transform} transform - Current transform
    	 * @param {Transform} layerTransform - Layer-specific transform
    	 * @param {number} border - Border size in tile units for prefetching
    	 * @param {number} bias - Resolution bias (0-1)
    	 * @returns {Object} Tile requirements
    	 * @returns {number} .level - Optimal pyramid level
    	 * @returns {BoundingBox[]} .pyramid - Array of tile bounding boxes per level
    	 */
    	neededBox(viewport, transform, layerTransform, border, bias) {
    		if (this.type == "image")
    			return { level: 0, pyramid: [new BoundingBox({ xLow: 0, yLow: 0, xHigh: 1, yHigh: 1 })] };

    		//here we are computing with inverse levels; level 0 is the bottom!
    		let iminlevel = Math.max(0, Math.min(Math.floor(-Math.log2(transform.z) + bias), this.nlevels - 1));
    		let minlevel = this.nlevels - 1 - iminlevel;

    		const bbox = this.getViewportBox(viewport, transform, layerTransform);

    		let pyramid = [];
    		for (let level = 0; level <= minlevel; level++) {
    			let ilevel = this.nlevels - 1 - level;
    			let side = this.tilesize * Math.pow(2, ilevel);

    			let qbox = new BoundingBox(bbox);
    			qbox.quantize(side);

    			//clamp!
    			qbox.xLow = Math.max(qbox.xLow - border, this.qbox[level].xLow);
    			qbox.yLow = Math.max(qbox.yLow - border, this.qbox[level].yLow);
    			qbox.xHigh = Math.min(qbox.xHigh + border, this.qbox[level].xHigh);
    			qbox.yHigh = Math.min(qbox.yHigh + border, this.qbox[level].yHigh);
    			pyramid[level] = qbox;
    		}
    		return { level: minlevel, pyramid: pyramid };
    	}

    	/**
    	 * Gets URL for a specific tile.
    	 * @param {number} id - Channel/raster ID
    	 * @param {TileObj} tile - Tile to get URL for
    	 * @returns {string} URL to fetch tile data
    	 * @throws {Error} If layout not defined or ready
    	 */
    	getTileURL(id, tile) {
    		throw Error("Layout not defined or ready.");
    	}

    	/**
    	 * Initializes single-image layout.
    	 * @private
    	 * @async
    	 */
    	async initImage() {
    		this.getTileURL = (rasterid, tile) => { return this.urls[rasterid]; };
    		this.nlevels = 1;
    		this.tilesize = 0;
    	}

    	/**
    	 * Initializes Google Maps layout.
    	 * @private
    	 * @async
    	 * @throws {Error} If width or height not specified
    	 */
    	async initGoogle() {
    		if (!this.width || !this.height)
    			throw "Google rasters require to specify width and height";

    		this.tilesize = 256;
    		this.overlap = 0;

    		let max = Math.max(this.width, this.height) / this.tilesize;
    		this.nlevels = Math.ceil(Math.log(max) / Math.LN2) + 1;

    		if (this.urls[0].includes('{')) {
    			this.getTileURL = (rasterid, tile) => {
    				let s = this.subdomains ? this.subdomains[Math.abs(tile.x + tile.y) % this.subdomains.length] : '';
    				let vars = { s, ...tile, z: tile.level };
    				return this.urls[rasterid].replace(/{(.+?)}/g, (match, p) => vars[p]);
    			};
    		} else
    			this.getTileURL = (rasterid, tile) => {
    				return this.urls[rasterid] + "/" + tile.level + "/" + tile.y + "/" + tile.x + '.' + this.suffix;
    			};
    	}

    	/**
    	 * Initializes DeepZoom layout.
    	 * @private
    	 * @async
    	 * @param {boolean} onepixel - Whether using 1px root tile variant
    	 * @throws {Error} If unable to fetch or parse DZI file
    	 */
    	async initDeepzoom(onepixel) {
    		let url = this.urls.filter(u => u)[0];
    		var response = await fetch(url);
    		if (!response.ok) {
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

    		let max = Math.max(this.width, this.height) / this.tilesize;
    		this.nlevels = Math.ceil(Math.log(max) / Math.LN2) + 1;

    		this.urls = this.urls.map(url => url ? url.substr(0, url.lastIndexOf(".")) + '_files/' : null);
    		this.skiplevels = 0;
    		if (onepixel)
    			this.skiplevels = Math.ceil(Math.log(this.tilesize) / Math.LN2);

    		this.getTileURL = (rasterid, tile) => {
    			let url = this.urls[rasterid];
    			let level = tile.level + this.skiplevels;
    			return url + level + '/' + tile.x + '_' + tile.y + '.' + this.suffix;
    		};
    	}

    	/**
    	 * Initializes Tarzoom layout.
    	 * @private
    	 * @async
    	 * @throws {Error} If unable to fetch or parse TZI file
    	 */
    	async initTarzoom() {
    		this.tarzoom = [];
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
    			tile.end = tar.offsets[tile.index + 1];
    			return tar.url;
    		};
    	}

    	/**
    	 * Initializes Tarzoom layout.
    	 * @private
    	 * @async
    	 * @throws {Error} If unable to fetch or parse TZI file
    	 */
    	async initITarzoom() {
    		const url = this.urls[0];
    		var response = await fetch(url);
    		if (!response.ok) {
    			this.status = "Failed loading " + url + ": " + response.statusText;
    			throw new Error(this.status);
    		}
    		let json = await response.json();
    		Object.assign(this, json); //suffix, tilesize, overlap, width, height, levels
    		this.url = url.substr(0, url.lastIndexOf(".")) + '.tzb';

    		this.getTileURL = (rasterid, tile) => {
    			let index = tile.index * this.stride;
    			tile.start = this.offsets[index];
    			tile.end = this.offsets[index + this.stride];
    			tile.offsets = [];
    			for (let i = 0; i < this.stride + 1; i++)
    				tile.offsets.push(this.offsets[index + i] - tile.start);
    			return this.url;
    		};
    	}

    	/**
    	 * Initializes Zoomify layout.
    	 * @private
    	 * @async
    	 * @throws {Error} If unable to fetch or parse ImageProperties.xml
    	 */
    	async initZoomify() {
    		const url = this.urls[0];
    		this.overlap = 0;
    		var response = await fetch(url);
    		if (!response.ok) {
    			this.status = "Failed loading " + url + ": " + response.statusText;
    			throw new Error(this.status);
    		}
    		let text = await response.text();
    		let xml = (new window.DOMParser()).parseFromString(text, "text/xml");
    		let doc = xml.documentElement;
    		this.tilesize = parseInt(doc.getAttribute('TILESIZE'));
    		this.width = parseInt(doc.getAttribute('WIDTH'));
    		this.height = parseInt(doc.getAttribute('HEIGHT'));
    		if (!this.tilesize || !this.height || !this.width)
    			throw "Missing parameter files for zoomify!";

    		let max = Math.max(this.width, this.height) / this.tilesize;
    		this.nlevels = Math.ceil(Math.log(max) / Math.LN2) + 1;

    		this.getTileURL = (rasterid, tile) => {
    			const tileUrl = this.urls[rasterid].substr(0, url.lastIndexOf("/"));
    			let group = tile.index >> 8;
    			return tileUrl + "/TileGroup" + group + "/" + tile.level + "-" + tile.x + "-" + tile.y + "." + this.suffix;
    		};
    	}

    	/**
    	 * Initializes IIIF layout.
    	 * @private
    	 * @async
    	 * @throws {Error} If unable to fetch or parse info.json
    	 */
    	async initIIIF() {
    		const url = this.urls[0];
    		this.overlap = 0;

    		var response = await fetch(url);
    		if (!response.ok) {
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
    			if (xr + tw * s > this.width)
    				ws = (this.width - xr + s - 1) / s;
    			let hs = tw;
    			if (yr + tw * s > this.height)
    				hs = (this.height - yr + s - 1) / s;

    			return `${tileUrl}/${xr},${yr},${wr},${hr}/${ws},${hs}/0/default.jpg`;
    		};
    	}

    	/**
    	 * Initializes IIP layout.
    	 * @private
    	 * @async
    	 * @throws {Error} If unable to fetch or parse server response
    	 */
    	async initIIP() {

    		const server = this.server ? (this.server + '?FIF=') : '';
    		const url = server + this.urls[0] + "&obj=Max-size&obj=Tile-size&obj=Resolution-number";

    		let response = await fetch(url);
    		if (!response.ok) {
    			this.status = "Failed loading " + url + ": " + response.statusText;
    			throw new Error(this.status);
    		}
    		let info = await response.text();

    		let tmp = info.split("Tile-size:");
    		if (!tmp[1]) return null;
    		this.tilesize = parseInt(tmp[1].split(" ")[0]);
    		tmp = info.split("Max-size:");
    		if (!tmp[1]) return null;
    		tmp = tmp[1].split('\n')[0].split(' ');
    		this.width = parseInt(tmp[0]);
    		this.height = parseInt(tmp[1]);
    		this.nlevels = parseInt(info.split("Resolution-number:")[1]);

    		this.getTileURL = (rasterid, tile) => {

    			// Tile index for this resolution
    			let index = tile.y * this.qbox[tile.level].xHigh + tile.x;

    			// Handle different formats if requested or indicated in the info.json
    			let command = "JTL"; // Default
    			if (this.suffix == "webp") command = "WTL";
    			else if (this.suffix == "png") command = "PTL";

    			let url = (this.server ? this.server + '?FIF=' : '') + this.urls[rasterid] + "&" + command + "=" + tile.level + "," + index;
    			return url;
    		};
    	}
    }

    /**
     * @event Layout#ready
     * Fired when the layout is ready for rendering.
     * This occurs when:
     * - Single-resolution image is fully downloaded
     * - Multi-resolution structure is initialized and validated
     * - Tile pyramid information is computed
     */

    /**
     * @event Layout#updateSize
     * Fired when the layout size changes and scene extension needs updating.
     * This occurs when:
     * - Image dimensions are determined
     * - Pyramid levels are initialized
     * - Bounding boxes are computed
     */

    /**
     * Factory function to create LayoutTiles instances.
     * @private
     * @param {string} url - URL to image resource
     * @param {Layout#Type} type - Layout type
     * @param {Object} [options] - Configuration options
     * @returns {LayoutTiles} New LayoutTiles instance
     */
    const factory = (url, type, options) => {
    	return new LayoutTiles(url, type, options);
    };

    // Register supported layout types
    for (let type of ['google', 'deepzoom1px', 'deepzoom', 'zoomify', 'iiif', 'iip', 'tarzoom', 'itarzoom'])
    	Layout.prototype.types[type] = factory;

    /**
     * @typedef {Object} ShaderFilter~Mode
     * A shader filter mode configuration
     * @property {string} id - Unique identifier for the mode
     * @property {boolean} enable - Whether the mode is active
     * @property {string} src - GLSL source code for the mode
     */

    /**
     * @typedef {Object} ShaderFilter~Sampler
     * A texture sampler used by the filter
     * @property {string} name - Unique name for the sampler
     * @property {WebGLTexture} [texture] - Associated WebGL texture
     * @property {WebGLUniformLocation} [location] - GPU location for the sampler
     */

    /**
     * 
     * Base class for WebGL shader filters in OpenLIME.
     * Provides infrastructure for creating modular shader effects that can be chained together.
     * 
     * Features:
     * - Modular filter architecture
     * - Automatic uniform management
     * - Dynamic mode switching
     * - Texture sampling support
     * - GLSL code generation
     * - Unique naming conventions
     * 
     * Technical Implementation:
     * - Generates unique names for uniforms and samplers
     * - Manages WebGL resource lifecycle
     * - Supports multiple filter modes
     * - Handles shader program integration
     */
    class ShaderFilter {
        /**
         * Creates a new shader filter
         * @param {Object} [options] - Filter configuration
         * @param {ShaderFilter~Mode} [options.modes={}] - Available filter modes
         * @param {Object} [options.uniforms={}] - Filter uniform variables
         * @param {Array<ShaderFilter~Sampler>} [options.samplers=[]] - Texture samplers
         */
        constructor(options) {
            options = Object.assign({
            }, options);
            Object.assign(this, options);
            this.name = this.constructor.name;
            this.uniforms = {};
            this.samplers = [];
            this.needsUpdate = true;
            this.shader = null;

            this.modes = {};
        }

        /**
         * Sets the active mode for the filter
         * @param {string} mode - Mode category to modify
         * @param {string} id - Specific mode ID to enable
         * @throws {Error} If shader not registered or mode doesn't exist
         */
        setMode(mode, id) {
            if (!this.shader)
                throw Error("Shader not registered");

            if (Object.keys(this.modes).length > 0) {
                const list = this.modes[mode];
                if (list) {
                    list.map(a => {
                        a.enable = a.id == id;
                    });
                    this.shader.needsUpdate = true;
                } else {
                    throw Error(`Mode "${mode}" not exist!`);
                }
            }
        }

        /**
         * Prepares filter resources for rendering
         * @param {WebGLRenderingContext} gl - WebGL context
         * @private
         */
        prepare(gl) {
            if (this.needsUpdate)
                if (this.createTextures) this.createTextures(gl);
            this.needsUpdate = false;
        }


        /**
         * Generates mode-specific GLSL code
         * @returns {string} GLSL declarations for enabled modes
         * @private
         */
        fragModeSrc() {
            let src = '';
            for (const key of Object.keys(this.modes)) {
                for (const e of this.modes[key]) {
                    if (e.enable) {
                        src += e.src + '\n';
                    }
                }
            }
            return src;
        }

        /**
         * Sets a uniform variable value
         * @param {string} name - Base name of uniform variable
         * @param {number|boolean|Array} value - Value to set
         * @throws {Error} If shader not registered
         */
        setUniform(name, value) {
            if (!this.shader) {
                throw Error(`Shader not registered`);
            }
            this.shader.setUniform(this.uniformName(name), value);
        }

        /**
         * Generates sampler declarations
         * @returns {string} GLSL sampler declarations
         * @private
         */
        fragSamplerSrc() {
            let src = '';
            for (let s of this.samplers) {
                src += `
            uniform sampler2D ${s.name};`;
            }
            return src;
        }

        /**
         * Generates uniform variable declarations
         * @returns {string} GLSL uniform declarations
         * @private
         */
        fragUniformSrc() {
            let src = '';
            for (const [key, value] of Object.entries(this.uniforms)) {
                src += `
            uniform ${this.uniforms[key].type} ${key};`;
            }
            return src;
        }

        /**
         * Generates filter-specific GLSL function
         * @param {WebGLRenderingContext} gl - WebGL context
         * @returns {string|null} GLSL function definition
         * @virtual
         */
        fragDataSrc(gl) {
            return null;
        }

        // Utility methods documentation
        /**
         * @returns {string} Generated function name for the filter
         * @private
         */
        functionName() {
            return this.name + "_data";
        }

        /**
         * @param {string} name - Base sampler name
         * @returns {string} Unique sampler identifier
         * @private
         */
        samplerName(name) {
            return `${this.name}_${name}`;
        }

        /**
         * @param {string} name - Base uniform name
         * @returns {string} Unique uniform identifier
         * @private
         */
        uniformName(name) {
            return `u_${this.name}_${name}`;
        }

        /**
         * @param {string} name - Base mode name
         * @returns {string} Unique mode identifier
         * @private
         */
        modeName(name) {
            return `m_${this.name}_${name}`;
        }

        /**
         * Finds a sampler by name
         * @param {string} name - Base sampler name
         * @returns {ShaderFilter~Sampler|undefined} Found sampler or undefined
         */
        getSampler(name) {
            const samplername = this.samplerName(name);
            return this.samplers.find(e => e.name == samplername);
        }
    }

    /**
     * 
     * @extends ShaderFilter
     * Test filter that replaces transparent pixels with a specified color
     */
    class ShaderFilterTest extends ShaderFilter {
        /**
         * Creates a test filter
         * @param {Object} [options] - Filter options
         * @param {number[]} [options.nodata_col=[1,1,0,1]] - Color for transparent pixels
         */
        constructor(options) {
            super(options);
            this.uniforms[this.uniformName('nodata_col')] = { type: 'vec4', needsUpdate: true, size: 4, value: [1, 1, 0, 1] };
        }

        fragDataSrc(gl) {
            return `
            vec4 ${this.functionName()}(vec4 col){
                return col.a > 0.0 ? col : ${this.uniformName('nodata_col')};
            }`;
        }
    }

    /**
     * 
     * @extends ShaderFilter
     * Filter that modifies the opacity of rendered content
     */
    class ShaderFilterOpacity extends ShaderFilter {
        /**
         * Creates an opacity filter
         * @param {number} opacity - Initial opacity value [0-1]
         * @param {Object} [options] - Additional filter options
         */    
        constructor(opacity, options) {
            super(options);
            this.uniforms[this.uniformName('opacity')] = { type: 'float', needsUpdate: true, size: 1, value: opacity };
        }

        fragDataSrc(gl) {
            return `
            vec4 ${this.functionName()}(vec4 col){
                return vec4(col.rgb, col.a * ${this.uniformName('opacity')});
            }`;
        }
    }

    /**
     * 
     * @extends ShaderFilter
     * Filter that applies gamma correction to colors
     */
    class ShaderGammaFilter extends ShaderFilter {
        /**
         * Creates a gamma correction filter
         * @param {Object} [options] - Filter options
         * @param {number} [options.gamma=2.2] - Gamma correction value
         */    
        constructor(options) {
            super(options);
            this.uniforms[this.uniformName('gamma')] = { type: 'float', needsUpdate: true, size: 1, value: 2.2 };
        }

        fragDataSrc(gl) {
            return `
            vec4 ${this.functionName()}(vec4 col){
                float igamma = 1.0/${this.uniformName('gamma')};
                return vec4(pow(col.r, igamma), pow(col.g, igamma), pow(col.b, igamma), col.a);
            }`;
        }
    }

    // vector field https://www.shadertoy.com/view/4s23DG
    // isolines https://www.shadertoy.com/view/Ms2XWc

    /**
     * @typedef {Object} ShaderFilterColormap~Options
     * Configuration options for colormap filter
     * @property {number[]} [inDomain=[]] - Input value range [min, max] for mapping
     * @property {number[]} [channelWeights=[1/3, 1/3, 1/3]] - RGB channel weights for grayscale conversion
     * @property {number} [maxSteps=256] - Number of discrete steps in the colormap texture
     */

    /**
     * 
     * @extends ShaderFilter
     * ShaderFilterColormap implements color mapping and visualization techniques.
     * Maps input RGB values to a specified colormap using customizable transfer functions.
     * 
     * Features:
     * - Custom colormap support
     * - Configurable input domain mapping
     * - Channel-weighted grayscale conversion
     * - Interpolated or discrete color mapping
     * - Out-of-range color handling
     * - GPU-accelerated processing
     * 
     * Technical Implementation:
     * - Uses 1D texture for colormap lookup
     * - Supports linear and nearest-neighbor interpolation
     * - Handles domain scaling and bias
     * - Configurable channel weight mixing
     * - WebGL 1.0 and 2.0 compatibility
     */
    class ShaderFilterColormap extends ShaderFilter {
        /**
         * Creates a new colormap filter
         * @param {ColorScale} colorscale - Colorscale object defining the mapping
         * @param {ShaderFilterColormap~Options} [options] - Configuration options
         * @param {number[]} [options.inDomain=[]] - Input domain range [min, max]
         * @param {number[]} [options.channelWeights=[1/3, 1/3, 1/3]] - RGB channel weights
         * @param {number} [options.maxSteps=256] - Colormap resolution
         * @throws {Error} If inDomain is invalid (length !== 2 or min >= max)
         * 
         * @example
         * ```javascript
         * // Create with custom domain and weights
         * const filter = new ShaderFilterColormap(colorscale, {
         *     inDomain: [0, 100],
         *     channelWeights: [0.2126, 0.7152, 0.0722], // Perceptual weights
         *     maxSteps: 512
         * });
         * ```
         */
        constructor(colorscale, options) {
            super(options);
            options = Object.assign({
                inDomain: [],
                channelWeights: [1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0],
                maxSteps: 256,
            }, options);
            Object.assign(this, options);

            if (this.inDomain.length != 2 && this.inDomain[1] <= this.inDomain[0]) {
                throw Error("inDomain bad format");
            }

            this.colorscale = colorscale;
            if (this.inDomain.length == 0) this.inDomain = this.colorscale.rangeDomain();

            const cscaleDomain = this.colorscale.rangeDomain();
            const scale = (this.inDomain[1] - this.inDomain[0]) / (cscaleDomain[1] - cscaleDomain[0]);
            const bias = (this.inDomain[0] - cscaleDomain[0]) / (cscaleDomain[1] - cscaleDomain[0]);

            this.samplers = [{ name: `${this.samplerName('colormap')}` }];

            this.uniforms[this.uniformName('channel_weigths')] = { type: 'vec3', needsUpdate: true, size: 3, value: this.channelWeights };
            this.uniforms[this.uniformName('low_color')] = { type: 'vec4', needsUpdate: true, size: 4, value: this.colorscale.lowColor.value() };
            this.uniforms[this.uniformName('high_color')] = { type: 'vec4', needsUpdate: true, size: 4, value: this.colorscale.highColor.value() };
            this.uniforms[this.uniformName('scale')] = { type: 'float', needsUpdate: true, size: 1, value: scale };
            this.uniforms[this.uniformName('bias')] = { type: 'float', needsUpdate: true, size: 1, value: bias };

        }

        /**
         * Creates the colormap texture in WebGL.
         * Samples colorscale at specified resolution,
         * creates 1D RGBA texture, configures texture filtering,
         * and associates texture with sampler.
         * 
         * @param {WebGLRenderingContext} gl - WebGL context
         * @returns {Promise<void>}
         * @private
         */
        async createTextures(gl) {
            const colormap = this.colorscale.sample(this.maxSteps);
            let textureFilter = gl.LINEAR;
            if (this.colorscale.type == 'bar') {
                textureFilter = gl.NEAREST;
            }
            const tex = gl.createTexture();
            gl.bindTexture(gl.TEXTURE_2D, tex);
            gl.texParameterf(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, textureFilter);
            gl.texParameterf(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, textureFilter);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
            gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, this.maxSteps, 1, 0, gl.RGBA, gl.UNSIGNED_BYTE, colormap.buffer);
            this.getSampler('colormap').tex = tex; // Link tex to sampler
        }

        /**
         * Generates the GLSL function for colormap lookup.
         * 
         * Processing steps:
         * 1. Channel-weighted grayscale conversion
         * 2. Domain scaling and bias
         * 3. Out-of-range handling
         * 4. Colormap texture lookup
         * 
         * @param {WebGLRenderingContext} gl - WebGL context
         * @returns {string} GLSL function definition
         * @private
         */
        fragDataSrc(gl) {
            return `
            vec4 ${this.functionName()}(vec4 col){
                if(col.a == 0.0) return col;
                float v = dot(col.rgb, ${this.uniformName('channel_weigths')});
                float cv = v*${this.uniformName('scale')} + ${this.uniformName('bias')};

                if(cv >= 1.0) return ${this.uniformName('high_color')};
                if(cv <= 0.0) return ${this.uniformName('low_color')};

                return texture(${this.samplerName('colormap')}, vec2(cv, 0.5));
            }`;
        }


    }

    // vector field https://www.shadertoy.com/view/4s23DG
    // isolines https://www.shadertoy.com/view/Ms2XWc

    /**
     * @typedef {Object} ShaderFilterVector~Options
     * Configuration options for vector field visualization
     * @property {number[]} [inDomain=[]] - Input value range [min, max] for magnitude mapping
     * @property {number} [maxSteps=256] - Number of discrete steps in the colormap texture
     * @property {number[]} [arrowColor=[0.0, 0.0, 0.0, 1.0]] - RGBA color for arrows when using 'col' mode
     */

    /**
     * @typedef {Object} ShaderFilterVector~Modes
     * Available visualization modes
     * @property {string} normalize - Arrow normalization ('on'|'off')
     * @property {string} arrow - Arrow coloring mode ('mag'|'col')
     * @property {string} field - Background field visualization ('none'|'mag')
     */

    /**
     * 
     * ShaderFilterVector implements 2D vector field visualization techniques.
     * Based on techniques from "2D vector field visualization by Morgan McGuire"
     * and enhanced by Matthias Reitinger.
     * 
     * Features:
     * - Arrow-based vector field visualization
     * - Magnitude-based or custom arrow coloring
     * - Optional vector normalization
     * - Background field visualization
     * - Customizable arrow appearance
     * - Smooth interpolation
     * 
     * Technical Implementation:
     * - Tile-based arrow rendering
     * - Signed distance field for arrow shapes
     * - Dynamic magnitude scaling
     * - Colormap-based magnitude visualization
     * - WebGL 1.0 and 2.0 compatibility
     *
     * Example usage:
     * ```javascript
     * // Basic usage with default options
     * const vectorField = new ShaderFilterVector(myColorScale);
     * shader.addFilter(vectorField);
     * 
     * // Configure visualization modes
     * vectorField.setMode('normalize', 'on');  // Normalize arrow lengths
     * vectorField.setMode('arrow', 'col');     // Use custom arrow color
     * vectorField.setMode('field', 'mag');     // Show magnitude field
     * ```
     * 
     * Advanced usage with custom configuration:
     * ```javascript
     * const vectorField = new ShaderFilterVector(colorscale, {
     *     inDomain: [-10, 10],         // Vector magnitude range
     *     maxSteps: 512,               // Higher colormap resolution
     *     arrowColor: [1, 0, 0, 1]     // Red arrows
     * });
     * 
     * // Add to shader pipeline
     * shader.addFilter(vectorField);
     * ```
     *
     * GLSL Implementation Details
     * 
     * Key Components:
     * 1. Arrow Generation:
     *    - Tile-based positioning
     *    - Shaft and head construction
     *    - Size and direction control
     * 
     * 2. Distance Functions:
     *    - line3(): Distance to line segment
     *    - line(): Signed distance to line
     *    - arrow(): Complete arrow shape
     * 
     * 3. Color Processing:
     *    - Vector magnitude computation
     *    - Colormap lookup
     *    - Mode-based blending
     * 
     * Constants:
     * - ARROW_TILE_SIZE: Spacing between arrows (16.0)
     * - ISQRT2: 1/sqrt(2) for magnitude normalization
     * 
     * Uniforms:
     * - {vec4} arrow_color - Custom arrow color
     * - {vec4} low_color - Color for values below range
     * - {vec4} high_color - Color for values above range
     * - {float} scale - Magnitude scaling factor
     * - {float} bias - Magnitude offset
     * - {sampler2D} colormap - Magnitude colormap texture
     *
     * @extends ShaderFilter
     */
    class ShaderFilterVector extends ShaderFilter {
        /**
         * Creates a new vector field visualization filter
         * @param {ColorScale} colorscale - Colorscale for magnitude mapping
         * @param {ShaderFilterVector~Options} [options] - Configuration options
         * @throws {Error} If inDomain is invalid (length !== 2 or min >= max)
         * 
         * @example
         * ```javascript
         * // Create with default options
         * const filter = new ShaderFilterVector(colorscale, {
         *     inDomain: [0, 1],
         *     maxSteps: 256,
         *     arrowColor: [0, 0, 0, 1]
         * });
         * ```
         */
        constructor(colorscale, options) {
            super(options);
            options = Object.assign({
                inDomain: [],
                maxSteps: 256,
                arrowColor: [0.0, 0.0, 0.0, 1.0],

            }, options);
            Object.assign(this, options);

            if (this.inDomain.length != 2 && this.inDomain[1] <= this.inDomain[0]) {
                throw Error("inDomain bad format");
            }

            this.colorscale = colorscale;
            if (this.inDomain.length == 0) this.inDomain = this.colorscale.rangeDomain();

            const cscaleDomain = this.colorscale.rangeDomain();

            const scale = Math.sqrt((this.inDomain[1] * this.inDomain[1] + this.inDomain[0] * this.inDomain[0]) / (cscaleDomain[1] * cscaleDomain[1] + cscaleDomain[0] * cscaleDomain[0]));
            const bias = 0.0;

            this.modes = {
                normalize: [
                    { id: 'off', enable: true, src: `const bool ${this.modeName('arrowNormalize')} = false;` },
                    { id: 'on', enable: false, src: `const bool ${this.modeName('arrowNormalize')} = true;` }
                ],
                arrow: [
                    { id: 'mag', enable: true, src: `const int ${this.modeName('arrowColor')} = 0;` },
                    { id: 'col', enable: false, src: `const int ${this.modeName('arrowColor')} = 1;` }
                ],
                field: [
                    { id: 'none', enable: true, src: `const int ${this.modeName('fieldColor')} = 0;` },
                    { id: 'mag', enable: false, src: `const int ${this.modeName('fieldColor')} = 1;` }
                ]
            };

            this.samplers = [{ name: `${this.samplerName('colormap')}` }];

            this.uniforms[this.uniformName('arrow_color')] = { type: 'vec4', needsUpdate: true, size: 4, value: this.arrowColor };
            this.uniforms[this.uniformName('low_color')] = { type: 'vec4', needsUpdate: true, size: 4, value: this.colorscale.lowColor.value() };
            this.uniforms[this.uniformName('high_color')] = { type: 'vec4', needsUpdate: true, size: 4, value: this.colorscale.highColor.value() };
            this.uniforms[this.uniformName('scale')] = { type: 'float', needsUpdate: true, size: 1, value: scale };
            this.uniforms[this.uniformName('bias')] = { type: 'float', needsUpdate: true, size: 1, value: bias };
        }

        /**
         * Creates the colormap texture for magnitude visualization.
         * Samples colorscale at specified resolution, creates 1D RGBA texture,
         * configures appropriate texture filtering, and links texture with sampler.
         * 
         * @param {WebGLRenderingContext} gl - WebGL context
         * @returns {Promise<void>}
         * @private
         */
        async createTextures(gl) {
            const colormap = this.colorscale.sample(this.maxSteps);
            let textureFilter = gl.LINEAR;
            if (this.colorscale.type == 'bar') {
                textureFilter = gl.NEAREST;
            }
            const tex = gl.createTexture();
            gl.bindTexture(gl.TEXTURE_2D, tex);
            gl.texParameterf(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, textureFilter);
            gl.texParameterf(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, textureFilter);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
            gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, this.maxSteps, 1, 0, gl.RGBA, gl.UNSIGNED_BYTE, colormap.buffer);
            this.getSampler('colormap').tex = tex; // Link tex to sampler
        }

        /**
         * Generates GLSL code for vector field visualization.
         * 
         * Shader Features:
         * - Tile-based arrow placement
         * - Signed distance field arrow rendering
         * - Multiple visualization modes
         * - Magnitude-based colormapping
         * - Smooth field interpolation
         * 
         * @param {WebGLRenderingContext} gl - WebGL context
         * @returns {string} GLSL function definition
         * @private
         */
        fragDataSrc(gl) {
            return `
        // 2D vector field visualization by Matthias Reitinger, @mreitinger
        // Based on "2D vector field visualization by Morgan McGuire, http://casual-effects.com", https://www.shadertoy.com/view/4s23DG
        
        const float ARROW_TILE_SIZE = 16.0;
        const float ISQRT2 = 0.70710678118; // 1/sqrt(2)

        // Computes the center pixel of the tile containing pixel pos
        vec2 arrowTileCenterCoord(vec2 pos) {
            return (floor(pos / ARROW_TILE_SIZE) + 0.5) * ARROW_TILE_SIZE;
        }

        // Computes the distance from a line segment
        float line3(vec2 a, vec2 b, vec2 c) {
            vec2 ab = a - b;
            vec2 cb = c - b;
            float d = dot(ab, cb);
            float len2 = dot(cb, cb);
            float t = 0.0;
            if (len2 != 0.0) {
              t = clamp(d / len2, 0.0, 1.0);
            }
            vec2 r = b + cb * t;
            return distance(a, r);
        }

        // Computes the signed distance from a line segment
        float line(vec2 p, vec2 p1, vec2 p2) {
            vec2 center = (p1 + p2) * 0.5;
            float len = length(p2 - p1);
            vec2 dir = (p2 - p1) / len;
            vec2 rel_p = p - center;
            float dist1 = abs(dot(rel_p, vec2(dir.y, -dir.x)));
            float dist2 = abs(dot(rel_p, dir)) - 0.5*len;
            return max(dist1, dist2);
        }
        
        // v = field sampled at arrowTileCenterCoord(p), scaled by the length
        // desired in pixels for arrows
        // Returns a signed distance from the arrow
        float arrow(vec2 p, vec2 v) {
            if (${this.modeName('arrowNormalize')}) v = normalize(v);
            v *= ARROW_TILE_SIZE * 0.5; // Change from [-1,1] to pixels
            // Make everything relative to the center, which may be fractional
            p -= arrowTileCenterCoord(p);
                
            float mag_v = length(v), mag_p = length(p);
            
            if (mag_v > 0.0) {
                // Non-zero velocity case
                vec2 dir_v = normalize(v);
                
                // We can't draw arrows larger than the tile radius, so clamp magnitude.
                // Enforce a minimum length to help see direction
                mag_v = clamp(mag_v, 2.0, ARROW_TILE_SIZE * 0.4);
        
                // Arrow tip location
                v = dir_v * mag_v;
        
                // Signed distance from shaft
                float shaft = line3(p, v, -v);
                // Signed distance from head
                float head = min(line3(p, v, 0.4*v + 0.2*vec2(-v.y, v.x)),
                                 line3(p, v, 0.4*v + 0.2*vec2(v.y, -v.x)));
                return min(shaft, head);
            } else {
                // Signed distance from the center point
                return mag_p;
            }
        }
        
        vec4 lookupColormap(float cv) {            
            if(cv >= 1.0) 
                return ${this.uniformName('high_color')};
            else if(cv <= 0.0) 
                return ${this.uniformName('low_color')};
            return texture(${this.samplerName('colormap')}, vec2(cv, 0.5));
        }

        vec4 ${this.functionName()}(vec4 col){
            if(col.a == 0.0) return col;

            vec2 p = v_texcoord*tileSize; // point in pixel
            vec2 pc_coord = arrowTileCenterCoord(p)/tileSize; // center coordinate
            vec4 pc_val = texture(kd, pc_coord); // [0..1] - lookup color in center
            float s = 2.0;
            float b = -1.0;
            vec2 uvc = vec2(pc_val.x*s+b, pc_val.y*s+b); // [-1..1]
            vec2 uvr =  vec2(col.r*s+b, col.g*s+b); // [-1..1]

            // Colors
            float vc = length(uvc)*ISQRT2;
            float cvc = vc*${this.uniformName('scale')} + ${this.uniformName('bias')};
            float vr = length(uvr)*ISQRT2;
            float cvr = vr*${this.uniformName('scale')} + ${this.uniformName('bias')};
            vec4 cmapc = lookupColormap(cvc);
            vec4 cmapr = lookupColormap(cvr);
                
            // Arrow            
            float arrow_dist = arrow(p, uvc);
            
            vec4 arrow_col = cmapc;
            vec4 field_col = vec4(0.0, 0.0, 0.0, 0.0);

            switch (${this.modeName('arrowColor')}) {
                case 0:
                    arrow_col = cmapc;
                    break;
                case 1:
                    arrow_col = ${this.uniformName('arrow_color')};               
                    break;
            }

            switch (${this.modeName('fieldColor')}) {
                case 0:
                    field_col = vec4(0.0, 0.0, 0.0, 0.0);
                    break;
                case 1:
                    field_col = cmapr;              
                    break;
            }

            float t = clamp(arrow_dist, 0.0, 1.0);
            return  mix(arrow_col, field_col, t);
        }`;
        }


    }

    // vector field https://www.shadertoy.com/view/4s23DG
    // isolines https://www.shadertoy.com/view/Ms2XWc

    /**
     * @typedef {Object} ShaderFilterVectorGlyph~Options
     * Configuration options for glyph-based vector field visualization
     * @property {number[]} [inDomain=[]] - Input value range [min, max] for magnitude mapping
     * @property {number} [maxSteps=256] - Number of discrete steps in the colormap texture
     * @property {number[]} [glyphColor=[0.0, 0.0, 0.0, 1.0]] - RGBA color for glyphs when using 'col' mode
     * @property {number} [glyphsStride=80] - Horizontal spacing between glyphs in the sprite sheet
     * @property {number[]} [glyphsSize=[304, 64]] - Dimensions of the glyph sprite sheet [width, height]
     */

    /**
     * @typedef {Object} ShaderFilterVectorGlyph~Modes
     * Available visualization modes
     * @property {string} normalize - Glyph size normalization ('on'|'off')
     * @property {string} glyph - Glyph coloring mode ('mag'|'col')
     * @property {string} field - Background field visualization ('none'|'mag')
     */

    /**
     * ShaderFilterVectorGlyph implements sprite-based vector field visualization.
     * Uses pre-rendered glyphs from an SVG sprite sheet for high-quality vector field representation.
     * 
     * @class
     * @extends ShaderFilter
     * @classdesc A shader filter that implements sprite-based vector field visualization.
     * 
     * Features:
     * - SVG glyph-based vector field visualization
     * - Magnitude-dependent glyph selection
     * - Custom glyph coloring
     * - Optional vector normalization
     * - Background field visualization
     * - Smooth rotation and scaling
     * 
     * Technical Implementation:
     * - Sprite sheet-based glyph rendering
     * - Dynamic glyph rotation and scaling
     * - Automatic magnitude mapping
     * - Alpha-based glyph composition
     * - WebGL texture management
     * 
     * GLSL Implementation Constants:
     * - GLYPH_TILE_SIZE: Spacing between glyphs (16.0)
     * - ISQRT2: 1/sqrt(2) for magnitude normalization
     * 
     *
     */
    class ShaderFilterVectorGlyph extends ShaderFilter {
        /**
         * Creates a new glyph-based vector field visualization filter
         * @param {ColorScale} colorscale - Colorscale for magnitude mapping
         * @param {string} glyphsUrl - URL to SVG sprite sheet containing glyphs
         * @param {ShaderFilterVectorGlyph~Options} [options] - Configuration options
         * @throws {Error} If inDomain is invalid or glyphsUrl is empty
         * 
         * @example
         * ```javascript
         * // Create with custom options
         * const filter = new ShaderFilterVectorGlyph(colorscale, 'glyphs.svg', {
         *     inDomain: [0, 1],
         *     glyphsSize: [304, 64],
         *     glyphsStride: 80,
         *     glyphColor: [0, 0, 0, 1]
         * });
         * ```
         */
        constructor(colorscale, glyphsUrl, options) {
            super(options);
            options = Object.assign({
                inDomain: [],
                maxSteps: 256,
                glyphColor: [0.0, 0.0, 0.0, 1.0],
                glyphsStride: 80,
                glyphsSize: [304, 64]
            }, options);
            Object.assign(this, options);

            if (this.inDomain.length != 2 && this.inDomain[1] <= this.inDomain[0]) {
                throw Error("inDomain bad format");
            }

            this.glyphsUrl = glyphsUrl;
            if (this.glyphsUrl.length == 0) throw Error("glyphUrl is empty: no items to display");

            this.colorscale = colorscale;
            if (this.inDomain.length == 0) this.inDomain = this.colorscale.rangeDomain();

            const cscaleDomain = this.colorscale.rangeDomain();

            const scale = Math.sqrt((this.inDomain[1] * this.inDomain[1] + this.inDomain[0] * this.inDomain[0]) / (cscaleDomain[1] * cscaleDomain[1] + cscaleDomain[0] * cscaleDomain[0]));
            const bias = 0.0;

            const gap = this.glyphsStride - this.glyphsSize[1];
            const glyphCount = Math.round((this.glyphsSize[0] + gap) / this.glyphsStride);

            this.modes = {
                normalize: [
                    { id: 'off', enable: true, src: `const bool ${this.modeName('glyphNormalize')} = false;` },
                    { id: 'on', enable: false, src: `const bool ${this.modeName('glyphNormalize')} = true;` }
                ],
                glyph: [
                    { id: 'mag', enable: true, src: `const int ${this.modeName('glyphColor')} = 0;` },
                    { id: 'col', enable: false, src: `const int ${this.modeName('glyphColor')} = 1;` }
                ],
                field: [
                    { id: 'none', enable: true, src: `const int ${this.modeName('fieldColor')} = 0;` },
                    { id: 'mag', enable: false, src: `const int ${this.modeName('fieldColor')} = 1;` }
                ]
            };

            this.samplers = [{ name: `${this.samplerName('colormap')}` }, { name: `${this.samplerName('glyphs')}` }];


            this.uniforms[this.uniformName('glyph_color')] = { type: 'vec4', needsUpdate: true, size: 4, value: this.glyphColor };
            this.uniforms[this.uniformName('glyph_count')] = { type: 'float', needsUpdate: true, size: 1, value: glyphCount };
            this.uniforms[this.uniformName('glyph_wh')] = { type: 'float', needsUpdate: true, size: 1, value: this.glyphsSize[1] };
            this.uniforms[this.uniformName('glyph_stride')] = { type: 'float', needsUpdate: true, size: 1, value: this.glyphsStride };

            this.uniforms[this.uniformName('low_color')] = { type: 'vec4', needsUpdate: true, size: 4, value: this.colorscale.lowColor.value() };
            this.uniforms[this.uniformName('high_color')] = { type: 'vec4', needsUpdate: true, size: 4, value: this.colorscale.highColor.value() };
            this.uniforms[this.uniformName('scale')] = { type: 'float', needsUpdate: true, size: 1, value: scale };
            this.uniforms[this.uniformName('bias')] = { type: 'float', needsUpdate: true, size: 1, value: bias };
        }

        /**
         * Creates textures for glyphs and colormap.
         * 
         * Implementation details:
         * 1. Glyph Texture:
         *    - Rasterizes SVG to image buffer
         *    - Creates and configures texture
         *    - Sets up linear filtering
         * 
         * 2. Colormap Texture:
         *    - Samples colorscale
         *    - Creates 1D RGBA texture
         *    - Configures appropriate filtering
         * 
         * @param {WebGLRenderingContext} gl - WebGL context
         * @returns {Promise<void>}
         * @private
         */
        async createTextures(gl) {
            // Glyphs
            const glyphsBuffer = await Util.rasterizeSVG(this.glyphsUrl, this.glyphsSize);
            const glyphsTex = gl.createTexture();
            gl.bindTexture(gl.TEXTURE_2D, glyphsTex);
            gl.texParameterf(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
            gl.texParameterf(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
            gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, glyphsBuffer);
            this.getSampler('glyphs').tex = glyphsTex;

            // Colormap
            const colormap = this.colorscale.sample(this.maxSteps);
            let textureFilter = gl.LINEAR;
            if (this.colorscale.type == 'bar') {
                textureFilter = gl.NEAREST;
            }
            const tex = gl.createTexture();
            gl.bindTexture(gl.TEXTURE_2D, tex);
            gl.texParameterf(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, textureFilter);
            gl.texParameterf(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, textureFilter);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
            gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
            gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, this.maxSteps, 1, 0, gl.RGBA, gl.UNSIGNED_BYTE, colormap.buffer);
            this.getSampler('colormap').tex = tex;
        }

        /**
         * Generates GLSL code for glyph-based vector field visualization.
         * 
         * Shader Features:
         * - Tile-based glyph placement
         * - Dynamic glyph rotation
         * - Magnitude-based glyph selection
         * - Alpha-based composition
         * - Multiple visualization modes
         * 
         * @param {WebGLRenderingContext} gl - WebGL context
         * @returns {string} GLSL function definition
         * @private
         */
        fragDataSrc(gl) {
            return `
        // 2D vector glyph visualization
        
        const float GLYPH_TILE_SIZE = 16.0;
        const float ISQRT2 = 0.70710678118; // 1/sqrt(2)

        // Computes the center pixel of the tile containing pixel pos
        vec2 glyphTileCenterCoord(vec2 pos) {
            return (floor(pos / GLYPH_TILE_SIZE) + 0.5) * GLYPH_TILE_SIZE;
        }

        float glyph(vec2 p, vec2 v) {
            if (${this.modeName('glyphNormalize')}) v = normalize(v);
            
            // Make everything relative to the center, which may be fractional
            p -= glyphTileCenterCoord(p);
                
            float mag_v = length(v), mag_p = length(p);
            
            if (mag_v > 0.0) {
                // Non-zero velocity case
                vec2 dir_v = normalize(v);
                
                float level = floor((1.0-mag_v*ISQRT2) * ${this.uniformName('glyph_count')});
                level = min(level, ${this.uniformName('glyph_count')} - 1.0);

                mat2 rotm = mat2(
                    dir_v[1], dir_v[0], // first column
                    -dir_v[0], dir_v[1]  // second column
                );
            
                float scaleToGlyph =  ${this.uniformName('glyph_wh')} / GLYPH_TILE_SIZE;
                vec2 pp = rotm * p; // p on axys with origin in tile center and aligned with direction dir_v
                pp += vec2(GLYPH_TILE_SIZE * 0.5, GLYPH_TILE_SIZE * 0.5); // pp in [0, GLYPH_TILE_SIZE]
                pp *= scaleToGlyph; // pp in [0, glyph_wh]
                pp.x += level * ${this.uniformName('glyph_stride')}; // apply stride
                pp.y = ${this.uniformName('glyph_wh')} - pp.y - 1.0; // invert y-axis
                //vec4 g = texelFetch(${this.samplerName('glyphs')}, ivec2(pp), 0);
                float w = ${this.uniformName('glyph_stride')}*(${this.uniformName('glyph_count')} -1.0) + ${this.uniformName('glyph_wh')};
                float h = ${this.uniformName('glyph_wh')};
                vec2 ppnorm = pp/vec2(w,h);
                vec4 g = texture(${this.samplerName('glyphs')}, ppnorm);
                return 1.0-g.a;

            } else {
                // Signed distance from the center point
                return mag_p;
            }
        }
        
        vec4 lookupColormap(float cv) {            
            if(cv >= 1.0) 
                return ${this.uniformName('high_color')};
            else if(cv <= 0.0) 
                return ${this.uniformName('low_color')};
            return texture(${this.samplerName('colormap')}, vec2(cv, 0.5));
        }

        vec4 ${this.functionName()}(vec4 col){
            if(col.a == 0.0) return col;

            vec2 p = v_texcoord*tileSize; // point in pixel
            vec2 pc_coord = glyphTileCenterCoord(p)/tileSize; // center coordinate
            vec4 pc_val = texture(kd, pc_coord); // [0..1] - lookup color in center
            float s = 2.0;
            float b = -1.0;
            vec2 uvc = vec2(pc_val.x*s+b, pc_val.y*s+b); // [-1..1]
            vec2 uvr =  vec2(col.r*s+b, col.g*s+b); // [-1..1]

            // Colors
            float vc = length(uvc)*ISQRT2;
            float cvc = vc*${this.uniformName('scale')} + ${this.uniformName('bias')};
            float vr = length(uvr)*ISQRT2;
            float cvr = vr*${this.uniformName('scale')} + ${this.uniformName('bias')};
            vec4 cmapc = lookupColormap(cvc);
            vec4 cmapr = lookupColormap(cvr);
                
            // Glyph            
            float glyph_dist = glyph(p, uvc);

            vec4 glyph_col = cmapc;
            vec4 field_col = vec4(0.0, 0.0, 0.0, 0.0);

            switch (${this.modeName('glyphColor')}) {
                case 0:
                    glyph_col = cmapc;
                    break;
                case 1:
                    glyph_col = ${this.uniformName('glyph_color')};               
                    break;
            }

            switch (${this.modeName('fieldColor')}) {
                case 0:
                    field_col = vec4(0.0, 0.0, 0.0, 0.0);
                    break;
                case 1:
                    field_col = cmapr;              
                    break;
            }

            float t = clamp(glyph_dist, 0.0, 1.0);
            return  mix(glyph_col, field_col, t);
        }`;
        }
    }

    /**
     * @typedef {Object} ShaderCombiner~Operation
     * A shader operation that combines two textures.
     * @property {string} first - Assigns first texture as output (cout = c1)
     * @property {string} second - Assigns second texture as output (cout = c2)
     * @property {string} mean - Calculates average of textures (cout = (c1 + c2)/2.0)
     * @property {string} diff - Calculates difference between textures (cout = c2.rgb - c1.rgb)
     */

    /**
     * Fired when shader combination mode changes.
     * @event ShaderCombiner#update
     * @type {Object}
     * @property {string} mode - New combination mode
     * @property {string} previousMode - Previous combination mode
     */

    /**
     * ShaderCombiner module provides texture combination operations for OpenLIME.
     * Supports both WebGL 1.0 and 2.0/3.0 GLSL specifications with automatic version detection.
     * 
     * ShaderCombiner class manages the combination of two input textures using various operations.
     * Features:
     * - Multiple combination modes (first, second, mean, diff)
     * - Automatic texture sampling
     * - WebGL 1.0 and 2.0 compatibility
     * - Alpha channel preservation
     * 
     * @extends Shader
     */
    class ShaderCombiner extends Shader {
    	/**
    	 * Creates a new ShaderCombiner instance.
    	 * @param {Object} [options] - Configuration options
    	 * @param {string} [options.mode='mean'] - Combination mode to use
    	 * @param {Array<Object>} [options.samplers] - Texture sampler definitions (inherited from Shader)
    	 * @param {Object} [options.uniforms] - Shader uniform variables (inherited from Shader)
    	 * @param {string} [options.label] - Display label for the shader (inherited from Shader)
    	 * @param {boolean} [options.debug] - Enable debug output (inherited from Shader)
    	 * @fires Shader#update
    	 */
    	constructor(options) {
    		super(options);

    		this.mode = 'mean', //Lighten Darken Contrast Inversion HSV components LCh components
    			this.samplers = [
    				{ id: 0, name: 'source1', type: 'vec3' },
    				{ id: 1, name: 'source2', type: 'vec3' }
    			];

    		this.modes = ['first', 'second', 'mean', 'diff'];
    		this.operations = {
    			'first': 'color = c1;',
    			'second': 'color = c2;',
    			'mean': 'color = (c1 + c2)/2.0;',
    			'diff': 'color = vec4(c2.rgb - c1.rgb, c1.a);'
    		};
    	}

    	/**
    	 * Gets fragment shader source code.
    	 * Implements texture combination operations.
    	 * @param {WebGLRenderingContext} gl - WebGL context
    	 * @returns {string} Fragment shader source code
    	 * @private
    	 */
    	fragShaderSrc(gl) {
    		let gl2 = !(gl instanceof WebGLRenderingContext);
    		let operation = this.operations[this.mode];
    		return `

${gl2 ? 'in' : 'varying'} vec2 v_texcoord;

uniform sampler2D source1;
uniform sampler2D source2;

vec4 data() {
	vec4 c1 = texture(source1, v_texcoord);
	vec4 c2 = texture(source2, v_texcoord);
	vec4 color;
	${operation};
	return color;
}
`;
    	}

    	/**
    	 * Gets vertex shader source code.
    	 * Provides basic vertex transformation and texture coordinate passing.
    	 * @param {WebGLRenderingContext} gl - WebGL context
    	 * @returns {string} Vertex shader source code
    	 * @private
    	 */
    	vertShaderSrc(gl) {
    		let gl2 = !(gl instanceof WebGLRenderingContext);
    		return `${gl2 ? '#version 300 es' : ''}


${gl2 ? 'in' : 'attribute'} vec4 a_position;
${gl2 ? 'in' : 'attribute'} vec2 a_texcoord;

${gl2 ? 'out' : 'varying'} vec2 v_texcoord;

void main() {
	gl_Position = a_position;
	v_texcoord = a_texcoord;
}`;
    	}
    }

    /**
     * Base class that handles user interaction via device events (mouse/touch events).
     * Provides an abstract user interface to define interaction actions such as panning, pinching, tapping, etc...
     * The actions are implemented by pre-defined callback functions:
     * * `panStart(e)` intercepts the initial pan event (movement of the mouse after pressing a mouse button or moving a finger).
     * The event is captured calling `e.preventDefault()`.
     * * `panMove(e)` receives and handles the pan event.
     * * `panEnd(e)` intercepts the final pan event (the user releases the left mouse button or removes his finger from the screen).
     * * `pinchStart(e1, e2)` intercepts the initial pinch event (a continuous gesture that tracks the positions between the first two fingers that touch the screen).
     * The event is captured calling `e1.preventDefault()`.
     * * `pinchMove(e1,e2)` receives and handles the pinch event.
     * * `pinchEnd(e1,e2)` intercepts the final pinch event (the user removes one of their two fingers from the screen).
     * * `mouseWheel(e)` receives and handles the mouse wheel event (the user rotates the mouse wheel button).
     * * `fingerSingleTap(e)` receives and handles the single-tap event (the user presses a mouse button quickly or touches the screen shortly with a finger).
     * * `fingerDoubleTap(e)` receives and handles the double-tap event (the user quickly presses a mouse button twice or shortly touches the screen with a finger twice).
     * 
     * `e.preventDefault()` will capture the event and wont be propagated to other controllers.
     * 
     * This class only describes user interactions by implementing actions or callbacks. A **Controller** works in concert with a **PointerManager** object 
     * that emits events and links them to actions.
     * 
     * @abstract
     * @example
     * // Create a pan-zoom controller and associate it with the viewer's pointer manager
     * const panzoom = new OpenLIME.ControllerPanZoom(viewer.camera, {
     *     priority: -1000,
     *     activeModifiers: [0, 1]
     * });
     * viewer.pointerManager.onEvent(panzoom);
     */
    class Controller {
    	/**
    	 * Creates a new Controller instance.
    	 * @param {Object} [options] - Configuration options
    	 * @param {boolean} [options.active=true] - Whether the controller is initially active
    	 * @param {boolean} [options.debug=false] - Enable debug logging
    	 * @param {number} [options.panDelay=50] - Inertial value for panning movements in milliseconds
    	 * @param {number} [options.zoomDelay=200] - Delay for smoothing zoom events in milliseconds
    	 * @param {number} [options.priority=0] - Controllers with higher priority are invoked first
    	 * @param {number[]} [options.activeModifiers=[0]] - Array of modifier states that activate this controller
    	 */
    	constructor(options) {
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

    	/**
    	 * Gets the modifier state from an event.
    	 * @param {Event} e - The event to check
    	 * @returns {number} Modifier state bitmask where:
    	 * - 0 = No modifiers
    	 * - 1 = Ctrl key
    	 * - 2 = Shift key
    	 * - 4 = Alt key
    	 * Multiple modifiers combine their values (e.g., Ctrl+Shift = 3)
    	 */
    	modifierState(e) {
    		let state = 0;
    		if (e.ctrlKey) state += 1;
    		if (e.shiftKey) state += 2;
    		if (e.altKey) state += 4;

    		return state;
    	}

    	/**
    	 * Captures all events, preventing them from reaching other controllers.
    	 * @private
    	 */
    	captureEvents() {
    		this.capture = true;
    	}

    	/**
    	 * Releases event capture, allowing events to reach other controllers.
    	 * @private
    	 */
    	releaseEvents() {
    		this.capture = false;
    	}

    	/**
    	 * Handles the start of a pan gesture.
    	 * @virtual
    	 * @param {Event} e - The pan start event
    	 * @description Called when user starts panning (mouse down or finger touch).
    	 * Call e.preventDefault() to capture the event.
    	 */
    	panStart(e) { }

    	/**
    	 * Handles pan movement.
    	 * @virtual
    	 * @param {Event} e - The pan move event
    	 * @description Called continuously during panning.
    	 */
    	panMove(e) { }

    	/**
    	 * Handles the end of a pan gesture.
    	 * @virtual
    	 * @param {Event} e - The pan end event
    	 * @description Called when panning ends (mouse up or finger lift).
    	 */
    	panEnd(e) { }

    	/**
    	 * Handles the start of a pinch gesture.
    	 * @virtual
    	 * @param {Event} e1 - First finger event
    	 * @param {Event} e2 - Second finger event
    	 * @description Called when user starts a two-finger pinch.
    	 * Call e1.preventDefault() to capture the event.
    	 */
    	pinchStart(e1, e2) { }

    	/**
    	 * Handles pinch movement.
    	 * @virtual
    	 * @param {Event} e1 - First finger event
    	 * @param {Event} e2 - Second finger event
    	 * @description Called continuously during pinching.
    	 */
    	pinchMove(e1, e2) { }

    	/**
    	 * Handles the end of a pinch gesture.
    	 * @virtual
    	 * @param {Event} e1 - First finger event
    	 * @param {Event} e2 - Second finger event
    	 * @description Called when pinch ends (finger lift).
    	 */
    	pinchEnd(e1, e2) { }

    	/**
    	 * Handles mouse wheel events.
    	 * @virtual
    	 * @param {WheelEvent} e - The wheel event
    	 * @description Called when user rotates mouse wheel.
    	 */
    	mouseWheel(e) { }

    	/**
    	 * Handles single tap/click events.
    	 * @virtual
    	 * @param {Event} e - The tap event
    	 * @description Called for quick mouse press or short finger touch.
    	 */
    	fingerSingleTap(e) { }

    	/**
    	 * Handles double tap/click events.
    	 * @virtual
    	 * @param {Event} e - The double tap event
    	 * @description Called for quick double mouse press or double finger touch.
    	 */
    	fingerDoubleTap(e) { }
    }

    /**
     * Callback for position updates.
     * @callback updatePosition
     * @param {number} x - X coordinate in the range [-1, 1]
     * @param {number} y - Y coordinate in the range [-1, 1]
     */

    /**
     * Clamps a value between a minimum and maximum.
     * @param {number} value - Value to clamp
     * @param {number} min - Minimum allowed value
     * @param {number} max - Maximum allowed value
     * @returns {number} Clamped value
     * @private
     */
    function clamp(value, min, max) {
    	return Math.max(min, Math.min(max, value));
    }

    /**
     * Controller for handling 2D position updates based on pan and tap events.
     * Extends the base Controller to track a 2D position (x, y) of the device pointer.
     * 
     * Supports two coordinate systems:
     * - Absolute: Coordinates mapped to [-1, 1] with origin at bottom-left of canvas
     * - Relative: Coordinates based on distance from initial pan position, scaled by speed
     * 
     * @extends Controller
     */
    class Controller2D extends Controller {
    	/**
    	 * Creates a new Controller2D instance.
    	 * @param {updatePosition} callback - Function called when position is updated
    	 * @param {Object} [options] - Configuration options
    	 * @param {boolean} [options.relative=false] - Whether to use relative coordinate system
    	 * @param {number} [options.speed=2.0] - Scaling factor for relative coordinates
    	 * @param {BoundingBox} [options.box] - Bounding box for coordinate constraints
    	 * @param {updatePosition} [options.onPanStart] - Callback for pan start event
    	 * @param {updatePosition} [options.onPanEnd] - Callback for pan end event
    	 * @param {boolean} [options.active=true] - Whether the controller is active
    	 * @param {number[]} [options.activeModifiers=[0]] - Array of active modifier states
    	 */
    	constructor(callback, options) {
    		super(options);
    		Object.assign(this, {
    			relative: false,
    			speed: 2.0,
    			start_x: 0,
    			start_y: 0,
    			current_x: 0,
    			current_y: 0,
    			onPanStart: null,
    			onPanEnd: null
    		}, options);

    		//By default the controller is active only with no modifiers.
    		//you can select which subsets of the modifiers are active.
    		this.callback = callback;

    		if (!this.box) { //FIXME What is that? Is it used?
    			this.box = new BoundingBox({ xLow: -0.99, yLow: -0.99, xHigh: 0.99, yHigh: 0.99 });
    		}

    		this.panning = false;
    	}

    	/**
    	 * Updates the stored position for relative coordinate system.
    	 * This is a convenience method typically used within callbacks.
    	 * @param {number} x - New X coordinate in range [-1, 1]
    	 * @param {number} y - New Y coordinate in range [-1, 1]
    	 */
    	setPosition(x, y) {
    		this.current_x = x;
    		this.current_y = y;
    		this.callback(x, y);
    	}

    	/**
    	 * Maps canvas pixel coordinates to normalized coordinates [-1, 1].
    	 * @param {MouseEvent|TouchEvent} e - Mouse or touch event
    	 * @returns {number[]} Array containing [x, y] in normalized coordinates
    	 * @private
    	 */
    	project(e) {
    		let rect = e.target.getBoundingClientRect();
    		let x = 2 * e.offsetX / rect.width - 1;
    		let y = 2 * (1 - e.offsetY / rect.height) - 1;
    		return [x, y]
    	}

    	/**
    	 * Converts event coordinates to the appropriate coordinate system (absolute or relative).
    	 * @param {MouseEvent|TouchEvent} e - Mouse or touch event
    	 * @returns {number[]} Array containing [x, y] in the chosen coordinate system
    	 * @private
    	 */
    	rangeCoords(e) {
    		let [x, y] = this.project(e);

    		if (this.relative) {
    			x = clamp(this.speed * (x - this.start_x) + this.current_x, -1, 1);
    			y = clamp(this.speed * (y - this.start_y) + this.current_y, -1, 1);
    		}
    		return [x, y];
    	}

    	/**
    	 * Handles start of pan gesture.
    	 * @param {MouseEvent|TouchEvent} e - Pan start event
    	 * @override
    	 */
    	panStart(e) {
    		if (!this.active || !this.activeModifiers.includes(this.modifierState(e)))
    			return;

    		if (this.relative) {
    			let [x, y] = this.project(e);
    			this.start_x = x;
    			this.start_y = y;
    		}
    		if (this.onPanStart)
    			this.onPanStart(...this.rangeCoords(e));
    		this.callback(...this.rangeCoords(e));
    		this.panning = true;
    		e.preventDefault();
    	}

    	/**
    	 * Handles pan movement.
    	 * @param {MouseEvent|TouchEvent} e - Pan move event
    	 * @returns {boolean} False if not currently panning
    	 * @override
    	 */
    	panMove(e) {
    		if (!this.panning)
    			return false;
    		this.callback(...this.rangeCoords(e));
    	}

    	/**
    	 * Handles end of pan gesture.
    	 * @param {MouseEvent|TouchEvent} e - Pan end event
    	 * @returns {boolean} False if not currently panning
    	 * @override
    	 */
    	panEnd(e) {
    		if (!this.panning)
    			return false;
    		this.panning = false;
    		if (this.relative) {
    			let [x, y] = this.project(e);
    			this.current_x = clamp(this.speed * (x - this.start_x) + this.current_x, -1, 1);
    			this.current_y = clamp(this.speed * (y - this.start_y) + this.current_y, -1, 1);
    		}
    		if (this.onPanEnd)
    			this.onPanEnd(...this.rangeCoords(e));
    	}

    	/**
    	 * Handles single tap/click events.
    	 * Only processes events in absolute coordinate mode.
    	 * @param {MouseEvent|TouchEvent} e - Tap event
    	 * @override
    	 */
    	fingerSingleTap(e) {
    		if (!this.active || !this.activeModifiers.includes(this.modifierState(e)))
    			return;
    		if (this.relative)
    			return;

    		this.callback(...this.rangeCoords(e));
    		e.preventDefault();
    	}

    }

    /**
     * ControllerPanZoom handles pan, zoom, and interaction events in a canvas element to manipulate camera parameters.
     * It supports multiple interaction methods including:
     * - Mouse drag for panning
     * - Mouse wheel for zooming
     * - Touch gestures (pinch to zoom)
     * - Double tap to zoom
     * 
     * The controller maintains state for ongoing pan and zoom operations and can be configured
     * to use different coordinate systems (HTML or GL) for calculations.
     * 
     * @extends Controller
     * @fires ControllerPanZoom#nowheel - Emitted when a wheel event is received but ctrl key is required and not pressed
     */
    class ControllerPanZoom extends Controller {
    	/**
    	 * Creates a new ControllerPanZoom instance.
    	 * @param {Camera} camera - The camera object to control
    	 * @param {Object} [options] - Configuration options
    	 * @param {number} [options.zoomAmount=1.2] - The zoom multiplier for wheel/double-tap events
    	 * @param {boolean} [options.controlZoom=false] - If true, requires Ctrl key to be pressed for zoom operations
    	 * @param {boolean} [options.useGLcoords=false] - If true, uses WebGL coordinate system instead of HTML
    	 * @param {number} [options.panDelay] - Delay for pan animations
    	 * @param {number} [options.zoomDelay] - Delay for zoom animations
    	 */
    	constructor(camera, options) {
    		super(options);

    		this.camera = camera;
    		this.zoomAmount = 1.2;          //for wheel or double tap event
    		this.controlZoom = false;       //require control+wheel to zoom

    		this.panning = false;           //true if in the middle of a pan
    		this.initialTransform = null;
    		this.startMouse = null;

    		this.zooming = false;           //true if in the middle of a pinch
    		this.initialDistance = 0.0;
    		this.useGLcoords = false;

    		if (options)
    			Object.assign(this, options);
    	}

    	/**
    	 * Handles the start of a pan operation
    	 * @private
    	 * @param {PointerEvent} e - The pointer event that initiated the pan
    	 */
    	panStart(e) {
    		if (!this.active || this.panning || !this.activeModifiers.includes(this.modifierState(e)))
    			return;
    		this.panning = true;

    		this.startMouse = CoordinateSystem.fromCanvasHtmlToViewport({ x: e.offsetX, y: e.offsetY }, this.camera, this.useGLcoords);

    		let now = performance.now();
    		this.initialTransform = this.camera.getCurrentTransform(now);
    		this.camera.target = this.initialTransform.copy(); //stop animation.
    		e.preventDefault();
    	}

    	/**
    	 * Updates camera position during a pan operation
    	 * @private
    	 * @param {PointerEvent} e - The pointer event with new coordinates
    	 */
    	panMove(e) {
    		if (!this.panning)
    			return;

    		let m = this.initialTransform;
    		const p = CoordinateSystem.fromCanvasHtmlToViewport({ x: e.offsetX, y: e.offsetY }, this.camera, this.useGLcoords);
    		let dx = (p.x - this.startMouse.x);
    		let dy = (p.y - this.startMouse.y);

    		this.camera.setPosition(this.panDelay, m.x + dx, m.y + dy, m.z, m.a);
    	}

    	/**
    	 * Ends the current pan operation
    	 * @private
    	 * @param {PointerEvent} e - The pointer event that ended the pan
    	 */
    	panEnd(e) {
    		this.panning = false;
    	}

    	/**
    	 * Calculates the Euclidean distance between two points
    	 * @private
    	 * @param {Object} e1 - First point with x, y coordinates
    	 * @param {Object} e2 - Second point with x, y coordinates
    	 * @returns {number} The distance between the points
    	 */
    	distance(e1, e2) {
    		return Math.sqrt(Math.pow(e1.x - e2.x, 2) + Math.pow(e1.y - e2.y, 2));
    	}

    	/**
    	 * Initializes a pinch zoom operation
    	 * @private
    	 * @param {TouchEvent} e1 - First touch point
    	 * @param {TouchEvent} e2 - Second touch point
    	 */
    	pinchStart(e1, e2) {
    		this.zooming = true;
    		this.initialDistance = Math.max(30, this.distance(e1, e2));
    		e1.preventDefault();
    		//e2.preventDefault(); //TODO this is optional?
    	}

    	/**
    	 * Updates zoom level during a pinch operation
    	 * @private
    	 * @param {TouchEvent} e1 - First touch point
    	 * @param {TouchEvent} e2 - Second touch point
    	 */
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
    		// FIXME CHECK ON TOUCH SCREEN
    		//const pos = this.camera.mapToScene((offsetX1 + offsetX2)/2, (offsetY1 + offsetY2)/2, this.camera.getCurrentTransform(performance.now()));
    		const pos = CoordinateSystem.fromCanvasHtmlToScene({ x: (offsetX1 + offsetX2) / 2, y: (offsetY1 + offsetY2) / 2 }, this.camera, this.useGLcoords);

    		const dz = scale / this.initialDistance;
    		this.camera.deltaZoom(this.zoomDelay, dz, pos.x, pos.y);
    		this.initialDistance = scale;
    		e1.preventDefault();
    	}

    	/**
    	 * Ends the current pinch zoom operation
    	 * @private
    	 * @param {TouchEvent} e - The touch event that ended the pinch
    	 * @param {number} x - The x coordinate of the pinch center
    	 * @param {number} y - The y coordinate of the pinch center
    	 * @param {number} scale - The final scale factor
    	 */
    	pinchEnd(e, x, y, scale) {
    		this.zooming = false;
    		e.preventDefault();
    	}

    	/**
    	 * Handles mouse wheel events for zooming
    	 * @private
    	 * @param {WheelEvent} e - The wheel event
    	 * @fires ControllerPanZoom#nowheel
    	 */
    	mouseWheel(e) {
    		if (this.controlZoom && !e.ctrlKey) {
    			this.emit('nowheel');
    			return;
    		}
    		let delta = -e.deltaY / 53;
    		//const pos = this.camera.mapToScene(e.offsetX, e.offsetY, this.camera.getCurrentTransform(performance.now()));
    		const pos = CoordinateSystem.fromCanvasHtmlToScene({ x: e.offsetX, y: e.offsetY }, this.camera, this.useGLcoords);
    		const dz = Math.pow(this.zoomAmount, delta);
    		this.camera.deltaZoom(this.zoomDelay, dz, pos.x, pos.y);
    		e.preventDefault();
    	}

    	/**
    	 * Handles double tap events for zooming
    	 * @private
    	 * @param {PointerEvent} e - The pointer event representing the double tap
    	 */
    	fingerDoubleTap(e) { }
    	fingerDoubleTap(e) {
    		if (!this.active || !this.activeModifiers.includes(this.modifierState(e)))
    			return;
    		//const pos = this.camera.mapToScene(e.offsetX, e.offsetY, this.camera.getCurrentTransform(performance.now()));
    		const pos = CoordinateSystem.fromCanvasHtmlToScene({ x: e.offsetX, y: e.offsetY }, this.camera, this.useGLcoords);

    		const dz = this.zoomAmount;
    		this.camera.deltaZoom(this.zoomDelay, dz, pos.x, pos.y);
    	}

    }
    addSignals(ControllerPanZoom, 'nowheel');

    /**
     * The `PointerManager` class serves as a central event manager that interprets raw pointer events 
     * (like mouse clicks, touch gestures, or stylus inputs) into higher-level gestures. It abstracts 
     * away the complexity of handling multiple input types and transforms them into common gestures 
     * like taps, holds, panning (drag), and pinch-to-zoom, which are common in modern user interfaces.
     * 
     * Key mechanisms:
     * 
     * 1. **Event Handling and Gesture Recognition**:
     *    - `PointerManager` listens for low-level pointer events (such as `pointerdown`, `pointermove`, 
     *      and `pointerup`) and converts them into higher-level gestures. 
     *    - For example, a quick touch and release is interpreted as a "tap," while a sustained touch 
     *      (greater than 600ms) is recognized as a "hold."
     *    - It handles both mouse and touch events uniformly, making it ideal for web applications that 
     *      need to support diverse input devices (mouse, trackpad, touch screens).
     * 
     * 2. **Multi-pointer Support**:
     *    - `PointerManager` supports multiple pointers simultaneously, making it capable of recognizing 
     *      complex gestures involving more than one finger or pointer, such as pinch-to-zoom. 
     *    - For multi-pointer gestures, it tracks each pointer's position and movement separately, 
     *      allowing precise gesture handling.
     * 
     * 3. **Idle Detection**:
     *    - The class includes idle detection mechanisms, which can trigger actions or events when no 
     *      pointer activity is detected for a specified period. This can be useful for implementing 
     *      user inactivity warnings or pausing certain interactive elements when the user is idle.
     * 
     * 4. **Callback-based Gesture Management**:
     *    - The core of the `PointerManager` class revolves around registering and triggering callbacks 
     *      for different gestures. Callbacks are provided by the user of this class for events like 
     *      pan (`onPan`), pinch (`onPinch`), and others.
     *    - The class ensures that once a gesture starts, it monitors and triggers the appropriate 
     *      callbacks, such as `panStart`, `panMove`, and `panEnd`, depending on the detected gesture.
     * 
     * 5. **Buffer Management**:
     *    - The `PointerManager` class also includes a buffer system for managing and storing recent 
     *      events, allowing the developer to enqueue, push, pop, or shift pointer data as needed. 
     *      This can be helpful for applications that need to track the history of pointer events 
     *      for gesture recognition or undo functionality.
     * 
     * 6. **Error Handling**:
     *    - The class includes error handling to ensure that all required gesture handlers are defined 
     *      by the user. For example, it will throw an error if any essential callback functions for 
     *      pan or pinch gestures are missing, ensuring robust gesture management.
     * 
     * Typical usage involves:
     * - Registering gesture handlers (e.g., for taps, panning, pinching).
     * - The class then monitors all pointer events and triggers the corresponding gesture callbacks 
     *   when appropriate.
     * 
     * Example:
     * ```js
     * const manager = new PointerManager();
     * manager.onPan({
     *   panStart: (e) => console.log('Pan started', e),
     *   panMove: (e) => console.log('Panning', e),
     *   panEnd: (e) => console.log('Pan ended', e),
     *   priority: 1
     * });
     * manager.onPinch({
     *   pinchStart: (e) => console.log('Pinch started', e),
     *   pinchMove: (e) => console.log('Pinching', e),
     *   pinchEnd: (e) => console.log('Pinch ended', e),
     *   priority: 1
     * });
     * ```
     * 
     * In this example, `PointerManager` registers handlers for pan and pinch gestures, automatically 
     * converting pointer events into the desired interactions. By abstracting the raw pointer events, 
     * `PointerManager` allows developers to focus on handling higher-level gestures without worrying 
     * about the underlying complexity.
    * @fires PointerManager#fingerHover - Triggered when a pointer moves over a target.
    * @fires PointerManager#fingerSingleTap - Triggered on a quick touch or click.
    * @fires PointerManager#fingerDoubleTap - Triggered on two quick touches or clicks.
    * @fires PointerManager#fingerHold - Triggered when a touch or click is held for more than 600ms.
    * @fires PointerManager#mouseWheel - Triggered when the mouse wheel is rotated.
    * @fires PointerManager#panStart - Triggered when a pan (drag) gesture begins.
    * @fires PointerManager#panMove - Triggered during a pan gesture.
    * @fires PointerManager#panEnd - Triggered when a pan gesture ends.
    * @fires PointerManager#pinchStart - Triggered when a pinch gesture begins.
    * @fires PointerManager#pinchMove - Triggered during a pinch gesture.
    * @fires PointerManager#pinchEnd - Triggered when a pinch gesture ends.
    * 
    */
    class PointerManager {
        /**
         * Creates a new PointerManager instance.
         * @param {HTMLElement} target - DOM element to attach event listeners to
         * @param {Object} [options] - Configuration options
         * @param {number} [options.pinchMaxInterval=250] - Maximum time (ms) between touches to trigger pinch
         * @param {number} [options.idleTime=60] - Seconds of inactivity before idle event
         */
        constructor(target, options) {

            this.target = target;

            Object.assign(this, {
                pinchMaxInterval: 250,        // in ms, fingerDown event max distance in time to trigger a pinch.
                idleTime: 60, //in seconds,
            });

            if (options)
                Object.assign(this, options);

            this.idleTimeout = null;
            this.idling = false;

            this.currentPointers = [];
            this.eventObservers = new Map();
            this.ppmm = PointerManager.getPixelsPerMM();

            this.target.style.touchAction = "none";
            this.target.addEventListener('pointerdown', (e) => this.handleEvent(e), false);
            this.target.addEventListener('pointermove', (e) => this.handleEvent(e), false);
            this.target.addEventListener('pointerup', (e) => this.handleEvent(e), false);
            this.target.addEventListener('pointercancel', (e) => this.handleEvent(e), false);
            this.target.addEventListener('wheel', (e) => this.handleEvent(e), false);

            this.startIdle();
        }

        /**
         * Constant for targeting all pointers.
         * @type {number}
         * @readonly
         */
        static get ANYPOINTER() { return -1; }

        /// Utilities

        /**
         * Splits a string into an array based on whitespace.
         * 
         * @param {string} str - The input string to split.
         * @returns {Array<string>} An array of strings split by whitespace.
         * @private
         */
        static splitStr(str) {
            return str.trim().split(/\s+/g);
        }

        /**
         * Calculates device pixels per millimeter.
         * @returns {number} Pixels per millimeter for current display
         * @private
         */
        static getPixelsPerMM() {
            // Get the device pixel ratio
            const pixelRatio = window.devicePixelRatio || 1;

            // Create a div to measure
            const div = document.createElement("div");
            div.style.width = "1in";
            div.style.height = "1in";
            div.style.position = "absolute";
            div.style.left = "-100%";
            div.style.top = "-100%";
            document.body.appendChild(div);

            // Measure the div
            const pixelsPerInch = div.offsetWidth * pixelRatio;

            // Clean up
            document.body.removeChild(div);

            // Convert pixels per inch to pixels per mm
            const pixelsPerMM = pixelsPerInch / 25.4;

            return pixelsPerMM;
        }

        ///////////////////////////////////////////////////////////
        /// Class interface

        /**
         * Registers event handlers.
         * @param {string} eventTypes - Space-separated list of event types
         * @param {Object|Function} obj - Handler object or function
         * @param {number} [idx=ANYPOINTER] - Pointer index to target, or ANYPOINTER for all
         * @returns {Object} Handler object
         */
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

        /**
         * Unregisters event handlers.
         * @param {string} eventTypes - Space-separated list of event types
         * @param {Object|Function} callback - Handler to remove
         * @param {number} [idx=ANYPOINTER] - Pointer index to target
         */
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

        /**
         * Registers a complete event handler with multiple callbacks.
         * @param {Object} handler - Handler object
         * @param {number} handler.priority - Handler priority (higher = earlier execution)
         * @param {Function} [handler.fingerHover] - Hover callback
         * @param {Function} [handler.fingerSingleTap] - Single tap callback
         * @param {Function} [handler.fingerDoubleTap] - Double tap callback
         * @param {Function} [handler.fingerHold] - Hold callback
         * @param {Function} [handler.mouseWheel] - Mouse wheel callback
         * @param {Function} [handler.panStart] - Pan start callback
         * @param {Function} [handler.panMove] - Pan move callback
         * @param {Function} [handler.panEnd] - Pan end callback
         * @param {Function} [handler.pinchStart] - Pinch start callback
         * @param {Function} [handler.pinchMove] - Pinch move callback
         * @param {Function} [handler.pinchEnd] - Pinch end callback
         * @throws {Error} If handler lacks priority or required callbacks
         */
        onEvent(handler) {
            const cb_properties = ['fingerHover', 'fingerSingleTap', 'fingerDoubleTap', 'fingerHold', 'mouseWheel', 'wentIdle', 'activeAgain'];
            if (!handler.hasOwnProperty('priority'))
                throw new Error("Event handler has not priority property");

            if (!cb_properties.some((e) => typeof (handler[e]) == 'function'))
                throw new Error("Event handler properties are wrong or missing");

            for (let e of cb_properties)
                if (typeof (handler[e]) == 'function') {
                    this.on(e, handler);
                }
            if (handler.panStart)
                this.onPan(handler);
            if (handler.pinchStart)
                this.onPinch(handler);
        }

        /**
         * Registers callbacks for pan gestures (start, move, and end).
         * 
         * @param {Object} handler - The handler object containing pan gesture callbacks.
         * @param {function} handler.panStart - Callback function executed when the pan gesture starts.
         * @param {function} handler.panMove - Callback function executed during the pan gesture movement.
         * @param {function} handler.panEnd - Callback function executed when the pan gesture ends.
         * @throws {Error} Throws an error if any required callback functions (`panStart`, `panMove`, `panEnd`) are missing.
         */
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

        /**
         * Registers callbacks for pinch gestures (start, move, and end).
         * 
         * @param {Object} handler - The handler object containing pinch gesture callbacks.
         * @param {function} handler.pinchStart - Callback function executed when the pinch gesture starts.
         * @param {function} handler.pinchMove - Callback function executed during the pinch gesture movement.
         * @param {function} handler.pinchEnd - Callback function executed when the pinch gesture ends.
         * @throws {Error} Throws an error if any required callback functions (`pinchStart`, `pinchMove`, `pinchEnd`) are missing.
         */
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
                    if (e1.timeStamp - e2.timeStamp > this.pinchMaxInterval) break;

                    handler.pinchStart(e1, e2);
                    if (!e1.defaultPrevented) break;

                    clearTimeout(this.currentPointers[e1.idx].timeout);
                    clearTimeout(this.currentPointers[e2.idx].timeout);

                    this.on('fingerMovingStart', (e) => e.preventDefault(), e1.idx); //we need to capture this event (pan conflict)
                    this.on('fingerMovingStart', (e) => e.preventDefault(), e2.idx);
                    this.on('fingerMoving', (e) => e2 && handler.pinchMove(e1 = e, e2), e1.idx); //we need to assign e1 and e2, to keep last position.
                    this.on('fingerMoving', (e) => e1 && handler.pinchMove(e1, e2 = e), e2.idx);

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
            if (!this.eventObservers.has(eventType)) {
                this.eventObservers.set(eventType, new Set());
            }
            this.eventObservers.get(eventType).add(obj);
        }

        broadcastOff(eventTypes, obj) {
            PointerManager.splitStr(eventTypes).forEach(eventType => {
                if (this.eventObservers.has(eventType)) {
                    if (!obj) {
                        this.eventObservers.delete(eventType);
                    } else {
                        const handlers = this.eventObservers.get(eventType);
                        handlers.delete(obj);
                        if (handlers.size === 0) {
                            this.eventObservers.delete(eventType);
                        }
                    }
                }
            });
        }

        broadcast(e) {
            if (!this.eventObservers.has(e.fingerType)) return;
            const handlers = Array.from(this.eventObservers.get(e.fingerType));
            handlers.sort((a, b) => b.priority - a.priority);
            for (const obj of handlers) {
                obj[e.fingerType](e);
                if (e.defaultPrevented) break;
            }
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

        startIdle() {
            if (this.idleTimeout)
                clearTimeout(this.idleTimeout);
            this.idleTimeout = setTimeout(() => {
                this.broadcast({ fingerType: 'wentIdle' });
                this.idling = true;
            }, this.idleTime * 1000);
        }

        handleEvent(e) {
            //IDLING MANAGEMENT
            if (this.idling) {
                this.broadcast({ fingerType: 'activeAgain' });
                this.idling = false;

            } else {
                this.startIdle();
            }

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

    /**
     * Handles events for a single pointer.
     * @private
     */
    class SinglePointerHandler {
        /**
         * Creates a new SinglePointerHandler instance.
         * @param {PointerManager} parent - Parent PointerManager instance
         * @param {number} pointerId - Pointer identifier
         * @param {Object} [options] - Configuration options
         * @param {number} [options.ppmm=3] - Pixels per millimeter
         */
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
                            if (e.defaultPrevented) this.status = this.stateEnum.IDLE;
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
                            if (e.defaultPrevented) this.status = this.stateEnum.IDLE;
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

    /**
     * Circular buffer for event history.
     * @private
     */
    class CircularBuffer {
        /**
         * Creates a new CircularBuffer instance.
         * @param {number} capacity - Maximum number of elements
         * @throws {TypeError} If capacity is not a positive integer
         */
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
            if (this.size === this.capacity) {
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
            if (this.size === 0 && start === 0 && (end === undefined || end === 0)) return [];
            if (typeof start !== "number" || !Number.isInteger(start) || start < 0) throw new TypeError("Invalid start value");
            if (start >= this.size) throw new RangeError("Start index past end of buffer: " + start);

            if (end === undefined) return this.buffer[(this.first + start) % this.capacity];

            if (typeof end !== "number" || !Number.isInteger(end) || end < 0) throw new TypeError("Invalid end value");
            if (end >= this.size) throw new RangeError("End index past end of buffer: " + end);

            const result = [];
            for (let i = start; i <= end; i++) {
                result.push(this.buffer[(this.first + i) % this.capacity]);
            }
            return result;
        }

        toArray() {
            if (this.size == 0) return [];
            return this.get(0, this.size - 1);
        }

    }

    /**
     * @typedef {Object} ViewerOptions
     * Configuration options for Viewer initialization
     * @property {string} [background] - CSS background style
     * @property {boolean} [autofit=true] - Auto-fit camera to scene
     * @property {Object} [canvas={}] - Canvas configuration options
     * @property {Camera} [camera] - Custom camera instance
     */

    /**
     * @typedef {Object} Viewport
     * Viewport configuration
     * @property {number} x - Left coordinate
     * @property {number} y - Top coordinate
     * @property {number} dx - Width in pixels
     * @property {number} dy - Height in pixels
     * @property {number} w - Total width
     * @property {number} h - Total height
     */

    /**
     * Fired when frame is drawn
     * @event Viewer#draw
     */

    /**
     * Fired when viewer is resized
     * @event Viewer#resize
     * @property {Viewport} viewport - New viewport configuration
     */

    /**
     * 
     * Central class of the OpenLIME framework.
     * Creates and manages the main viewer interface, coordinates components,
     * and handles rendering pipeline.
     * 
     * Core Responsibilities:
     * - Canvas management
     * - Layer coordination
     * - Camera control
     * - Event handling
     * - Rendering pipeline
     * - Resource management
     * 
     *
     *
     * Component Relationships:
     * ```
     * Viewer
     * ├── Canvas
     * │   └── Layers
     * ├── Camera
     * ├── PointerManager
     * └── Controllers
     * ```
     * 
     * Rendering Pipeline:
     * 1. Camera computes current transform
     * 2. Canvas prepares render state
     * 3. Layers render in order
     * 4. Post-processing applied
     * 5. Frame timing recorded
     * 
     * Event System:
     * - draw: Emitted after each frame render
     * - resize: Emitted when viewport changes
     * 
     * Performance Considerations:
     * - Uses requestAnimationFrame
     * - Tracks frame timing
     * - Handles device pixel ratio
     * - Optimizes redraw requests
     * 
     * Resource Management:
     * - Automatic canvas cleanup
     * - Proper event listener removal
     * - ResizeObserver handling
     * 
     * @fires Viewer#draw
     * @fires Viewer#resize
     * 
     * @example
     * ```javascript
     * // Basic viewer setup
     * const viewer = new OpenLIME.Viewer('#container');
     * 
     * // Add image layer
     * const layer = new OpenLIME.Layer({
     *     layout: 'image',
     *     type: 'image',
     *     url: 'image.jpg'
     * });
     * viewer.addLayer('main', layer);
     * 
     * // Access components
     * const camera = viewer.camera;
     * const canvas = viewer.canvas;
     * ```
     */
    class Viewer {
    	/**
    	 * Creates a new Viewer instance
    	 * @param {HTMLElement|string} div - Container element or selector
    	 * @param {ViewerOptions} [options] - Configuration options
    	 * @param {number} [options.idleTime=60] - Seconds of inactivity before idle event
    	 * @throws {Error} If container element not found
    	 * 
    	 * Component Setup:
    	 * 1. Creates/configures canvas element
    	 * 2. Sets up overlay system
    	 * 3. Initializes camera
    	 * 4. Creates pointer manager
    	 * 5. Sets up resize observer
    	 * 
    	 * @example
    	 * ```javascript
    	 * // Create with options
    	 * const viewer = new OpenLIME.Viewer('.container', {
    	 *     background: '#000000',
    	 *     autofit: true,
    	 *     canvas: {
    	 *         preserveDrawingBuffer: true
    	 *     }
    	 * });
    	 * ```
    	 */
    	constructor(div, options) {

    		Object.assign(this, {
    			background: null,
    			autofit: true,
    			canvas: {},
    			camera: new Camera(),
    			idleTime: 60 // in seconds
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

    		if (this.autofit)
    			this.canvas.addEvent('updateSize', () => this.camera.fitCameraBox(0));

    		this.pointerManager = new PointerManager(this.overlayElement, { idleTime: this.idleTime });

    		this.canvasElement.addEventListener('contextmenu', (e) => {
    			e.preventDefault();
    			return false;
    		});

    		let resizeobserver = new ResizeObserver(entries => {
    			for (let entry of entries) {
    				this.resize(entry.contentRect.width, entry.contentRect.height);
    			}
    		});
    		resizeobserver.observe(this.canvasElement);

    		this.resize(this.canvasElement.clientWidth, this.canvasElement.clientHeight);

    	}

    	/**
    	 * Adds a device event controller to the viewer.
    	 * @param {Controller} controller An OpenLIME controller.
    	 */
    	addController(controller) {
    		this.pointerManager.onEvent(controller);
    	}

    	/**
    	 * Adds layer to viewer
    	 * @param {string} id - Unique layer identifier
    	 * @param {Layer} layer - Layer instance
    	 * @fires Canvas#update
    	 * 
    	 * @example
    	 * ```javascript
    	 * const layer = new OpenLIME.Layer({
    	 *     type: 'image',
    	 *     url: 'image.jpg'
    	 * });
    	 * viewer.addLayer('background', layer);
    	 * ```
    	 */
    	addLayer(id, layer) {
    		this.canvas.addLayer(id, layer);
    		this.redraw();
    	}

    	/**
    	 * Removes layer from viewer
    	 * @param {Layer|string} layer - Layer instance or ID
    	 * @fires Canvas#update
    	 */
    	removeLayer(layer) {
    		if (typeof (layer) == 'string')
    			layer = this.canvas.layers[layer];
    		if (layer) {
    			this.canvas.removeLayer(layer);
    			this.redraw();
    		}
    	}


    	/**
    	 * Handles viewer resizing
    	 * @param {number} width - New width in CSS pixels
    	 * @param {number} height - New height in CSS pixels
    	 * @private
    	 * @fires Viewer#resize
    	 */
    	resize(width, height) {
    		if (width == 0 || height == 0) return;
    		// Test with retina display!
    		this.canvasElement.width = width * window.devicePixelRatio;
    		this.canvasElement.height = height * window.devicePixelRatio;

    		let view = { x: 0, y: 0, dx: width, dy: height, w: width, h: height };
    		this.camera.setViewport(view);
    		this.canvas.updateSize();
    		this.emit('resize', view);

    		this.canvas.prefetch();
    		this.redraw();
    	}

    	/**
    	 * Schedules next frame for rendering
    	 * Uses requestAnimationFrame for optimal performance
    	 */
    	redraw() {
    		if (this.animaterequest) return;
    		this.animaterequest = requestAnimationFrame((time) => { this.draw(time); });
    		this.requestTime = performance.now();
    	}

    	/**
    	 * Performs actual rendering
    	 * @param {number} time - Current timestamp
    	 * @private
    	 * @fires Viewer#draw
    	 */
    	draw(time) {
    		if (!time) time = performance.now();
    		this.animaterequest = null;

    		let elapsed = performance.now() - this.requestTime;
    		this.canvas.addRenderTiming(elapsed);

    		this.camera.viewport;
    		this.camera.getCurrentTransform(time);

    		let done = this.canvas.draw(time);
    		if (!done)
    			this.redraw();
    		this.emit('draw');
    	}
    }

    addSignals(Viewer, 'draw');
    addSignals(Viewer, 'resize'); //args: viewport

    /**
     *  @default
     */
    let url = 'skin/skin.svg';

    /**
     *  @default
     */
    let pad = 5;

    let svg = null;


    /**
     * @typedef {Object} SkinIcon
     * A UI icon element from the skin file
     * @property {string} class - CSS class name (must start with 'openlime-')
     * @property {SVGElement} element - SVG DOM element
     */

    /**
     * Manages SVG-based user interface elements (skin) for OpenLIME.
     * 
     * The Skin system provides a centralized way to manage and customize UI elements
     * through an SVG-based theming system. Each UI component (buttons, menus, toolbars, 
     * dialogs) sources its visual elements from a single SVG file.
     * 
     * Design Requirements:
     * - SVG elements must have class names prefixed with 'openlime-'
     * - Icons should be properly viewboxed for scaling
     * - SVG should use relative paths for resources
     * 
     * Technical Features:
     * - Async SVG loading
     * - DOM-based SVG manipulation
     * - Element cloning support
     * - Automatic viewbox computation
     * - Padding management
     * - Transform handling
     * 
     *
     * Default Configuration
     * 
     * - {string} url - Default skin URL ('skin/skin.svg')
     * - {number} pad - Icon padding in SVG units (5)
     * 
     * File Structure Requirements:
     * ```xml
     * <svg>
     *   <!-- Icons should use openlime- prefix -->
     *   <g class="openlime-home">...</g>
     *   <g class="openlime-zoom">...</g>
     *   <g class="openlime-menu">...</g>
     * </svg>
     * ```
     * 
     * Common Icon Classes:
     * - openlime-home: Home/reset view
     * - openlime-zoom: Zoom controls
     * - openlime-menu: Menu button
     * - openlime-close: Close button
     * - openlime-next: Next/forward
     * - openlime-prev: Previous/back
     * 
     * Usage Notes:
     * - Always use async/await with icon methods
     * - Icons are cloned to allow multiple instances
     * - SVG is loaded once and cached
     * - Padding is applied uniformly
     * - ViewBox is computed automatically
     *
     * 
     * @static
     */
    class Skin {
    	/**
    	 * Sets the URL for the skin SVG file
    	 * @param {string} url - Path to SVG file containing UI elements
    	 * 
    	 * @example
    	 * ```javascript
    	 * // Set custom skin location
    	 * Skin.setUrl('/assets/custom-skin.svg');
    	 * ```
    	 */
    	static setUrl(u) { url = u; }

    	/**
    	 * Loads and parses the skin SVG file
    	 * Creates a DOM-based SVG element for future use
    	 * 
    	 * @throws {Error} If SVG file fails to load
    	 * @returns {Promise<void>}
    	 * 
    	 * @example
    	 * ```javascript
    	 * await Skin.loadSvg();
    	 * // SVG is now loaded and ready for use
    	 * ```
    	 */
    	static async loadSvg() {
    		var response = await fetch(url);
    		if (!response.ok) {
    			throw Error("Failed loading " + url + ": " + response.statusText);
    		}

    		let text = await response.text();
    		let parser = new DOMParser();
    		svg = parser.parseFromString(text, "image/svg+xml").documentElement;
    	}

    	/**
    	 * Retrieves a specific element from the skin by CSS selector
    	 * Automatically loads the SVG if not already loaded
    	 * 
    	 * @param {string} selector - CSS selector for the desired element
    	 * @returns {Promise<SVGElement>} Cloned SVG element
    	 * @throws {Error} Implicitly if element not found
    	 * 
    	 * @example
    	 * ```javascript
    	 * // Get home icon
    	 * const homeIcon = await Skin.getElement('.openlime-home');
    	 * 
    	 * // Get menu button
    	 * const menuBtn = await Skin.getElement('.openlime-menu');
    	 * ```
    	 */
    	static async getElement(selector) {
    		if (!svg)
    			await Skin.loadSvg();
    		return svg.querySelector(selector).cloneNode(true);
    	}

    	/**
    	 * Appends an SVG icon to a container element
    	 * Handles both string selectors and SVG elements
    	 * Automatically manages viewBox and transformations
    	 * 
    	 * @param {HTMLElement} container - Target DOM element to append icon to
    	 * @param {string|SVGElement} icon - Icon selector or SVG element
    	 * @returns {Promise<SVGElement>} Processed and appended SVG element
    	 * 
    	 * Processing steps:
    	 * 1. Loads icon (from selector or element)
    	 * 2. Creates SVG wrapper if needed
    	 * 3. Computes and sets viewBox
    	 * 4. Applies padding
    	 * 5. Handles transformations
    	 * 6. Appends to container
    	 * 
    	 * @example
    	 * ```javascript
    	 * // Append by selector
    	 * const icon1 = await Skin.appendIcon(
    	 *     document.querySelector('.toolbar'),
    	 *     '.openlime-zoom'
    	 * );
    	 * 
    	 * // Append existing SVG
    	 * const icon2 = await Skin.appendIcon(
    	 *     container,
    	 *     existingSvgElement
    	 * );
    	 * ```
    	 */
    	static async appendIcon(container, icon) {
    		let element = null;
    		let box = null;
    		if (typeof icon == 'string') {
    			element = await Skin.getElement(icon);
    			icon = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
    			icon.appendChild(element);
    			document.body.appendChild(icon);
    			box = element.getBBox();
    			let tlist = element.transform.baseVal;
    			if (tlist.numberOfItems == 0)
    				tlist.appendItem(icon.createSVGTransform());
    			tlist.getItem(0).setTranslate(-box.x, -box.y);
    		} else {
    			document.body.appendChild(icon);
    			box = icon.getBBox();
    		}
    		icon.setAttribute('viewBox', `${-pad} ${-pad} ${box.width + 2 * pad} ${box.height + 2 * pad}`);
    		icon.setAttribute('preserveAspectRatio', 'xMidYMid meet');
    		container.appendChild(icon);
    		return icon;
    	}
    }

    /**
     * ScaleBar module provides measurement scale visualization and unit conversion functionality.
     * Includes both a base Units class for unit management and a ScaleBar class for visual representation.
     *
     * Units class provides unit conversion and formatting capabilities.
     * Supports various measurement units and automatic unit selection based on scale.
     */
    class Units {
    	/**
    	 * Creates a new Units instance.
    	 * @param {Object} [options] - Configuration options
    	 * @param {string[]} [options.units=['km', 'm', 'cm', 'mm', 'µm']] - Available units in order of preference
    	 * @param {Object.<string, number>} [options.allUnits] - All supported units and their conversion factors to millimeters
    	 * @param {number} [options.precision=2] - Number of decimal places for formatted values
    	 */
    	constructor(options) {
    		this.units = ["km", "m", "cm", "mm", "µm"],
    			this.allUnits = { "µm": 0.001, "mm": 1, "cm": 10, "m": 1000, "km": 1e6, "in": 254, "ft": 254 * 12 };
    		this.precision = 2;
    		if (options)
    			Object.assign(options, this);
    	}

    	/**
    	 * Formats a measurement value with appropriate units.
    	 * Automatically selects the best unit if none specified.
    	 * @param {number} d - Value to format (in millimeters)
    	 * @param {string} [unit] - Specific unit to use for formatting
    	 * @returns {string} Formatted measurement with units (e.g., "5.00 mm" or "1.00 m")
    	 * 
    	 * @example
    	 * const units = new Units();
    	 * units.format(1500);       // Returns "1.50 m"
    	 * units.format(1500, 'mm'); // Returns "1500.00 mm"
    	 */
    	format(d, unit) {
    		if (d == 0)
    			return '';
    		if (unit)
    			return (d / this.allUnits[unit]).toFixed(this.precision) + unit;

    		let best_u = null;
    		let best_penalty = 100;
    		for (let u of this.units) {
    			let size = this.allUnits[u];
    			let penalty = d <= 0 ? 0 : Math.abs(Math.log10(d / size) - 1);
    			if (penalty < best_penalty) {
    				best_u = u;
    				best_penalty = penalty;
    			}
    		}
    		return this.format(d, best_u);
    	}
    }

    /**
     * ScaleBar class creates a visual scale bar that updates with viewer zoom level.
     * Features:
     * - Automatic scale adjustment based on zoom
     * - Smart unit selection
     * - SVG-based visualization
     * - Configurable size and appearance
     * @extends Units
     */
    class ScaleBar extends Units {
    	/**
    	 * Creates a new ScaleBar instance.
    	 * @param {number} pixelSize - Size of a pixel in real-world units (in mm)
    	 * @param {Viewer} viewer - The OpenLIME viewer instance
    	 * @param {Object} [options] - Configuration options
    	 * @param {number} [options.width=200] - Width of the scale bar in pixels
    	 * @param {number} [options.fontSize=24] - Font size for scale text in pixels
    	 * @param {number} [options.precision=0] - Number of decimal places for scale values
    	 * 
    	 * @property {SVGElement} svg - Main SVG container element
    	 * @property {SVGElement} line - Scale bar line element
    	 * @property {SVGElement} text - Scale text element
    	 * @property {number} lastScaleZoom - Last zoom level where scale was updated
    	 */
    	constructor(pixelSize, viewer, options) {
    		super(options);
    		options = Object.assign(this, {
    			pixelSize: pixelSize,
    			viewer: viewer,
    			width: 200,
    			fontSize: 24,
    			precision: 0
    		}, options);
    		Object.assign(this, options);

    		this.svg = Util.createSVGElement('svg', { viewBox: `0 0 ${this.width} 30` });
    		this.svg.classList.add('openlime-scale');

    		this.line = Util.createSVGElement('line', { x1: 5, y1: 26.5, x2: this.width - 5, y2: 26.5 });

    		this.text = Util.createSVGElement('text', { x: '50%', y: '16px', 'dominant-basiline': 'middle', 'text-anchor': 'middle' });
    		this.text.textContent = "";

    		this.svg.appendChild(this.line);
    		this.svg.appendChild(this.text);
    		this.viewer.containerElement.appendChild(this.svg);
    		this.viewer.addEvent('draw', () => { this.updateScale(); });
    	}

    	/**
    	 * Updates the scale bar based on current zoom level.
    	 * Called automatically on viewer draw events.
    	 * @private
    	 */
    	updateScale() {
    		//let zoom = this.viewer.camera.getCurrentTransform(performance.now()).z;
    		let zoom = this.viewer.camera.target.z;
    		if (zoom == this.lastScaleZoom)
    			return;
    		this.lastScaleZoom = zoom;
    		let s = this.bestLength(this.width / 2, this.width, this.pixelSize, zoom);

    		let margin = this.width - s.length;
    		this.line.setAttribute('x1', margin / 2);
    		this.line.setAttribute('x2', this.width - margin / 2);
    		this.text.textContent = this.format(s.label);
    	}

    	/**
    	 * Calculates the best scale length and label value for current zoom.
    	 * Tries to find a "nice" round number that fits within the given constraints.
    	 * @private
    	 * @param {number} min - Minimum desired length in pixels
    	 * @param {number} max - Maximum desired length in pixels
    	 * @param {number} pixelSize - Size of a pixel in real-world units
    	 * @param {number} zoom - Current zoom level
    	 * @returns {Object} Scale information
    	 * @returns {number} .length - Length of scale bar in pixels
    	 * @returns {number} .label - Value to display (in real-world units)
    	 */
    	bestLength(min, max, pixelSize, zoom) {
    		pixelSize /= zoom;
    		//closest power of 10:
    		let label10 = Math.pow(10, Math.floor(Math.log(max * pixelSize) / Math.log(10)));
    		let length10 = label10 / pixelSize;
    		if (length10 > min) return { length: length10, label: label10 };

    		let label20 = label10 * 2;
    		let length20 = length10 * 2;
    		if (length20 > min) return { length: length20, label: label20 };

    		let label50 = label10 * 5;
    		let length50 = length10 * 5;

    		if (length50 > min) return { length: length50, label: label50 };
    		return { length: 0, label: 0 }
    	}
    }

    /**
     * @fileoverview
     * Ruler module provides measurement functionality for the OpenLIME viewer.
     * Allows users to measure distances in the scene with an interactive ruler tool.
     * Extends the Units class to handle unit conversions and formatting.
     *
     * Ruler class creates an interactive measurement tool for the OpenLIME viewer.
     * Features:
     * - Interactive distance measurement
     * - SVG-based visualization
     * - Scale-aware display
     * - Multiple measurement history
     * - Touch and mouse support
     * 
     * @extends Units
     */
    class Ruler extends Units {
    	/**
    	 * Creates a new Ruler instance.
    	 * @param {Viewer} viewer - The OpenLIME viewer instance
    	 * @param {number} pixelSize - Size of a pixel in real-world units
    	 * @param {Object} [options] - Configuration options
    	 * @param {boolean} [options.enabled=false] - Whether the ruler is initially enabled
    	 * @param {number} [options.priority=100] - Event handling priority
    	 * @param {number} [options.fontSize=18] - Font size for measurements in pixels
    	 * @param {number} [options.markerSize=8] - Size of measurement markers in pixels
    	 * @param {string} [options.cursor='crosshair'] - Cursor style when ruler is active
    	 */
    	constructor(viewer, pixelSize, options) {
    		super(options);
    		Object.assign(this, {
    			viewer: viewer,
    			camera: viewer.camera,
    			overlay: viewer.overlayElement,
    			pixelSize: pixelSize,
    			enabled: false,
    			priority: 100,
    			measure: null, //current measure
    			history: [],  //past measures
    			fontSize: 18,
    			markerSize: 8,
    			cursor: "crosshair",

    			svg: null,
    			first: null,
    			second: null
    		});
    		if (options)
    			Object.assign(this, options);
    	}

    	/**
    	 * Activates the ruler tool.
    	 * Creates SVG elements if needed and sets up event listeners.
    	 * Changes cursor to indicate tool is active.
    	 */
    	start() {
    		this.enabled = true;
    		this.previousCursor = this.overlay.style.cursor;
    		this.overlay.style.cursor = this.cursor;

    		if (!this.svg) {
    			this.svg = Util.createSVGElement('svg', { class: 'openlime-ruler' });
    			this.svgGroup = document.createElementNS('http://www.w3.org/2000/svg', 'g');
    			this.svg.append(this.svgGroup);
    			this.overlay.appendChild(this.svg);
    			this.viewer.addEvent('draw', () => this.update());
    			this.update();
    		}
    	}

    	/**
    	 * Deactivates the ruler tool.
    	 * Restores original cursor and clears current measurement.
    	 */
    	end() {
    		this.enabled = false;
    		this.overlay.style.cursor = this.previousCursor;
    		this.clear();
    	}

    	/**
    	 * Clears all measurements.
    	 * Removes all SVG elements and resets measurement history.
    	 */
    	clear() {
    		this.svgGroup.replaceChildren([]);
    		this.measure = null;
    		this.history = [];
    	}

    	/*finish() {
    		let m = this.measure;
    		m.line = Util.createSVGElement('line', { x1: m.x1, y1: m.y1, x2: m.x2, y2: m.y2 });
    		this.svgGroup.appendChild(m.line);

    		m.text = Util.createSVGElement('text');
    		m.text.textContent = this.format(this.length(m));
    		this.svgGroup.appendChild(m.text);

    		this.history.push(m);
    		this.measure = null;
    		this.update();
    	}*/

    	/**
    	 * Updates the visual representation of all measurements.
    	 * Handles camera transformations and viewport changes.
    	 * @private
    	 */
    	update() {
    		if (!this.history.length)
    			return;
    		//if not enabled skip
    		let t = this.camera.getGlCurrentTransform(performance.now());
    		let viewport = this.camera.glViewport();
    		this.svg.setAttribute('viewBox', `${-viewport.w / 2} ${-viewport.h / 2} ${viewport.w} ${viewport.h}`);
    		let c = { x: 0, y: 0 }; //this.boundingBox().corner(0);
    		this.svgGroup.setAttribute("transform",
    			`translate(${t.x} ${t.y}) rotate(${-t.a} 0 0) scale(${t.z} ${t.z}) translate(${c.x} ${c.y})`);

    		for (let m of this.history)
    			this.updateMeasure(m, t);
    	}

    	/**
    	 * Creates a marker SVG element.
    	 * @private
    	 * @param {number} x - X coordinate in scene space
    	 * @param {number} y - Y coordinate in scene space
    	 * @returns {SVGElement} The created marker element
    	 */
    	createMarker(x, y) {
    		let m = Util.createSVGElement("path");
    		this.svgGroup.appendChild(m);
    		return m;
    	}

    	/**
    	 * Updates a marker's position and size.
    	 * @private
    	 * @param {SVGElement} marker - The marker to update
    	 * @param {number} x - X coordinate in scene space
    	 * @param {number} y - Y coordinate in scene space
    	 * @param {number} size - Marker size in pixels
    	 */
    	updateMarker(marker, x, y, size) {
    		let d = `M ${x - size} ${y} L ${x + size} ${y} M ${x} ${y - size} L ${x} ${y + size}`;
    		marker.setAttribute('d', d);
    	}

    	/**
    	 * Updates measurement text display.
    	 * Handles text positioning and scaling based on camera transform.
    	 * @private
    	 * @param {Object} measure - The measurement object to update
    	 * @param {number} fontsize - Font size in pixels
    	 */
    	updateText(measure, fontsize) {
    		measure.text.setAttribute('font-size', fontsize + "px");

    		let dx = measure.x1 - measure.x2;
    		let dy = measure.y1 - measure.y2;

    		let length = Math.sqrt(dx * dx + dy * dy);
    		if (length > 0) {
    			dx /= length;
    			dy /= length;
    		}
    		if (dx < 0) {
    			dx = -dx;
    			dy = -dy;
    		}

    		let mx = (measure.x1 + measure.x2) / 2;
    		let my = (measure.y1 + measure.y2) / 2;
    		if (dy / dx < 0) {
    			mx -= 0.25 * dy * fontsize;
    			my += dx * fontsize;
    		} else {
    			my -= 0.25 * fontsize;
    			mx += 0.25 * fontsize;
    		}
    		measure.text.setAttribute('x', mx);
    		measure.text.setAttribute('y', my);
    		measure.text.textContent = this.format(length * this.pixelSize);
    	}

    	/**
    	 * Creates a new measurement.
    	 * Sets up SVG elements for line, markers, and text.
    	 * @private
    	 * @param {number} x - Initial X coordinate
    	 * @param {number} y - Initial Y coordinate
    	 * @returns {Object} Measurement object containing all SVG elements and coordinates
    	 */
    	createMeasure(x, y) {
    		let m = {
    			marker1: this.createMarker(x, y),
    			x1: x, y1: y,
    			marker2: this.createMarker(x, y),
    			x2: x, y2: y
    		};
    		m.line = Util.createSVGElement('line', { x1: m.x1, y1: m.y1, x2: m.x2, y2: m.y2 });
    		this.svgGroup.appendChild(m.line);

    		m.text = Util.createSVGElement('text');
    		m.text.textContent = '';
    		this.svgGroup.appendChild(m.text);

    		return m;
    	}

    	/**
    	 * Updates a measurement's visual elements.
    	 * @private
    	 * @param {Object} measure - The measurement to update
    	 * @param {Transform} transform - Current camera transform
    	 */
    	updateMeasure(measure, transform) {
    		let markersize = window.devicePixelRatio * this.markerSize / transform.z;

    		this.updateMarker(measure.marker1, measure.x1, measure.y1, markersize);

    		this.updateMarker(measure.marker2, measure.x2, measure.y2, markersize);

    		let fontsize = window.devicePixelRatio * this.fontSize / transform.z;
    		this.updateText(measure, fontsize);

    		for (let p of ['x1', 'y1', 'x2', 'y2'])
    			measure.line.setAttribute(p, measure[p]);
    	}

    	/**
    	 * Handles single tap/click events.
    	 * Creates or completes measurements.
    	 * @private
    	 * @param {Event} e - The pointer event
    	 * @returns {boolean} Whether the event was handled
    	 */
    	fingerSingleTap(e) {
    		if (!this.enabled)
    			return false;

    		let transform = this.camera.getCurrentTransform(performance.now());
    		let { x, y } = this.camera.mapToScene(e.layerX, e.layerY, transform);


    		if (!this.measure) {
    			this.measure = this.createMeasure(x, y);
    			this.history.push(this.measure);
    		} else {
    			this.measure.x2 = x;
    			this.measure.y2 = y;
    			this.measure = null;
    		}
    		this.update();
    		e.preventDefault();
    	}

    	/**
    	 * Handles hover/move events.
    	 * Updates the current measurement endpoint.
    	 * @private
    	 * @param {Event} e - The pointer event
    	 * @returns {boolean} Whether the event was handled
    	 */
    	fingerHover(e) {
    		if (!this.enabled || !this.measure)
    			return false;

    		let transform = this.camera.getCurrentTransform(performance.now());
    		let { x, y } = this.camera.mapToScene(e.layerX, e.layerY, transform);
    		this.measure.x2 = x;
    		this.measure.y2 = y;
    		this.update();
    		e.preventDefault();
    	}
    }

    /**
     * @typedef {Object} UIAction
     * Action configuration for toolbar buttons
     * @property {string} title - Display title for the action
     * @property {boolean} display - Whether to show in toolbar
     * @property {string} [key] - Keyboard shortcut key
     * @property {Function} task - Callback function for action
     * @property {string} [icon] - Custom SVG icon path or content
     * @property {string} [html] - HTML content for help dialog
     */

    /**
     * @typedef {Object} MenuEntry
     * Menu configuration item
     * @property {string} [title] - Large title text
     * @property {string} [section] - Section header text
     * @property {string} [html] - Raw HTML content
     * @property {string} [button] - Button text
     * @property {string} [group] - Button group identifier
     * @property {string} [layer] - Associated layer ID
     * @property {string} [mode] - Layer visualization mode
     * @property {Function} [onclick] - Click handler
     * @property {Function} [oninput] - Input handler for sliders
     * @property {MenuEntry[]} [list] - Nested menu entries
     */

    /**
     * 
     * UIBasic implements a complete user interface for OpenLIME viewers.
     * Provides toolbar controls, layer management, and interactive features.
     * 
     * Core Features:
     * - Customizable toolbar
     * - Layer management
     * - Light direction control
     * - Camera controls
     * - Keyboard shortcuts
     * - Scale bar
     * - Measurement tools
     * 
     * Built-in Actions:
     * - home: Reset camera view
     * - fullscreen: Toggle fullscreen mode
     * - layers: Show/hide layer menu
     * - zoomin/zoomout: Camera zoom controls
     * - rotate: Rotate view
     * - light: Light direction control
     * - ruler: Distance measurement
     * - help: Show help dialog
     * - snapshot: Save view as image
     *
     * Implementation Details
     * 
     * Layer Management:
     * - Layers can be toggled individually
     * - Layer visibility affects associated controllers
     * - Overlay layers behave independently
     * - Layer state is reflected in menu UI
     * 
     * Mouse/Touch Interaction:
     * - Uses PointerManager for event handling
     * - Supports multi-touch gestures
     * - Handles drag operations for light control
     * - Manages tool state transitions
     * 
     * Menu System:
     * - Hierarchical structure
     * - Dynamic updates based on state
     * - Group-based selection
     * - Mode-specific entries
     * 
     * Controller Integration:
     * - Light direction controller
     * - Pan/zoom controller
     * - Measurement controller
     * - Priority-based event handling
     * 
     * Dialog System:
     * - Modal blocking of underlying UI
     * - Non-modal floating windows
     * - Content injection system
     * - Event-based communication
     * 
     * Skin System:
     * - SVG-based icons
     * - Dynamic loading
     * - CSS customization
     * - Responsive layout
     * 
     * Keyboard Support:
     * - Configurable shortcuts
     * - Action mapping
     * - Mode-specific keys
     * - Focus handling
     * 
     * See the complete example in: {@link https://github.com/cnr-isti-vclab/openlime/tree/main/dist/examples/ui-custom|GitHub ui-custom example}
     */
    class UIBasic {
    	/**
    	 * Creates a new UIBasic instance
    	 * @param {Viewer} viewer - OpenLIME viewer instance
    	 * @param {UIBasic~Options} [options] - Configuration options
    	 * 
    	 * @fires UIBasic#lightdirection
    	 * 
    	 * @example
    	 * ```javascript
    	 * const ui = new UIBasic(viewer, {
    	 *     // Enable specific actions
    	 *     actions: {
    	 *         light: { display: true },
    	 *         zoomin: { display: true },
    	 *         layers: { display: true }
    	 *     },
    	 *     // Add measurement support
    	 *     pixelSize: 0.1,
    	 *     // Add attribution
    	 *     attribution: "© Example Source"
    	 * });
    	 * ```
    	 */
    	constructor(viewer, options) {
    		//we need to know the size of the scene but the layers are not ready.
    		let camera = viewer.camera;
    		Object.assign(this, {
    			viewer: viewer,
    			camera: viewer.camera,
    			skin: 'skin/skin.svg',
    			autoFit: true, //FIXME to be moved in the viewer?
    			//skinCSS: 'skin.css', // TODO: probably not useful
    			actions: {
    				home: { title: 'Home', display: true, key: 'Home', task: (event) => { if (camera.boundingBox) camera.fitCameraBox(250); } },
    				fullscreen: { title: 'Fullscreen', display: true, key: 'f', task: (event) => { this.toggleFullscreen(); } },
    				layers: { title: 'Layers', display: true, key: 'Escape', task: (event) => { this.toggleLayers(); } },
    				zoomin: { title: 'Zoom in', display: false, key: '+', task: (event) => { camera.deltaZoom(250, 1.25, 0, 0); } },
    				zoomout: { title: 'Zoom out', display: false, key: '-', task: (event) => { camera.deltaZoom(250, 1 / 1.25, 0, 0); } },
    				rotate: { title: 'Rotate', display: false, key: 'r', task: (event) => { camera.rotate(250, -45); } },
    				light: { title: 'Light', display: 'auto', key: 'l', task: (event) => { this.toggleLightController(); } },
    				ruler: { title: 'Ruler', display: false, task: (event) => { this.toggleRuler(); } },
    				help: { title: 'Help', display: false, key: '?', task: (event) => { this.toggleHelp(this.actions.help); }, html: '<p>Help here!</p>' }, //FIXME Why a boolean in toggleHelp?
    				snapshot: { title: 'Snapshot', display: false, task: (event) => { this.snapshot(); } }, //FIXME not work!
    			},
    			postInit: () => { },
    			pixelSize: null,
    			unit: null, //FIXME to be used with ruler
    			attribution: null,     //image attribution
    			lightcontroller: null,
    			showLightDirections: false,
    			enableTooltip: true,
    			controlZoomMessage: null, //"Use Ctrl + Wheel to zoom instead of scrolling" ,
    			menu: []
    		});

    		Object.assign(this, options);
    		if (this.autoFit) //FIXME Check if fitCamera is triggered only if the layer is loaded. Is updateSize the right event?
    			this.viewer.canvas.addEvent('updateSize', () => this.viewer.camera.fitCameraBox(0));

    		this.panzoom = new ControllerPanZoom(this.viewer.camera, {
    			priority: -1000,
    			activeModifiers: [0, 1],
    			controlZoom: this.controlZoomMessage != null
    		});
    		if (this.controlZoomMessage)
    			this.panzoom.addEvent('nowheel', () => { this.showOverlayMessage(this.controlZoomMessage); });
    		this.viewer.pointerManager.onEvent(this.panzoom); //register wheel, doubleclick, pan and pinch
    		// this.viewer.pointerManager.on("fingerSingleTap", { "fingerSingleTap": (e) => { this.showInfo(e); }, priority: 10000 });

    		/*let element = entry.element;
    		let group = element.getAttribute('data-group');
    		let layer = element.getAttribute('data-layer');
    		let mode = element.getAttribute('data-mode');
    		let active = (layer && this.viewer.canvas.layers[layer].visible) &&
    			(!mode || this.viewer.canvas.layers[layer].getMode() == mode);
    		entry.element.classList.toggle('active', active); */

    		this.menu.push({ section: "Layers" });
    		for (let [id, layer] of Object.entries(this.viewer.canvas.layers)) {
    			let modes = [];
    			for (let m of layer.getModes()) {
    				let mode = {
    					button: m,
    					mode: m,
    					layer: id,
    					onclick: () => { layer.setMode(m); },
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
    				layer: id
    			};
    			if (modes.length > 1) layerEntry.list = modes;

    			if (layer.annotations) {
    				layerEntry.list = [];
    				//setTimeout(() => { 
    				layerEntry.list.push(layer.annotationsEntry());
    				//this.updateMenu();
    				//}, 1000);
    				//TODO: this could be a convenience, creating an editor which can be
    				//customized later using layer.editor.
    				//if(layer.editable) 
    				//	layer.editor = this.editor;
    			}
    			this.menu.push(layerEntry);
    		}

    		let controller = new Controller2D(
    			(x, y) => {
    				for (let layer of lightLayers)
    					layer.setLight([x, y], 0);
    				if (this.showLightDirections)
    					this.updateLightDirections(x, y);
    				this.emit('lightdirection', [x, y, Math.sqrt(1 - x * x + y * y)]);
    			}, {
    			// TODO: IS THIS OK? It was false before
    			active: false,
    			activeModifiers: [2, 4],
    			control: 'light',
    			onPanStart: this.showLightDirections ? () => {
    				Object.values(this.viewer.canvas.layers).filter(l => l.annotations != null).forEach(l => l.setVisible(false));
    				this.enableLightDirections(true);
    			} : null,
    			onPanEnd: this.showLightDirections ? () => {
    				Object.values(this.viewer.canvas.layers).filter(l => l.annotations != null).forEach(l => l.setVisible(true));
    				this.enableLightDirections(false);
    			} : null,
    			relative: true
    		});

    		controller.priority = 0;
    		this.viewer.pointerManager.onEvent(controller);
    		this.lightcontroller = controller;


    		let lightLayers = [];
    		for (let [id, layer] of Object.entries(this.viewer.canvas.layers))
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

    	/**
    	 * Shows overlay message
    	 * @param {string} msg - Message to display
    	 * @param {number} [duration=2000] - Display duration in ms
    	 */
    	showOverlayMessage(msg, duration = 2000) {
    		if (this.overlayMessage) {
    			clearTimeout(this.overlayMessage.timeout);
    			this.overlayMessage.timeout = setTimeout(() => this.destroyOverlayMessage(), duration);
    			return;
    		}


    		let background = document.createElement('div');
    		background.classList.add('openlime-overlaymsg');
    		background.innerHTML = `<p>${msg}</p>`;
    		this.viewer.containerElement.appendChild(background);

    		this.overlayMessage = {
    			background,
    			timeout: setTimeout(() => this.destroyOverlayMessage(), duration)
    		};
    	}

    	/**
    	 * Removes the overlay message
    	 * @private
    	 */
    	destroyOverlayMessage() {
    		this.overlayMessage.background.remove();
    		this.overlayMessage = null;
    	}

    	/**
    	 * Retrieves menu entry for a specific layer
    	 * @param {string} id - Layer identifier
    	 * @returns {UIBasic~MenuEntry|undefined} Found menu entry or undefined
    	 * @private
    	 */
    	getMenuLayerEntry(id) {
    		const found = this.menu.find(e => e.layer == id);
    		return found;
    	}

    	/**
    	 * Creates SVG elements for light direction indicators
    	 * @private
    	 */
    	createLightDirections() {
    		this.lightDirections = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
    		this.lightDirections.setAttribute('viewBox', '-100, -100, 200 200');
    		this.lightDirections.setAttribute('preserveAspectRatio', 'xMidYMid meet');
    		this.lightDirections.style.display = 'none';
    		this.lightDirections.classList.add('openlime-lightdir');
    		for (let x = -1; x <= 1; x++) {
    			for (let y = -1; y <= 1; y++) {
    				let line = document.createElementNS('http://www.w3.org/2000/svg', 'line');
    				line.pos = [x * 35, y * 35];
    				//line.setAttribute('data-start', `${x} ${y}`);
    				this.lightDirections.appendChild(line);
    			}
    		}
    		this.viewer.containerElement.appendChild(this.lightDirections);
    	}

    	/**
    	 * Updates light direction indicator positions
    	 * @param {number} lx - Light X coordinate
    	 * @param {number} ly - Light Y coordinate
    	 * @private
    	 */
    	updateLightDirections(lx, ly) {
    		let lines = [...this.lightDirections.children];
    		for (let line of lines) {
    			let x = line.pos[0];
    			let y = line.pos[1];

    			line.setAttribute('x1', 0.6 * x - 25 * 0 * lx);
    			line.setAttribute('y1', 0.6 * y + 25 * 0 * ly);
    			line.setAttribute('x2', x / 0.6 + 60 * lx);
    			line.setAttribute('y2', y / 0.6 - 60 * ly);
    		}
    	}

    	/**
    	 * Toggles visibility of light direction indicators
    	 * @param {boolean} show - Whether to show indicators
    	 * @private
    	 */
    	enableLightDirections(show) {
    		this.lightDirections.style.display = show ? 'block' : 'none';
    	}

    	/**
    	 * Initializes UI components
    	 * Sets up toolbar, menu, and controllers
    	 * @private
    	 * @async
    	 */
    	init() {
    		(async () => {

    			document.addEventListener('keydown', (e) => this.keyDown(e), false);
    			document.addEventListener('keyup', (e) => this.keyUp(e), false);

    			this.createMenu();
    			this.updateMenu();
    			this.viewer.canvas.addEvent('update', () => this.updateMenu());

    			if (this.actions.light && this.actions.light.display === 'auto')
    				this.actions.light.display = true;


    			if (this.skin)
    				await this.loadSkin();
    			/* TODO: this is probably not needed
    			if(this.skinCSS)
    				await this.loadSkinCSS();
    			*/

    			this.setupActions();


    			/* Get pixel size from options if provided or from layer metadata
    			 */
    			if (this.pixelSize) {
    				this.scalebar = new ScaleBar(this.pixelSize, this.viewer);
    			}
    			else if (this.viewer.canvas.layers[Object.keys(this.viewer.canvas.layers)[0]].pixelSize) {
    				let pixelSize = this.viewer.canvas.layers[Object.keys(this.viewer.canvas.layers)[0]].pixelSizePerMM();
    				this.scalebar = new ScaleBar(pixelSize, this.viewer);
    			}

    			if (this.attribution) {
    				var p = document.createElement('p');
    				p.classList.add('openlime-attribution');
    				p.innerHTML = this.attribution;
    				this.viewer.containerElement.appendChild(p);
    			}

    			for (let l of Object.values(this.viewer.canvas.layers)) {
    				this.setLayer(l);
    				break;
    			}

    			if (this.actions.light && this.actions.light.active)
    				this.toggleLightController();
    			if (this.actions.layers && this.actions.layers.active)
    				this.toggleLayers();

    			this.postInit();

    		})().catch(e => { console.log(e); throw Error("Something failed") });
    	}

    	/**
    	 * Handles keyboard down events
    	 * @param {KeyboardEvent} e - Keyboard event
    	 * @private
    	 */
    	keyDown(e) {
    	}

    	/**
    	 * Processes keyboard shortcuts
    	 * @param {KeyboardEvent} e - Keyboard event
    	 * @private
    	 */
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

    	/**
    	 * Loads and initializes skin SVG elements
    	 * @returns {Promise<void>}
    	 * @private
    	 * @async
    	 */
    	async loadSkin() {
    		let toolbar = document.createElement('div');
    		toolbar.classList.add('openlime-toolbar');
    		this.viewer.containerElement.appendChild(toolbar);

    		//toolbar manually created with parameters (padding, etc) + css for toolbar positioning and size.
    		{
    			for (let [name, action] of Object.entries(this.actions)) {

    				if (action.display !== true)
    					continue;

    				if ('icon' in action) {
    					if (typeof action.icon == 'string') {
    						if (Util.isSVGString(action.icon)) {
    							action.icon = Util.SVGFromString(action.icon);
    						} else {
    							action.icon = await Util.loadSVG(action.icon);
    						}
    						action.icon.classList.add('openlime-button');
    					}
    				} else {
    					action.icon = '.openlime-' + name;
    				}

    				action.element = await Skin.appendIcon(toolbar, action.icon);
    				if (this.enableTooltip) {
    					let title = document.createElementNS('http://www.w3.org/2000/svg', 'title');
    					title.textContent = action.title;
    					action.element.appendChild(title);
    				}
    			}

    		}
    	}

    	/**
    	 * Initializes action buttons and their event handlers
    	 * @private
    	 */
    	setupActions() {
    		for (let [name, action] of Object.entries(this.actions)) {
    			let element = action.element;
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
    				this.setLayer(this.viewer.layers[id]);
    			});
    		}
    	}

    	/**
    	 * Toggles light direction control mode
    	 * @param {boolean} [on] - Force specific state
    	 * @private
    	 */
    	toggleLightController(on) {
    		let div = this.viewer.containerElement;
    		let active = div.classList.toggle('openlime-light-active', on);
    		this.lightActive = active;

    		for (let layer of Object.values(this.viewer.canvas.layers))
    			for (let c of layer.controllers)
    				if (c.control == 'light') {
    					c.active = true;
    					c.activeModifiers = active ? [0, 2, 4] : [2, 4];  //nothing, shift and alt
    				}
    	}

    	/**
    	 * Toggles fullscreen mode
    	 * Handles browser-specific fullscreen APIs
    	 * @private
    	 */
    	toggleFullscreen() {
    		let canvas = this.viewer.canvasElement;
    		let div = this.viewer.containerElement;
    		let active = div.classList.toggle('openlime-fullscreen-active');

    		if (!active) {
    			var request = document.exitFullscreen || document.webkitExitFullscreen ||
    				document.mozCancelFullScreen || document.msExitFullscreen;
    			request.call(document); document.querySelector('.openlime-scale > line');

    			this.viewer.resize(canvas.offsetWidth, canvas.offsetHeight);
    		} else {
    			var request = div.requestFullscreen || div.webkitRequestFullscreen ||
    				div.mozRequestFullScreen || div.msRequestFullscreen;
    			request.call(div);
    		}
    		this.viewer.resize(canvas.offsetWidth, canvas.offsetHeight);
    	}

    	/**
    	 * Toggles measurement ruler tool
    	 * @private
    	 */
    	toggleRuler() {
    		if (!this.ruler) {
    			this.ruler = new Ruler(this.viewer, this.pixelSize);
    			this.viewer.pointerManager.onEvent(this.ruler);
    		}

    		if (!this.ruler.enabled)
    			this.ruler.start();
    		else
    			this.ruler.end();
    	}

    	/**
    	 * Toggles help dialog
    	 * @param {UIBasic~Action} help - Help action configuration
    	 * @param {boolean} [on] - Force specific state
    	 * @private
    	 */
    	toggleHelp(help, on) {
    		if (!help.dialog) {
    			help.dialog = new UIDialog(this.viewer.containerElement, { modal: true, class: 'openlime-help-dialog' });
    			help.dialog.setContent(help.html);
    		} else
    			help.dialog.toggle(on);
    	}

    	/**
    	 * Creates and downloads canvas snapshot
    	 * @private
    	 */
    	snapshot() {
    		var e = document.createElement('a');
    		e.setAttribute('href', this.viewer.canvas.canvasElement.toDataURL());
    		e.setAttribute('download', 'snapshot.png');
    		e.style.display = 'none';
    		document.body.appendChild(e);
    		e.click();
    		document.body.removeChild(e);
    	}

    	/* Layer management */

    	/**
    	 * Creates HTML for menu entry
    	 * @param {UIBasic~MenuEntry} entry - Menu entry to create
    	 * @returns {string} Generated HTML
    	 * @private
    	 */
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
    			let value = ('value' in entry) ? entry['value'] : 50;
    			html += `<input type="range" min="1" max="100" value="${value}" class="openlime-slider ${classes}" ${id}>`;
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

    	/**
    	 * Attaches event handlers to menu entry elements
    	 * @param {UIBasic~MenuEntry} entry - Menu entry to process
    	 * @private
    	 */
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

    	/**
    	 * Updates menu entry state
    	 * @param {UIBasic~MenuEntry} entry - Menu entry to update
    	 * @private
    	 */
    	updateEntry(entry) {
    		let status = entry.status ? entry.status() : '';
    		entry.element.classList.toggle('active', status == 'active');

    		if ('list' in entry)
    			for (let e of entry.list)
    				this.updateEntry(e);
    	}

    	/**
    	 * Updates all menu entries
    	 * @private
    	 */
    	updateMenu() {
    		for (let entry of this.menu)
    			this.updateEntry(entry);
    	}

    	/**
    	 * Creates main menu structure
    	 * @private
    	 */
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
    		this.viewer.containerElement.appendChild(this.layerMenu);

    		for (let entry of this.menu) {
    			this.addEntryCallbacks(entry);
    		}


    		/*		for(let li of document.querySelectorAll('[data-layer]'))
    					li.addEventListener('click', (e) => {
    						this.setLayer(this.viewer.canvas.layers[li.getAttribute('data-layer')]);
    					}); */
    	}

    	/**
    	 * Toggles layer menu visibility
    	 * @private
    	 */
    	toggleLayers() {
    		this.layerMenu.classList.toggle('open');
    	}

    	/**
    	 * Sets active layer and updates UI
    	 * @param {Layer|string} layer_on - Layer or layer ID to activate
    	 */
    	setLayer(layer_on) {
    		if (typeof layer_on == 'string')
    			layer_on = this.viewer.canvas.layers[layer_on];

    		if (layer_on.overlay) { //just toggle
    			layer_on.setVisible(!layer_on.visible);

    		} else {
    			for (let layer of Object.values(this.viewer.canvas.layers)) {
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
    		this.viewer.redraw();
    	}

    	/**
    	 * Hides layers menu
    	 */
    	// closeLayersMenu() {
    	// 	this.layerMenu.style.display = 'none';
    	// }
    }

    /**
     * A **UIDialog** is a top-level window used for communications with the user. It may be modal or modeless.
     * The content of the dialog can be either an HTML text or a pre-built DOM element.
     * When hidden, a dialog emits a 'closed' event.
     */
    class UIDialog { //FIXME standalone class
    	/**
    	 * Instatiates a UIDialog object.
    	 * @param {HTMLElement} container The HTMLElement on which the dialog is focused
    	 * @param {Object} [options] An object literal with UIDialog parameters.
    	 * @param {bool} options.modal Whether the dialog is modal. 
    	 */
    	constructor(container, options) {
    		Object.assign(this, {
    			dialog: null,
    			content: null,
    			container: container,
    			modal: false,
    			class: null,
    			visible: false,
    			backdropEvents: true
    		}, options);
    		this.create();
    	}

    	/**
    	 * Creates dialog DOM structure
    	 * @private
    	 */
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

    		if (this.modal) { //FIXME backdrown => backdrop
    			if (this.backdropEvents) background.addEventListener('click', (e) => { if (e.target == background) this.hide(); });
    			background.appendChild(dialog);
    			this.container.appendChild(background);
    			this.element = background;
    		} else {
    			this.container.appendChild(dialog);
    			this.element = dialog;
    		}

    		this.dialog = dialog;
    		this.content = content;
    		this.hide();
    	}

    	/**
    	 * Sets dialog content
    	 * @param {string|HTMLElement} html - Content to display
    	 */
    	setContent(html) {
    		if (typeof (html) == 'string')
    			this.content.innerHTML = html;
    		else
    			this.content.replaceChildren(html);
    	}

    	/**
    	 * Shows the dialog.
    	 */
    	show() {
    		this.element.classList.remove('hidden');
    		this.visible = true;
    	}

    	/**
    	 * Hides the dialog and emits closed event
    	 * @fires UIDialog#closed
    	 */
    	hide() {
    		/**
    		 * The event is fired when the dialog is closed.
    		 * @event UIDialog#closed
    		 */
    		this.element.classList.add('hidden');
    		this.visible = false;
    		this.emit('closed');
    	}

    	/**
    	 * Toggles fade effect
    	 * @param {boolean} on - Whether to enable fade effect
    	 */
    	fade(on) { //FIXME Does it work?
    		this.element.classList.toggle('fading');
    	}

    	/**
    	 * Toggles dialog visibility
    	 * @param {boolean} [force] - Force specific state
    	 */
    	toggle(force) { //FIXME Why not remove force?
    		this.element.classList.toggle('hidden', force);
    		this.visible = !this.visible; //FIXME not in sync with 'force'
    	}
    }

    /**
     * Event Definitions
     * 
     * Light Direction Change Event:
     * @event UIBasic#lightdirection
     * @type {Object}
     * @property {number[]} direction - [x, y, z] normalized light vector
     * 
     * Dialog Close Event:
     * @event UIDialog#closed
     * Emitted when dialog is closed through any means
     */

    addSignals(UIDialog, 'closed');
    addSignals(UIBasic, 'lightdirection');

    /**
     * The Draggable class enables an element to be dragged and moved around within its parent element.
     * It creates a draggable container with a handle that can be used to initiate the dragging action.
     * The draggable element is positioned absolutely within its parent container and can be customized
     * through various options.
     * 
     * The class requires either a top or bottom position and either a left or right position to be 
     * specified through the options parameter. These determine the initial position of the draggable
     * element within its parent container. The default positioning is bottom=20 and right=20.
     * 
     * Features:
     * - Flexible positioning using top/bottom and left/right coordinates
     * - Customizable handle size and appearance
     * - Automatic position updates on window resize
     * - Touch-enabled dragging support
     * - Configurable spacing between handle and content
     * 
     * 
     */
    class Draggable {
        /**
         * Creates a new Draggable instance.
         * 
         * @param {HTMLElement} element - The element to be made draggable
         * @param {HTMLElement|string} parent - The parent element where the draggable container will be appended.
         *                                     Can be either an HTMLElement or a CSS selector string
         * @param {Object} [options] - Configuration options for the draggable element
         * @param {number} [options.top] - The initial top position in pixels. Mutually exclusive with bottom
         * @param {number} [options.bottom=20] - The initial bottom position in pixels. Mutually exclusive with top
         * @param {number} [options.left] - The initial left position in pixels. Mutually exclusive with right
         * @param {number} [options.right=20] - The initial right position in pixels. Mutually exclusive with left
         * @param {number} [options.handleSize=10] - The size of the drag handle in pixels
         * @param {number} [options.handleGap=5] - The gap between the handle and the draggable content in pixels
         * @param {number} [options.zindex=200] - The z-index of the draggable container
         * @param {string} [options.handleColor='#f0f0f0b3'] - The background color of the handle (supports rgba)
         */
        constructor(element, parent, options) {
            options = Object.assign({
                top: null,
                bottom: 20,
                left: null,
                right: 20,
                handleSize: 10,
                handleGap: 5,
                zindex: 200,
                handleColor: '#f0f0f0b3' // rgba(240, 240, 240, 0.7)
            }, options);
            Object.assign(this, options);
            this.element = element;
            this.parent = parent;
            if (typeof (this.parent) == 'string')
                this.parent = document.querySelector(this.parent);

            if (this.left) this.right = null;
            if (this.top) this.bottom = null;

            // Disable context menu
            if (!('setCtxMenu' in window)) {
                window.addEventListener("contextmenu", e => e.preventDefault());
                window.setCtxMenu = true;
            }

            this.container = document.createElement('div');
            this.container.classList.add('openlime-draggable');
            this.container.style = `display: flex; gap:${this.handleGap}px; position: absolute; z-index: ${this.zindex}; touch-action: none; visibility: visible;`;
            this.handle = document.createElement('div');
            this.handle.style = `border-radius: 4px; background-color: ${this.handleColor}; padding: 0; width: ${this.handleSize}px; height: ${this.handleSize}px; z-index: 205;`;
            this.container.appendChild(this.handle);
            this.parent.appendChild(this.container);

            this.dragEvents();
            this.element.style.position = 'unset';

            this.appendChild(this.element);

            addEventListener("resize", (event) => {
                this.updatePos();
            });
        }

        /**
         * Appends an HTML element to the draggable container and updates its position.
         * @param {HTMLElement} element - The element to append to the draggable container
         */
        appendChild(e) {
            this.container.appendChild(e);
            this.updatePos();
        }

        /**
         * Updates the position of the draggable container based on its current options and parent dimensions.
         * This method is called automatically on window resize and when elements are appended.
         * @private
         */
        updatePos() {
            const w = this.container.offsetWidth;
            const h = this.container.offsetHeight;
            let t = 0;
            let l = 0;
            if (this.top) t = this.top;
            if (this.bottom) t = this.parent.offsetHeight - this.bottom - h;
            if (this.left) l = this.left;
            if (this.right) l = this.parent.offsetWidth - this.right - w;
            this.container.style.top = `${t}px`;
            this.container.style.left = `${l}px`;
        }

        /**
         * Sets up the drag event listeners for the handle.
         * Manages pointer events for drag start, drag, and drag end operations.
         * @private
         */
        dragEvents() {

            let offsetX, offsetY;
            const self = this;

            this.handle.addEventListener("pointerdown", dragStart);
            document.addEventListener("pointerup", dragEnd);

            function dragStart(e) {
                e.preventDefault();
                self.container.style.opacity = 0.6;
                offsetX = e.clientX - self.container.offsetLeft;
                offsetY = e.clientY - self.container.offsetTop;
                document.addEventListener("pointermove", drag);
            }

            function drag(e) {
                e.preventDefault();
                self.container.style.opacity = 0.6;
                self.container.style.left = e.clientX - offsetX + "px";
                self.container.style.top = e.clientY - offsetY + "px";
            }

            function dragEnd() {
                self.container.style.opacity = 1.0;
                document.removeEventListener("pointermove", drag);
            }
        }
    }

    /*
     * @fileoverview
     * LightSphereController module provides a spherical interface for controlling light direction.
     * It creates a circular canvas-based UI element that allows users to interactively adjust
     * lighting direction through pointer interactions.
     */

    /**
     * LightSphereController creates an interactive sphere UI for light direction control.
     * Features:
     * - Circular interface with gradient background
     * - Pointer-based interaction for light direction
     * - Configurable size, position, and colors
     * - Minimum theta angle constraint
     * - Visual feedback with gradient and marker
     */
    class LightSphereController {
        /**
         * Creates a new LightSphereController instance.
         * @param {HTMLElement|string} parent - Parent element or selector where the controller will be mounted
         * @param {Object} [options] - Configuration options
         * @param {number} [options.width=128] - Width of the controller in pixels
         * @param {number} [options.height=128] - Height of the controller in pixels
         * @param {number} [options.top=60] - Top position offset in pixels
         * @param {number} [options.right=0] - Right position offset in pixels
         * @param {number} [options.thetaMin=0] - Minimum theta angle in degrees (constrains interaction radius)
         * @param {string} [options.colorSpot='#ffffff'] - Color of the central spot in the gradient
         * @param {string} [options.colorBkg='#0000ff'] - Color of the outer edge of the gradient
         * @param {string} [options.colorMark='#ff0000'] - Color of the position marker
         */    
        constructor(parent, options) {        
            options = Object.assign({
                width: 128,
                height: 128,
                top: 60,
                right: 0,
                thetaMin: 0,
                colorSpot: '#ffffff',
                colorBkg: '#0000ff',
                colorMark: '#ff0000'
            }, options);
            Object.assign(this, options);
            this.parent = parent;
            this.layers = [];
            if (typeof (this.parent) == 'string')
                this.parent = document.querySelector(this.parent);

            this.lightDir = [0, 0];

            this.containerElement = document.createElement('div');
            this.containerElement.style = `padding: 0; position: absolute; width: ${this.width}px; height: ${this.height}px; top:${this.top}px; right:${this.right}px; z-index: 200; touch-action: none; visibility: visible;`;
            this.containerElement.classList.add('openlime-lsc');

            (this.width * 0.5) * (1 - 0.8);
            this.dlCanvas = document.createElement('canvas');
            this.dlCanvas.width = this.width;
            this.dlCanvas.height = this.height;
            // this.dlCanvas.style = ''
            this.dlCanvasCtx = this.dlCanvas.getContext("2d");
            this.dlGradient = '';
            this.containerElement.appendChild(this.dlCanvas);
            this.parent.appendChild(this.containerElement);

            this.r = this.width * 0.5;
            this.thetaMinRad = this.thetaMin / 180.0 * Math.PI;
            this.rmax = this.r * Math.cos(this.thetaMinRad);

            this.interactLightDir(this.width * 0.5, this.height * 0.5);

            this.pointerDown = false;
            this.dlCanvas.addEventListener("pointerdown", (e) => {
                this.pointerDown = true;
                const rect = this.dlCanvas.getBoundingClientRect();
                let clickPosX =
                    (this.dlCanvas.width * (e.clientX - rect.left)) /
                    rect.width;
                let clickPosY =
                    (this.dlCanvas.height * (e.clientY - rect.top)) /
                    rect.height;
                this.interactLightDir(clickPosX, clickPosY);
                e.preventDefault();
            });

            this.dlCanvas.addEventListener("pointermove", (e) => {
                if (this.pointerDown) {
                    const rect = this.dlCanvas.getBoundingClientRect();
                    let clickPosX =
                        (this.dlCanvas.width * (e.clientX - rect.left)) /
                        rect.width;
                    let clickPosY =
                        (this.dlCanvas.height * (e.clientY - rect.top)) /
                        rect.height;
                    this.interactLightDir(clickPosX, clickPosY);
                    e.preventDefault();
                }
            });

            this.dlCanvas.addEventListener("pointerup", (e) => {
                this.pointerDown = false;
            });

            this.dlCanvas.addEventListener("pointerout", (e) => {
                this.pointerDown = false;
            });

        }

        /**
         * Adds a layer to be controlled by this light sphere.
         * The layer must support light control operations.
         * @param {Layer} layer - Layer to be controlled
         */    
        addLayer(l) {
            this.layers.push(l);
        }

        /**
         * Makes the controller visible.
         * @returns {string} The visibility style value
         */    
        show() {
            return this.containerElement.style.visibility = 'visible';
        }

        /**
         * Hides the controller.
         * @returns {string} The visibility style value
         */    
        hide() {
            return this.containerElement.style.visibility = 'hidden';
        }

        /**
         * Computes the radial gradient based on current light direction.
         * Creates a gradient that provides visual feedback about the light position.
         * @private
         */    
        computeGradient() {
            const x = (this.lightDir[0] + 1.0) * this.dlCanvas.width * 0.5;
            const y = (-this.lightDir[1] + 1.0) * this.dlCanvas.height * 0.5;
            this.dlGradient = this.dlCanvasCtx.createRadialGradient(
                x, y, this.dlCanvas.height / 8.0,
                x, y, this.dlCanvas.width / 1.2
            );
            this.dlGradient.addColorStop(0, this.colorSpot);
            this.dlGradient.addColorStop(1, this.colorBkg);
        }

        /**
         * Handles interaction to update light direction.
         * Converts pointer position to light direction vector while respecting constraints.
         * @private
         * @param {number} x - X coordinate in canvas space
         * @param {number} y - Y coordinate in canvas space
         */    
        interactLightDir(x, y) {
            let xc = x - this.r;
            let yc = this.r - y;
            const phy = Math.atan2(yc, xc);
            let l = Math.sqrt(xc * xc + yc * yc);
            l = l > this.rmax ? this.rmax : l;
            xc = l * Math.cos(this.thetaMinRad) * Math.cos(phy);
            yc = l * Math.cos(this.thetaMinRad) * Math.sin(phy);
            x = xc + this.r;
            y = this.r - yc;
            this.lightDir[0] = 2 * (x / this.dlCanvas.width - 0.5);
            this.lightDir[1] = 2 * (1 - y / this.dlCanvas.height - 0.5);
            // console.log('LD ', this.lightDir);
            for (const l of this.layers) {
                if (l.controls.light) l.setControl('light', this.lightDir, 5);
            }
            this.computeGradient();
            this.drawLightSelector(x, y);
        }

        /**
         * Draws the light direction selector UI.
         * Renders:
         * - Circular background with gradient
         * - Position marker at current light direction
         * @private
         * @param {number} x - X coordinate for position marker
         * @param {number} y - Y coordinate for position marker
         */    
        drawLightSelector(x, y) {
            this.dlCanvasCtx.clearRect(0, 0, this.dlCanvas.width, this.dlCanvas.height);
            this.dlCanvasCtx.beginPath();

            this.dlCanvasCtx.arc(
                this.dlCanvas.width / 2,
                this.dlCanvas.height / 2,
                this.dlCanvas.width / 2,
                0,
                2 * Math.PI
            );
            this.dlCanvasCtx.fillStyle = this.dlGradient;
            this.dlCanvasCtx.fill();

            this.dlCanvasCtx.beginPath();
            this.dlCanvasCtx.arc(x, y, this.dlCanvas.width / 30, 0, 2 * Math.PI);
            this.dlCanvasCtx.strokeStyle = this.colorMark;
            this.dlCanvasCtx.lineWidth = 2;
            this.dlCanvasCtx.stroke();
        }
    }

    /**
     * @typedef {Object} ShaderRTI~Basis
     * Configuration data for basis functions
     * @property {Float32Array} [basis] - PCA basis for rbf and bln modes
     * @property {number[][]} [lights] - Light directions for rbf interpolation
     * @property {number} [sigma] - RBF interpolation parameter
     * @property {number} [ndimensions] - PCA dimension space
     */

    /**
     * @typedef {Object} ShaderRTI~Options
     * Configuration options for RTI shader
     * @property {string} [mode='normal'] - Initial rendering mode
     * @property {string} [type] - Basis type: 'ptm'|'hsh'|'sh'|'rbf'|'bln'
     * @property {string} [colorspace] - Color space: 'lrgb'|'rgb'|'mrgb'|'mycc'
     * @property {number} [nplanes] - Number of coefficient planes
     * @property {number[]} [yccplanes] - Number of planes for YCC components
     * @property {Object} [material] - Material parameters for dequantization
     */

    /**
     * ShaderRTI implements various Reflectance Transformation Imaging techniques.
     * Works in conjunction with LayerRTI for interactive relighting of cultural heritage objects.
     * 
     * Supported Basis Types:
     * - PTM (Polynomial Texture Maps)
     * - HSH (Hemispherical Harmonics)
     * - SH (Spherical Harmonics)
     * - RBF (Radial Basis Functions)
     * - BLN (Bilinear Interpolation)
     * 
     * Features:
     * - Multiple rendering modes (light, normals, diffuse, specular)
     * - Various color space support
     * - Automatic basis selection
     * - Real-time coefficient interpolation
     * - Normal map visualization
     * - Material property control
     * 
     * Technical Implementation:
     * - Efficient GPU-based relighting
     * - Dynamic shader generation
     * - Coefficient plane management
     * - Light vector transformation
     * - Color space conversion
     * 
     * @extends Shader
     */
    class ShaderRTI extends Shader {
    	/**
    	 * Creates a new RTI shader
    	 * @param {ShaderRTI~Options} [options] - Configuration options
    	 * 
    	 * @example
    	 * ```javascript
    	 * // Create PTM shader
    	 * const shader = new ShaderRTI({
    	 *     type: 'ptm',
    	 *     colorspace: 'rgb',
    	 *     mode: 'light'
    	 * });
    	 * ```
    	 */
    	constructor(options) {
    		super({});

    		Object.assign(this, {
    			modes: ['light', 'normals', 'diffuse', 'specular'],
    			mode: 'normal',
    			type: ['ptm', 'hsh', 'sh', 'rbf', 'bln'],
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

    		if (this.relight)
    			this.init(this.relight);

    		this.setMode('light');
    	}

    	/**
    	 * Sets the rendering mode
    	 * @param {string} mode - One of: 'light', 'normals', 'diffuse', 'specular'
    	 * @throws {Error} If mode is not recognized
    	 */
    	setMode(mode) {
    		if (!(this.modes.includes(mode)))
    			throw Error("Unknown mode: " + mode);
    		this.mode = mode;

    		if (mode != 'light') {
    			this.lightWeights([0.612, 0.354, 0.707], 'base');
    			this.lightWeights([-0.612, 0.354, 0.707], 'base1');
    			this.lightWeights([0, -0.707, 0.707], 'base2');
    		}
    		this.needsUpdate = true;
    	}

    	/**
    	 * Updates light direction for relighting
    	 * @param {number[]} light - Light vector [x, y], automatically normalized
    	 * @throws {Error} If shader is not initialized
    	 */
    	setLight(light) {
    		if (!this.uniforms.light)
    			throw "Shader not initialized, wait on layer ready event for setLight."

    		let x = light[0];
    		let y = light[1];

    		//map the square to the circle.
    		let r = Math.sqrt(x * x + y * y);
    		if (r > 1) {
    			x /= r;
    			y /= r;
    		}
    		let z = Math.sqrt(Math.max(0, 1 - x * x - y * y));
    		light = [x, y, z];

    		if (this.mode == 'light')
    			this.lightWeights(light, 'base');
    		this.setUniform('light', light);
    	}

    	/**
    	 * Sets specular exponent for specular enhancement mode
    	 * @param {number} value - Specular exponent
    	 */
    	setSpecularExp(value) {
    		this.setUniform('specular_exp', value);
    	}

    	/**
    	 * Initializes shader with RTI configuration
    	 * @param {Object} relight - RTI configuration data
    	 * @param {string} relight.type - Basis type
    	 * @param {string} relight.colorspace - Color space
    	 * @param {Object} relight.material - Material parameters
    	 * @param {number[]} relight.basis - Optional PCA basis
    	 */
    	init(relight) {
    		Object.assign(this, relight);
    		if (this.colorspace == 'mycc')
    			this.nplanes = this.yccplanes[0] + this.yccplanes[1] + this.yccplanes[2];
    		else
    			this.yccplanes = [0, 0, 0];


    		this.planes = [];
    		this.njpegs = 0;
    		while (this.njpegs * 3 < this.nplanes)
    			this.njpegs++;

    		for (let i = 0; i < this.njpegs; i++)
    			this.samplers.push({ id: i, name: 'plane' + i, type: 'vec3' });

    		if (this.normals)
    			this.samplers.push({ id: this.njpegs, name: 'normals', type: 'vec3' });

    		this.material = this.materials[0];

    		if (this.lights)
    			this.lights + new Float32Array(this.lights);

    		if (this.type == "rbf")
    			this.ndimensions = this.lights.length / 3;


    		if (this.type == "bilinear") {
    			this.ndimensions = this.resolution * this.resolution;
    			this.type = "bln";
    		}

    		this.scale = this.material.scale;
    		this.bias = this.material.bias;

    		if (['mrgb', 'mycc'].includes(this.colorspace))
    			this.loadBasis(this.basis);


    		this.uniforms = {
    			light: { type: 'vec3', needsUpdate: true, size: 3, value: [0.0, 0.0, 1] },
    			specular_exp: { type: 'float', needsUpdate: false, size: 1, value: 10 },
    			bias: { type: 'vec3', needsUpdate: true, size: this.nplanes / 3, value: this.bias },
    			scale: { type: 'vec3', needsUpdate: true, size: this.nplanes / 3, value: this.scale },
    			base: { type: 'vec3', needsUpdate: true, size: this.nplanes },
    			base1: { type: 'vec3', needsUpdate: false, size: this.nplanes },
    			base2: { type: 'vec3', needsUpdate: false, size: this.nplanes }
    		};

    		this.lightWeights([0, 0, 1], 'base');
    	}

    	/**
    	 * Computes basis-specific light weights
    	 * @param {number[]} light - Light direction vector
    	 * @param {string} basename - Uniform name for weights
    	 * @param {number} [time] - Animation time
    	 * @private
    	 */
    	lightWeights(light, basename, time) {
    		let value;
    		switch (this.type) {
    			case 'ptm': value = PTM.lightWeights(light); break;
    			case 'hsh': value = HSH.lightWeights(light); break;
    			case 'sh': value = SH.lightWeights(light); break;
    			case 'rbf': value = RBF.lightWeights(light, this); break;
    			case 'bln': value = BLN.lightWeights(light, this); break;
    		}
    		this.setUniform(basename, value, time);
    	}

    	baseLightOffset(p, l, k) {
    		return (p * this.ndimensions + l) * 3 + k;
    	}

    	basePixelOffset(p, x, y, k) {
    		return (p * this.resolution * this.resolution + (x + y * this.resolution)) * 3 + k;
    	}

    	loadBasis(data) {
    		let tmp = new Uint8Array(data);
    		this.basis = new Float32Array(data.length);

    		new Float32Array(tmp.length);
    		for (let plane = 0; plane < this.nplanes + 1; plane++) {
    			for (let c = 0; c < this.ndimensions; c++) {
    				for (let k = 0; k < 3; k++) {
    					let o = this.baseLightOffset(plane, c, k);
    					if (plane == 0)
    						this.basis[o] = tmp[o] / 255;
    					else
    						this.basis[o] = ((tmp[o] - 127) / this.material.range[plane - 1]);
    				}
    			}
    		}
    	}

    	fragShaderSrc(gl) {

    		let basetype = 'vec3'; //(this.colorspace == 'mrgb' || this.colorspace == 'mycc')?'vec3':'float';
    		let gl2 = !(gl instanceof WebGLRenderingContext);
    		let str = `


#define np1 ${this.nplanes + 1}

${gl2 ? 'in' : 'varying'} vec2 v_texcoord;

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

    		for (let n = 0; n < this.njpegs; n++)
    			str += `
uniform sampler2D plane${n};
`;

    		if (this.normals)
    			str += `
uniform sampler2D normals;
`;

    		if (this.colorspace == 'mycc')
    			str +=
    				`

const int ny0 = ${this.yccplanes[0]};
const int ny1 = ${this.yccplanes[1]};
`;

    		switch (this.colorspace) {
    			case 'lrgb': str += LRGB.render(this.njpegs, gl2); break;
    			case 'rgb': str += RGB.render(this.njpegs, gl2); break;
    			case 'mrgb': str += MRGB.render(this.njpegs, gl2); break;
    			case 'mycc': str += MYCC.render(this.njpegs, this.yccplanes[0], gl2); break;
    		}

    		str += `

vec4 data() {

`;
    		if (this.mode == 'light') {
    			str += `
	vec4 color = render(base);
`;
    		} else {
    			str += `
	vec4 color;
`;
    			if (this.normals)
    				str += `
	vec3 normal = (texture${gl2 ? '' : '2D'}(normals, v_texcoord).zyx *2.0) - 1.0;
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
    			switch (this.mode) {
    				case 'normals': str += `
	normal = (normal + 1.0)/2.0;
	color = vec4(0.0, normal.xy, 1);
`;
    					break;

    				case 'diffuse':
    					if (this.colorspace == 'lrgb' || this.colorspace == 'rgb')
    						str += `
vec4 diffuse = texture${gl2 ? '' : '2D'}(plane0, v_texcoord);
float s = dot(light, normal);
color = vec4(s * diffuse.xyz, 1);
`;
    					else
    						str += `
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

    		str += `return color;
}`;
    		return str;
    	}
    }


    class LRGB {
    	static render(njpegs, gl2) {
    		let str = `
vec4 render(vec3 base[np1]) {
	float l = 0.0;
`;
    		for (let j = 1, k = 0; j < njpegs; j++, k += 3) {
    			str += `
	{
		vec4 c = texture${gl2 ? '' : '2D'}(plane${j}, v_texcoord);
		l += base[${k}].x*(c.x - bias[${j}].x)*scale[${j}].x;
		l += base[${k + 1}].x*(c.y - bias[${j}].y)*scale[${j}].y;
		l += base[${k + 2}].x*(c.z - bias[${j}].z)*scale[${j}].z;
	}
`;
    		}
    		str += `
	vec3 basecolor = (texture${gl2 ? '' : '2D'}(plane0, v_texcoord).xyz - bias[0])*scale[0];

	return l*vec4(basecolor, 1);
}
`;
    		return str;
    	}
    }


    class RGB {
    	static render(njpegs, gl2) {
    		let str = `
vec4 render(vec3 base[np1]) {
	vec4 rgb = vec4(0, 0, 0, 1);`;

    		for (let j = 0; j < njpegs; j++) {
    			str += `
	{
		vec4 c = texture${gl2 ? '' : '2D'}(plane${j}, v_texcoord);
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
    		for (let j = 0; j < njpegs; j++) {
    			str +=
    				`	c = texture${gl2 ? '' : '2D'}(plane${j}, v_texcoord);
	r = (c.xyz - bias[${j}])* scale[${j}];

	rgb += base[${j}*3+1]*r.x;
	rgb += base[${j}*3+2]*r.y;
	rgb += base[${j}*3+3]*r.z;
`;
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
    		for (let j = 0; j < njpegs; j++) {
    			str += `

	c = texture${gl2 ? '' : '2D'}(plane${j}, v_texcoord);

	r = (c.xyz - bias[${j}])* scale[${j}];
`;

    			if (j < ny1) {
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

    /* FIXME  only HSH is exported, is this class complete? */
    /* PTM utility functions 
     */
    class PTM {
    	/* @param {Array} v expects light direction as [x, y, z]
    	*/
    	static lightWeights(v) {
    		let b = [1.0, v[0], v[1], v[0] * v[0], v[0] * v[1], v[1] * v[1]];
    		let base = new Float32Array(18);
    		for (let i = 0; i < 18; i++)
    			base[3 * i] = base[3 * i + 1] = base[3 * i + 2] = b[i];
    		return base;
    	}
    }


    /* HSH utility functions 
     */
    class HSH {
    	static minElevation = 0.15;
    	/* @param {Array} v expects light direction as [x, y, z]
    	*/
    	static lightWeights(v) {
    		let PI = 3.1415;
    		let phi = Math.atan2(v[1], v[0]);
    		if (phi < 0)
    			phi = 2 * PI + phi;
    		let theta = Math.min(Math.acos(v[2]), PI / 2 - this.minElevation);

    		let cosP = Math.cos(phi);
    		let cosT = Math.cos(theta);
    		let cosT2 = cosT * cosT;

    		let b = [
    			1.0 / Math.sqrt(2 * PI),

    			Math.sqrt(6 / PI) * (cosP * Math.sqrt(cosT - cosT2)),
    			Math.sqrt(3 / (2 * PI)) * (-1 + 2 * cosT),
    			Math.sqrt(6 / PI) * (Math.sqrt(cosT - cosT2) * Math.sin(phi)),

    			Math.sqrt(30 / PI) * (Math.cos(2 * phi) * (-cosT + cosT2)),
    			Math.sqrt(30 / PI) * (cosP * (-1 + 2 * cosT) * Math.sqrt(cosT - cosT2)),
    			Math.sqrt(5 / (2 * PI)) * (1 - 6 * cosT + 6 * cosT2),
    			Math.sqrt(30 / PI) * ((-1 + 2 * cosT) * Math.sqrt(cosT - cosT2) * Math.sin(phi)),
    			Math.sqrt(30 / PI) * ((-cosT + cosT2) * Math.sin(2 * phi))
    		];
    		let base = new Float32Array(27);
    		for (let i = 0; i < 27; i++)
    			base[3 * i] = base[3 * i + 1] = base[3 * i + 2] = b[i];
    		return base;
    	}
    }

    class SH {
    	/* @param {Array} v expects light direction as [x, y, z]
    	*/
    	static lightWeights(v) {
    		let PI = 3.1415;
    		let A = 0.5 * Math.sqrt(3.0 / PI);
    		let B = 0.5 * Math.sqrt(15 / PI);
    		let b = [
    			0.5 / Math.sqrt(PI),
    			A * v[0],
    			A * v[2],
    			A * v[1],
    			B * v[0] * v[1],
    			B * v[0] * v[2],
    			0.5 * Math.sqrt(5 / PI) * (3 * v[2] * v[2] - 1),
    			B * v[1] * v[2],
    			0.5 * B * (v[1] * v[1] - v[0] * v[0])
    		];

    		let base = new Float32Array(27);
    		for (let i = 0; i < 27; i++)
    			base[3 * i] = base[3 * i + 1] = base[3 * i + 2] = b[i];
    		return base;
    	}
    }

    class AdaptiveRBF {
    	constructor(samples, alpha = 1.0, k = 5) {
    		this.samples = samples;
    		this.alpha = alpha; //how smooth is the interpolation
    		this.beta = 2;  //filtering smooth distance (higher will result in bumpy);
    	}

    	distance(a, b) {
    		const dx = a[0] - b[0], dy = a[1] - b[1], dz = a[2] - b[2];
    		return Math.sqrt(dx * dx + dy * dy + dz * dz);
    	}

    	smoothMinDist(neighbors) {
    		let num = 0, denom = 0;
    		for (const { dist } of neighbors) {
    			const weight = Math.exp(-this.beta * dist);
    			num += dist * weight;
    			denom += weight;
    		}
    		return num / denom;
    	}

    	findNeighbors(x, y, z) {
    		return this.samples
    			.map((s, i) => ({ index: i, dist: this.distance(s, [x, y, z]) }))
    			.sort((a, b) => a.dist - b.dist)
    	}

    	rbf(r, epsilon) {
    		return Math.exp(-epsilon * r * r);
    	}

    	weights(x, y, z) {
    		const neighbors = this.findNeighbors(x, y, z);

    		const meanDist =  this.smoothMinDist(neighbors);
    		const epsilon = this.alpha / meanDist;

    		let denom = 0;
    		let weights = [];
    		for (const { index, dist } of neighbors) {
    			const w = this.rbf(dist, epsilon);
    			weights.push([index, w]);
    			denom += w;
    		}

    		for(let w of weights)
    			w[1] /= denom;
    		return weights;
    	}
    }


    class RBF {
    	/* @param {Array} v expects light direction as [x, y, z]
    	*/
    	static lightWeights(lpos, shader) {

    		let weights = RBF.rbf(lpos, shader);

    		let np = shader.nplanes;
    		let lweights = new Float32Array((np + 1) * 3);

    		for (let p = 0; p < np + 1; p++) {
    			for (let k = 0; k < 3; k++) {
    				for (let l = 0; l < weights.length; l++) {
    					let o = shader.baseLightOffset(p, weights[l][0], k);
    					lweights[3 * p + k] += weights[l][1] * shader.basis[o];
    				}
    			}
    		}
    		return lweights;
    	}

    	static rbf(lpos, shader) {
    		let weights = new Array(shader.ndimensions);

    		let samples = new Array(shader.ndimensions);

    		for (let i = 0; i < weights.length; i++) {
    			let dx = shader.lights[i * 3 + 0];
    			let dy = shader.lights[i * 3 + 1];
    			let dz = shader.lights[i * 3 + 2];

    			samples[i] = [dx, dy, dz];
    		}

    		const rbf = new AdaptiveRBF(samples, 8, 24);
    		return rbf.weights(lpos[0], lpos[1], lpos[2]);
    		
    	}
    }

    class BLN {
    	static lightWeights(lpos, shader) {
    		let np = shader.nplanes;
    		let s = Math.abs(lpos[0]) + Math.abs(lpos[1]) + Math.abs(lpos[2]);

    		//rotate 45 deg.
    		let x = (lpos[0] + lpos[1]) / s;
    		let y = (lpos[1] - lpos[0]) / s;
    		x = (x + 1.0) / 2.0;
    		y = (y + 1.0) / 2.0;
    		x = x * (shader.resolution - 1.0);
    		y = y * (shader.resolution - 1.0);

    		let sx = Math.min(shader.resolution - 2, Math.max(0, Math.floor(x)));
    		let sy = Math.min(shader.resolution - 2, Math.max(0, Math.floor(y)));
    		let dx = x - sx;
    		let dy = y - sy;

    		//bilinear interpolation coefficients.
    		let s00 = (1 - dx) * (1 - dy);
    		let s10 = dx * (1 - dy);
    		let s01 = (1 - dx) * dy;
    		let s11 = dx * dy;

    		let lweights = new Float32Array((np + 1) * 3);

    		//TODO optimize away basePixel

    		for (let p = 0; p < np + 1; p++) {
    			for (let k = 0; k < 3; k++) {
    				let o00 = shader.basePixelOffset(p, sx, sy, k);
    				let o10 = shader.basePixelOffset(p, sx + 1, sy, k);
    				let o01 = shader.basePixelOffset(p, sx, sy + 1, k);
    				let o11 = shader.basePixelOffset(p, sx + 1, sy + 1, k);

    				lweights[3 * p + k] =
    					s00 * shader.basis[o00] +
    					s10 * shader.basis[o10] +
    					s01 * shader.basis[o01] +
    					s11 * shader.basis[o11];

    			}
    		}
    		return lweights;
    	}
    }

    /**
     * @typedef {Object} LayerRTIOptions
     * @property {string} url - URL to RTI info.json file (required)
     * @property {string} layout - Layout type: 'image', 'deepzoom', 'google', 'iiif', 'zoomify', 'tarzoom', 'itarzoom'
     * @property {boolean} [normals=false] - Whether to load normal maps
     * @property {string} [server] - IIP server URL (for IIP layout)
     * @property {number} [worldRotation=0] - Global rotation offset
     * @extends LayerOptions
     */

    /**
     * LayerRTI implements Reflectance Transformation Imaging (RTI) visualization.
     * 
     * RTI is an imaging technique that captures surface reflectance data to enable
     * interactive relighting of an object from different directions. The layer handles
     * the 'relight' data format, which consists of:
     * 
     * - info.json: Contains RTI parameters and configuration
     * - plane_*.jpg: Series of coefficient images
     * - normals.jpg: Optional normal map (when using normals=true)
     * 
     * Features:
     * - Interactive relighting
     * - Multiple layout support
     * - Normal map integration
     * - Light direction control
     * - Animation support
     * - World rotation handling
     * 
     * Technical Details:
     * - Uses coefficient-based relighting
     * - Supports multiple image planes
     * - Handles various tiling schemes
     * - Manages WebGL resources
     * - Coordinates light transformations
     * 
     * Data Format Support:
     * - Relight JSON configuration
     * - Multiple layout systems
     * - JPEG coefficient planes
     * - Optional normal maps
     * - IIP image protocol
     * 
     * @extends Layer
     * 
     * @example
     * ```javascript
     * // Create RTI layer with deepzoom layout
     * const rtiLayer = new OpenLIME.Layer({
     *   type: 'rti',
     *   url: 'path/to/info.json',
     *   layout: 'deepzoom',
     *   normals: true
     * });
     * 
     * // Add to viewer
     * viewer.addLayer('rti', rtiLayer);
     * 
     * // Change light direction with animation
     * rtiLayer.setLight([0.5, 0.5], 1000);
     * ```
     * 
     * @see {@link https://github.com/cnr-isti-vclab/relight|Relight on GitHub}
     */
    class LayerRTI extends Layer {
    	/**
    	 * Creates a new LayerRTI instance
    	 * @param {LayerRTIOptions} options - Configuration options
    	 * @throws {Error} If rasters options is not empty
    	 * @throws {Error} If url is not provided
    	 */
    	constructor(options) {
    		super(options);

    		if (Object.keys(this.rasters).length != 0)
    			throw "Rasters options should be empty!";

    		if (!this.url)
    			throw "Url option is required";

    		this.shaders['rti'] = new ShaderRTI({ normals: this.normals });
    		this.setShader('rti');

    		this.addControl('light', [0, 0]);
    		this.worldRotation = 0; //if the canvas or ethe layer rotate, light direction neeeds to be rotated too.

    		this.loadJson(this.url);
    	}

    	/**
    	 * Constructs URL for image plane resources based on layout type
    	 * @param {string} url - Base URL
    	 * @param {string} plane - Plane identifier
    	 * @returns {string} Complete URL for the resource
    	 * @private
    	 */
    	imageUrl(url, plane) {
    		let path = this.url.substring(0, this.url.lastIndexOf('/') + 1);
    		switch (this.layout.type) {
    			case 'image': return path + plane + '.jpg';			case 'google': return path + plane;			case 'deepzoom': return path + plane + '.dzi';			case 'tarzoom': return path + plane + '.tzi';			case 'itarzoom': return path + 'planes.tzi';			case 'zoomify': return path + plane + '/ImageProperties.xml';			case 'iip': return url;			case 'iiif': throw Error("Unimplemented");
    			default: throw Error("Unknown layout: " + layout.type);
    		}
    	}

    	/**
    	 * Sets the light direction with optional animation
    	 * @param {number[]} light - Light direction vector [x, y]
    	 * @param {number} [dt] - Animation duration in milliseconds
    	 */
    	setLight(light, dt) {
    		this.setControl('light', light, dt);
    	}

    	/**
    	 * Loads and processes RTI configuration
    	 * @param {string} url - URL to info.json
    	 * @private
    	 * @async
    	 */
    	loadJson(url) {
    		(async () => {
    			let infoUrl = url;

    			// Need to handle embedded RTI info.json when using IIP and TIFF image stacks
    			if (this.layout.type == "iip") infoUrl = (this.server ? this.server + '?FIF=' : '') + url + "&obj=description";

    			var response = await fetch(infoUrl);
    			if (!response.ok) {
    				this.status = "Failed loading " + infoUrl + ": " + response.statusText;
    				return;
    			}
    			let json = await response.json();

    			// Update layout image format and pixelSize if provided in info.json
    			this.layout.suffix = json.format;
    			if (json.pixelSizeInMM) this.pixelSize = json.pixelSizeInMM;

    			this.shader.init(json);
    			let urls = [];
    			for (let p = 0; p < this.shader.njpegs; p++) {
    				let imageUrl = this.layout.imageUrl(url, 'plane_' + p);
    				urls.push(imageUrl);
    				let raster = new Raster({ format: 'vec3' });
    				this.rasters.push(raster);
    			}
    			if (this.normals) { // ITARZOOM must include normals and currently has a limitation: loads the entire tile
    				let imageUrl = this.layout.imageUrl(url, 'normals');
    				urls.push(imageUrl);
    				let raster = new Raster({ format: 'vec3' });
    				this.rasters.push(raster);
    			}
    			this.layout.setUrls(urls);

    		})().catch(e => { console.log(e); this.status = e; });
    	}

    	/**
    	 * Updates light direction based on control state
    	 * Handles world rotation transformations
    	 * @returns {boolean} Whether interpolation is complete
    	 * @override
    	 * @private
    	 */
    	interpolateControls() {
    		let done = super.interpolateControls();
    		if (!done) {
    			let light = this.controls['light'].current.value;
    			//this.shader.setLight(light);
    			let rotated = Transform.rotate(light[0], light[1], this.worldRotation * Math.PI);
    			this.shader.setLight([rotated.x, rotated.y]);
    		}
    		return done;
    	}

    	/**
    	 * Renders the RTI visualization
    	 * Updates world rotation and manages drawing
    	 * @param {Transform} transform - Current view transform
    	 * @param {Object} viewport - Current viewport
    	 * @returns {boolean} Whether render completed successfully
    	 * @override
    	 * @private
    	 */
    	draw(transform, viewport) {
    		this.worldRotation = transform.a + this.transform.a;
    		return super.draw(transform, viewport);
    	}
    }

    /**
     * Register this layer type with the Layer factory
     * @type {Function}
     * @private
     */
    Layer.prototype.types['rti'] = (options) => { return new LayerRTI(options); };

    /**
     * @typedef {Object} ShaderNeural~NetworkConfig
     * Configuration for neural network weights and parameters
     * @property {number} n - Number of neurons per layer (padded to multiple of 4)
     * @property {number} c - Number of input channels (padded to multiple of 4)
     * @property {string} colorspace - Color space for processing ('rgb'|'xyz'|etc)
     * @property {number} nplanes - Number of coefficient planes
     * @property {number} scale - Dequantization scale factor
     * @property {number} bias - Dequantization bias
     */

    /**
     * ShaderNeural implements a WebGL-based neural network for real-time image relighting.
     * Used in conjunction with LayerNeuralRTI for Neural Reflectance Transformation Imaging.
     * 
     * Features:
     * - Three-layer neural network architecture
     * - Real-time image relighting
     * - Multiple texture plane support
     * - Configurable network parameters
     * - ELU activation function
     * - WebGL acceleration
     * - Automatic color space conversion
     * 
     * Technical Implementation:
     * - Forward pass computation in fragment shader
     * - Vectorized operations for performance
     * - Dynamic shader generation based on network size
     * - Multi-texture sampling
     * - Weight matrix management
     * - Dequantization support
     * 
    /**
     * Neural Network Architecture Details
     * 
     * The network consists of three layers:
     * 1. Input Layer:
     *    - Accepts coefficient planes and light direction
     *    - Applies dequantization and normalization
     * 
     * 2. Hidden Layers:
     *    - Two fully connected layers
     *    - ELU activation function
     *    - Vectorized operations for efficiency
     * 
     * 3. Output Layer:
     *    - Produces final RGB/XYZ color
     *    - Linear activation
     * 
     * Implementation Notes:
     * - All matrices are packed into vec4 for efficient GPU processing
     * - Network dimensions are padded to multiples of 4
     * - Uses texture sampling for coefficient input
     * - Implements forward pass only
     *
     *
     * Example usage with LayerNeuralRTI:
     * ```javascript
     * // Create neural shader
     * const shader = new ShaderNeural({
     *     mode: 'light',
     *     nplanes: 9
     * });
     * 
     * // Configure network
     * shader.setShaderInfo(samples, 9, 52, 12, 'rgb');
     * 
     * // Update weights
     * shader.setUniform('layer1_weights', weights1);
     * shader.setUniform('layer1_biases', biases1);
     * // ... set other layers
     * 
     * // Set light direction
     * shader.setLight([0.5, 0.3]);
     * ```
     *
     * Fragment Shader Implementation
     * 
     * Key Components:
     * 1. Input Processing:
     *    - Texture sampling
     *    - Dequantization
     *    - Light direction incorporation
     * 
     * 2. Network Computation:
     *    - Vectorized matrix multiplication
     *    - ELU activation function
     *    - Layer-wise processing
     * 
     * 3. Output Processing:
     *    - Color space conversion
     *    - Final color computation
     * 
     * Uniforms:
     * - {sampler2D} u_texture_[1-3] - Coefficient plane textures
     * - {vec2} lights - Light direction vector
     * - {vec4[]} layer[1-3]_weights - Layer weight matrices
     * - {vec4[]} layer[1-3]_biases - Layer bias vectors
     * - {vec3} min - Minimum values for dequantization
     * - {vec3} max - Maximum values for dequantization
     * 
     * @extends Shader
     */
    class ShaderNeural extends Shader {
    	/**
    	 * Creates a new neural network shader
    	 * @param {Object} [options] - Configuration options
    	 * @param {string[]} [options.modes=['light']] - Available modes
    	 * @param {string} [options.mode='light'] - Initial mode
    	 * @param {number} [options.nplanes=null] - Number of coefficient planes
    	 * @param {number} [options.scale=null] - Dequantization scale factor
    	 * @param {number} [options.bias=null] - Dequantization bias
    	 */
    	constructor(options) {
    		super({});

    		Object.assign(this, {
    			modes: ['light'],
    			mode: 'light',

    			nplanes: null,	 //number of coefficient planes

    			scale: null,	  //factor and bias are used to dequantize coefficient planes.
    			bias: null,

    		});
    		Object.assign(this, options);

    		this.samplers = [
    			{ id: 1, name: 'u_texture_1', type: 'vec3' },
    			{ id: 2, name: 'u_texture_2', type: 'vec3' },
    			{ id: 3, name: 'u_texture_3', type: 'vec3' }
    		];

    		this.uniforms = {
    			lights: { type: 'vec2', needsUpdate: true, size: 2, value: [0.0, 0.0] },
    			min: { type: 'vec3', needsUpdate: true, size: 3, value: [0, 0, 0] },
    			max: { type: 'vec3', needsUpdate: true, size: 3, value: [1, 1, 1] },
    			layer1_weights: { type: 'vec4', needsUpdate: true, size: this.c * this.n / 4 },
    			layer1_biases: { type: 'vec4', needsUpdate: true, size: this.n / 4 },
    			layer2_weights: { type: 'vec4', needsUpdate: true, size: this.n * this.n / 4 },
    			layer2_biases: { type: 'vec4', needsUpdate: true, size: this.n / 4 },
    			layer3_weights: { type: 'vec4', needsUpdate: true, size: this.n * 3 / 4 },
    			layer3_biases: { type: 'vec3', needsUpdate: true, size: 1 },
    		};
    	}

    	/**
    	 * Creates WebGL program and retrieves attribute locations
    	 * @param {WebGLRenderingContext} gl - WebGL context
    	 * @override
    	 * @private
    	 */
    	createProgram(gl) {
    		super.createProgram(gl);
    		this.position_location = gl.getAttribLocation(this.program, "a_position");
    		this.texcoord_location = gl.getAttribLocation(this.program, "a_texcoord");
    	}

    	/**
    	 * Sets the light direction for relighting
    	 * @param {number[]} light - Light direction vector [x, y]
    	 */
    	setLight(light) {
    		this.setUniform('lights', light);
    	}

    	/**
    	 * Initializes default weights
    	 */
    	init() {
    		this.lightWeights([0, 0, 1], 'base');
    	}

    	/**
    	 * Configures shader for specific network architecture
    	 * @param {number[]} samples - Input samples
    	 * @param {number} planes - Number of coefficient planes
    	 * @param {number} n - Neurons per layer
    	 * @param {number} c - Input channels
    	 * @param {string} colorspace - Color space for processing
    	 */
    	setShaderInfo(samples, planes, n, c, colorspace) {
    		this.samples = samples;
    		this.planes = planes;
    		this.n = n;
    		this.c = c;
    		this.colorspace = colorspace;
    	}

    	/**
    	 * Generates vertex shader source code
    	 * @param {WebGLRenderingContext} gl - WebGL context
    	 * @returns {string} Vertex shader source
    	 * @private
    	 */
    	vertShaderSrc(gl) {
    		return `#version 300 es
in vec2 a_position;
in vec2 a_texcoord;
out vec2 v_texcoord;
void main() {
	gl_Position = vec4(a_position, 0.0, 1.0);
	v_texcoord = a_texcoord;
}`;
    	}

    	/**
    	 * Generates fragment shader source code implementing neural network
    	 * @param {WebGLRenderingContext} gl - WebGL context
    	 * @returns {string} Fragment shader source
    	 * @private
    	 */
    	fragShaderSrc(gl) {
    		return `
vec4 inputs[${this.c / 4}];    // 12/4
vec4 output1[${this.n / 4}];  // 52/4
vec4 output2[${this.n / 4}];  // 52/4
vec3 output3;

in vec2 v_texcoord;
uniform sampler2D u_texture_1;
uniform sampler2D u_texture_2;
uniform sampler2D u_texture_3;
uniform vec2 lights;

uniform vec4 layer1_weights[${this.c * this.n / 4}]; // 12*52/4
uniform vec4 layer1_biases[${this.n / 4}];  // 52/4
uniform vec4 layer2_weights[${this.n * this.n / 4}]; // 52*52/4
uniform vec4 layer2_biases[${this.n / 4}];  // 52/4
uniform vec4 layer3_weights[${this.n * 3 / 4}];  // 52*3/4
uniform vec3 layer3_biases;

uniform vec3 min[${this.planes / 3}];
uniform vec3 max[${this.planes / 3}];

float elu(float a){
	return (a > 0.0) ? a : (exp(a) - 1.0);
}


vec4 relightCoeff(vec3 color_1, vec3 color_2, vec3 color_3) {
	// Rescaling features
    color_1 = color_1 * (max[0] - min[0]) + min[0];
    color_2 = color_2 * (max[1] - min[1]) + min[1];
    color_3 = color_3 * (max[2] - min[2]) + min[2];

	// building input
	inputs[0] = vec4(color_1, color_2.x);
	inputs[1] = vec4(color_2.yz, color_3.xy);
	inputs[2] = vec4(color_3.z, lights, 0.0);

	float sum = 0.0;

	// layer 1 - 11 x 49
	for (int i=0; i < ${this.n}; i++){
		sum = 0.0;
		for (int j=0; j < ${this.c / 4}; j++){
			sum += dot(inputs[j], layer1_weights[${this.c / 4}*i+j]);
		}
		output1[i/4][i%4] = elu(sum + layer1_biases[i/4][i%4]);
	}
	
	// layer 2 - 49 x 49
	for (int i=0; i < ${this.n}; i++){
		sum = 0.0;
		for (int j=0; j < ${this.n / 4}; j++){
			sum += dot(output1[j], layer2_weights[${this.n / 4}*i+j]);
		}
		output2[i/4][i%4] = elu(sum + layer2_biases[i/4][i%4]);
	}

	// layer 3 - 49 x 3
	for (int i=0; i < 3; i++){
		sum = 0.0;
		for (int j=0; j < ${this.n / 4}; j++){
			sum += dot(output2[j], layer3_weights[${this.n / 4}*i+j]);
		}
		output3[i] = sum + layer3_biases[i];
	}
	return vec4(output3.${this.colorspace}, 1.0);
}

vec4 relight(vec2 v) {
	vec3 color_1 = texture(u_texture_1, v).${this.colorspace};
	vec3 color_2 = texture(u_texture_2, v).${this.colorspace};
	vec3 color_3 = texture(u_texture_3, v).${this.colorspace};
	return relightCoeff(color_1, color_2, color_3);
}


vec4 data() {
	return relight(v_texcoord);
}
vec4 data1() {
	vec2 uv = v_texcoord;
	bool showDiff = false;
	bool showA = false;
	if(v_texcoord.x > 0.5) {
		showDiff = true;
		uv.x -= 0.5;
	}
	if(v_texcoord.y > 0.5) {
		showA = true;
		uv.y -= 0.5;
	}
	vec2 o = floor(uv*128.0)/128.0;
	float step = 1.0/256.0;

	vec4 a = vec4(0, 0, 0, 0);
	vec3 color_1 = vec3(0, 0, 0);
	vec3 color_2 = vec3(0, 0, 0);
	vec3 color_3 = vec3(0, 0, 0);

	for(float y = 0.0; y <= step; y = y + step) {
		for(float x = 0.0; x <= step; x = x + step) {
			vec2 d = o + vec2(x, y);
			a += 0.25*relight(d);

			color_1 += texture(u_texture_1, d).${this.colorspace};
			color_2 += texture(u_texture_2, d).${this.colorspace};
			color_3 += texture(u_texture_3, d).${this.colorspace};
		}
	}
	vec4 b = relightCoeff(0.25*color_1, 0.25*color_2, 0.25*color_3);
	float diff = 255.0*length((a - b).xyz);
	if(showDiff) {
		if(diff < 10.0) {
			return vec4(0.0, 0.0, 0.0, 1.0);
		} else if (diff < 20.0) {
			return vec4(0.0, 0.0, 1.0, 1.0);
		} else if(diff < 40.0) {
			return vec4(0.0, 1.0, 0.0, 1.0);
		} else
			return vec4(1.0, 0.0, 0.0, 1.0);
	} 
	if(showA)
		return a;
	return b;
}

		`;
    	}
    }

    /**
     * @typedef {Object} LayerNeuralRTIOptions
     * @property {string} url - URL to the Neural RTI configuration JSON
     * @property {Layout} layout - Layout system for image loading
     * @property {number} [convergenceSpeed=1.2] - Speed of quality convergence
     * @property {number} [maxTiles=40] - Maximum number of tiles to process
     * @property {string} [colorspace='rgb'] - Color space for processing
     * @extends LayerOptions
     */

    /**
     * LayerNeuralRTI implements real-time Neural Reflectance Transformation Imaging.
     * This layer uses a neural network to perform real-time relighting of images,
     * offering improved quality and performance compared to traditional RTI approaches.
     * 
     * Features:
     * - Neural network-based relighting
     * - Adaptive quality scaling
     * - Frame rate optimization
     * - Progressive refinement
     * - Multi-plane texture support
     * - WebGL acceleration
     * 
     * Technical Details:
     * - Uses 3-layer neural network
     * - Supports multiple color spaces
     * - Implements adaptive tile processing
     * - Handles dynamic quality adjustment
     * - Manages frame buffer operations
     * - Coordinates light transformations
     * 
     * Performance Optimizations:
     * - Dynamic resolution scaling
     * - FPS-based quality adjustment
     * - Progressive refinement system
     * - Tile caching
     * - Batch processing
     * 
     * @extends Layer
     * 
    	* @example
     * ```javascript
     * // Create Neural RTI layer
     * const neuralRTI = new OpenLIME.Layer({
     *   type: 'neural',
     *   url: 'config.json',
     *   layout: 'deepzoom',
     *   convergenceSpeed: 1.2,
     *   maxTiles: 40
     * });
     * 
     * // Add to viewer
     * viewer.addLayer('rti', neuralRTI);
     * 
     * // Change light direction
     * neuralRTI.setLight([0.5, 0.3], 1000);
     * ```
     */
    class LayerNeuralRTI extends Layer {
    	/**
    	 * Creates a new LayerNeuralRTI instance
    	 * @param {LayerNeuralRTIOptions} options - Configuration options
    	 */
    	constructor(options) {
    		super(options || {});
    		this.currentRelightFraction = 1.0; //(min: 0, max 1)
    		this.maxTiles = 40;
    		this.relighted = false;
    		this.convergenceSpeed = 1.2;
    		this.addControl('light', [0, 0]);
    		this.worldRotation = 0; //if the canvas or ethe layer rotate, light direction neeeds to be rotated too.

    		let textureUrls = [
    			null,
    			this.layout.imageUrl(this.url, 'plane_1'),
    			this.layout.imageUrl(this.url, 'plane_2'),
    			this.layout.imageUrl(this.url, 'plane_3'),
    		];

    		this.layout.setUrls(textureUrls);

    		for (let url of textureUrls) {
    			let raster = new Raster({ format: 'vec3' });
    			this.rasters.push(raster);
    		}

    		this.imageShader = new Shader({
    			'label': 'Rgb',
    			'samplers': [{ id: 0, name: 'kd', type: 'vec3', load: false }]
    		});

    		this.neuralShader = new ShaderNeural();

    		this.shaders = { 'standard': this.imageShader, 'neural': this.neuralShader };
    		this.setShader('neural');
    		this.neuralShader.setLight([0, 0]);


    		(async () => { await this.loadNeural(this.url); })();
    	}

    	/**
    	 * Sets light direction with optional animation
    	 * @param {number[]} light - Light direction vector [x, y]
    	 * @param {number} [dt] - Animation duration in milliseconds
    	 */
    	setLight(light, dt) {
    		this.setControl('light', light, dt);
    	}

    	/** @ignore */
    	loadTile(tile, callback) {
    		this.shader = this.neuralShader;
    		super.loadTile(tile, callback);
    	}

    	/**
    	 * Loads neural network configuration and weights
    	 * @param {string} url - URL to configuration JSON
    	 * @private
    	 * @async
    	 */
    	async loadNeural(url) {
    		await this.initialize(url);
    	}

    	/**
    	 * Initializes neural network parameters
    	 * @param {string} json_url - URL to configuration
    	 * @private
    	 * @async
    	 */
    	async initialize(json_url) {

    		const info = await this.loadJSON(json_url);
    		this.max = info.max.flat(1);
    		this.min = info.min.flat(1);

    		this.width = info.width;
    		this.height = info.height;

    		let parameters = {};
    		for (let i = 0; i < 3; i++) {
    			let key = 'layer' + (i + 1);
    			parameters[key + '_weights'] = info.weights[i];//(await this.loadJSON(data_path + "/parameters/" + w + "_weights.json")).flat(1);
    			parameters[key + '_biases'] = info.biases[i]; //(await this.loadJSON(data_path + "/parameters/" + w + "_biases.json")).flat(1);
    		}

    		for (const [name, value] of Object.entries(parameters))
    			this.neuralShader.setUniform(name, value);

    		//this.neuralShader.updateUniforms(gl, this.neuralShader.program);
    		this.neuralShader.setUniform('min', this.min);
    		this.neuralShader.setUniform('max', this.max);

    		// make the fragment shader flexible to different network configurations
    		let n = info.samples;
    		let c = info.planes + 2;
    		while (n % 4 != 0)
    			n++;
    		while (c % 4 != 0)
    			c++;
    		this.neuralShader.setShaderInfo(info.samples, info.planes, n, c, info.colorspace);

    		this.networkParameters = parameters;
    	}

    	/** @ignore */
    	setCoords() {
    		let gl = this.gl;

    		let coords = new Float32Array([-1, -1, -1, 1, 1, 1, 1, -1]);

    		this.coords_buffer = gl.createBuffer();
    		gl.bindBuffer(gl.ARRAY_BUFFER, this.coords_buffer);
    		gl.bufferData(gl.ARRAY_BUFFER, coords, gl.STATIC_DRAW);

    		let texCoords = new Float32Array([0, 0, 0, 1, 1, 1, 1, 0]);
    		this.texCoords_buffer = gl.createBuffer();
    		gl.bindBuffer(gl.ARRAY_BUFFER, this.texCoords_buffer);
    		gl.bufferData(gl.ARRAY_BUFFER, texCoords, gl.STATIC_DRAW);
    	}


    	// little set of functions to get model, coeff and info
    	/** @ignore */
    	async loadJSON(info_file) {
    		const info_response = await fetch(info_file);
    		const info = await info_response.json();
    		return info;
    	}

    	/* ************************************************************************** */

    	/**
    	 * Renders the Neural RTI visualization
    	 * Handles quality adaptation and progressive refinement
    	 * @param {Transform} transform - Current view transform
    	 * @param {Object} viewport - Current viewport
    	 * @returns {boolean} Whether render completed
    	 * @override
    	 * @private
    	 */
    	draw(transform, viewport) {
    		//TODO this is duplicated code. move this check up 
    		if (this.status != 'ready')
    			return true;

    		this.worldRotation = transform.a + this.transform.a;

    		if (this.networkParameters !== undefined) {

    			let previousRelightFraction = this.relightFraction;
    			//adjust maxTiles to presserve framerate only when we had a draw which included relighting (but not a refine operation!).
    			if (this.relighted) {
    				if (this.canvas.fps > this.canvas.targetfps * 1.5) {
    					this.currentRelightFraction = Math.min(1.0, this.currentRelightFraction * this.convergenceSpeed);
    					//console.log('fps fast: ', this.canvas.fps, this.currentRelightFraction);
    				} else if (this.canvas.fps < this.canvas.targetfps * 0.75) {
    					this.currentRelightFraction = Math.max(this.currentRelightFraction / this.convergenceSpeed, 1 / 128);
    					this.convergenceSpeed = Math.max(1.05, Math.pow(this.convergenceSpeed, 0.9));
    					console.log('fps slow: ', this.canvas.fps, this.currentRelightFraction);
    				}
    			}
    			//this.refine = true;

    			//setup final refinement
    			if (this.refineTimeout)
    				clearTimeout(this.refineTimeout);

    			if (this.currentRelightFraction < 0.75 && this.refine == false)
    				this.refineTimeout = setTimeout(() => { this.emit('update'); this.refine = true; }, Math.max(400, 4000 / this.canvas.fps));

    			this.relightFraction = this.refine ? 1.0 : this.currentRelightFraction;
    			this.relightFraction = Math.round(this.relightFraction * 8) / 8;

    			let sizeChanged = this.relightFraction != previousRelightFraction;

    			let w = Math.round((this.layout.tilesize || this.layout.width) * this.relightFraction);
    			let h = Math.round((this.layout.tilesize || this.layout.height) * this.relightFraction);

    			//console.log("Canvas fps: ", this.canvas.fps, "relighted: ", this.relighted, "Refine? ", this.refine, " fraction: ", this.relightFraction, " w: ", this.tileRelightWidth);
    			this.refine = false;

    			let available = this.layout.available(viewport, transform, this.transform, 0, this.mipmapBias, this.tiles);

    			let tiles = Object.values(available);
    			if (tiles.length == 0)
    				return;
    			if (sizeChanged)
    				for (let tile of tiles)
    					tile.neuralUpdated = false;

    			this.relighted = false;
    			this.totTiles = 0;
    			this.totPixels = 0;
    			for (let tile of tiles) {
    				if (tile.neuralUpdated && !sizeChanged)
    					continue;
    				if (!this.relighted) {
    					this.relighted = true; //update fps next turn.
    					this.preRelight([viewport.x, viewport.y, viewport.dx, viewport.dy], w, h, sizeChanged);
    				}
    				this.relightTile(tile, w, h, sizeChanged);
    				this.totPixels += w * h;
    				this.totTiles += 1;
    			}
    			if (this.relighted)
    				this.postRelight();

    			this.relighted = this.relighted && !this.refine; //udpate fps only if not refined.
    		}

    		this.shader = this.imageShader;
    		let done = super.draw(transform, viewport);
    		this.shader = this.neuralShader;

    		return done;
    	}

    	/**
    	 * Prepares WebGL resources for relighting
    	 * @param {number[]} viewport - Viewport parameters
    	 * @param {number} w - Width for processing
    	 * @param {number} h - Height for processing
    	 * @private
    	 */
    	preRelight(viewport, w, h) {
    		let gl = this.gl;

    		if (!this.neuralShader.program) {
    			this.neuralShader.createProgram(gl);
    			gl.useProgram(this.neuralShader.program);
    			for (var i = 0; i < this.neuralShader.samplers.length; i++)
    				gl.uniform1i(this.neuralShader.samplers[i].location, i);
    		} else
    			gl.useProgram(this.neuralShader.program);

    		this.neuralShader.updateUniforms(gl);

    		if (!this.coords_buffer)
    			this.setCoords();

    		gl.bindBuffer(gl.ARRAY_BUFFER, this.coords_buffer);
    		gl.vertexAttribPointer(this.neuralShader.position_location, 2, gl.FLOAT, false, 0, 0);

    		gl.bindBuffer(gl.ARRAY_BUFFER, this.texCoords_buffer);
    		gl.vertexAttribPointer(this.neuralShader.texcoord_location, 2, gl.FLOAT, false, 0, 0);

    		gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);
    		gl.enable(gl.BLEND);

    		if (!this.framebuffer)
    			this.framebuffer = gl.createFramebuffer();
    		gl.bindFramebuffer(gl.FRAMEBUFFER, this.framebuffer);


    		//save previous viewport
    		this.backupViewport = viewport;
    		gl.viewport(0, 0, w, h);
    	}

    /**
     * Finalizes the relighting pass and restores rendering state
     * 
     * This method performs cleanup after relighting operations by:
     * 1. Unbinding the framebuffer to return to normal rendering
     * 2. Restoring the original viewport dimensions
     * 
     * Technical details:
     * - Restores WebGL rendering state
     * - Returns framebuffer binding to default
     * - Resets viewport to original dimensions
     * - Must be called after all tiles have been processed
     * 
     * @private
     * @see preRelight - Called at start of relighting process
     * @see relightTile - Called for each tile during relighting
     */
    	postRelight() {
    		let gl = this.gl;
    		gl.bindFramebuffer(gl.FRAMEBUFFER, null);

    		//restore previous viewport
    		let v = this.backupViewport;
    		this.gl.viewport(v[0], v[1], v[2], v[3]);
    	}

    	/**
    	 * Processes individual tile using neural network
    	 * @param {Object} tile - Tile to process
    	 * @param {number} w - Processing width
    	 * @param {number} h - Processing height
    	 * @param {boolean} sizeChanged - Whether tile size changed
    	 * @private
    	 */
    	relightTile(tile, w, h, sizeChanged) {
    		let gl = this.gl;


    		let needsCreate = tile.tex[0] == null;
    		if (needsCreate) {
    			let tex = tile.tex[0] = gl.createTexture();
    			gl.bindTexture(gl.TEXTURE_2D, tex);
    			// set the filtering so we don't need mips
    			gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
    			gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    			gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    		}
    		if (sizeChanged || needsCreate) {
    			gl.bindTexture(gl.TEXTURE_2D, tile.tex[0]);
    			// define size and format of level 0
    			gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA,
    				w, h, 0,
    				gl.RGBA, gl.UNSIGNED_BYTE, null);

    			//gl.bindTexture(gl.TEXTURE_2D, null);
    		}

    		for (var i = 0; i < this.neuralShader.samplers.length; i++) {
    			let id = this.neuralShader.samplers[i].id;
    			gl.activeTexture(gl.TEXTURE0 + i);
    			gl.bindTexture(gl.TEXTURE_2D, tile.tex[id]);
    		}

    		gl.framebufferTexture2D(gl.FRAMEBUFFER, gl.COLOR_ATTACHMENT0,
    			gl.TEXTURE_2D, tile.tex[0], 0);
    		gl.drawArrays(gl.TRIANGLE_FAN, 0, 4);

    		tile.neuralUpdated = true;
    	}

    	/**
    	 * Updates light direction and marks tiles for update
    	 * @returns {boolean} Whether updates are complete
    	 * @override
    	 * @private
    	 */
    	interpolateControls() {
    		let done = super.interpolateControls();
    		if (done)
    			return true;

    		let light = this.controls['light'].current.value;
    		let rotated = Transform.rotate(light[0], light[1], this.worldRotation * Math.PI);
    		light = [rotated.x, rotated.y];
    		this.neuralShader.setLight(light);


    		for (let [id, tile] of this.tiles)
    			tile.neuralUpdated = false;
    		return false;
    	}
    }

    /**
     * Register this layer type with the Layer factory
     * @type {Function}
     * @private
     */
    Layer.prototype.types['neural'] = (options) => { return new LayerNeuralRTI(options); };

    exports.BoundingBox = BoundingBox;
    exports.Camera = Camera;
    exports.Canvas = Canvas;
    exports.Color = Color;
    exports.Colormap = Colormap;
    exports.ColormapLegend = ColormapLegend;
    exports.Controller = Controller;
    exports.Controller2D = Controller2D;
    exports.ControllerPanZoom = ControllerPanZoom;
    exports.CoordinateSystem = CoordinateSystem;
    exports.Draggable = Draggable;
    exports.HSH = HSH;
    exports.Layer = Layer;
    exports.LayerAnnotationImage = LayerAnnotationImage;
    exports.LayerCombiner = LayerCombiner;
    exports.LayerImage = LayerImage;
    exports.LayerMaskedImage = LayerMaskedImage;
    exports.LayerNeuralRTI = LayerNeuralRTI;
    exports.LayerRTI = LayerRTI;
    exports.Layout = Layout;
    exports.LayoutTileImages = LayoutTileImages;
    exports.LayoutTiles = LayoutTiles;
    exports.LightSphereController = LightSphereController;
    exports.PointerManager = PointerManager;
    exports.Raster = Raster;
    exports.Ruler = Ruler;
    exports.ScaleBar = ScaleBar;
    exports.Shader = Shader;
    exports.ShaderCombiner = ShaderCombiner;
    exports.ShaderFilter = ShaderFilter;
    exports.ShaderFilterColormap = ShaderFilterColormap;
    exports.ShaderFilterOpacity = ShaderFilterOpacity;
    exports.ShaderFilterTest = ShaderFilterTest;
    exports.ShaderFilterVector = ShaderFilterVector;
    exports.ShaderFilterVectorGlyph = ShaderFilterVectorGlyph;
    exports.ShaderGammaFilter = ShaderGammaFilter;
    exports.ShaderNeural = ShaderNeural;
    exports.ShaderRTI = ShaderRTI;
    exports.Skin = Skin;
    exports.Tile = Tile;
    exports.Transform = Transform;
    exports.UIBasic = UIBasic;
    exports.UIDialog = UIDialog;
    exports.Units = Units;
    exports.Util = Util;
    exports.Viewer = Viewer;

    Object.defineProperty(exports, '__esModule', { value: true });

}));
