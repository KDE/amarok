/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "MultiSourceCapabilityImpl.h"


MultiSourceCapabilityImpl::MultiSourceCapabilityImpl(Meta::MultiTrack * track)
    : Meta::MultiSourceCapability()
    , m_track( track )
{
    //forward from track, as there might  be several instances of MultiSourceCapabilityImpl active for one track.
    connect( m_track, SIGNAL( urlChanged( const KUrl &) ), this, SIGNAL( urlChanged( const KUrl &) ) );
}

void MultiSourceCapabilityImpl::setSource( int source )
{
    DEBUG_BLOCK
    m_track->setSource( source );
    const KUrl url = m_track->playableUrl();

}



#include "MultiSourceCapabilityImpl.moc"
