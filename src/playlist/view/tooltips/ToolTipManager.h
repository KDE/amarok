/*******************************************************************************
 *   Copyright (C) 2008 by Konstantin Heil <konst.heil@stud.uni-heidelberg.de> *
 *   Copyright (C) 2009-2010 Oleksandr Khayrullin <saniokh@gmail.com>          *
 *                                                                             *
 *   This program is free software; you can redistribute it and/or modify      *
 *   it under the terms of the GNU General Public License as published by      *
 *   the Free Software Foundation; either version 2 of the License, or         *
 *   (at your option) any later version.                                       *
 *                                                                             *
 *   This program is distributed in the hope that it will be useful,           *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 *   GNU General Public License for more details.                              *
 *                                                                             *
 *   You should have received a copy of the GNU General Public License         *
 *   along with this program; if not, write to the                             *
 *   Free Software Foundation, Inc.,                                           *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA                *
 *******************************************************************************/

#ifndef TOOLTIPMANAGER_H
#define TOOLTIPMANAGER_H

#include <QObject>
#include <QRect>
#include <QIcon>
#include <QModelIndex>
#include "meta/Meta.h"
#include "AmarokToolTip.h"
#include "playlist/PlaylistDefines.h"

class QAbstractItemView;
class QModelIndex;
class QTimer;
class KToolTipItem;

/**
 * @brief Manages the tooltips for an item view.
 *
 * When hovering an item, a tooltip is shown after
 * a short timeout. The tooltip is hidden again when the
 * viewport is hovered or the item view has been left.
 */
class ToolTipManager : public QObject
{
    Q_OBJECT

public:
    explicit ToolTipManager(QAbstractItemView* parent);
    virtual ~ToolTipManager();

    /**
    * Exclude the field from being shown on the tooltip because it's already on the playlist
    * @param column The column to be excluded
    * @param single If ON, the item is excluded from the tooltip for a Single item (not Head or Body)
    */
    void excludeField( const Playlist::Column& column, bool single );

    /**
    * Exclude the album cover from being shown on the tooltip because it's already on the playlist
    * @param single If ON, the item is excluded from the tooltip for a Single item (not Head or Body)
    */
    void excludeCover( bool single );

    /**
    * Cancel all exclusions
    */
    void cancelExclusions();

public slots:
    /**
     * Hides the currently shown tooltip. Invoking this method is
     * only needed when the tooltip should be hidden although
     * an item is hovered.
     */
    void hideTip();

protected:
    virtual bool eventFilter(QObject* watched, QEvent* event);

private slots:
    void requestToolTip(const QModelIndex& index);
    void hideToolTip();
    void prepareToolTip();

private:
    void showToolTip(const QIcon& icon, const QString& text);

    QAbstractItemView* m_view;

    QTimer* m_timer;
    QRect m_itemRect;

    Meta::TrackPtr m_track;

    bool m_excludes[Playlist::NUM_COLUMNS];
    bool m_excludes_single[Playlist::NUM_COLUMNS];
    bool m_singleItem;

    AmarokBalloonTooltipDelegate * g_delegate;

    /**
     * Prepares a row for the playlist tooltips consisting of an icon representing
     * an mp3 tag and its value
     * @param column The column used to display the icon
     * @param value The QString value to be shown
     * @param force Set to TRUE to always return a line
     * @return The line to be shown or an empty QString if the value is null
     */
    QString HTMLLine( const Playlist::Column& column, const QString& value, bool force = false );

    /**
     * Prepares a row for the playlist tooltips consisting of an icon representing
     * an mp3 tag and its value
     * @param column The column used to display the icon
     * @param value The integer value to be shown
     * @param force Set to TRUE to always return a line
     * @return The line to be shown or an empty QString if the value is 0
     */
    QString HTMLLine( const Playlist::Column& column, const int value, bool force = false );
    /**
     * Inserts "<br>" tags in long lines of text so that it doesn't become too long
     * @param text The text in HTML to process
     * @return The processed text
     */
    QString breakLongLinesHTML(const QString& text);

    /**
     * Returns TRUE if a data of a column should be shown
     * Makes it easier to determine considering the Single items don't share
     * configuration with Head or Body items
     * @param column The column in question
     * @return TRUE if the line should be shown
     */
    bool isVisible( const Playlist::Column& column );
};

#endif

