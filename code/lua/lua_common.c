#include "lua_common.h"

/* -----------------------------------------------------------------------------
            Utility Functions
----------------------------------------------------------------------------- */

void Lua_Init(void) {
	L = luaL_newstate();
	luaL_openlibs(L);

	lua_initialized = qtrue;
}

void Lua_Shutdown(void) {
	lua_close(L);
}

void Lua_Exec(char *str) {
	int err;

	if (!lua_initialized) {
		Com_Error(ERR_FATAL, "Lua call made before initialization\n");
		return;
	}

	err = luaL_loadstring(L, str) || lua_pcall(L, 0, LUA_MULTRET, 0);

	if (err) {
		Com_Printf("Lua error: %s\n", lua_tostring(L, -1));
		lua_pop(L, 1);
	}
}

/* -----------------------------------------------------------------------------
            Commands
----------------------------------------------------------------------------- */

void Cmd_Lua_Exec_f(void) {
	char *contents;
	char filename[MAX_QPATH];

	if (Cmd_Argc() != 2) {
		Com_Printf("lua_exec <filename>: execute a lua file\n");
		return;
	}

	Q_strncpyz(filename, Cmd_Argv(1), sizeof(filename));
	COM_DefaultExtension(filename, sizeof(filename), ".lua");
	FS_ReadFile(filename, (void **)&contents);

	if (!contents) {
		Com_Printf ("Couldn't read %s\n", Cmd_Argv(1));
		return;
	}

	Com_Printf("Executing %s\n", filename);
	Lua_Exec(contents);

	FS_FreeFile(contents);
}
