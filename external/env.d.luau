-- filesystem
declare fs: {
	read_file: (path: string) -> string;
	write_file: (path: string, content: string) -> ();
	exists: (path: string) -> boolean;
	is_file: (path: string) -> boolean;
	is_folder: (path: string) -> boolean;
	list: (path: string) -> {string};
	new_folder: (path: string) -> ();
	delete_file: (path: string) -> ();
	delete_folder: (path: string) -> ();
}



-- luau
type compiler_settings = {
	optimizationLevel: number?;
	debugLevel: number?;
}
declare luau: {
	compile: (source: string, settings: compiler_settings) -> string;
}



-- task
declare task: {
	wait: (duration: number?) -> number;
	spawn: (func: (...any) -> (...any), ...any) -> thread;
	defer: (func: (...any) -> (...any), ...any) -> thread;
	delay: (duration: number?, func: (...any) -> (...any), ...any) -> thread;
	cancel: (thread: thread) -> ();
}



-- graphics
type event = {
	type: "test";
}
type event_type = typeof(({} :: event).type)
declare gfx: {
	create_window: (width: number, height: number, title: string?) -> buffer;
	window_exists: (fb: buffer) -> boolean;
	render: (fb: buffer) -> ();
	get_window_events: (fb: buffer) -> {event};
	set_window_size: (fb: buffer, width: number, height: number) -> ();
}