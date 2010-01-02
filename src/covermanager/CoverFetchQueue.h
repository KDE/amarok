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

#ifndef AMAROK_COVERFETCHQUEUE_H
#define AMAROK_COVERFETCHQUEUE_H

#include "meta/Meta.h"
#include "CoverFetchUnit.h"

#include <KIO/Job>
#include <KUrl>

#include <QByteArray>
#include <QList>
#include <QObject>

class CoverFetchPayload;

/**
 * A queue that keeps track of albums to fetch covers for.
 */
class CoverFetchQueue : public QObject
{
    Q_OBJECT

public:
    CoverFetchQueue( QObject *parent = 0 );
    ~CoverFetchQueue();

    bool add( const Meta::AlbumPtr album,
              bool interactive = false,
              const QByteArray &xml = QByteArray() );

    bool contains( const Meta::AlbumPtr album ) const;
    int index( const Meta::AlbumPtr album ) const;
    int size() const;
    bool isEmpty() const;

    void clear();
    const CoverFetchUnit::Ptr take( const Meta::AlbumPtr album );

public slots:
    void remove( const CoverFetchUnit::Ptr unit );

signals:
    void fetchUnitAdded( const CoverFetchUnit::Ptr );

private:
    bool add( const CoverFetchUnit::Ptr unit );
    void remove( const Meta::AlbumPtr album );

    QList< CoverFetchUnit::Ptr > m_queue;
    Q_DISABLE_COPY( CoverFetchQueue );
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

#endif /* AMAROK_COVERFETCHQUEUE_H */
