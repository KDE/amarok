/***************************************************************************
 * copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>        *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "ContextItemManager.h"

#include "amarok.h"
#include "amarokconfig.h"
#include "debug.h"
#include "../contextview.h"
#include "ContextItem.h"
#include "ContextItemManagerWidget.h"
#include "LastFmItem.h"
#include "LyricsItem.h"
#include "WikipediaItem.h"

#include <QMap>

/*///////////////////////////////////////////////////////////////////////////
 NOTE: These classes deal with the handling of Context Items. If you want to 
 write your own Context Item, it needs to subclass ContextItem.h, AND you need
 to add it to the src/amarokrc file, AND you need to add it to ContextItemManager
constructor in order for the dialog and context view to pick it up.
///////////////////////////////////////////////////////////////////////////*/

/////////////////////////////////////////////////////////////////
//// Class ContextItemManager
/////////////////////////////////////////////////////////////////

ContextItemManager *ContextItemManager::s_instance = 0;

ContextItemManager::ContextItemManager()
: m_visible( false )
{
    
    DEBUG_BLOCK
        
    s_instance = this;
    // NOTE if you want to add a context item, you MUST add it to this map, or
    // else it will never be started! also, make sure the name string corresponds
    // to what the item name() function reports
    m_itemsMap[ "wikipedia" ] = new WikipediaItem();
    m_itemsMap[ "lastfm" ] = new LastFmItem();
    m_itemsMap[ "lyrics" ] = LyricsItem::instance(); // i know, you shouldn't ever 
    // save a copy of a singleton...
    
    applyConfig();

}

void ContextItemManager::applyConfig()
{
    DEBUG_BLOCK
    //read in the config data, and start the enabled items
    m_itemsOrder = Amarok::config( "ContextItemManager" ).readEntry( "Items", QStringList() );
    int i = m_startBox; // start the counting from the last context box owned 
    // by the contextview itself
    foreach( QString str, m_itemsOrder )
    {
        bool enabled = Amarok::config( "ContextItemManager" ).readEntry( str, false );
        debug() << "Reading Context item: " << str << enabled << endl;
        m_itemsEnabled.insert( m_itemsMap.value( str ) , enabled );
        if( enabled ) m_itemsMap.value( str )->enable();
        else m_itemsMap.value( str )->disable();
        m_itemsMap.value( str )->setPosition( i ); // tell it where to put
        i++;  // itself in the contextview
    }
}


//workhorse method: shows the ContextItemManagerWidget, updates the ContextView with changes
void ContextItemManager::showDialog()
{
    DEBUG_BLOCK
        
    if( ContextItemManagerWidget::instance() )
    {
        debug() << "using already running ContextManager instance" << endl;
        ContextItemManagerWidget::instance()->raise();
        return;
    }
    
    debug() << "context items in map: " << endl;
    foreach( ContextItem* key, m_itemsEnabled.keys() )
        debug() << "context item: " << key->name() << endl;
    ContextItemManagerWidget dialog( 0, "Context Item Manager", &m_itemsEnabled, m_itemsOrder );
    

    if( dialog.exec() == QDialog::Accepted )
            applyConfig();
    
    
}

#include "ContextItemManager.moc"
