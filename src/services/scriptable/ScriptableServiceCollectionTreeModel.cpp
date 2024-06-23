/****************************************************************************************
 * Copyright (c) 2013 Matěj Laitl <matej@laitl.cz>                                      *
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

#define DEBUG_PREFIX "ScriptableServiceCollectionTreeModel"

#include "ScriptableServiceCollectionTreeModel.h"

#include "AmarokMimeData.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/TextualQueryFilter.h"
#include "services/scriptable/ScriptableServiceMeta.h"
#include "services/scriptable/ScriptableServiceQueryMaker.h"

ScriptableServiceCollectionTreeModel::ScriptableServiceCollectionTreeModel(
        Collections::Collection *collection,
        const QList<CategoryId::CatMenuId> &levelType )
    : SingleCollectionTreeItemModel( collection, levelType )
{
}

QMimeData *
ScriptableServiceCollectionTreeModel::mimeData( const QList<CollectionTreeItem *> &items ) const
{
    // this is basically a copy of superclass method with a couple of changes:
    // 1. we don't reuse tracks already in the model
    // 2. we tell the querymaker to masquerade special tracks

    using namespace Collections;
    Meta::TrackList tracks;
    QList<QueryMaker *> queries;
    for( CollectionTreeItem *item : items )
    {
        if( item->isTrackItem() )
        {
            using namespace Meta;
            const ScriptableServiceTrack *serviceTrack =
                    dynamic_cast<const ScriptableServiceTrack *>( item->data().data() );
            if( !serviceTrack )
            {
                error() << "failed to convert generic track" << item->data() << "to ScriptableServiceTrack";
                continue;
            }
            tracks << serviceTrack->playableTrack();
            continue;
        }

        ScriptableServiceQueryMaker *qm = qobject_cast<ScriptableServiceQueryMaker *>( item->queryMaker() );
        if( !qm )
        {
            error() << "failed to convert generic QueryMaker to ScriptableService one";
            continue;
        }
        qm->setConvertToMultiTracks( true );
        for( CollectionTreeItem *tmp = item; tmp; tmp = tmp->parent() )
            tmp->addMatch( qm, levelCategory( tmp->level() - 1 ) );
        Collections::addTextualFilter( qm, m_currentFilter );
        queries.append( qm );
    }

    if( queries.isEmpty() && tracks.isEmpty() )
        return nullptr;

    AmarokMimeData *mimeData = new AmarokMimeData();
    mimeData->setTracks( tracks );
    mimeData->setQueryMakers( queries );
    mimeData->startQueries();
    return mimeData;
}
