var version = Amarok.Version();
print ( "Amarok Version: " + version );

//test engine:
//Amarok.Engine.Stop();
//Amarok.Engine.Play();

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
    Amarok.Window.testMenu1.triggered.connect(Menu1Clicked);
    Amarok.Window.testMenu2.triggered.connect(Menu2Clicked);
}
catch ( e )
{
    print ( e );
}
