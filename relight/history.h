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
	Event(Type t, int id, Sphere &sp): type(t), sphere_id(id), sphere(sp) {}
};


class History: std::vector<Event> {
public:
	int current_action = -1;
	void push(const Event &event);
	Event undo();
	Event redo();
};

#endif // HISTORY_H
