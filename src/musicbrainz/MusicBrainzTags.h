/****************************************************************************************
 * Copyright (c) 2010 Sergey Ivanov <123kash@gmail.com>                                 *
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

#ifndef MUSICBRAINZTAGSMODEL_H
#define MUSICBRAINZTAGSMODEL_H

#include <KLocalizedString>
#include <core/meta/Meta.h>
#include <QAbstractItemModel>
#include <QReadWriteLock>
#include <QItemDelegate>
#include <QTreeView>

class MusciBrainzTagsItem
{
    public:
        MusciBrainzTagsItem( MusciBrainzTagsItem *parent = 0,
                             const Meta::TrackPtr track = Meta::TrackPtr(),
                             const QVariantMap tags = QVariantMap() );

        ~MusciBrainzTagsItem();

        MusciBrainzTagsItem *parent() const;
        MusciBrainzTagsItem *child( const int row ) const;
        void appendChild( MusciBrainzTagsItem *child );
        int childCount() const;
        int row() const;

        Qt::ItemFlags flags() const;

        Meta::TrackPtr track() const;
        QVariant data( int column ) const;
        QVariantMap data() const;
        QVariant dataValue( const QString &key ) const;
        bool dataContains( const QString &key );
        void setData( QVariantMap tags );

        MusciBrainzTagsItem *checkedItem() const;
        void checkFirst();
        void uncheckAll();
        bool checked() const;
        void setChecked( bool checked );

    private:
        void setDataValue( const QString &key, const QVariant &value );
        void setParent( MusciBrainzTagsItem *parent );
        MusciBrainzTagsItem *m_parent;
        QList< MusciBrainzTagsItem * > m_childItems;

        Meta::TrackPtr m_track;
        QVariantMap m_data;
        bool m_checked;

        mutable QReadWriteLock m_dataLock;
        mutable QReadWriteLock m_childrenLock;
        mutable QReadWriteLock m_parentLock;
};

class MusicBrainzTagsModel : public QAbstractItemModel
{
    Q_OBJECT

    public:
        MusicBrainzTagsModel( QObject *parent = 0 );
        ~MusicBrainzTagsModel();

        QModelIndex index( int row, int column, const QModelIndex &parent = QModelIndex() ) const;
        QModelIndex parent( const QModelIndex &index ) const;

        Qt::ItemFlags flags( const QModelIndex &index ) const;
        QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const;
        bool setData( const QModelIndex &index, const QVariant &value, int role );
        QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

        int rowCount( const QModelIndex &parent = QModelIndex() ) const;
        int columnCount( const QModelIndex &parent = QModelIndex() ) const;

        QMap < Meta::TrackPtr, QVariantMap > getAllChecked(); 

    public slots:
        void addTrack( const Meta::TrackPtr track, const QVariantMap tags );
        void selectAll( int section );

    private:
        MusciBrainzTagsItem *m_rootItem;
};

class MusicBrainzTagsModelDelegate : public QItemDelegate
{
    public:
        explicit MusicBrainzTagsModelDelegate( QObject *parent = 0 );
    protected:
        virtual void drawCheck( QPainter *painter, const QStyleOptionViewItem &option,
                                const QRect &rect, Qt::CheckState state ) const;
};

class MusicBrainzTagsView : public QTreeView
{
    Q_OBJECT
    public:
        explicit MusicBrainzTagsView( QWidget *parent = 0 );

        ~MusicBrainzTagsView();

    protected:
        virtual void contextMenuEvent( QContextMenuEvent *event );

    private slots:
        void openArtistPage();
        void openReleasePage();
        void openTrackPage();

    private:
        QIcon *artistIcon;
        QIcon *releaseIcon;
        QIcon *trackIcon;
};
#endif // MUSICBRAINZTAGSMODEL_H
