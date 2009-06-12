
plasmoid.drawAppletBackground = function()
{
  return true;
}

plasmoid.dataUpdate = function(a, b)
{
    print( "DOING SOMETHING" );
    if ( b.current )
      label.text = "Playing " + b.current[ "xesam:title" ] + " from " + b.current[ "xesam:author" ] + " on " + b.current[ "xesam:album" ];
}


layout = new LinearLayout( plasmoid );
layout.setAlignment( 2 );
label1 = new Label( );
label1.text = "This is a javascript applet. Just to show off that we can get data from c++ dataengines, here's info on the currently playing track:";
layout.addItem( label1 );
label = new Label();
layout.addItem( label );

label.text = "This is a Javascript Applet";

plasmoid.dataEngine("amarok-current").connectSource( "current", plasmoid );
