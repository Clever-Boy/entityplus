#include "ui_local.h"


#define ART_FRAMEL				"menu/art/frame2_l"
#define ART_FRAMER				"menu/art/frame1_r"
#define ART_BACK0				"menu/art/back_0"
#define ART_BACK1				"menu/art/back_1"

#define ID_BACK					99

typedef struct {
	menuframework_s		menu;

	menutext_s			banner;
	menubitmap_s		framel;
	menubitmap_s		framer;
	
	menutext_s			tableheader;
	menutext_s			scoretexts[SCOREBOARD_LENGTH];
	
	menuradiobutton_s	paintballmode;
	menuradiobutton_s	bigheadmode;
	menuradiobutton_s	machinegunonly;
	menuradiobutton_s	instagib;
	menuradiobutton_s	resetscoreafterdeath;

	menubitmap_s		back;
} scores_t;

static scores_t s_scores;
char	*levelname;

static void Scores_Event( void* ptr, int notification ) {
	if( notification != QM_ACTIVATED ) {
		return;
	}

	switch( ((menucommon_s*)ptr)->id ) {
		case ID_BACK:
			UI_PopMenu();
			break;
	}
}

void Scores_GenerateScoringTable( void ) {
	highscores_t	hs;
	int				i, n;
	char			carnagePad[5][7];
	char			accuracyScorePad[5][5];
	char			accuracyPad[5][3];
	char			secretsScorePad[5][5];

	hs = COM_LoadLevelScores( levelname );

	//calculate paddings for carnage
	for ( i = 0; i < SCOREBOARD_LENGTH; i++ ) {
		strcpy( carnagePad[i], "") ;
		n = 1000000;
		while ( n > 1 ) {
			if ( hs.highscores[i].carnageScore < n ) {
				strcat( carnagePad[i], " " );
				n = n / 10;
			} else
				break;
		}
	}

	//calculate paddings for accuracy score
	for ( i = 0; i < SCOREBOARD_LENGTH; i++ ) {
		strcpy( accuracyScorePad[i], "") ;
		n = 1000;
		while ( n > 1 ) {
			if ( hs.highscores[i].accuracyScore < n ) {
				strcat( accuracyScorePad[i], " " );
				n = n / 10;
			} else
				break;
		}
	}

	//calculate paddings for accuracy percentage
	for ( i = 0; i < SCOREBOARD_LENGTH; i++ ) {
		strcpy( accuracyPad[i], "") ;
		n = 100;
		while ( n > 1 ) {
			if ( hs.highscores[i].accuracy < n ) {
				strcat( accuracyPad[i], " " );
				n = n / 10;
			} else
				break;
		}
	}

	//calculate paddings for secrets score
	for ( i = 0; i < SCOREBOARD_LENGTH; i++ ) {
		strcpy( secretsScorePad[i], "") ;
		n = 1000;
		while ( n > 1 ) {
			if ( hs.highscores[i].secretsScore < n ) {
				strcat( secretsScorePad[i], " " );
				n = n / 10;
			} else
				break;
		}
	}

	for (i = 0; i < SCOREBOARD_LENGTH; i++ ) {
		sprintf( s_scores.scoretexts[i].string, "%i.  | %i%s | %i%s (%i%%)%s | %i%s (%i/%i) |", 
			i + 1,
			hs.highscores[i].carnageScore, carnagePad[i],
			hs.highscores[i].accuracyScore, accuracyScorePad[i], hs.highscores[i].accuracy, accuracyPad[i],
			hs.highscores[i].secretsScore, secretsScorePad[i], hs.highscores[i].secretsFound, hs.highscores[i].secretsCount);
	}
}

static void Scores_MenuInit( void ) {
	int				i;
	int				x, y;
	static			char scorebuffers[SCOREBOARD_LENGTH][4096];

	memset( &s_scores, 0, sizeof(scores_t) );

	Scores_Cache();

	s_scores.menu.wrapAround = qtrue;
	s_scores.menu.fullscreen = qtrue;

	s_scores.banner.generic.type	= MTYPE_BTEXT;
	s_scores.banner.generic.x		= 320;
	s_scores.banner.generic.y		= 16;
	s_scores.banner.string			= "SCORES";
	s_scores.banner.color			= color_white;
	s_scores.banner.style			= UI_CENTER;

	s_scores.framel.generic.type	= MTYPE_BITMAP;
	s_scores.framel.generic.name	= ART_FRAMEL;
	s_scores.framel.generic.flags	= QMF_INACTIVE;
	s_scores.framel.generic.x		= 0;
	s_scores.framel.generic.y		= 78;
	s_scores.framel.width  			= 256;
	s_scores.framel.height  		= 329;

	s_scores.framer.generic.type	= MTYPE_BITMAP;
	s_scores.framer.generic.name	= ART_FRAMER;
	s_scores.framer.generic.flags	= QMF_INACTIVE;
	s_scores.framer.generic.x		= 376;
	s_scores.framer.generic.y		= 76;
	s_scores.framer.width  			= 256;
	s_scores.framer.height  		= 334;

	s_scores.back.generic.type		= MTYPE_BITMAP;
	s_scores.back.generic.name		= ART_BACK0;
	s_scores.back.generic.flags		= QMF_LEFT_JUSTIFY|QMF_PULSEIFFOCUS;
	s_scores.back.generic.callback	= Scores_Event;
	s_scores.back.generic.id		= ID_BACK;
	s_scores.back.generic.x			= 0;
	s_scores.back.generic.y			= 480-64;
	s_scores.back.width				= 128;
	s_scores.back.height			= 64;
	s_scores.back.focuspic			= ART_BACK1;

	y = 196;
	x = 80;
	s_scores.tableheader.generic.type	= MTYPE_TEXT;
	s_scores.tableheader.generic.flags	= QMF_CENTER_JUSTIFY|QMF_INACTIVE;
	s_scores.tableheader.generic.x		= x;
	s_scores.tableheader.generic.y		= y;
	s_scores.tableheader.style			= UI_LEFT|UI_SMALLFONT;
	s_scores.tableheader.color			= color_red;
	s_scores.tableheader.string			= "POS | CARNAGE | ACCURACY    | SECRETS | DEATHS | SKILL | TOTAL";

	for (i = 0; i < SCOREBOARD_LENGTH; i++ ) {
		y+= 16;
		s_scores.scoretexts[i].generic.type		= MTYPE_TEXT;
		s_scores.scoretexts[i].generic.flags	= QMF_CENTER_JUSTIFY|QMF_INACTIVE;
		s_scores.scoretexts[i].generic.x		= x;
		s_scores.scoretexts[i].generic.y		= y;
		s_scores.scoretexts[i].style			= UI_LEFT|UI_SMALLFONT;
		s_scores.scoretexts[i].color			= color_red;
		s_scores.scoretexts[i].string			= scorebuffers[i];
	}
	
	Menu_AddItem( &s_scores.menu, &s_scores.banner );
	Menu_AddItem( &s_scores.menu, &s_scores.framel );
	Menu_AddItem( &s_scores.menu, &s_scores.framer );
	Menu_AddItem( &s_scores.menu, &s_scores.back );

	Menu_AddItem( &s_scores.menu, &s_scores.tableheader );
	for (i = 0; i < SCOREBOARD_LENGTH; i++ )
		Menu_AddItem( &s_scores.menu, &s_scores.scoretexts[i] );

	Scores_GenerateScoringTable();
}


void Scores_Cache( void ) {
	trap_R_RegisterShaderNoMip( ART_FRAMEL );
	trap_R_RegisterShaderNoMip( ART_FRAMER );
	trap_R_RegisterShaderNoMip( ART_BACK0 );
	trap_R_RegisterShaderNoMip( ART_BACK1 );
}

void UI_ScoresMenu( char *mapname ) {
	levelname = mapname;
	Scores_MenuInit();
	UI_PushMenu( &s_scores.menu );
}
