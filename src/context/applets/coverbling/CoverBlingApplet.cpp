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
#include <QPixmap>
#include <QGraphicsPixmapItem>
#include <KStandardDirs>

#define DEBUG_PREFIX "CoverBlingApplet"

MyGraphicItem::MyGraphicItem( const QPixmap & pixmap, QGraphicsItem * parent, QGraphicsScene * scene)
    : QGraphicsPixmapItem(pixmap,parent,scene)
{
}
void MyGraphicItem::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
   emit clicked();
}
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
    connect( qm, SIGNAL(queryDone()), this, SLOT( slideChanged(0)));
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

    QPixmap pix1 = QPixmap(KStandardDirs::locate( "data", "amarok/images/blingtofirst.png" ));
    //pix1 = pix1.scaled(18,18,Qt::KeepAspectRatio,Qt::SmoothTransformation);
    QPixmap pix2 = QPixmap(KStandardDirs::locate( "data", "amarok/images/blingtolast.png" ));
    //pix2 = pix2.scaled(18,18,Qt::KeepAspectRatio,Qt::SmoothTransformation);
    QPixmap pix3 = QPixmap(KStandardDirs::locate( "data", "amarok/images/blingfastback.png" ));
    //pix3 = pix3.scaled(18,18,Qt::KeepAspectRatio,Qt::SmoothTransformation);
    QPixmap pix4 = QPixmap(KStandardDirs::locate( "data", "amarok/images/blingfastforward.png" ));
    //pix4 = pix4.scaled(18,18,Qt::KeepAspectRatio,Qt::SmoothTransformation);

    m_blingtofirst = new MyGraphicItem(pix1,this);
    m_blingtofirst->setOffset(30,m_vertical_size-30);
    connect(m_blingtofirst,SIGNAL(clicked()),this,SLOT(skipToFirst()));

    m_blingtolast = new MyGraphicItem(pix2,this);
    m_blingtolast->setOffset(m_horizontal_size+30,m_vertical_size-30);
    connect(m_blingtolast,SIGNAL(clicked()),this,SLOT(skipToLast()));

    m_blingfastback = new MyGraphicItem(pix3,this);
    m_blingfastback->setOffset(60,m_vertical_size-30);
    connect(m_blingfastback,SIGNAL(clicked()),this,SLOT(fastBackward()));

    m_blingfastforward = new MyGraphicItem(pix4,this);
    m_blingfastforward->setOffset(m_horizontal_size,m_vertical_size-30);
    connect(m_blingfastforward,SIGNAL(clicked()),this,SLOT(fastForward()));
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
void CoverBlingApplet::skipToFirst()
{
    if (m_pictureflow)
    {
	m_pictureflow->setCenterIndex(0);	
    }
}
void CoverBlingApplet::skipToLast()
{
    if (m_pictureflow)
    {
	int nbslides = m_pictureflow->slideCount();
	m_pictureflow->setCenterIndex(nbslides);
    }
}
void CoverBlingApplet::fastForward()
{
    if (m_pictureflow)
    {
	int nbslides = m_pictureflow->slideCount();
	int current = m_pictureflow->centerIndex();
	m_pictureflow->showSlide(current+nbslides/10);	
    }
}
void CoverBlingApplet::fastBackward()
{
    if (m_pictureflow)
    {
	int nbslides = m_pictureflow->slideCount();
	int current = m_pictureflow->centerIndex();
	m_pictureflow->showSlide(current-nbslides/10);	
    }
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
/*void CoverBlingApplet::constraintsEvent( Plasma::Constraints constraints )
{
    Q_UNUSED( constraints )
    prepareGeometryChange();
   
}*/
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

