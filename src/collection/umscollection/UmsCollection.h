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

#ifndef UMSCOLLECTION_H
#define UMSCOLLECTION_H

#include "UmsHandler.h"

#include "MediaDeviceCollection.h"
#include "Debug.h"

#include <KIcon>

#include <QtGlobal>

class UmsCollection;
class MediaDeviceInfo;

class UmsCollectionFactory : public MediaDeviceCollectionFactory<UmsCollection>
{
    Q_OBJECT

    public:
        UmsCollectionFactory();
        virtual ~UmsCollectionFactory();
};

class UmsCollection : public MediaDeviceCollection
{
    Q_OBJECT

    public:
        // inherited methods

        UmsCollection( MediaDeviceInfo* info );
        virtual ~UmsCollection();

        virtual bool possiblyContainsTrack( const KUrl &url ) const;
        virtual Meta::TrackPtr trackForUrl( const KUrl &url );

        virtual QString collectionId() const;
        virtual QString prettyName() const;
        virtual KIcon icon() const { return KIcon("drive-removable-media-usb-pendrive"); };

        // HACK: this function will be deleted later
        //void writeDatabase() { m_handler->writeDatabase(); }

    private:
        QString m_mountPoint;
};

#endif
