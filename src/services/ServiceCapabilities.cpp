/****************************************************************************************
 * Copyright (c) 2010 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#include "ServiceCapabilities.h"
#include "ServiceMetaBase.h"


ServiceBookmarkThisCapability::ServiceBookmarkThisCapability( BookmarkThisProvider * provider )
    : Capabilities::BookmarkThisCapability(nullptr)
    , m_provider( provider )
{
}


ServiceBookmarkThisCapability::~ServiceBookmarkThisCapability()
{
}

bool ServiceBookmarkThisCapability::isBookmarkable()
{
    return m_provider->isBookmarkable();
}

QString ServiceBookmarkThisCapability::browserName()
{
    return m_provider->browserName();
}

QString ServiceBookmarkThisCapability::collectionName()
{
    return m_provider->collectionName();
}

bool ServiceBookmarkThisCapability::simpleFiltering()
{
    return m_provider->simpleFiltering();
}

QAction * ServiceBookmarkThisCapability::bookmarkAction() const
{
    return m_provider->bookmarkAction();
}

///////////////////////////////////////////////////////


ServiceActionsCapability::ServiceActionsCapability(ActionsProvider * actionsProvider)
    : Capabilities::ActionsCapability( )
    , m_actionsProvider( actionsProvider )
{
}

ServiceActionsCapability::~ServiceActionsCapability()
{
}

QList< QAction * > ServiceActionsCapability::actions() const
{
    return m_actionsProvider->actions();
}

///////////////////////////////////////////////////////


ServiceSourceInfoCapability::ServiceSourceInfoCapability(SourceInfoProvider * sourceInfoProvider)
   : SourceInfoCapability()
{
    m_sourceInfoProvider = sourceInfoProvider;
}

ServiceSourceInfoCapability::~ServiceSourceInfoCapability()
{
}

QString
ServiceSourceInfoCapability::sourceName()
{
    return m_sourceInfoProvider->sourceName();
}

QString
ServiceSourceInfoCapability::sourceDescription()
{
    return m_sourceInfoProvider->sourceDescription();
}

QPixmap
ServiceSourceInfoCapability::emblem()
{
    return m_sourceInfoProvider->emblem();
}


QString
ServiceSourceInfoCapability::scalableEmblem()
{
    return m_sourceInfoProvider->scalableEmblem();
}


////////////////////////////////////////////////////////////


ServiceFindInSourceCapability::ServiceFindInSourceCapability( Meta::ServiceTrack *track )
    : Capabilities::FindInSourceCapability()
    , m_track( track )
{}

void ServiceFindInSourceCapability::findInSource( QFlags<TargetTag> tag )
{
    Q_UNUSED( tag )
    DEBUG_BLOCK
    if( m_track->artist() && m_track->album() && !m_track->collectionName().isEmpty() )
    {
        QString collection =m_track->collectionName();
        QString artist = m_track->artist()->prettyName();
        QString album = m_track->album()->prettyName();

        AmarokUrl url;
        url.setCommand( QStringLiteral("navigate") );
        url.setPath( "internet/" + collection );
        if( !m_track->simpleFiltering() )
        {
            url.setArg( QStringLiteral("filter"), "artist:\"" + artist + "\" AND album:\"" + album + "\"" );
            url.setArg( QStringLiteral("levels"), QStringLiteral("artist-album") );
            debug() << "running url: " << url.url();
            url.run();
        }


    }
}
