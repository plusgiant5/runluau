# The Modular Luau Runtime
Fast, small, simple, intended to be modified.

## Purpose
Runs [Luau](https://github.com/luau-lang/luau) code on your computer through the command line.
Has a plugin system to give it more capabilities than Luau would normally have. (filesystem, shell commands, graphics, or anything else you want if you make your own plugin)
Has a `build` command which turns a Luau script into a standalone executable file with embedded plugins.
You can pass arguments into Luau scripts with `--args` and access them from the script by using `...` in the global scope.

## How To Build
1. Clone [Luau](https://github.com/luau-lang/luau) using Visual Studio.
2. Set the Windows environment variable `%LUAUSRC%` to the path to the cloned Luau repo.
3. Clone runluau (this repo) using Visual Studio.
4. Clone [runluau-plugins](https://github.com/plusgiant5/runluau-plugins) with Visual Studio, and make sure it's in the same folder that runluau was cloned into.
5. No x86 support by default. Pick x64 Release or x64 Debug configurations for Luau, runluau, and runluau-plugins inside Visual Studio.
6. For your first build, build Luau first, then runluau, then runluau-plugins. After, you can do any in any order.
7. Use in the `out` folder.

## Linux/Mac Support?
Would be a very difficult feat, as the plugins system is based off loading DLLs, and the build system generates Windows executables.

## Command Line Arguments
You can read the arguments passed into a Luau script simply by using `...` in the global scope. It will be a tuple of strings.

## Debugging in Visual Studio
The debugger settings are usually set to whatever random thing I was testing at the time. Change the command, command arguments, and working directory in the runluau project configuration -> Debugging to debug for yourself.
