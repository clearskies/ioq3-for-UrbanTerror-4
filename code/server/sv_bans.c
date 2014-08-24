
#include "server.h"
#include "../sqlite3/sqlite3.h"

#define BAN_TABLE_NAME "bans"
#define RANGE_BAN_TABLE_NAME "rangebans"

const char *schema = "CREATE TABLE IF NOT EXISTS `" BAN_TABLE_NAME "` ("
					 "`id` INTEGER NULL DEFAULT NULL,"
					 "`ip` MEDIUMTEXT(24) UNIQUE NOT NULL DEFAULT '255.255.255.255',"
					 "`expire` INTEGER(32) NOT NULL DEFAULT -1,"
					 "PRIMARY KEY (`id`)"
					 ");"
					 "CREATE TABLE IF NOT EXISTS `" RANGE_BAN_TABLE_NAME "` ("
					 "`id` INTEGER NULL DEFAULT NULL,"
					 "`ip` MEDIUMTEXT(24) UNIQUE NOT NULL DEFAULT '255.255.255.255',"
					 "`expire` INTEGER(32) NOT NULL DEFAULT -1,"
					 "PRIMARY KEY (`id`)"
					 ");";

sqlite3 *database;
cvar_t *sv_bandb;
qboolean active = qfalse;

int matchRows = 0;

qboolean Bans_IsValidAddress(char *ipString);
qboolean Bans_IsRangeAddress(char *ipString);
qboolean Bans_AddressMatches(char *ip1, char *ip2);

static int Bans_GenericCallback(void *x, int numColumns, char **columnValue, char **columnName);
static int Bans_IPExistsCallback(void *x, int numColumns, char **columnValue, char **columnName);
static int Bans_RangeMatchesCallback(char *ip, int numColumns, char **columnValue, char **columnName);



void SV_BansInit(void) {
	/* ============================================================
	 This function initializes the database and ensures that there
	 is a `bans` tables with the correct schema
	=============================================================*/

	int returnCode;
	char *errorMessage;

	sv_bandb = Cvar_Get("sv_bandb", "bans.sqlite", CVAR_ARCHIVE | CVAR_LATCH);

	char *databaseFile = va("%s/%s/%s", Cvar_VariableString("fs_homepath"), Cvar_VariableString("fs_game"), sv_bandb->string);

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
	char ip[24], query[MAX_STRING_CHARS];
	char *errorMessage;
	int returnCode;
	int ipMatches = 0;
	int currentTime = (int)time(NULL);

	sprintf(ip, "%i.%i.%i.%i", addr.ip[0], addr.ip[1], addr.ip[2], addr.ip[3]);

	matchRows = 0;
	Com_sprintf(query, MAX_STRING_CHARS, "SELECT * FROM `bans` WHERE `ip` = '%s' AND (`expire` < 0 OR `expire` > %i);", ip, currentTime);
	returnCode = sqlite3_exec(database, query, Bans_IPExistsCallback, NULL, &errorMessage);
	if (returnCode != SQLITE_OK) {
		Com_Printf("[ERROR] Database: %s\n", errorMessage);
		sqlite3_free(errorMessage);
		return qfalse;
	}

	if (matchRows)
		return qtrue;

	matchRows = 0;
	Com_sprintf(query, MAX_STRING_CHARS, "SELECT * FROM `rangebans` WHERE `expire` < 0 OR `expire` > %i;", currentTime);
	returnCode = sqlite3_exec(database, query, Bans_RangeMatchesCallback, ip, &errorMessage);
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
	char *table = BAN_TABLE_NAME;

	if (!Bans_IsValidAddress(ip)) {
		Com_Printf("Invalid IP address: %s\n", ip);
		return;
	}

	if (Bans_IsRangeAddress(ip))
		table = RANGE_BAN_TABLE_NAME;
	

	int expireTime = -1;
	int returnCode;
	char *errorMessage;
	char query[MAX_STRING_CHARS];

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
	Com_sprintf(query, MAX_STRING_CHARS, "SELECT * FROM `%s` WHERE `ip` = '%s';", table, ip);
	returnCode = sqlite3_exec(database, query, Bans_IPExistsCallback, NULL, &errorMessage);
	if (returnCode != SQLITE_OK) {
		Com_Printf("[ERROR] Database: %s\n", errorMessage);
		sqlite3_free(errorMessage);
		return;
	}

	if (matchRows)
		Com_sprintf(query, MAX_STRING_CHARS, "UPDATE `%s` SET `expire` =  %i WHERE `ip` = '%s';", \
			table, expireTime, ip);
	else
		Com_sprintf(query, MAX_STRING_CHARS, "INSERT INTO `%s` (`ip`, `expire`) VALUES ('%s', %i);",
			table, ip, expireTime);
	
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

		if (Bans_AddressMatches(ip, temp))
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
	char *table = BAN_TABLE_NAME;

	if (!Bans_IsValidAddress(ip)) {
		Com_Printf("Invalid IP address: %s\n", ip);
		return;
	}

	if (Bans_IsRangeAddress(ip))
		table = RANGE_BAN_TABLE_NAME;

	int returnCode;
	char *errorMessage;
	char query[MAX_STRING_CHARS];

	matchRows = 0;
	Com_sprintf(query, MAX_STRING_CHARS, "SELECT * FROM `%s` WHERE `ip` = '%s';", table, ip);
	returnCode = sqlite3_exec(database, query, Bans_IPExistsCallback, NULL, &errorMessage);
	if (returnCode != SQLITE_OK) {
		Com_Printf("[ERROR] Database: %s\n", errorMessage);
		sqlite3_free(errorMessage);
		return;
	}

	if (!matchRows) {
		Com_Printf("'%s' was not found in the ban database.\n", ip);
		return;
	}

	Com_sprintf(query, MAX_STRING_CHARS, "DELETE FROM `%s` WHERE `ip` = '%s';", table, ip);
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

qboolean Bans_IsRangeAddress(char *ipString) {
	int n = 0;
	int octets[4];

	sscanf(ipString, "%i.%i.%i.%i", octets, octets + 1, octets + 2, octets + 3);

	for (n = 0; n < 4; n++) {
		if (octets[n] == 0) 
			return qtrue;
	}

	return qfalse;
}

qboolean Bans_AddressMatches(char *ip1, char *ip2) {
	// ip1 is assumed to be either a range or a normal address
	// ip2 is assumed to be a normal address

	int n;
	int octets[2][4];

	sscanf(ip1, "%i.%i.%i.%i", octets[0], octets[0] + 1, octets[0] + 2, octets[0] + 3);
	sscanf(ip2, "%i.%i.%i.%i", octets[1], octets[1] + 1, octets[1] + 2, octets[1] + 3);

	for (n = 0; n < 4; n++) {
		if (!octets[0][n] || octets[0][n] == octets[1][n])
			continue;
		else
			return qfalse;
	}

	return qtrue;
}

/* ==================
  Callbacks
================== */

static int Bans_GenericCallback(void *x, int numColumns, char **columnValue, char **columnName) {
	return 0;
}

static int Bans_IPExistsCallback(void *x, int numColumns, char **columnValue, char **columnName) {
	matchRows = 1;
	return 0;
}

static int Bans_RangeMatchesCallback(char *ip, int numColumns, char **columnValue, char **columnName) {
	int i;
	for (i = 0; i < numColumns; i++) {
		if (!Q_stricmp(columnName[i], "ip") && Bans_AddressMatches(columnValue[i], ip)) {
			matchRows = 1;
			return 0;
		}
	}
	return 0;
}