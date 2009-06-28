/****************************************************************************************
 * Copyright (c) 2009 Casey Link <unnamedrambler@gmail.com>                             *
 * Copyright (c) 2008 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
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

#include "SqlCollectionCapability.h"
#include "SqlCollection.h"
#include "Debug.h"
#include "SvgHandler.h"
#include "context/popupdropper/libpud/PopupDropperAction.h"

#include "MetaQueryMaker.h"

#include <KIcon>
#include <KLocale>

using namespace Meta;


SqlCollectionCapability::SqlCollectionCapability ( SqlCollection *coll )
        : CollectionCapability()
        , m_coll ( coll ) {}

QList<PopupDropperAction *>
SqlCollectionCapability::collectionActions ( QueryMaker *qm )
{
    DEBUG_BLOCK
    QList< PopupDropperAction* > actions;

    // Create helper

    Meta::CollectionCapabilityHelper *helper = new Meta::CollectionCapabilityHelper ( qm );

    // Create action

    PopupDropperAction *deleteAction = new PopupDropperAction ( The::svgHandler()->getRenderer ( "amarok/images/pud_items.svg" ),
            "delete", KIcon ( "remove-amarok" ), i18n ( "&Delete Tracks" ), 0 );

    // Delete action triggered() goes to helper's run query

    helper->setAction ( deleteAction, m_coll, SLOT ( deleteTracksSlot ( Meta::TrackList ) ) );

    actions.append ( deleteAction );

    return actions;
}

// NOTE: NYI
QList<PopupDropperAction *>
SqlCollectionCapability::collectionActions ( const TrackList tracklist )
{
    Q_UNUSED ( tracklist );

    QList< PopupDropperAction* > actions;

    return actions;
}

#include "SqlCollectionCapability.moc"
