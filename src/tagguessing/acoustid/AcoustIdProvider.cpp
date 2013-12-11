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

#include "AcoustIdProvider.h"

AcoustIdProvider::AcoustIdProvider()
{
}

AcoustIdProvider::~AcoustIdProvider()
{
}

void
AcoustIdProvider::run( const Meta::TrackList &tracks )
{
    m_fingerprintCalculator = new FingerprintCalculator ( tracks );
    connect( m_fingerprintCalculator, SIGNAL(trackDecoded(Meta::TrackPtr,QString)), SLOT(fingerprintCalculated(Meta::TrackPtr,QString)) );
    m_fingerprintCalculator->run();
}

void
AcoustIdProvider::fingerprintCalculated( const Meta::TrackPtr &track, const QString &fingerprint )
{

}

#include "AcoustIdProvider.moc"
