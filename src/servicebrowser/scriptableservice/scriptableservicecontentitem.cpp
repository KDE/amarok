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


#include "scriptableservicecontentitem.h"


#include "debug.h"
#include "magnatunedatabasehandler.h"



ScriptableServiceContentItem::ScriptableServiceContentItem(QString name, QString url, QString infoHtml, ScriptableServiceContentItem * parent)
{
    m_name = name;
    m_url = url;
    m_parent = parent;
    m_infoHtml = infoHtml;
    //m_hasPopulatedChildItems = false;
}

 ScriptableServiceContentItem::~ScriptableServiceContentItem()
 {
     qDeleteAll(m_childItems);
 }

void ScriptableServiceContentItem::addChildItem ( ScriptableServiceContentItem * childItem ) {

    m_childItems.append( childItem );

}

ScriptableServiceContentItem *ScriptableServiceContentItem::child(int row)
{

   return dynamic_cast<ScriptableServiceContentItem*>( m_childItems.value( row ) );

}

int ScriptableServiceContentItem::childCount() const
{

    return m_childItems.count();
}

int ScriptableServiceContentItem::columnCount() const
{
    return 1; //FIXME!!
}

QVariant ScriptableServiceContentItem::data(int column) const  //FIXME!!! do We need more columns (for track length and so on...)
{
    return m_name;
}

int ScriptableServiceContentItem::row() const
{
    if (m_parent){
        return m_parent->getChildItems().indexOf(const_cast<ScriptableServiceContentItem*>(this));
    }
    return 0;
} 

QList<ServiceModelItemBase*> ScriptableServiceContentItem::getChildItems() const {

    return m_childItems;
}

bool ScriptableServiceContentItem::hasChildren () const {
    return ( m_childItems.size() > 0 );
}

QString ScriptableServiceContentItem::getUrl() {

    return m_url;
}

QString ScriptableServiceContentItem::getInfoHtml() {
    return m_infoHtml;
}