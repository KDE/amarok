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

#include <KDirLister> //stack allocated
#include <KDirOperator>
#include <KUrl>       //stack allocated
#include <KVBox>      //baseclass

#include <QCheckBox>  //inlined functions
#include <QTreeWidget> // BaseClass

namespace CollectionFolder { class Item; }

class CollectionSetup : public KVBox
{
    friend class CollectionFolder::Item;

public:
    static CollectionSetup* instance() { return s_instance; }

    CollectionSetup( QWidget* );
    void writeConfig();

    QStringList dirs() const { return m_dirs; }
    bool recursive() const { return m_recursive->isChecked(); }
    bool monitor() const { return m_monitor->isChecked(); }

private:
    static CollectionSetup* s_instance;

    KDirOperator *m_dirOperator;
    QStringList m_dirs;
    QCheckBox *m_recursive;
    QCheckBox *m_monitor;
};


namespace CollectionFolder  //just to keep it out of the global namespace
{

class Item : public QObject, public QTreeWidgetItem
{
Q_OBJECT
public:
    Item( QTreeWidget *parent, const QString &root );
    Item( QTreeWidgetItem *parent, const KUrl &url , bool full_disable=false );

    bool isFullyDisabled() const { return m_fullyDisabled; }
    bool isDisabled() const { return isFullyDisabled() || ( CollectionSetup::instance()->recursive() && QTreeWidgetItem::parent() && ( QTreeWidgetItem::parent()->checkState( 0 ) == Qt::Checked ) ); }
    QString fullPath() const;

    void setExpanded( bool b ); // reimpl.
    void setCheckState( int column, Qt::CheckState checkState ); // reimpl.
//     void paintCell( QPainter * p, const QColorGroup & cg, int column, int width, int align ); // reimpl.

public slots:
    void newItems( const KFileItemList& );

private:
    KDirLister m_lister;
    KUrl       m_url;
    bool       m_listed;
    bool       m_fullyDisabled;
};
}

#endif
