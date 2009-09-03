/****************************************************************************************
 * Copyright (c) 2003 Frerich Raabe <raabe@kde.org>                                     *
 * Copyright (c) 2005 Alexandre Pereira de Oliveira <aleprj@gmail.com>                  *
 * Copyright (c) 2008 Teo Mrnjavac <teo.mrnjavac@gmail.com>                             *
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

#include "FilenameLayoutDialog.h"

#include <QRegExp>
#include <QList>

class FileNameScheme
{
    public:
        typedef QList<FileNameScheme> List;

        FileNameScheme()
            : m_cod()
            , m_titleField( 0 )
            , m_artistField( 0 )
            , m_albumField( 0 )
            , m_trackField( 0 )
            , m_commentField( 0 )
            , m_yearField( 0 )
            , m_composerField( 0 )
            , m_genreField( 0 )
        { }

        FileNameScheme( const QString &s );

        bool matches( const QString &s ) const;

        QString title() const;
        QString artist() const;
        QString album() const;
        QString track() const;
        QString comment() const;
        QString year() const;
        QString composer() const;
        QString genre() const;

        QString pattern() const { return m_cod; }

    private:
        QString composeRegExp( const QString &s ) const;
        QString m_cod;

        mutable QRegExp m_regExp;

        int m_titleField;
        int m_artistField;
        int m_albumField;
        int m_trackField;
        int m_commentField;
        int m_yearField;
        int m_composerField;
        int m_genreField;
};

class TagGuesser
{
    public:

        enum Type { FileName = 0, MusicBrainz = 1 };

        static QStringList schemeStrings();
        static void setSchemeStrings( const QStringList &schemes );

        TagGuesser();
        TagGuesser( const QString &absFileName, FilenameLayoutDialog *dialog );

        void guess( const QString &absFileName, FilenameLayoutDialog *dialog );

        QString title() const { return m_title; }
        QString artist() const { return m_artist; }
        QString album() const { return m_album; }
        QString track() const { return m_track; }
        QString comment() const { return m_comment; }
        QString year() const { return m_year; }
        QString composer() const { return m_composer; }
        QString genre() const { return m_genre; }

    private:
        void loadSchemes();
        QString capitalizeWords( const QString &s, const int &caseOptions );

        FileNameScheme::List m_schemes;
        QString m_title;
        QString m_artist;
        QString m_album;
        QString m_track;
        QString m_comment;
        QString m_year;
        QString m_composer;
        QString m_genre;
};

#endif /* TAGGUESSER_H */

