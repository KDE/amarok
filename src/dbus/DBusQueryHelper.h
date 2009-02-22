/*
 *  Copyright 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>
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
 
#ifndef DBUSQUERYHELPER_H
#define DBUSQUERYHELPER_H

#include "meta/Meta.h"

#include<QList>
#include<QMap>
#include<QObject>
#include<QString>

class QueryMaker;

class DBusQueryHelper : public QObject
{
    Q_OBJECT
    
    public:
        DBusQueryHelper( QObject *parent, QueryMaker *qm, const QString &token );
        
    private slots:
        void slotResultReady( const QString &collectionId, const Meta::TrackList &tracks );
        
        void slotQueryDone();
    
    signals:
        void queryResult( const QString &token, const QList<QMap<QString, QVariant> > &result );
        
    private:
        QString m_token;
        QList<QMap<QString, QVariant> > m_result;
};

#endif
