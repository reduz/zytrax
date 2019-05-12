//
// C++ Implementation: undo_redo
//
// Description:
//
//
// Author: Juan Linietsky <reduzio@gmail.com>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "undo_redo.h"

void UndoRedo::_delete_group(Group *p_group, bool p_do, bool p_undo) {

	for (List<CommandBase *>::Element *E = p_group->undo_method_list.front(); E; E = E->next()) {
		delete E->get();
	}
	for (List<CommandBase *>::Element *E = p_group->do_method_list.front(); E; E = E->next()) {
		delete E->get();
	}

	if (p_undo) {
		for (List<Data *>::Element *E = p_group->undo_data.front(); E; E = E->next()) {
			E->get()->free();
		}
	}

	if (p_do) {
		for (List<Data *>::Element *E = p_group->do_data.front(); E; E = E->next()) {
			E->get()->free();
		}
	}

	delete p_group;
}

void UndoRedo::begin_action(String p_name, bool p_mergeable) {

	if (group_rc > 0) {
		group_rc++;
		return;
	}

	if (group_list.size() > current_group) {
		//delete redo history
		for (int i = current_group; i < group_list.size(); i++) {

			_delete_group(group_list[i], true, false);
		}
		group_list.resize(current_group);
	}

	Group *g = new Group;
	g->name = p_name;
	group_list.push_back(g);
	g->merge_backward = false;
	g->merge_forward = false;

	if (p_mergeable && current_group > 0 && group_list[current_group - 1]->name == p_name) {
		//mergeable command, use previous group, append to it.
		group_list[current_group - 1]->merge_forward = true;
		g->merge_backward = true;
	} else {
	}

	group_rc++;

	if (action_callback) {
		action_callback(group_list[current_group]->name, action_callback_userdata);
	}
}

void UndoRedo::commit_action() {

	ERR_FAIL_COND(group_rc <= 0);

	group_rc--;

	if (group_rc > 0) {
		return;
	}

	redo();
}

void UndoRedo::undo() {

	if (current_group > group_list.size() || current_group == 0)
		return;

	do {
		current_group--;

		for (List<CommandBase *>::Element *E = group_list[current_group]->undo_method_list.front(); E; E = E->next()) {
			E->get()->call();
		}
		if (action_callback) {
			action_callback(group_list[current_group]->name, action_callback_userdata);
		}

	} while (current_group > 0 && group_list[current_group]->merge_backward);
}

void UndoRedo::redo() {

	if (current_group >= group_list.size())
		return;

	do {

		for (List<CommandBase *>::Element *E = group_list[current_group]->do_method_list.front(); E; E = E->next()) {
			E->get()->call();
		}

		current_group++;
		if (action_callback) {
			action_callback(group_list[current_group - 1]->name, action_callback_userdata);
		}
	} while (group_list[current_group - 1]->merge_forward);
}

int UndoRedo::get_current_version() {
	return current_group;
}
void UndoRedo::clean() {

	for (int i = 0; i < group_list.size(); i++) {
		_delete_group(group_list[i], false, true);
	}
	current_group = 0;
}

void UndoRedo::set_action_callback(ActionCallback p_action_callback, void *p_action_callback_userdata) {
	action_callback = p_action_callback;
	action_callback_userdata = p_action_callback_userdata;
}
UndoRedo::UndoRedo() {
	current_group = 0;
	group_rc = 0;
	action_callback = NULL;
	action_callback_userdata = NULL;
}

UndoRedo::~UndoRedo() {
	clean();
}
