#include "pch.h"

#include "scheduler.h"

#include "luau.h"

typedef struct {
	lua_State* thread;
	lua_State* from;
	int args;
} resume_request_t;
std::list<resume_request_t> threads_to_resume;

void scheduler::cycle() {
	std::list<resume_request_t>::iterator iterator = threads_to_resume.begin();
	while (iterator != threads_to_resume.end()) {
		resume_request_t request = *iterator;
		//printf("With status %d\n", lua_status(request.thread));
		luau::resume_and_handle_status(request.thread, request.from, request.args);
		threads_to_resume.erase(iterator++);
	}
}
void scheduler::add_thread_to_resume_queue(lua_State* thread, lua_State* from, int args) {
	threads_to_resume.push_back({thread, from, args});
}