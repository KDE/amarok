Importer.load("debug.js");

print ( "this is script.js" );

//test engine:

var version = Amarok.Version();
print ( ( "Amarok Version: " + version ));

var Engine;
Engine = Amarok.Engine;
Engine.Stop( true );
Engine.Play();
Engine.Seek ( 60*1000 );


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

Amarok.Window.testMenu1.triggered.connect(Menu1Clicked);
Amarok.Window.testMenu2.triggered.connect(Menu2Clicked);


var OSD = Amarok.OSD;
OSD.setText( "Hey there!" );
OSD.show();


var StatusBar = Amarok.Window.Statusbar;
StatusBar.shortMessage( "Hey there!" );
