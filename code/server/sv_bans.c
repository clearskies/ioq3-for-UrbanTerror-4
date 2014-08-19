
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

qboolean Bans_IsValidAddress(char *ipString);
static int Bans_GenericCallback(void *x, int numColumns, char **columnText, char **columnResults);

int matchRows = 0;
static int Bans_IPExists(void *x, int numColumns, char **columnText, char **columnResults);



void SV_BansInit(void) {
	/* ============================================================
	 This function initializes the database and ensures that there
	 is a `bans` tables with the correct schema
	=============================================================*/

	int returnCode;
	char *errorMessage;

	sv_bandb = Cvar_Get("sv_bandb", "bans.sqlite", CVAR_ARCHIVE | CVAR_LATCH);

	char *databaseFile = va("%s%s/%s", Cvar_VariableString("fs_homepath"), Cvar_VariableString("fs_game"), sv_bandb->string);

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

qboolean Bans_CheckIP(netadr_t addr) {
	char ip[24];
	char *query, *errorMessage;
	int returnCode;

	sprintf(ip, "%i.%i.%i.%i", addr.ip[0], addr.ip[1], addr.ip[2], addr.ip[3]);

	matchRows = 0;
	query = va("SELECT * FROM `bans` WHERE `ip` = '%s';", ip);
	returnCode = sqlite3_exec(database, query, Bans_IPExists, NULL, &errorMessage);
	if (returnCode != SQLITE_OK) {
		Com_Printf("[ERROR] Database: %s\n", errorMessage);
		sqlite3_free(errorMessage);
		return qfalse;
	}

	if (matchRows)
		return qtrue;
	
	return qfalse;
}

/* ==================
  Commands
================== */

void Bans_AddIP(void) {
	if (Cmd_Argc() < 2) {
		Com_Printf("Usage: addip <ip> [<minutes>]\n");
	}

	if (!active) {
		Com_Printf("The database ban system isn't active "
			"(because the database file could not be opened)\n");
		return;
	}

	char *ip = Cmd_Argv(1);

	if (!Bans_IsValidAddress(ip)) {
		Com_Printf("Invalid IP address: %s\n", ip);
		return;
	}

	int expireTime = -1;
	int returnCode;
	char *errorMessage;
	char *query;

	int i;
	client_t *cl;
	char temp[24];

	if (Cmd_Argc() == 3) {
		expireTime = atoi(Cmd_Argv(2));

		expireTime *= 60;
		expireTime += (int)time(NULL);
	}

	// If the IP already exists in the database, update the expiry timestamp
	matchRows = 0;
	query = va("SELECT * FROM `bans` WHERE `ip` = '%s';", ip);
	returnCode = sqlite3_exec(database, query, Bans_IPExists, NULL, &errorMessage);
	if (returnCode != SQLITE_OK) {
		Com_Printf("[ERROR] Database: %s\n", errorMessage);
		sqlite3_free(errorMessage);
		return;
	}

	if (matchRows)
		query = va("UPDATE `bans` SET `expire` =  %i WHERE `ip` = '%s';", \
			expireTime, ip);
	else
		query = va("INSERT INTO `bans` (`ip`, `expire`) VALUES ('%s', %i);",
			ip, expireTime);
	
	returnCode = sqlite3_exec(database, query, Bans_GenericCallback, NULL, &errorMessage);
	if (returnCode != SQLITE_OK) {
		Com_Printf("[ERROR] Database: %s\n", errorMessage);
		sqlite3_free(errorMessage);
		return;
	}
	
	Com_Printf("'%s' successfully added to the ban database. Expires: %s", ip,
		expireTime == -1 ? "never\n" : ctime((time_t *)&expireTime));

	for (i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++) {
		if (!cl->state)
			continue;
		
		sprintf(temp, "%i.%i.%i.%i", cl->netchan.remoteAddress.ip[0],
			cl->netchan.remoteAddress.ip[1], 
			cl->netchan.remoteAddress.ip[2], 
			cl->netchan.remoteAddress.ip[3]);

		if (!Q_stricmp(ip, temp))
			SV_DropClient(cl, "You have been banned.");
	}
}

void Bans_RemoveIP(void) {
	if (Cmd_Argc() < 2) {
		Com_Printf("Usage: removeip <ip>\n");
	}

	if (!active) {
		Com_Printf("The database ban system isn't active "
			"(because the database file could not be opened)\n");
		return;
	}

	char *ip = Cmd_Argv(1);

	if (!Bans_IsValidAddress(ip)) {
		Com_Printf("Invalid IP address: %s\n", ip);
		return;
	}

	int returnCode;
	char *errorMessage;
	char *query;

	matchRows = 0;
	query = va("SELECT * FROM `bans` WHERE `ip` = '%s';", ip);
	returnCode = sqlite3_exec(database, query, Bans_IPExists, NULL, &errorMessage);
	if (returnCode != SQLITE_OK) {
		Com_Printf("[ERROR] Database: %s\n", errorMessage);
		sqlite3_free(errorMessage);
		return;
	}

	if (!matchRows) {
		Com_Printf("'%s' was not found in the ban database.\n", ip);
		return;
	}

	query = va("DELETE FROM `bans` WHERE `ip` = '%s';", ip);
	returnCode = sqlite3_exec(database, query, Bans_GenericCallback, NULL, &errorMessage);
	if (returnCode != SQLITE_OK) {
		Com_Printf("[ERROR] Database: %s\n", errorMessage);
		sqlite3_free(errorMessage);
		return;
	}
	
	Com_Printf("'%s' successfully removed from the ban database.\n", ip);
}


/* ==================
  Utility Functions
================== */


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

/* ==================
  Callbacks
================== */

static int Bans_GenericCallback(void *x, int numColumns, char **columnText, char **columnResults) {
	int i;

	for (i = 0; i < numColumns; i++)
		Com_Printf("%s = %s\n", columnResults[i], columnText[i] ? columnText[i] : "NULL");
	Com_Printf("\n");

	return 0;
}

static int Bans_IPExists(void *x, int numColumns, char **columnText, char **columnResults) {
	matchRows = 1;
	return 0;
}