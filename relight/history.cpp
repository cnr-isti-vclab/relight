#include "history.h"

void History::push(const Event &action) {
	current_action++;
	resize(current_action);
	push_back(action);
}

Event History::undo() {
	if(current_action == -1)
		return Event();
	return (*this)[current_action--];
}

Event History::redo() {
    if(current_action >= (int)size())
		return Event();
	return (*this)[current_action++];
}
