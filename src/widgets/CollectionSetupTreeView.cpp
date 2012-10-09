/****************************************************************************************
 * Copyright (c) 2003 Scott Wheeler <wheeler@kde.org>                                   *
 * Copyright (c) 2004 Max Howell <max.howell@methylblue.com>                            *
 * Copyright (c) 2004-2008 Mark Kretschmann <kretschmann@kde.org>                       *
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2008 Sebastian Trueg <trueg@kde.org>                                   *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "CollectionSetupTreeView.h"
#include "dialogs/CollectionSetup.h"

#include "core/support/Debug.h"
#include "core/collections/Collection.h"
#include "core-impl/collections/support/CollectionManager.h"

#include <QAction>
#include <QApplication>
#include <QMenu>

CollectionSetupTreeView::CollectionSetupTreeView( QWidget *parent )
        : QTreeView( parent )
{
    DEBUG_BLOCK
    m_rescanDirAction = new QAction( this );
    connect( this, SIGNAL( pressed(const QModelIndex &) ), this, SLOT( slotPressed(const QModelIndex&) ) );
    connect( m_rescanDirAction, SIGNAL( triggered() ), this, SLOT( slotRescanDirTriggered() ) );
}

CollectionSetupTreeView::~CollectionSetupTreeView()
{
    this->disconnect();
    delete m_rescanDirAction;
}

void
CollectionSetupTreeView::slotPressed( const QModelIndex &index )
{
    DEBUG_BLOCK
    // --- show context menu on right mouse button
    QObject *grandParent = parent() ? parent()->parent() : 0;
    CollectionSetup *collSetup = qobject_cast<CollectionSetup *>( grandParent );
    if( ( QApplication::mouseButtons() & Qt::RightButton ) && collSetup )
    {
        m_currDir = collSetup->modelFilePath( index );
        debug() << "Setting current dir to " << m_currDir;

        // check if there is an sql collection covering the directory
        bool covered = false;
        QList<Collections::Collection*> queryableCollections = CollectionManager::instance()->queryableCollections();
        foreach( Collections::Collection *collection, queryableCollections )
        {
            if( collection->isDirInCollection( m_currDir ) )
                covered = true;
        }

        // it's covered, so we can show the rescan option
        if( covered )
        {
            m_rescanDirAction->setText( i18n( "Rescan '%1'", m_currDir ) );
            QMenu menu;
            menu.addAction( m_rescanDirAction );
            menu.exec( QCursor::pos() );
        }
    }
}

void
CollectionSetupTreeView::slotRescanDirTriggered()
{
    DEBUG_BLOCK
    CollectionManager::instance()->startIncrementalScan( m_currDir );
}

#include "CollectionSetupTreeView.moc"
