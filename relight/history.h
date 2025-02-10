#ifndef HISTORY_H
#define HISTORY_H

#include <QPoint>
#include <vector>

#include "../src/sphere.h"


class Event {
public:
	enum Type { NO_ACTION, ADD_SPHERE, REMOVE_SPHERE, ADD_BORDER, REMOVE_BORDER, MOVE_BORDER, ENABLE_IMAGE, DISABLE_IMAGE};
	Type type = NO_ACTION;
	int sphere_id = -1; //which sphere was removed
	Sphere sphere; //when a sphere is modified or removed.

	Event() {}
	Event(Type t, int id, Sphere &sp): type(t), sphere_id(id) {
		copySphere(sp);
	}
	Event(const Event &e) {
		type = e.type;
		sphere_id = e.sphere_id;
		copySphere(e.sphere);
	}
	void copySphere(const Sphere &sp) {
		sphere.border = sp.border;
		sphere.center = sp.center;
		sphere.directions = sp.directions;
		sphere.eAngle = sp.eAngle;
		sphere.eFocal = sp.eFocal;
		sphere.eHeight = sp.eHeight;
		sphere.eWidth = sp.eWidth;
		sphere.ellipse = sp.ellipse;
		sphere.fitted = sp.fitted;
		sphere.image_size = sp.image_size;
		sphere.inner = sp.inner;
		sphere.lights = sp.lights;
		sphere.radius = sp.radius;
		sphere.smallradius = sp.smallradius;
		sphere.sphereImg = sp.sphereImg;
		sphere.thumbs = sp.thumbs;
	}
};


class History: std::vector<Event> {
public:
	int current_action = -1;
	void push(const Event &event);
	Event undo();
	Event redo();
};

#endif // HISTORY_H
