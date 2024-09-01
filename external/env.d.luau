-- filesystem
declare fs: {
	readfile: (path: string) -> string;
	writefile: (path: string, content: string) -> ();
	exists: (path: string) -> boolean;
	isfile: (path: string) -> boolean;
	isfolder: (path: string) -> boolean;
	listfiles: (path: string) -> {string};
	makefolder: (path: string) -> ();
	delfile: (path: string) -> ();
	delfolder: (path: string) -> ();
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