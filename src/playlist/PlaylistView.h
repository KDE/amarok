/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#ifndef AMAROK_PLAYLISTVIEW_H
#define AMAROK_PLAYLISTVIEW_H

#include <QTableView>
class QAbstractItemModel;
/**
 * The view of the playlist, used to send user interaction signals back to the model.
 */
namespace PlaylistNS {
    class View : public QTableView
    {
        Q_OBJECT
        public:
            View( QWidget* parent ) : QTableView( parent ) { }
            virtual void setModel( QAbstractItemModel * model );
    };
}
#endif
