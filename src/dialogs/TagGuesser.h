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

class TagGuesser
{
    public:

        TagGuesser();

        /**
        *   Sets the Filename to guess from
        *   @arg fileName Filename to guess from
        */
        void setFilename( const QString &fileName );

        /**
        *   Sets the schema to guess from
        *   @arg schema schema to guess from
        */
        void setSchema( const QString &schema );

        /**
        *   Sets the case type
        *   @arg caseOptions the case type to use
        **/
        void setCaseType( const int caseOptions );

        /**
        *   Sets whether trailing spaces are cut from tags
        *   @arg cutSpaces should trailing spaces be cut from tags
        **/
        void setCutTrailingSpaces( const bool cutTrailingSpaces );

        /**
        *   Sets whether underscores should be converted to spaces
        *   @arg convertUnderscores should underscores be converted
        */
        void setConvertUnderscores( const bool convertUnderscores );

        /**
        *   tries guessing tags from filename and options
        */
        bool guess();

        /**
        *  @returns a list of guessed Tags
        */
        QMap<qint64,QString> tags() { return m_tags; };

    private:

        QMap<qint64,QString> m_tags;        //!< Taglist (e.g. < Meta::valArtist,"some artist" >
        bool m_guessed;                     //!< Is true when guessing was done
        QString m_fileName;                 //!< Filename to be guessed from
        QString m_schema;                   //!< Schema after which should be guessed
        int m_caseOptions;                  //!< Case options to change tags after
        bool m_cutTrailingSpaces;           //!< Whether trailing spaces should be cut from tags
        bool m_convertUnderscores;          //!< Whether underscores should be converted to spaces

        /**
        *   Converts a tag to specific case type
        *   @arg tag    tag to convert
        *   @arg type   case type to convert tag to
        *   @returns    the converted tag
        */
        QString convertTagCaseType( const QString &tag, int type );
};

#endif /* TAGGUESSER_H */

