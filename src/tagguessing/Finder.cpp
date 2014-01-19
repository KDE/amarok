/****************************************************************************************
 * Copyright (c) 2013 Vedant Agarwala <vedant.kota@gmail.com>                           *
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

#include "Finder.h"

#include "musicbrainz/MusicBrainzFinder.h"

#ifdef LIBCHROMAPRINT_FOUND
#include "acoustid/Acoustidprovider.h"
#endif

using namespace TagGuessing;

Finder::Finder(QObject *parent) :
    QObject( parent )
{
    addProviders();
}

Finder::~Finder()
{
    // the individual providers will be freed as "this" is their parent.
}

bool
Finder::isRunning()
{
    foreach( const ProviderPtr &provider, m_providers )
        if( provider->isRunning() )
            return true;
    return false;
}

void
Finder::run( const Meta::TrackList &tracks )
{
    foreach( const ProviderPtr &provider, m_providers )
        provider->run( tracks );
}

void
Finder::lookUpByPUID( const Meta::TrackPtr &track, const QString &puid )
{
    foreach ( const ProviderPtr &provider, m_providers)
        provider->lookUpByPUID( track, puid );
}

void
Finder::addProviders()
{
    m_providers.append( new MusicBrainzFinder( this ) );
#ifdef LIBCHROMAPRINT_FOUND
    m_providers.append( new AcoustIDFinder( this ) );
#endif
}

#include "Finder.moc"
