#pragma once
#include "imageprovider.h"
#include "project.h"

// Thin adapter so widgets that take an ImageProvider* can work with
// the active Project without requiring Project to inherit ImageProvider.
struct ProjectProvider : ImageProvider {
	Project &proj;
	explicit ProjectProvider(Project &p) : proj(p) {}
	int imageCount() const override { return (int)proj.images.size(); }
	Image &imageAt(int i) override { return proj.images[i]; }
	const Image &imageAt(int i) const override { return proj.images[i]; }
	QImage readImage(int i) override { return proj.readImage(i); }
};
