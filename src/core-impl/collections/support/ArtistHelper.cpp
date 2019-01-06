/****************************************************************************************
 * Copyright (c) 2010 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "ArtistHelper.h"

#include <QStringList>

#include <KLocalizedString>

QString
ArtistHelper::bestGuessAlbumArtist( const QString &albumArtist, const QString &trackArtist,
                                    const QString &genre, const QString &composer)
{
    QString best( albumArtist );

    // - for classical tracks it's the composer
    if( best.isEmpty() &&
        (genre.compare( i18nc( "The genre name for classical music", "Classical" ), Qt::CaseInsensitive ) == 0 ||
         genre.compare( QLatin1String( "Classical" ), Qt::CaseInsensitive ) == 0 ) )
        best = ArtistHelper::realTrackArtist( composer );

    // - for "normal" tracks it's the track artist
    if( best.isEmpty() )
        best = ArtistHelper::realTrackArtist( trackArtist );

    // - "Various Artists" is the same as no artist
    if( best.compare( i18n( "Various Artists" ), Qt::CaseInsensitive ) == 0 ||
        best.compare( QLatin1String( "Various Artists" ), Qt::CaseInsensitive ) == 0 )
        best.clear();

    return best;
}

QString
ArtistHelper::realTrackArtist( const QString &trackArtistTag )
{
    bool featuring = false;
    QStringList trackArtists;
    if( trackArtistTag.contains( QLatin1String("featuring") ) )
    {
        featuring = true;
        trackArtists = trackArtistTag.split( QStringLiteral("featuring") );
    }
    else if( trackArtistTag.contains( QLatin1String("feat.") ) )
    {
        featuring = true;
        trackArtists = trackArtistTag.split( QStringLiteral("feat.") );
    }
    else if( trackArtistTag.contains( QLatin1String("ft.") ) )
    {
        featuring = true;
        trackArtists = trackArtistTag.split( QStringLiteral("ft.") );
    }
    else if( trackArtistTag.contains( QLatin1String("f.") ) )
    {
        featuring = true;
        trackArtists = trackArtistTag.split( QStringLiteral("f.") );
    }

    //this needs to be improved

    if( featuring )
    {
        //always use the first artist
        QString tmp = trackArtists[0].simplified();
        //artists are written as "A (feat. B)" or "A [feat. B]" as well
        if( tmp.endsWith(QLatin1String(" (")) || tmp.endsWith( QLatin1String(" [") ) )
            tmp = tmp.left( tmp.length() -2 ).simplified(); //remove last two characters

        if( tmp.isEmpty() )
            return trackArtistTag; //huh?
        else
        {
            return tmp;
        }
    }
    else
    {
        return trackArtistTag;
    }
}
