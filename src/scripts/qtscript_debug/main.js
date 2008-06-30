Importer.load("debug.js");

print ( "this is main.js" );

//test engine:

var version = Amarok.Version();
print ( ( "Amarok Version: " + version ));

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
    print ("hey, menu1!");
}

function Menu2Clicked()
{
    print ("hey, menu2!");
}

Amarok.Window.addSeparator();
Amarok.Window.addMenu( "testMenu1" );
Amarok.Window.addMenu( "testMenu2" );

Amarok.Window.Menu.testMenu1.triggered.connect(Menu1Clicked);
Amarok.Window.Menu.testMenu2.triggered.connect(Menu2Clicked);


var TrackInfo = Amarok.Engine.TrackInfo;
var OSD = Amarok.OSD;
var StatusBar = Amarok.Window.Statusbar;

if ( TrackInfo.Artist || TrackInfo.Album )
{
    OSD.setText( "Hey, this is " + TrackInfo.Artist );
    OSD.show();

    StatusBar.shortMessage( "You are listening to album: " + TrackInfo.Album );
}

function Configured()
{
    print( "Configure Button Clicked!" );
}

Amarok.configured.connect( Configured );