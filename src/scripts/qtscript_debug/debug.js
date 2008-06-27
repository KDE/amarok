var version = Amarok.Version();
print ( "Amarok Version: " + version );


//test engine:
var Engine;
Engine = Amarok.Engine;
//Engine.stop( true );
//Engine.Play();
Engine.Seek ( 60*1000 );
/*
//test WindowAction

function Menu1Clicked()
{
    print ("hey, fuck menu1!");
}

function Menu2Clicked()
{
    print ("hey, fuck menu2!");
}

Amarok.Window.addSeparator();
Amarok.Window.addMenu( "testMenu1" );
Amarok.Window.addMenu( "testMenu2" );

try
{
    Amarok.Window.Menu.testMenu1.triggered.connect(Menu1Clicked);
    Amarok.Window.Menu.testMenu2.triggered.connect(Menu2Clicked);
}
catch ( e )
{
    print ( e );
}
*/
var OSD = Amarok.OSD;
try
{
    OSD.setText( "Hey, fuck OSD!" );
    OSD.show();
}

catch ( e )
{
    print ( e );
}
