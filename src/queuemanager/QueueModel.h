/***************************************************************************
 * copyright            : (C) 2007 Seb Ruiz <ruiz@kde.org>                 *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_QUEUEMODEL_H
#define AMAROK_QUEUEMODEL_H

#include "meta.h"

#include <QAbstractListModel>

namespace QueueManagerNS
{
    class Model : public QAbstractListModel
    {
        Q_OBJECT

        public:
            Model( QObject *parent = 0 );
            ~Model() { }

            int      rowCount( const QModelIndex &parent = QModelIndex() )    const;
            int      columnCount( const QModelIndex &parent = QModelIndex() ) const { Q_UNUSED( parent ); return 1; }
            bool     removeRows( int row, int count, const QModelIndex &parent = QModelIndex() );
            QVariant data( const QModelIndex &index, int role ) const;

            // Drag and Drop
            QStringList     mimeTypes() const;
            Qt::DropActions supportedDropActions() const;

            static Model *s_instance;

        public slots:
            void clear();

        private:
            Meta::TrackList m_tracks;   // list of tracks in order of desired queue
    };
}

#endif /* AMAROK_QUEUEMODEL_H */
