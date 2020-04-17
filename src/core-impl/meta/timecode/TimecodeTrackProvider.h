/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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
 
#ifndef TIMECODETRACKPROVIDER_H
#define TIMECODETRACKPROVIDER_H

#include "core/collections/Collection.h"

/**
A track provider that recognizes timecode track urls

	@author Nikolaj Hald Nielsen <nhn@kde.org>
*/
class TimecodeTrackProvider : public Collections::TrackProvider{
public:
    TimecodeTrackProvider();

    ~TimecodeTrackProvider() override;

    bool possiblyContainsTrack( const QUrl &url ) const override;
    Meta::TrackPtr trackForUrl( const QUrl &url ) override;

};

#endif
