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

#ifndef CONTEXT_ITEM_MANAGER_WIDGET_H
#define CONTEXT_ITEM_MANAGER_WIDGET_H

#include "ContextItem.h"

#include "kdialog.h"
#include "klistwidget.h"

#include <QListWidgetItem>
#include <QMap>
#include <QWidget>

using namespace Context;

class ContextItemList : public KListWidget
{
    Q_OBJECT
    
public:
    ContextItemList( QWidget *parent, const char *name = 0 );
    ~ContextItemList() {}
    
    bool hasSelection();
    bool isEmpty() { return ( count() == 0 ); }
    
public slots:
    void moveSelectedUp() { moveSelected( -1 ); }
    void moveSelectedDown() { moveSelected( 1 ); }
    
signals:
    void changed();
    
private:
    void moveSelected( int direction );
    

    
};

class ContextItemManagerWidget : public KDialog
{
    Q_OBJECT
    static ContextItemManagerWidget* s_instance; 
    
public: 
    static ContextItemManagerWidget *instance() { return s_instance; }
    
    ContextItemManagerWidget( QWidget *parent = 0, const char *name = 0, QMap< ContextItem*, bool >* enabled = 0, QStringList order = QStringList() );
    
private slots:
    
    void toggleState();
    void updateButtons();
    void applyNow();
    void changed();
    
private:
    
    void insertItems();
    void insertItem( const QString& name, bool enabled );
    
    bool findInContextMap( const QString name );
    void insertInContextMap( const QString name, bool val );

    QMap< ContextItem*, bool >* m_contextItems; // holds the real data :)
    QStringList m_itemsOrder; // order of listview items
    
    ContextItemList *m_listview;
    
    KPushButton *m_up;
    KPushButton *m_down;
    KPushButton *m_toggle;
};

#endif
