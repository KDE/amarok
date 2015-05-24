/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#include "OpmlDirectoryService.h"

#include "amarokurls/AmarokUrlHandler.h"
#include "core/support/Debug.h"
#include "core/support/Components.h"
#include "core/interfaces/Logger.h"
#include "browsers/CollectionTreeItem.h"
#include "browsers/SingleCollectionTreeItemModel.h"
#include "OpmlDirectoryInfoParser.h"
#include "OpmlDirectoryModel.h"
#include "OpmlDirectoryView.h"
#include "playlistmanager/PlaylistManager.h"
#include "core/podcasts/PodcastProvider.h"
#include "ServiceSqlRegistry.h"
#include "widgets/SearchWidget.h"

#include <KStandardDirs>
#include <KTemporaryFile>
#include <threadweaver/ThreadWeaver.h>

#include <typeinfo>

using namespace Meta;

AMAROK_EXPORT_SERVICE_PLUGIN( opmldirectory, OpmlDirectoryServiceFactory )

OpmlDirectoryServiceFactory::OpmlDirectoryServiceFactory( QObject *parent, const QVariantList &args )
    : ServiceFactory( parent, args )
{
    KPluginInfo pluginInfo( "amarok_service_opmldirectory.desktop", "services" );
    pluginInfo.setConfig( config() );
    m_info = pluginInfo;
}

void OpmlDirectoryServiceFactory::init()
{
    ServiceBase* service = new OpmlDirectoryService( this, "OpmlDirectory", i18n( "Podcast Directory" ) );
    m_initialized = true;
    emit newService( service );
}


QString OpmlDirectoryServiceFactory::name()
{
    return "OpmlDirectory";
}

KConfigGroup OpmlDirectoryServiceFactory::config()
{
    return Amarok::config( "Service_OpmlDirectory" );
}


OpmlDirectoryService::OpmlDirectoryService( OpmlDirectoryServiceFactory* parent, const QString &name, const QString &prettyName )
 : ServiceBase( name, parent, false, prettyName )
{
    setShortDescription( i18n( "A large listing of podcasts" ) );
    setIcon( KIcon( "view-services-opml-amarok" ) );

    setLongDescription( i18n( "A comprehensive list of searchable podcasts that you can subscribe to directly from within Amarok." ) );

    KIconLoader loader;
    setImagePath( loader.iconPath( "view-services-opml-amarok", -128, true ) );

    The::amarokUrlHandler()->registerRunner( this, command() );

    setServiceReady( true );
}


OpmlDirectoryService::~OpmlDirectoryService()
{
}

void OpmlDirectoryService::polish()
{
    generateWidgetInfo();
    if ( m_polished )
        return;

    //do not allow this content to get added to the playlist. At least not for now
    setPlayableTracks( false );

    //TODO: implement searching
    m_searchWidget->setVisible( false );

    OpmlDirectoryView* opmlView = new OpmlDirectoryView( this );
    opmlView->setHeaderHidden( true );
    opmlView->setFrameShape( QFrame::NoFrame );
    opmlView->setDragEnabled ( true );
    opmlView->setSortingEnabled( false );
    opmlView->setSelectionMode( QAbstractItemView::ExtendedSelection );
    opmlView->setDragDropMode ( QAbstractItemView::DragOnly );
    opmlView->setEditTriggers( QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed );
    setView( opmlView );
    QUrl opmlLocation( Amarok::saveLocation() );
    opmlLocation = opmlLocation.adjusted(QUrl::StripTrailingSlash);
    opmlLocation.setPath(opmlLocation.path() + '/' + ( "podcast_directory.opml" ));

    if( !QFile::exists( opmlLocation.toLocalFile() ) )
    {
        //copy from the standard data dir
        QUrl schippedOpmlLocation( KStandardDirs::locate( "data", "amarok/data/" ) );
        schippedOpmlLocation = schippedOpmlLocation.adjusted(QUrl::StripTrailingSlash);
        schippedOpmlLocation.setPath(schippedOpmlLocation.path() + '/' + ( "podcast_directory.opml" ));
        if( !QFile::copy( schippedOpmlLocation.toLocalFile(), opmlLocation.toLocalFile() ) )
        {
            debug() << QString( "Failed to copy from %1 to %2" )
            .arg( schippedOpmlLocation.toLocalFile(), opmlLocation.toLocalFile() );
            //TODO: error box drawn in the view's area.
            return;
        }
    }

    setModel( new OpmlDirectoryModel( opmlLocation, this ) );

    m_subscribeButton = new QPushButton( m_bottomPanel );
    m_subscribeButton->setText( i18n( "Subscribe" ) );
    m_subscribeButton->setObjectName( "subscribeButton" );
    m_subscribeButton->setIcon( KIcon( "get-hot-new-stuff-amarok" ) );

    m_subscribeButton->setEnabled( false );

    connect( m_subscribeButton, SIGNAL(clicked()), this, SLOT(subscribe()) );

    m_addOpmlButton = new QPushButton( m_bottomPanel );
    m_addOpmlButton->setText( i18n( "Add OPML" ) );
    m_addOpmlButton->setObjectName( "addOpmlButton" );
    m_addOpmlButton->setIcon( KIcon( "list-add-amarok" ) );

    connect( m_addOpmlButton, SIGNAL(clicked()), model(), SLOT(slotAddOpmlAction()) );

    connect( view()->selectionModel(),
             SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
             SLOT(slotSelectionChanged(QItemSelection,QItemSelection))
           );

    setInfoParser( new OpmlDirectoryInfoParser() );

    m_polished = true;
}

QString
OpmlDirectoryService::command() const
{
    return "service-podcastdirectory";
}

QString
OpmlDirectoryService::prettyCommand() const
{
    return i18n( "Add an OPML file to the list." );
}

bool
OpmlDirectoryService::run( AmarokUrl url )
{
    //make sure this category is shown.
    AmarokUrl( "amarok://navigate/internet/OpmlDirectory" ).run();
    if( url.path() == QLatin1String( "addOpml" ) )
    {
        OpmlDirectoryModel *opmlModel = qobject_cast<OpmlDirectoryModel *>( model() );
        Q_ASSERT_X(opmlModel, "OpmlDirectoryService::run()", "fix if a proxy is used");

        opmlModel->slotAddOpmlAction();
        return true;
    }

    return false;
}

void
OpmlDirectoryService::subscribe()
{
    OpmlDirectoryModel * opmlModel = dynamic_cast<OpmlDirectoryModel *>( model() );
    Q_ASSERT( opmlModel );
    opmlModel->subscribe( view()->selectionModel()->selectedIndexes() );
}

void
OpmlDirectoryService::slotSelectionChanged( const QItemSelection &selected,
                                            const QItemSelection &deselected )
{
    Q_UNUSED(selected)
    Q_UNUSED(deselected)
    m_subscribeButton->setEnabled( !view()->selectionModel()->selectedIndexes().isEmpty() );
}
