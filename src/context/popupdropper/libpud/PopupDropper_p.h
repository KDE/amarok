/***************************************************************************
 *   Copyright (c) 2008  Jeff Mitchell <mitchell@kde.org>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef POPUPDROPPER_P_H
#define POPUPDROPPER_P_H

#include <QList>
#include <QMap>
#include <QObject>
#include <QRectF>
#include <QTimeLine>
#include <QTimer>

#include "PopupDropper.h"
#include "PopupDropperView.h"

class QSvgRenderer;
class QWidget;

class PopupDropperPrivate : public QObject
{
    Q_OBJECT

public:
    PopupDropperPrivate( PopupDropper* parent, bool sa, QWidget* widget );
    ~PopupDropperPrivate();

    void newSceneView( PopupDropper* pud );
    void setParent( QObject* parent );

    bool standalone;
    QWidget* widget;
    QGraphicsScene* scene;
    PopupDropperView* view;
    PopupDropper::Fading fade;
    QTimeLine fadeHideTimer;
    QTimeLine fadeShowTimer;
    int fadeInTime;
    int fadeOutTime;
    QTimer deleteTimer;
    int deleteTimeout;
    int frameMax;
    QColor windowColor;
    QBrush windowBackgroundBrush;
    QColor baseTextColor;
    QColor hoveredTextColor;
    QPen hoveredBorderPen;
    QBrush hoveredFillBrush;
    QString file;
    QSvgRenderer* sharedRenderer;
    int horizontalOffset;
    QList<PopupDropperItem*> pdiItems;
    int overlayLevel;
    bool entered;
    QMap<QAction*, PopupDropperPrivate*> submenuMap;
    bool submenu;
    QList<QGraphicsItem*> allItems;
    bool quitOnDragLeave;
    bool onTop;
    QRectF widgetRect;

    //queuedHide: To prevent multiple hide() from being sent if it's already being hidden
    bool queuedHide;

    void dragLeft();
    void dragEntered();
    void startDeleteTimer();

    void reposItems();
    bool amIOnTop( PopupDropperView* pdv );
 
private slots:
    void fadeHideTimerFrameChanged( int frame );
    void fadeShowTimerFrameChanged( int frame );
    void fadeShowTimerFinished();
    void fadeHideTimerFinished();
    void deleteTimerFinished();

private:
    PopupDropper* q;
};

#endif //POPUPDROPPER_P_H
