/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
#include "jamendocontentmodel.h"
#include "jamendodatabasehandler.h"


JamendoContentModel::JamendoContentModel(QObject *parent, const QString &genre )
     : ServiceModelBase(parent)
{
     m_genre = genre;
     m_rootContentItem = new JamendoContentItem( m_genre );
     //m_infoParser = new JamendoInfoParser();
    // connect( m_infoParser, SIGNAL (info ( const QString &) ), this, SLOT( infoParsed( const QString &) ) );


    m_artistIcon = KIcon( Amarok::icon( "artist" ) );
    m_albumIcon = KIcon( Amarok::icon( "album" ) );
    m_trackIcon = KIcon( Amarok::icon( "track" ) );

     
}

JamendoContentModel::~JamendoContentModel()
{
      delete m_rootContentItem;
}

int JamendoContentModel::columnCount(const QModelIndex &parent) const
{

   //debug() << "JamendoContentModel::columnCount" << endl;


        if (parent.isValid())
         return static_cast<JamendoContentItem*>(parent.internalPointer())->columnCount();
     else
         return m_rootContentItem->columnCount();

}

QVariant JamendoContentModel::data(const QModelIndex &index, int role) const
{
    //debug() << "JamendoContentModel::data" << endl;
    

    if (!index.isValid())
        return QVariant();



    if (role == Qt::DisplayRole) {
        JamendoContentItem *item = static_cast<JamendoContentItem*>(index.internalPointer());
        return item->data(index.column());
    } else if ( role == Qt::DecorationRole ) {
        JamendoContentItem *item = static_cast<JamendoContentItem*>(index.internalPointer());

        if ( item->getType() == JAMENDO_ARTIST )
            return m_artistIcon;
        else if ( item->getType() == JAMENDO_ALBUM )
            return m_albumIcon;
        else if ( item->getType() == JAMENDO_TRACK )
            return m_trackIcon;
        else 
            return QVariant();

    } else {
        return QVariant();
    }
}

QVariant JamendoContentModel::headerData(int section, Qt::Orientation orientation, int role) const
{

    //debug() << "JamendoContentModel::headerData" << endl;

         if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
         return "Artist / Album / Track";

     return QVariant();

}

QModelIndex JamendoContentModel::index(int row, int column, const QModelIndex &parent) const
{

     //debug() << "JamendoContentModel::index, row: " << row << ", column: " << column << endl;

     JamendoContentItem *parentItem;

     if (!parent.isValid())
         parentItem = m_rootContentItem;
     else
         parentItem = static_cast<JamendoContentItem*>(parent.internalPointer());

     JamendoContentItem *childItem = parentItem->child(row);
     if (childItem)
         return createIndex(row, column, childItem);
     else
         return QModelIndex();


}

QModelIndex JamendoContentModel::parent(const QModelIndex &index) const
{


      //debug() << "JamendoContentModel::parent" << endl; 
      if (!index.isValid()) {
         //debug() << "JamendoContentModel::parent, index invalid... " << endl; 
         return QModelIndex();
     }

     JamendoContentItem *childItem = static_cast<JamendoContentItem*>(index.internalPointer());
     JamendoContentItem *parentItem = static_cast<JamendoContentItem*>(childItem->parent() );

     if (parentItem == m_rootContentItem)
         //debug() << "JamendoContentModel::parent, root item... " << endl; 
         return QModelIndex();

     return createIndex(parentItem->row(), 0, parentItem);


}

int JamendoContentModel::rowCount(const QModelIndex &parent) const
{
      // debug() << "JamendoContentModel::rowCount"  << endl;

      JamendoContentItem *parentItem;

     if (!parent.isValid())
         parentItem = m_rootContentItem;
     else
         parentItem = static_cast<JamendoContentItem*>(parent.internalPointer());

      debug() << "JamendoContentModel::rowCount called on node: " << parentItem->data( 0 ).toString() << ", count: " << parentItem->childCount() << endl;

     return parentItem->childCount();


} 

bool JamendoContentModel::hasChildren ( const QModelIndex & parent ) const {

    JamendoContentItem* item;

     if (!parent.isValid())
         item = m_rootContentItem;
     else
         item = static_cast<JamendoContentItem*>(parent.internalPointer());


     debug() << "JamendoContentModel::hasChildren called on node: " << item->data( 0 ).toString() << ", has children: " << item->hasChildren() << endl;

    return item->hasChildren();
}

void JamendoContentModel::setGenre( const QString &genre ) {

    m_genre = genre;
    delete m_rootContentItem;
    m_rootContentItem = new JamendoContentItem( m_genre );
    reset();

}


void JamendoContentModel::requestHtmlInfo ( const QModelIndex & index ) const {

    //debug() << "JamendoContentModel::requestHtmlInfo"  << endl;
    JamendoContentItem* item;

    if (!index.isValid()) {
        debug() << "    invalid item"  << endl;
        item = m_rootContentItem;
    } else {
        item = static_cast<JamendoContentItem*>(index.internalPointer());
        debug() << "    valid item"  << endl;
    }

   switch ( item->getType() ) {
       case JAMENDO_ARTIST:
           debug() << "    artist"  << endl;
           //m_infoParser->getInfo( item->getContentUnion().artistValue );
           break;
       case JAMENDO_ALBUM:
           debug() << "    album"  << endl;
          // m_infoParser->getInfo( item->getContentUnion().albumValue );
           break;
       default:
           debug() << "    none of the above!?"  << endl;
     }

}

void JamendoContentModel::infoParsed( const QString &infoHtml ) {
    //debug() << "JamendoContentModel::infoParsed"  << endl;
    emit( infoChanged ( infoHtml ) );
}

bool JamendoContentModel::canFetchMore(const QModelIndex & parent) const
{

     JamendoContentItem* item;

     if (!parent.isValid())
         item = m_rootContentItem;
     else
         item = static_cast<JamendoContentItem*>(parent.internalPointer());

    debug() << "JamendoContentModel::canFetchMore called on node: " << item->data( 0 ).toString()  << endl;


    if ( ( item->getType() == JAMENDO_ARTIST ) || ( item->getType() == JAMENDO_ALBUM )  ) {
        debug() << "    YES!" << endl;
        return true;
    }
    else 
        return false;

}

void JamendoContentModel::fetchMore(const QModelIndex & parent)
{
    JamendoContentItem* item;

    if (!parent.isValid())
        item = m_rootContentItem;
    else
        item = static_cast<JamendoContentItem*>(parent.internalPointer());

    debug() << "JamendoContentModel::fetchMore called on node: " << item->data( 0 ).toString()  << endl;

    int count = item->prePopulate();


    debug() << "JamendoContentModel::fetchMore item has : " << count << " new child items"  << endl;

    if (!count)
        return; //no rows to insert
    beginInsertRows( parent, 0, count - 1 );
    item->populate();
    endInsertRows();
}



#include "jamendocontentmodel.moc"
