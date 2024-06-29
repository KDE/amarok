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

#include "QStringx.h"
#include "core/meta/Meta.h"
#include "core/meta/Statistics.h"
#include "core/support/Amarok.h"

#include <QRegularExpression>
#include <KLocalizedString>

TrackOrganizer::TrackOrganizer( const Meta::TrackList &tracks, QObject* parent )
    : QObject( parent )
    , m_allTracks( tracks )
    , m_trackOffset( 0 )
    , m_postfixThe( false )
    , m_AsciiOnly( false )
    , m_UnderscoresNotSpaces( false )
    , m_vfatSafe( false )
{

}

QString TrackOrganizer::buildDestination(const QString& format, const Meta::TrackPtr& track) const
{
    // get hold of the shared pointers
    Meta::AlbumPtr album = track->album();
    Meta::ArtistPtr artist = track->artist();
    Meta::ComposerPtr composer = track->composer();
    Meta::ArtistPtr albumArtist = album ? album->albumArtist() : Meta::ArtistPtr();
    Meta::GenrePtr genre = track->genre();
    Meta::YearPtr year = track->year();

    bool isCompilation = album && album->isCompilation();

    QMap<QString, QString> args;
    QString strArtist = artist ? artist->name() : QString();
    QString strAlbumArtist = isCompilation ? i18n( "Various Artists" ) :
        ( albumArtist ? albumArtist->name() : strArtist );

    args[QStringLiteral("theartist")] = strArtist;
    args[QStringLiteral("thealbumartist")] = strAlbumArtist;

    if( m_postfixThe )
    {
        Amarok::manipulateThe( strArtist, true );
        Amarok::manipulateThe( strAlbumArtist, true );
    }

    if ( track->trackNumber() )
    {
        QString trackNum = QStringLiteral("%1").arg( track->trackNumber(), 2, 10, QLatin1Char('0') );
        args[QStringLiteral("track")] = trackNum;
    }
    args[QStringLiteral("title")] = track->name();
    args[QStringLiteral("artist")] = strArtist;
    args[QStringLiteral("composer")] = composer ? composer->name() : QString();
    // if year == 0 then we don't want include it
    QString strYear = year ? year->name() : QString();
    args[QStringLiteral("year")] = strYear.localeAwareCompare( QStringLiteral("0") ) == 0 ? QString() : strYear;
    args[QStringLiteral("album")] = track->album() ? track->album()->name() : QString();
    args[QStringLiteral("albumartist")] = strAlbumArtist;
    args[QStringLiteral("comment")] = track->comment();
    args[QStringLiteral("genre")] = genre ? genre->name() : QString();
    if( m_targetFileExtension.isEmpty() )
        args[QStringLiteral("filetype")] = track->type();
    else
        args[QStringLiteral("filetype")] = m_targetFileExtension;
    QString strFolder = QFileInfo( track->playableUrl().toLocalFile() ).path();
    strFolder = strFolder.mid( commonPrefixLength( m_folderPrefix, strFolder ) );
    args[QStringLiteral("folder")] = strFolder;
    args[QStringLiteral("initial")] = strAlbumArtist.mid( 0, 1 ).toUpper(); //artists starting with The are already handled above
    if( track->discNumber() )
        args[QStringLiteral("discnumber")] = QString::number( track->discNumber() );
    args[QStringLiteral("collectionroot")] = m_folderPrefix;

    // some additional properties not supported by organize dialog.
    args[QStringLiteral("rating")] = QString::number( track->statistics()->rating() );
    args[QStringLiteral("filesize")] = QString::number( track->filesize() );
    args[QStringLiteral("length")] = QString::number( track->length() / 1000 );

    // Fill up default empty values for StringX formatter
    // TODO make this values changeable by user
    args[QStringLiteral("default_album")]           = i18n( "Unknown album" );
    args[QStringLiteral("default_albumartist")]     = i18n( "Unknown artist" );
    args[QStringLiteral("default_artist")]          = args[QStringLiteral("albumartist")];
    args[QStringLiteral("default_thealbumartist")]  = args[QStringLiteral("albumartist")];
    args[QStringLiteral("default_theartist")]       = args[QStringLiteral("albumartist")];
    args[QStringLiteral("default_comment")]         = i18n( "No comments" );
    args[QStringLiteral("default_composer")]        = i18n( "Unknown composer" );
    args[QStringLiteral("default_discnumber")]      = i18n( "Unknown disc number" );
    args[QStringLiteral("default_genre")]           = i18n( "Unknown genre" );
    args[QStringLiteral("default_title")]           = i18n( "Unknown title" );
    args[QStringLiteral("default_year")]            = i18n( "Unknown year" );

    for( const QString &key : args.keys() )
        if( key != QStringLiteral("collectionroot") && key != QStringLiteral("folder") )
            args[key] = args[key].replace( QLatin1Char('/'), QLatin1Char('-') );

    Amarok::QStringx formatx( format );
    QString result = formatx.namedOptArgs( args );
    return cleanPath( result );
}

QString TrackOrganizer::cleanPath( const QString& path ) const
{
    QString result = path;

    if( m_AsciiOnly )
    {
        result = Amarok::cleanPath( result );
        result = Amarok::asciiPath( result );
    }

    if( !m_regexPattern.isEmpty() )
        result.replace( QRegularExpression( m_regexPattern ), m_replaceString );

    result = result.simplified();
    if( m_UnderscoresNotSpaces )
        result.replace( QRegularExpression( QLatin1String("\\s") ), QLatin1String("_") );

    if( m_vfatSafe )
        // we use UnixBehaviour even on windows, because even there we use / as directory
        // separator currently (QFile mangles it internally)
        result = Amarok::vfatPath( result, Amarok::UnixBehaviour );

    QFileInfo info( result ); // Used to polish path string. (e.g. remove '//')
    return info.absoluteFilePath();
}

int TrackOrganizer::commonPrefixLength( const QString &a, const QString &b )
{
    int prefix = 0;
    while( prefix < a.length() && prefix < b.length() &&
           a.at(prefix) == b.at(prefix) )
        prefix++;
    return prefix;
}

QMap< Meta::TrackPtr, QString > TrackOrganizer::getDestinations( unsigned int batchSize )
{
    QMap<Meta::TrackPtr, QString> destinations;

    int newOffset = m_trackOffset + batchSize;
    //don't go out of bounds in the for loop
    if( newOffset >= m_allTracks.count() )
        newOffset = m_allTracks.count();

    if( batchSize == 0 )
    {
        m_trackOffset = 0;
        newOffset = m_allTracks.count();
    }

    for( ; m_trackOffset < newOffset ; m_trackOffset++ )
    {
        Meta::TrackPtr track = m_allTracks.value( m_trackOffset );
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

void TrackOrganizer::setPostfixThe(bool flag)
{
    m_postfixThe = flag;
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
