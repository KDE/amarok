/******************************************************************************
 * Copyright (c) 2003 Frerich Raabe <raabe@kde.org>                           *
 *           (c) 2005 Alexandre Pereira de Oliveira <aleprj@gmail.com>        *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License as             *
 * published by the Free Software Foundation; either version 2 of             *
 * the License, or (at your option) any later version.                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
 ******************************************************************************/

#define DEBUG_PREFIX "TagGuesser"

#include "TagGuesser.h"

#include "Amarok.h"
#include "Debug.h"

#include <KApplication>
#include <KConfig>
#include <KConfigGroup>
#include <KGlobal>
#include <KMacroExpanderBase>

FileNameScheme::FileNameScheme( const QString &s )
    : m_cod( s )                    
    , m_titleField( -1 )               
    , m_artistField( -1 )
    , m_albumField( -1 )
    , m_trackField( -1 )
    , m_commentField( -1 )
    , m_yearField( -1 )
    , m_composerField( -1 )
    , m_genreField( -1 )
{
    int artist  = s.indexOf( "%artist" );
    int title   = s.indexOf( "%title" );
    int track   = s.indexOf( "%track" );
    int album   = s.indexOf( "%album" );
    int comment = s.indexOf( "%comment" );
    int year    = s.indexOf( "%year" );
    int composer = s.indexOf( "%composer" );
    int genre   = s.indexOf( "%genre" );

    int fieldNumber = 1;
    int i = s.indexOf( '%' );
    while ( i > -1 ) {
        if ( title == i )
            m_titleField = fieldNumber++;
        if ( artist == i )
            m_artistField = fieldNumber++;
        if ( album == i )
            m_albumField = fieldNumber++;
        if ( track == i )
            m_trackField = fieldNumber++;
        if ( comment == i )
            m_commentField = fieldNumber++;
        if ( year == i )
            m_yearField = fieldNumber++;
        if ( composer == i )
            m_composerField = fieldNumber++;
        if ( genre == i )
            m_genreField = fieldNumber++;

        i = s.indexOf('%', i + 1);
    }
    m_regExp.setPattern( composeRegExp( s ) );
}

bool FileNameScheme::matches( const QString &fileName ) const
{
    /* Strip extension ('.mp3') because '.' may be part of a title, and thus
     * does not work as a separator.
     */
    QString stripped = fileName;
    stripped.truncate( stripped.lastIndexOf( '.' ) );
    return m_regExp.exactMatch( stripped );
}

QString FileNameScheme::title() const
{
    if( m_titleField == -1 )
        return QString();
    return m_regExp.capturedTexts()[ m_titleField ];
}

QString FileNameScheme::artist() const
{
    if( m_artistField == -1 )
        return QString();
    return m_regExp.capturedTexts()[ m_artistField ];
}

QString FileNameScheme::album() const
{
    if( m_albumField == -1 )
        return QString();
    return m_regExp.capturedTexts()[ m_albumField ];
}

QString FileNameScheme::track() const
{
    if( m_trackField == -1 )
        return QString();
    return m_regExp.capturedTexts()[ m_trackField ];
}

QString FileNameScheme::comment() const
{
    if( m_commentField == -1 )
        return QString();
    return m_regExp.capturedTexts()[ m_commentField ];
}

QString FileNameScheme::year() const
{
    if( m_yearField == -1 )
        return QString();
    return m_regExp.capturedTexts()[ m_yearField ];
}

QString FileNameScheme::composer() const
{
    if( m_composerField == -1 )
        return QString();
    return m_regExp.capturedTexts()[ m_composerField ];
}

QString FileNameScheme::genre() const
{
    if( m_genreField == -1 )
        return QString();
    return m_regExp.capturedTexts()[ m_genreField ];
}

QString FileNameScheme::composeRegExp( const QString &s ) const
{
    QHash<QString, QString> substitutions;

    KConfigGroup config(KGlobal::config(), "TagGuesser");

    substitutions[ "title" ] = config.readEntry( "Title regexp", "([\\w\\s'&_,\\.]+)" );
    substitutions[ "artist" ] = config.readEntry( "Artist regexp", "([\\w\\s'&_,\\.]+)" );
    substitutions[ "album" ] = config.readEntry( "Album regexp", "([\\w\\s'&_,\\.]+)" );
    substitutions[ "track" ] = config.readEntry( "Track regexp", "(\\d+)" );
    substitutions[ "comment" ] = config.readEntry( "Comment regexp", "([\\w\\s_]+)" );
    substitutions[ "year" ] = config.readEntry( "Year regexp", "(\\d+)" );
    substitutions[ "composer" ] = config.readEntry( "Composer regexp", "([\\w\\s'&_,\\.]+)" );
    substitutions[ "genre" ] = config.readEntry( "Genre regexp", "([\\w\\s'&_,\\.]+)" );


    QString regExp = QRegExp::escape( s.simplified() );
    regExp = ".*" + regExp;
    regExp.replace( ' ', "\\s+" );
    regExp = KMacroExpander::expandMacros( regExp, substitutions );
    regExp += "[^/]*$";
    return regExp;
}

QStringList TagGuesser::schemeStrings()
{
    QStringList schemes;

    schemes = Amarok::config( "TagGuesser" ).readEntry( "Filename schemes", QStringList() );

    if ( schemes.isEmpty() ) {
        schemes += "%track - %title";
        schemes += "%artist - (%track) - %title [%comment]";
        schemes += "%artist - (%track) - %title (%comment)";
        schemes += "%artist - (%track) - %title";
        schemes += "%artist - [%track] - %title [%comment]";
        schemes += "%artist - [%track] - %title (%comment)";
        schemes += "%artist - [%track] - %title";
        schemes += "%artist - %track - %title [%comment]";
        schemes += "%artist - %track - %title (%comment)";
        schemes += "%artist - %track - %title";
        schemes += "(%track) %artist - %title [%comment]";
        schemes += "(%track) %artist - %title (%comment)";
        schemes += "(%track) %artist - %title";
        schemes += "[%track] %artist - %title [%comment]";
        schemes += "[%track] %artist - %title (%comment)";
        schemes += "[%track] %artist - %title";
        schemes += "%track %artist - %title [%comment]";
        schemes += "%track %artist - %title (%comment)";
        schemes += "%track %artist - %title";
        schemes += "(%artist) %title [%comment]";
        schemes += "(%artist) %title (%comment)";
        schemes += "(%artist) %title";
        schemes += "%artist - %title [%comment]";
        schemes += "%artist - %title (%comment)";
        schemes += "%artist - %title";
        schemes += "%artist/%album/[%track] %title [%comment]";
        schemes += "%artist/%album/[%track] %title (%comment)";
        schemes += "%artist/%album/[%track] %title";
    }
    return schemes;
}

void TagGuesser::setSchemeStrings( const QStringList &schemes )
{
    KConfigGroup cfg = Amarok::config("TagGuesser");
    cfg.writeEntry( "Filename schemes", schemes );
    cfg.sync();
}

TagGuesser::TagGuesser()
{
    loadSchemes(); 
}

TagGuesser::TagGuesser( const QString &absFileName )
{
    loadSchemes();
    guess( absFileName );
}

void TagGuesser::loadSchemes()      //note to self: this method should get its scheme from FilenameLayoutDialog, ideally instantiating it here and storing the result on accept()
{
    const QStringList schemes = schemeStrings();
    QStringList::ConstIterator it = schemes.constBegin();
    QStringList::ConstIterator end = schemes.constEnd();
    for ( ; it != end; ++it )
        m_schemes += FileNameScheme( *it );
}

void TagGuesser::guess( const QString &absFileName )
{
    m_title = m_artist = m_album = m_track = m_comment = m_year = m_composer = m_genre = QString();

    FileNameScheme::List::ConstIterator it = m_schemes.constBegin();
    FileNameScheme::List::ConstIterator end = m_schemes.constEnd();
    for ( ; it != end; ++it ) {
        const FileNameScheme schema( *it );
        if( schema.matches( absFileName ) ) {
            debug() <<"Schema used: " << " " << schema.pattern();
            m_title = capitalizeWords( schema.title().replace( '_', " " ) ).trimmed();
            m_artist = capitalizeWords( schema.artist().replace( '_', " " ) ).trimmed();
            m_album = capitalizeWords( schema.album().replace( '_', " " ) ).trimmed();
            m_track = schema.track().trimmed();
            m_comment = schema.comment().replace( '_', " " ).trimmed();
            m_year = schema.year().trimmed();
            m_composer = capitalizeWords( schema.composer().replace( '_', " " ) ).trimmed();
            m_genre = capitalizeWords( schema.genre().replace( '_', " " ) ).trimmed();
            break;
        }
    }
}

QString TagGuesser::capitalizeWords( const QString &s )
{
    if( s.isEmpty() )
        return s;

    QString result = s;
    result[ 0 ] = result[ 0 ].toUpper();

    const QRegExp wordRegExp( "\\s\\w" );
    int i = result.indexOf( wordRegExp );
    while ( i > -1 ) {
        result[ i + 1 ] = result[ i + 1 ].toUpper();
        i = result.indexOf( wordRegExp, ++i );
    }

    return result;
}

