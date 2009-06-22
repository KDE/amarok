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

#include "MtpDeviceInfo.h"
#include "MediaDeviceInfo.h"

MtpDeviceInfo::MtpDeviceInfo( QString udi )
: MediaDeviceInfo()
{
    m_type = MTP_T;
    m_udi = udi;
}

MtpDeviceInfo::~MtpDeviceInfo()
{
}

#include "MtpDeviceInfo.moc"
