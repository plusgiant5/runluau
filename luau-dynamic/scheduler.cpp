#include "pch.h"

#include "scheduler.h"

#include "luau.h"

typedef struct {
	lua_State* thread;
	lua_State* from;
	int args;
	std::function<void()> setup_func;
} resume_request_t;
std::list<resume_request_t> threads_to_resume;

static std::recursive_mutex list_mutex;
void scheduler::cycle() {
	std::lock_guard<std::recursive_mutex> lock(list_mutex);
	//printf("Items: %lld\n", threads_to_resume.size());
	std::list<resume_request_t>::iterator iterator = threads_to_resume.begin();
	while (iterator != threads_to_resume.end()) {
		resume_request_t request = *iterator;
		//printf("With status %d\n", lua_status(request.thread));
		if (luau::resume_and_handle_status(request.thread, request.from, request.args, request.setup_func)) {
			threads_to_resume.erase(iterator++);
		} else {
			iterator++;
		}
	}
}
void scheduler::add_thread_to_resume_queue(lua_State* thread, lua_State* from, int args, std::function<void()> setup_func) {
	std::lock_guard<std::recursive_mutex> lock(list_mutex);
	threads_to_resume.push_back({thread, from, args, setup_func});
}