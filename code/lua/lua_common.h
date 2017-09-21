#ifndef _LUA_COMMON_H_
#define _LUA_COMMON_H_

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"

#include <lua5.2/lua.h>
#include <lua5.2/lauxlib.h>
#include <lua5.2/lualib.h>

lua_State *L;

qboolean lua_initialized;

void Lua_Init(void);
void Lua_Shutdown(void);
void Lua_Exec(char *str);

void Cmd_Lua_Exec_f(void);
void Cmd_Lua_Run_f(void);

#endif
