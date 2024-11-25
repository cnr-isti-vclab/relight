/*
 * Copyright (C) 2009 Josh A. Beam
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __RASTERIZER_H__
#define __RASTERIZER_H__

//#include "mesh/mesh.h"
#include <vector>

class Edge {
public:
    float h1, h2;
    int X1, Y1, X2, Y2;

    Edge(float h1, int x1, int y1, float h2, int x2, int y2);
};

class Span {
public:
    float h1, h2;
    int X1, X2;

    Span(float h1, int x1, float h2, int x2);
};

class Rasterizer {
public:

	int width;
	int height;
	std::vector<int> ids;
	Rasterizer(int w, int h) {
		width = w;
		height = h;
		ids.resize(w*h, -1);
	}


public:
	void SetPixel(unsigned int x, unsigned int y, float h, int id);
	void SetPixel(int x, int y, float h, int id);
	void SetPixel(float x, float y, float h, int id);

	void DrawTriangle(float h1, float x1, float y1, float h2, float x2, float y2, float h3, float x3, float y3, int id);

	void DrawLine(float h1, float x1, float y1, float h2, float x2, float y2, int id);
	void DrawSpan(const Span &span, int y, int id);
	void DrawSpansBetweenEdges(const Edge &e1, const Edge &e2, int id);

};

#endif /* __RASTERIZER_H__ */
