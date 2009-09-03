/****************************************************************************************
 * Copyright (c) 2008 Dan Meltzer <parallelgrapefruit@gmail.com>                        *
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
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

#ifndef MYDIROPERATOR_H
#define MYDIROPERATOR_H

#include "collection/Collection.h"
#include "playlist/PlaylistController.h"
#include "SvgHandler.h"


#include <KDirOperator>
#include <KFileItem>
#include <KMenu>
#include <KUrl>

/**
 * Stores a collection associated with an action for move/copy to collection
 */
class CollectionAction : public QAction
{
public:
    explicit CollectionAction( Amarok::Collection *coll, QObject *parent = 0 )
            : QAction( parent )
            , m_collection( coll )
    {
        setText( m_collection->prettyName() );
    }

    Amarok::Collection *collection() const
    {
        return m_collection;
    }

private:
    Amarok::Collection *m_collection;
};

class MyDirOperator : public KDirOperator
{
    Q_OBJECT

public:
    MyDirOperator( const KUrl &url, QWidget *parent );
    ~MyDirOperator();

private slots:
    void aboutToShowContextMenu();
    void fileSelected( const KFileItem & /*file*/ );

    void slotPrepareMoveTracks();
    void slotPrepareCopyTracks();
    void slotMoveTracks( const Meta::TrackList& tracks );
    void slotCopyTracks( const Meta::TrackList& tracks );
    void slotPlayChildTracks();
    void slotAppendChildTracks();
    void slotEditTracks();

protected:
    bool eventFilter( QObject *, QEvent * );

private:
    QList<QAction*> createBasicActions();
    void playChildTracks( const KFileItemList &items, Playlist::AddOptions insertMode );
    Meta::TrackList tracksForEdit() const;

    bool m_copyActivated;
    bool m_moveActivated;
    CollectionAction* m_copyAction;
    CollectionAction* m_moveAction;
};

#endif

