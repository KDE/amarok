/* 
   Copyright (C) 2008 Jeff Mitchell <mitchell@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef POPUPDROPPER_P_H
#define POPUPDROPPER_P_H

#include <QList>
#include <QObject>

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
    bool closeAtEndOfFade;
    int frameMax;
    QColor windowColor;
    QColor textColor;
    QString file;
    QSvgRenderer* sharedRenderer;
    int itemCount;
    int totalItems;
    QList<PopupDropperItem*> pdiItems;

private slots:
    void timerFrameChanged( int frame );
    void timerFinished();

private:
    PopupDropper* q;
};

#endif //POPUPDROPPER_P_H
