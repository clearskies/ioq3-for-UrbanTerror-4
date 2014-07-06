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

#define		CON_TEXTSIZE	32768
typedef struct {
	qboolean	initialized;

	short	text[CON_TEXTSIZE];
	int		current;		// line where next message will be printed
	int		x;				// offset in current line for next print
	int		display;		// bottom of console displays this line

	int 	linewidth;		// characters across screen
	int		totallines;		// total lines in console scrollback

	float	xadjust;		// for wide aspect screens

	float	displayFrac;	// aproaches finalFrac at scr_conspeed
	float	finalFrac;		// 0.0 to 1.0 lines of console to display

	int		vislines;		// in scanlines

	int		times[NUM_CON_TIMES];	// cls.realtime time the line was generated
								// for transparent notify lines
	vec4_t color;
} console_t;

extern	console_t	con;

console_t	con;

cvar_t		*con_conspeed;
cvar_t		*con_notifytime;
cvar_t		*con_coloredKills;
cvar_t		*con_coloredHits;

cvar_t		*con_bgAlpha;
cvar_t		*con_bgColour;
cvar_t		*con_prompt;
cvar_t		*con_consoleHeight;
cvar_t		*con_promptColour;
cvar_t		*con_timePrompt;
cvar_t		*con_scrollLock;
cvar_t		*con_drawScrollbar;
cvar_t		*con_fadeIn;

cvar_t		*con_nochat;
qboolean suppressNext = qfalse;

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
		con.text[i] = (ColorIndex(COLOR_WHITE)<<8) | ' ';
	}

	Con_Bottom();		// go to end
}

						
/*
================
Con_Dump_f

Save the console contents out to a file
================
*/
void Con_Dump_f (void)
{
	int		l, x, i;
	short	*line;
	fileHandle_t	f;
	char	buffer[1024];

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("usage: condump <filename>\n");
		return;
	}

	Com_Printf ("Dumped console text to %s.\n", Cmd_Argv(1) );

	f = FS_FOpenFileWrite( Cmd_Argv( 1 ) );
	if (!f)
	{
		Com_Printf ("ERROR: couldn't open.\n");
		return;
	}

	// skip empty lines
	for (l = con.current - con.totallines + 1 ; l <= con.current ; l++)
	{
		line = con.text + (l%con.totallines)*con.linewidth;
		for (x=0 ; x<con.linewidth ; x++)
			if ((line[x] & 0xff) != ' ')
				break;
		if (x != con.linewidth)
			break;
	}

	// write the remaining lines
	buffer[con.linewidth] = 0;
	for ( ; l <= con.current ; l++)
	{
		line = con.text + (l%con.totallines)*con.linewidth;
		for(i=0; i<con.linewidth; i++)
			buffer[i] = line[i] & 0xff;
		for (x=con.linewidth-1 ; x>=0 ; x--)
		{
			if (buffer[x] == ' ')
				buffer[x] = 0;
			else
				break;
		}
		strcat( buffer, "\n" );
		FS_Write(buffer, strlen(buffer), f);
	}

	FS_FCloseFile( f );
}

/*
================
Con_ChatDump_f

Save the console chat out to a file
================
*/
void Con_ChatDump_f (void)
{
	int		l, x, i;
	short	*line;
	fileHandle_t	f;
	char	buffer[1024];

	if (Cmd_Argc() < 2)
	{
		Com_Printf ("usage: chatdump <filename>\n");
		return;
	}
	
	Com_Printf ("Dumped chat to %s.\n", Cmd_Argv(1) );

	f = FS_FOpenFileWrite( Cmd_Argv( 1 ) );
	if (!f)
	{
		Com_Printf ("ERROR: couldn't open.\n");
		return;
	}

	// skip empty lines
	for (l = con.current - con.totallines + 1 ; l <= con.current ; l++)
	{
		line = con.text + (l%con.totallines)*con.linewidth;
		for (x=0 ; x<con.linewidth ; x++)
			if ((line[x] & 0xff) != ' ')
				break;
		if (x != con.linewidth)
			break;
	}

	// write the remaining lines
	buffer[con.linewidth] = 0;
	for ( ; l <= con.current ; l++)
	{
		line = con.text + (l%con.totallines)*con.linewidth;
		for(i=0; i<con.linewidth; i++)
			buffer[i] = line[i] & 0xff;
		for (x=con.linewidth-1 ; x>=0 ; x--)
		{
			if (buffer[x] == ' ')
				buffer[x] = 0;
			else
				break;
		}
		if (!strstr(buffer, ":"))
			continue;
		strcat( buffer, "\n" );
		FS_Write(buffer, strlen(buffer), f);
	}

	FS_FCloseFile( f );
}

						
/*
================
Con_ClearNotify
================
*/
void Con_ClearNotify( void ) {
	int		i;
	
	for ( i = 0 ; i < NUM_CON_TIMES ; i++ ) {
		con.times[i] = 0;
	}
}

						

/*
================
Con_CheckResize

If the line width has changed, reformat the buffer.
================
*/
void Con_CheckResize (void)
{
	int		i, j, width, oldwidth, oldtotallines, numlines, numchars;
	short	tbuf[CON_TEXTSIZE];

	width = (SCREEN_WIDTH / SMALLCHAR_WIDTH) - 2;

	if (width == con.linewidth)
		return;

	if (width < 1)			// video hasn't been initialized yet
	{
		width = DEFAULT_CONSOLE_WIDTH;
		con.linewidth = width;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		for(i=0; i<CON_TEXTSIZE; i++)

			con.text[i] = (ColorIndex(COLOR_WHITE)<<8) | ' ';
	}
	else
	{
		oldwidth = con.linewidth;
		con.linewidth = width;
		oldtotallines = con.totallines;
		con.totallines = CON_TEXTSIZE / con.linewidth;
		numlines = oldtotallines;

		if (con.totallines < numlines)
			numlines = con.totallines;

		numchars = oldwidth;
	
		if (con.linewidth < numchars)
			numchars = con.linewidth;

		Com_Memcpy (tbuf, con.text, CON_TEXTSIZE * sizeof(short));
		for(i=0; i<CON_TEXTSIZE; i++)

			con.text[i] = (ColorIndex(COLOR_WHITE)<<8) | ' ';


		for (i=0 ; i<numlines ; i++)
		{
			for (j=0 ; j<numchars ; j++)
			{
				con.text[(con.totallines - 1 - i) * con.linewidth + j] =
						tbuf[((con.current - i + oldtotallines) %
							  oldtotallines) * oldwidth + j];
			}
		}

		Con_ClearNotify ();
	}

	con.current = con.totallines - 1;
	con.display = con.current;
}


/*
================
Con_Init
================
*/
void Con_Init (void) {
	int		i;

	con_notifytime = Cvar_Get ("con_notifytime", "3", 0);
	con_conspeed = Cvar_Get ("scr_conspeed", "30", 0);

	con_coloredKills = Cvar_Get("con_coloredKills", "0", CVAR_ARCHIVE);
	con_nochat = Cvar_Get("con_nochat", "0", CVAR_ARCHIVE);
	con_coloredHits = Cvar_Get("con_coloredHits", "0", CVAR_ARCHIVE);

	con_bgAlpha = Cvar_Get("con_bgAlpha", "90", CVAR_ARCHIVE);
	con_bgColour = Cvar_Get("con_bgColor", "0", CVAR_ARCHIVE);
	con_prompt = Cvar_Get("con_prompt", "]", CVAR_ARCHIVE);
	con_consoleHeight = Cvar_Get("con_consoleHeight", "50", CVAR_ARCHIVE);
	con_promptColour = Cvar_Get("con_promptColor", "7", CVAR_ARCHIVE);
	con_timePrompt = Cvar_Get("con_timePrompt", "0", CVAR_ARCHIVE);
	con_scrollLock = Cvar_Get("con_scrollLock", "1", CVAR_ARCHIVE);
	con_drawScrollbar = Cvar_Get("con_drawScrollbar", "0", CVAR_ARCHIVE);
	con_fadeIn = Cvar_Get("con_fadeIn", "0", CVAR_ARCHIVE);

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
	Cmd_AddCommand ("condump", Con_Dump_f);

	Cmd_AddCommand ("chatdump", Con_ChatDump_f);
}


/*
===============
Con_Linefeed
===============
*/
void Con_Linefeed (qboolean skipnotify)
{
	int		i;

	// mark time for transparent overlay
	if (con.current >= 0)
	{
	if (skipnotify)
		  con.times[con.current % NUM_CON_TIMES] = 0;
	else
		  con.times[con.current % NUM_CON_TIMES] = cls.realtime;
	}

	con.x = 0;
	if (con.display == con.current)
		con.display++;
	con.current++;
	for(i=0; i<con.linewidth; i++)
		con.text[(con.current%con.totallines)*con.linewidth+i] = (ColorIndex(COLOR_WHITE)<<8) | ' ';
}

int nameToTeamColour(char *name) {
	int i, team = 2;
	char *cs;
	for (i = 0; i < MAX_CLIENTS; i++) {
		cs = cl.gameState.stringData + cl.gameState.stringOffsets[544 + i];
		if (!Q_stricmp(Info_ValueForKey(cs, "n"), name)) {
			team = atoi(Info_ValueForKey(cs, "t"));
			if (team == TEAM_RED) {
				team = 1;
			} else if (team == TEAM_BLUE) {
				team = 4;
			} else {
				team = 2;
			}
			break;
		}
	}
	return team;
}

int damageToColour(int damage) {
	int colour;

	if (damage >= 50) {
		colour = 1;
	} else if (damage >= 25) {
		colour = 8;
	} else if (damage >= 17) {
		colour = 3;
	} else {
		colour = 2;
	}

	return colour;
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
	int		y;
	int		c, l;
	int		color;
	qboolean skipnotify = qfalse;		// NERVE - SMF
	int prev;							// NERVE - SMF

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
	
	if (!con.initialized) {
		con.color[0] = 
		con.color[1] = 
		con.color[2] =
		con.color[3] = 1.0f;
		con.linewidth = -1;
		Con_CheckResize ();
		con.initialized = qtrue;
	}

	if (suppressNext) {
		suppressNext = qfalse;
		return;
	}

	int i, team;
	char player1[MAX_NAME_LENGTH + 1], player2[MAX_NAME_LENGTH + 1];
	char nplayer1[MAX_NAME_LENGTH + 5], nplayer2[MAX_NAME_LENGTH + 5];
	char newtxt[MAX_STRING_CHARS + 1];

	if (con_nochat && con_nochat->integer) {
		qboolean pubChat = qfalse;
		qboolean teamChat = qfalse;
		if (strstr(txt, "^3: ^3"))
			pubChat = qtrue;
		
		if (strstr(txt, "^7: ^3") || strstr(txt, "): ^3"))
			teamChat = qtrue;

		if (con_nochat->integer == 1 && pubChat) {
			suppressNext = qtrue;
			return;
		} else if (con_nochat->integer == 2 && teamChat) {
			suppressNext = qtrue;
			return;
		} else if (con_nochat->integer == 3 && (pubChat || teamChat)) {
			suppressNext = qtrue;
			return;
		}
	}

	if (cls.state == CA_ACTIVE && con_coloredKills && con_coloredKills->integer) {
		char **search;
		int found = 0;
		int killLogNum = Cvar_VariableIntegerValue("cg_drawKillLog");
		if (killLogNum == 1) {
			search = killLog1;
		} else if (killLogNum == 2) {
			search = killLog2;
		} else if (killLogNum == 3) {
			search = killLog3;
		}

		if (killLogNum > 0 && killLogNum < 4) {
			int temp;
			for (i = 0; ; i++) {
				if (!search[i])
					break;

				if (sscanf(txt, search[i], player1, player2) == 2) {
					found = 1;
					if (killLogNum == 1) {
						temp = strlen(player2);
						if (player2[temp - 2] == '\'' && player2[temp - 1] == 's') {
							player2[temp - 2] = 0;
						}
					} else if (killLogNum > 1) {
						temp = strlen(player2);
						player2[temp - 1] = 0;
					}

					team = nameToTeamColour(player1);
					sprintf(nplayer1, "^%i%s^7", team, player1);

					team = nameToTeamColour(player2);
					sprintf(nplayer2, "^%i%s^7", team, player2);

					sprintf(newtxt, search[i], nplayer1, nplayer2);
					txt = newtxt;
					break;
				}
			}

			if (!found) {
				for (i = 0; ; i++) {
					if (!killLogSingle[i]) {
						break;
					}

					if (sscanf(txt, killLogSingle[i], player1, player2) == 2) {
						team = nameToTeamColour(player1);
						sprintf(nplayer1, "^%i%s^7", team, player1);

						sprintf(newtxt, killLogSingle[i], nplayer1, player2);
						txt = newtxt;
						break;
					}
				}
			}

		}
	}

	if (cls.state == CA_ACTIVE && con_coloredHits && con_coloredHits->integer && Cvar_VariableIntegerValue("cg_showbullethits") == 2) {
		char damageString[12];
		int damage, damageCol;
 
		for (i = 0; ; i++) {
			if (!hitLog1[i])
					break;
 
			if (sscanf(txt, hitLog1[i], player1, player2, damageString) == 3) {
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
				break;
			}
		}

		for (i = 0; ; i++) {
			if (!hitLog2[i])
					break;
 
			if (sscanf(txt, hitLog2[i], player1, player2, damageString) == 3) {
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
				sprintf(newtxt, hitLog2[i], nplayer1, nplayer2, damageString);
				txt = newtxt;
				break;
			}
		}
 
		for (i = 0; ; i++) {
			if (!hitLog3[i])
					break;
 
			if (sscanf(txt, hitLog3[i], player2, damageString) == 2) {
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
				break;
			}
		}
 
		for (i = 0; ; i++) {
			if (!hitLog4[i])
					break;
 
			if (sscanf(txt, hitLog4[i], player2, damageString) == 2) {
				damage = atoi(damageString);
				damageCol = damageToColour(damage);

				if (con_coloredHits->integer == 2) {
					team = nameToTeamColour(player2);
					sprintf(nplayer2, "^%i%s^7", team, player2);
				} else {
					sprintf(nplayer2, "%s", player2);
				}

				sprintf(damageString, "^%i%i%%^7", damageCol, damage);
				sprintf(newtxt, hitLog4[i], nplayer2, damageString);
				txt = newtxt;
				break;
			}
		}
	}

	color = ColorIndex(COLOR_WHITE);

	while ( (c = *txt) != 0 ) {
		if ( Q_IsColorString( txt ) ) {
			color = ColorIndex( *(txt+1) );
			txt += 2;
			continue;
		}

		// count word length
		for (l=0 ; l< con.linewidth ; l++) {
			if ( txt[l] <= ' ') {
				break;
			}

		}

		// word wrap
		if (l != con.linewidth && (con.x + l >= con.linewidth) ) {
			Con_Linefeed(skipnotify);

		}

		txt++;

		switch (c)
		{
		case '\n':
			Con_Linefeed (skipnotify);
			break;
		case '\r':
			con.x = 0;
			break;
		default:	// display character and advance
			y = con.current % con.totallines;
			con.text[y*con.linewidth+con.x] = (color << 8) | c;
			con.x++;
			if (con.x >= con.linewidth) {
				Con_Linefeed(skipnotify);
				con.x = 0;
			}
			break;
		}
	}

	if (con_scrollLock && !con_scrollLock->integer) {
		Con_Bottom();
	}


	// mark time for transparent overlay
	if (con.current >= 0) {
		// NERVE - SMF
		if ( skipnotify ) {
			prev = con.current % NUM_CON_TIMES - 1;
			if ( prev < 0 )
				prev = NUM_CON_TIMES - 1;
			con.times[prev] = 0;
		}
		else
		// -NERVE - SMF
			con.times[con.current % NUM_CON_TIMES] = cls.realtime;
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

	y = con.vislines - ( SMALLCHAR_HEIGHT * 2 );

	Con_RE_SetColor( con.color );

	if (con_promptColour->integer >= 0 && con_promptColour->integer < 10) {
		Con_RE_SetColor(g_color_table[con_promptColour->integer]);
	}

	int promptLen = strlen(con_prompt->string) + 1;
	int i;
	char *prompt;

	if (con_timePrompt->integer) {
		qtime_t curTime;
		Com_RealTime(&curTime);
		prompt = Z_Malloc(promptLen + 11);
		Com_sprintf(prompt, promptLen + 11, "[%02i:%02i:%02i] %s", curTime.tm_hour, curTime.tm_min, curTime.tm_sec, con_prompt->string);
	} else {
		prompt = Z_Malloc(promptLen);
		Com_sprintf(prompt, promptLen, "%s", con_prompt->string);
	}

	promptLen = strlen(prompt);

	for (i = 0; i < promptLen; i++) {
		SCR_DrawSmallChar( con.xadjust + (i + 1) * SMALLCHAR_WIDTH, y, prompt[i]);
	}

	Con_RE_SetColor(con.color);

	if (opacityMult)
	Field_Draw( &g_consoleField, con.xadjust + (promptLen + 1) * SMALLCHAR_WIDTH, y,
		SCREEN_WIDTH - 3 * SMALLCHAR_WIDTH, qtrue);
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
	for (i= con.current-NUM_CON_TIMES+1 ; i<=con.current ; i++)
	{
		if (i < 0)
			continue;
		time = con.times[i % NUM_CON_TIMES];
		if (time == 0)
			continue;
		time = cls.realtime - time;
		if (time > con_notifytime->value*1000)
			continue;
		text = con.text + (i % con.totallines)*con.linewidth;

		if (cl.snap.ps.pm_type != PM_INTERMISSION && (cls.keyCatchers & (KEYCATCH_UI | KEYCATCH_CGAME)) ) {
			continue;
		}

		for (x = 0 ; x < con.linewidth ; x++) {
			if ( ( text[x] & 0xff ) == ' ' ) {
				continue;
			}
			if ( ( (text[x]>>8)&7 ) != currentColor ) {
				currentColor = (text[x]>>8)&7;
				Con_RE_SetColor( g_color_table[currentColor] );
			}
			SCR_DrawSmallChar( cl_conXOffset->integer + con.xadjust + (x+1)*SMALLCHAR_WIDTH, v, text[x] & 0xff );
		}

		v += SMALLCHAR_HEIGHT;
	}

	Con_RE_SetColor( NULL );

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
			SCREEN_WIDTH - ( skip + 1 ) * BIGCHAR_WIDTH, qtrue );

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

	lines = cls.glconfig.vidHeight * frac;
	if (lines <= 0)
		return;

	if (lines > cls.glconfig.vidHeight )
		lines = cls.glconfig.vidHeight;

	// on wide screens, we will center the text
	con.xadjust = 0;
	SCR_AdjustFrom640( &con.xadjust, NULL, NULL, NULL );

	// draw the background
	y = frac * SCREEN_HEIGHT - 2;
	if ( y < 1 ) {
		y = 0;
	}
	else {
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
		SCR_AdjustedFillRect(0, 0, SCREEN_WIDTH, y, bgColour);
	}

	lineColour[0] = 0.0/255.0;
	lineColour[1] = 100.0/255.0;
	lineColour[2] = 100.0/255.0;
	lineColour[3] = 1;
	SCR_AdjustedFillRect(0, y, SCREEN_WIDTH, 2, lineColour);

	// draw the version number

	// Con_RE_SetColor( g_color_table[ColorIndex(COLOR_RED)] );
	Con_RE_SetColor(lineColour);

	i = strlen( SVN_VERSION );

	for (x=0 ; x<i ; x++) {

		SCR_DrawSmallChar( cls.glconfig.vidWidth - ( i - x ) * SMALLCHAR_WIDTH, 

			(lines-(SMALLCHAR_HEIGHT+SMALLCHAR_HEIGHT/2)), SVN_VERSION[x] );

	}


	// draw the text
	con.vislines = lines;
	rows = (lines-SMALLCHAR_WIDTH)/SMALLCHAR_WIDTH;		// rows of text to draw

	y = lines - (SMALLCHAR_HEIGHT*3);

	// draw from the bottom up
	if (con.display != con.current)
	{
	// draw arrows to show the buffer is backscrolled
		Con_RE_SetColor(lineColour);
		for (x=0 ; x<con.linewidth ; x+=4)
			SCR_DrawSmallChar( con.xadjust + (x+1)*SMALLCHAR_WIDTH, y, '^' );
		y -= SMALLCHAR_HEIGHT;
		rows--;
	}
	
	row = con.display;

	if ( con.x == 0 ) {
		row--;
	}


	//-------------------------------------------------------------------------
	if (con_drawScrollbar->integer && con.displayFrac == con.finalFrac) {
		vec4_t scrollbarBG;
		scrollbarBG[0] = scrollbarBG[1] = scrollbarBG[2] = 1;
		scrollbarBG[3] = 0.2;

		int scrollbarBGHeight;
		int visibleHeight;
		int scrollbarPos;
		int totalLines = con.current;
		int visible = rows;

		if (con_consoleHeight->integer >= 0 && con_consoleHeight->integer <= 100) {
			scrollbarBGHeight = ((con_consoleHeight->integer/100.0) * SCREEN_HEIGHT) - 60;
		} else {
			scrollbarBGHeight = 180;
		}

		if (scrollbarBGHeight >= 10) {
			visibleHeight = visible / (float)totalLines * scrollbarBGHeight;
			scrollbarPos = (con.display - rows) / (float)(totalLines - rows) * (scrollbarBGHeight - visibleHeight);
			
			SCR_AdjustedFillRect(618, 30, 2, scrollbarBGHeight, scrollbarBG);
			scrollbarBG[3] = 0.8;
			SCR_AdjustedFillRect(618, 30 + scrollbarPos, 2, visibleHeight, scrollbarBG);
		}
	}
	//-------------------------------------------------------------------------


	currentColor = 7;
	Con_RE_SetColor( g_color_table[currentColor] );

	for (i=0 ; i<rows ; i++, y -= SMALLCHAR_HEIGHT, row--)
	{
		if (row < 0)
			break;
		if (con.current - row >= con.totallines) {
			// past scrollback wrap point
			continue;	
		}

		text = con.text + (row % con.totallines)*con.linewidth;

		for (x=0 ; x<con.linewidth ; x++) {
			if ( ( text[x] & 0xff ) == ' ' ) {
				continue;
			}

			if ( ( (text[x]>>8)&7 ) != currentColor ) {
				currentColor = (text[x] >> 8) % 10;
				Con_RE_SetColor( g_color_table[currentColor] );
			}
			SCR_DrawSmallChar(  con.xadjust + (x+1)*SMALLCHAR_WIDTH, y, text[x] & 0xff );
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
	// check for console width changes from a vid mode change
	Con_CheckResize ();

	// if disconnected, render console full screen
	if ( cls.state == CA_DISCONNECTED ) {
		if ( !( cls.keyCatchers & (KEYCATCH_UI | KEYCATCH_CGAME)) ) {
			Con_DrawSolidConsole( 1.0 );
			return;
		}
	}

	if ( con.displayFrac ) {
		Con_DrawSolidConsole( con.displayFrac );
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
	// decide on the destination height of the console
	if ( cls.keyCatchers & KEYCATCH_CONSOLE ) {
		if (con_consoleHeight->integer >= 0 && con_consoleHeight->integer <= 100)
			con.finalFrac = con_consoleHeight->integer / 100.0;
		else 
			con.finalFrac = 0.5;		// half screen

		targetOpacityMult = 1;
	} else {
		con.finalFrac = 0;				// none visible

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

		if (con_consoleHeight->integer >= 0 && con_consoleHeight->integer <= 100)
			con.finalFrac = con_consoleHeight->integer / 100.0;
		else 
			con.finalFrac = 0.5;		// half screen
		con.displayFrac = con.finalFrac;
	}
	
	// scroll towards the destination height
	if (con.finalFrac < con.displayFrac)
	{
		con.displayFrac -= con_conspeed->value*cls.realFrametime*0.001;
		if (con.finalFrac > con.displayFrac)
			con.displayFrac = con.finalFrac;

	}
	else if (con.finalFrac > con.displayFrac)
	{
		con.displayFrac += con_conspeed->value*cls.realFrametime*0.001;
		if (con.finalFrac < con.displayFrac)
			con.displayFrac = con.finalFrac;
	}

}


void Con_PageUp( void ) {
	con.display -= 2;
	if ( con.current - con.display >= con.totallines ) {
		con.display = con.current - con.totallines + 1;
	}
}

void Con_PageDown( void ) {
	con.display += 2;
	if (con.display > con.current) {
		con.display = con.current;
	}
}

void Con_Top( void ) {
	con.display = con.totallines;
	if ( con.current - con.display >= con.totallines ) {
		con.display = con.current - con.totallines + 1;
	}
}

void Con_Bottom( void ) {
	con.display = con.current;
}


void Con_Close( void ) {
	if ( !com_cl_running->integer ) {
		return;
	}
	Field_Clear( &g_consoleField );
	Con_ClearNotify ();
	cls.keyCatchers &= ~KEYCATCH_CONSOLE;
	con.finalFrac = 0;				// none visible
	con.displayFrac = 0;
}
