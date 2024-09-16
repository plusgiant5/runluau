# The Modular Luau Runtime

## How to Build
1. Clone [Luau](https://github.com/luau-lang/luau) using Visual Studio.
2. Set the Windows environment variable `%LUAUSRC%` to the path to the cloned Luau repo.
3. Clone runluau (this repo) using Visual Studio.
4. Clone [runluau-plugins](https://github.com/plusgiant5/runluau-plugins) with Visual Studio, and make sure it's in the same folder that runluau was cloned into.
5. No x86 support by default. Pick x64 Release or x64 Debug on Luau, runluau, and runluau-plugins inside Visual Studio.
6. For your first build, build Luau first, then runluau, then runluau-plugins. After, you can do any in any order.
7. Use in the `out` folder.
