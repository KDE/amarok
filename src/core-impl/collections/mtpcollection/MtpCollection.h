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

#ifndef MTPCOLLECTION_H
#define MTPCOLLECTION_H

#include "MtpHandler.h"

#include "MediaDeviceCollection.h"
#include "core/support/Debug.h"

#include <QtGlobal>

#include <QIcon>

class MediaDeviceInfo;

namespace Collections {

class MtpCollection;

class MtpCollectionFactory : public MediaDeviceCollectionFactory<MtpCollection>
{
    Q_PLUGIN_METADATA(IID AmarokPluginFactory_iid FILE "amarok_collection-mtpcollection.json")
    Q_INTERFACES(Plugins::PluginFactory)
    Q_OBJECT

    public:
        MtpCollectionFactory();
        ~MtpCollectionFactory() override;

};

class MtpCollection : public MediaDeviceCollection
{
    Q_OBJECT

	public:

    explicit MtpCollection( MediaDeviceInfo* );
    ~MtpCollection() override;

    QString collectionId() const override;
    QString prettyName() const override;
    QIcon icon() const override { return QIcon::fromTheme(QStringLiteral("multimedia-player")); }

    //void writeDatabase();
};

} //namespace Collections

#endif
