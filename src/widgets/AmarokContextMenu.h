/****************************************************************************************
 * Copyright (c) 2012 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#ifndef AMAROKCONTEXTMENU_H
#define AMAROKCONTEXTMENU_H

#include <KMenu>

#include <QAction>

namespace Amarok
{

/** A KMenu wich dynamically responds to Keyboard modifier presses
  * example: Ctrl key is pressed and Trash action is replaced with Delete action.
  */
class ContextMenu : public KMenu
{
    Q_OBJECT
public:
    explicit ContextMenu( QWidget *parent = 0 );

    /** Sets up 2 actions of which the alternative is only shown if the trigger key is
      * pressed.
      * @param action has to be added using addAction prior to calling this function
      * @param alterative has to be added using addAction prior to calling this function
      */
    void setAlternatives( QAction *action, QAction *alterative, Qt::Key trigger );

protected:
    // reimplemented to show tooltips and respond to keyboard modifiers
    virtual bool event( QEvent *e );

    //reimplemented to toggle between action alternatives when a modifier key is used.
    virtual void keyPressEvent( QKeyEvent *e );
    virtual void keyReleaseEvent( QKeyEvent *e );

private:
    typedef QPair<QAction *, QAction *> QActionPair;

    //which actions to toggle.
    QMultiMap<int, QActionPair> m_alternativeActionMap;
};

} //namespace Amarok

#endif // AMAROKCONTEXTMENU_H
