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

#ifndef MEDIA_DEVICE_INFO_H
#define MEDIA_DEVICE_INFO_H

#include "mediadevicecollection_export.h"

#include <QObject>
#include <QString>

typedef enum {
    IPOD_T,
    MTP_T,
    UNK_T
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
