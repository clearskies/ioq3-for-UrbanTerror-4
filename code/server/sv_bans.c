
#include "server.h"
#include "../sqlite3/sqlite3.h"

const char *schema = "CREATE TABLE `bans` ("
					 "`id` INTEGER NULL DEFAULT NULL,"
					 "`ip` MEDIUMTEXT(24) UNIQUE NOT NULL DEFAULT '255.255.255.255',"
					 "`expire` INTEGER(32) NOT NULL DEFAULT -1,"
					 "PRIMARY KEY (`id`)"
					 ");";

sqlite3 *database;
cvar_t *sv_bandb;

qboolean active = qfalse;

static int Bans_GenericCallback(void *x, int numColumns, char **columnText, char **columnResults) {
	int i;

	for (i = 0; i < numColumns; i++)
		Com_Printf("%s = %s\n", columnResults[i], columnText[i] ? columnText[i] : "NULL");
	Com_Printf("\n");

	return 0;
}

void SV_BansInit(void) {
	/* ============================================================
	 This function initializes the database and ensures that there
	 is a `bans` tables with the correct schema
	=============================================================*/

	int returnCode;
	char *errorMessage;

	sv_bandb = Cvar_Get("sv_bandb", "bans.sqlite", CVAR_ARCHIVE | CVAR_LATCH);
	char *databaseFile = sv_bandb->string;

	Com_Printf("\n");

	returnCode = sqlite3_open(databaseFile, &database);
	if (returnCode) {
		Com_Printf("[ERROR] Could not open database file: %s\n", databaseFile);
		Com_Printf("[ERROR] %s\n", sqlite3_errmsg(database));

		sqlite3_close(database);

		return;
	}

	active = qtrue;
	Com_Printf("[SUCCESS] Database file loaded.\n");

	returnCode = sqlite3_exec(database, schema, Bans_GenericCallback, NULL, &errorMessage);
	if (returnCode != SQLITE_OK) {
		Com_Printf("[ERROR] Database: %s\n\n", errorMessage);
		sqlite3_free(errorMessage);
		return;
	}

	Com_Printf("[SUCCESS] Database: correct table schema.\n");
	Com_Printf("[SUCCESS] Database ban system started.\n\n");
}

void SV_BansShutdown(void) {
	sqlite3_close(database);
	Com_Printf("\n\nDatabase ban system stopped.\n\n");
}

qboolean Bans_IsValidAddress(char *ipString) {
	int r, n = 0;
	int octets[4];

	r = sscanf(ipString, "%i.%i.%i.%i", octets, octets + 1, octets + 2, octets + 3);
	if (r != 4)
		return qfalse;

	for (n = 0; n < 4; n++) {
		// Not exactly correct, but whatever
		if (octets[n] < 0 || octets[n] > 255) 
			return qfalse;
	}

	return qtrue;
}

void Bans_AddIP(void) {
	if (Cmd_Argc() < 2) {
		Com_Printf("Usage: addip <ip> [<minutes>]\n");
	}

	if (!active) {
		Com_Printf("The database ban system isn't active "
			"(because the database file could not be opened)\n");
		return;
	}

	if (!Bans_IsValidAddress(Cmd_Argv(1))) {
		Com_Printf("That's not a valid IP address.\n");
		return;
	}

	int expireTime = -1;
	int returnCode;
	char *errorMessage;
	char *query;

	if (Cmd_Argc() == 3) {
		expireTime = atoi(Cmd_Argv(2));

		expireTime *= 60;
		expireTime += (int)time(NULL);
	}

	query = va("INSERT INTO `bans` (`ip`, `expire`) VALUES (\"%s\", %i);",
		Cmd_Argv(1), expireTime);

	returnCode = sqlite3_exec(database, query, Bans_GenericCallback, NULL, &errorMessage);
	if (returnCode != SQLITE_OK) {
		Com_Printf("[ERROR] Database: %s\n", errorMessage);
		sqlite3_free(errorMessage);
		return;
	}
	
	Com_Printf("'%s' successfully added to the ban database.\n", Cmd_Argv(1));
}