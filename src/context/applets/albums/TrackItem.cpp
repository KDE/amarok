/*******************************************************************************
* copyright              : (C) 2008 Seb Ruiz <ruiz@kde.org>                    *
*                                                                              *
********************************************************************************/

/*******************************************************************************
*                                                                              *
*   This program is free software; you can redistribute it and/or modify       *
*   it under the terms of the GNU General Public License as published by       *
*   the Free Software Foundation; either version 2 of the License, or          *
*   (at your option) any later version.                                        *
*                                                                              *
********************************************************************************/

#include "TrackItem.h"

#include <QFont>

TrackItem::TrackItem()
    : QStandardItem()
{
    setEditable( false );
}

void
TrackItem::setTrack( Meta::TrackPtr trackPtr )
{
    if( m_track )
        unsubscribeFrom( m_track );
    m_track = trackPtr;
    subscribeTo( m_track );

    metadataChanged( m_track );
}

void
TrackItem::metadataChanged( Meta::TrackPtr track )
{
    int trackNumber = track->trackNumber();
    QString trackName = track->prettyName();

    QString text;

    if( trackNumber > 0 )
        text = QString( "%1\t%2" ).arg( QString::number( trackNumber ), trackName );
    else
        text = QString( "\t%1" ).arg( trackName );

    setText( text );
}

void
TrackItem::italicise()
{
    QFont f = font();
    f.setItalic( true );
    setFont( f );
}

