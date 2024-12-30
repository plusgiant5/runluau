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
export type luau_compiler_settings = {
	optimizationLevel: number?;
	debugLevel: number?;
}
declare luau: {
	compile: (source: string, settings: luau_compiler_settings) -> string;
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
export type gfx_input_type =
				| "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9"
				| "A"| "B"| "C"| "D"| "E"| "F"| "G"| "H"| "I"| "J"| "K"| "L"| "M"| "N"| "O"| "P"| "Q"| "R"| "S"| "T"| "U"| "V"| "W"| "X"| "Y"| "Z"
				| "Space" | "Tab"
				| "F1" | "F2" | "F3" | "F4" | "F5" | "F6" | "F7" | "F8" | "F9" | "F10" | "F11" | "F12"
				| "NumPad0" | "NumPad1" | "NumPad2" | "NumPad3" | "NumPad4" | "NumPad5" | "NumPad6" | "NumPad7" | "NumPad8" | "NumPad9"
				| "NumPadMultiply" | "NumPadAdd" | "NumPadSeparator" | "NumPadSubtract" | "NumPadDecimal" | "NumPadDivide"
				| "Shift" | "LeftShift" | "RightShift"
				| "Control" | "LeftControl" | "RightControl"
				| "Alt" | "LeftAlt" | "RightAlt"
				| "LeftArrow" | "UpArrow" | "RightArrow" | "DownArrow" | "Home" | "End" | "PageUp" | "PageDown"
				| "Insert" | "Delete" | "Escape"
				| "CapsLock" | "NumLock" | "ScrollLock"
				| "LeftWindows" | "RightWindows" | "Applications"
				| "VolumeMute" | "VolumeDown" | "VolumeUp" | "MediaNextTrack" | "MediaPrevTrack" | "MediaStop" | "MediaPlayPause"
				| "Backspace" | "Enter" | "Clear" | "Select"
				| "Execute" | "Print" | "Help" | "Sleep" | "PrintScreen" | "Pause"
				| "Semicolon" | "Plus" | "Minus" | "Comma" | "Period" | "Slash" | "Tilde" | "OpenBracket" | "CloseBracket" | "Backslash" | "Quote"
				| "LeftMouseButton" | "RightMouseButton" | "MiddleMouseButton" | "XButton1" | "XButton2"
export type gfx_event = {
	event_type: "input";
	down: boolean;
	input: gfx_input_type;
} | {
	event_type: "scroll";
	delta: number;
}
export type gfx_event_type = typeof(({} :: gfx_event).event_type)
declare gfx: {
	create_window: (width: number, height: number, title: string?, pixel_size: number?) -> buffer;
	window_exists: (fb: buffer) -> boolean;
	get_window_events: (fb: buffer) -> {gfx_event};
	set_window_size: (fb: buffer, width: number, height: number) -> ();
	get_cursor_position: (fb: buffer) -> (number, number);
}



-- osunsafe
declare os: {
	execute: (command: string) -> ();
	capture: (command: string) -> string;
	getenv: (variable: string) -> string?;
	setenv: (variable: string, value: string) -> ();
}