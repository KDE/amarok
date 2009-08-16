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

#ifndef PLAYLISTGROUPINGACTION_H
#define PLAYLISTGROUPINGACTION_H

#include <KAction>
#include <KMenu>

namespace Playlist
{

/**
 * Action used to show a menu for selecting the grouping category.
 * @author Téo Mrnjavac <teo.mrnjavac@gmail.com>
 */
class GroupingAction : public KAction
{
    Q_OBJECT
public:
    /**
     * Constructor.
     * @param parent the parent QWidget
     */
    GroupingAction( QWidget *parent = 0 );

    /**
     * Destructor.
     */
    ~GroupingAction();

    /**
     * Accessor for the internal name of the current grouping category.
     * @return the name of the category.
     */
    QString currentGroupingCategory() const { return m_groupingActions->checkedAction()->data().toString(); }

    /**
     * Accessor for the pretty name of the current grouping category.
     * @return the visible name of the category.
     */
    QString prettyGroupingCategory() const { return m_groupingActions->checkedAction()->text(); }

    /**
     * Returns the QActionGroup that contains the grouping categories.
     * @return the QActionGroup.
     */
    QActionGroup *groupingActionGroup() const { return m_groupingActions; }

protected slots:
    /**
     * Applies a grouping category to the playlist.
     * @param the action that corresponds to the requested grouping category.
     */
    void setGrouping( QAction *groupingAction );

private:
    QActionGroup *m_groupingActions;
    KMenu *m_groupingMenu;
};

}   //namespace Playlist

#endif  //PLAYLISTGROUPINGACTION_H
