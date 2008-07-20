/***************************************************************************
   begin                : Tue Feb 4 2003
   copyright            : (C) 2003 Scott Wheeler <wheeler@kde.org>
                        : (C) 2004 Max Howell <max.howell@methylblue.com>
                        : (C) 2004 Mark Kretschmann <markey@web.de>
                        : (C) 2008 Seb Ruiz <ruiz@kde.org>
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_COLLECTIONSETUP_H
#define AMAROK_COLLECTIONSETUP_H

#include <KDirLister> //stack allocated
#include <KUrl>       //stack allocated
#include <KVBox>      //baseclass

#include <QCheckBox>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include "Debug.h"

namespace CollectionFolder { class Item; }

// Reimplement sizeHint to have directorylist not being too big for "low" (1024x768 is not exactly low) resolutions
class CollectionView : public QTreeWidget
{
    public:
        explicit CollectionView( QWidget * parent = 0 )
            : QTreeWidget( parent )
        { }

        QSize sizeHint() const
        {
            return QSize( 400, 150 );
        }
};

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

        CollectionView *m_view;
        QStringList m_dirs;
        QCheckBox *m_recursive;
        QCheckBox *m_monitor;
};


namespace CollectionFolder //just to keep it out of the global namespace
{

class Item : public QObject, public QTreeWidgetItem
{
        Q_OBJECT

    public:
        Item( QTreeWidget *parent, const QString &root );
        Item( QTreeWidgetItem *parent, const KUrl &url , bool full_disable=false );

        QTreeWidgetItem *parent() const { return static_cast<QTreeWidgetItem*>( QTreeWidgetItem::parent() ); }
        bool isFullyDisabled() const { return m_fullyDisabled; }
        bool isDisabled() const { return isFullyDisabled() || ( CollectionSetup::instance()->recursive() && parent() && parent()->checkState(0) == Qt::Checked ); }
        QString fullPath() const;

        void setExpanded( bool b ); // reimpl.
        void setCheckState( int column, Qt::CheckState state ); // reimpl.
        //void paintCell( QPainter * p, const QColorGroup & cg, int column, int width, int align ); // reimpl.

    public slots:
        void newItems( const KFileItemList& );
        void completed() { }

    private:
        KDirLister m_lister;
        KUrl       m_url;
        bool       m_listed;
        bool       m_fullyDisabled;
};

} // end namespace CollectionFolder

#endif

