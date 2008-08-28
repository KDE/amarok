Importer.loadQtBinding( "qt.core" );
Importer.include( "debug/debug.js" );

//test engine:

var version = Amarok.Info.version();
print( "Amarok Version: " + version );

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
    print("hey, menu1!");
}

function Menu2Clicked()
{
    print("hey, menu2!");
}

Amarok.Window.addToolsSeparator();

if ( Amarok.Window.addToolsMenu( "id1", "test Menu1" ) )
    Amarok.Window.ToolsMenu.id1.triggered.connect(Menu1Clicked);
else
    print( "Menu1 already exists!" );

if ( Amarok.Window.addToolsMenu( "id2", "test Menu2" ) )
    Amarok.Window.ToolsMenu.id2.triggered.connect(Menu2Clicked);
else
    print( "Menu2 already exists!" );


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
    print(" Track is not valid! ");
}

function onConfigure()
{
    Amarok.alert( "This script does not require any configuration." );
}

function TrackChanged()
{
    print( "Track Changed!" );
}

function TrackSeeked()
{
    print( "Track Seeked!" );
}

function TrackFinished()
{
    print( "Track Finished!" );
}

function PlaylistCountChanged( count )
{
    print( "Playlist Count Changed!" );
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

location = Amarok.Collection.collectionLocation();
for ( var x in location )
{
    print ( location[x] );
}