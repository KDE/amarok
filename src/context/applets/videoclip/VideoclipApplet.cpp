/****************************************************************************************
 * Copyright (c) 2009 Simon Esneault <simon.esneault@gmail.com>                         *
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

#define DEBUG_PREFIX "VideoclipApplet"

#include "VideoclipApplet.h" 

#include "CustomVideoWidget.h"
#include "PaletteHandler.h"
#include "VideoItemButton.h"

// Amarok
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "EngineController.h"
#include "core-impl/meta/stream/Stream.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "context/ContextView.h"
#include "context/Svg.h"
#include "context/widgets/RatingWidget.h"
#include "playlist/PlaylistModelStack.h"
#include "SvgHandler.h"
#include "widgets/TextScrollingWidget.h"

// KDE
#include <KColorScheme>
#include <KConfigDialog>
#include <KStandardDirs>
#include <KVBox>
#include <Plasma/Theme>
#include <Plasma/BusyWidget>
#include <Plasma/IconWidget>
#include <Plasma/Label>
#include <Plasma/Separator>
#include <Plasma/ScrollWidget>
#include <Phonon/MediaObject>
#include <Phonon/Path>
#include <Phonon/VideoWidget>

// Qt
#include <QGraphicsLinearLayout>
#include <QGraphicsProxyWidget>
#include <QGraphicsSceneResizeEvent>
#include <QGraphicsWidget>
#include <QGridLayout>
#include <QLabel>
#include <QPainter>
#include <QToolButton>
#include <QScrollArea>
#include <QScrollBar>

Q_DECLARE_METATYPE ( VideoInfo *)
K_EXPORT_AMAROK_APPLET( videoclip, VideoclipApplet )

/**
 * Plasma applet for showing a videoclip in the context view
 */
VideoclipApplet::VideoclipApplet( QObject* parent, const QVariantList& args )
        : Context::Applet( parent, args )
        , m_settingsIcon( 0 )
        , m_youtubeHQ( false )
{
    DEBUG_BLOCK
    setHasConfigurationInterface( true );

    EngineController *engine = The::engineController();

    connect( engine, SIGNAL( trackPlaying( Meta::TrackPtr ) ),
             this, SLOT( trackPlaying() ) );
    connect( engine, SIGNAL( stopped( qint64, qint64 ) ),
             this, SLOT( stopped() ) );

    const Phonon::MediaObject *media = engine->phononMediaObject();

    connect( media, SIGNAL( stateChanged( Phonon::State, Phonon::State ) ),
             this, SLOT( stateChanged( Phonon::State, Phonon::State ) ) );

    // setMaximumHeight( 300 );
    // setPreferredHeight( 300 );
}

void
VideoclipApplet::init()
{
    DEBUG_BLOCK

    // Call the base implementation.
    Context::Applet::init();

    // CustomWidget is a special VideoWidget for interaction
    m_videoWidget = new CustomVideoWidget();
    m_videoWidget.data()->setParent( Context::ContextView::self()->viewport(), Qt::SubWindow | Qt::FramelessWindowHint );
    m_videoWidget.data()->hide();
    
    // we create path no need to add a lot of fancy thing 
    Phonon::createPath( const_cast<Phonon::MediaObject*>( The::engineController()->phononMediaObject() ), m_videoWidget.data() );
    
    // Load pixmap
    m_pixYoutube = QPixmap( KStandardDirs::locate( "data", "amarok/images/amarok-videoclip-youtube.png" ) );
    m_pixDailymotion = QPixmap( KStandardDirs::locate( "data", "amarok/images/amarok-videoclip-dailymotion.png" ) );
    m_pixVimeo = QPixmap( KStandardDirs::locate( "data", "amarok/images/amarok-videoclip-vimeo.png" ) );

    QAction* langAction = new QAction( this );
    langAction->setIcon( KIcon( "preferences-system" ) );
    langAction->setVisible( true );
    langAction->setEnabled( true );
    langAction->setText( i18n( "Settings" ) );
    m_settingsIcon = addAction( langAction );
    connect( m_settingsIcon, SIGNAL( clicked() ), this, SLOT( showConfigurationInterface() ) );
    
    // Create label
    QFont labelFont;
    labelFont.setPointSize( labelFont.pointSize() + 2 );
    m_headerText = new TextScrollingWidget( this );
    m_headerText->setBrush( Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor ) );
    m_headerText->setFont( labelFont );
    m_headerText->setText( i18n( "Video Clip" ) );
    m_headerText->setDrawBackground( true );
    m_headerText->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );

    // Set the collapse size
    setCollapseHeight( m_headerText->boundingRect().height() + 2 * standardPadding() + 2 );

    // Create layout
    m_scrollLayout = new QGraphicsLinearLayout( Qt::Horizontal );
    m_scrollLayout->setContentsMargins( 8, 0, 8, 0 );
    m_scrollLayout->setSpacing( 2 );
    m_scrollLayout->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );

    // create a scroll Area
    m_scroll = new Plasma::ScrollWidget( this );
    QGraphicsWidget *scrollContent = new QGraphicsWidget( this );
    scrollContent->setLayout( m_scrollLayout );
    m_scroll->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    m_scroll->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    m_scroll->setWidget( scrollContent );

    QGraphicsLinearLayout *headerLayout = new QGraphicsLinearLayout;
    headerLayout->addItem( m_headerText );
    headerLayout->addItem( m_settingsIcon );
    headerLayout->setContentsMargins( 0, 4, 0, 2 );
    headerLayout->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );

    m_layout = new QGraphicsLinearLayout( Qt::Vertical, this );
    m_layout->addItem( headerLayout );
    m_layout->addItem( m_scroll );
    m_layout->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );

    connectSource( "videoclip" );
    connect( dataEngine( "amarok-videoclip" ), SIGNAL( sourceAdded( const QString & ) ),
             this, SLOT( connectSource( const QString & ) ) );

    // Read config and inform the engine.
    KConfigGroup config = Amarok::config("Videoclip Applet");
    m_youtubeHQ = config.readEntry( "YoutubeHQ", false );
    dataEngine( "amarok-videoclip" )->query( QString( "videoclip:youtubeHQ:" ) + QString().setNum( m_youtubeHQ ) );

    setCollapseOn();
    updateConstraints();
    update();
}

VideoclipApplet::~VideoclipApplet()
{
    delete m_videoWidget.data();
    qDeleteAll( m_videoItemButtons );
}

void
VideoclipApplet::trackPlaying()
{
    DEBUG_BLOCK
    // on new track, we expand the applet if not already
    setCollapseOff();
    // m_layout->addItem( m_scroll );
    // m_scroll->show();
    // m_videoWidget.data()->show();
    setMinimumHeight( 300 );
    emit sizeHintChanged( Qt::MinimumSize );
    layout()->invalidate();
}

void
VideoclipApplet::stateChanged(Phonon::State currentState, Phonon::State oldState)
{
    // DEBUG_BLOCK
    // debug() << "video old state: " << oldState << " new state: " << currentState;

    if( currentState == oldState )
        return;

    switch( currentState )
    {
        // when switching from buffering to to playing, we launch the vid widget
        case Phonon::PlayingState:
        {
            // We need this as when song switching the state will do
            // playing > stopped > playing > loading > buffering > playing

            // --well, not on OS X. there it will go oldState == stopped,
            // newState == playing, after which the track actually starts playing
            // adding StoppedState below makes the videoapplet work for video podcasts.
            // I suggest adding a special case for OS X if this breaks on other platforms - max
            if( oldState == Phonon::BufferingState || oldState == Phonon::StoppedState )
            {
                debug() <<" video state : playing";
                setCollapseOff();
                setMinimumHeight( 300 );
                emit sizeHintChanged( Qt::MinimumSize );
                layout()->invalidate();

                if( The::engineController()->phononMediaObject()->hasVideo() )
                {
                    setBusy( false );
                    debug() << "Show VideoWidget";
                    m_scroll->hide();
                    m_videoWidget.data()->show();
                    m_videoWidget.data()->activateWindow();
                    Phonon::createPath( const_cast<Phonon::MediaObject*>( The::engineController()->phononMediaObject() ), m_videoWidget.data() );
                    if( m_videoWidget.data()->isActiveWindow() ) {
                        //FIXME dual-screen this seems to still show
                        QContextMenuEvent e( QContextMenuEvent::Other, QPoint() );
                        QApplication::sendEvent( m_videoWidget.data(), &e );
                    }
                }
                else
                {
                    debug() << "Hide VideoWidget";
                    m_videoWidget.data()->hide();
                    m_scroll->show();
                }
            }
            break;
        }

        // When buffering/loading  a vid, we want the nice Busy animation, and no collapsing
        case Phonon::BufferingState:
        case Phonon::LoadingState:
        {
            debug() <<" video state : buffering";

            setBusy( true );
            m_videoWidget.data()->hide();
            m_scroll->hide();
            break;
        }

        default:
            debug() <<" video state : unknown";
            setCollapseOn();
            break;
    }
}

void
VideoclipApplet::stopped()
{
    DEBUG_BLOCK
    // On playback ending, we hide everything and collapse
    setBusy( false );
    m_scroll->hide();
    m_videoWidget.data()->hide();
    // m_layout->removeItem( m_scroll );
    setCollapseOn();
}

void
VideoclipApplet::resizeEvent( QGraphicsSceneResizeEvent * event )
{
    DEBUG_BLOCK
    Context::Applet::resizeEvent( event );
    debug() << "new:" << event->newSize() << "old:" << event->oldSize();
}

void 
VideoclipApplet::constraintsEvent( Plasma::Constraints constraints )
{
    Context::Applet::constraintsEvent( constraints );
    m_headerText->setScrollingText( i18n( "Video Clip" ) );
}

void 
VideoclipApplet::connectSource( const QString &source )
{
    if ( source == "videoclip" )
        dataEngine( "amarok-videoclip" )->connectSource( "videoclip", this );
}

void 
VideoclipApplet::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data ) // SLOT
{
    Q_UNUSED( name )
    if( data.empty() )
        return;
    
    DEBUG_BLOCK
    if ( !m_videoWidget.data()->isVisible() && !The::engineController()->phononMediaObject()->hasVideo() )
    {
        // int width = 130;
        // Properly delete previsouly allocated item
        int count = m_scrollLayout->count();
        if( count > 0 )
        {
            while( --count >= 0 )
            {
                QGraphicsLayoutItem *child = m_scrollLayout->itemAt( 0 );
                m_scrollLayout->removeItem( child );
                delete child;
            }
        }
        
        // if we get a message, show it
        if ( data.contains( "message" ) && data["message"].toString().contains("Fetching"))
        {
            m_headerText->setScrollingText( i18n( "Video Clip" ) );
            updateConstraints();
            update();
            debug() <<" message fetching ";
            m_scroll->hide();
            // m_layout->removeItem( m_scroll );
			setBusy( true );
        }
		else if ( data.contains( "message" ) )
		{
            //if nothing found, we collapse and inform user
            m_headerText->setScrollingText( i18n( "Video Clip " ) + ':' + i18n( " No information found..." ) );
            update();
			setBusy( false );
            m_scroll->hide();
            // m_layout->removeItem( m_scroll );
            setCollapseOn();
		}
        else if ( data.contains( "item:0" ) )
        {
            m_headerText->setScrollingText( i18n( "Video Clip" ) );
            setCollapseOff();
            m_scroll->show();

			setBusy(false);
            for (int i=0; i< data.size(); i++ )
            {
                VideoInfo *item = data[ QString ("item:" )+QString().setNum(i) ].value<VideoInfo *>() ;
                if( !( item->url.isEmpty() ) ) // prevent some weird stuff ...
                {
                    VideoItemButton *vidButton = new VideoItemButton();
                    vidButton->setVideoInfo( item );
                    vidButton->setAttribute( Qt::WA_NoSystemBackground );
                    m_videoItemButtons.append( vidButton );

                    connect( vidButton, SIGNAL( appendRequested( VideoInfo * ) ), this, SLOT ( appendVideoClip( VideoInfo * ) ) );
                    connect( vidButton, SIGNAL( queueRequested( VideoInfo* ) ), this, SLOT ( queueVideoClip( VideoInfo * ) ) );
                    connect( vidButton, SIGNAL( appendPlayRequested( VideoInfo * ) ), this, SLOT ( appendPlayVideoClip( VideoInfo * ) ) );

                    // create link (and resize, no more than 3 lines long)
                    QString title( item->title );
                    if( title.size() > 45 )
                        title.resize( 45 );

                    QLabel *duration =  new QLabel( item->duration + QString( "<br>" ) + item->views + QString( " views" ) );
                    duration->setAttribute( Qt::WA_NoSystemBackground );
                    duration->setAlignment( Qt::AlignCenter );

                    QLabel *webi = new QLabel;
                    webi->setAttribute( Qt::WA_NoSystemBackground );
                    webi->setAlignment( Qt::AlignCenter );

                    if( item->source == QString( "youtube" ) )
                        webi->setPixmap( m_pixYoutube );
                    else if ( item->source == QString( "dailymotion" ) )
                        webi->setPixmap( m_pixDailymotion );
                    else if ( item->source == QString( "vimeo" ) )
                        webi->setPixmap( m_pixVimeo );

                    QGraphicsWidget *widget = new QGraphicsWidget( m_scroll );
                    widget->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );

                    QString href = QString( "<a href=\"%1\">%2</a>" ).arg( item->url, title );
                    Plasma::Label *linkWidget = new Plasma::Label( widget );
                    linkWidget->setText( href );
                    linkWidget->setTextSelectable( true );
                    linkWidget->nativeWidget()->setOpenExternalLinks( true );
                    linkWidget->nativeWidget()->setWordWrap( true );
                    linkWidget->nativeWidget()->setAlignment( Qt::AlignHCenter );

                    QGraphicsProxyWidget *videoWidget = new QGraphicsProxyWidget( widget );
                    QGraphicsProxyWidget *durationWidget = new QGraphicsProxyWidget( widget );
                    QGraphicsProxyWidget *webiWidget = new QGraphicsProxyWidget( widget );

                    videoWidget->setWidget( vidButton );
                    durationWidget->setWidget( duration );
                    webiWidget->setWidget( webi );

                    RatingWidget *rating = new RatingWidget( widget );
                    rating->setAcceptedMouseButtons( 0 );
                    rating->setRating( int( item->rating * 2) );

                    QGraphicsLinearLayout *l = new QGraphicsLinearLayout( Qt::Vertical, widget );
                    l->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
                    l->setSpacing( 3 );
                    l->addItem( videoWidget );
                    l->addItem( linkWidget );
                    l->addItem( webiWidget );
                    l->addItem( durationWidget );
                    l->addItem( rating );

                    l->setAlignment( videoWidget, Qt::AlignHCenter );
                    l->setAlignment( linkWidget, Qt::AlignHCenter );
                    l->setAlignment( webiWidget, Qt::AlignHCenter );
                    l->setAlignment( durationWidget, Qt::AlignHCenter );
                    l->setAlignment( rating, Qt::AlignHCenter );

                    m_scrollLayout->addItem( widget );

                    if( i < data.size() - 1 )
                    {
                        Plasma::Separator *line = new Plasma::Separator( this );
                        line->setOrientation( Qt::Vertical );
                        m_scrollLayout->addItem( line );
                    }
                }
            }
        }
    }
    // FIXME This should be in stateChanged(), but for now it help fixing the bug 210332
    else if ( The::engineController()->phononMediaObject()->hasVideo()
        && The::engineController()->phononMediaObject()->state() != Phonon::BufferingState
        && The::engineController()->phononMediaObject()->state() != Phonon::LoadingState )
    {
        setBusy( false );
        debug() << " VideoclipApplet | Show VideoWidget";
        m_scroll->hide();
        m_videoWidget.data()->show();
        m_videoWidget.data()->activateWindow();
        if ( m_videoWidget.data()->inputPaths().isEmpty() )
            Phonon::createPath( const_cast<Phonon::MediaObject*>( The::engineController()->phononMediaObject() ), m_videoWidget.data() );
        if( m_videoWidget.data()->isActiveWindow() )
        {
            QContextMenuEvent e( QContextMenuEvent::Other, QPoint() );
            QApplication::sendEvent( m_videoWidget.data(), &e );
        }
    }
    
    updateConstraints();
}

void 
VideoclipApplet::appendVideoClip( VideoInfo *info )
{
	DEBUG_BLOCK
    QAbstractButton *button = qobject_cast<QAbstractButton *>(QObject::sender() );
    if ( button )
    {
        QStringList lst = button->text().split(" | ");
    
        MetaStream::Track *tra = new MetaStream::Track(KUrl( info->videolink ) );
        tra->setTitle( info->title );
        tra->setAlbum( info->source );
        tra->setArtist( info->artist );
        tra->album()->setImage( info->cover.toImage() );
        Meta::TrackPtr track( tra );
        //append to the playlist the newly retrieved
        The::playlistController()->insertOptioned(track , Playlist::Append );
    }
}

void
VideoclipApplet::queueVideoClip( VideoInfo *info )
{
    DEBUG_BLOCK
    QAbstractButton *button = qobject_cast<QAbstractButton *>(QObject::sender() );
    if ( button )
    {
        QStringList lst = button->text().split(" | ");
        
        MetaStream::Track *tra = new MetaStream::Track(KUrl( info->videolink ) );
        tra->setTitle( info->title );
        tra->setAlbum( info->source );
        tra->setArtist( info->artist );
        tra->album()->setImage( info->cover.toImage() );
        Meta::TrackPtr track( tra );
        //append to the playlist the newly retrieved
        The::playlistController()->insertOptioned(track , Playlist::Queue );
    }
}

void
VideoclipApplet::appendPlayVideoClip( VideoInfo *info )
{
    DEBUG_BLOCK
    QAbstractButton *button = qobject_cast<QAbstractButton *>(QObject::sender() );
    if ( button )
    {
        QStringList lst = button->text().split(" | ");
        
        MetaStream::Track *tra = new MetaStream::Track(KUrl( info->videolink ) );
        tra->setTitle( info->title );
        tra->setAlbum( info->source );
        tra->setArtist( info->artist );
        tra->album()->setImage( info->cover.toImage() );
        Meta::TrackPtr track( tra );
        //append to the playlist the newly retrieved
        The::playlistController()->insertOptioned( track, Playlist::AppendAndPlayImmediately );
    }
}

void
VideoclipApplet::createConfigurationInterface( KConfigDialog *parent )
{
    KConfigGroup configuration = config();
    QWidget *settings = new QWidget;
    ui_Settings.setupUi( settings );
    
    // TODO bad, it's done manually ...
    if ( m_youtubeHQ == true )
        ui_Settings.checkYoutubeHQ->setChecked( true );
    
    parent->addPage( settings, i18n( "Video Clip Settings" ), "preferences-system");
    connect( parent, SIGNAL( accepted() ), this, SLOT( saveSettings( ) ) );
}

void
VideoclipApplet::saveSettings()
{
    DEBUG_BLOCK
    KConfigGroup config = Amarok::config("Videoclip Applet");
    
    m_youtubeHQ = ui_Settings.checkYoutubeHQ->isChecked();
    config.writeEntry( "YoutubeHQ", m_youtubeHQ );
    
    dataEngine( "amarok-videoclip" )->query( QString( "videoclip:youtubeHQ:" ) + QString().setNum( m_youtubeHQ ) );
}

#include "VideoclipApplet.moc"
#include "../../../core/support/SmartPointerList.moc"
