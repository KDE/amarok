/****************************************************************************************
 * Copyright (c) 2011 Emmanuel Wagner <manu.wagner@sfr.fr>                              *
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

#define DEBUG_PREFIX "CoverGridApplet"

#include "CoverGridApplet.h"

#include "AlbumItem.h"
#include "core/support/Amarok.h"
#include "EngineController.h"
#include "core/support/Debug.h"
#include "context/ContextView.h"
#include "core/collections/Collection.h"
#include "core/meta/Meta.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "playlist/PlaylistModelStack.h"
#include "playlist/PlaylistController.h"

#include <QAction>
#include <KColorScheme>
#include <KConfigDialog>
#include <KGlobalSettings>
#include <Plasma/ScrollWidget>
#include "covermanager/CoverCache.h"
#include <KStandardDirs>

#include <QGraphicsLinearLayout>
#include <QGraphicsProxyWidget>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsWidget>
#include <QLabel>
#include <QPixmap>
#include <QGraphicsPixmapItem>
#include <QGraphicsGridLayout>

CoverGridApplet::CoverGridApplet( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
{
    DEBUG_BLOCK

    setHasConfigurationInterface( true );
}

void
CoverGridApplet::init()
{
    // Call the base implementation.
    Context::Applet::init();
    setBackgroundHints( Plasma::Applet::NoBackground );

    KConfigGroup config = Amarok::config( "CoverGrid Applet" );
    m_coversize = config.readEntry( "CoverSize", 70 );

    QGraphicsLinearLayout* lay = new QGraphicsLinearLayout( Qt::Vertical );
    setLayout( lay );
    m_scroll = new Plasma::ScrollWidget( this );
    m_scroll->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    m_scroll->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
    m_scroll->show();

    m_proxywidget = new QGraphicsProxyWidget( this ) ;
    m_layout = new QGraphicsGridLayout( m_proxywidget );
    m_layout->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );

    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    // create a scroll Area
    m_scroll->setWidget( m_proxywidget );
    lay->addItem( m_scroll );

    Collections::Collection *coll = CollectionManager::instance()->primaryCollection();
    Collections::QueryMaker *qm = coll->queryMaker();
    qm->setAutoDelete( true );
    qm->setQueryType( Collections::QueryMaker::Album );
    qm->orderBy( Meta::valArtist );

    connect( qm, SIGNAL(newResultReady(Meta::AlbumList)),
             this, SLOT(slotAlbumQueryResult(Meta::AlbumList)) );
    qm->run();
}

CoverGridApplet::~CoverGridApplet()
{
    delete m_proxywidget;
}

void CoverGridApplet::slotAlbumQueryResult( Meta::AlbumList albums ) //SLOT
{
    DEBUG_BLOCK

    m_album_list = albums;
    prepareLayout();
}

void CoverGridApplet::createConfigurationInterface( KConfigDialog *parent )
{
    KConfigGroup configuration = config();
    QWidget * const settings = new QWidget;
    ui_Settings.setupUi( settings );
    if( m_coversize )
        ui_Settings.coversizeSpin->setValue( m_coversize );
    parent->addPage( settings, i18n( "Covergrid Settings" ), "preferences-system" );
    connect( parent, SIGNAL(accepted()), this, SLOT(saveSettings()) );
}
bool
CoverGridApplet::hasHeightForWidth() const
{
    return false;
}
void CoverGridApplet::saveSettings()
{
    m_coversize = ui_Settings.coversizeSpin->value();
    KConfigGroup config = Amarok::config( "CoverGrid Applet" );
    config.writeEntry( "CoverSize", m_coversize );
    prepareLayout();
}
void CoverGridApplet::prepareLayout()
{
    int nb_prev = m_layout->count();
    for( int i = nb_prev - 1; i >= 0; i-- ) m_layout->removeAt( i );
    m_layout->invalidate();

    const int horizontal_size = boundingRect().width();
    int x_pos = 0;
    int y_pos = 0;
    int nb_albums =  m_album_list.size();
    int nbcolumns = horizontal_size / m_coversize;
    for( int index = 0; index < nb_albums; index++ )
    {
        Meta::AlbumPtr album = m_album_list[index];
        QPixmap pixmap;
        if( album->hasImage() )
        {
            pixmap = The::coverCache()->getCover( album, m_coversize );
        }
        else
        {
            pixmap = QPixmap( KStandardDirs::locate( "data", "amarok/images/nocover.png" ) );
            QImage image = pixmap.toImage();
            image = image.scaled( QSize( m_coversize, m_coversize ), Qt::KeepAspectRatio, Qt::SmoothTransformation );
            pixmap = QPixmap::fromImage( image );
        }

        QGraphicsProxyWidget* proxywidget = new QGraphicsProxyWidget( this ) ;

        AlbumItem *pixmap_widget = new AlbumItem( pixmap, album );
        proxywidget->setWidget( pixmap_widget );
        m_layout->addItem( proxywidget, y_pos, x_pos, 0 );
        x_pos ++;

        if( x_pos > nbcolumns )
        {
            x_pos = 0; y_pos++;
        }
    }
    m_layout->activate();
}

