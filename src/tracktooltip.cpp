/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  tracktooltip.cpp  -  Provides an interface to a plain QWidget, which is independent of KDE (bypassed to X11)
  begin:     Tue 10 Feb 2004
  copyright: (C) 2004 by Christian Muehlhaeuser
  email:     chris@chris.de
*/

#include "tracktooltip.h"
#include "metabundle.h"
#include "collectiondb.h"
#include <dirent.h>
#include <sys/stat.h>
#include <kurl.h>
#include <qtooltip.h>


void TrackToolTip::add( QWidget * widget, const MetaBundle & tags )
{
    const QString tr = "<tr><td width=70 align=right>%1:</td><td align=left>%2</td></tr>";

    CollectionDB db;
    QString tipBuf;
    QString image;
    image = db.getImageForPath( tags.url().directory(), "", 80 );

    //NOTE it seems to be necessary to <center> each element indivdually
    tipBuf += "<center><b>amaroK</b></center><table cellpadding='2' cellspacing='2' align='center'><tr>";
    
    if ( !image.isEmpty() )
        tipBuf += QString( "<td><table cellpadding='0' cellspacing='0'><tr><td><img width='80' height='80' src='%1'></td></tr></table></td>" )
                  .arg( image );

    tipBuf += "<td><table cellpadding='0' cellspacing='0'>"; //style='font-face: Arial; font-size: 8px;'
    tipBuf += tr.arg( i18n( "Title" ),  tags.title() );
    tipBuf += tr.arg( i18n( "Artist" ), tags.artist() );
    tipBuf += tr.arg( i18n( "Album" ), tags.album() );
    tipBuf += tr.arg( i18n( "Length" ), tags.prettyLength() );
    tipBuf += tr.arg( i18n( "Bitrate" ), tags.prettyBitrate() );
    tipBuf += tr.arg( i18n( "Samplerate" ), tags.prettySampleRate() );
    tipBuf += "</table></td>";

    tipBuf += "</tr></table></center>";

    QToolTip::add( widget, tipBuf );
}
