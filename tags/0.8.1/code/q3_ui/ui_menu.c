// Copyright (C) 1999-2000 Id Software, Inc.
//
/*
=======================================================================

MAIN MENU

=======================================================================
*/


#include "ui_local.h"


#define ID_SINGLEPLAYER				10
#define ID_SETUP					12
#define ID_EXIT						17

#define ART_OVERLAY						"menu/art/mainoverlay"
#define ART_BACKGROUND					"menu/backgrounds/01"
#define ART_BACKGROUND2					"menu/backgrounds/02"

#define MAIN_MENU_VERTICAL_SPACING	34
#define MAIN_MENU_MARGIN_LEFT		48
#define MAIN_MENU_MARGIN_TOP		310



typedef struct {
	menuframework_s	menu;

	menutext_s		singleplayer;
	menutext_s		multiplayer;
	menutext_s		setup;
	menutext_s		demos;
	menutext_s		cinematics;
	menutext_s		teamArena;
	menutext_s		mods;
	menutext_s		exit;

	menutext_s		header;
	menubitmap_s	logo;
	menubitmap_s	overlay;

	qhandle_t		bannerModel;
	qhandle_t		menuModel;
	vec3_t			menuModelOrigin;
	vec3_t			menuModelAngles;

	int				backgroundDelay;
} mainmenu_t;


static mainmenu_t s_main;

typedef struct {
	menuframework_s menu;
	char errorMessage[4096];
} errorMessage_t;

static errorMessage_t s_errorMessage;

/*
=================
MainMenu_ExitAction
=================
*/
static void MainMenu_ExitAction( qboolean result ) {
	if( !result ) {
		return;
	}
	UI_PopMenu();
	UI_CreditMenu();
}



/*
=================
Main_MenuEvent
=================
*/
void Main_MenuEvent (void* ptr, int event) {
	if( event != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
	case ID_SINGLEPLAYER:
		UI_EPLevelMenu();
		break;

	case ID_SETUP:
		UI_SetupMenu();
		break;

	case ID_EXIT:
		UI_ConfirmMenu( "EXIT GAME?", NULL, MainMenu_ExitAction );
		break;
	}
}


/*
===============
MainMenu_Cache
===============
*/
void MainMenu_Cache( void ) {
}

sfxHandle_t ErrorMessage_Key(int key)
{
	trap_Cvar_Set( "com_errorMessage", "" );
	UI_MainMenu();
	return (menu_null_sound);
}

/*
===============
Main_MenuDraw
TTimo: this function is common to the main menu and errorMessage menu
===============
*/

static void Main_MenuDraw( void ) {
	qtime_t tm;
	int seed;

	if (strlen(s_errorMessage.errorMessage))
	{
		UI_DrawProportionalString_AutoWrapped( 320, 192, 600, 20, s_errorMessage.errorMessage, UI_CENTER|UI_SMALLFONT|UI_DROPSHADOW, menu_text_color );
	}
	else
	{
		int t;
		
		if ( !s_main.backgroundDelay )
			s_main.backgroundDelay = 5000;

		t = uis.realtime % s_main.backgroundDelay;
		if ( t < 32 || (t > 60 && t < 92) || (t > 150 && t < 200) || (t > 240 && t < 270) )
		{
			UI_DrawNamedPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ART_BACKGROUND2 );
		}
		else
			UI_DrawNamedPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, ART_BACKGROUND );

		//determine delay until next lighting strike
		if ( t == 270 ) {
			trap_RealTime(&tm);
			seed = 1;
			seed = seed * 31 + tm.tm_sec;
			seed = seed * 31 + tm.tm_min;
			seed = seed * 31 + tm.tm_hour;
			seed = seed * 31 + tm.tm_mday;
			srand( seed );
			
			s_main.backgroundDelay = 5000 + (1500 - (rand() % 3000));	//delay somewhere between 3500 and 6500 ms
		}

		Menu_Draw( &s_main.menu );		
	}
}


/*
===============
UI_TeamArenaExists
===============
*/
static qboolean UI_TeamArenaExists( void ) {
	int		numdirs;
	char	dirlist[2048];
	char	*dirptr;
  char  *descptr;
	int		i;
	int		dirlen;

	numdirs = trap_FS_GetFileList( "$modlist", "", dirlist, sizeof(dirlist) );
	dirptr  = dirlist;
	for( i = 0; i < numdirs; i++ ) {
		dirlen = strlen( dirptr ) + 1;
    descptr = dirptr + dirlen;
		if (Q_stricmp(dirptr, "missionpack") == 0) {
			return qtrue;
		}
    dirptr += dirlen + strlen(descptr) + 1;
	}
	return qfalse;
}


/*
===============
UI_MainMenu

The main menu only comes up when not in a game,
so make sure that the attract loop server is down
and that local cinematics are killed
===============
*/
void UI_MainMenu( void ) {
	int		y;

	trap_Cvar_Set( "sv_killserver", "1" );
	
	memset( &s_main, 0, sizeof(mainmenu_t) );
	memset( &s_errorMessage, 0, sizeof(errorMessage_t) );

	// com_errorMessage would need that too
	MainMenu_Cache();
	
	trap_Cvar_VariableStringBuffer( "com_errorMessage", s_errorMessage.errorMessage, sizeof(s_errorMessage.errorMessage) );
	if (strlen(s_errorMessage.errorMessage))
	{	
		s_errorMessage.menu.draw = Main_MenuDraw;
		s_errorMessage.menu.key = ErrorMessage_Key;
		s_errorMessage.menu.fullscreen = qtrue;
		s_errorMessage.menu.wrapAround = qtrue;
		s_errorMessage.menu.showlogo = qfalse;		

		trap_Key_SetCatcher( KEYCATCH_UI );
		uis.menusp = 0;
		UI_PushMenu ( &s_errorMessage.menu );
		return;
	}
	
	s_main.menu.draw = Main_MenuDraw;
	s_main.menu.fullscreen = qtrue;
	s_main.menu.wrapAround = qtrue;
	s_main.menu.showlogo = qfalse;

	y = MAIN_MENU_MARGIN_TOP;

	//add overlay
	s_main.overlay.generic.type		= MTYPE_BITMAP;
	s_main.overlay.generic.name		= ART_OVERLAY;
	s_main.overlay.generic.flags	= QMF_INACTIVE;
	s_main.overlay.generic.x		= MAIN_MENU_VERTICAL_SPACING;
	s_main.overlay.generic.y		= y - 48;
	s_main.overlay.width  			= 256;
	s_main.overlay.height			= 256;

	//add header
	s_main.header.generic.type		= MTYPE_PTEXT;
	s_main.header.generic.x			= MAIN_MENU_MARGIN_LEFT;
	s_main.header.generic.y			= y;
	s_main.header.string			= "ENTITYPLUS";
	s_main.header.color				= color_ochre;

	//add menu buttons
	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.singleplayer.generic.type		= MTYPE_PTEXT;
	s_main.singleplayer.generic.flags		= QMF_PULSEIFFOCUS;
	s_main.singleplayer.generic.x			= MAIN_MENU_MARGIN_LEFT;
	s_main.singleplayer.generic.y			= y;
	s_main.singleplayer.generic.id			= ID_SINGLEPLAYER;
	s_main.singleplayer.generic.callback	= Main_MenuEvent; 
	s_main.singleplayer.string				= "NEW GAME";
	s_main.singleplayer.color				= color_white;

	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.setup.generic.type				= MTYPE_PTEXT;
	s_main.setup.generic.flags				= QMF_PULSEIFFOCUS;
	s_main.setup.generic.x					= MAIN_MENU_MARGIN_LEFT;
	s_main.setup.generic.y					= y;
	s_main.setup.generic.id					= ID_SETUP;
	s_main.setup.generic.callback			= Main_MenuEvent; 
	s_main.setup.string						= "SETUP";
	s_main.setup.color						= color_white;

	y += MAIN_MENU_VERTICAL_SPACING;
	s_main.exit.generic.type				= MTYPE_PTEXT;
	s_main.exit.generic.flags				= QMF_PULSEIFFOCUS;
	s_main.exit.generic.x					= MAIN_MENU_MARGIN_LEFT;
	s_main.exit.generic.y					= y;
	s_main.exit.generic.id					= ID_EXIT;
	s_main.exit.generic.callback			= Main_MenuEvent; 
	s_main.exit.string						= "EXIT";
	s_main.exit.color						= color_white;

	Menu_AddItem( &s_main.menu,	&s_main.overlay );
	Menu_AddItem( &s_main.menu,	&s_main.header );
	Menu_AddItem( &s_main.menu,	&s_main.singleplayer );
	Menu_AddItem( &s_main.menu,	&s_main.setup );
	Menu_AddItem( &s_main.menu,	&s_main.exit );             

	trap_Key_SetCatcher( KEYCATCH_UI );
	uis.menusp = 0;
	UI_PushMenu ( &s_main.menu );
		
}