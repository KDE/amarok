/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/
#ifndef AMAROK_COLLECTION_WIDGET
#define AMAROK_COLLECTION_WIDGET

#include <KVBox>

class SearchWidget;
class CollectionTreeView;

class CollectionWidget : public KVBox
{
    Q_OBJECT
    public:
        CollectionWidget( const char* name );
        static CollectionWidget *instance() { return s_instance; }
        CollectionTreeView *view() const { return m_treeView; }
    public slots:
        //void slotSetFilter();

    private slots:
        void sortByArtist();
        void sortByArtistAlbum();
        void sortByArtistYearAlbum();
        void sortByAlbum();
        void sortByGenreArtist();
        void sortByGenreArtistAlbum();

    private:
        SearchWidget           *m_searchWidget;
        CollectionTreeView* m_treeView;
        static CollectionWidget *s_instance;
};

#endif
