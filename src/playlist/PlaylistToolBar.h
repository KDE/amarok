/****************************************************************************************
 * Copyright (c) 2011 Teo Mrnjavac <teo@kde.org>                                        *
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

#ifndef AMAROK_PLAYLISTTOOLBAR_H
#define AMAROK_PLAYLISTTOOLBAR_H

#include <KActionMenu>

#include <QActionGroup>
#include <QToolBar>

namespace Playlist {

/**
  * The Playlist::ToolBar class provides a toolbar with collapsible actions, which end up
  * in a menu if the toolbar is too narrow to fit all the icons in a row.
  * @author Teo Mrnjavac <teo@kde.org
  */
class ToolBar : public QToolBar
{
    Q_OBJECT
public:
    /**
      * Constructor.
      * @param parent a pointer to the parent widget.
      */
    explicit ToolBar( QWidget *parent );

    /**
      * Adds a list of actions which are either placed at the beginning of the toolbar, or
      * collapsed into a menu at the beginning of the toolbar.
      * @param actions the collapsible actions.
      */
    void addCollapsibleActions( const QActionGroup *actions );

private Q_SLOTS:
    /**
      * Sets the collapsed state of the toolbar.
      * @param collapsed true if the actions are to be collapsed, otherwise false.
      */
    void setCollapsed( bool collapsed );

    /**
      * Handles the collapsing state after an action is added.
      */
    void onActionsAdded();

protected:
    void resizeEvent( QResizeEvent *event ) override;
    void actionEvent( QActionEvent *event ) override;

private:
    /**
      * Computes the limit width, if the toolbar is smaller then the collapsible actions
      * should be collapsed.
      */
    inline int limitWidth()
    {
        int limitWidth;
        if( m_collapsed )
            limitWidth = (actions().count() + m_collapsibleActions->actions().count() -1)*27;
        else
            limitWidth = (actions().count() -1)*27;
        return limitWidth;
    }

    KActionMenu *m_playlistOperationsMenu;
    QActionGroup *m_collapsibleActions;
    QActionGroup *m_visibleActions;

    bool m_collapsed;
};

} // namespace Playlist

#endif // AMAROK_PLAYLISTTOOLBAR_H
