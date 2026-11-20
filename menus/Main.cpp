/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/


#include "Framework.h"
#include "Action.h"
#include "Bitmap.h"
#include "PicButton.h"
#include "YesNoMessageBox.h"
#include "keydefs.h"
#include "MenuStrings.h"
#include "PlayerIntroduceDialog.h"
#include "gameinfo.h"
#include "AnimatedBanner.h"
#include "MovieBanner.h"

#define MAX_GAMEMENU_ITEMS	16

#define GAMEMENU_ONLYINGAME	BIT( 0 )
#define GAMEMENU_NOTMULTI	BIT( 1 )
#define GAMEMENU_NOTSINGLE	BIT( 2 )

#define ART_MINIMIZE_N	"gfx/shell/min_n"
#define ART_MINIMIZE_F	"gfx/shell/min_f"
#define ART_MINIMIZE_D	"gfx/shell/min_d"
#define ART_CLOSEBTN_N	"gfx/shell/cls_n"
#define ART_CLOSEBTN_F	"gfx/shell/cls_f"
#define ART_CLOSEBTN_D	"gfx/shell/cls_d"

class CMenuMain: public CMenuFramework
{
public:
	CMenuMain() : CMenuFramework( "CMenuMain" ) { }

	bool KeyDown( int key ) override;

	void QuitDialogCb();

private:
	void _Init() override;
	void _VidInit( ) override;
	void Think() override;

	void VidInit(bool connected);

	void DisconnectCb();
	void DisconnectDialogCb();
	void HazardCourseDialogCb();
	void HazardCourseCb();

	CMenuAnimatedBanner animatedBanner;
	CMenuMovieBanner movieBanner;

	CMenuPicButton	console;
	CMenuPicButton	resumeGame;
	CMenuPicButton	disconnect;
	CMenuPicButton	newGame;
	CMenuPicButton	hazardCourse;
	CMenuPicButton	configuration;
	CMenuPicButton	saveRestore;
	CMenuPicButton	multiPlayer;
	CMenuPicButton	customGame;
	CMenuPicButton	previews;
	CMenuPicButton	quit;

	// buttons on top right. Maybe should be drawn if fullscreen == 1?
	CMenuBitmap	minimizeBtn;
	CMenuBitmap	quitButton;

	// quit dialog
	CMenuYesNoMessageBox dialog;

	bool bTrainMap;
	bool bCustomGame;

	bool LoadGameMenuRes( void );
	void GameMenuCommand( const char *command );
	void UpdateGameMenuItems( void );
	void GameMenuItemCb( void *pExtra );

	bool bGameMenuRes;
	int iNumGameMenuItems;
	CMenuPicButton gameMenuItems[MAX_GAMEMENU_ITEMS];
	char gameMenuLabels[MAX_GAMEMENU_ITEMS][128];
	char gameMenuNames[MAX_GAMEMENU_ITEMS][128];
	char gameMenuCommands[MAX_GAMEMENU_ITEMS][256];
	int gameMenuFlags[MAX_GAMEMENU_ITEMS];
};

bool CMenuMain::LoadGameMenuRes( void )
{
	char *afile = (char *)EngFuncs::COM_LoadFile( "resource/GameMenu.res", NULL );
	char *pfile = afile;
	char token[1024];
	int depth = 0;
	int item = -1;

	iNumGameMenuItems = 0;

	if( !afile )
		return false;

	while(( pfile = EngFuncs::COM_ParseFile( pfile, token, sizeof( token ))) != NULL )
	{
		if( !strcmp( token, "{" ))
		{
			depth++;

			if( depth == 2 && iNumGameMenuItems < MAX_GAMEMENU_ITEMS )
			{
				item = iNumGameMenuItems++;
				gameMenuLabels[item][0] = 0;
				gameMenuCommands[item][0] = 0;
				gameMenuFlags[item] = 0;
			}
			continue;
		}

		if( !strcmp( token, "}" ))
		{
			if( depth == 2 )
				item = -1;

			depth--;
			continue;
		}

		if( depth != 2 || item < 0 )
			continue;

		char key[1024];
		Q_strncpy( key, token, sizeof( key ));

		pfile = EngFuncs::COM_ParseFile( pfile, token, sizeof( token ));

		if( !pfile )
			break;

		if( !stricmp( key, "label" ))
			Q_strncpy( gameMenuLabels[item], token, sizeof( gameMenuLabels[item] ));
		else if( !stricmp( key, "command" ))
			Q_strncpy( gameMenuCommands[item], token, sizeof( gameMenuCommands[item] ));
		else if( !stricmp( key, "OnlyInGame" ) && atoi( token ))
			gameMenuFlags[item] |= GAMEMENU_ONLYINGAME;
		else if( !stricmp( key, "notmulti" ) && atoi( token ))
			gameMenuFlags[item] |= GAMEMENU_NOTMULTI;
		else if( !stricmp( key, "notsingle" ) && atoi( token ))
			gameMenuFlags[item] |= GAMEMENU_NOTSINGLE;
	}

	EngFuncs::COM_FreeFile( afile );

	return iNumGameMenuItems > 0;
}

void CMenuMain::GameMenuCommand( const char *command )
{
	if( !command || !command[0] )
		return;

	if( !stricmp( command, "OpenNewGameDialog" ))
		UI_NewGame_Menu();
	else if( !stricmp( command, "OpenLoadGameDialog" ))
		UI_LoadGame_Menu();
	else if( !stricmp( command, "OpenSaveGameDialog" ))
		UI_SaveGame_Menu();
	else if( !stricmp( command, "OpenOptionsDialog" ))
		UI_Options_Menu();
	else if( !stricmp( command, "OpenServerBrowser" ))
		UI_GameMenu_ServerBrowser();
	else if( !stricmp( command, "OpenCreateMultiplayerGameDialog" ))
		UI_CreateGame_Menu();
	else if( !stricmp( command, "OpenChangeGameDialog" ))
		UI_CustomGame_Menu();
	else if( !stricmp( command, "OpenPlayerListDialog" ))
		UI_MultiPlayer_Menu();
	else if( !stricmp( command, "Quit" ))
		QuitDialogCb();
	else if( !stricmp( command, "QuitNoConfirm" ))
		EngFuncs::ClientCmd( false, "quit \"menu\"\n" );
	else if( !stricmp( command, "ResumeGame" ))
		UI_CloseMenu();
	else if( !stricmp( command, "Disconnect" ))
		EngFuncs::ClientCmd( false, "disconnect\n" );
	else if( !strncmp( command, "engine ", 7 ))
	{
		char cmd[256];

		snprintf( cmd, sizeof( cmd ), "%s\n", command + 7 );
		EngFuncs::ClientCmd( false, cmd );
		UI_CloseMenu();
	}
}

void CMenuMain::GameMenuItemCb( void *pExtra )
{
	int item = (int)(intptr_t)pExtra;

	if( item < 0 || item >= iNumGameMenuItems )
		return;

	GameMenuCommand( gameMenuCommands[item] );
}

void CMenuMain::UpdateGameMenuItems( void )
{
	bool isInGame = CL_IsActive();
	bool isMulti = isInGame && gpGlobals->maxClients > 1;
	int hoffset = ( 70 / 640.0 ) * 1024.0;
	int ygap = (( 404 - 373 ) / 480.0 ) * 768.0;
	int yoffset = ( 404 / 480.0 ) * 768.0 + ygap;
	int i;

	for( i = iNumGameMenuItems - 1; i >= 0; i-- )
	{
		bool visible = true;

		if(( gameMenuFlags[i] & GAMEMENU_ONLYINGAME ) && !isInGame )
			visible = false;
		else if(( gameMenuFlags[i] & GAMEMENU_NOTMULTI ) && isMulti )
			visible = false;

		gameMenuItems[i].SetVisibility( visible );

		if( visible )
		{
			gameMenuItems[i].SetCoord( hoffset, yoffset );
			yoffset -= ygap;
		}
	}

	console.SetVisibility( gpGlobals->developer );
	console.SetCoord( hoffset, yoffset );
}

void CMenuMain::QuitDialogCb()
{
	if( CL_IsActive() && EngFuncs::GetCvarFloat( "host_serverstate" ) && EngFuncs::GetCvarFloat( "maxplayers" ) == 1.0f )
		dialog.SetMessage( L( "StringsList_235" ) );
	else
		dialog.SetMessage( L( "GameUI_QuitConfirmationText" ) );

	dialog.onPositive.SetCommand( false, "quit \"menu dialog\"\n" );
	dialog.Show();
}

void CMenuMain::DisconnectCb()
{
	EngFuncs::ClientCmd( false, "disconnect\n" );
	VidInit( false );
	CalcPosition();
	CalcSizes();
	VidInitItems();
}

void CMenuMain::DisconnectDialogCb()
{
	dialog.onPositive = VoidCb( &CMenuMain::DisconnectCb );
	dialog.SetMessage( L( "Really disconnect?" ) );
	dialog.Show();
}

void CMenuMain::HazardCourseDialogCb()
{
	dialog.onPositive = VoidCb( &CMenuMain::HazardCourseCb );;
	dialog.SetMessage( L( "StringsList_234" ) );
	dialog.Show();
}

/*
=================
CMenuMain::Key
=================
*/
bool CMenuMain::KeyDown( int key )
{
	if( UI::Key::IsEscape( key ) )
	{
		if ( CL_IsActive( ))
		{
			if( !dialog.IsVisible() )
				UI_CloseMenu();
		}
		else
		{
			QuitDialogCb( );
		}
		return true;
	}
	return CMenuFramework::KeyDown( key );
}

/*
=================
UI_Main_HazardCourse
=================
*/
void CMenuMain::HazardCourseCb()
{
	if( EngFuncs::GetCvarFloat( "host_serverstate" ) && EngFuncs::GetCvarFloat( "maxplayers" ) > 1 )
		EngFuncs::HostEndGame( "end of the game" );

	EngFuncs::CvarSetValue( "skill", 1.0f );
	EngFuncs::CvarSetValue( "deathmatch", 0.0f );
	EngFuncs::CvarSetValue( "teamplay", 0.0f );
	EngFuncs::CvarSetValue( "pausable", 1.0f ); // singleplayer is always allowing pause
	EngFuncs::CvarSetValue( "coop", 0.0f );
	EngFuncs::CvarSetValue( "maxplayers", 1.0f ); // singleplayer

	EngFuncs::PlayBackgroundTrack( NULL, NULL );

	EngFuncs::ClientCmd( false, "hazardcourse\n" );
}

void CMenuMain::_Init( void )
{
	bGameMenuRes = LoadGameMenuRes();

	// console
	console.SetNameAndStatus( L( "GameUI_Console" ), bGameMenuRes ? "" : L( "Show console" ) );
	console.iFlags |= QMF_NOTIFY;
	console.SetPicture( PC_CONSOLE );
	console.SetVisibility( gpGlobals->developer );
	SET_EVENT_MULTI( console.onReleased,
	{
		UI_SetActiveMenu( false );
		EngFuncs::KEY_SetDest( KEY_CONSOLE );
	});

	if( bGameMenuRes )
	{
		int i;

		quitButton.SetPicture( ART_CLOSEBTN_N, ART_CLOSEBTN_F, ART_CLOSEBTN_D );
		quitButton.iFlags = QMF_MOUSEONLY;
		quitButton.eFocusAnimation = QM_HIGHLIGHTIFFOCUS;
		quitButton.onReleased = VoidCb( &CMenuMain::QuitDialogCb );

		minimizeBtn.SetPicture( ART_MINIMIZE_N, ART_MINIMIZE_F, ART_MINIMIZE_D );
		minimizeBtn.iFlags = QMF_MOUSEONLY;
		minimizeBtn.eFocusAnimation = QM_HIGHLIGHTIFFOCUS;
		minimizeBtn.onReleased.SetCommand( false, "minimize\n" );

		dialog.Link( this );

		if( FBitSet( gMenu.m_gameinfo.flags, GFL_ANIMATED_TITLE ))
		{
			if( !uiStatic.lowmemory && animatedBanner.TryLoad())
				AddItem( animatedBanner );
		}
		else
		{
			AddItem( movieBanner );
		}

		AddItem( banner );

		for( i = 0; i < iNumGameMenuItems; i++ )
		{
			const char *src = L( gameMenuLabels[i] );
			char *dst = gameMenuNames[i];
			char *end = gameMenuNames[i] + sizeof( gameMenuNames[i] ) - 1;

			while( *src && dst < end )
			{
				if( *src == '&' )
				{
					src++;
					if( *src != '&' )
						continue;
				}
				*dst++ = *src++;
			}
			*dst = 0;

			gameMenuItems[i].SetNameAndStatus( gameMenuNames[i], "" );
			gameMenuItems[i].iFlags |= QMF_NOTIFY;
			gameMenuItems[i].onReleased = MenuCb( &CMenuMain::GameMenuItemCb );
			gameMenuItems[i].onReleased.pExtra = (void *)(intptr_t)i;
			AddItem( gameMenuItems[i] );
		}

		AddItem( console );
		AddItem( minimizeBtn );
		AddItem( quitButton );
		return;
	}

	if( gMenu.m_gameinfo.trainmap[0] && stricmp( gMenu.m_gameinfo.trainmap, gMenu.m_gameinfo.startmap ) != 0 )
		bTrainMap = true;
	else bTrainMap = false;

	if( EngFuncs::GetCvarFloat( "host_allow_changegame" ))
		bCustomGame = true;
	else bCustomGame = false;

	resumeGame.SetNameAndStatus( L( "GameUI_GameMenu_ResumeGame" ), L( "StringsList_188" ) );
	resumeGame.SetPicture( PC_RESUME_GAME );
	resumeGame.iFlags |= QMF_NOTIFY;
	resumeGame.onReleased = UI_CloseMenu;

	disconnect.SetNameAndStatus( L( "GameUI_GameMenu_Disconnect" ), L( "Disconnect from server" ) );
	disconnect.SetPicture( PC_DISCONNECT );
	disconnect.iFlags |= QMF_NOTIFY;
	disconnect.onReleased = VoidCb( &CMenuMain::DisconnectDialogCb );

	newGame.SetNameAndStatus( L( "GameUI_NewGame" ), L( "StringsList_189" ) );
	newGame.SetPicture( PC_NEW_GAME );
	newGame.iFlags |= QMF_NOTIFY;
	newGame.onReleased = UI_NewGame_Menu;

	hazardCourse.SetNameAndStatus( L( "GameUI_TrainingRoom" ), L( "StringsList_190" ) );
	hazardCourse.SetPicture( PC_HAZARD_COURSE );
	hazardCourse.iFlags |= QMF_NOTIFY;
	hazardCourse.onReleasedClActive = VoidCb( &CMenuMain::HazardCourseDialogCb );
	hazardCourse.onReleased = VoidCb( &CMenuMain::HazardCourseCb );

	multiPlayer.SetNameAndStatus( L( "GameUI_Multiplayer" ), L( "StringsList_198" ) );
	multiPlayer.SetPicture( PC_MULTIPLAYER );
	multiPlayer.iFlags |= QMF_NOTIFY;
	multiPlayer.onReleased = UI_MultiPlayer_Menu;

	configuration.SetNameAndStatus( L( "GameUI_Options" ), L( "StringsList_193" ) );
	configuration.SetPicture( PC_CONFIG );
	configuration.iFlags |= QMF_NOTIFY;
	configuration.onReleased = UI_Options_Menu;

	saveRestore.iFlags |= QMF_NOTIFY;

	customGame.SetNameAndStatus( L( "GameUI_ChangeGame" ), L( "StringsList_530" ) );
	customGame.SetPicture( PC_CUSTOM_GAME );
	customGame.iFlags |= QMF_NOTIFY;
	customGame.onReleased = UI_CustomGame_Menu;

	previews.SetNameAndStatus( L( "Previews" ), L( "StringsList_400" ) );
	previews.SetPicture( PC_PREVIEWS );
	previews.iFlags |= QMF_NOTIFY;
	SET_EVENT( previews.onReleased, EngFuncs::ShellExecute( MenuStrings[ IDS_MEDIA_PREVIEWURL ], NULL, false ) );

	quit.SetNameAndStatus( L( "GameUI_GameMenu_Quit" ), L( "GameUI_QuitConfirmationText" ) );
	quit.SetPicture( PC_QUIT );
	quit.iFlags |= QMF_NOTIFY;
	quit.onReleased = VoidCb( &CMenuMain::QuitDialogCb );

	quitButton.SetPicture( ART_CLOSEBTN_N, ART_CLOSEBTN_F, ART_CLOSEBTN_D );
	quitButton.iFlags = QMF_MOUSEONLY;
	quitButton.eFocusAnimation = QM_HIGHLIGHTIFFOCUS;
	quitButton.onReleased = VoidCb( &CMenuMain::QuitDialogCb );

	minimizeBtn.SetPicture( ART_MINIMIZE_N, ART_MINIMIZE_F, ART_MINIMIZE_D );
	minimizeBtn.iFlags = QMF_MOUSEONLY;
	minimizeBtn.eFocusAnimation = QM_HIGHLIGHTIFFOCUS;
	minimizeBtn.onReleased.SetCommand( false, "minimize\n" );

	if ( gMenu.m_gameinfo.gamemode == GAME_MULTIPLAYER_ONLY || gMenu.m_gameinfo.startmap[0] == 0 )
		newGame.SetGrayed( true );

	if ( gMenu.m_gameinfo.gamemode == GAME_SINGLEPLAYER_ONLY )
		multiPlayer.SetGrayed( true );

	if ( gMenu.m_gameinfo.gamemode == GAME_MULTIPLAYER_ONLY )
	{
		saveRestore.SetGrayed( true );
		hazardCourse.SetGrayed( true );
	}

	// too short execute string - not a real command
	if( strlen( MenuStrings[IDS_MEDIA_PREVIEWURL] ) <= 3 )
	{
		previews.SetGrayed( true );
	}

	// server.dll needs for reading savefiles or startup newgame
	if( !EngFuncs::CheckGameDll( ))
	{
		saveRestore.SetGrayed( true );
		hazardCourse.SetGrayed( true );
		newGame.SetGrayed( true );
	}

	if( FBitSet( gMenu.m_gameinfo.flags, GFL_ANIMATED_TITLE ))
	{
		if( !uiStatic.lowmemory && animatedBanner.TryLoad())
			AddItem( animatedBanner );
	}
	else
	{
		AddItem( movieBanner );
	}

	dialog.Link( this );

	AddItem( banner );
	AddItem( console );
	AddItem( disconnect );
	AddItem( resumeGame );
	AddItem( newGame );

	if ( bTrainMap )
		AddItem( hazardCourse );

	AddItem( configuration );
	AddItem( saveRestore );
	AddItem( multiPlayer );

	if ( bCustomGame )
		AddItem( customGame );

	AddItem( previews );
	AddItem( quit );
	AddItem( minimizeBtn );
	AddItem( quitButton );
}

/*
=================
UI_Main_Init
=================
*/
void CMenuMain::VidInit( bool connected )
{
	if( bGameMenuRes )
	{
		minimizeBtn.SetRect( uiStatic.width - 72, 13, 32, 32 );
		quitButton.SetRect( uiStatic.width - 36, 13, 32, 32 );
		UpdateGameMenuItems();
		return;
	}

	int hoffset = ( 70 / 640.0 ) * 1024.0;

	// in original menu Previews is located at specific point
	int previews_voffset = ( 404 / 480.0 ) * 768.0;

	// no visible console button gap
	int ygap = (( 404 - 373 ) / 480.0 ) * 768.0;

	// statically positioned items
	minimizeBtn.SetRect( uiStatic.width - 72, 13, 32, 32 );
	quitButton.SetRect( uiStatic.width - 36, 13, 32, 32 );

	previews.SetCoord( hoffset, previews_voffset );
	quit.SetCoord( hoffset, previews_voffset + ygap );

	// let's start calculating positions
	int yoffset = previews_voffset - ygap;

	if( bCustomGame )
	{
		customGame.SetCoord( hoffset, yoffset );
		yoffset -= ygap;
	}

	multiPlayer.SetCoord( hoffset, yoffset );
	yoffset -= ygap;

	bool single = gpGlobals->maxClients < 2;

	saveRestore.SetCoord( hoffset, yoffset );
	yoffset -= ygap;

	configuration.SetCoord( hoffset, yoffset );
	yoffset -= ygap;

	if( bTrainMap )
	{
		hazardCourse.SetCoord( hoffset, yoffset );
		yoffset -= ygap;
	}

	newGame.SetCoord( hoffset, yoffset );
	yoffset -= ygap;

	if( connected )
	{
		resumeGame.SetCoord( hoffset, yoffset );
		yoffset -= ygap;

		if( !single )
		{
			disconnect.SetCoord( hoffset, yoffset );
			yoffset -= ygap;
		}
	}

	console.SetCoord( hoffset, yoffset );
	yoffset -= ygap;

	// now figure out what's visible
	resumeGame.SetVisibility( connected );
	disconnect.SetVisibility( connected && !single );

	if( connected && single )
	{
		saveRestore.SetNameAndStatus( L( "Save\\Load Game" ), L( "StringsList_192" ) );
		saveRestore.SetPicture( PC_SAVE_LOAD_GAME );
		saveRestore.onReleased = UI_SaveLoad_Menu;
	}
	else
	{
		saveRestore.SetNameAndStatus( L( "GameUI_LoadGame" ), L( "StringsList_191" ) );
		saveRestore.SetPicture( PC_LOAD_GAME );
		saveRestore.onReleased = UI_LoadGame_Menu;
	}
}

void CMenuMain::_VidInit()
{
	VidInit( CL_IsActive() );
}

void CMenuMain::Think()
{
	if( bGameMenuRes )
	{
		UpdateGameMenuItems();
		CMenuFramework::Think();
		return;
	}

	if( gpGlobals->developer )
	{
		if( !console.IsVisible( ))
			console.Show();
	}
	else
	{
		if( console.IsVisible( ))
			console.Hide();
	}

	CMenuFramework::Think();
}

ADD_MENU( menu_main, CMenuMain, UI_Main_Menu );

/*
=================
UI_QuitDialog_f

called by the engine when the OS asks to quit (window close button, Cmd+Q on macOS...)
=================
*/
static void UI_QuitDialog_f( void )
{
	// nothing to lose, quit immediately
	if( !CL_IsActive( ))
	{
		EngFuncs::ClientCmd( false, "quit \"menu\"\n" );
		return;
	}

	if( !UI_IsVisible( ))
		UI_Main_Menu();

	menu_main->QuitDialogCb();
}
ADD_COMMAND( menu_quit, UI_QuitDialog_f );
