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
#include <QObject>
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

    bool standalone;
    QWidget* widget;
    QGraphicsScene scene;
    PopupDropperView view;
    bool success;
    PopupDropper::Fading fade;
    QTimeLine fadeTimer;
    int fadeInTime;
    int fadeOutTime;
    QTimer deleteTimer;
    int deleteTimeout;
    bool closeAtEndOfFade;
    int frameMax;
    QColor windowColor;
    QColor textColor;
    QString file;
    QSvgRenderer* sharedRenderer;
    int horizontalOffset;
    int itemCount;
    int totalItems;
    QList<PopupDropperItem*> pdiItems;
    int overlayLevel;
    bool entered;

    void dragLeft();
    void dragEntered();
    void startDeleteTimer();
 
private slots:
    void fadeTimerFrameChanged( int frame );
    void fadeTimerFinished();
    void deleteTimerFinished();

private:
    PopupDropper* q;
};

#endif //POPUPDROPPER_P_H
