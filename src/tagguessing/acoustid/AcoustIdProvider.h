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

#ifndef ACOUSTIDPROVIDER_H
#define ACOUSTIDPROVIDER_H

#include "FingerprintCalculator.h"
#include "tagguessing/Provider.h"

class AcoustIdProvider : public TagGuessing::Provider
{
    Q_OBJECT
public:
    AcoustIdProvider();
    ~AcoustIdProvider();
    bool isRunning() const;

public slots:
    void run( const Meta::TrackList &tracks );
    void lookUpByPUID( const Meta::TrackPtr &track, const QString &puid );

    void fingerprintCalculated( const Meta::TrackPtr &track, const QString &fingerprint );

private:
    FingerprintCalculator* m_fingerprintCalculator;
};

#endif // ACOUSTIDPROVIDER_H
