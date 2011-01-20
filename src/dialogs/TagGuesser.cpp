/****************************************************************************************
 * Copyright (c) 2009 Daniel Dewald <Daniel.Dewald@time-shift.de>                       *
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

#define DEBUG_PREFIX "TagGuesser"

#include "TagGuesser.h"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "shared/TagsFromFileNameGuesser.h"

TagGuesser::TagGuesser()
          : m_guessed( false )
          , m_caseOptions( 0 )
          , m_cutTrailingSpaces( false )
          , m_convertUnderscores( false )
{
}

// sets filename to guess from
void
TagGuesser::setFilename( const QString &fileName )
{
    m_fileName = fileName;
}

// sets schema to guess with
void
TagGuesser::setSchema( const QString &schema )
{
    m_schema = QString( schema ).replace( ".", "\\." );
}

// sets case type to convert tags into
void
TagGuesser::setCaseType( const int caseOptions )
{
    m_caseOptions = caseOptions;
}

// sets whether trailing spcaes should be removes
void
TagGuesser::setCutTrailingSpaces( const bool cutTrailingSpaces )
{
    m_cutTrailingSpaces = cutTrailingSpaces;
}

// sets whether underscore should be converted
void
TagGuesser::setConvertUnderscores( const bool convertUnderscores )
{
    m_convertUnderscores = convertUnderscores;
}

// guesses tags depending on the schema
bool
TagGuesser::guess()
{
    m_guessed = false;
    if( !m_fileName.isEmpty() && !m_schema.isEmpty() )
    {
        Meta::FieldHash tags = Meta::Tag::TagGuesser::guessTagsByScheme( m_fileName, m_schema,
                                                                         m_cutTrailingSpaces,
                                                                         m_convertUnderscores );
        foreach( qint64 key, tags.keys() )
        {
            if( !key )
                continue;

            m_tags.insert( key, convertTagCaseType( tags[key].toString(), m_caseOptions ) );
        }

        m_guessed = !m_tags.isEmpty();
    }

    return m_guessed;
}

// Converts a tag to its case type version
QString
TagGuesser::convertTagCaseType( const QString &tag, int type )
{
    if( tag.isEmpty() )
        return tag;

    switch( type )
    {
        case 0:
            return tag;
        case 1:
            return tag.toLower().replace( 0, 1, tag.left( 1 ).toUpper() );
        case 2:
        {
            QString complete;
            QStringList splitted = tag.toLower().split( " ", QString::SkipEmptyParts );

            foreach( QString word, splitted )
            {
                if( word.length() > 1 )
                    word.replace( 0, 1, word.left( 1 ).toUpper() );
                complete += word + " ";
            }
            complete.truncate( complete.length() - 1 );

            return complete;
        }
        case 3:
            return tag.toUpper();
        case 4:
            return tag.toLower();
        default:
            return tag;
    }
}

// creates a colored version of the filename
QString
TagGuesser::coloredFileName()
{
    if( m_guessed && !m_tags.isEmpty() )
    {
        QString coloredFileName = m_fileName;
        Meta::FieldHash tags = Meta::Tag::TagGuesser::guessTagsByScheme( m_fileName, m_schema,
                                                                         false, false );
        foreach( qint64 key, tags.keys() )
        {
            QString value = tags[key].toString();
            coloredFileName.replace( value, "<font color=\"" + fieldColor( key ) +
                                            "\">" + value + "</font>", Qt::CaseInsensitive );
        }
        return coloredFileName;
    }

    return m_fileName;
}

QString
TagGuesser::fieldColor( qint64 field )
{
    Qt::GlobalColor color;
    switch ( field )
    {
        case Meta::valAlbum:
            color = album_color;
            break;

        case Meta::valAlbumArtist:
            color = albumartist_color;
            break;

        case Meta::valArtist:
            color = artist_color;
            break;

        case Meta::valComment:
            color = comment_color;
            break;

        case Meta::valComposer:
            color = composer_color;
            break;

        case Meta::valDiscNr:
            color = discnr_color;
            break;

        case Meta::valGenre:
            color = genre_color;
            break;

        case Meta::valTitle:
            color = title_color;
            break;

        case Meta::valTrackNr:
            color = track_color;
            break;

        case Meta::valYear:
            color = year_color;
            break;

        default:
            color = Qt::black;
    }

    return QColor( color ).name();
}
