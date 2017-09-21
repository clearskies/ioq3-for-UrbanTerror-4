#include "lua_common.h"

/* -----------------------------------------------------------------------------
            Lua Functions
----------------------------------------------------------------------------- */

/* ================
Executes a string as a command
================ */
int lua_q_command(lua_State *ls) {
	if (lua_gettop(ls) != 1) {
		return luaL_error(L, "q_command() expects one argument");
	}

	const char *cmd = lua_tostring(ls, 1);
	Cbuf_ExecuteText(EXEC_NOW, cmd);

	return 0;
}

/* ================
Prints a string followed by a newline in the console
================ */
int lua_q_print(lua_State *ls) {
	const char *msg;

	if (lua_gettop(ls) != 1) {
		return luaL_error(L, "q_print() expects one argument");
	}

	msg = lua_tostring(ls, 1);
	Com_Printf("%s\n", msg);

	return 0;
}

/* ================
Prints a string without a newline in the console
================ */
int lua_q_printr(lua_State *ls) {
	const char *msg;

	if (lua_gettop(ls) != 1) {
		return luaL_error(L, "q_printr() expects one argument");
	}

	msg = lua_tostring(ls, 1);
	Com_Printf("%s", msg);

	return 0;
}

/* ================
Sets the value of a cvar
================ */
int lua_q_cvar_set(lua_State *ls) {
	const char *name, *value;

	if (lua_gettop(ls) != 2) {
		return luaL_error(L, "q_cvar_set() expects one argument");
	}

	name = lua_tostring(ls, 1);
	value = lua_tostring(ls, 2);
	Cvar_SetLatched(name, value);

	return 0;
}


/* ================
Gets the string value of a cvar
================ */
int lua_q_cvar_gets(lua_State *ls) {
	const char *var;

	if (lua_gettop(ls) != 1) {
		return luaL_error(L, "q_cvar_gets() expects one argument");
	}

	var = lua_tostring(ls, 1);
	lua_pushstring(ls, Cvar_VariableString(var));

	return 1;
}

/* ================
Gets the numeric value of a cvar
================ */
int lua_q_cvar_getn(lua_State *ls) {
	const char *var;

	if (lua_gettop(ls) != 1) {
		return luaL_error(L, "q_cvar_gets() expects one argument");
	}

	var = lua_tostring(ls, 1);
	lua_pushnumber(ls, Cvar_VariableValue(var));

	return 1;
}

/* -----------------------------------------------------------------------------
            Utility Functions
----------------------------------------------------------------------------- */

void Lua_Init(void) {
	L = luaL_newstate();
	luaL_openlibs(L);

	lua_register(L, "q_command", lua_q_command);
	lua_register(L, "q_print", lua_q_print);
	lua_register(L, "q_printr", lua_q_printr);

	lua_register(L, "q_cvar_set", lua_q_cvar_set);
	lua_register(L, "q_cvar_gets", lua_q_cvar_gets);
	lua_register(L, "q_cvar_getn", lua_q_cvar_getn);

	lua_initialized = qtrue;
}

void Lua_Shutdown(void) {
	lua_close(L);
}

/* -----------------------------------------------------------------------------
            Commands
----------------------------------------------------------------------------- */

void Cmd_Lua_Exec_f(void) {
	char *contents;
	char filename[MAX_QPATH];
	int err;

	if (Cmd_Argc() != 2) {
		Com_Printf("lua_exec <filename>: execute a lua file\n");
		return;
	}

	if (!lua_initialized) {
		Com_Error(ERR_FATAL, "Lua call made before initialization\n");
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

	err = luaL_loadstring(L, contents) || lua_pcall(L, 0, LUA_MULTRET, 0);
	if (err) {
		Com_Printf("Lua error: %s\n", lua_tostring(L, -1));
		lua_pop(L, 1);
	}

	FS_FreeFile(contents);
}

void Cmd_Lua_Run_f(void) {
	int err, i;

	if (Cmd_Argc() < 2) {
		Com_Printf("lua_run <function> [arguments]: run a lua function\n");
		return;
	}

	if (!lua_initialized) {
		Com_Error(ERR_FATAL, "Lua call made before initialization\n");
		return;
	}

	lua_getglobal(L, Cmd_Argv(1));
	for (i = 2; i < Cmd_Argc(); i++) {
		lua_pushstring(L, Cmd_Argv(i));
	}

	// Set nresults to 0 because we don't actually want to do anything with the
	// return values. If this changes, we can make it LUA_MULTRET
	err = lua_pcall(L, Cmd_Argc() - 2, 0, 0);
	if (err) {
		Com_Printf("Lua error: %s\n", lua_tostring(L, -1));
		lua_pop(L, 1);
	}
}
