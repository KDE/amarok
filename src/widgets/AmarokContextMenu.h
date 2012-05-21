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

protected:
    // reimplemented to show tooltips and respond to keyboard modifiers
    virtual bool event( QEvent *e );

};

} //namespace Amarok

#endif // AMAROKCONTEXTMENU_H
