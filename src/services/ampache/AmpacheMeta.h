/****************************************************************************************
 * Copyright (c) 2007 Casey Link <unnamedrambler@gmail.com>                             *
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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

#ifndef AMPACHEMETA_H
#define AMPACHEMETA_H


#include "../ServiceMetaBase.h"
#include "../ServiceAlbumCoverDownloader.h"
#include "../ShowInServiceAction.h"

#include <KStandardDirs>

#include <QDateTime>
#include <QString>
#include <QStringList>
#include <QList>


namespace Meta
{


class AmpacheTrack  : public ServiceTrack
{

public:

    explicit AmpacheTrack( const QString& title, ServiceBase * service = 0 )
    : ServiceTrack( title )
    , m_service( service )
    , m_showInServiceAction( 0 )
    { }

    virtual QString sourceName() { return "Ampache"; }
    virtual QString sourceDescription() { return "The Ampache music server project: http://Ampache.org"; }
    virtual QPixmap emblem()  { return QPixmap( KStandardDirs::locate( "data", "amarok/images/emblem-ampache.png" ) );  }
    virtual QString scalableEmblem()  { return  KStandardDirs::locate( "data", "amarok/images/emblem-ampache-scalable.svgz" );  }

    virtual QList< QAction *> currentTrackActions();

private:
    ServiceBase * m_service;
    ShowInServiceAction * m_showInServiceAction;

};


class AmpacheAlbum  : public ServiceAlbumWithCover
{
private:
    QString m_coverURL;


public:
    AmpacheAlbum( const QString &name );
    AmpacheAlbum( const QStringList &resultRow );

    ~AmpacheAlbum();

    virtual QString downloadPrefix() const { return "ampache"; }

    virtual void setCoverUrl( const QString &coverURL );
    virtual QString coverUrl() const;

    bool operator==( const Meta::Album &other ) const {
        return name() == other.name();
    }

};

class AmpacheArtist : public ServiceArtist
{
    private:
        QString m_coverURL;


    public:
        AmpacheArtist( const QString &name, ServiceBase * service )
            : ServiceArtist( name )
            , m_service( service )
             { }

        virtual bool isBookmarkable() { return true; }
        virtual QString collectionName() { return m_service->name(); }
        virtual bool simpleFiltering() { return true; }

    private:
        ServiceBase * m_service;

};

}

#endif
