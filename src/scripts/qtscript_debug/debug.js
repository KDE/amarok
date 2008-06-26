var version = Amarok.Version();
print ( "Amarok Version: " + version );

//test engine:
//Amarok.Engine.Stop();
//Amarok.Engine.Play();

//test WindowAction

function MenuClicked()
{
    print ("hey, fuck!");
}

Amarok.Window.addSeparator();
Amarok.Window.addMenu( "testMenu" );

try
{
    Amarok.Window.testMenu.triggered.connect(MenuClicked);
}
catch ( e )
{
    print ( e );
}
