/****************************************************************************************
 * Copyright (c) 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                             *
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

#ifndef AMAROK_PLAYLISTMODELSTACK_H
#define AMAROK_PLAYLISTMODELSTACK_H

#include "proxymodels/AbstractModel.h"
#include "PlaylistModel.h"
#include "proxymodels/FilterProxy.h"
#include "proxymodels/SortProxy.h"
#include "proxymodels/SearchProxy.h"
#include "proxymodels/GroupingProxy.h"

namespace Playlist
{

/**
 * Singleton class that handles and wraps around the Playlist models architecture.
 * To talk to the playlist, use The::playlist(). Playlist::ModelStack::instance()->source()
 * should only be used internally or in very specific situations.
 * @author Téo Mrnjavac <teo.mrnjavac@gmail.com>
 */
class ModelStack : public QObject
{
    Q_OBJECT
public:
    /**
     * Accessor for the singleton pattern.
     * @return a pointer to the only instance of Playlist::ModelStack.
     */
    static ModelStack *instance();

    /**
     * Singleton destructor.
     */
    static void destroy();

    /**
     * The topmost proxy model in the stack
     */
    GroupingProxy * top();
    Model *         source();
    SortProxy *     sortProxy();

private:
    /**
     * Constructor.
     */
    ModelStack();

    /**
     * Destructor.
     */
    ~ModelStack();

    static ModelStack *s_instance;       //! Instance member.

    Model *             m_model;
    FilterProxy *       m_filter;
    SortProxy *         m_sort;
    SearchProxy *       m_search;
    GroupingProxy *     m_grouping;
};

}   //namespace Playlist

namespace The
{
AMAROK_EXPORT Playlist::GroupingProxy* playlist();
}

#endif  //AMAROK_PLAYLISTMODELSTACK_H
