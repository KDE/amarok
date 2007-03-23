 /*
  Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>

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

#include "debug.h"
#include "servicemodelbase.h"
#include "servicemodelitembase.h"

#include "kurl.h"

#include <QMimeData>

ServiceModelBase::ServiceModelBase( QObject *parent ) 
     : QAbstractItemModel(parent)
{

    setSupportedDragActions( Qt::CopyAction );
}

void ServiceModelBase::resetModel() {
   reset();
}

Qt::ItemFlags ServiceModelBase::flags ( const QModelIndex & index ) const {

    //By default all items are dragable. Override in subclass if not happy with this... :-)
       if (!index.isValid())
        return Qt::ItemIsEnabled;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
}

QMimeData* ServiceModelBase::mimeData( const QModelIndexList &indices ) const {
    if ( indices.isEmpty() )
        return 0;

    KUrl::List urls;

    foreach( QModelIndex index, indices ) {
        if (index.isValid()) {
            ServiceModelItemBase *item = static_cast<ServiceModelItemBase*>(index.internalPointer());
            //debug() << "adding url: " << item->getUrls() << endl;
            urls += item->getUrls();
        }
    }

    QMimeData *mimeData = new QMimeData();
    urls.populateMimeData(mimeData);

    return mimeData;
}


#include "servicemodelbase.moc"
