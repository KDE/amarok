plasmoid.drawStandardBackground = true;

plasmoid.dataUpdate = function(a, b)
{
    print( "DOING SOMETHING" );
    label.text = "Playing " + b.current[ "xesam:title" ] + " from " + b.current[ "xesam:author" ] + " on " + b.current[ "xesam:album" ];
}


layout = new LinearLayout( plasmoid );
label = new Label();
layout.addItem( label );

label.text = "This is a Javascript Applet";

plasmoid.dataEngine("amarok-current").connectSource( "current", plasmoid );
