/****************************************************************************************
 * Copyright (c) 2008 Soren Harward <stharward@gmail.com>                               *
 * Copyright (c) 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                             *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef PRETTYLISTVIEW_H
#define PRETTYLISTVIEW_H

#include "EngineObserver.h"
#include "playlist/proxymodels/GroupingProxy.h"

#include <QListView>
#include <QModelIndex>
#include <QPersistentModelIndex>
#include <QRect>

#include <QAction>

class PopupDropper;
class QContextMenuEvent;
class QDragLeaveEvent;
class QDragMoveEvent;
class QDropEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class QTimer;

namespace Playlist
{
class PrettyListView : public QListView, public EngineObserver
{
    Q_OBJECT

public:
    PrettyListView( QWidget* parent = 0 );
    ~PrettyListView();

signals:
    void found();
    void notFound();

    // these slots are used by the ContextMenu
public slots:
    void editTrackInformation();
    void playFirstSelected();
    void dequeueSelection();
    void queueSelection();
    void removeSelection();
    void stopAfterTrack();
    void scrollToActiveTrack();
    void selectSource();

    void find( const QString & searchTerm, int fields, bool filter );
    void findNext( const QString & searchTerm, int fields  );
    void findPrevious( const QString & searchTerm, int fields  );
    void clearSearchTerm();
    void showOnlyMatches( bool onlyMatches );

    void itemsAdded( const QModelIndex& parent, int firstRow, int lastRow );

    virtual void engineNewTrackPlaying(); // from EngineObserver

protected slots:
    void newPalette( const QPalette & palette );

private slots:
    void trackActivated( const QModelIndex& );
    void updateProxyTimeout();
    void fixInvisible(); // Workaround for BUG 184714; see implementation for more comments.
    void redrawActive();

private:
    void showEvent( QShowEvent* );
    void contextMenuEvent( QContextMenuEvent* );
    void dragLeaveEvent( QDragLeaveEvent* );
    void dragMoveEvent( QDragMoveEvent* );
    void dropEvent( QDropEvent* );
    void keyPressEvent( QKeyEvent* );
    void mousePressEvent( QMouseEvent* );
    void mouseReleaseEvent( QMouseEvent* );
    void paintEvent( QPaintEvent* );
    void startDrag( Qt::DropActions supportedActions );


    bool mouseEventInHeader( const QMouseEvent* ) const;
    QItemSelectionModel::SelectionFlags headerPressSelectionCommand( const QModelIndex&, const QMouseEvent* ) const;
    QItemSelectionModel::SelectionFlags headerReleaseSelectionCommand( const QModelIndex&, const QMouseEvent* ) const;

    void startProxyUpdateTimeout();

    QRect                 m_dropIndicator;
    QPersistentModelIndex m_headerPressIndex;
    bool                  m_mousePressInHeader;
    bool                  m_skipAutoScroll;

    QTimer       *m_proxyUpdateTimer;
    PopupDropper *m_pd;

    AbstractModel *m_topmostProxy;

public:
    QList<int> selectedRows() const;
};
}
#endif
