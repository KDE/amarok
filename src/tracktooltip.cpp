/*
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
*/

/*
  tracktooltip.cpp
  begin:     Tue 10 Feb 2004
  copyright: (C) 2004 by Christian Muehlhaeuser
  email:     chris@chris.de
*/

#include "tracktooltip.h"
#include "metabundle.h"
#include "collectiondb.h"
#include <qtooltip.h>
#include <qapplication.h>


void TrackToolTip::add( QWidget * widget, const MetaBundle & tags )
{
    QString tipBuf;
    QStringList left, right;
    const QString tableRow = "<tr><td width=70 align=right>%1:</td><td align=left>%2</td></tr>";

    QString image = CollectionDB::instance()->albumImage( tags.artist(), tags.album() );

    left  << i18n( "Title" ) << i18n( "Artist" ) << i18n( "Album" );
    right << tags.title() << tags.artist() << tags.album();
    
    if ( tags.length() )
    {
        left << i18n( "Length" );
        right << tags.prettyLength();
    }
    if ( tags.bitrate() )
    {
        left << i18n( "Bitrate" );
        right << tags.prettyBitrate();
    }
    if ( tags.sampleRate() )
    {
        left << i18n( "Samplerate" );
        right << tags.prettySampleRate();
    }
    
        

    //NOTE it seems to be necessary to <center> each element indivdually
    tipBuf += "<center><b>amaroK</b></center><table cellpadding='2' cellspacing='2' align='center'><tr>";

    if ( !image.isEmpty() && image.find( QString("nocover") ) == -1 )
    {
        int imageSize = QApplication::desktop()->width() / 16;
        tipBuf +=( QString( "<td><table cellpadding='0' cellspacing='0'><tr><td>"
                           "<img width='%1' height='%2' src='%3'>"
                           "</td></tr></table></td>" )
                           .arg( QString::number(imageSize) )
                           .arg( QString::number(imageSize) )
                           .arg( image )
                           );
                           
    }

    tipBuf += "<td><table cellpadding='0' cellspacing='0'>";

    for( uint x = 0; x < left.count(); ++x )
        if ( !right[x].isEmpty() )
            tipBuf += tableRow.arg( left[x] ).arg( right[x] );

    tipBuf += "</table></td>";
    tipBuf += "</tr></table></center>";

    QToolTip::add( widget, tipBuf );
}
