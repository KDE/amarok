var version = Amarok.Version();
print ( "Amarok Version: " + version );

//test engine:
//Amarok.Engine.Stop();
//Amarok.Engine.Play();

//test WindowAction

Amarok.Window.addSeparator();
Amarok.Window.addMenu( "testMenu" );

try
{
    Amarok.Window.testMenu.setDisabled( true );
}
catch ( e )
{
    print ( e );
}
