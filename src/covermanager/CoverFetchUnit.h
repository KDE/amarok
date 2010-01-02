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

/**
 * A work unit for the cover fetcher queue.
 */
class CoverFetchUnit : public QSharedData
{
public:
    typedef KSharedPtr< CoverFetchUnit > Ptr;

    CoverFetchUnit( Meta::AlbumPtr album, const CoverFetchPayload *url, bool interactive = false );
    CoverFetchUnit( const CoverFetchUnit &cpy );
    explicit CoverFetchUnit() {}
    ~CoverFetchUnit();

    Meta::AlbumPtr album() const;
    const QStringList &errors() const;
    const CoverFetchPayload *url() const;

    bool isInteractive() const;

    template< typename T >
        void addError( const T &error );

    CoverFetchUnit &operator=( const CoverFetchUnit &rhs );
    bool operator==( const CoverFetchUnit &other ) const;
    bool operator!=( const CoverFetchUnit &other ) const;

private:
    Meta::AlbumPtr m_album;
    QStringList m_errors;
    bool m_interactive;
    const CoverFetchPayload *m_url;
};

/**
 * An abstract class for preparing URLs suitable for fetching album covers from
 * Last.fm.
 */
class CoverFetchPayload
{
public:
    enum Type { INFO, ART };
    CoverFetchPayload( const Meta::AlbumPtr album, enum Type type );
    virtual ~CoverFetchPayload();

    enum Type type() const;
    const KUrl::List &urls() const;

protected:
    KUrl::List m_urls;

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
 * Prepares URL suitable for getting an album's info as an CoverFetchInfoPayload from Last.fm.
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
 * Prepares URL suitable for getting an album's cover from Last.fm.
 */
class CoverFetchArtPayload : public CoverFetchPayload
{
public:
    explicit CoverFetchArtPayload( const Meta::AlbumPtr album );
    ~CoverFetchArtPayload();

    void setXml( const QByteArray &xml );

protected:
    void prepareUrls();

private:
    QString m_xml;

    /// Available album cover sizes in Last.fm's api
    enum CoverSize
    {
        Small = 0,  //! 34px
        Medium,     //! 64px
        Large,      //! 128px
        ExtraLarge  //! 300px
    };

    /// convert CoverSize enum to string
    QString coverSize( enum CoverSize size ) const;

    /// lower, remove whitespace, and do Unicode normalization on a QString
    QString normalize( const QString &raw );

    /// lower, remove whitespace, and do Unicode normalization on a QStringList
    QStringList normalize( const QStringList &rawList );

    Q_DISABLE_COPY( CoverFetchArtPayload );
};


#endif /* AMAROK_COVERFETCHUNIT_H */
