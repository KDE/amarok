/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  playlisttooltip.cpp  -  Provides an interface to a plain QWidget, which is independent of KDE (bypassed to X11)
  begin:     Tue 10 Feb 2004
  copyright: (C) 2004 by Christian Muehlhaeuser
  email:     chris@chris.de
*/

#include "playlisttooltip.h"
#include "metabundle.h"
#include <dirent.h>
#include <sys/stat.h>
#include <kurl.h>
#include <qtooltip.h>


void PlaylistToolTip::add( QWidget * widget, const MetaBundle & tags )
{
    const KURL &url = tags.url();
    const QString tr = "<tr><td width=70 align=right>%1:</td><td align=left>%2</td></tr>";

    QString tipBuf;

    tipBuf += "<center><table>"; //style='font-face: Arial; font-size: 8px;'
    tipBuf += tr.arg( i18n( "Title" ),  tags.title() );
    tipBuf += tr.arg( i18n( "Artist" ), tags.artist() );
    tipBuf += tr.arg( i18n( "Length" ), tags.prettyLength() );
    tipBuf += tr.arg( i18n( "Bitrate" ), tags.prettyBitrate() );
    tipBuf += tr.arg( i18n( "Samplerate" ), tags.prettySampleRate() );
    tipBuf += "</table>";

    QStringList validExtensions;
    validExtensions << "jpg" << "png" << "gif" << "jpeg";

    DIR *d = opendir( url.directory( FALSE, FALSE ).local8Bit() );
    if ( d )
    {
        const QString td = "<td align=center valign=center><img width=100 src='%1%2'></td>";
        int rowcnt = 0;
        dirent *ent;

        while ( ( ent = readdir( d ) ) )
        {
            QString file( ent->d_name );

            if ( validExtensions.contains( file.mid( file.findRev('.')+1 ) ) )
            {
                // we found an image, let's add it to the tooltip
                if ( rowcnt == 0 )
                    tipBuf += "<table><tr><td></td></tr><tr>"; //extra row for spacing

                tipBuf += td.arg( url.directory( FALSE, TRUE ), file );

                if ( ++rowcnt == 3)
                {
                    rowcnt = 0;
                    tipBuf += "</tr><tr>";
                }

                /* TODO this generates a small image of the coverArt
                QImage img( path );//, ext.local8Bit() );
                QPixmap pix;
                if( pix.convertFromImage( img.smoothScale( 64, 64 ) ) )
                    return pix;
                */
            }
        }

        if ( rowcnt > 0 )
            tipBuf += "</tr></table>";

        closedir( d );
    }

    tipBuf += "</center>";

    QToolTip::add( widget, tipBuf );
}
