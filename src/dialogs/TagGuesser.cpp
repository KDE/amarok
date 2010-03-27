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

#include "Amarok.h"
#include "core/support/Debug.h"

TagGuesser::TagGuesser()
{
    m_sortedTags.clear();
    m_tags.clear();
    m_guessed = false;
    m_fileName = "";
    m_schema = "";
    m_caseOptions = 0;
    m_convertUnderscores = false;
    m_cutTrailingSpaces = false;
}

// sets filename to guess from
void
TagGuesser::setFilename( const QString fileName )
{
    m_fileName = fileName;
}

// sets schema to guess with
void
TagGuesser::setSchema( const QString schema )
{
    m_schema = schema;
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
    if( ( !m_fileName.isEmpty() ) && ( !m_schema.isEmpty() ) )
    {
        QString regExpr = getRegExpFromSchema( m_schema );
        QRegExp fileExpr(regExpr + "\\.(.*)");
        QRegExp schemaExpr(regExpr);
        
        int pos1 = schemaExpr.indexIn( m_schema );
        int pos2 = fileExpr.indexIn( m_fileName );
        
        if( ( pos1 > -1 ) && ( pos2 > -1 ) )
        {
            for( int x = 1; x<=fileExpr.numCaptures()-1; x++)
            {
                if( x <= schemaExpr.numCaptures() )
                {
                    QString tag = fileExpr.cap(x);
                    QString type = schemaExpr.cap(x).replace("%","");
                    QString orgTag = tag;

                    if( m_convertUnderscores )
                        tag = convertUnderscores( tag );
                    
                    if( m_cutTrailingSpaces )
                        tag = tag.trimmed();

                    if( m_caseOptions > 0 )
                        tag = convertTagCaseType( tag, m_caseOptions );

                    if( type != "ignore" )
                    {
                        TagStruct otag;
                        otag.type = type;
                        otag.tag = orgTag;
                        debug() << orgTag;
                        m_sortedTags[x-1] = otag;
                        m_tags[type] = tag;
                    }
                    
                }
            }
        }

        m_guessed = true;
    }
    else
    {
        m_guessed = false;
    }

    return m_guessed;
}

// Converts a tag to its case type version
QString
TagGuesser::convertTagCaseType( QString tag, int type )
{
    if( tag.isEmpty() )
        return tag;
    
    switch(type)
    {
        case(0):
        {
            return tag;
        }
        case(1):
        {
            tag = tag.toLower();
            return tag.replace( 0, 1, tag.left( 1 ).toUpper() );
        }
        case(2):
        {
            tag = tag.toLower();
            QStringList splitted = tag.split(" ");
            QString complete;

            foreach( QString word, splitted )
            {
                if(word.count()>1)
                    word.replace( 0, 1, word.left( 1 ).toUpper() );
                complete += word + " ";
            }

            complete.remove(complete.length()-1);
            
            return complete;
        }
        case(3):
        {
            return tag.toUpper();
        }
        case(4):
        {
            return tag.toLower();
        }
        default:
        {
            return tag;
        }
        
    }
}

// converts all underscores in a tag to spaces
QString
TagGuesser::convertUnderscores( QString tag )
{
    return tag.replace( "_", " " );
}

// removes all trailing spaces in a tag
QString
TagGuesser::cutTagTrailingSpaces( QString tag )
{
    return tag.trimmed();
}

// creates a regular expression from a schema
QString
TagGuesser::getRegExpFromSchema( QString schema )
{
    return schema.replace( QRegExp("(%track|%title|%artist|%composer|%year|%album|%comment|%genre|%ignore)"), "(.*)" );
}

// creates a colored version of the filename
QString
TagGuesser::coloredFileName()
{
    if( m_guessed )
    {
        QMap<int,TagStruct>::const_iterator tag = m_sortedTags.constBegin();
        QMap<int,TagStruct>::const_iterator end = m_sortedTags.constEnd();
        int Pos = 0;
        QString colored = m_fileName;

        for( ; tag != end; ++tag )
        {
            Pos = colored.indexOf( tag.value().tag, Pos, Qt::CaseInsensitive );

            if( ( m_tags.contains( "album" ) ) && ( tag.value().type == "album" ) )
            {
                colored.insert( Pos,QString( "<font color=\"" + QColor( album_color ).name() + "\">" ) );
                Pos += tag.value().tag.length()+22;
                colored.insert( Pos,QString( "</font>" ) );
                Pos += 7;
            }
            else if( ( m_tags.contains( "artist" ) ) && ( tag.value().type == "artist" ) )
            {
                colored.insert( Pos,QString( "<font color=\"" + QColor( artist_color ).name() + "\">" ) );
                Pos += tag.value().tag.length()+22;
                colored.insert( Pos,QString( "</font>" ) );
                Pos += 7;
            }
            else if( ( m_tags.contains( "comment" ) ) && ( tag.value().type == "comment" ) )
            {
                colored.insert( Pos,QString( "<font color=\"" + QColor( comment_color ).name() + "\">" ) );
                Pos += tag.value().tag.length()+22;
                colored.insert( Pos,QString( "</font>" ) );
                Pos += 7;
            }
            else if( ( m_tags.contains( "composer" ) ) && ( tag.value().type == "composer" ) )
            {
                colored.insert( Pos,QString( "<font color=\"" + QColor( composer_color ).name() + "\">" ) );
                Pos += tag.value().tag.length()+22;
                colored.insert( Pos,QString( "</font>" ) );
                Pos += 7;
            }
            else if( ( m_tags.contains( "genre" ) ) && ( tag.value().type == "genre" ) )
            {
                colored.insert( Pos,QString( "<font color=\"" + QColor( genre_color ).name() + "\">" ) );
                Pos += tag.value().tag.length()+22;
                colored.insert( Pos,QString( "</font>" ) );
                Pos += 7;
            }
            else if( ( m_tags.contains( "title" ) ) && ( tag.value().type == "title" ) )
            {
                colored.insert( Pos,QString( "<font color=\"" + QColor( title_color ).name() + "\">" ) );
                Pos += tag.value().tag.length()+22;
                colored.insert( Pos,QString( "</font>" ) );
                Pos += 7;
            }
            else if( ( m_tags.contains( "track" ) ) && ( tag.value().type == "track" ) )
            {
                colored.insert( Pos,QString( "<font color=\"" + QColor( track_color ).name() + "\">" ) );
                Pos += tag.value().tag.length()+22;
                colored.insert( Pos,QString( "</font>" ) );
                Pos += 7;
            }
            else if( ( m_tags.contains( "year" ) ) && ( tag.value().type == "year" ) )
            {
                colored.insert( Pos,QString( "<font color=\"" + QColor( year_color ).name() + "\">" ) );
                Pos += tag.value().tag.length()+22;
                colored.insert( Pos,QString( "</font>" ) );
                Pos += 7;
            }
            else
            {
                debug() << tag.value().type << " is an unknown tag type.";
            }
        }
        
        return colored;
    }
    else
    {
        return m_fileName;
    }
}
