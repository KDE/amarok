/****************************************************************************************
 * Copyright (c) 2009-2010 Joffrey Clavel <jclavel@clabert.info>                        *
 * Copyright (c) 2010 Alexandre Mendes <alex.mendes1988@gmail.com>                      *
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

#define DEBUG_PREFIX "ArtistWidget"

#include "ArtistWidget.h"

//Amarok
#include "amarokurls/AmarokUrl.h"
#include "core/collections/Collection.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core/collections/QueryMaker.h"
#include "core/support/Debug.h"
#include "playlist/PlaylistModelStack.h"
#include "PaletteHandler.h"
#include "SvgHandler.h"

//KDE
#include <KColorUtils>
#include <KGlobalSettings>
#include <KIcon>
#include <Plasma/Label>
#include <Plasma/PushButton>

//Qt
#include <QDesktopServices>
#include <QFontMetricsF>
#include <QGraphicsGridLayout>
#include <QGraphicsLinearLayout>
#include <QLabel>
#include <QPainter>
#include <QPixmapCache>
#include <QTextDocument>

#include <cmath>

ArtistWidget::ArtistWidget( const SimilarArtistPtr &artist,
                            QGraphicsWidget *parent, Qt::WindowFlags wFlags )
    : QGraphicsWidget( parent, wFlags )
    , m_artist( artist )
{
    setAttribute( Qt::WA_NoSystemBackground, true );

    m_image = new QLabel;
    m_image->setAttribute( Qt::WA_NoSystemBackground, true );
    m_image->setFixedSize( 128, 128 );
    QGraphicsProxyWidget *imageProxy = new QGraphicsProxyWidget( this );
    imageProxy->setWidget( m_image );

    m_nameLabel = new QLabel;
    m_match     = new QLabel;
    m_topTrackLabel = new QLabel;
    m_desc      = new Plasma::Label( this );
    m_desc      = new QGraphicsWidget( this );

    QGraphicsProxyWidget *nameProxy     = new QGraphicsProxyWidget( this );
    QGraphicsProxyWidget *matchProxy    = new QGraphicsProxyWidget( this );
    QGraphicsProxyWidget *topTrackProxy = new QGraphicsProxyWidget( this );
    nameProxy->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    matchProxy->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Fixed );
    topTrackProxy->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );

    nameProxy->setWidget( m_nameLabel );
    matchProxy->setWidget( m_match );
    topTrackProxy->setWidget( m_topTrackLabel );

    m_nameLabel->setAttribute( Qt::WA_NoSystemBackground );
    m_match->setAttribute( Qt::WA_NoSystemBackground );
    m_topTrackLabel->setAttribute( Qt::WA_NoSystemBackground );

    m_image->setAlignment( Qt::AlignCenter );
    m_match->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
    m_nameLabel->setAlignment( Qt::AlignLeft | Qt::AlignVCenter );
    m_topTrackLabel->setAlignment( Qt::AlignLeft | Qt::AlignVCenter );

    m_nameLabel->setWordWrap( false );
    m_match->setWordWrap( false );
    m_topTrackLabel->setWordWrap( false );

    m_match->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    m_topTrackLabel->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    m_match->setMinimumWidth( 10 );
    m_topTrackLabel->setMinimumWidth( 10 );
    m_nameLabel->setMinimumWidth( 10 );

    QFontMetricsF fm( font() );
    m_desc->setMinimumHeight( fm.lineSpacing() * 6 );
    m_desc->setMaximumHeight( fm.lineSpacing() * 6 );
    m_desc->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    m_descLayout.setCacheEnabled( true );

    QFont artistFont;
    artistFont.setPointSize( artistFont.pointSize() + 2 );
    artistFont.setBold( true );
    m_nameLabel->setFont( artistFont );
    m_topTrackLabel->setFont( KGlobalSettings::smallestReadableFont() );
    m_match->setFont( KGlobalSettings::smallestReadableFont() );
    m_image->setFont( KGlobalSettings::smallestReadableFont() );

    m_navigateButton = new Plasma::PushButton( this );
    m_navigateButton->setMaximumSize( QSizeF( 22, 22 ) );
    m_navigateButton->setIcon( KIcon( "edit-find" ) );
    m_navigateButton->setToolTip( i18n( "Show in Media Sources" ) );
    m_navigateButton->hide();
    connect( m_navigateButton, SIGNAL(clicked()), this, SLOT(navigateToArtist()) );
    
    m_lastfmStationButton = new Plasma::PushButton( this );
    m_lastfmStationButton->setMaximumSize( QSizeF( 22, 22 ) );
    m_lastfmStationButton->setIcon( KIcon("view-services-lastfm-amarok") );
    m_lastfmStationButton->setToolTip( i18n( "Add Last.fm artist station to the Playlist" ) );
    connect( m_lastfmStationButton, SIGNAL(clicked()), this, SLOT(addLastfmArtistStation()) );

    m_topTrackButton = new Plasma::PushButton( this );
    m_topTrackButton->setMaximumSize( QSizeF( 22, 22 ) );
    m_topTrackButton->setIcon( KIcon( "media-track-add-amarok" ) );
    m_topTrackButton->setToolTip( i18n( "Add top track to the Playlist" ) );
    m_topTrackButton->hide();
    connect( m_topTrackButton, SIGNAL(clicked()), this, SLOT(addTopTrackToPlaylist()) );

    QGraphicsLinearLayout *buttonsLayout = new QGraphicsLinearLayout( Qt::Horizontal );
    buttonsLayout->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Fixed );
    buttonsLayout->addItem( m_topTrackButton );
    buttonsLayout->addItem( m_navigateButton );
    buttonsLayout->addItem( m_lastfmStationButton );

    QString artistUrl = m_artist->url().url();
    if( !artistUrl.isEmpty() )
    {
        m_urlButton = new Plasma::PushButton( this );
        m_urlButton->setMaximumSize( QSizeF( 22, 22 ) );
        m_urlButton->setIcon( KIcon("applications-internet") );
        m_urlButton->setToolTip( i18n( "Open Last.fm webpage for this artist" ) );
        connect( m_urlButton, SIGNAL(clicked()), this, SLOT(openArtistUrl()) );
        buttonsLayout->addItem( m_urlButton );
    }

    // the image display is extended on two row
    m_layout = new QGraphicsGridLayout;
    m_layout->addItem( imageProxy, 0, 0, 3, 1 );
    m_layout->addItem( nameProxy, 0, 1 );
    m_layout->addItem( buttonsLayout, 0, 2, Qt::AlignRight );
    m_layout->addItem( topTrackProxy, 1, 1 );
    m_layout->addItem( matchProxy, 1, 2, Qt::AlignRight );
    m_layout->addItem( m_desc, 2, 1, 1, 2 );
    setLayout( m_layout );

    m_match->setText( i18n( "Match: %1%", QString::number( m_artist->match() ) ) );
    m_nameLabel->setText( m_artist->name() );

    fetchPhoto();
    queryArtist();
    setDescription( m_artist->description() );
    setTopTrack( m_artist->topTrack() );
}

ArtistWidget::~ArtistWidget()
{
}

void
ArtistWidget::fetchPhoto()
{
    // display a message for the user while the fetch of the picture
    m_image->clear();
    m_image->setText( i18n( "Loading the picture..." ) );

    QPixmap image;
    if( QPixmapCache::find( m_artist->urlImage().url(), &image ) )
    {
        m_image->setPixmap( image );
        return;
    }
    The::networkAccessManager()->getData( m_artist->urlImage(), this,
         SLOT(setImageFromInternet(KUrl,QByteArray,NetworkAccessManagerProxy::Error)), Qt::QueuedConnection );
}

void
ArtistWidget::setImageFromInternet( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e )
{
    Q_UNUSED( url );
    if( e.code != QNetworkReply::NoError )
    {
        m_image->clear();
        m_image->setText( i18n( "Unable to fetch the picture: %1", e.description ) );
        return;
    }

    QPixmap image;
    if( image.loadFromData( data ) )
    {
        image = image.scaled( 116, 116, Qt::KeepAspectRatio, Qt::SmoothTransformation );
        image = The::svgHandler()->addBordersToPixmap( image, 6, QString(), true );
        m_image->setPixmap( image );
        QPixmapCache::insert( url.url(), image );
    }
}

void
ArtistWidget::queryArtist()
{
    // Figure out of this applet is present in the local collection,
    // and show the "show in collection" button if so
    m_navigateButton->hide();

    Collections::QueryMaker *qm = CollectionManager::instance()->queryMaker();
    
    qm->setQueryType( Collections::QueryMaker::Artist );
    qm->addFilter( Meta::valArtist, m_nameLabel->text() );
    qm->limitMaxResultSize( 1 );
    qm->setAutoDelete( true );

    connect( qm, SIGNAL(newResultReady(QString,Meta::ArtistList)),
             SLOT(resultReady(QString,Meta::ArtistList)), Qt::QueuedConnection );

    qm->run();
}

SimilarArtistPtr
ArtistWidget::artist() const
{
    return m_artist;
}

void
ArtistWidget::clear()
{
    m_image->clear();
    m_nameLabel->clear();
    m_match->clear();
    m_topTrackLabel->clear();
}

void
ArtistWidget::openArtistUrl()
{
    // somehow Last.fm decides to supply this url without the scheme
    KUrl artistUrl = QString( "http://%1" ).arg( m_artist->url().url() );
    if( artistUrl.isValid() )
        QDesktopServices::openUrl( artistUrl );
}

void
ArtistWidget::setDescription( const QString &description )
{
    if( description.isEmpty() )
    {
        m_descLayout.clearLayout();
        m_descLayout.setText( i18n( "No description available in your language" ) );
    }
    else
    {
        QTextDocument doc;
        doc.setHtml( description );
        QString plain = doc.toPlainText();
        m_descLayout.setText( plain );
    }
    layoutDescription();
}

void
ArtistWidget::setTopTrack( const QString &topTrack )
{
    m_topTrackButton->hide();
    
    if( topTrack.isEmpty() )
    {
        m_topTrackLabel->setText( i18n("Top track not found") );
    }
    else
    {
        m_topTrackTitle = topTrack;
        m_topTrackLabel->setText( i18n("Top track: %1", topTrack) );

        Collections::QueryMaker *qm = CollectionManager::instance()->queryMaker();

        qm->setQueryType( Collections::QueryMaker::Track );
        qm->beginAnd();
        qm->addFilter( Meta::valArtist, m_nameLabel->text() );
        qm->addFilter( Meta::valTitle, m_topTrackTitle );
        qm->endAndOr();
        qm->limitMaxResultSize( 1 );
        qm->setAutoDelete( true );

        connect( qm, SIGNAL(newResultReady(QString,Meta::TrackList)),
                 SLOT(resultReady(QString,Meta::TrackList)) );
         
        qm->run();
    }
}

void
ArtistWidget::resizeEvent( QGraphicsSceneResizeEvent *event )
{
    QGraphicsWidget::resizeEvent( event );
    layoutDescription();
    QFontMetrics fm( m_match->font() );
    m_match->setMaximumWidth( fm.width( m_match->text() ) );
}

void
ArtistWidget::paint( QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *widget )
{
    QGraphicsWidget::paint( p, option, widget );
    m_descLayout.draw( p, m_desc->geometry().topLeft() );

    // fade out the last bit of text if not all of the description is shown
    if( m_descCropped && m_descLayout.lineCount() > 0 )
    {
        QFontMetricsF fm( m_desc->font() );
        QRectF rect = fm.boundingRect( m_desc->geometry(), Qt::AlignCenter, "placeholder" );
        rect.moveTo( m_desc->geometry().bottomRight() );
        QTextLine lastLine = m_descLayout.lineAt( m_descLayout.lineCount() - 1 );
        qreal xoffset = lastLine.naturalTextWidth() - lastLine.width() - rect.width();
        rect = rect.translated( xoffset, -rect.height() );

        QColor bgColor = The::paletteHandler()->highlightColor( 0.4, 1.05 );
        bgColor.setAlphaF( bgColor.alphaF() * 0.5 );
        QColor winColor = The::paletteHandler()->palette().window().color();
        QColor fadeColor = KColorUtils::overlayColors( winColor, bgColor );

        QLinearGradient alphaGradient( 0, 0, 1, 0 );
        alphaGradient.setCoordinateMode( QGradient::ObjectBoundingMode );
        alphaGradient.setColorAt( 0, QColor(0, 0, 0, 0) );
        alphaGradient.setColorAt( 1, fadeColor );
        p->fillRect( rect, alphaGradient );
    }
}

void
ArtistWidget::layoutDescription()
{
    QFontMetricsF fm( m_desc->font() );
    QRectF geom = m_desc->geometry();
    int maxLines = floor( m_desc->size().height() / fm.lineSpacing() );
    int leading = fm.leading();
    qreal height = 0;
    m_descCropped = true;
    m_descLayout.clearLayout();
    m_descLayout.beginLayout();
    while( m_descLayout.lineCount() < maxLines )
    {
        QTextLine line = m_descLayout.createLine();
        if( !line.isValid() )
        {
            m_descCropped = false;
            break;
        }

        line.setLineWidth( geom.width() );
        height += leading;
        line.setPosition( QPointF(0, height) );
        height += line.height();
    }
    m_descLayout.endLayout();
    update();
}

void
ArtistWidget::addTopTrackToPlaylist()
{
    The::playlistController()->insertOptioned( m_topTrack, Playlist::AppendAndPlay );
}

void
ArtistWidget::navigateToArtist()
{
    AmarokUrl url;
    url.setCommand( "navigate" );
    url.setPath( "collections" );
    url.appendArg( "filter", "artist:\"" + m_artist->name() + "\"" );
    url.run();
}

void
ArtistWidget::addLastfmArtistStation()
{
    const QString url = "lastfm://artist/" + m_artist->name() + "/similarartists";
    Meta::TrackPtr lastfmtrack = CollectionManager::instance()->trackForUrl( KUrl( url ) );
    The::playlistController()->insertOptioned( lastfmtrack, Playlist::AppendAndPlay );
}

void
ArtistWidget::resultReady( const QString &collectionId, const Meta::ArtistList &artists )
{
    Q_UNUSED( collectionId )
    if( artists.length() > 0 )
        m_navigateButton->show();
}

ArtistsListWidget::ArtistsListWidget( QGraphicsWidget *parent )
    : Plasma::ScrollWidget( parent )
    , m_separatorCount( 0 )
{
    m_layout = new QGraphicsLinearLayout( Qt::Vertical );
    QGraphicsWidget *content = new QGraphicsWidget( this );
    content->setLayout( m_layout );
    setWidget( content );
}

ArtistsListWidget::~ArtistsListWidget()
{
    clear();
}

int
ArtistsListWidget::count() const
{
    return m_layout->count() - m_separatorCount;
}

void
ArtistsListWidget::addItem( ArtistWidget *widget )
{
    m_layout->addItem( widget );
    m_widgets << widget;
    addSeparator();
}

void
ArtistsListWidget::addArtist( const SimilarArtistPtr &artist )
{
    ArtistWidget *widget = new ArtistWidget( artist );
    m_layout->addItem( widget );
    m_widgets << widget;
    addSeparator();
}

void
ArtistsListWidget::addArtists( const SimilarArtist::List &artists )
{
    foreach( const SimilarArtistPtr &artist, artists )
        addArtist( artist );
}

void
ArtistsListWidget::addSeparator()
{
    // can also use Plasma::Separator here but that's in kde 4.4
    QFrame *separator = new QFrame;
    separator->setFrameStyle( QFrame::HLine );
    separator->setAutoFillBackground( false );
    QGraphicsProxyWidget *separatorProxy = new QGraphicsProxyWidget;
    separatorProxy->setWidget( separator );
    m_layout->addItem( separatorProxy );
    ++m_separatorCount;
}

void
ArtistsListWidget::clear()
{
    qDeleteAll( m_widgets );
    m_widgets.clear();
    int count = m_layout->count();
    if( count > 0 )
    {
        while( --count >= 0 )
        {
            QGraphicsLayoutItem *child = m_layout->itemAt( 0 );
            m_layout->removeItem( child );
            delete child;
        }
        widget()->resize( size().width(), 0 );
        m_separatorCount = 0;
    }
}

bool
ArtistsListWidget::isEmpty() const
{
    return count() == 0;
}

QString
ArtistsListWidget::name() const
{
    return m_name;
}

void
ArtistsListWidget::setName( const QString &name )
{
    m_name = name;
}

void
ArtistWidget::resultReady( const QString &collectionId, const Meta::TrackList &tracks )
{
    Q_UNUSED( collectionId )
    if( !tracks.isEmpty() )
    {
        m_topTrack = tracks.first();
        m_navigateButton->show();
        m_topTrackButton->show();
    }
}

void
ArtistsListWidget::setDescription( const QString &artist, const QString &description )
{
    foreach( ArtistWidget *widget, m_widgets )
    {
        if( widget->artist()->name() == artist )
            widget->setDescription( description );
    }
}

void
ArtistsListWidget::setTopTrack( const QString &artist, const QString &track )
{
    foreach( ArtistWidget *widget, m_widgets )
    {
        if( widget->artist()->name() == artist )
            widget->setTopTrack( track );
    }
}
