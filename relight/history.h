#ifndef HISTORY_H
#define HISTORY_H

#include <QPoint>
#include <vector>

#include "sphere.h"

class Action {
public:
	enum Type { NO_ACTION, ADD_SPHERE, REMOVE_SPHERE, ADD_BORDER, REMOVE_BORDER, MOVE_BORDER, ENABLE_IMAGE, DISABLE_IMAGE};
	Type type = NO_ACTION;
	Sphere sphere; //when a sphere is modified or removed.
	int sphere_id = -1; //which sphere was removed

	Action() {}
	Action(Type t, int id, Sphere &sp): type(t), sphere_id(id), sphere(sp) {}
};

class History: std::vector<Action> {
public:
	int current_action = -1;
	void push(const Action &action);
	Action undo();
	Action redo();
};

#endif // HISTORY_H
