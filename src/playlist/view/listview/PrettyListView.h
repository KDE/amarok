/****************************************************************************************
 * Copyright (c) 2008 Soren Harward <stharward@gmail.com>                               *
 * Copyright (c) 2009 Téo Mrnjavac <teo@kde.org>                                        *
 * Copyright (c) 2009 Oleksandr Khayrullin <saniokh@gmail.com>                          *
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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef PRETTYLISTVIEW_H
#define PRETTYLISTVIEW_H

#include "PrettyItemDelegate.h"
#include "playlist/proxymodels/GroupingProxy.h"
#include "playlist/view/PlaylistViewCommon.h"

#include <QListView>
#include <QModelIndex>
#include <QPersistentModelIndex>
#include <QRect>

#include <QDateTime>
#include <QTimer>

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
class PrettyListView : public QListView, public ViewCommon
{
    Q_OBJECT

public:
    explicit PrettyListView( QWidget* parent = nullptr );
    ~PrettyListView() override;

protected:
    int verticalOffset() const override;

Q_SIGNALS:
    void found();
    void notFound();

    // these slots are used by the ContextMenu
public Q_SLOTS:
    void editTrackInformation();
    void playFirstSelected();
    void dequeueSelection();
    void queueSelection();

    /* Switch queue state for selected rows in playlist */
    void switchQueueState();

    void removeSelection();
    void stopAfterTrack();
    void scrollToActiveTrack();
    void selectSource();

    void downOneTrack();
    void upOneTrack();

    // Workaround for BUG 222961 and BUG 229240: see implementation for more comments.
    void setCurrentIndex( const QModelIndex &index );
    void selectionModel_setCurrentIndex( const QModelIndex &index, QItemSelectionModel::SelectionFlags command );    // Never call selectionModel()->setCurrentIndex() directly!

    void find( const QString & searchTerm, int fields, bool filter );
    void findNext( const QString & searchTerm, int fields  );
    void findPrevious( const QString & searchTerm, int fields  );
    void clearSearchTerm();
    void showOnlyMatches( bool onlyMatches );
    void findInSource();

protected:
    void showEvent( QShowEvent* ) override;
    void contextMenuEvent( QContextMenuEvent* ) override;
    void dragEnterEvent( QDragEnterEvent *event ) override;
    void dragLeaveEvent( QDragLeaveEvent* ) override;
    void dragMoveEvent( QDragMoveEvent* ) override;
    void dropEvent( QDropEvent* ) override;
    void keyPressEvent( QKeyEvent* ) override;
    void mousePressEvent( QMouseEvent* ) override;
    void mouseReleaseEvent( QMouseEvent* ) override;

    /** Draws a "drop here" text if empty */
    void paintEvent( QPaintEvent* ) override;

    void startDrag( Qt::DropActions supportedActions ) override;
    bool edit( const QModelIndex &index, EditTrigger trigger, QEvent *event ) override;

protected Q_SLOTS:
    void newPalette( const QPalette & palette );

private Q_SLOTS:
    void slotPlaylistActiveTrackChanged();
    void bottomModelRowsInserted( const QModelIndex& parent, int start, int end );
    void bottomModelRowsInsertedScroll();
    void moveTrackSelection( int offset );

    void slotSelectionChanged();
    void trackActivated( const QModelIndex& );
    void updateProxyTimeout();
    void fixInvisible(); // Workaround for BUG 184714; see implementation for more comments.
    void redrawActive();
    void playlistLayoutChanged();

private:
    bool mouseEventInHeader( const QMouseEvent* ) const;
    QItemSelectionModel::SelectionFlags headerPressSelectionCommand( const QModelIndex&, const QMouseEvent* ) const;
    QItemSelectionModel::SelectionFlags headerReleaseSelectionCommand( const QModelIndex&, const QMouseEvent* ) const;

    void startProxyUpdateTimeout();

    QRect                 m_dropIndicator;
    QPersistentModelIndex m_headerPressIndex;
    bool                  m_mousePressInHeader;

    bool                  m_skipAutoScroll;
    bool                  m_firstScrollToActiveTrack;
    quint64               m_rowsInsertedScrollItem;

    QString               m_searchTerm;
    int                   m_fields;
    bool                  m_filter;

    bool    m_showOnlyMatches;

    QTimer       *m_proxyUpdateTimer;
    PopupDropper *m_pd;

    PrettyItemDelegate * m_prettyDelegate;

    QTimer *m_animationTimer;
    QDateTime m_lastTimeSelectionChanged; // we want to prevent a click to change the selection and open the editor (BR 220818)

public:
    QList<int> selectedRows() const;
};
}
#endif
