#!/usr/bin/env kjscmd
// (c) 2004 Mark Kretschmann <markey@web.de>
// See COPYING file for licensing information.


ScriptManager.connect( ScriptManager, "stop(const QString&)", this, "slotStop" );
ScriptManager.connect( ScriptManager, "configure(const QString&)", this, "slotConfigure" );

// Load the demo gui
var gui = Factory.loadui( "setup.ui" );

// ScriptManager.connect( gui.child( "pushButton2" ), "clicked()", this, "slotPlay" );


////////////////////////////////////////////////////////////////////////////////
// functions
////////////////////////////////////////////////////////////////////////////////

function slotStop( name )
{
    print( "slotStop()\n" );
}


function slotConfigure( name )
{
    print( "slotConfigure()\n" );
    
    gui.exec();
    print( "after gui.exec()\n" );
    
    if ( gui.child( "activatedCheckBox" ).checked ) {
        print( "alarm activated.\n" );
        
        var alarmTime = gui.child( "alarmTimeEdit" ).time;
        print( alarmTime.toString() );
        print( "\n" );
        
        var date = new Date();
        print( date.getTime() );
        print( "\n" );
    }
}


function slotPlay( name )
{
    print( "slotPlay()\n" );
    DcopHandler.play();
}

