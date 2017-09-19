#include "lua_common.h"

void lua_exec(char *code) {
	lua_State *L = luaL_newstate();
	luaL_openlibs(L);
	luaL_dostring(L, code);
	lua_close(L);
}

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
	lua_exec(contents);

	FS_FreeFile(contents);
}
