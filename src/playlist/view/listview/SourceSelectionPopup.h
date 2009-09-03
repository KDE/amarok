/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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
 
#ifndef PLAYLISTSOURCESELECTIONPOPUP_H
#define PLAYLISTSOURCESELECTIONPOPUP_H

#include <QDialog>

#include "meta/capabilities/MultiSourceCapability.h"

#include <QStringList>

class QListWidget;
class QListWidgetItem;

namespace Playlist {

/**
A small popup to let the user choose between the sources for tracks or streams that might have several.

	@author 
*/
class SourceSelectionPopup : public QDialog
{
    Q_OBJECT
public:
    SourceSelectionPopup( QWidget * parent, Meta::MultiSourceCapability * msc );

    ~SourceSelectionPopup();

signals:
    void sourceChanged( int source );

protected slots:
    void sourceSelected( QListWidgetItem * item );

private:
    QListWidget *m_listWidget;
    Meta::MultiSourceCapability * m_msc;
};

}

#endif
