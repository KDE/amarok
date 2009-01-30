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

#include "MediaDeviceInfo.h"

MediaDeviceInfo::MediaDeviceInfo()
: QObject()
{
//    m_type = type;
//    m_udi = udi;
}

MediaDeviceInfo::~MediaDeviceInfo()
{
}

device_info_t
MediaDeviceInfo::type()
{
    return m_type;
}

QString
MediaDeviceInfo::udi()
{
    return m_udi;
}

#include "MediaDeviceInfo.moc"
