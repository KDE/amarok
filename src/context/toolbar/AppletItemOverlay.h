/****************************************************************************************
 * Copyright (c) 2008 Leo Franchi <lfranchi@kde.org>                                    *
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

#ifndef AMAROK_APPLET_ITEM_OVERLAY_H
#define AMAROK_APPLET_ITEM_OVERLAY_H

#include <QWidget>

class QGraphicsLinearLayout;
class QGraphicsWidget;
class QToolButton;

// NOTE inspiration and code taken from kdebase/workspace/plasma/shells/desktop/panelappletoverlay.{h,cpp}

namespace Plasma
{
    class Applet;
}

namespace Context
{
    
class Applet;
class AppletToolbarAppletItem;
    
class AppletItemOverlay : public QWidget
{
    Q_OBJECT
    
public:
    AppletItemOverlay(AppletToolbarAppletItem *applet, QGraphicsLinearLayout* layout, QWidget *parent);
    ~AppletItemOverlay();
    
    AppletToolbarAppletItem* applet();
protected:
    virtual void resizeEvent( QResizeEvent* );
    virtual void paintEvent(QPaintEvent *event);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void enterEvent(QEvent *event);
    virtual void leaveEvent(QEvent *event);
    
signals:
    void moveApplet( Plasma::Applet*, int, int );
    void deleteApplet( Plasma::Applet* );
    
private slots:
    void deleteApplet();
    void delaySyncGeometry();
    void syncGeometry();

private:
    void swapWithPrevious();
    void swapWithNext();
    
    AppletToolbarAppletItem *m_applet;
    QGraphicsWidget *m_spacer;
    QGraphicsLinearLayout *m_layout;
    QRectF m_prevGeom;
    QRectF m_nextGeom;
    QPoint m_origin;
    QToolButton* m_deleteIcon;
    int m_offset;
    int m_index;
    bool m_clickDrag;
};

}

#endif
