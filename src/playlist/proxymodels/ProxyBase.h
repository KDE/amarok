/***************************************************************************
 *   Copyright © 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef AMAROK_PROXYBASE_H
#define AMAROK_PROXYBASE_H

#include "AbstractModel.h"
#include "playlist/PlaylistItem.h"

#include <QSortFilterProxyModel>

namespace Playlist
{

/**
 * A ProxyModel that implements all the common forwarders for the interface of any
 * playlist proxy.
 * @author Téo Mrnjavac <teo.mrnjavac@gmail.com>
 */
class ProxyBase : public QSortFilterProxyModel, public AbstractModel
{
    Q_OBJECT
public:
    /**
     * Constructor.
     */
    ProxyBase( QObject *parent = 0 );

    /**
     * Destructor.
     */
    virtual ~ProxyBase();

// Common public forwarder methods that pretty much just forward stuff through the stack of
// proxies start here.
// Please keep them sorted alphabetically.  -- Téo



signals:
    /**
     * Signal forwarded from the source model. IDs are unique so they shouldn't be modified
     * by the proxies.
     * @param the list of id's added that are also represented by this proxy.
     */
    void insertedIds( const QList<quint64>& );

    /**
     * Signal forwarded from the source model. IDs are unique so they shouldn't be modified
     * by the proxies.
     * @param the list of id's removed that are also represented by this proxy.
     */
    void removedIds( const QList<quint64>& );

protected:
    AbstractModel *m_belowModel;
};

}   //namespace Playlist

#endif  //AMAROK_PROXYBASE_H
