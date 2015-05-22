/****************************************************************************************
 * Copyright (c) 2009 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
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

#include "WriteCapability.h"

Handler::WriteCapability::~WriteCapability()
{
    // nothing to do here
}

void
Handler::WriteCapability::libSetIsCompilation( Meta::MediaDeviceTrackPtr &track, bool isCompilation )
{
    Q_UNUSED( track )
    Q_UNUSED( isCompilation )
    // provide default implementation so that MTP collection doesn't need to override.
}

void
Handler::WriteCapability::libSetReplayGain( Meta::MediaDeviceTrackPtr &track, qreal newReplayGain )
{
    Q_UNUSED( track )
    Q_UNUSED( newReplayGain )
}

