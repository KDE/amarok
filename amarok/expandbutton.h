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

#include <qpushbutton.h>
#include <qptrlist.h>

class QWidget;
class QString;
class QMouseEvent;
class QPixmap;
class QTimer;

class PlayerApp;
extern PlayerApp *pApp;

/**
 *@author mark
 */

class ExpandButton : public QPushButton
{
    Q_OBJECT
        public:
        ExpandButton( const QString &text, QWidget *parent=0 );
        ExpandButton( const QString &text, ExpandButton *parent=0 );

        ~ExpandButton();

        void mouseReleaseEvent( QMouseEvent *e );
        void mouseMoveEvent( QMouseEvent *e );

// ATTRIBUTES ------
        QPtrList<ExpandButton> m_ButtonList;

    public slots:
        void slotStartExpand();
        void slotAnimTimer();

    private:
        enum AnimPhase { ANIM_IDLE, ANIM_EXPAND, ANIM_SHOW, ANIM_SHRINK };
        AnimPhase m_animFlag;

        int m_animHeight;
        float m_animAdd, m_animSpeed;

        QTimer *m_pTimer;
        QPixmap *m_pSavePixmap, *m_pComposePixmap;
        QPixmap *m_pBlitMap1, *m_pBlitMap2;
};
#endif
