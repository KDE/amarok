/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/
#ifndef AMAROK_COLLECTION_WIDGET
#define AMAROK_COLLECTION_WIDGET

#include <QWidget>

class SearchWidget;
class CollectionTreeView;

class CollectionWidget : public QWidget
{
    Q_OBJECT
    public:
        CollectionWidget( const char* name );
    public slots:
        //void slotSetFilter();
    private:
        SearchWidget           *m_searchWidget;
        CollectionTreeView* m_treeView;
};

#endif
