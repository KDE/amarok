/****************************************************************************************
 * Copyright (c) 2008 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef MEDIA_DEVICE_INFO_H
#define MEDIA_DEVICE_INFO_H

#include "mediadevicecollection_export.h"

#include <QObject>
#include <QString>


//FIXME why does this stuff need to be in a libamarok file at all? It
//Basically makes it impossible to write a new device type purely as a plugin...
//Baaaaad xevix! Bad! :-P  ( nhn - 250609 )
typedef enum {
    IPOD_T,
    MTP_T,
    UNK_T,
    AUDIOCD_T
} device_info_t;

class MEDIADEVICECOLLECTION_EXPORT MediaDeviceInfo : public QObject
{
    Q_OBJECT
    public:

        device_info_t type();
        QString udi();

    protected:
        MediaDeviceInfo();
        virtual ~MediaDeviceInfo();

        device_info_t m_type;
        QString m_udi;
};

#endif
