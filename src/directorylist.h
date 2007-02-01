/***************************************************************************
                         directorylist.h
                            -------------------
   begin                : Tue Feb 4 2003
   copyright            : (C) 2003 Scott Wheeler <wheeler@kde.org>
                        : (C) 2004 Max Howell <max.howell@methylblue.com>
                        : (C) 2004 Mark Kretschmann <markey@web.de>
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

#include <qcheckbox.h>  //inlined functions
#include <q3listview.h>  //baseclass
#include <q3vbox.h>      //baseclass

#include <kdirlister.h> //stack allocated
#include <kurl.h>       //stack allocated


namespace Collection { class Item; }

class QFixedListView : public Q3ListView
// Reimplement sizeHint to have directorylist not being too big for "low" (1024x768 is not exactly low) resolutions
{
public:
    QFixedListView ( QWidget * parent = 0, const char * name = 0, Qt::WFlags f = 0 )
                   :Q3ListView(parent, name, f) {};
    QSize sizeHint() const
    {
        return QSize(400, 100);
    }

};

class CollectionSetup : public Q3VBox
{
    friend class Collection::Item;

public:
    static CollectionSetup* instance() { return s_instance; }

    CollectionSetup( QWidget* );
    void writeConfig();

    QStringList dirs() const { return m_dirs; }
    bool recursive() const { return m_recursive->isChecked(); }
    bool monitor() const { return m_monitor->isChecked(); }

private:
    static CollectionSetup* s_instance;

    QFixedListView *m_view;
    QStringList m_dirs;
    QCheckBox *m_recursive;
    QCheckBox *m_monitor;
};


namespace Collection { //just to keep it out of the global namespace

class Item : public QObject, public Q3CheckListItem
{
Q_OBJECT
public:
    Item( Q3ListView *parent );
    Item( Q3ListViewItem *parent, const KUrl &url , bool full_disable=false );

    Q3CheckListItem *parent() const { return static_cast<Q3CheckListItem*>( Q3ListViewItem::parent() ); }
    bool isFullyDisabled() const { return m_fullyDisabled; }
    bool isDisabled() const { return isFullyDisabled() || ( CollectionSetup::instance()->recursive() && parent() && parent()->isOn() ); }
    QString fullPath() const;

    void setOpen( bool b ); // reimpl.
    void stateChange( bool ); // reimpl.
    void activate(); // reimpl.
    void paintCell( QPainter * p, const QColorGroup & cg, int column, int width, int align ); // reimpl.

public slots:
    void newItems( const KFileItemList& );
    void completed() { if( childCount() == 0 ) { setExpandable( false ); repaint(); } }

private:
    KDirLister m_lister;
    KUrl       m_url;
    bool       m_listed;
    bool       m_fullyDisabled;
};
}

#endif
