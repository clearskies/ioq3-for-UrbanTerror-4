#include "client.h"

#define DISCORD_UPDATE_MSEC 20000
#define APPLICATION_ID "378976623116025856"
#define NUM_GAME_TYPES 12
#define NUM_OFFICIAL_MAPS 34

static const char *GAME_TYPES[NUM_GAME_TYPES] = {
    "Free for All",
    "Last Man Standing",
    "Unknown",
    "Team Deathmatch",
    "Team Survivor",
    "Follow the Leader",
    "Capture and Hold",
    "Capture the Flag",
    "Bomb",
    "Jump",
    "Freeze Tag",
    "Gun Game"
};

static const char *OFFICIAL_MAPS[NUM_OFFICIAL_MAPS] = {
    "ut4_abbey",
    "ut4_algiers",
    "ut4_austria",
    "ut4_bohemia",
    "ut4_casa",
    "ut4_cascade",
    "ut4_docks",
    "ut4_dressingroom",
    "ut4_eagle",
    "ut4_elgin",
    "ut4_firingrange",
    "ut4_ghosttown",
    "ut4_herring",
    "ut4_killroom",
    "ut4_kingdom",
    "ut4_kingpin",
    "ut4_mandolin",
    "ut4_mykonos_a17",
    "ut4_oildepot",
    "ut4_paris",
    "ut4_prague",
    "ut4_prominence",
    "ut4_raiders",
    "ut4_ramelle",
    "ut4_ricochet",
    "ut4_riyadh",
    "ut4_sanc",
    "ut4_suburbs",
    "ut4_subway",
    "ut4_swim",
    "ut4_thingley",
    "ut4_tombs",
    "ut4_turnpike",
    "ut4_uptown"
};

static qboolean is_official_map(char *s) {
    int i;
    for (i = 0; i < NUM_OFFICIAL_MAPS; i++) {
        if (!Q_stricmp(OFFICIAL_MAPS[i], s)) {
            return qtrue;
        }
    }

    return qfalse;
}

static void handleDiscordReady() {
    Com_Printf("Discord: ready\n");
}

static void handleDiscordDisconnected(int errcode, const char* message) {
    Com_Printf("Discord: disconnected (%d: %s)\n", errcode, message);
}

static void handleDiscordError(int errcode, const char* message) {
    Com_Printf("Discord: error (%d: %s)\n", errcode, message);
}

static void handleDiscordJoin(const char* secret) {
    Com_Printf("Discord: join (%s)\n", secret);
}

static void handleDiscordSpectate(const char* secret) {
    Com_Printf("Discord: spectate (%s)\n", secret);
}

static void handleDiscordJoinRequest(const DiscordJoinRequest* request) {
    Com_Printf("Discord: join request from %s (%s)\n", request->username, request->userId);
}

void CL_InitDiscord(void) {
    DiscordEventHandlers handlers;
    memset(&handlers, 0, sizeof(handlers));

    handlers.ready = handleDiscordReady;
    handlers.disconnected = handleDiscordDisconnected;
    handlers.errored = handleDiscordError;
    handlers.joinGame = handleDiscordJoin;
    handlers.spectateGame = handleDiscordSpectate;
    handlers.joinRequest = handleDiscordJoinRequest;

    Discord_Initialize(APPLICATION_ID, &handlers, 1, NULL);

    Com_Printf("DISCORD: PRESENCE INIIALIZED\n");
}

void CL_RunDiscord(void) {
    static int accumulated_time = 0;

    #ifdef DISCORD_DISABLE_IO_THREAD
        Discord_UpdateConnection();
    #endif
    Discord_RunCallbacks();

    accumulated_time += cls.frametime;
    if (accumulated_time >= DISCORD_UPDATE_MSEC) {
        CL_UpdateDiscordPresence();
        accumulated_time = 0;
    }
}

void CL_UpdateDiscordPresence(void) {
    char details_buffer[256];
    char partyid_buffer[256];
    char gametype_buffer[256];
    char gametype_image[256];

    DiscordRichPresence discordPresence;
    memset(&discordPresence, 0, sizeof(discordPresence));

    if (cls.state == CA_ACTIVE) {
        char *s;
        int i, players, maxplayers, gtype;
        int time_current, time_start, time_end, now;

        char *serverInfo = cl.gameState.stringData + cl.gameState.stringOffsets[CS_SERVERINFO];

        // ---------------------------------------------------------------------
        s = Info_ValueForKey(serverInfo, "sv_hostname");
        Com_sprintf(details_buffer, sizeof(details_buffer), "On %s", s);
        Q_CleanStr(details_buffer);

        discordPresence.details = details_buffer;
        // ---------------------------------------------------------------------


        // ---------------------------------------------------------------------
        s = Info_ValueForKey(serverInfo, "sv_maxclients");
        maxplayers = atoi(s);
        players = 0;
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (cl.gameState.stringOffsets[CS_PLAYERS + i]) {
                players++;
            }
        }

        discordPresence.partySize = players;
        discordPresence.partyMax = maxplayers;
        // ---------------------------------------------------------------------


        // ---------------------------------------------------------------------
        s = Info_ValueForKey(serverInfo, "g_gametype");
        gtype = atoi(s);
        if (gtype >= 0 && gtype < NUM_GAME_TYPES && gtype != 2) {
            Com_sprintf(gametype_buffer, sizeof(gametype_buffer), "%s", GAME_TYPES[gtype]);
            Com_sprintf(gametype_image, sizeof(gametype_image), "gametype-%d", gtype);
        } else {
            Com_sprintf(gametype_buffer, sizeof(gametype_buffer), "Unknown Gametype (%d)", gtype);
            Com_sprintf(gametype_image, sizeof(gametype_image), "gametype-default");
        }

        // use the small image for the gametype
        discordPresence.smallImageKey = gametype_image;
        discordPresence.smallImageText = gametype_buffer;
        // ---------------------------------------------------------------------


        // ---------------------------------------------------------------------
        s = cl.gameState.stringData + cl.gameState.stringOffsets[CS_LEVEL_START_TIME];
        time_start = atoi(s) / 1000;

        time_current = cl.serverTime / 1000;

        s = Info_ValueForKey(serverInfo, "timelimit");
        time_end = atoi(s) * 60;

        now = (unsigned)time(NULL);
        discordPresence.startTimestamp = now - time_current;

        if (time_start) {
            discordPresence.startTimestamp += time_start;

            if (time_end) {
                discordPresence.endTimestamp = discordPresence.startTimestamp + time_end;
            }
        }
        // ---------------------------------------------------------------------

        discordPresence.state = "In Game";

        // if we use the servername (actually the address) for this party/join
        // stuff, we can easily let other people connect to the same server
        Com_sprintf(partyid_buffer, sizeof(partyid_buffer), "party-%s", cls.servername);
        discordPresence.partyId = partyid_buffer;

        discordPresence.largeImageKey = clc.mapname;
        discordPresence.largeImageText = clc.mapname;
    } else if (clc.demoplaying) {
        discordPresence.state = "Watching Demo";
    } else {
        discordPresence.state = "In Menus";
    }

    // use the large image for the map
    if (strlen(clc.mapname) == 0 || !is_official_map(clc.mapname)) {
        discordPresence.largeImageKey = "map-default";

        if (strlen(clc.mapname) > 0) {
            discordPresence.largeImageText = clc.mapname;
        }
    }

    Discord_UpdatePresence(&discordPresence);

    Com_Printf("\n\n----------------------------\n");
    Com_Printf("DISCORD: presence updated\n");
    Com_Printf("DISCORD: state - %s\n", discordPresence.state);
    Com_Printf("DISCORD: details - %s\n", discordPresence.details);
    Com_Printf("DISCORD: startTimestamp - %ld\n", discordPresence.startTimestamp);
    Com_Printf("DISCORD: endTimestamp - %ld\n", discordPresence.endTimestamp);
    Com_Printf("DISCORD: largeImageKey - %s\n", discordPresence.largeImageKey);
    Com_Printf("DISCORD: largeImageText - %s\n", discordPresence.largeImageText);
    Com_Printf("DISCORD: smallImageKey - %s\n", discordPresence.smallImageKey);
    Com_Printf("DISCORD: smallImageText - %s\n", discordPresence.smallImageText);
    Com_Printf("DISCORD: partyId - %s\n", discordPresence.partyId);
    Com_Printf("DISCORD: party - (%d / %d)\n", discordPresence.partySize, discordPresence.partyMax);
    Com_Printf("DISCORD: matchSecret - %s\n", discordPresence.matchSecret);
    Com_Printf("DISCORD: joinSecret - %s\n", discordPresence.joinSecret);
    Com_Printf("DISCORD: spectateSecret - %s\n", discordPresence.spectateSecret);
    Com_Printf("DISCORD: instance - %d\n", discordPresence.instance);
    Com_Printf("----------------------------\n\n");
}
