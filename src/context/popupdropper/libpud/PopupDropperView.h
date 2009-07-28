/****************************************************************************************
 * Copyright (c) 2008 Jeff Mitchell <mitchell@kde.org>                                  *
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

#ifndef POPUPDROPPER_VIEW_H
#define POPUPDROPPER_VIEW_H

#include <QGraphicsView>

class PopupDropper;
class PopupDropperViewPrivate;

class PopupDropperView : public QGraphicsView
{
    Q_OBJECT

public:
    PopupDropperView( PopupDropper *pd, QGraphicsScene *scene, QWidget *parent );
    ~PopupDropperView();

    void dropEvent( QDropEvent *event );
    void dragEnterEvent( QDragEnterEvent *event );
    void dragMoveEvent( QDragMoveEvent *event );
    void dragLeaveEvent( QDragLeaveEvent *event );

    void resetView();

    void deactivateHover();

    bool entered() const;
    void setEntered( bool entered );
    
private:
    friend class PopupDropperViewPrivate;
    PopupDropperViewPrivate* const d;
};

#endif /* AMAROK_POPUPDROPPER_VIEW_H */

