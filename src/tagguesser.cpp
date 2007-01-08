/*
 * (c) 2005 Alexandre Oliveira <aleprj@gmail.com>
 * (c) 2003 Frerich Raabe <raabe@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#define DEBUG_PREFIX "TagGuesser"

#include "amarok.h"
#include "debug.h"
#include "tagguesser.h"

#include <kapplication.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kmacroexpander.h>

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
    int artist  = s.find( "%artist" );
    int title   = s.find( "%title" );
    int track   = s.find( "%track" );
    int album   = s.find( "%album" );
    int comment = s.find( "%comment" );
    int year    = s.find( "%year" );
    int composer = s.find( "%composer" );
    int genre   = s.find( "%genre" );

    int fieldNumber = 1;
    int i = s.find( '%' );
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

        i = s.find('%', i + 1);
    }
    m_regExp.setPattern( composeRegExp( s ) );
}

bool FileNameScheme::matches( const QString &fileName ) const
{
    /* Strip extension ('.mp3') because '.' may be part of a title, and thus
     * does not work as a separator.
     */
    QString stripped = fileName;
    stripped.truncate( stripped.findRev( '.' ) );
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
    QMap<QString, QString> substitutions;

    KConfigGroup config(KGlobal::config(), "TagGuesser");

    substitutions[ "title" ] = config.readEntry( "Title regexp", "([\\w\\s'&_,\\.]+)" );
    substitutions[ "artist" ] = config.readEntry( "Artist regexp", "([\\w\\s'&_,\\.]+)" );
    substitutions[ "album" ] = config.readEntry( "Album regexp", "([\\w\\s'&_,\\.]+)" );
    substitutions[ "track" ] = config.readEntry( "Track regexp", "(\\d+)" );
    substitutions[ "comment" ] = config.readEntry( "Comment regexp", "([\\w\\s_]+)" );
    substitutions[ "year" ] = config.readEntry( "Year regexp", "(\\d+)" );
    substitutions[ "composer" ] = config.readEntry( "Composer regexp", "([\\w\\s'&_,\\.]+)" );
    substitutions[ "genre" ] = config.readEntry( "Genre regexp", "([\\w\\s'&_,\\.]+)" );


    QString regExp = QRegExp::escape( s.simplifyWhiteSpace() );
    regExp = ".*" + regExp;
    regExp.replace( ' ', "\\s+" );
    regExp = KMacroExpander::expandMacros( regExp, substitutions );
    regExp += "[^/]*$";
    return regExp;
}

QStringList TagGuesser::schemeStrings()
{
    QStringList schemes;

    schemes = Amarok::config( "TagGuesser" )->readListEntry( "Filename schemes" );

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
    KConfig *cfg = KGlobal::config();
    {
        KConfigGroupSaver saver( cfg, "TagGuesser" );
        cfg->writeEntry( "Filename schemes", schemes );
    }
    cfg->sync();
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

void TagGuesser::loadSchemes()
{
    const QStringList schemes = schemeStrings();
    QStringList::ConstIterator it = schemes.begin();
    QStringList::ConstIterator end = schemes.end();
    for ( ; it != end; ++it )
        m_schemes += FileNameScheme( *it );
}

void TagGuesser::guess( const QString &absFileName )
{
    m_title = m_artist = m_album = m_track = m_comment = m_year = m_composer = m_genre = QString::null;

    FileNameScheme::List::ConstIterator it = m_schemes.begin();
    FileNameScheme::List::ConstIterator end = m_schemes.end();
    for ( ; it != end; ++it ) {
        const FileNameScheme schema( *it );
        if( schema.matches( absFileName ) ) {
            debug() <<"Schema used: " << " " << schema.pattern() <<endl;
            m_title = capitalizeWords( schema.title().replace( '_', " " ) ).stripWhiteSpace();
            m_artist = capitalizeWords( schema.artist().replace( '_', " " ) ).stripWhiteSpace();
            m_album = capitalizeWords( schema.album().replace( '_', " " ) ).stripWhiteSpace();
            m_track = schema.track().stripWhiteSpace();
            m_comment = schema.comment().replace( '_', " " ).stripWhiteSpace();
            m_year = schema.year().stripWhiteSpace();
            m_composer = capitalizeWords( schema.composer().replace( '_', " " ) ).stripWhiteSpace();
            m_genre = capitalizeWords( schema.genre().replace( '_', " " ) ).stripWhiteSpace();
            break;
        }
    }
}

QString TagGuesser::capitalizeWords( const QString &s )
{
    if( s.isEmpty() )
        return s;

    QString result = s;
    result[ 0 ] = result[ 0 ].upper();

    const QRegExp wordRegExp( "\\s\\w" );
    int i = result.find( wordRegExp );
    while ( i > -1 ) {
        result[ i + 1 ] = result[ i + 1 ].upper();
        i = result.find( wordRegExp, ++i );
    }

    return result;
}

