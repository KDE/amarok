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

#include "QueueModel.h"

#include "AmarokMimeData.h"
#include "debug.h"
#include "Meta.h"
#include "TheInstances.h"

using namespace QueueManagerNS;
using namespace Meta;

Model *Model::s_instance = 0;

Model::Model( QObject* parent )
    : QAbstractListModel( parent )
{
    s_instance = this;
}

QStringList
Model::mimeTypes() const
{
    QStringList ret = QAbstractListModel::mimeTypes();
    ret << AmarokMimeData::TRACK_MIME;
    debug() << ret;
    return ret;
}

Qt::DropActions
Model::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

void
Model::clear()
{
    removeRows( 0, m_tracks.size() );
}

int
Model::rowCount( const QModelIndex& ) const
{
    return m_tracks.size();
}

QVariant
Model::data( const QModelIndex& index, int role ) const
{
    int row        = index.row();
    TrackPtr track = m_tracks.at( row );

    if( row == -1 )
        return QVariant();

//     if( role == TrackRole && track )
//         return QVariant::fromValue( track );

    else if( role == Qt::DisplayRole )
        return track->name();

    return QVariant();
}

bool
Model::removeRows( int position, int rows, const QModelIndex& /*parent*/  )
{
    Q_UNUSED( position ); Q_UNUSED( rows );
    // do something
    return true;
}

namespace The {
    QueueManagerNS::Model* queueModel() { return QueueManagerNS::Model::s_instance; }
}
