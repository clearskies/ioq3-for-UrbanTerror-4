/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// console.c

#include "client.h"
#include "killLog.h"
#include "hitLog.h"


int g_console_field_width = 78;

#define	NUM_CON_TIMES 4
#define	CON_TEXTSIZE	32768

typedef struct {
	qboolean	initialized;

	short	text[CON_TEXTSIZE];
	int		current;		// line where next message will be printed
	int		x;				// offset in current line for next print
	int		display;		// bottom of console displays this line

	int 	linewidth;		// characters across screen
	int		totallines;		// total lines in console scrollback

	float	xadjust;		// for wide aspect screens
	float   yadjust;

	float	displayFrac;	// aproaches finalFrac at scr_conspeed
	float	finalFrac;		// 0.0 to 1.0 lines of console to display

	int		vislines;		// in scanlines

	int		times[NUM_CON_TIMES];	// cls.realtime time the line was generated
								// for transparent notify lines
	vec4_t color;
} console_t;


#define CONSOLE_ALL 0
#define CONSOLE_GENERAL 1
#define CONSOLE_KILLS 2
#define CONSOLE_HITS 3
#define CONSOLE_CHAT 4
#define CONSOLE_DEV 5

console_t consoles[6];
int currentConsoleNum = CONSOLE_ALL;
console_t	*currentCon = &consoles[CONSOLE_ALL];
char *consoleNames[] = {
	"All",
	"General",
	"Kills",
	"Hits",
	"Chat",
	"Dev"
};
int numConsoles = 5;

int lastServerCmdNum = -1;
qboolean ignoreNewline = qtrue;

void Con_SpecificBottom(console_t *console);

cvar_t		*con_conspeed;
cvar_t		*con_notifytime;
cvar_t		*con_coloredKills;
cvar_t		*con_coloredHits;

cvar_t		*con_bgAlpha;
cvar_t		*con_bgColour;
cvar_t		*con_prompt;
cvar_t		*con_height;
cvar_t		*con_timeDisplay;
cvar_t		*con_promptColour;
cvar_t		*con_scrollLock;
cvar_t		*con_drawScrollbar;
cvar_t		*con_fadeIn;
cvar_t		*con_margin;
cvar_t		*con_showVersion;
cvar_t		*con_chatTime;
cvar_t		*con_borderRGB;


#define	DEFAULT_CONSOLE_WIDTH	78

vec4_t	console_color = {1.0, 1.0, 1.0, 1.0};

float opacityMult = 1;
float targetOpacityMult = 1;

void Con_RE_SetColor(vec4_t colour) {
	vec4_t c;
	if (colour) {
		c[0] = colour[0];
		c[1] = colour[1];
		c[2] = colour[2];
		c[3] = colour[3] * opacityMult;
		re.SetColor(c);
	} else {
		re.SetColor(NULL);
	}
}

void SCR_AdjustedFillRect(float x, float y, float width, float height, const float *color) {
	vec4_t c;
	if (color) {
		c[0] = color[0];
		c[1] = color[1];
		c[2] = color[2];
		c[3] = color[3] * opacityMult;
	} else {
		c[0] = 1;
		c[1] = 1;
		c[2] = 1;
		c[3] = opacityMult;
	}

	SCR_FillRect(x, y, width, height, c);
}

void SCR_AdjustedDrawString(int x, int y, float size, const char *string, float *setColor, qboolean forceColor) {
	vec4_t c;
	if (setColor) {
		c[0] = setColor[0];
		c[1] = setColor[1];
		c[2] = setColor[2];
		c[3] = setColor[3] * opacityMult;
	} else {
		c[0] = 1;
		c[1] = 1;
		c[2] = 1;
		c[3] = opacityMult;
	}
	SCR_DrawCondensedString(x, y, size, string, c, forceColor);
}

void SCR_AdjustedFontText(float x, float y, float scale, vec4_t color, const char *text, int style) {
	vec4_t c;
	if (color) {
		c[0] = color[0];
		c[1] = color[1];
		c[2] = color[2];
		c[3] = color[3] * opacityMult;
	} else {
		c[0] = 1;
		c[1] = 1;
		c[2] = 1;
		c[3] = opacityMult;
	}
	SCR_DrawFontText(x, y, scale, c, text, style);
}

#define BOX_MARGIN 30

int adjustedScreenWidth = SCREEN_WIDTH;
int margin = 0;
int adjustedXMargin = 0;
int adjustedYMargin = 0;


/*
================
Con_ToggleConsole_f
================
*/
void Con_ToggleConsole_f (void) {
	// closing a full screen console restarts the demo loop
	if ( cls.state == CA_DISCONNECTED && cls.keyCatchers == KEYCATCH_CONSOLE ) {
		CL_StartDemoLoop();
		return;
	}

	Field_Clear( &g_consoleField );
	g_consoleField.widthInChars = g_console_field_width;

	Con_ClearNotify ();
	cls.keyCatchers ^= KEYCATCH_CONSOLE;
}

/*
================
Con_MessageMode_f
================
*/
void Con_MessageMode_f (void) {
	chat_playerNum = -1;
	chat_team = qfalse;
	chat_console = qfalse;
	chat_rcon = qfalse;
	Field_Clear( &chatField );
	chatField.widthInChars = 30;

	cls.keyCatchers ^= KEYCATCH_MESSAGE;
}

/*
================
Con_MessageModeC_f
================
*/
void Con_MessageModeC_f (void) {
	chat_playerNum = -1;
	chat_team = qfalse;
	chat_console = qtrue;
	chat_rcon = qfalse;
	Field_Clear( &chatField );
	chatField.widthInChars = 30;

	cls.keyCatchers ^= KEYCATCH_MESSAGE;
}

/*
================
Con_MessageModeR_f
================
*/
void Con_MessageModeR_f (void) {
	chat_playerNum = -1;
	chat_team = qfalse;
	chat_console = qfalse;
	chat_rcon = qtrue;
	Field_Clear( &chatField );
	chatField.widthInChars = 30;

	cls.keyCatchers ^= KEYCATCH_MESSAGE;
}

/*
================
Con_MessageMode2_f
================
*/
void Con_MessageMode2_f (void) {
	chat_playerNum = -1;
	chat_team = qtrue;
	chat_console = qfalse;
	chat_rcon = qfalse;
	Field_Clear( &chatField );
	chatField.widthInChars = 25;
	cls.keyCatchers ^= KEYCATCH_MESSAGE;
}

/*
================
Con_MessageMode3_f
================
*/
void Con_MessageMode3_f (void) {
	chat_playerNum = VM_Call( cgvm, CG_CROSSHAIR_PLAYER );
	if ( chat_playerNum < 0 || chat_playerNum >= MAX_CLIENTS ) {
		chat_playerNum = -1;
		return;
	}
	chat_team = qfalse;
	chat_console = qfalse;
	chat_rcon = qfalse;
	Field_Clear( &chatField );
	chatField.widthInChars = 30;
	cls.keyCatchers ^= KEYCATCH_MESSAGE;
}

/*
================
Con_MessageMode4_f
================
*/
void Con_MessageMode4_f (void) {
	chat_playerNum = VM_Call( cgvm, CG_LAST_ATTACKER );
	if ( chat_playerNum < 0 || chat_playerNum >= MAX_CLIENTS ) {
		chat_playerNum = -1;
		return;
	}
	chat_team = qfalse;
	chat_console = qfalse;
	chat_rcon = qfalse;
	Field_Clear( &chatField );
	chatField.widthInChars = 30;
	cls.keyCatchers ^= KEYCATCH_MESSAGE;
}

/*
================
Con_Clear_f
================
*/
void Con_Clear_f (void) {
	int		i;

	for ( i = 0 ; i < CON_TEXTSIZE ; i++ ) {
		currentCon->text[i] = (ColorIndex(COLOR_WHITE)<<8) | ' ';
	}

	Con_Bottom();		// go to end
}


/*
================
Con_Dump_f

Save the console contents out to a file
================
*/
void Con_Dump (console_t *console) {
	int		l, x, i;
	short	*line;
	fileHandle_t	f;
	char	buffer[1024];

	f = FS_FOpenFileWrite( Cmd_Argv( 1 ) );
	if (!f)
	{
		Com_Printf ("ERROR: couldn't open %s.\n", Cmd_Argv(1));
		return;
	}

	Com_Printf ("Dumped console text to %s.\n", Cmd_Argv(1) );

	// skip empty lines
	for (l = console->current - console->totallines + 1 ; l <= console->current ; l++)
	{
		line = console->text + (l%console->totallines)*console->linewidth;
		for (x=0 ; x<console->linewidth ; x++)
			if ((line[x] & 0xff) != ' ')
				break;
		if (x != console->linewidth)
			break;
	}

	// write the remaining lines
	buffer[console->linewidth] = 0;
	for ( ; l <= console->current ; l++)
	{
		line = console->text + (l%console->totallines)*console->linewidth;
		for(i=0; i<console->linewidth; i++)
			buffer[i] = line[i] & 0xff;
		for (x=console->linewidth-1 ; x>=0 ; x--)
		{
			if (buffer[x] == ' ')
				buffer[x] = 0;
			else
				break;
		}

		#ifdef _WIN32
		strcat(buffer, "\r\n");
		#else
		strcat(buffer, "\n");
		#endif

		FS_Write(buffer, strlen(buffer), f);
	}

	FS_FCloseFile( f );
}

void Con_DumpThis_f(void) {
	if (Cmd_Argc() != 2) {
		Com_Printf ("Usage: condump <filename>\n");
		return;
	}

	Con_Dump(currentCon);
}

void Con_ChatDump_f(void) {
	if (Cmd_Argc() != 2) {
		Com_Printf ("Usage: chatdump <filename>\n");
		return;
	}

	Con_Dump(&consoles[CONSOLE_CHAT]);
}


/*
================
Con_ClearNotify
================
*/
void Con_ClearNotify( void ) {
	int		i;

	for ( i = 0 ; i < NUM_CON_TIMES ; i++ ) {
		currentCon->times[i] = 0;
	}
}



/*
================
Con_CheckResize

If the line width has changed, reformat the buffer.
================
*/
void Con_CheckResize (console_t *console)
{
	int		i, j, width, oldwidth, oldtotallines, numlines, numchars;
	short	tbuf[CON_TEXTSIZE];

	float xscale = cls.glconfig.vidWidth / 640.0;
	width = (cls.glconfig.vidWidth - adjustedXMargin * 2 - (int)(42 * xscale)) / (SMALLCHAR_WIDTH - 1);

	if (width == console->linewidth)
		return;

	if (width < 1)			// video hasn't been initialized yet
	{
		width = DEFAULT_CONSOLE_WIDTH;
		console->linewidth = width;
		console->totallines = CON_TEXTSIZE / console->linewidth;
		for(i=0; i<CON_TEXTSIZE; i++)
			console->text[i] = (ColorIndex(COLOR_WHITE)<<8) | ' ';
	}
	else
	{
		oldwidth = console->linewidth;
		console->linewidth = width;
		oldtotallines = console->totallines;
		console->totallines = CON_TEXTSIZE / console->linewidth;
		numlines = oldtotallines;

		if (console->totallines < numlines)
			numlines = console->totallines;

		numchars = oldwidth;

		if (console->linewidth < numchars)
			numchars = console->linewidth;

		Com_Memcpy (tbuf, console->text, CON_TEXTSIZE * sizeof(short));
		for(i=0; i<CON_TEXTSIZE; i++)

			console->text[i] = (ColorIndex(COLOR_WHITE)<<8) | ' ';


		for (i=0 ; i<numlines ; i++)
		{
			for (j=0 ; j<numchars ; j++)
			{
				console->text[(console->totallines - 1 - i) * console->linewidth + j] =
						tbuf[((console->current - i + oldtotallines) %
							  oldtotallines) * oldwidth + j];
			}
		}

		Con_ClearNotify ();
	}

	console->current = console->totallines - 1;
	console->display = console->current;
}

void Con_PrevTab() {
	currentConsoleNum--;
	if (currentConsoleNum < 0)
		currentConsoleNum = numConsoles - 1;
	currentCon = &consoles[currentConsoleNum];
}

void Con_NextTab() {
	currentConsoleNum++;
	if (currentConsoleNum == numConsoles)
		currentConsoleNum = CONSOLE_ALL;
	currentCon = &consoles[currentConsoleNum];
}

/*
================
Con_Init
================
*/
void Con_Init (void) {
	int		i;

	con_notifytime = Cvar_Get ("con_notifytime", "3", 0);
	con_conspeed = Cvar_Get ("scr_conspeed", "3", CVAR_ARCHIVE);

	con_coloredKills = Cvar_Get("con_coloredKills", "0", CVAR_ARCHIVE);
	con_coloredHits = Cvar_Get("con_coloredHits", "0", CVAR_ARCHIVE);

	con_bgAlpha = Cvar_Get("con_bgAlpha", "90", CVAR_ARCHIVE);
	con_bgColour = Cvar_Get("con_bgColor", "0", CVAR_ARCHIVE);
	con_prompt = Cvar_Get("con_prompt", "]", CVAR_ARCHIVE);
	con_height = Cvar_Get("con_height", "50", CVAR_ARCHIVE);
	con_timeDisplay = Cvar_Get("con_timeDisplay", "1", CVAR_ARCHIVE);
	con_promptColour = Cvar_Get("con_promptColor", "7", CVAR_ARCHIVE);
	con_scrollLock = Cvar_Get("con_scrollLock", "1", CVAR_ARCHIVE);
	con_drawScrollbar = Cvar_Get("con_drawScrollbar", "0", CVAR_ARCHIVE);
	con_fadeIn = Cvar_Get("con_fadeIn", "0", CVAR_ARCHIVE);
	con_margin = Cvar_Get("con_margin", "0", CVAR_ARCHIVE);
	con_showVersion = Cvar_Get("con_showVersion", "1", CVAR_ARCHIVE);
	con_chatTime = Cvar_Get("con_chatTime", "0", CVAR_ARCHIVE);
	con_borderRGB = Cvar_Get("con_borderRGB", "0 100 100", CVAR_ARCHIVE);

	Field_Clear( &g_consoleField );
	g_consoleField.widthInChars = g_console_field_width;
	for ( i = 0 ; i < COMMAND_HISTORY ; i++ ) {
		Field_Clear( &historyEditLines[i] );
		historyEditLines[i].widthInChars = g_console_field_width;
	}
	CL_LoadConsoleHistory( );

	Cmd_AddCommand ("toggleconsole", Con_ToggleConsole_f);
	Cmd_AddCommand ("messagemode", Con_MessageMode_f);
	Cmd_AddCommand ("messagemodec", Con_MessageModeC_f);
	Cmd_AddCommand ("messagemoder", Con_MessageModeR_f);
	Cmd_AddCommand ("messagemode2", Con_MessageMode2_f);
	Cmd_AddCommand ("messagemode3", Con_MessageMode3_f);
	Cmd_AddCommand ("messagemode4", Con_MessageMode4_f);
	Cmd_AddCommand ("clear", Con_Clear_f);
	Cmd_AddCommand ("condump", Con_DumpThis_f);
	Cmd_AddCommand ("chatdump", Con_ChatDump_f);
}


/*
===============
Con_Linefeed
===============
*/
void Con_Linefeed (console_t *console, qboolean skipnotify)
{
	int		i;

	// mark time for transparent overlay
	if (console->current >= 0)
	{
	if (skipnotify)
		  console->times[console->current % NUM_CON_TIMES] = 0;
	else
		  console->times[console->current % NUM_CON_TIMES] = cls.realtime;
	}

	console->x = 0;
	if (console->display == console->current)
		console->display++;
	console->current++;
	for(i=0; i<console->linewidth; i++)
		console->text[(console->current%console->totallines)*console->linewidth+i] = (ColorIndex(COLOR_WHITE)<<8) | ' ';
}

int nameToTeamColour(char *name) {
	int i, team = 2, myteam;
	char *cs;
	for (i = 0; i < MAX_CLIENTS; i++) {
		cs = cl.gameState.stringData + cl.gameState.stringOffsets[544 + i];
		if (!Q_stricmp(Info_ValueForKey(cs, "n"), name)) {
			team = atoi(Info_ValueForKey(cs, "t"));
			myteam = atoi(Info_ValueForKey(cl.gameState.stringData + cl.gameState.stringOffsets[544 + clc.clientNum], "t"));

			if (team == myteam)
				return skinToChatColour(team, Cvar_VariableValue("cg_skinAlly"));
			else
				return skinToChatColour(team, Cvar_VariableValue("cg_skinEnemy"));
		}
	}

	return team;
}

int damageToColour(int damage) {
	if (damage >= 50) {
		return 1;
	} else if (damage >= 25) {
		return 8;
	} else if (damage >= 17) {
		return 3;
	} else {
		return 2;
	}
}

int healthToColour(int health) {
	if (health >= 60) {
		return 2;
	} else if (health >= 35) {
		return 3;
	} else if (health >= 15) {
		return 8;
	} else {
		return 1;
	}
}

void writeTextToConsole(console_t *console, char *txt, qboolean skipnotify) {
	int		y;
	int		c, l;
	int		color;
	int prev;							// NERVE - SMF

	color = ColorIndex(COLOR_WHITE);

	while ( (c = *txt) != 0 ) {
		if ( Q_IsColorString( txt ) ) {
			color = ColorIndex( *(txt+1) );
			txt += 2;
			continue;
		}

		// count word length
		for (l=0 ; l< console->linewidth ; l++) {
			if ( txt[l] <= ' ') {
				break;
			}

		}

		// word wrap
		if (l != console->linewidth && (console->x + l >= console->linewidth) ) {
			Con_Linefeed(console, skipnotify);
		}

		txt++;

		switch (c)
		{
		case '\n':
			Con_Linefeed (console, skipnotify);
			break;
		case '\r':
			console->x = 0;
			break;
		default:	// display character and advance
			y = console->current % console->totallines;
			console->text[y*console->linewidth+console->x] = (color << 8) | c;
			console->x++;
			if (console->x >= console->linewidth) {
				Con_Linefeed(console, skipnotify);
				console->x = 0;
			}
			break;
		}
	}

	// mark time for transparent overlay
	if (console->current >= 0) {
		// NERVE - SMF
		if ( skipnotify ) {
			prev = console->current % NUM_CON_TIMES - 1;
			if ( prev < 0 )
				prev = NUM_CON_TIMES - 1;
			console->times[prev] = 0;
		}
		else
		// -NERVE - SMF
			console->times[console->current % NUM_CON_TIMES] = cls.realtime;
	}

	if (con_scrollLock && !con_scrollLock->integer) {
		Con_SpecificBottom(console);
	}
}

void CL_DevConsolePrint(char *txt) {
	int i;
	qboolean skipnotify = qfalse;
	// TTimo - prefix for text that shows up in console but not in notify
	// backported from RTCW
	if ( !Q_strncmp( txt, "[skipnotify]", 12 ) ) {
		skipnotify = qtrue;
		txt += 12;
	}

	// for some demos we don't want to ever show anything on the console
	if ( cl_noprint && cl_noprint->integer ) {
		return;
	}

	for (i = 0; i < 6; i++) {
		if (!consoles[i].initialized) {
			consoles[i].color[0] =
			consoles[i].color[1] =
			consoles[i].color[2] =
			consoles[i].color[3] = 1.0f;
			consoles[i].linewidth = -1;
			Con_CheckResize (&consoles[i]);
			consoles[i].initialized = qtrue;
		}
	}

	writeTextToConsole(&consoles[CONSOLE_DEV], txt, skipnotify);
}

/*
================
CL_ConsolePrint

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be logged to disk
If no console is visible, the text will appear at the top of the game window
================
*/
void CL_ConsolePrint( char *txt ) {
	int team;
	char player1[MAX_NAME_LENGTH + 1], player2[MAX_NAME_LENGTH + 1];
	char nplayer1[MAX_NAME_LENGTH + 5], nplayer2[MAX_NAME_LENGTH + 5];
	char newtxt[MAX_STRING_CHARS + 1];
	char damageString[12];
	int damage, damageCol;
	char **search;
	char *playerhad;
	int found = 0;
	int killLogNum = Cvar_VariableIntegerValue("cg_drawKillLog");
	int i;

	qboolean skipnotify = qfalse; // NERVE - SMF

	// For tabs
	qboolean isChat = qfalse;
	qboolean isHit = qfalse;
	qboolean isKill = qfalse;

	qboolean normalChat = qfalse;

	if (ignoreNewline && !strcmp(txt, "\n")) {
		ignoreNewline = qfalse;
		return;
	}

	ignoreNewline = qfalse;

	// TTimo - prefix for text that shows up in console but not in notify
	// backported from RTCW
	if ( !Q_strncmp( txt, "[skipnotify]", 12 ) ) {
		skipnotify = qtrue;
		txt += 12;
	}

	// for some demos we don't want to ever show anything on the console
	if ( cl_noprint && cl_noprint->integer ) {
		return;
	}

	for (i = 0; i < numConsoles; i++) {
		if (!consoles[i].initialized) {
			consoles[i].color[0] =
			consoles[i].color[1] =
			consoles[i].color[2] =
			consoles[i].color[3] = 1.0f;
			consoles[i].linewidth = -1;
			Con_CheckResize (&consoles[i]);
			consoles[i].initialized = qtrue;
		}
	}

	if (txt[0] == 17) {
		isKill = qtrue;
		txt++;
	} else if (txt[0] == 18) {
		isHit = qtrue;
		txt++;
	} else if (txt[0] == 19) {
		isChat = qtrue;
		txt++;
	}


	if (cls.state == CA_ACTIVE ) {
		char *cmd;
		cmd = clc.serverCommands[clc.lastExecutedServerCommand & (MAX_RELIABLE_COMMANDS-1)];
		if (clc.lastExecutedServerCommand != lastServerCmdNum) {
			if (isChat && (strstr(cmd, "cchat") == cmd || strstr(cmd, "tcchat") == cmd)) {
				normalChat = qtrue;
			}
		}
		lastServerCmdNum = clc.lastExecutedServerCommand;

		if (txt[strlen(txt) - 1] == '\n') {
			ignoreNewline = qtrue;
		}

		if (isChat && normalChat && con_chatTime->integer) {
			qtime_t curTime;
			Com_RealTime(&curTime);
			int hour = curTime.tm_hour;

			if (con_chatTime->integer == 2)
				hour = hourTo12(hour);

			sprintf(newtxt, "[%02i:%02i:%02i] %s", hour, curTime.tm_min, curTime.tm_sec, txt);
			txt = newtxt;
		}

		if (killLogNum == 1) {
			search = killLog1;
		} else if (killLogNum == 2) {
			search = killLog2;
		} else if (killLogNum > 2) {
			search = killLog3;
		}

		playerhad = "%s had %s health.";
		if (sscanf(txt, playerhad, player2, damageString) == 2) {
			damage = atoi(damageString);
			damageCol = healthToColour(damage);
			team = nameToTeamColour(player2);
			sprintf(nplayer2, "^%i%s^7", team, player2);
			sprintf(damageString, "^%i%i%%^7", damageCol, damage);
			sprintf(newtxt, playerhad, nplayer2, damageString);
			txt = newtxt;
		}

		if (killLogNum > 0 && killLogNum < 6) {
			int temp;
			for (i = 0; ; i++) {
				if (!search[i])
					break;

				if (sscanf(txt, search[i], player1, player2) == 2) {
					found = 1;
					if (con_coloredKills && con_coloredKills->integer) {
						temp = strlen(search[i]);
						if (!strcmp(&search[i][temp - 3], "%s.")) {
							player2[strlen(player2) - 1] = 0;
						} else if (Q_strsub(search[i], "%s's")) {
							player2[strlen(player2) - 2] = 0;
						}

						team = nameToTeamColour(player1);
						sprintf(nplayer1, "^%i%s^7", team, player1);

						team = nameToTeamColour(player2);
						sprintf(nplayer2, "^%i%s^7", team, player2);

						sprintf(newtxt, search[i], nplayer1, nplayer2);
						txt = newtxt;

					}
					break;
				}
			}

			if (!found) {
				for (i = 0; ; i++) {
					if (!killLogSingle[i]) {
						break;
					}

					if (sscanf(txt, killLogSingle[i], player1, player2) == 2) {
						if (con_coloredKills && con_coloredKills->integer) {
							team = nameToTeamColour(player1);
							sprintf(nplayer1, "^%i%s^7", team, player1);

							sprintf(newtxt, killLogSingle[i], nplayer1, player2);
							txt = newtxt;
						}
						break;
					}
				}
			}
		}

		if (Cvar_VariableIntegerValue("cg_showbullethits") == 2) {
			for (i = 0; ; i++) {
				if (!hitLog1[i])
						break;

				if (sscanf(txt, hitLog1[i], player1, player2, damageString) == 3) {
					if (con_coloredHits && con_coloredHits->integer) {
						damage = atoi(damageString);
						damageCol = damageToColour(damage);

						if (con_coloredHits->integer == 2) {
							team = nameToTeamColour(player1);
							sprintf(nplayer1, "^%i%s^7", team, player1);

							team = nameToTeamColour(player2);
							sprintf(nplayer2, "^%i%s^7", team, player2);
						} else {
							sprintf(nplayer1, "%s", player1);
							sprintf(nplayer2, "%s", player2);
						}

						sprintf(damageString, "^%i%i%%^7", damageCol, damage);
						sprintf(newtxt, hitLog1[i], nplayer1, nplayer2, damageString);
						txt = newtxt;
					}
					break;
				}
			}

			for (i = 0; ; i++) {
				if (!hitLog2[i])
						break;

				if (sscanf(txt, hitLog2[i], player1, player2, damageString) == 3) {
					if (con_coloredHits && con_coloredHits->integer) {
						damage = atoi(damageString);
						damageCol = damageToColour(damage);

						if (con_coloredHits->integer == 2) {
							if (strcmp(player1, "You")) {
								team = nameToTeamColour(player1);
								sprintf(nplayer1, "^%i%s^7", team, player1);
							} else {
								sprintf(nplayer1, "%s", player1);
							}

							team = nameToTeamColour(player2);
							sprintf(nplayer2, "^%i%s^7", team, player2);
						} else {
							sprintf(nplayer1, "%s", player1);
							sprintf(nplayer2, "%s", player2);
						}

						sprintf(damageString, "^%i%i%%^7", damageCol, damage);
						sprintf(newtxt, hitLog2[i], nplayer1, nplayer2, damageString);
						txt = newtxt;
					}
					break;
				}
			}

			for (i = 0; ; i++) {
				if (!hitLog3[i])
						break;

				if (sscanf(txt, hitLog3[i], player2, damageString) == 2) {
					if (con_coloredHits && con_coloredHits->integer) {
						damage = atoi(damageString);
						damageCol = damageToColour(damage);

						if (con_coloredHits->integer == 2) {
							team = nameToTeamColour(player2);
							sprintf(nplayer2, "^%i%s^7", team, player2);
						} else {
							sprintf(nplayer2, "%s", player2);
						}

						sprintf(damageString, "^%i%i%%^7", damageCol, damage);
						sprintf(newtxt, hitLog3[i], nplayer2, damageString);
						txt = newtxt;
					}
					break;
				}
			}
		} else if (Cvar_VariableIntegerValue("cg_showbullethits") == 3) {
			if (sscanf(txt, "%s was hit by %s for %s damage.", player1, player2, damageString) == 3) {
				if (con_coloredHits && con_coloredHits->integer) {
					damage = atoi(damageString);
					damageCol = damageToColour(damage);

					if (con_coloredHits->integer == 2) {
						team = nameToTeamColour(player1);
						sprintf(nplayer1, "^%i%s^7", team, player1);

						team = nameToTeamColour(player2);
						sprintf(nplayer2, "^%i%s^7", team, player2);
					} else {
						sprintf(nplayer1, "%s", player1);
						sprintf(nplayer2, "%s", player2);
					}

					sprintf(damageString, "^%i%i%%^7", damageCol, damage);
					sprintf(newtxt, "%s was hit by %s for %s damage.", nplayer1, nplayer2, damageString);
					txt = newtxt;
				}
			} else if (sscanf(txt, "%s hit %s for %s damage.", player1, player2, damageString) == 3) {
				if (con_coloredHits && con_coloredHits->integer) {
					damage = atoi(damageString);
					damageCol = damageToColour(damage);

					if (con_coloredHits->integer == 2) {
						if (strcmp(player1, "You")) {
							team = nameToTeamColour(player1);
							sprintf(nplayer1, "^%i%s^7", team, player1);
						} else {
							sprintf(nplayer1, "%s", player1);
						}

						team = nameToTeamColour(player2);
						sprintf(nplayer2, "^%i%s^7", team, player2);
					} else {
						sprintf(nplayer1, "%s", player1);
						sprintf(nplayer2, "%s", player2);
					}

					sprintf(damageString, "^%i%i%%^7", damageCol, damage);
					sprintf(newtxt, "%s hit %s for %s damage.", nplayer1, nplayer2, damageString);
					txt = newtxt;
				}
			} else if (sscanf(txt, "You were hit by %s for %s damage.", player2, damageString) == 2) {
				if (con_coloredHits && con_coloredHits->integer) {
					damage = atoi(damageString);
					damageCol = damageToColour(damage);

					if (con_coloredHits->integer == 2) {
						team = nameToTeamColour(player2);
						sprintf(nplayer2, "^%i%s^7", team, player2);
					} else {
						sprintf(nplayer2, "%s", player2);
					}

					sprintf(damageString, "^%i%i%%^7", damageCol, damage);
					sprintf(newtxt, "You were hit by %s for %s damage.", nplayer2, damageString);
					txt = newtxt;
				}
			}
		}
	}

	writeTextToConsole(&consoles[CONSOLE_ALL], txt, skipnotify);

	if (isKill) {
		writeTextToConsole(&consoles[CONSOLE_KILLS], txt, skipnotify);
	} else if (isHit) {
		writeTextToConsole(&consoles[CONSOLE_HITS], txt, skipnotify);
	} else if (isChat) {
		writeTextToConsole(&consoles[CONSOLE_CHAT], txt, skipnotify);
	} else {
		writeTextToConsole(&consoles[CONSOLE_GENERAL], txt, skipnotify);
	}
}


/*
==============================================================================

DRAWING

==============================================================================
*/


/*
================
Con_DrawInput

Draw the editline after a ] prompt
================
*/
void Con_DrawInput (void) {
	int		y;

	if ( cls.state != CA_DISCONNECTED && !(cls.keyCatchers & KEYCATCH_CONSOLE ) ) {
		return;
	}

	y = currentCon->vislines - ( SMALLCHAR_HEIGHT * 2 );

	Con_RE_SetColor( currentCon->color );

	if (con_promptColour->integer >= 0 && con_promptColour->integer < 10) {
		Con_RE_SetColor(g_color_table[con_promptColour->integer]);
	}

	int promptLen = strlen(con_prompt->string) + 1;
	int i;
	char *prompt;

	if (con_timeDisplay->integer > 1) {
		qtime_t curTime;
		Com_RealTime(&curTime);
		prompt = Z_Malloc(promptLen + 11);
		int hour = curTime.tm_hour;
		Com_sprintf(prompt, promptLen + 11, "[%02i:%02i:%02i] %s", hour, curTime.tm_min, curTime.tm_sec, con_prompt->string);
	} else {
		prompt = Z_Malloc(promptLen);
		Com_sprintf(prompt, promptLen, "%s", con_prompt->string);
	}

	promptLen = strlen(prompt);

	for (i = 0; i < promptLen; i++) {
		SCR_DrawSmallChar( currentCon->xadjust + (i + 1) * (SMALLCHAR_WIDTH - 1) + adjustedXMargin, y + adjustedYMargin, prompt[i]);
	}

	Con_RE_SetColor(currentCon->color);

	if (opacityMult)
	Field_Draw( &g_consoleField, currentCon->xadjust + (promptLen + 1) * (SMALLCHAR_WIDTH - 1) + adjustedXMargin, y + adjustedYMargin,
		adjustedScreenWidth - 3 * (SMALLCHAR_WIDTH - 1), qtrue);
}


/*
================
Con_DrawNotify

Draws the last few lines of output transparently over the game top
================
*/
void Con_DrawNotify (void)
{
	int		x, v;
	short	*text;
	int		i;
	int		time;
	int		skip;
	int		currentColor;

	currentColor = 7;
	Con_RE_SetColor( g_color_table[currentColor] );

	v = 0;
	for (i= currentCon->current-NUM_CON_TIMES+1 ; i<=currentCon->current ; i++)
	{
		if (i < 0)
			continue;
		time = currentCon->times[i % NUM_CON_TIMES];
		if (time == 0)
			continue;
		time = cls.realtime - time;
		if (time > con_notifytime->value*1000)
			continue;
		text = currentCon->text + (i % currentCon->totallines)*currentCon->linewidth;

		if (cl.snap.ps.pm_type != PM_INTERMISSION && (cls.keyCatchers & (KEYCATCH_UI | KEYCATCH_CGAME)) ) {
			continue;
		}

		for (x = 0 ; x < currentCon->linewidth ; x++) {
			if ( ( text[x] & 0xff ) == ' ' ) {
				continue;
			}
			if ( ( (text[x]>>8)&7 ) != currentColor ) {
				currentColor = (text[x]>>8)&7;
				Con_RE_SetColor( g_color_table[currentColor] );
			}
			SCR_DrawSmallChar( cl_conXOffset->integer + currentCon->xadjust + (x+1)*(SMALLCHAR_WIDTH - 1), v, text[x] & 0xff );
		}

		v += SMALLCHAR_HEIGHT;
	}

	Con_RE_SetColor(NULL);

	if (cls.keyCatchers & (KEYCATCH_UI | KEYCATCH_CGAME) ) {
		return;
	}

	// draw the chat line
	if ( cls.keyCatchers & KEYCATCH_MESSAGE )
	{
		if (chat_team)
		{
			SCR_DrawBigString (8, v, "say_team:", 1.0f );
			skip = 10;
		}
		else if (chat_console) {
			SCR_DrawBigString(8, v, "/", 1.0f);
			skip = 1;
		}
		else if (chat_rcon) {
			SCR_DrawBigString(8, v, "rcon:", 1.0f);
			skip = 6;
		}
		else
		{
			SCR_DrawBigString (8, v, "say:", 1.0f );
			skip = 5;
		}

		Field_BigDraw( &chatField, skip * BIGCHAR_WIDTH, v,
			adjustedScreenWidth - ( skip + 1 ) * BIGCHAR_WIDTH, qtrue );

		v += BIGCHAR_HEIGHT;
	}

}

/*
================
Con_DrawSolidConsole

Draws the console with the solid background
================
*/
void Con_DrawSolidConsole( float frac ) {
	int				i, x, y;
	int				rows;
	short			*text;
	int				row;
	int				lines;
//	qhandle_t		conShader;
	int				currentColor;
	vec4_t			lineColour, bgColour;
	vec4_t			darkTextColour;
	int r, g, b;

	lines = cls.glconfig.vidHeight * frac;
	if (lines <= 0)
		return;

	if (lines > cls.glconfig.vidHeight )
		lines = cls.glconfig.vidHeight;

	// on wide screens, we will center the text
	currentCon->xadjust = 0;
	SCR_AdjustFrom640( &currentCon->xadjust, NULL, NULL, NULL );

	if (con_bgColour->integer >= 0 && con_bgColour->integer < 10) {
		bgColour[0] = g_color_table[con_bgColour->integer][0];
		bgColour[1] = g_color_table[con_bgColour->integer][1];
		bgColour[2] = g_color_table[con_bgColour->integer][2];
	} else {
		bgColour[0] = 0.0;
		bgColour[1] = 0.0;
		bgColour[2] = 0.0;
	}
	if (con_bgAlpha->integer >= 0 && con_bgAlpha->integer <= 100) {
		bgColour[3] = con_bgAlpha->integer/100.0;
	} else {
		bgColour[3] = 0.9;
	}

	// draw the background
	y = frac * SCREEN_HEIGHT - 2;
	if ( y < 1 ) {
		y = 0;
	} else {
		SCR_AdjustedFillRect(margin, margin, adjustedScreenWidth, y, bgColour);
	}

	if (sscanf(con_borderRGB->string, "%i %i %i", &r, &g, &b) == 3 &&
		r >=0 && r <= 255 &&
		g >=0 && g <= 255 &&
		b >=0 && b <= 255) {
		lineColour[0] = r/255.0;
		lineColour[1] = g/255.0;
		lineColour[2] = b/255.0;
	} else {
		lineColour[0] = 0;
		lineColour[1] = 100.0/255.0;
		lineColour[2] = 100.0/255.0;
	}
	lineColour[3] = 1;

	darkTextColour[0] = darkTextColour[1] = darkTextColour[2] = 0.25;
	darkTextColour[3] = 1;

	int conPixHeight = 240;
	if (con_height->integer >= 0 && con_height->integer <= 100) {
		conPixHeight = con_height->integer/100.0 * SCREEN_HEIGHT;
	}

	if (margin) {
		SCR_AdjustedFillRect(margin, margin, adjustedScreenWidth, 1, lineColour);
		SCR_AdjustedFillRect(margin, margin, 1, conPixHeight - 1, lineColour);
		SCR_AdjustedFillRect(margin + adjustedScreenWidth - 1, margin, 1, conPixHeight - 1, lineColour);
	}

	int totalOffset = 0;

	int vertOffset;
	int horizOffset;
	horizOffset = margin;
	vertOffset = y + margin;
	int tabWidth;
	int tabHeight;
	float old;

	for (i = 0; i < numConsoles; i++) {
		if (i == currentConsoleNum) {
			tabWidth = SCR_FontWidth(consoleNames[i], 0.24f) + 30;
			tabHeight = 22;
			lineColour[3] = 1;
		} else {
			tabWidth = SCR_FontWidth(consoleNames[i], 0.18f) + 18;
			tabHeight = 18;
			lineColour[3] = 0.3;
		}

		// tab background
		if (i) {
			SCR_AdjustedFillRect(horizOffset + 1, vertOffset, tabWidth - 1, tabHeight, bgColour);
		} else {
			SCR_AdjustedFillRect(horizOffset, vertOffset, tabWidth, tabHeight, bgColour);
		}

		if (i != currentConsoleNum) {
			old = lineColour[3];
			lineColour[3] = 1;

			// top border
			SCR_AdjustedFillRect(horizOffset, vertOffset, tabWidth, 1, lineColour);
			lineColour[3] = old;
		}

		// bottom border
		SCR_AdjustedFillRect(horizOffset, vertOffset + tabHeight - 1, tabWidth, 1, lineColour);

		// left border
		if ((!i && margin) || (i && i == currentConsoleNum)) {
			SCR_AdjustedFillRect(horizOffset, vertOffset, 1, tabHeight, lineColour);
		}

		// right border
		SCR_AdjustedFillRect(horizOffset + tabWidth, vertOffset, 1, tabHeight, lineColour);

		if (i == currentConsoleNum) {
			SCR_AdjustedFontText(horizOffset + 15, vertOffset + 14, 0.24f, g_color_table[7], consoleNames[i], ITEM_TEXTSTYLE_SHADOWED);
		} else {
			SCR_AdjustedFontText(horizOffset + 9, vertOffset + 12, 0.18f, darkTextColour, consoleNames[i], ITEM_TEXTSTYLE_SHADOWED);
		}

		horizOffset += tabWidth;
	}

	totalOffset = horizOffset;

	if (!totalOffset)
		totalOffset = margin;

	lineColour[3] = 1;

	SCR_AdjustedFillRect(totalOffset, y + margin, adjustedScreenWidth - totalOffset + margin, 1, lineColour);

	// draw the version number

	// Con_RE_SetColor( g_color_table[ColorIndex(COLOR_RED)] );
	Con_RE_SetColor(lineColour);

	if (con_showVersion && con_showVersion->integer) {
		qtime_t now;
		Com_RealTime(&now);

		char *version = va("%s / %s", SVN_VERSION, Cvar_VariableString("ui_modversion"));

		i = strlen(version) + 16;

		lineColour[3] *= opacityMult;
		if (con_timeDisplay->integer == 1 || con_timeDisplay->integer == 3)
			SCR_DrawSmallStringExt(cls.glconfig.vidWidth - i * (SMALLCHAR_WIDTH - 1) - adjustedXMargin,
				(lines-(SMALLCHAR_HEIGHT+SMALLCHAR_HEIGHT/2)) + adjustedYMargin,
				va("[%02i:%02i:%02i]", now.tm_hour, now.tm_min, now.tm_sec), lineColour, qtrue);

		lineColour[3] = 0.3 * opacityMult;
		SCR_DrawSmallStringExt(cls.glconfig.vidWidth - (i - 12) * (SMALLCHAR_WIDTH - 1) - adjustedXMargin,
			(lines-(SMALLCHAR_HEIGHT+SMALLCHAR_HEIGHT/2)) + adjustedYMargin,
			version, lineColour, qtrue);
		lineColour[3] = 1 * opacityMult;
	}

	// draw the text
	currentCon->vislines = lines;
	rows = (lines-(SMALLCHAR_WIDTH - 1))/(SMALLCHAR_WIDTH - 1);		// rows of text to draw

	y = lines - (SMALLCHAR_HEIGHT*3);

	// draw from the bottom up
	if (currentCon->display != currentCon->current)
	{
	// draw arrows to show the buffer is backscrolled
		Con_RE_SetColor(lineColour);
		for (x=0 ; x<currentCon->linewidth ; x+=3)
			SCR_DrawSmallChar( currentCon->xadjust + (x+1)*(SMALLCHAR_WIDTH - 1) + adjustedXMargin, y + margin * 2, '*' );
		y -= SMALLCHAR_HEIGHT;
		rows--;
	}

	row = currentCon->display;

	if ( currentCon->x == 0 ) {
		row--;
	}


	//-------------------------------------------------------------------------
	if (con_drawScrollbar->integer && currentCon->displayFrac == currentCon->finalFrac) {
		vec4_t scrollbarBG;
		scrollbarBG[0] = scrollbarBG[1] = scrollbarBG[2] = 1;
		scrollbarBG[3] = 0.2;

		int scrollbarBGHeight;
		int visibleHeight;
		int scrollbarPos;
		int totalLines = currentCon->current;
		int visible = rows;

		if (con_height->integer >= 0 && con_height->integer <= 100) {
			scrollbarBGHeight = ((con_height->integer/100.0) * SCREEN_HEIGHT) - 60;
		} else {
			scrollbarBGHeight = 180;
		}

		if (scrollbarBGHeight >= 10) {
			visibleHeight = visible / (float)totalLines * scrollbarBGHeight;
			scrollbarPos = currentCon->display / (float)totalLines * (scrollbarBGHeight - visibleHeight);

			SCR_AdjustedFillRect(618 - margin, margin + 30, 2, scrollbarBGHeight, scrollbarBG);
			scrollbarBG[3] = 0.8;
			SCR_AdjustedFillRect(618 - margin, margin + 30 + scrollbarPos, 2, visibleHeight, scrollbarBG);
		}
	}
	//-------------------------------------------------------------------------


	currentColor = 7;
	Con_RE_SetColor( g_color_table[currentColor] );

	for (i=0 ; i<rows ; i++, y -= SMALLCHAR_HEIGHT, row--)
	{
		if (y < 0) {
			break;
		}

		if (row < 0)
			break;
		if (currentCon->current - row >= currentCon->totallines) {
			// past scrollback wrap point
			continue;
		}

		text = currentCon->text + (row % currentCon->totallines)*currentCon->linewidth;

		for (x=0 ; x<currentCon->linewidth ; x++) {
			if ( ( text[x] & 0xff ) == ' ' ) {
				continue;
			}

			if ( ( (text[x]>>8)&7 ) != currentColor ) {
				currentColor = (text[x] >> 8) % 10;
				Con_RE_SetColor( g_color_table[currentColor] );
			}
			SCR_DrawSmallChar(currentCon->xadjust + (x+1)*(SMALLCHAR_WIDTH - 1) + adjustedXMargin, y + adjustedYMargin, text[x] & 0xff );
		}
	}

	// draw the input prompt, user text, and cursor if desired
	Con_DrawInput ();

	Con_RE_SetColor( NULL );
}



/*
==================
Con_DrawConsole
==================
*/
void Con_DrawConsole( void ) {
	if (com_developer && com_developer->integer) {
		numConsoles = 6;
	} else {
		numConsoles = 5;
	}

	// check for console width changes from a vid mode change
	Con_CheckResize (currentCon);

	// if disconnected, render console full screen
	if ( cls.state == CA_DISCONNECTED ) {
		if ( !( cls.keyCatchers & (KEYCATCH_UI | KEYCATCH_CGAME)) ) {
			Con_DrawSolidConsole( 1.0 );
			return;
		}
	}

	if ( currentCon->displayFrac ) {
		Con_DrawSolidConsole( currentCon->displayFrac );
	} else {
		// draw notify lines
		if ( cls.state == CA_ACTIVE ) {
			Con_DrawNotify ();
		}
	}
}

//================================================================

/*
==================
Con_RunConsole

Scroll it up or down
==================
*/
void Con_RunConsole (void) {
	adjustedScreenWidth = SCREEN_WIDTH;
	margin = 0;
	adjustedXMargin = 0;
	adjustedYMargin = 0;

	if (con_margin && con_margin->integer > 0 && con_margin->integer <= 50) {
		Cvar_Set("con_fadeIn", "1");
		adjustedScreenWidth = SCREEN_WIDTH - con_margin->integer * 2;
		margin = con_margin->integer;
		adjustedXMargin = margin * (cls.glconfig.vidWidth / 640.0);
		adjustedYMargin = margin * (cls.glconfig.vidHeight / 480.0);
		currentCon->yadjust = margin;
	}

	// decide on the destination height of the console
	if ( cls.keyCatchers & KEYCATCH_CONSOLE ) {
		if (con_height->integer >= 0 && con_height->integer <= 100)
			currentCon->finalFrac = con_height->integer / 100.0;
		else
			currentCon->finalFrac = 0.5;		// half screen

		targetOpacityMult = 1;
	} else {
		currentCon->finalFrac = 0;				// none visible

		targetOpacityMult = 0;
	}

	if (con_fadeIn && con_fadeIn->integer) {
		float moveDist = con_conspeed->value*cls.realFrametime*0.001;
		if (targetOpacityMult < opacityMult) {
			opacityMult -= moveDist;
			if (opacityMult < targetOpacityMult)
				opacityMult = targetOpacityMult;
		} else if (targetOpacityMult > opacityMult) {
			opacityMult += moveDist;
			if (opacityMult > targetOpacityMult)
				opacityMult = targetOpacityMult;
		}

		if (con_height->integer >= 0 && con_height->integer <= 100)
			currentCon->finalFrac = con_height->integer / 100.0;
		else
			currentCon->finalFrac = 0.5;		// half screen

		if (!targetOpacityMult && !opacityMult)
			currentCon->displayFrac = 0;
		else
			currentCon->displayFrac = currentCon->finalFrac;
	} else {
		// scroll towards the destination height
		if (currentCon->finalFrac < currentCon->displayFrac)
		{
			currentCon->displayFrac -= con_conspeed->value*cls.realFrametime*0.001;
			if (currentCon->finalFrac > currentCon->displayFrac)
				currentCon->displayFrac = currentCon->finalFrac;

		}
		else if (currentCon->finalFrac > currentCon->displayFrac)
		{
			currentCon->displayFrac += con_conspeed->value*cls.realFrametime*0.001;
			if (currentCon->finalFrac < currentCon->displayFrac)
				currentCon->displayFrac = currentCon->finalFrac;
		}
	}

}


void Con_PageUp( void ) {
	currentCon->display -= 2;
	if ( currentCon->current - currentCon->display >= currentCon->totallines ) {
		currentCon->display = currentCon->current - currentCon->totallines + 1;
	}
}

void Con_PageDown( void ) {
	currentCon->display += 2;
	if (currentCon->display > currentCon->current) {
		currentCon->display = currentCon->current;
	}
}

void Con_Top( void ) {
	currentCon->display = currentCon->totallines;
	if ( currentCon->current - currentCon->display >= currentCon->totallines ) {
		currentCon->display = currentCon->current - currentCon->totallines + 1;
	}
}

void Con_Bottom( void ) {
	currentCon->display = currentCon->current;
}

void Con_SpecificBottom(console_t *console) {
	console->display = console->current;
}

void Con_Close( void ) {
	if ( !com_cl_running->integer ) {
		return;
	}
	Field_Clear( &g_consoleField );
	Con_ClearNotify ();
	cls.keyCatchers &= ~KEYCATCH_CONSOLE;
	currentCon->finalFrac = 0;				// none visible
	currentCon->displayFrac = 0;
}

int hourTo12(int hour) {
	return (hour % 12) ? (hour % 12) : 12;
}
