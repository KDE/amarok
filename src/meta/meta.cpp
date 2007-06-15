/* This file is part of the KDE project
   Copyright (C) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#include "meta.h"

#include "amarok.h"
#include "amarokconfig.h"
#include "collection.h"
#include "querymaker.h"

#include <QDir>
#include <QImage>

#include <KStandardDirs>

bool
Meta::Track::inCollection() const
{
    return false;
}

Collection*
Meta::Track::collection() const
{
    return 0;
}

QString
Meta::Track::cachedLyrics() const
{
    return QString();
}

void
Meta::Track::setCachedLyrics( const QString &lyrics )
{
    Q_UNUSED( lyrics )
}

void
Meta::Track::addMatchTo( QueryMaker *qm )
{
    qm->addMatch( TrackPtr( this ) );
}

void
Meta::Track::finishedPlaying( double playedFraction )
{
    Q_UNUSED( playedFraction )
}

void
Meta::Artist::addMatchTo( QueryMaker *qm )
{
    qm->addMatch( ArtistPtr( this ) );
}

void
Meta::Album::addMatchTo( QueryMaker *qm )
{
    qm->addMatch( AlbumPtr( this ) );
}

QPixmap
Meta::Album::image( int size, bool withShadow ) const
{
    QDir cacheCoverDir = QDir( Amarok::saveLocation( "albumcovers/cache/" ) );
    if ( size <= 1 )
        size = AmarokConfig::coverPreviewSize();
    QString sizeKey = QString::number( size ) + '@';

    QImage img;
    if( cacheCoverDir.exists( sizeKey + "nocover.png" ) )
         img = QImage( cacheCoverDir.filePath( sizeKey + "nocover.png" ) );
    else
    {
        img = QImage( KStandardDirs::locate( "data", "amarok/images/nocover.png" ) ); //optimise this!
        img.scaled( size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation );
        img.save( cacheCoverDir.filePath( sizeKey + "nocover.png" ), "PNG" );
    }

    //if ( withShadow )
        //s = makeShadowedImage( s );

    return QPixmap::fromImage( img );
}

void
Meta::Genre::addMatchTo( QueryMaker *qm )
{
    qm->addMatch( GenrePtr( this ) );
}

void
Meta::Composer::addMatchTo( QueryMaker *qm )
{
    qm->addMatch( ComposerPtr( this ) );
}

void
Meta::Year::addMatchTo( QueryMaker *qm )
{
    qm->addMatch( YearPtr( this ) );
}
