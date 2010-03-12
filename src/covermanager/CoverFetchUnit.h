/****************************************************************************************
 * Copyright (c) 2009 Rick W. Chen <stuffcorpse@archlinux.us>                           *
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

#ifndef AMAROK_COVERFETCHUNIT_H
#define AMAROK_COVERFETCHUNIT_H

#include "meta/Meta.h"

#include <KSharedPtr>

class CoverFetchPayload;
class CoverFetchSearchPayload;

namespace CoverFetch
{
    enum Option
    {
        Automatic,      //! Automtically save cover for the specified album, if one is found
        Interactive,    //! Opens a dialog for the user to decide, and add more searches if desired
        WildInteractive //! As \ref Interactive, but without filtering results (used for web search)
    };

    enum ImageSize
    {
        NormalSize,     //! Normal cover size, for storage and display
        ThumbSize       //! Thumbnail size, for icon views
    };

    typedef QHash< QString, QString > Metadata;
    typedef QHash< KUrl, Metadata > Urls;
}

/**
 * A work unit for the cover fetcher queue.
 */
class CoverFetchUnit : public QSharedData
{
public:
    typedef KSharedPtr< CoverFetchUnit > Ptr;

    CoverFetchUnit( Meta::AlbumPtr album,
                    const CoverFetchPayload *payload,
                    CoverFetch::Option opt = CoverFetch::Automatic );
    CoverFetchUnit( const CoverFetchPayload *payload, CoverFetch::Option opt );
    CoverFetchUnit( const CoverFetchSearchPayload *payload );
    CoverFetchUnit( const CoverFetchUnit &cpy );
    explicit CoverFetchUnit() {}
    ~CoverFetchUnit();

    Meta::AlbumPtr album() const;
    const QStringList &errors() const;
    CoverFetch::Option options() const;
    const CoverFetchPayload *payload() const;

    bool isInteractive() const;

    template< typename T >
        void addError( const T &error );

    CoverFetchUnit &operator=( const CoverFetchUnit &rhs );
    bool operator==( const CoverFetchUnit &other ) const;
    bool operator!=( const CoverFetchUnit &other ) const;

private:
    Meta::AlbumPtr m_album;
    QStringList m_errors;
    CoverFetch::Option m_options;
    const CoverFetchPayload *m_payload;
};

/**
 * An abstract class for preparing URLs suitable for fetching album covers from
 * Last.fm.
 */
class CoverFetchPayload
{
public:
    enum Type { Info, Search, Art };
    CoverFetchPayload( const Meta::AlbumPtr album, enum Type type );
    virtual ~CoverFetchPayload();

    enum Type type() const;
    const CoverFetch::Urls &urls() const;

protected:
    CoverFetch::Urls m_urls;

    const QString &method() const { return m_method; }
    Meta::AlbumPtr album()  const { return m_album; }

    bool isPrepared() const;
    virtual void prepareUrls() = 0;

private:
    Meta::AlbumPtr m_album;
    const QString  m_method;
    enum Type      m_type;

    Q_DISABLE_COPY( CoverFetchPayload );
};

/**
 * Prepares URL suitable for getting an album's info from Last.fm.
 */
class CoverFetchInfoPayload : public CoverFetchPayload
{
public:
    explicit CoverFetchInfoPayload( const Meta::AlbumPtr album );
    ~CoverFetchInfoPayload();

protected:
    void prepareUrls();

private:
    Q_DISABLE_COPY( CoverFetchInfoPayload );
};

/**
 * Prepares URL for searching albums on Last.fm using wild mode.
 * See \ref CoverFetch::WildInteractive mode.
 */
class CoverFetchSearchPayload : public CoverFetchPayload
{
public:
    explicit CoverFetchSearchPayload( const QString &query = QString() );
    ~CoverFetchSearchPayload();

    QString query() const;

    void setQuery( const QString &query );

protected:
    void prepareUrls();

private:
    QString m_query;

    Q_DISABLE_COPY( CoverFetchSearchPayload );
};

/**
 * Prepares URL suitable for getting an album's cover from Last.fm.
 */
class CoverFetchArtPayload : public CoverFetchPayload
{
public:
    explicit CoverFetchArtPayload( const Meta::AlbumPtr album,
                                   const CoverFetch::ImageSize size = CoverFetch::NormalSize,
                                   bool wild = false );
    explicit CoverFetchArtPayload( const CoverFetch::ImageSize size, bool wild = false );
    ~CoverFetchArtPayload();

    bool isWild() const;

    CoverFetch::ImageSize imageSize() const;

    void setXml( const QByteArray &xml );

protected:
    void prepareUrls();

private:
    CoverFetch::ImageSize m_size;
    QString m_xml;

    /// search is wild mode?
    bool m_wild;

    /// convert ImageSize enum to string
    QString coverSize2str( enum CoverFetch::ImageSize size ) const;

    /// convert string to ImageSize
    enum CoverFetch::ImageSize str2CoverSize( const QString &string ) const;

    /// lower, remove whitespace, and do Unicode normalization on a QString
    QString normalize( const QString &raw );

    /// lower, remove whitespace, and do Unicode normalization on a QStringList
    QStringList normalize( const QStringList &rawList );

    Q_DISABLE_COPY( CoverFetchArtPayload );
};


#endif /* AMAROK_COVERFETCHUNIT_H */
