/***************************************************************************
 *   Copyright (c) 2009  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
 
#ifndef PLAYLISTITEMEDITWIDGET_H
#define PLAYLISTITEMEDITWIDGET_H

#include "FilenameLayoutWidget.h"
#include "playlist/view/listview/PrettyItemConfig.h"

#include <KVBox>

class KHBox;
class QCheckBox;
class QSpinBox;

/**
A widget to define the layout of a single type of playlist item ( head, body or single )

	@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class PlaylistItemEditWidget : public KVBox
{
    Q_OBJECT
public:
    PlaylistItemEditWidget( QWidget *parent = 0 );

    ~PlaylistItemEditWidget();

    void readLayout( Playlist::PrettyItemConfig config );
    Playlist::PrettyItemConfig config();

private slots:

    void numberOfRowsChanged( int noOfRows );

private:

    KVBox * m_rowsBox;
    QSpinBox * m_noOfRowsSpinBox;
    QCheckBox * m_showCoverCheckBox;

    QMap<int, FilenameLayoutWidget*> m_rowMap;
};

#endif
