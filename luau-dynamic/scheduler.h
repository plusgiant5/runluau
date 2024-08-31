#pragma once

namespace scheduler {
	void cycle();
	void add_thread_to_resume_queue(lua_State* thread, lua_State* from, int args);
}