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
 
#ifndef TAGGUESSER_H
#define TAGGUESSER_H

#include <QMap>
#include <QString>
#include <QRegExp>

#define album_color Qt::red
#define artist_color Qt::blue
#define comment_color Qt::gray
#define composer_color Qt::magenta
#define genre_color Qt::cyan
#define title_color Qt::green
#define track_color Qt::yellow
#define year_color Qt::darkRed

class TagGuesser
{
    public:

        TagGuesser();

        /**
        *   Sets the Filename to guess from
        *   @arg fileName Filename to guess from
        */
        void setFilename( const QString fileName );

        /**
        *   Sets the schema to guess from
        *   @arg schema schema to guess from
        */
        void setSchema( const QString schema );

        /**
        *   Sets the case type
        *   @arg caseOptions the case type to use
        **/
        void setCaseType( const int caseOptions );

        /**
        *   Sets wether trailing spaces are cut from tags
        *   @arg cutSpaces should trailing spaces be cut from tags
        **/
        void setCutTrailingSpaces( const bool cutTrailingSpaces );

        /**
        *   Sets wether underscores should be converted to spaces
        *   @arg convertUnderscores should underscores be converted
        */
        void setConvertUnderscores( const bool convertUnderscores );
        
        /**
        *   tries guessing tags from filename and options
        */
        bool guess();

        /**
        *   @Returns a list of guessed Tags
        */
        QMap<QString,QString> tags() { return m_tags; };

        /**
        *   @Returns a colored version of the filename
        */
        QString coloredFileName();

    private:

        struct TagStruct
        {
            QString type;
            QString tag;
        };
        
        QMap<QString,QString> m_tags;
        QMap<int,TagStruct> m_sortedTags;
        bool m_guessed;
        QString m_fileName;
        QString m_schema;
        int m_caseOptions;
        bool m_cutTrailingSpaces;
        bool m_convertUnderscores;

        /**
        *   Converts a tag to specific case type
        *   @arg tag    tag to convert
        *   @arg type   case type to convert tag to
        *   @returns    the converted tag
        */
        QString convertTagCaseType( QString tag, int type );

        /**
        *   Converts all underscores in a filename to spaces
        *   @arg filename the filename to be converted
        *   @returns the converted filename
        */
        QString convertUnderscores( QString tag );

        /**
        *   Cuts trailing spaces from a tag
        *   @arg tag    tag to be used
        *   @returns    the tag without trailing spaces
        */
        QString cutTagTrailingSpaces( QString tag );

        /**
        *   Generates a regular expression from a given schema
        *   @arg schema schema to generate expression from
        *   @returns a regular expression
        */
        QString getRegExpFromSchema( QString schema );
};

#endif /* TAGGUESSER_H */

