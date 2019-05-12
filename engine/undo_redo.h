//
// C++ Interface: undo_redo
//
// Description:
//
//
// Author: Juan Linietsky <reduzio@gmail.com>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef UNDO_REDO_H
#define UNDO_REDO_H

#include "list.h"
#include "rstring.h"
#include "vector.h"
//#include "simple_type.h"
/**
	@author Juan Linietsky <reduzio@gmail.com>
*/
class UndoRedo {
public:
	typedef void (*ActionCallback)(const String &, void *);

protected:
	struct CommandDataBase {

		virtual ~CommandDataBase() {}
	};

	template <class T>
	struct CommandData : public CommandDataBase {

		T *data;

		~CommandData() { delete data; }
		CommandData(T *p_data) { data = p_data; }
	};

	struct CommandBase {

		List<CommandDataBase *> command_data;

	public:
		template <class T>
		CommandBase *with_data(T *p_data) {

			command_data.push_back(new CommandData<T>(p_data));
			return this;
		}

		virtual void call() = 0;
		virtual ~CommandBase() {}
	};

	template <class T>
	struct Command0 : public CommandBase {

		typedef void (T::*Method)();
		T *instance;
		Method method;
		virtual void call() {
			(instance->*method)();
		}
		Command0(T *p_instance, Method p_method) {
			instance = p_instance;
			method = p_method;
		}
	};

	template <class T, class P1>
	struct Command1 : public CommandBase {

		typedef void (T::*Method)(P1);
		T *instance;
		Method method;
		P1 p1;
		virtual void call() {
			(instance->*method)(p1);
		}
		Command1(T *p_instance, Method p_method, P1 p_p1) {
			instance = p_instance;
			method = p_method;
			p1 = p_p1;
		}
	};

	/**/

	template <class T, class P1, class P2>
	struct Command2 : public CommandBase {

		typedef void (T::*Method)(P1, P2);
		T *instance;
		Method method;
		P1 p1;
		P2 p2;
		virtual void call() {
			(instance->*method)(p1, p2);
		}
		Command2(T *p_instance, Method p_method, P1 p_p1, P2 p_p2) {
			instance = p_instance;
			method = p_method;
			p1 = p_p1;
			p2 = p_p2;
		}
	};

	/**/

	template <class T, class P1, class P2, class P3>
	struct Command3 : public CommandBase {

		typedef void (T::*Method)(P1, P2, P3);
		T *instance;
		Method method;
		P1 p1;
		P2 p2;
		P3 p3;
		virtual void call() {
			(instance->*method)(p1, p2, p3);
		}
		Command3(T *p_instance, Method p_method, P1 p_p1, P2 p_p2, P3 p_p3) {
			instance = p_instance;
			method = p_method;
			p1 = p_p1;
			p2 = p_p2;
			p3 = p_p3;
		}
	};

	/**/

	template <class T, class P1, class P2, class P3, class P4>
	struct Command4 : public CommandBase {

		typedef void (T::*Method)(P1, P2, P3, P4);
		T *instance;
		Method method;
		P1 p1;
		P2 p2;
		P3 p3;
		P4 p4;
		virtual void call() {
			(instance->*method)(p1, p2, p3, p4);
		}
		Command4(T *p_instance, Method p_method, P1 p_p1, P2 p_p2, P3 p_p3, P4 p_p4) {
			instance = p_instance;
			method = p_method;
			p1 = p_p1;
			p2 = p_p2;
			p3 = p_p3;
			p4 = p_p4;
		}
	};

	/**/

	template <class T, class P1, class P2, class P3, class P4, class P5>
	struct Command5 : public CommandBase {

		typedef void (T::*Method)(P1, P2, P3, P4, P5);
		T *instance;
		Method method;
		P1 p1;
		P2 p2;
		P3 p3;
		P4 p4;
		P5 p5;
		virtual void call() {
			(instance->*method)(p1, p2, p3, p4, p5);
		}
		Command5(T *p_instance, Method p_method, P1 p_p1, P2 p_p2, P3 p_p3, P4 p_p4, P5 p_p5) {
			instance = p_instance;
			method = p_method;
			p1 = p_p1;
			p2 = p_p2;
			p3 = p_p3;
			p4 = p_p4;
			p5 = p_p5;
		}
	};

	/* methods */

	template <class T, class M, class P1>
	Command1<T, P1> *command(T *p_instance, M p_method, P1 p1) {

		return new Command1<T, P1>(p_instance, p_method, p1);
	}

	template <class T, class M, class P1, class P2>
	Command2<T, P1, P2> *command(T *p_instance, M p_method, P1 p1, P2 p2) {

		return new Command2<T, P1, P2>(p_instance, p_method, p1, p2);
	}

	template <class T, class M, class P1, class P2, class P3>
	Command3<T, P1, P2, P3> *command(T *p_instance, M p_method, P1 p1, P2 p2, P3 p3) {

		return new Command3<T, P1, P2, P3>(p_instance, p_method, p1, p2, p3);
	}

	template <class T, class M, class P1, class P2, class P3, class P4>
	Command4<T, P1, P2, P3, P4> *command(T *p_instance, M p_method, P1 p1, P2 p2, P3 p3, P4 p4) {

		return new Command4<T, P1, P2, P3, P4>(p_instance, p_method, p1, p2, p3, p4);
	}

	template <class T, class M, class P1, class P2, class P3, class P4, class P5>
	Command5<T, P1, P2, P3, P4, P5> *command(T *p_instance, M p_method, P1 p1, P2 p2, P3 p3, P4 p4, P5 p5) {

		return new Command5<T, P1, P2, P3, P4, P5>(p_instance, p_method, p1, p2, p3, p4, p5);
	}

	/*****/

	class Data {
	public:
		virtual void free() = 0;
		virtual ~Data() {}
	};

	template <class T>
	class DataPtr : public Data {

		T *ptr;

	public:
		virtual void free() { delete ptr; }
		DataPtr(T *p_ptr) { ptr = p_ptr; }
		~DataPtr() {}
	};

private:
	struct Group {

		List<CommandBase *> do_method_list;
		List<CommandBase *> undo_method_list;
		List<Data *> do_data;
		List<Data *> undo_data;
		String name;
		bool merge_forward;
		bool merge_backward;
	};

	void _delete_group(Group *p_group, bool p_do, bool p_undo);
	Vector<Group *> group_list;
	int current_group;
	int group_rc;

	ActionCallback action_callback;
	void *action_callback_userdata;

public:
	void begin_action(String p_name, bool p_mergeable = false);

	template <class T, class M>
	void do_method(T *p_instance, M p_method) {

		group_list[current_group]->do_method_list.push_back(new Command0<T>(p_instance, p_method));
	}

	template <class T, class M, class P1>
	void do_method(T *p_instance, M p_method, P1 p1) {

		group_list[current_group]->do_method_list.push_back(new Command1<T, P1>(p_instance, p_method, p1));
	}

	template <class T, class M, class P1, class P2>
	void do_method(T *p_instance, M p_method, P1 p1, P2 p2) {

		group_list[current_group]->do_method_list.push_back(new Command2<T, P1, P2>(p_instance, p_method, p1, p2));
	}

	template <class T, class M, class P1, class P2, class P3>
	void do_method(T *p_instance, M p_method, P1 p1, P2 p2, P3 p3) {

		group_list[current_group]->do_method_list.push_back(new Command3<T, P1, P2, P3>(p_instance, p_method, p1, p2, p3));
	}

	template <class T, class M, class P1, class P2, class P3, class P4>
	void do_method(T *p_instance, M p_method, P1 p1, P2 p2, P3 p3, P4 p4) {

		group_list[current_group]->do_method_list.push_back(new Command4<T, P1, P2, P3, P4>(p_instance, p_method, p1, p2, p3, p4));
	}

	template <class T, class M>
	void undo_method(T *p_instance, M p_method) {

		group_list[current_group]->undo_method_list.push_back(new Command0<T>(p_instance, p_method));
	}

	template <class T, class M, class P1>
	void undo_method(T *p_instance, M p_method, P1 p1) {

		group_list[current_group]->undo_method_list.push_back(new Command1<T, P1>(p_instance, p_method, p1));
	}

	template <class T, class M, class P1, class P2>
	void undo_method(T *p_instance, M p_method, P1 p1, P2 p2) {

		group_list[current_group]->undo_method_list.push_back(new Command2<T, P1, P2>(p_instance, p_method, p1, p2));
	}

	template <class T, class M, class P1, class P2, class P3>
	void undo_method(T *p_instance, M p_method, P1 p1, P2 p2, P3 p3) {

		group_list[current_group]->undo_method_list.push_back(new Command3<T, P1, P2, P3>(p_instance, p_method, p1, p2, p3));
	}

	template <class T, class M, class P1, class P2, class P3, class P4>
	void undo_method(T *p_instance, M p_method, P1 p1, P2 p2, P3 p3, P4 p4) {

		group_list[current_group]->undo_method_list.push_back(new Command4<T, P1, P2, P3, P4>(p_instance, p_method, p1, p2, p3, p4));
	}

	template <class T>
	void do_data(T *p_data) {

		group_list[current_group]->do_data.push_back(new DataPtr<T>(p_data));
	}

	template <class T>
	void undo_data(T *p_data) {

		group_list[current_group]->undo_data.push_back(new DataPtr<T>(p_data));
	}

	void commit_action();

	void undo();
	void redo();
	void clean();

	int get_current_version();

	void set_action_callback(ActionCallback p_action_callback, void *p_action_callback_userdata);

	UndoRedo();
	~UndoRedo();
};

#endif
