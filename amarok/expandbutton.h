/***************************************************************************
                          expandbutton.h  -  description
                             -------------------
    begin                : Die Feb 4 2003
    copyright            : (C) 2003 by Mark Kretschmann
    email                :
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef EXPANDBUTTON_H
#define EXPANDBUTTON_H

#include <qpushbutton.h> //baseclass
#include <qptrlist.h>    //m_ButtonList

class QMouseEvent;
class QPixmap;
class QString;
class QTimerEvent;
class QWidget;

class ExpandButton : public QPushButton
{
    Q_OBJECT

    public:
        ExpandButton( const QString&, QWidget * = 0, QObject* = 0, const char* = 0 );
        ExpandButton( const QString&, ExpandButton * = 0, QObject* = 0, const char* = 0 );

    private slots:
        void slotDelayExpand();
        void slotStartExpand();

    private:
        void drawButtonLabel( QPainter * );
        void mouseMoveEvent( QMouseEvent* );
        void mouseReleaseEvent( QMouseEvent* );
        void timerEvent( QTimerEvent* );

// ATTRIBUTES ------
        enum AnimPhase { ANIM_IDLE, ANIM_EXPAND, ANIM_SHOW, ANIM_SHRINK };
        static const float ANIM_SPEED = 0.5;
        static const int   TIMER_MS   = 20;
        
        AnimPhase m_animFlag;
        bool      m_expanded;
        QPtrList<ExpandButton> m_ButtonList;

        int     m_animHeight;
        float   m_animAdd;

        QWidget *m_pPaintWidget;
        QTimer  *m_pTimer;
        QPixmap *m_pSavePixmap;
        QPixmap *m_pComposePixmap;
        QPixmap *m_pBlitMap1;
        QPixmap *m_pBlitMap2;
};

      
#endif
