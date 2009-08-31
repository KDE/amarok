/****************************************************************************************
 * Copyright (c) 2009 Martin Aumueller <aumuell@reserv.at>                              *
 * Copyright (c) 2008 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
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

#ifndef IPHONEOSCOLLECTION_H
#define IPHONEOSCOLLECTION_H

#include "MediaDeviceCollection.h"
#include "Debug.h"

#include <KIcon>

#include <QtGlobal>

class IphoneOsCollection;
class MediaDeviceInfo;

class IphoneOsCollectionFactory : public MediaDeviceCollectionFactory<IphoneOsCollection>
{
    Q_OBJECT
    public:
        IphoneOsCollectionFactory();
        virtual ~IphoneOsCollectionFactory();
};

class IphoneOsCollection : public MediaDeviceCollection
{
    Q_OBJECT
    public:
        // inherited methods

        IphoneOsCollection( MediaDeviceInfo* info );
        virtual ~IphoneOsCollection();

        virtual bool possiblyContainsTrack( const KUrl &url ) const;
        virtual Meta::TrackPtr trackForUrl( const KUrl &url );

        virtual QString collectionId() const;
        virtual QString prettyName() const;
        virtual KIcon icon() const { return KIcon("multimedia-player-apple-ipod"); };

    private:
        QString            m_mountPoint;
};

#endif
