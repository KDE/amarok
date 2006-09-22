/* This file is part of the KDE Project
   Copyright (c) 2004 K�in Ottens <ervin ipsquad net>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _MEDIUM_H_
#define _MEDIUM_H_

#include "amarok_export.h"

#include <qstring.h>
#include <qstringlist.h>
#include <kurl.h>

class LIBAMAROK_EXPORT Medium
{
public:
    typedef QValueList<const Medium> List;

    static const uint AUTODETECTED = 0;
    static const uint ID = 1;
    static const uint NAME = 2;
    static const uint LABEL = 3;
    static const uint USER_LABEL = 4;
    static const uint MOUNTABLE = 5;
    static const uint DEVICE_NODE = 6;
    static const uint MOUNT_POINT = 7;
    static const uint FS_TYPE = 8;
    static const uint MOUNTED = 9;
    static const uint BASE_URL = 10;
    static const uint MIME_TYPE = 11;
    static const uint ICON_NAME = 12;
    static const uint PROPERTIES_COUNT = 13;
    static const QString SEPARATOR;

    Medium();
    Medium(const Medium *medium);
    Medium(const QString &id, const QString &name);
    static const Medium create(const QStringList &properties);
    static List createList(const QStringList &properties);

    const QStringList &properties() const { return m_properties; };

    bool isAutodetected() const { return m_properties[AUTODETECTED]=="true"; };
    QString id() const { return m_properties[ID]; };
    QString name() const { return m_properties[NAME]; };
    QString label() const { return m_properties[LABEL]; };
    QString userLabel() const { return m_properties[USER_LABEL]; };
    bool isMountable() const { return m_properties[MOUNTABLE]=="true"; };
    QString deviceNode() const { return m_properties[DEVICE_NODE]; };
    QString mountPoint() const { return m_properties[MOUNT_POINT]; };
    QString fsType() const { return m_properties[FS_TYPE]; };
    bool isMounted() const { return m_properties[MOUNTED]=="true"; };
    QString baseURL() const { return m_properties[BASE_URL]; };
    QString mimeType() const { return m_properties[MIME_TYPE]; };
    QString iconName() const { return m_properties[ICON_NAME]; };

    bool needMounting() const;
    KURL prettyBaseURL() const;
    QString prettyLabel() const;

    void setAutodetected(bool autodetected);
    void setId(const QString &id);
    void setMountPoint(const QString &mountPoint);
    void setName(const QString &name);
    void setLabel(const QString &label);
    void setUserLabel(const QString &label);
    void setFsType(const QString &type);

    bool mountableState(bool mounted);
    void mountableState(const QString &deviceNode,
                        const QString &mountPoint,
                        const QString &fsType, bool mounted);
    void unmountableState(const QString &baseURL = QString::null);

    void setMimeType(const QString &mimeType);
    void setIconName(const QString &iconName);

private:
    void loadUserLabel();

    QStringList m_properties;

friend class QValueListNode<const Medium>;
};

#endif
