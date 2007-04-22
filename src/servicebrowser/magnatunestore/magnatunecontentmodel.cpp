/***************************************************************************
 *   Copyright (c) 2006, 2007                                              *
 *        Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/ 

#include "amarok.h"
#include "debug.h"
#include "magnatunecontentmodel.h"
#include "magnatunedatabasehandler.h"


MagnatuneContentModel::MagnatuneContentModel(QObject *parent, const QString &genre )
     : ServiceModelBase(parent)
{
     m_genre = genre;
     m_rootContentItem = new MagnatuneContentItem( m_genre );
     m_infoParser = new MagnatuneInfoParser();
     connect( m_infoParser, SIGNAL (info ( const QString &) ), this, SLOT( infoParsed( const QString &) ) );


    m_artistIcon = KIcon( Amarok::icon( "artist" ) );
    m_albumIcon = KIcon( Amarok::icon( "album" ) );
    m_trackIcon = KIcon( Amarok::icon( "track" ) );

     
}

MagnatuneContentModel::~MagnatuneContentModel()
{
      delete m_rootContentItem;
}

int MagnatuneContentModel::columnCount(const QModelIndex &parent) const
{

   //debug() << "MagnatuneContentModel::columnCount" << endl;


        if (parent.isValid())
         return static_cast<MagnatuneContentItem*>(parent.internalPointer())->columnCount();
     else
         return m_rootContentItem->columnCount();

}

QVariant MagnatuneContentModel::data(const QModelIndex &index, int role) const
{
    //debug() << "MagnatuneContentModel::data" << endl;
    

    if (!index.isValid())
        return QVariant();



    if (role == Qt::DisplayRole) {
        MagnatuneContentItem *item = static_cast<MagnatuneContentItem*>(index.internalPointer());
        return item->data(index.column());
    } else if ( role == Qt::DecorationRole ) {
        MagnatuneContentItem *item = static_cast<MagnatuneContentItem*>(index.internalPointer());

        if ( item->getType() == MAGNATUNE_ARTIST )
            return m_artistIcon;
        else if ( item->getType() == MAGNATUNE_ALBUM )
            return m_albumIcon;
        else if ( item->getType() == MAGNATUNE_TRACK )
            return m_trackIcon;
        else 
            return QVariant();

    } else {
        return QVariant();
    }
}

QVariant MagnatuneContentModel::headerData(int section, Qt::Orientation orientation, int role) const
{

    //debug() << "MagnatuneContentModel::headerData" << endl;

         if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
         return "Artist / Album / Track";

     return QVariant();

}

QModelIndex MagnatuneContentModel::index(int row, int column, const QModelIndex &parent) const
{

     //debug() << "MagnatuneContentModel::index, row: " << row << ", column: " << column << endl;

     MagnatuneContentItem *parentItem;

     if (!parent.isValid())
         parentItem = m_rootContentItem;
     else
         parentItem = static_cast<MagnatuneContentItem*>(parent.internalPointer());

     MagnatuneContentItem *childItem = parentItem->child(row);
     if (childItem)
         return createIndex(row, column, childItem);
     else
         return QModelIndex();


}

QModelIndex MagnatuneContentModel::parent(const QModelIndex &index) const
{


      //debug() << "MagnatuneContentModel::parent" << endl; 
      if (!index.isValid()) {
         //debug() << "MagnatuneContentModel::parent, index invalid... " << endl; 
         return QModelIndex();
     }

     MagnatuneContentItem *childItem = static_cast<MagnatuneContentItem*>(index.internalPointer());
     MagnatuneContentItem *parentItem = static_cast<MagnatuneContentItem*>(childItem->parent() );

     if (parentItem == m_rootContentItem)
         //debug() << "MagnatuneContentModel::parent, root item... " << endl; 
         return QModelIndex();

     return createIndex(parentItem->row(), 0, parentItem);


}

int MagnatuneContentModel::rowCount(const QModelIndex &parent) const
{
      // debug() << "MagnatuneContentModel::rowCount"  << endl;

      MagnatuneContentItem *parentItem;

     if (!parent.isValid())
         parentItem = m_rootContentItem;
     else
         parentItem = static_cast<MagnatuneContentItem*>(parent.internalPointer());

      //debug() << "MagnatuneContentModel::rowCount called on node: " << parentItem->data( 0 ).toString() << ", count: " << parentItem->childCount() << endl;

     return parentItem->childCount();


} 

bool MagnatuneContentModel::hasChildren ( const QModelIndex & parent ) const {

    MagnatuneContentItem* item;

     if (!parent.isValid())
         item = m_rootContentItem;
     else
         item = static_cast<MagnatuneContentItem*>(parent.internalPointer());

    return item->hasChildren();
}

void MagnatuneContentModel::setGenre( const QString &genre ) {

    m_genre = genre;
    delete m_rootContentItem;
    m_rootContentItem = new MagnatuneContentItem( m_genre );
    reset();

}


void MagnatuneContentModel::requestHtmlInfo ( const QModelIndex & index ) const {

    //debug() << "MagnatuneContentModel::requestHtmlInfo"  << endl;
    MagnatuneContentItem* item;

    if (!index.isValid()) {
        debug() << "    invalid item"  << endl;
        item = m_rootContentItem;
    } else {
        item = static_cast<MagnatuneContentItem*>(index.internalPointer());
        debug() << "    valid item"  << endl;
    }

   switch ( item->getType() ) {
       case MAGNATUNE_ARTIST:
           debug() << "    artist"  << endl;
           m_infoParser->getInfo( item->getContentUnion().artistValue );
           break;
       case MAGNATUNE_ALBUM:
           debug() << "    album"  << endl;
           m_infoParser->getInfo( item->getContentUnion().albumValue );
           break;
       default:
           debug() << "    none of the above!?"  << endl;
     }

}

void MagnatuneContentModel::infoParsed( const QString &infoHtml ) {
    //debug() << "MagnatuneContentModel::infoParsed"  << endl;
    emit( infoChanged ( infoHtml ) );
}

bool MagnatuneContentModel::canFetchMore(const QModelIndex & parent) const
{

     MagnatuneContentItem* item;

     if (!parent.isValid())
         item = m_rootContentItem;
     else
         item = static_cast<MagnatuneContentItem*>(parent.internalPointer());

    //debug() << "MagnatuneContentModel::canFetchMore called on node: " << item->data( 0 ).toString()  << endl;


    if ( ( item->getType() == MAGNATUNE_ARTIST ) || ( item->getType() == MAGNATUNE_ALBUM )  ) {
        debug() << "    YES!" << endl;
        return true;
    }
    else 
        return false;

}

void MagnatuneContentModel::fetchMore(const QModelIndex & parent)
{
    MagnatuneContentItem* item;

     if (!parent.isValid())
         item = m_rootContentItem;
     else
         item = static_cast<MagnatuneContentItem*>(parent.internalPointer());

       //debug() << "MagnatuneContentModel::fetchMore called on node: " << item->data( 0 ).toString()  << endl;

     int count = item->prePopulate();
     beginInsertRows( parent, 0, count );
     item->populate();
     endInsertRows();
}



#include "magnatunecontentmodel.moc"
