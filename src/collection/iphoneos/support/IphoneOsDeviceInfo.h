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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef IPHONEOSDEVICEINFO_H
#define IPHONEOSDEVICEINFO_H

#include "MediaDeviceInfo.h"

class IphoneOsDeviceInfo : public MediaDeviceInfo
{
    Q_OBJECT
    public:
        IphoneOsDeviceInfo( QString mountpoint, QString udi );
        ~IphoneOsDeviceInfo();

        QString mountpoint();

    private:
        QString m_mountpoint;
};

#endif
