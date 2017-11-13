#include "client.h"

#define APPLICATION_ID "378976623116025856"
#define NUM_GAME_TYPES 12
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

    Com_Printf("\n\n\n\nDISCORD INITIALIZED\n\n\n\n");
}

void CL_RunDiscord(void) {
    #ifdef DISCORD_DISABLE_IO_THREAD
        Discord_UpdateConnection();
    #endif
    Discord_RunCallbacks();
}

void CL_UpdateDiscordPresence(void) {
    char details_buffer[256];
    char partyid_buffer[256];
    char gametype_buffer[256];
    char gametype_image[256];
    char servername_buffer[256];

    DiscordRichPresence discordPresence;
    memset(&discordPresence, 0, sizeof(discordPresence));

    if (cls.state == CA_ACTIVE) {
        char *s;
        int i, players, maxplayers, gtype;

        char *serverInfo = cl.gameState.stringData + cl.gameState.stringOffsets[CS_SERVERINFO];

        // ------------------------------
        s = Info_ValueForKey(serverInfo, "sv_hostname");
        Q_strncpyz(servername_buffer, s, sizeof(servername_buffer));
        Q_CleanStr(servername_buffer);
        // ------------------------------

        // ------------------------------
        s = Info_ValueForKey(serverInfo, "sv_maxclients");
        maxplayers = atoi(s);
        players = 0;
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (cl.gameState.stringOffsets[544 + i]) {
                players++;
            }
        }
        // ------------------------------

        // ------------------------------
        s = Info_ValueForKey(serverInfo, "g_gametype");
        gtype = atoi(s);
        if (gtype >= 0 && gtype < NUM_GAME_TYPES && gtype != 2) {
            Com_sprintf(gametype_buffer, sizeof(gametype_buffer), "%s", GAME_TYPES[gtype]);
            Com_sprintf(gametype_image, sizeof(gametype_image), "gametype-%d", gtype);
        } else {
            Com_sprintf(gametype_buffer, sizeof(gametype_buffer), "Unknown Gametype (%d)", gtype);
            Com_sprintf(gametype_image, sizeof(gametype_image), "gametype-default");
        }
        // ------------------------------

        Com_sprintf(details_buffer, sizeof(details_buffer), "On %s", servername_buffer);

        discordPresence.state = "Playing";
        discordPresence.details = details_buffer;

        // if we use the servername (actually the address) for this party/join
        // stuff, we can easily let other people connect to the same server
        Com_sprintf(partyid_buffer, sizeof(partyid_buffer), "party-%s", cls.servername);
        discordPresence.partyId = partyid_buffer;
        discordPresence.joinSecret = cls.servername;

        discordPresence.partySize = players;
        discordPresence.partyMax = maxplayers;

        // use the small image for the gametype
        discordPresence.smallImageKey = gametype_image;
        discordPresence.smallImageText = gametype_buffer;

        Com_Printf("smallimage %s %s\n", gametype_image, gametype_buffer);
    } else if (clc.demoplaying) {
        discordPresence.state = "Watching Demo";
    } else {
        discordPresence.state = "In Menus";
    }

    if ( cls.state == CA_ACTIVE || clc.demoplaying ) {
        Com_Printf("Mapname; %s\n", clc.mapname);

        // use the large image for the map
        discordPresence.largeImageKey = clc.mapname;
        discordPresence.largeImageText = clc.mapname;
    }

    Discord_UpdatePresence(&discordPresence);

    Com_Printf("Discord: updated presence\n");
}
