/***************************************************************************
 * copyright            : (C) 2007 Seb Ruiz <ruiz@kde.org>                 *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#define DEBUG_PREFIX "QueueManager"

#include "QueueManager.h"

#include "Debug.h"
#include "QueueModel.h"

#include <KApplication>
#include <KLocale>
#include <KVBox>
#include <KWindowSystem>

#include <QListView>

using namespace QueueManagerNS;

QueueManager *QueueManager::s_instance = 0;

QueueManager::QueueManager( QWidget *parent, const char *name )
    : KDialog( parent )
{
    setObjectName( name );
    setModal( false );
    setButtons( Ok|Apply|Cancel );
    setDefaultButton( Ok );
    showButtonSeparator( true );

    s_instance = this;

    // Gives the window a small title bar, and skips a taskbar entry
#ifdef Q_WS_X11
    KWindowSystem::setType( winId(), NET::Utility );
    KWindowSystem::setState( winId(), NET::SkipTaskbar );
#endif

    kapp->setTopWidget( this );
    setCaption( KDialog::makeStandardCaption( i18n("Queue Manager") ) );
    setInitialSize( QSize( 400, 260 ) );

    KVBox *mainBox = new KVBox( this );
    setMainWidget( mainBox );

    QListView *queueList = new QListView( mainWidget() );
    queueList->setViewMode( QListView::ListMode );
    queueList->setAcceptDrops( true );
    queueList->setDragEnabled( true );
    queueList->setDropIndicatorShown( true );

    QueueManagerNS::Model *model = The::queueModel();
    queueList->setModel( model );

    s_instance->enableButtonApply( false );

//     insertItems();
}

QueueManager::~QueueManager()
{
    s_instance = 0;
}

#include "QueueManager.moc"

