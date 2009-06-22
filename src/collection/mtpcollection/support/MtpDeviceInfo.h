/*****************************************************************************
* copyright            : (C) 2008 Alejandro Wainzinger <aikawarazuni@gmail.com>
*
 *****************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef MTP_DEVICE_INFO_H
#define MTP_DEVICE_INFO_H

#include "MediaDeviceInfo.h"

class MtpDeviceInfo : public MediaDeviceInfo
{
    Q_OBJECT
    public:
        MtpDeviceInfo( QString udi );
        ~MtpDeviceInfo();

    private:

};

#endif
