/***************************************************************************
                         directorylist.h  -  description
                            -------------------
   begin                : Tue Feb 4 2003
   copyright            : (C) 2003 by Scott Wheeler
   email                : wheeler@kde.org
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_DIRECTORYLIST_H
#define AMAROK_DIRECTORYLIST_H

#include <kdirlister.h> //stack allocated
#include <kurl.h>       //stack allocated
#include <qvbox.h>      //baseclass
#include <qcheckbox.h>  //inlined functions
#include <qlistview.h>  //baseclass


class CollectionSetup : public QVBox
{
public:
    CollectionSetup( QWidget* );

    QStringList dirs() const;
    static bool recursive() { return s_recursive->isChecked(); }
    static bool monitor()   { return s_monitor->isChecked(); }

    static QStringList s_dirs;

private:
    QListView *m_view;
    static QCheckBox *s_recursive;
    static QCheckBox *s_monitor;
};


namespace Collection { //just to keep it out of the global namespace

class Item : public QObject, public QCheckListItem
{
Q_OBJECT
public:
    Item( QListView *parent )
        : QCheckListItem( parent, "/", QCheckListItem::CheckBox  )
        , m_lister( true )
        , m_url( "file:/" )
        , m_listed( false )
    {
        m_lister.setDirOnlyMode( true );
        connect( &m_lister, SIGNAL(newItems( const KFileItemList& )), SLOT(newItems( const KFileItemList& )) );
        setOpen( true );
        setVisible( true );
    }

    Item( QListViewItem *parent, const KURL &url )
        : QCheckListItem( parent, url.fileName(), QCheckListItem::CheckBox  )
        , m_lister( true )
        , m_url( url )
        , m_listed( false )
    {
        m_lister.setDirOnlyMode( true );
        setExpandable( true );
        connect( &m_lister, SIGNAL(newItems( const KFileItemList& )), SLOT(newItems( const KFileItemList& )) );
        connect( &m_lister, SIGNAL(completed()), SLOT(completed()) );
        connect( &m_lister, SIGNAL(canceled()), SLOT(completed()) );
    }

    QCheckListItem *parent() const { return (QCheckListItem*)QListViewItem::parent(); }
    bool isDisabled() const { return CollectionSetup::recursive() && parent() && parent()->isOn(); }
    QString fullPath() const;

    virtual void setOpen( bool b )
    {
        if ( !m_listed )
        {
            m_lister.openURL( m_url, true );
            m_listed = true;
        }

        QListViewItem::setOpen( b );
    }

    virtual void stateChange( bool b )
    {
        if( CollectionSetup::recursive() )
            for( QListViewItem *item = firstChild(); item; item = item->nextSibling() )
                static_cast<QCheckListItem*>(item)->QCheckListItem::setOn( b );
    }

    virtual void activate()
    {
        if( !isDisabled() )
           QCheckListItem::activate();
    }

    virtual void paintCell( QPainter * p, const QColorGroup & cg, int column, int width, int align );

public slots:
    void newItems( const KFileItemList& );
    void completed()
    {
        if( childCount() == 0 ) { setExpandable( false ); repaint(); }
    }

private:
    KDirLister m_lister;
    KURL       m_url;
    bool       m_listed;
};
}

#endif
