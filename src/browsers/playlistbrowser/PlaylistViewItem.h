/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2008 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#ifndef PLAYLISTVIEWITEM_H
#define PLAYLISTVIEWITEM_H

#include "meta/PlaylistGroup.h"

#include <QSharedData>
#include <KSharedPtr>

/**
	@author Nikolaj Hald Nielsen <nhn@kde.org>
*/

class PlaylistViewItem;
typedef KSharedPtr<PlaylistViewItem> PlaylistViewItemPtr;

class PlaylistViewItem : public virtual QSharedData
{
    public:
        PlaylistViewItem() : QSharedData() {}

        virtual  ~PlaylistViewItem() {};

        virtual Meta::PlaylistGroupPtr parent() const = 0;
        virtual int childCount() const { return 0; }
        virtual QString name() const = 0;
        virtual QString description() const = 0;
        virtual void rename( const QString &name ) = 0;

};

Q_DECLARE_METATYPE( PlaylistViewItemPtr )

#endif
