/****************************************************************************************
 * Copyright (c) 2010 Bart Cerneels <bart.cerneels@kde.org                              *
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

#ifndef OPMLDIRECTORYMODEL_H
#define OPMLDIRECTORYMODEL_H

#include "OpmlOutline.h"

#include <KUrl>

#include <QAbstractItemModel>

class OpmlParser;

class OpmlDirectoryModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit OpmlDirectoryModel( KUrl outlineUrl, QObject *parent = 0 );
    ~OpmlDirectoryModel();

    virtual QModelIndex index( int row, int column, const QModelIndex &parent = QModelIndex() ) const;
    virtual QModelIndex parent( const QModelIndex &index ) const;
    virtual int rowCount( const QModelIndex &parent = QModelIndex() ) const;
    virtual bool hasChildren( const QModelIndex &parent = QModelIndex() ) const;
    virtual int columnCount( const QModelIndex &parent = QModelIndex() ) const;
    virtual QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const;

signals:

public slots:

protected:
    virtual bool canFetchMore( const QModelIndex &parent ) const;
    virtual void fetchMore( const QModelIndex &parent );

private slots:
    void slotOpmlOutlineParsed( OpmlOutline * );
    void slotOpmlParsingDone();

private:
    KUrl m_rootOpmlUrl;
    QList<OpmlOutline *> m_rootOutlines;

    QMap<OpmlParser *,QModelIndex> m_currentFetchingMap;
};

#endif // OPMLDIRECTORYMODEL_H
