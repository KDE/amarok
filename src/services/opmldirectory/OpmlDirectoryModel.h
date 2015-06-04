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

#include <QUrl>

#include <QAbstractItemModel>

class OpmlParser;

class QAction;
typedef QList<QAction *> QActionList;

class OpmlDirectoryModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    //TODO: make these rols part of a common class in Amarok::.
    enum
    {
        ActionRole = Qt::UserRole, //list of QActions for the index
        DecorationUriRole, //a URI for the decoration to be fetched by the view.
        CustomRoleOffset //first role that can be used by sublasses for their own data
    };

    explicit OpmlDirectoryModel( QUrl outlineUrl, QObject *parent = 0 );
    ~OpmlDirectoryModel();

    // QAbstractItemModel methods
    virtual QModelIndex index( int row, int column, const QModelIndex &parent = QModelIndex() ) const;
    virtual Qt::ItemFlags flags( const QModelIndex &index ) const;
    virtual QModelIndex parent( const QModelIndex &index ) const;
    virtual int rowCount( const QModelIndex &parent = QModelIndex() ) const;
    virtual bool hasChildren( const QModelIndex &parent = QModelIndex() ) const;
    virtual int columnCount( const QModelIndex &parent = QModelIndex() ) const;
    virtual QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const;
    virtual bool setData( const QModelIndex &index, const QVariant &value, int role = Qt::EditRole );
    virtual bool removeRows( int row, int count, const QModelIndex &parent = QModelIndex() );

    // OpmlDirectoryModel methods
    virtual void saveOpml( const QUrl &saveLocation );
    virtual OpmlNodeType opmlNodeType( const QModelIndex &idx ) const;

    //TODO: extract these into OpmlPodcastDirectoryModel subclass
    void subscribe( const QModelIndexList &indexes ) const;

Q_SIGNALS:

public Q_SLOTS:
    void slotAddOpmlAction();
    void slotAddFolderAction();

protected:
    virtual bool canFetchMore( const QModelIndex &parent ) const;
    virtual void fetchMore( const QModelIndex &parent );

private Q_SLOTS:
    void slotOpmlHeaderDone();
    void slotOpmlOutlineParsed( OpmlOutline * );
    void slotOpmlParsingDone();
    void slotOpmlWriterDone( int result );

private:
    QModelIndex addOutlineToModel( QModelIndex parentIdx, OpmlOutline *oultine );

    QUrl m_rootOpmlUrl;
    QList<OpmlOutline *> m_rootOutlines;

    QMap<OpmlParser *,QModelIndex> m_currentFetchingMap;
    QMap<OpmlOutline *,QPixmap> m_imageMap;

    QAction *m_addOpmlAction;
    QAction *m_addFolderAction;
};

Q_DECLARE_METATYPE(QActionList)
//we store these in a QVariant for the addFolder and addOpml actions
Q_DECLARE_METATYPE( QModelIndex )

#endif // OPMLDIRECTORYMODEL_H
