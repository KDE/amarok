/**************************************************************************
*   Copy Cover Script for Amarok 2.0                                      *
*                                                                         *
*   Copyright                                                             *
*   (C) 2009 Sven Krohlas <sven@asbest-online.de>                         *
*   (C) 2008 Simon Esneault  <simon.esneault@gmail.com>                   *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
**************************************************************************/


Importer.loadQtBinding( "qt.core" );
Importer.loadQtBinding( "qt.gui" );
Importer.loadQtBinding( "qt.uitools" );

function copyCover()
{
	var TrackInfo = Amarok.Engine.currentTrack();
	if ( TrackInfo.isValid ) // if track is valid
	{
		var path_image=TrackInfo.imageUrl.substring( 7 );
		var image=new QImage( path_image );
		if ( !image.isNull() ) // if we have a cover
		{
			// Make a the correct path
			var path=TrackInfo.path.substring( 0, TrackInfo.path.lastIndexOf( "/" ) + 1 );
            
            if ( writeCover == true )
            {
                var patha=path+"cover.png";
                
                // try to load the image first, to prevent rewritting
                var image2 = new QImage ( patha );
                if ( !image2.isNull() ) // if already an image
                {
                    if ( image2 != image ) // and image are different
                    {
                        if( image.save( patha ) )
                            Amarok.Window.Statusbar.shortMessage( "Copy-Cover has written " + patha );
                        else
                            Amarok.Window.Statusbar.shortMessage( "Copy-Cover can not write " + patha );
                    }
                }
                else
                {
                    if( image.save( patha ) )
                        Amarok.Window.Statusbar.shortMessage( "Copy-Cover has written " + patha );
                    else
                        Amarok.Window.Statusbar.shortMessage( "Copy-Cover can not write " + patha );
                }
            }
            
            if ( writeArtistAlbum == true )
            {
                path+=TrackInfo.artist+"-"+TrackInfo.album+".png";
                
                // try to load the image first, to prevent rewritting
                var image2 = new QImage( path );
                if ( !image2.isNull() ) // if already an image
                {
                    if ( image2 != image ) // and image are different
                    {
                        if( image.save(path ) )
                            Amarok.Window.Statusbar.shortMessage( "Copy-Cover has written " + path );
                        else
                            Amarok.Window.Statusbar.shortMessage( "Copy-Cover can not write " + path );
                    }
                }
                else
                {
                
                    if( image.save( path ) )
                        Amarok.Window.Statusbar.shortMessage( "Copy-Cover has written " + path );
                    else
                        Amarok.Window.Statusbar.shortMessage( "Copy-Cover can not write " + path );
                }
            }            
		}
		else
			Amarok.Window.Statusbar.shortMessage( "Copy-Cover can not read the image" );
		
	}	
	else
	{
		Amarok.debug( "COPY-COVER -> Track not valid " );
	}  
}


function saveConfiguration()
{
    //Pretty messy :S
    if ( mainWindow.widget.checkBox.checked )
    {
        Amarok.Script.writeConfig( "writeCover", "true" );
        writeCover = true;
    }
    else
    {
        Amarok.Script.writeConfig( "writeCover", "false" );
        writeCover = false;
    }
    if ( mainWindow.widget.checkBox_2.checked )
    {
        Amarok.Script.writeConfig( "writeArtistAlbum", "true" );
        writeArtistAlbum = true;
    }
    else
    {
        Amarok.Script.writeConfig( "writeArtistAlbum", "false" );
        writeArtistAlbum = false;
    }
}

function readConfiguration()
{
    if ( Amarok.Script.readConfig( "writeCover", "true" ) == "true" )
        mainWindow.widget.checkBox.setChecked( true );
    else
        mainWindow.widget.checkBox.setChecked( false );

    if ( Amarok.Script.readConfig( "writeArtistAlbum", "false" ) == "false" )
        mainWindow.widget.checkBox_2.setChecked( false );
    else
        mainWindow.widget.checkBox_2.setChecked( true );

}

function openSettings()
{
    mainWindow.show();
    Amarok.debug( "COPY-COVER -> Show configuration" );
}

function init()
{
    try
    {
        // Ui stuff
        var UIloader = new QUiLoader( this );
        var uifile = new QFile ( Amarok.Info.scriptPath() + "/copycover.ui" );
        uifile.open( QIODevice.ReadOnly );
        mainWindow = UIloader.load( uifile, this ); //load the ui file
        uifile.close();

        // read configuration
        readConfiguration();

        // connect the button ok/cancel to save/read config.
        mainWindow.buttonBox.accepted.connect( saveConfiguration );
        mainWindow.buttonBox.rejected.connect( readConfiguration );
        
        // Add tool menu, and a callback
        Amarok.Window.addSettingsMenu( "copycov", "Copy Cover Settings", "amarok" );
        Amarok.Window.SettingsMenu.copycov['triggered()'].connect(openSettings );
        
        // call every track changed
        Amarok.Engine.trackChanged.connect( copyCover ) ;
    }
    catch( err )
    {
        Amarok.debug( err );
    }
}

var writeCover = true;
var writeArtistAlbum = false;
init();





