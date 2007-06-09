/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/
 

#include "collectiontreeview.h"
#include "CollectionWidget.h"

#include <QVBoxLayout>

#include <KLineEdit>
#include "searchwidget.h"

CollectionWidget::CollectionWidget( const char* name )
{
    setObjectName( name );
    QVBoxLayout* layout = new QVBoxLayout;
    m_treeView = new CollectionTreeView( this );
    //layout->addWidget( new KLineEdit( this ) );
    layout->addWidget( new SearchWidget( this, m_treeView ) );
    layout->addWidget( m_treeView );
    setLayout( layout );
}

#include "CollectionWidget.moc"
