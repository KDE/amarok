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
#include "metabundle.h" //moved from header to stop a compile dependency
#include <qapplication.h>
#include <qpainter.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <kurl.h>


PlaylistToolTip::PlaylistToolTip( QWidget * parent )
    : QToolTip( parent )
{
}

void PlaylistToolTip::add( QWidget * widget, const MetaBundle & tags )
{
    const KURL &url = tags.m_url;

    QString tipBuf = "<center><table style='font-face: Arial; font-size: 8px;'>";
    QString s = "<tr><td width='70'>%1:</td><td align='left'>%2</td></tr>";
    tipBuf += s.arg( i18n( "Title" ),  tags.m_title );
    tipBuf += s.arg( i18n( "Artist" ), tags.m_artist );
    tipBuf += s.arg( i18n( "Length" ), tags.prettyLength() );
    tipBuf += s.arg( i18n( "Bitrate" ), tags.prettyBitrate() );
    tipBuf += s.arg( i18n( "Samplerate" ), tags.prettySampleRate() ) + "<br>";
    tipBuf += "</table>";

    int rowcnt = 0;
    QString curAlign;
    DIR *d = opendir( url.directory( FALSE, FALSE ).local8Bit() );
    if ( d )
    {
        dirent *ent;
        while ( ( ent = readdir( d ) ) )
        {
            QString file( ent->d_name );

            if ( file.contains( ".jpg", FALSE ) || file.contains( ".png", FALSE ) ||
                 file.contains( ".gif", FALSE ) )
            {
                // we found an image, let's add it to the tooltip
                if ( rowcnt == 0 )
                    tipBuf += "<table><tr>";

                tipBuf += "<td width='104' align='center'><img width='100' src='" + url.directory( FALSE, FALSE ) + "/" + file + "'></td>";

                rowcnt++;
                if ( rowcnt == 3)
                {
                    rowcnt = 0;
                    tipBuf += "</tr><tr>";
                }
            }

        }
    }

    if ( rowcnt > 0 )
        tipBuf += "</tr></table>";
    tipBuf += "</center>";

    QToolTip::add( widget, tipBuf );
}
