/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
 
#ifndef PLAYLISTLAYOUTCONFIGWIDGET_H
#define PLAYLISTLAYOUTCONFIGWIDGET_H

#include <KVBox>

class PlaylistLayoutEditDialog;
class QComboBox;

namespace Playlist {

/**
    A widget containing the gui needed to define playlist layouts

    @author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
 */

class LayoutConfigWidget : public KVBox
{
    Q_OBJECT
public:
    LayoutConfigWidget( QWidget * parent );
    ~LayoutConfigWidget();

private slots:

    void setActiveLayout( const QString &layout );
    void layoutListChanged();
    void editItem( const QString &itemName );
    void deleteItem( const QString &itemName );

private:
    PlaylistLayoutEditDialog * m_playlistEditDialog;

    QComboBox *m_comboBox;

};

}

#endif
