#include "history.h"

void History::push(const Action &action) {
	current_action++;
	resize(current_action);
	push_back(action);
}

Action History::undo() {
	if(current_action == -1)
		return Action();
	return (*this)[current_action--];
}

Action History::redo() {
    if(current_action >= (int)size())
		return Action();
	return (*this)[current_action++];
}
