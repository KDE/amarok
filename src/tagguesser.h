// (c) 2005 Alexandre Pereira de Oliveira <aleprj@gmail.com>
// (c) 2003 Frerich Raabe <raabe@kde.org>
// See COPYING file for licensing information.

#ifndef TAGGUESSER_H
#define TAGGUESSER_H

#include <qregexp.h>

class FileNameScheme
{
    public:
        typedef QValueList<FileNameScheme> List;

        FileNameScheme()
            : m_cod()
            , m_titleField( -1 )
            , m_artistField( -1 )
            , m_albumField( -1 )
            , m_trackField( -1 )
            , m_commentField( -1 )
            , m_yearField( -1 )
            , m_composerField( -1 )
            , m_genreField( -1 )
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

        QString pattern() const { return m_cod; };

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
        TagGuesser( const QString &absFileName );

        void guess( const QString &absFileName );

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
        QString capitalizeWords( const QString &s );

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

