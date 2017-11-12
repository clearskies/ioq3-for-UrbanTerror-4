#include "client.h"

static const char* APPLICATION_ID = "378976623116025856";

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
    char buffer[256];

    DiscordRichPresence discordPresence;
    memset(&discordPresence, 0, sizeof(discordPresence));

    discordPresence.state = "Playing";
    discordPresence.details = "In Menus";
    discordPresence.startTimestamp = time(0);
    discordPresence.endTimestamp = time(0) + 5 * 60;

    if (strlen(clc.mapname)) {
        Com_sprintf(buffer, sizeof(buffer), "Playing on %s", clc.mapname);
        discordPresence.details = buffer;

        // use the large image for the map
        discordPresence.largeImageKey = clc.mapname;
        discordPresence.largeImageText = clc.mapname;
    } else {
        discordPresence.details = "In Menus";
    }

    // use the small image for the gametype
    discordPresence.smallImageKey = "logo_small";
    discordPresence.smallImageText = "Capture the Flag";

    discordPresence.partyId = "party1234";
    discordPresence.partySize = 1;
    discordPresence.partyMax = 24;
    discordPresence.matchSecret = "match_secret";
    discordPresence.joinSecret = "join_secret";
    discordPresence.spectateSecret = "spectate_secret";
    discordPresence.instance = 0;
    Discord_UpdatePresence(&discordPresence);

    Com_Printf("\n\n\n\nupdated discord presence\n\n\n\n");
}
