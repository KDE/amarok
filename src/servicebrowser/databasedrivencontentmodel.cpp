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
#include "databasedrivencontentmodel.h"
#include "databasehandlerbase.h"


DatabaseDrivenContentModel::DatabaseDrivenContentModel(QObject *parent, const QString &genre )
     : ServiceModelBase(parent)
     , m_dbHandler ( 0 )
     , m_infoParser( 0 )
{
     m_genre = genre;

    
     //m_infoParser = new DatabaseDrivenInfoParser();
    // connect( m_infoParser, SIGNAL (info ( const QString &) ), this, SLOT( infoParsed( const QString &) ) );


    m_artistIcon = KIcon( Amarok::icon( "artist" ) );
    m_albumIcon = KIcon( Amarok::icon( "album" ) );
    m_trackIcon = KIcon( Amarok::icon( "track" ) );

     
}

DatabaseDrivenContentModel::~DatabaseDrivenContentModel()
{
      delete m_rootContentItem;
}

int DatabaseDrivenContentModel::columnCount(const QModelIndex &parent) const
{

   //debug() << "DatabaseDrivenContentModel::columnCount" << endl;


        if (parent.isValid())
         return static_cast<DatabaseDrivenContentItem*>(parent.internalPointer())->columnCount();
     else
         return m_rootContentItem->columnCount();

}

QVariant DatabaseDrivenContentModel::data(const QModelIndex &index, int role) const
{
    //debug() << "DatabaseDrivenContentModel::data" << endl;
    

    if (!index.isValid())
        return QVariant();



    if (role == Qt::DisplayRole) {
        DatabaseDrivenContentItem *item = static_cast<DatabaseDrivenContentItem*>(index.internalPointer());
        return item->data(index.column());
    } else if ( role == Qt::DecorationRole ) {
        DatabaseDrivenContentItem *item = static_cast<DatabaseDrivenContentItem*>(index.internalPointer());

        if ( item->getType() == SERVICE_ITEM_ARTIST )
            return m_artistIcon;
        else if ( item->getType() == SERVICE_ITEM_ALBUM )
            return m_albumIcon;
        else if ( item->getType() == SERVICE_ITEM_TRACK )
            return m_trackIcon;
        else 
            return QVariant();

    } else {
        return QVariant();
    }
}

QVariant DatabaseDrivenContentModel::headerData(int section, Qt::Orientation orientation, int role) const
{

    //debug() << "DatabaseDrivenContentModel::headerData" << endl;

         if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
         return "Artist / Album / Track";

     return QVariant();

}

QModelIndex DatabaseDrivenContentModel::index(int row, int column, const QModelIndex &parent) const
{

     //debug() << "DatabaseDrivenContentModel::index, row: " << row << ", column: " << column << endl;

     DatabaseDrivenContentItem *parentItem;

     if (!parent.isValid())
         parentItem = m_rootContentItem;
     else
         parentItem = static_cast<DatabaseDrivenContentItem*>(parent.internalPointer());

     DatabaseDrivenContentItem *childItem = parentItem->child(row);
     if (childItem)
         return createIndex(row, column, childItem);
     else
         return QModelIndex();


}

QModelIndex DatabaseDrivenContentModel::parent(const QModelIndex &index) const
{


      //debug() << "DatabaseDrivenContentModel::parent" << endl; 
      if (!index.isValid()) {
         //debug() << "DatabaseDrivenContentModel::parent, index invalid... " << endl; 
         return QModelIndex();
     }

     DatabaseDrivenContentItem *childItem = static_cast<DatabaseDrivenContentItem*>(index.internalPointer());
     DatabaseDrivenContentItem *parentItem = static_cast<DatabaseDrivenContentItem*>(childItem->parent() );

     if (parentItem == m_rootContentItem)
         //debug() << "DatabaseDrivenContentModel::parent, root item... " << endl; 
         return QModelIndex();

     return createIndex(parentItem->row(), 0, parentItem);


}

int DatabaseDrivenContentModel::rowCount(const QModelIndex &parent) const
{
      // debug() << "DatabaseDrivenContentModel::rowCount"  << endl;

      DatabaseDrivenContentItem *parentItem;

     if (!parent.isValid())
         parentItem = m_rootContentItem;
     else
         parentItem = static_cast<DatabaseDrivenContentItem*>(parent.internalPointer());

      //debug() << "DatabaseDrivenContentModel::rowCount called on node: " << parentItem->data( 0 ).toString() << ", count: " << parentItem->childCount() << endl;

     return parentItem->childCount();


} 

bool DatabaseDrivenContentModel::hasChildren ( const QModelIndex & parent ) const {

    DatabaseDrivenContentItem* item;

     if (!parent.isValid())
         item = m_rootContentItem;
     else
         item = static_cast<DatabaseDrivenContentItem*>(parent.internalPointer());


     //debug() << "DatabaseDrivenContentModel::hasChildren called on node: " << item->data( 0 ).toString() << ", has children: " << item->hasChildren() << endl;

    return item->hasChildren();
}

void DatabaseDrivenContentModel::setGenre( const QString &genre ) {

    m_genre = genre;
    delete m_rootContentItem;
    m_rootContentItem = new DatabaseDrivenContentItem( m_genre, m_dbHandler );
    reset();

}


void DatabaseDrivenContentModel::requestHtmlInfo ( const QModelIndex & index ) const {

    //debug() << "DatabaseDrivenContentModel::requestHtmlInfo"  << endl;
    DatabaseDrivenContentItem* item;

    if (!index.isValid()) {
        debug() << "    invalid item"  << endl;
        item = m_rootContentItem;
    } else {
        item = static_cast<DatabaseDrivenContentItem*>(index.internalPointer());
        debug() << "    valid item"  << endl;
    }

   switch ( item->getType() ) {
       case SERVICE_ITEM_ARTIST:
           debug() << "    artist"  << endl;
           if( m_infoParser != 0 )
              m_infoParser->getInfo( item->getContentUnion().artistValue );
           break;
       case SERVICE_ITEM_ALBUM:
           debug() << "    album"  << endl;
           if( m_infoParser != 0 )
               m_infoParser->getInfo( item->getContentUnion().albumValue );
           break;
        case SERVICE_ITEM_TRACK:
           debug() << "    track"  << endl;
           if( m_infoParser != 0 )
               m_infoParser->getInfo( item->getContentUnion().trackValue );
           break;
       default:
           debug() << "    none of the above!?"  << endl;
     }

}

void DatabaseDrivenContentModel::infoParsed( const QString &infoHtml ) {
    //debug() << "DatabaseDrivenContentModel::infoParsed"  << endl;
    emit( infoChanged ( infoHtml ) );
}

bool DatabaseDrivenContentModel::canFetchMore(const QModelIndex & parent) const
{

     DatabaseDrivenContentItem* item;

     if (!parent.isValid())
         item = m_rootContentItem;
     else
         item = static_cast<DatabaseDrivenContentItem*>(parent.internalPointer());

    //debug() << "DatabaseDrivenContentModel::canFetchMore called on node: " << item->data( 0 ).toString()  << endl;


    if ( ( item->getType() == SERVICE_ITEM_ARTIST ) || ( item->getType() == SERVICE_ITEM_ALBUM )  ) {
        //debug() << "    YES!" << endl;
        return true;
    }
    else 
        return false;

}

void DatabaseDrivenContentModel::fetchMore(const QModelIndex & parent)
{
    DatabaseDrivenContentItem* item;

    if (!parent.isValid())
        item = m_rootContentItem;
    else
        item = static_cast<DatabaseDrivenContentItem*>(parent.internalPointer());

    //debug() << "DatabaseDrivenContentModel::fetchMore called on node: " << item->data( 0 ).toString()  << endl;

    int count = item->prePopulate();


    //debug() << "DatabaseDrivenContentModel::fetchMore item has : " << count << " new child items"  << endl;

    if (!count)
        return; //no rows to insert
    beginInsertRows( parent, 0, count - 1 );
    item->populate();
    endInsertRows();
}

void DatabaseDrivenContentModel::setDbHandler(DatabaseHandlerBase * dbHandler)
{

    m_dbHandler = dbHandler;
    m_rootContentItem = new DatabaseDrivenContentItem( m_genre, m_dbHandler );

}

void DatabaseDrivenContentModel::setInfoParser(InfoParserBase * infoParser)
{
    m_infoParser = infoParser;
}



#include "databasedrivencontentmodel.moc"
