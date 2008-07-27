Importer.loadQtBinding( "qt.core" );
Importer.load( "qtscript_debug/debug" );

Debug.initialize();
Debug.app_name = "QtScript Test";

Debug.debug ( "this is main.js" );

//test engine:

var version = Amarok.Version();
Debug.debug ( ( "Amarok Version: " + version ));

/*
var Engine;
Engine = Amarok.Engine;
Engine.Stop( true );
Engine.Play();
Engine.Seek ( 60*1000 );
*/

//test WindowAction

function Menu1Clicked()
{
    Debug.debug ("hey, menu1!");
}

function Menu2Clicked()
{
    Debug.debug ("hey, menu2!");
}

Amarok.Window.addToolsSeparator();

if ( Amarok.Window.addToolsMenu( "testMenu1" ) )
    Amarok.Window.ToolsMenu.testMenu1.triggered.connect(Menu1Clicked);
else
    Debug.warning ( "Menu1 already exists!" );

if ( Amarok.Window.addToolsMenu( "testMenu2" ) )
    Amarok.Window.ToolsMenu.testMenu2.triggered.connect(Menu2Clicked);
else
    Debug.warning ( "Menu2 already exists!" );


var TrackInfo = Amarok.Engine.currentTrack();
var OSD = Amarok.Window.OSD;
var StatusBar = Amarok.Window.Statusbar;

if ( TrackInfo.isValid )
{
    OSD.setText( "Hey, this is " + TrackInfo.artist );
    OSD.show();

    StatusBar.longMessage( "You are listening to album: " + TrackInfo.album );
}
else
{
    Debug.warning (" Track is not valid! ");
}

function onConfigure()
{
    Amarok.alert( "This script does not require any configuration." );
}

function TrackChanged()
{
    Debug.debug( "Track Changed!" );
}

function TrackSeeked()
{
    Debug.debug( "Track Seeked!" );
}

function TrackFinished()
{
    Debug.debug( "Track Finished!" );
}

function PlaylistCountChanged( count )
{
    Debug.debug( "Playlist Count Changed!" );
}

function VolumeChanged( volume )
{
    print( "Volume changed to: " + volume );
}

Amarok.configured.connect( onConfigure );
Amarok.Engine.trackChanged.connect( TrackChanged );
Amarok.Engine.trackSeeked.connect( TrackSeeked );
Amarok.Engine.trackFinished.connect( TrackFinished );
Amarok.Playlist.CountChanged.connect( PlaylistCountChanged );
Amarok.Engine.volumeChanged.connect( VolumeChanged );

var time = QTime.currentTime();
Debug.debug( time.hour() );
Debug.debug( time.minute() );