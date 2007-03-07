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
#include "scriptableservicecontentmodel.h"


ScriptableServiceContentModel::ScriptableServiceContentModel( QObject *parent, QString header )
     : ServiceModelBase( parent )
{

    m_header = header;
    m_rootContentItem = new ScriptableServiceContentItem( "Base Item", "", "", 0 );
    
    m_contentIndex = 0;
    m_contentItemMap[m_contentIndex] = m_rootContentItem;
    

}

ScriptableServiceContentModel::~ScriptableServiceContentModel()
{
      delete m_rootContentItem;
}

int ScriptableServiceContentModel::insertItem( QString name, QString url, QString infoHtml, const int parentId ) {

    if (! m_contentItemMap.contains( parentId ) ) {
        return -1;
    } else {
        m_contentIndex++;
        ScriptableServiceContentItem * newItem = new ScriptableServiceContentItem( name, url, infoHtml, m_contentItemMap[parentId] );
        m_contentItemMap[m_contentIndex] = newItem;
        m_contentItemMap[parentId]->addChildItem ( newItem );
        reset (); //fixme
        return m_contentIndex;
    }

   

}

int ScriptableServiceContentModel::columnCount( const QModelIndex &parent ) const
{

   debug() << "ScriptableServiceContentModel::columnCount" << endl;


    if (parent.isValid())
        return static_cast<ScriptableServiceContentItem*>( parent.internalPointer() )->columnCount();
    else
        return m_rootContentItem->columnCount();

}

QVariant ScriptableServiceContentModel::data( const QModelIndex &index, int role ) const
{
    debug() << "ScriptableServiceContentModel::data" << endl;
    

    if ( !index.isValid() )
        return QVariant();

    if ( role != Qt::DisplayRole )
        return QVariant();

    ScriptableServiceContentItem *item = static_cast<ScriptableServiceContentItem*>( index.internalPointer() );

    return item->data( index.column() );

}

Qt::ItemFlags ScriptableServiceContentModel::flags(const QModelIndex &index) const
{

    debug() << "ScriptableServiceContentModel::flags" << endl;
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant ScriptableServiceContentModel::headerData(int section, Qt::Orientation orientation, int role) const
{

    debug() << "ScriptableServiceContentModel::headerData" << endl;

         if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
         return m_header;

     return QVariant();

}

QModelIndex ScriptableServiceContentModel::index(int row, int column, const QModelIndex &parent) const
{

     debug() << "ScriptableServiceContentModel::index, row: " << row << ", column: " << column << endl;

     ScriptableServiceContentItem *parentItem;

     if (!parent.isValid())
         parentItem = m_rootContentItem;
     else
         parentItem = static_cast<ScriptableServiceContentItem*>(parent.internalPointer());

     ScriptableServiceContentItem *childItem = parentItem->child(row);
     if (childItem)
         return createIndex(row, column, childItem);
     else
         return QModelIndex();


}

QModelIndex ScriptableServiceContentModel::parent(const QModelIndex &index) const
{


      debug() << "ScriptableServiceContentModel::parent" << endl; 
      if (!index.isValid()) {
         debug() << "ScriptableServiceContentModel::parent, index invalid... " << endl; 
         return QModelIndex();
     }

     ScriptableServiceContentItem *childItem = static_cast<ScriptableServiceContentItem*>(index.internalPointer());
     ScriptableServiceContentItem *parentItem = static_cast<ScriptableServiceContentItem*>(childItem->parent() );

     if (parentItem == m_rootContentItem)
         //debug() << "MagnatuneContentModel::parent, root item... " << endl; 
         return QModelIndex();

     return createIndex(parentItem->row(), 0, parentItem);


}

int ScriptableServiceContentModel::rowCount(const QModelIndex &parent) const
{
      debug() << "MagnatuneContentModel::rowCount"  << endl;

      ScriptableServiceContentItem *parentItem;

     if (!parent.isValid())
         parentItem = m_rootContentItem;
     else
         parentItem = static_cast<ScriptableServiceContentItem*>(parent.internalPointer());

     return parentItem->childCount();


} 

bool ScriptableServiceContentModel::hasChildren ( const QModelIndex & parent ) const {

    ScriptableServiceContentItem* item;

     if (!parent.isValid())
         item = m_rootContentItem;
     else
         item = static_cast<ScriptableServiceContentItem*>(parent.internalPointer());

    return item->hasChildren();
}

void ScriptableServiceContentModel::requestHtmlInfo ( const QModelIndex & index ) const {

    ScriptableServiceContentItem* item;

     if (!index.isValid())
         item = m_rootContentItem;
     else
         item = static_cast<ScriptableServiceContentItem*>(index.internalPointer());

    emit( infoChanged ( item->getUrl() ) );
}





#include "scriptableservicecontentmodel.moc"
