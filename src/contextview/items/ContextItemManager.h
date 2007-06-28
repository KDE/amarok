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

#ifndef CONTEXT_ITEM_MANAGER_H
#define CONTEXT_ITEM_MANAGER_H

#include "ContextItem.h"
#include "ContextItemManagerWidget.h"

#include <QMap>

// this is the controller singleton class for ContextItemManagerWidget as well as the main Context Item manager
class ContextItemManager : public QObject
{
    Q_OBJECT
    static ContextItemManager* s_instance; 

public:
    static ContextItemManager *instance()
    {
        if( !s_instance )
            return new ContextItemManager();
        return s_instance;
    }
    
    void applyConfig();
    
public slots:
    
    void showDialog();
    
private:
    ContextItemManager();
    
    void enableItem( ContextItem* item );

    void addItem( ContextItem* item );
    // these two maps hold the required info about the context items: which are enabled, and the string representation -> pointer link
    QMap< ContextItem*, bool > m_itemsEnabled;
    QMap< QString, ContextItem* > m_itemsMap;
    QStringList m_itemsOrder;
    
    bool m_visible;
};

#endif
