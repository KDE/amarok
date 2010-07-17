/***************************************************************************
 *   Copyright (c) 2010 Casey Link <unnamedrambler@gmail.com>              *
 *   Copyright (c) 2008 Bonne Eggleston <b.eggleston@gmail.com>            *
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
 ***************************************************************************/

#include "TrackOrganizer.h"

#include "core/support/Amarok.h"
#include "QStringx.h"

#include <KLocale>

TrackOrganizer::TrackOrganizer( const Meta::TrackList &tracks, QObject* parent )
    : QObject( parent )
    , m_allTracks( tracks )
    , m_IgnoreThe( false )
    , m_AsciiOnly( false )
    , m_UnderscoresNotSpaces( false )
    , m_vfatSafe( false )
{

}

QString TrackOrganizer::buildDestination(const QString& format, const Meta::TrackPtr& track) const
{
    bool isCompilation = track->album() && track->album()->isCompilation();

    QMap<QString, QString> args;
    QString artist = track->artist() ? track->artist()->name() : QString();
    QString albumartist;
    if( isCompilation )
        albumartist = i18n( "Various Artists" );
    else
    {
        if( track->album() && track->album()->hasAlbumArtist() )
            albumartist = track->album()->albumArtist()->name();
        else
            albumartist = artist;
    }
    args["theartist"] = cleanPath( artist );
    args["thealbumartist"] = cleanPath( albumartist );

    if( m_IgnoreThe && artist.startsWith( "The ", Qt::CaseInsensitive ) )
        Amarok::manipulateThe( artist, true );

    artist = cleanPath( artist );

    if( m_IgnoreThe && albumartist.startsWith( "The ", Qt::CaseInsensitive ) )
        Amarok::manipulateThe( albumartist, true );

    albumartist = cleanPath( albumartist );

    //these additional columns from MetaBundle were used before but haven't
    //been ported yet. Do they need to be?
    //Bpm,Directory,Bitrate,SampleRate,Mood
    args["folder"] = m_folderPrefix;
    args["title"] = cleanPath( track->prettyName() );
    args["composer"] = track->composer() ? cleanPath( track->composer()->prettyName() ) : QString();

    // if year == 0 then we don't want include it
    QString year = track->year() ? cleanPath( track->year()->prettyName() ) : QString();
    args["year"] = year.localeAwareCompare( "0" ) == 0 ? QString() : year;
    args["album"] = track->album() ? cleanPath( track->album()->prettyName() ) : QString();

    if( track->discNumber() )
        args["discnumber"] = QString::number( track->discNumber() );

    args["genre"] = track->genre() ? cleanPath( track->genre()->prettyName() ) : QString();
    args["comment"] = cleanPath( track->comment() );
    args["artist"] = artist;
    args["albumartist"] = albumartist;
    args["initial"] = albumartist.mid( 0, 1 ).toUpper();    //artists starting with The are already handled above
    if( m_targetFileExtension == QString() )
        args["filetype"] = track->type();
    else
        args["filetype"] = m_targetFileExtension;
    args["rating"] = track->rating();
    args["filesize"] = track->filesize();
    args["length"] = track->length() / 1000;

    if ( track->trackNumber() )
    {
        QString trackNum = QString("%1").arg( track->trackNumber(), 2, 10, QChar('0') );
        args["track"] = trackNum;
    }

    Amarok::QStringx formatx( format );
    QString result = formatx.namedOptArgs( args );
    if( !result.startsWith( '/' ) )
        result.prepend( "/" );

   return result.replace( QRegExp( "/\\.*" ), "/" );
}

QString TrackOrganizer::cleanPath( const QString& component ) const
{
    QString result = component;

    if( m_AsciiOnly )
    {
        result = Amarok::cleanPath( result );
        result = Amarok::asciiPath( result );
    }

    if( !m_regexPattern.isEmpty() )
        result.replace( QRegExp( m_regexPattern ), m_replaceString );

    result.simplified();
    if( m_UnderscoresNotSpaces )
        result.replace( QRegExp( "\\s" ), "_" );
//     debug()<<"I'm about to do Amarok::vfatPath( result ), this is result: "<<result;
    if( m_vfatSafe )
        result = Amarok::vfatPath( result );

    result.replace( '/', '-' );

    return result;
}

QMap< Meta::TrackPtr, QString > TrackOrganizer::getDestinations()
{
    QMap<Meta::TrackPtr, QString> destinations;
    foreach( const Meta::TrackPtr &track, m_allTracks )
    {
        if( track )
            destinations.insert( track, buildDestination( m_format, track ) );
    }
    return destinations;
}

void TrackOrganizer::setFormatString( const QString& format )
{
    m_format = format;
}

void TrackOrganizer::setFolderPrefix(const QString& prefix)
{
    m_folderPrefix = prefix;
}

void TrackOrganizer::setAsciiOnly(bool flag)
{
    m_AsciiOnly = flag;
}

void TrackOrganizer::setIgnoreThe(bool flag)
{
    m_IgnoreThe = flag;
}

void TrackOrganizer::setReplaceSpaces(bool flag)
{
    m_UnderscoresNotSpaces = flag;
}

void TrackOrganizer::setVfatSafe(bool flag)
{
    m_vfatSafe = flag;
}

void TrackOrganizer::setReplace(const QString& regex, const QString& string)
{
    m_regexPattern = regex;
    m_replaceString = string;
}

void TrackOrganizer::setTargetFileExtension( const QString &fileExtension )
{
    m_targetFileExtension = fileExtension;
}
