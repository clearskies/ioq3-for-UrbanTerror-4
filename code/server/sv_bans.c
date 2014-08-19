
#include "server.h"
#include "../sqlite3/sqlite3.h"

const char *schema = "CREATE TABLE IF NOT EXISTS `bans`"
					 "(`id` INTEGER NULL DEFAULT NULL, "
					 "`ip` MEDIUMTEXT(16) NULL DEFAULT NULL, "
					 "`expire` INTEGER(32) NULL DEFAULT -1, "
					 "PRIMARY KEY (`id`));";

sqlite3 *database;
cvar_t *sv_bandb;

qboolean active = qfalse;

static int Bans_SchemeCallback(void *x, int numColumns, char **columnText, char **columnResults) {
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
		Com_Printf("[ERROR] Database: Could not open file: %s\n", databaseFile);
		Com_Printf("[ERROR] %s\n", sqlite3_errmsg(database));

		sqlite3_close(database);

		return;
	}

	active = qtrue;
	Com_Printf("[SUCCESS] Database: file loaded.\n");

	returnCode = sqlite3_exec(database, schema, Bans_SchemeCallback, NULL, &errorMessage);
	if (returnCode != SQLITE_OK) {
		Com_Printf("[ERROR] Database: %s\n\n", errorMessage);
		sqlite3_free(errorMessage);
	}

	Com_Printf("[SUCCESS] Database: correct table schema.\n\n");
}