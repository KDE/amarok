/****************************************************************************************
 * Copyright (c) 2009 Téo Mrnjavac <teo@kde.org>                                        *
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

#ifndef AMAROK_PLAYLISTMODELSTACK_H
#define AMAROK_PLAYLISTMODELSTACK_H

#include "proxymodels/AbstractModel.h"
#include "PlaylistModel.h"
#include "proxymodels/SortFilterProxy.h"
#include "proxymodels/SearchProxy.h"
#include "proxymodels/GroupingProxy.h"

// THE PLAYLIST MODELS ARCHITECTURE
// Amarok's playlist uses Qt's Model-View design pattern, so we have
// * 1 source model which feeds the tracks data: Playlist::Model
// * 3 proxies which modify the rows: SortFilterProxy==>SearchProxy==>GroupingProxy
// * 1 or more views, such as Playlist::PrettyListView.
// At any time a view should ONLY talk to the topmost proxy model, exposed by Playlist::
// ModelStack::instance()->groupingProxy() or The::playlist() for short.
// External classes that talk to the playlist should only talk to The::playlist(),
// exceptionally to Playlist::ModelStack::instance()->bottom() if they really really need
// the source model.
//
// Each playlist model implements the interface defined in Playlist::AbstractModel.
// Each playlist proxy is a subclass od QSortFilterProxyModel through Playlist::ProxyBase,
// and uses the default implementations of AbstractModel methods defined in ProxyBase.
// To add a new proxy the recommended procedure is to subclass ProxyBase (which drags in
// QSortFilterProxyModel, this is no problem because QSortFilterProxyModel is transparent
// and fast by default), reimplement the relevant methods from ProxyBase and insert it in
// the right place in the ModelStack constructor.
//      --Téo 13/8/2009

namespace Playlist
{

/**
 * Singleton class that handles and wraps around the Playlist models architecture.
 * To talk to the playlist, use The::playlist(). Playlist::ModelStack::instance()->bottom()
 * should only be used internally or in very specific situations.
 * @author Téo Mrnjavac <teo@kde.org>
 */
class AMAROK_EXPORT ModelStack : public QObject
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
     * Use the 'The::playlist()' model unless you have a specific need for a lower model.
     */
    GroupingProxy           *groupingProxy();
    SortFilterProxy         *sortProxy();
    SortFilterProxy         *filterProxy();
    Playlist::Model         *bottom();

private:
    /**
     * Constructor.
     */
    ModelStack();

    /**
     * Destructor.
     */
    ~ModelStack() override;

    static ModelStack *s_instance;       //!< Instance member.

    GroupingProxy   *m_grouping;
    SearchProxy     *m_search;
    SortFilterProxy *m_sortfilter;
    Model           *m_model;
};

}   //namespace Playlist

namespace The
{
    AMAROK_EXPORT Playlist::AbstractModel* playlist();
}

#endif  //AMAROK_PLAYLISTMODELSTACK_H
