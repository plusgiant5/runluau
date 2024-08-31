declare getscriptbytecode: ((script: BaseScript) -> string)
declare getscripthash: ((script: BaseScript) -> string)
declare getgenv: (() -> {[any]: any})
declare rconsoleprint: ((...any) -> ())
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
type compiler_settings = {
	optimizationLevel: number?;
	debugLevel: number?;
}
declare luau: {
	compile: (source: string, settings: compiler_settings) -> string;
}