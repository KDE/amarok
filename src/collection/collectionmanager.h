/*
 *  Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#ifndef AMAROK_COLLECTIONMANAGER_H
#define AMAROK_COLLECTIONMANAGER_H

#include "collection.h"
#include "meta.h"
#include "querymaker.h"

#include <QList>
#include <QObject>

class CollectionManager : public QObject
{
    Q_OBJECT

    public:
        static CollectionManager * instance();

        ~CollectionManager();

        QueryMaker * queryMaker();
        void startFullScan();

        QList<Collection*> collections();

        /**
            This method will try to get a Track object for the given url. This method will return 0 if no Track object
            could be created for the url.
        */
        Meta::TrackPtr trackForUrl( const KUrl &url );

        QStringList sqlQuery( const QString &query );
        int sqlInsert( const QString &statement, const QString &table ); //TODO move to interface

        void addUnmanagedCollection( Collection *newCollection );
        void removeUnmanagedCollection( Collection *collection );

    signals:
        void collectionAdded( Collection *newCollection );
        void collectionRemoved( QString collectionId );

    private slots:
        void slotNewCollection( Collection *newCollection );
        void slotRemoveCollection();

    private:
        CollectionManager();

        void init();

        class Private;
        Private * const d;
};

#endif /* AMAROK_COLLECTIONMANAGER_H */
