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
#include <qapplication.h>
#include <qpainter.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>


PlaylistToolTip::PlaylistToolTip( QWidget * parent )
    : QToolTip( parent )
{
}

void PlaylistToolTip::add( QWidget * widget, const KURL url, const MetaBundle & tags )
{
    QString tipBuf =  "<table style='font-face: Arial; font-size: 8px;'><tr><td>Title: </td><td colspan='2' align='right'>" + tags.m_title + "</td></tr>" +
                      "<tr><td>Artist: </td><td colspan='2' align='right'>" + tags.m_artist + "</td></tr>" +
                      "<tr><td>Length: </td><td colspan='2' align='right'>" + QString::number( tags.m_length ) + "</td></tr>" +
                      "<tr><td>Bitrate: </td><td colspan='2' align='right'>" + QString::number( tags.m_bitrate ) + "</td></tr>" +
                      "<tr><td>Samplerate: </td><td colspan='2' align='right'>" + QString::number( tags.m_sampleRate ) + "<br></td></tr><tr>";

    int rowcnt = 0;
    QString curAlign;
    DIR *d = opendir( url.directory( FALSE, FALSE ).local8Bit() );
    if ( d )
    {
        dirent *ent;
        while ( ( ent = readdir( d ) ) )
        {
            QString file( ent->d_name );

            if ( file == "." || file == ".." ) continue;
            if ( file.contains( ".jpg", FALSE ) || file.contains( ".png", FALSE ) ||
                 file.contains( ".gif", FALSE ) )
            {
                switch ( rowcnt )
                {
                    case 2: curAlign = "right"; break;
                    case 1: curAlign = "center"; break;
                    case 0: curAlign = "left"; break;
                }
                tipBuf += "<td width='104' align='" + curAlign + "'><img width='100' src='" + url.directory( FALSE, FALSE ) + "/" + file + "'></td>";

                rowcnt++;
                if ( rowcnt == 3)
                {
                    rowcnt = 0;
                    tipBuf += "</tr><tr>";
                }
            }
            
        }
    }

    tipBuf += "</tr></table>";
    QToolTip::add( widget, tipBuf );    
}
