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
#include "PaletteHandler.h"
#include "SvgHandler.h"
#include "amarokurls/AmarokUrl.h"
#include "core/collections/Collection.h"
#include "core/collections/QueryMaker.h"
#include "core/meta/Meta.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "playlist/PlaylistController.h"

//KDE
#include <KColorUtils>
#include <KGlobalSettings>
#include <QIcon>
#include <Plasma/PushButton>
#include <Plasma/Separator>

//Qt
#include <QDesktopServices>
#include <QFontMetricsF>
#include <QGraphicsGridLayout>
#include <QGraphicsLinearLayout>
#include <QLabel>
#include <QPainter>
#include <QPixmapCache>
#include <QSignalMapper>
#include <QTextDocument>
#include <QTimer>

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
    m_image->setCursor( Qt::PointingHandCursor );
    QGraphicsProxyWidget *imageProxy = new QGraphicsProxyWidget( this );
    imageProxy->setWidget( m_image );
    m_image->installEventFilter( this );

    m_nameLabel = new QLabel;
    m_match     = new QLabel;
    m_tagsLabel = new QLabel;
    m_topTrackLabel = new QLabel;
    m_bio = new QGraphicsWidget( this );

    QGraphicsProxyWidget *nameProxy     = new QGraphicsProxyWidget( this );
    QGraphicsProxyWidget *matchProxy    = new QGraphicsProxyWidget( this );
    QGraphicsProxyWidget *topTrackProxy = new QGraphicsProxyWidget( this );
    QGraphicsProxyWidget *tagsProxy     = new QGraphicsProxyWidget( this );
    nameProxy->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
    matchProxy->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Preferred );
    topTrackProxy->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
    tagsProxy->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
    imageProxy->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );

    nameProxy->setWidget( m_nameLabel );
    matchProxy->setWidget( m_match );
    topTrackProxy->setWidget( m_topTrackLabel );
    tagsProxy->setWidget( m_tagsLabel );

    m_nameLabel->setAttribute( Qt::WA_NoSystemBackground );
    m_match->setAttribute( Qt::WA_NoSystemBackground );
    m_topTrackLabel->setAttribute( Qt::WA_NoSystemBackground );
    m_tagsLabel->setAttribute( Qt::WA_NoSystemBackground );

    m_image->setAlignment( Qt::AlignCenter );
    m_match->setAlignment( Qt::AlignRight | Qt::AlignVCenter );
    m_nameLabel->setAlignment( Qt::AlignLeft | Qt::AlignVCenter );
    m_topTrackLabel->setAlignment( Qt::AlignLeft | Qt::AlignVCenter );
    m_tagsLabel->setAlignment( Qt::AlignLeft | Qt::AlignVCenter );

    m_nameLabel->setWordWrap( false );
    m_match->setWordWrap( false );
    m_topTrackLabel->setWordWrap( false );
    m_tagsLabel->setWordWrap( false );

    m_match->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
    m_topTrackLabel->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
    m_match->setMinimumWidth( 10 );
    m_topTrackLabel->setMinimumWidth( 10 );
    m_nameLabel->setMinimumWidth( 10 );
    m_tagsLabel->setMinimumWidth( 10 );

    QFontMetricsF fm( font() );
    m_bio->setMinimumHeight( fm.lineSpacing() * 5 );
    m_bio->setMaximumHeight( fm.lineSpacing() * 5 );
    m_bio->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );
    m_bioLayout.setCacheEnabled( true );

    QFont artistFont;
    artistFont.setPointSize( artistFont.pointSize() + 2 );
    artistFont.setBold( true );
    m_nameLabel->setFont( artistFont );
    m_topTrackLabel->setFont( KGlobalSettings::smallestReadableFont() );
    m_tagsLabel->setFont( KGlobalSettings::smallestReadableFont() );
    m_match->setFont( KGlobalSettings::smallestReadableFont() );

    m_navigateButton = new Plasma::PushButton( this );
    m_navigateButton->setMaximumSize( QSizeF( 22, 22 ) );
    m_navigateButton->setIcon( QIcon::fromTheme( "edit-find" ) );
    m_navigateButton->setToolTip( i18n( "Show in Media Sources" ) );
    connect( m_navigateButton, SIGNAL(clicked()), this, SLOT(navigateToArtist()) );
    
    m_lastfmStationButton = new Plasma::PushButton( this );
    m_lastfmStationButton->setMaximumSize( QSizeF( 22, 22 ) );
    m_lastfmStationButton->setIcon( QIcon::fromTheme("view-services-lastfm-amarok") );
    m_lastfmStationButton->setToolTip( i18n( "Add Last.fm artist station to the Playlist" ) );
    connect( m_lastfmStationButton, SIGNAL(clicked()), this, SLOT(addLastfmArtistStation()) );

    m_topTrackButton = new Plasma::PushButton( this );
    m_topTrackButton->setMaximumSize( QSizeF( 22, 22 ) );
    m_topTrackButton->setIcon( QIcon::fromTheme( "media-track-add-amarok" ) );
    m_topTrackButton->setToolTip( i18n( "Add top track to the Playlist" ) );
    m_topTrackButton->hide();
    connect( m_topTrackButton, SIGNAL(clicked()), this, SLOT(addTopTrackToPlaylist()) );

    m_similarArtistButton = new Plasma::PushButton( this );
    m_similarArtistButton->setMaximumSize( QSizeF( 22, 22 ) );
    m_similarArtistButton->setIcon( QIcon::fromTheme( "similarartists-amarok" ) );
    m_similarArtistButton->setToolTip( i18n( "Show Similar Artists to %1", m_artist->name() ) );
    connect( m_similarArtistButton, SIGNAL(clicked()), this, SIGNAL(showSimilarArtists()) );

    QGraphicsLinearLayout *buttonsLayout = new QGraphicsLinearLayout( Qt::Horizontal );
    buttonsLayout->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Preferred );
    buttonsLayout->addItem( m_topTrackButton );
    buttonsLayout->addItem( m_navigateButton );
    buttonsLayout->addItem( m_lastfmStationButton );

    QString artistUrl = m_artist->url().url();
    if( !artistUrl.isEmpty() )
    {
        m_urlButton = new Plasma::PushButton( this );
        m_urlButton->setMaximumSize( QSizeF( 22, 22 ) );
        m_urlButton->setIcon( QIcon::fromTheme("internet-services") );
        m_urlButton->setToolTip( i18n( "Open Last.fm webpage for this artist" ) );
        connect( m_urlButton, SIGNAL(clicked()), this, SLOT(openArtistUrl()) );
        buttonsLayout->addItem( m_urlButton );
    }

    buttonsLayout->addItem( m_similarArtistButton );

    // the image display is extended on two row
    m_layout = new QGraphicsGridLayout( this );
    m_layout->addItem( imageProxy, 0, 0, 4, 1 );
    m_layout->addItem( nameProxy, 0, 1 );
    m_layout->addItem( buttonsLayout, 0, 2, Qt::AlignRight );
    m_layout->addItem( topTrackProxy, 1, 1 );
    m_layout->addItem( matchProxy, 1, 2, Qt::AlignRight );
    m_layout->addItem( tagsProxy, 2, 1, 1, 2 );
    m_layout->addItem( m_bio, 3, 1, 1, 2 );

    m_match->setText( i18n( "Match: %1%", QString::number( m_artist->match() ) ) );
    m_nameLabel->setText( m_artist->name() );

    QTimer::singleShot( 0, this, SLOT(updateInfo()) );
}

void
ArtistWidget::updateInfo()
{
    fetchPhoto();
    fetchInfo();
    fetchTopTrack();
}

ArtistWidget::~ArtistWidget()
{
    clear();
}

bool
ArtistWidget::eventFilter( QObject *obj, QEvent *event )
{
    if( obj == m_image )
    {
        if( event->type() == QEvent::MouseButtonPress )
        {
            emit showBio();
            event->accept();
            return true;
        }
    }
    return QGraphicsWidget::eventFilter( obj, event );
}

void
ArtistWidget::fetchPhoto()
{
    // display a message for the user while the fetch of the picture
    m_image->clear();

    QPixmap image;
    if( QPixmapCache::find( m_artist->urlImage().url(), &image ) )
    {
        m_image->setPixmap( image );
        return;
    }
    m_image->setPixmap( Amarok::semiTransparentLogo( 120 ) );

    if( m_artist->urlImage().isEmpty() )
        return;

    The::networkAccessManager()->getData( m_artist->urlImage(), this,
         SLOT(photoFetched(QUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
}

void
ArtistWidget::fetchInfo()
{
    // we generate the url for the demand on the lastFM Api
    QUrl url;
    url.setScheme( "http" );
    url.setHost( "ws.audioscrobbler.com" );
    url.setPath( "/2.0/" );
    url.addQueryItem( "method", "artist.getInfo" );
    url.addQueryItem( "api_key", Amarok::lastfmApiKey() );
    url.addQueryItem( "artist", m_artist->name() );

    The::networkAccessManager()->getData( url, this,
         SLOT(parseInfo(QUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
}

void
ArtistWidget::fetchTopTrack()
{
    // we generate the url for the demand on the lastFM Api
    QUrl url;
    url.setScheme( "http" );
    url.setHost( "ws.audioscrobbler.com" );
    url.setPath( "/2.0/" );
    url.addQueryItem( "method", "artist.getTopTracks" );
    url.addQueryItem( "api_key", Amarok::lastfmApiKey() );
    url.addQueryItem( "artist",  m_artist->name() );

    The::networkAccessManager()->getData( url, this,
         SLOT(parseTopTrack(QUrl,QByteArray,NetworkAccessManagerProxy::Error)) );
}

void
ArtistWidget::photoFetched( const QUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e )
{
    if( url != m_artist->urlImage() )
        return;

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
        m_image->setToolTip( i18nc( "@info:tooltip Artist biography", "Show Biography" ) );
        m_image->setPixmap( image );
        QPixmapCache::insert( url.url(), image );
    }
}

void
ArtistWidget::parseInfo( const QUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e )
{
    Q_UNUSED( url )
    if( e.code != QNetworkReply::NoError )
        return;

    if( data.isEmpty() )
        return;

    QXmlStreamReader xml( data );
    xml.readNextStartElement(); // lfm
    if( xml.attributes().value(QLatin1String("status")) != QLatin1String("ok") )
    {
        setBioSummary( QString() );
        return;
    }

    QString summary;
    xml.readNextStartElement(); // artist
    while( xml.readNextStartElement() )
    {
        if( xml.name() == QLatin1String("tags") )
        {
            m_tags.clear();
            while( xml.readNextStartElement() )
            {
                if( xml.name() != QLatin1String("tag") )
                    continue;

                while( xml.readNextStartElement() )
                {
                    if( xml.name() == QLatin1String("name") )
                        m_tags << xml.readElementText();
                    else
                        xml.skipCurrentElement();
                }
            }
        }
        else if( xml.name() == QLatin1String("bio") )
        {
            while( xml.readNextStartElement() )
            {
                if( xml.name() == QLatin1String("published") )
                    m_fullBio.first = KDateTime::fromString( xml.readElementText(), "%a, %d %b %Y %H:%M:%S" );
                else if( xml.name() == QLatin1String("summary") )
                    summary = xml.readElementText().simplified();
                else if( xml.name() == QLatin1String("content") )
                    m_fullBio.second = xml.readElementText().replace( QRegularExpression(QLatin1String("\n+")), QLatin1String("<br>") );
                else
                    xml.skipCurrentElement();
            }
        }
        else
            xml.skipCurrentElement();
    }
    setBioSummary( summary );
    setTags();
}

void
ArtistWidget::parseTopTrack( const QUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e )
{
    Q_UNUSED( url )
    if( e.code != QNetworkReply::NoError )
        return;

    if( data.isEmpty() )
        return;

    QXmlStreamReader xml( data );
    xml.readNextStartElement(); // lfm
    if( xml.attributes().value(QLatin1String("status")) != QLatin1String("ok") )
    {
        setTopTrack( QString() );
        return;
    }

    QString topTrack;
    xml.readNextStartElement(); // toptracks
    while( xml.readNextStartElement() )
    {
        if( xml.name() != QLatin1String("track") )
        {
            xml.skipCurrentElement();
            continue;
        }

        while( xml.readNextStartElement() )
        {
            if( xml.name() != QLatin1String("name") )
            {
                xml.skipCurrentElement();
                continue;
            }
            topTrack = xml.readElementText();
            break;
        }

        if( !topTrack.isEmpty() )
            break;
    }
    setTopTrack( topTrack );
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
    QUrl artistUrl = QString( "http://%1" ).arg( m_artist->url().url() );
    if( artistUrl.isValid() )
        QDesktopServices::openUrl( artistUrl );
}

void
ArtistWidget::setBioSummary( const QString &bio )
{
    if( bio.isEmpty() )
    {
        m_bioLayout.clearLayout();
        m_bioLayout.setText( i18n( "No description available." ) );
    }
    else
    {
        QTextDocument doc;
        doc.setHtml( bio );
        QString plain = doc.toPlainText();
        m_bioLayout.setText( plain );
    }
    layoutBio();
}

void
ArtistWidget::setTags()
{
    QString tags = m_tags.isEmpty() ? i18n( "none" ) : m_tags.join( QLatin1String(", ") );
    QString label = i18nc( "@label:textbox", "Tags: %1", tags );
    m_tagsLabel->setText( label );
}

void
ArtistWidget::setTopTrack( const QString &topTrack )
{
    if( topTrack.isEmpty() )
    {
        m_topTrackLabel->setText( i18n("Top track not found") );
        m_topTrackButton->hide();
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

        connect( qm, SIGNAL(newResultReady(Meta::TrackList)),
                 SLOT(resultReady(Meta::TrackList)) );
         
        qm->run();
    }
}

void
ArtistWidget::resizeEvent( QGraphicsSceneResizeEvent *event )
{
    QGraphicsWidget::resizeEvent( event );
    layoutBio();
    QFontMetrics fm( m_match->font() );
    m_match->setMaximumWidth( fm.horizontalAdvance( m_match->text() ) );
}

void
ArtistWidget::paint( QPainter *p, const QStyleOptionGraphicsItem *option, QWidget *widget )
{
    QGraphicsWidget::paint( p, option, widget );

    p->save();
    QFontMetricsF fm( m_bio->font() );
    QPointF pos = m_bio->geometry().topLeft();
    const int maxLines = floor( m_bio->size().height() / fm.lineSpacing() );
    for( int i = 0, lines = m_bioLayout.lineCount(); i < lines; ++i )
    {
        const QTextLine &line = m_bioLayout.lineAt( i );
        if( m_bioCropped && (i == (maxLines - 1)) )
        {
            // fade out the last bit of text if not all of the bio is shown
            QLinearGradient alphaGradient( 0, 0, 1, 0 );
            alphaGradient.setCoordinateMode( QGradient::ObjectBoundingMode );
            const QColor &textColor = The::paletteHandler()->palette().text().color();
            alphaGradient.setColorAt( 0, textColor );
            alphaGradient.setColorAt( 0.85, textColor );
            alphaGradient.setColorAt( 1, Qt::transparent );
            QPen pen = p->pen();
            pen.setBrush( alphaGradient );
            p->setPen( pen );
        }
        line.draw( p, pos );
        pos.ry() += line.leading();
    }
    p->restore();
}

void
ArtistWidget::layoutBio()
{
    QFontMetricsF fm( m_bio->font() );
    QRectF geom = m_bio->geometry();
    int maxLines = floor( m_bio->size().height() / fm.lineSpacing() );
    int leading = fm.leading();
    qreal height = 0;
    m_bioCropped = true;
    m_bioLayout.clearLayout();
    m_bioLayout.beginLayout();
    while( m_bioLayout.lineCount() < maxLines )
    {
        QTextLine line = m_bioLayout.createLine();
        if( !line.isValid() )
        {
            m_bioCropped = false;
            break;
        }

        line.setLineWidth( geom.width() );
        height += leading;
        line.setPosition( QPointF(0, height) );
        height += line.height();
    }
    m_bioLayout.endLayout();
    update();
}

KDateTime
ArtistWidget::bioPublished() const
{
    return m_fullBio.first;
}

QString
ArtistWidget::fullBio() const
{
    return m_fullBio.second;
}

void
ArtistWidget::addTopTrackToPlaylist()
{
    The::playlistController()->insertOptioned( m_topTrack, Playlist::OnAppendToPlaylistAction );
}

void
ArtistWidget::navigateToArtist()
{
    AmarokUrl url;
    url.setCommand( "navigate" );
    url.setPath( "collections" );
    url.setArg( "filter", "artist:\"" + AmarokUrl::escape( m_artist->name() ) + '\"' );
    url.run();
}

void
ArtistWidget::addLastfmArtistStation()
{
    const QString url = "lastfm://artist/" + m_artist->name() + "/similarartists";
    Meta::TrackPtr lastfmtrack = CollectionManager::instance()->trackForUrl( QUrl( url ) );
    The::playlistController()->insertOptioned( lastfmtrack, Playlist::OnAppendToPlaylistAction );
}

ArtistsListWidget::ArtistsListWidget( QGraphicsWidget *parent )
    : Plasma::ScrollWidget( parent )
    , m_separatorCount( 0 )
{
    QGraphicsWidget *content = new QGraphicsWidget( this );
    m_layout = new QGraphicsLinearLayout( Qt::Vertical, content );
    setWidget( content );

    m_showArtistsSigMapper = new QSignalMapper( this );
    connect( m_showArtistsSigMapper, SIGNAL(mapped(QString)), SIGNAL(showSimilarArtists(QString)) );

    m_showBioSigMapper = new QSignalMapper( this );
    connect( m_showBioSigMapper, SIGNAL(mapped(QString)), SIGNAL(showBio(QString)) );
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
ArtistsListWidget::addArtist( const SimilarArtistPtr &artist )
{
    if( !m_widgets.isEmpty() )
        addSeparator();
    ArtistWidget *widget = new ArtistWidget( artist );
    const QString &name = artist->name();
    connect( widget, SIGNAL(showSimilarArtists()), m_showArtistsSigMapper, SLOT(map()) );
    m_showArtistsSigMapper->setMapping( widget, name );
    connect( widget, SIGNAL(showBio()), m_showBioSigMapper, SLOT(map()) );
    m_showBioSigMapper->setMapping( widget, name );
    m_layout->addItem( widget );
    m_widgets << widget;
}

void
ArtistsListWidget::addArtists( const SimilarArtist::List &artists )
{
    for( const SimilarArtistPtr &artist : artists )
        addArtist( artist );
    updateGeometry();
}

void
ArtistsListWidget::addSeparator()
{
    m_layout->addItem( new Plasma::Separator );
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
        m_separatorCount = 0;
    }
    m_layout->invalidate();
    updateGeometry();
}

QSizeF
ArtistsListWidget::sizeHint( Qt::SizeHint which, const QSizeF &constraint ) const
{
    QSizeF sz = Plasma::ScrollWidget::sizeHint( which, constraint );
    if( count() == 0 )
        sz.rheight() = 0;
    return sz;
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

ArtistWidget *
ArtistsListWidget::widget( const QString &artistName )
{
    for( ArtistWidget *widget : m_widgets )
    {
        if( widget->artist()->name() == artistName )
            return widget;
    }
    return 0;
}

void
ArtistWidget::resultReady( const Meta::TrackList &tracks )
{
    if( !tracks.isEmpty() )
    {
        m_topTrack = tracks.first();
        m_topTrackButton->show();
    }
}

