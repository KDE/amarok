/****************************************************************************************
 * Copyright (c) 2010 Emmanuel Wagner <manu.wagner@sfr.fr>                         *
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

#include "CoverBlingApplet.h"

// Amarok
#include "Amarok.h"
#include "Debug.h"
#include "context/ContextView.h"
#include "context/widgets/TextScrollingWidget.h"
#include "collection/Collection.h"
#include "collection/CollectionManager.h"
#include "context/widgets/RatingWidget.h"
#include "playlist/PlaylistModelStack.h"
// KDE
#include <KAction>
#include <KColorScheme>
#include <KConfigDialog>
#include <KGlobalSettings>
#include <Plasma/BusyWidget>
#include <Plasma/IconWidget>
#include <Plasma/Theme>

// Qt
#include <QGraphicsLinearLayout>
#include <QGraphicsProxyWidget>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsWidget>
#include <QGraphicsProxyWidget>
#include <QLabel>

#define DEBUG_PREFIX "CoverBlingApplet"

CoverBlingApplet::CoverBlingApplet( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
{
    DEBUG_BLOCK
    setHasConfigurationInterface( false );
}

void 
CoverBlingApplet::init()
{
    setBackgroundHints( Plasma::Applet::NoBackground );

    m_vertical_size = 300;
    m_horizontal_size = 500;

    resize( m_horizontal_size,m_vertical_size);
   
    m_layout = new QGraphicsProxyWidget(this);
    m_pictureflow = new PictureFlow();
    m_layout->setWidget(m_pictureflow);
    QSize slideSize(150,150);
    m_pictureflow->setSlideSize(slideSize);
    m_pictureflow->show();    
    Amarok::Collection *coll = CollectionManager::instance()->primaryCollection();
    QueryMaker *qm = coll->queryMaker();
    qm->setAutoDelete( true );
    qm->setQueryType( QueryMaker::Album );
    qm->orderBy( Meta::valArtist );

    connect( qm, SIGNAL( newResultReady( QString, Meta::AlbumList ) ),
             this, SLOT( slotAlbumQueryResult( QString, Meta::AlbumList ) ) );
    connect( qm, SIGNAL(queryDone()), this, SLOT( slideChanged(1)));
    connect(m_pictureflow, SIGNAL(centerIndexChanged(int)),this,SLOT(slideChanged(int)));
    connect(m_pictureflow,SIGNAL(doubleClicked(int)),this,SLOT(playAlbum(int)));
    qm->run();
    m_label = new QGraphicsSimpleTextItem(this);
    QBrush brush = KColorScheme( QPalette::Active ).foreground( KColorScheme::NormalText );
    m_label ->setBrush( brush );
    QFont labelFont;
    QFont bigFont( labelFont );
    bigFont.setPointSize( bigFont.pointSize() +  2 );
    m_label->setFont( labelFont );
    m_label ->setPos(m_horizontal_size/2,m_vertical_size-50);
    // pour le rating
    m_ratingWidget = new RatingWidget( this );
    m_ratingWidget->setSpacing( 2 );
    //connect( m_ratingWidget, SIGNAL( ratingChanged( int ) ), SLOT( changeTrackRating( int ) ) );
    m_ratingWidget->setPos(m_horizontal_size/2,m_vertical_size-30);
    m_ratingWidget->setRating(0);
    m_ratingWidget->setEnabled(FALSE);
}

CoverBlingApplet::~CoverBlingApplet()
{
    DEBUG_BLOCK
}
void CoverBlingApplet::slotAlbumQueryResult( QString collectionId, Meta::AlbumList albums) //SLOT
{
    DEBUG_BLOCK
    Q_UNUSED( collectionId );
    foreach( Meta::AlbumPtr album, albums )
        m_pictureflow->addAlbum(album);
}
void CoverBlingApplet::slideChanged(int islideindex)
{
	Meta::AlbumPtr album = m_pictureflow->album(islideindex);
	if (album)
	{
		Meta::ArtistPtr artist = album->albumArtist();
		QString label = album->prettyName();
		if (artist) label+= " - " + artist->prettyName();
		m_label->setText(label);
		m_label->show();
		int nbtracks = 0;
		int rating = 0;       
		foreach( Meta::TrackPtr track, album->tracks())
		{
        		nbtracks++;
        		if (track) 
				rating+=track->rating();
		}
		if (nbtracks)
        		rating = rating/nbtracks;
		m_ratingWidget->setRating(rating);
	}
}
void CoverBlingApplet::playAlbum(int islideindex)
{
	Meta::AlbumPtr album = m_pictureflow->album(islideindex);
	if (album)
	{
    		The::playlistController()->insertOptioned( album->tracks(), Playlist::LoadAndPlay );
	}

}
void 
CoverBlingApplet::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{
    Q_UNUSED( p );
    Q_UNUSED( option );
    Q_UNUSED( contentsRect );
    p->setRenderHint( QPainter::Antialiasing );
    // tint the whole applet
    addGradientToAppletBackground( p );
}
#include "CoverBlingApplet.moc"

