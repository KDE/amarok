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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/ 


#include "scriptableservicecontentitem.h"


#include "debug.h"




ScriptableServiceContentItem::ScriptableServiceContentItem(const QString &name, const QString &url, const QString &infoHtml, ScriptableServiceContentItem * parent)
{
    m_name = name;
    m_url = url;
    m_parent = parent;
    m_infoHtml = infoHtml;
    m_hasPopulatedChildItems = true;

    m_type = STATIC;
}

ScriptableServiceContentItem::ScriptableServiceContentItem(const QString &name, QString const &callbackScript, const QString & callbackArgument, const QString &infoHtml, ScriptableServiceContentItem * parent) {
   
    m_name = name;
    m_parent = parent;
    m_infoHtml = infoHtml;
    
    m_callbackScript = callbackScript;
    m_callbackArgument = callbackArgument;

    m_hasPopulatedChildItems = false;
    m_type = DYNAMIC;
}

 ScriptableServiceContentItem::~ScriptableServiceContentItem()
 {
     qDeleteAll(m_childItems);
 }

void ScriptableServiceContentItem::addChildItem ( ScriptableServiceContentItem * childItem ) {

    m_childItems.append( childItem );
    m_hasPopulatedChildItems = true;

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

bool ScriptableServiceContentItem::hasChildren() const {
    
    if (m_type == STATIC)
        return ( m_childItems.size() > 0 );
    else
        return true;
}

QString ScriptableServiceContentItem::getUrl() {

    return m_url;
}

QString ScriptableServiceContentItem::getInfoHtml() {
    return m_infoHtml;
}

int ScriptableServiceContentItem::getType() { 
    return m_type; 
}

bool ScriptableServiceContentItem::isPopulated() {
    return m_hasPopulatedChildItems;
}

QString ScriptableServiceContentItem::getCallbackScript() { 
    return m_callbackScript; 
}


QString ScriptableServiceContentItem::getCallbackArgument() { 
    return m_callbackArgument; 
}


