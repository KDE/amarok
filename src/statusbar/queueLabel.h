/***************************************************************************
 *   Copyright (C) 2005 by Gábor Lehel <illissius@gmail.com>               *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#ifndef AMAROK_QUEUELABEL_H
#define AMAROK_QUEUELABEL_H

#include <qlabel.h>
#include "playlistitem.h"

class QueueLabel: public QLabel //homonym, heh heh
{
    Q_OBJECT

    public:
        QueueLabel( QWidget *parent, const char *name = 0 );

    signals:
        void queueChanged( const PLItemList &, const PLItemList & );

    public slots:
        virtual void update();

        virtual void setNum( int num );

    protected:
        virtual void mousePressEvent( QMouseEvent* e );

    private:
        QString veryNiceTitle( PlaylistItem * item ) const;
};

#endif
