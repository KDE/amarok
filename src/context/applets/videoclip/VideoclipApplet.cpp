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
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

//Plasma applet for showing videoclip in the context view

#include "VideoclipApplet.h" 
#include "VideoItemButton.h"
#include "CustomVideoWidget.h"

// Amarok
#include "Amarok.h"
#include "Debug.h"
#include "EngineController.h"
#include "meta/stream/Stream.h"
#include "collection/CollectionManager.h"
#include "context/ContextView.h"
#include "context/Svg.h"
#include "playlist/PlaylistController.h"
#include "SvgHandler.h"
#include "widgets/kratingpainter.h"
#include "widgets/kratingwidget.h"

// KDE
#include <KAction>
#include <KColorScheme>
#include <KConfigDialog>
#include <KMenu>
#include <KStandardDirs>
#include <KVBox>
#include <Plasma/Theme>
#include <Plasma/BusyWidget>
#include <Plasma/IconWidget>
#include <Phonon/MediaObject>
#include <Phonon/Path>
#include <Phonon/VideoWidget>

// Qt
#include <QGraphicsLinearLayout>
#include <QGraphicsProxyWidget>
#include <QGraphicsTextItem>
#include <QGraphicsWidget>
#include <QGridLayout>
#include <QLabel>
#include <QPainter>
#include <QToolButton>
#include <QScrollArea>

#define DEBUG_PREFIX "VideoclipApplet"

Q_DECLARE_METATYPE ( VideoInfo *)
K_EXPORT_AMAROK_APPLET( videoclip, VideoclipApplet )


VideoclipApplet::VideoclipApplet( QObject* parent, const QVariantList& args )
        : Context::Applet( parent, args )
        , EngineObserver( The::engineController() )
        , m_settingsIcon( 0 )
        , m_youtubeHQ( false )
{
    DEBUG_BLOCK
    setHasConfigurationInterface( true );
}

void 
VideoclipApplet::init()
{
    setBackgroundHints( Plasma::Applet::NoBackground );

    m_height = 300;
    resize( size().width(), m_height );

    // CustomWidget is a special VideoWidget for interaction
    m_videoWidget = new CustomVideoWidget();
    m_videoWidget->setParent( Context::ContextView::self()->viewport(), Qt::SubWindow | Qt::FramelessWindowHint );
    m_videoWidget->hide();
    
    // we create path no need to add a lot of fancy thing 
    Phonon::createPath( const_cast<Phonon::MediaObject*>( The::engineController()->phononMediaObject() ), m_videoWidget );

    
    // Load pixmap
    m_pixYoutube = new QPixmap( KStandardDirs::locate( "data", "amarok/images/amarok-videoclip-youtube.png" ) );
    m_pixDailymotion = new QPixmap( KStandardDirs::locate( "data", "amarok/images/amarok-videoclip-dailymotion.png" ) );
    m_pixVimeo = new QPixmap( KStandardDirs::locate( "data", "amarok/images/amarok-videoclip-vimeo.png" ) );

    QAction* langAction = new QAction( this );
    langAction->setIcon( KIcon( "preferences-system" ) );
    langAction->setVisible( true );
    langAction->setEnabled( true );
    m_settingsIcon = addAction( langAction );
    m_settingsIcon->setToolTip( i18n( "Settings" ) );
    connect( m_settingsIcon, SIGNAL( activated() ), this, SLOT( showConfigurationInterface() ) );

    
    // Create label
    QFont labelFont;
    labelFont.setPointSize( labelFont.pointSize() + 2 );
    m_headerText = new QGraphicsSimpleTextItem( this );
    m_headerText->setBrush( Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor ) );
    m_headerText->setFont( labelFont );
    m_headerText->setText( i18n( "Video Clip" ) );

    // Set the collapse size
    setCollapseHeight( m_headerText->boundingRect().height() + 3 * standardPadding() );

    // Create layout
    m_layout = new QHBoxLayout();
    m_layout->setSizeConstraint( QLayout::SetMinAndMaxSize );
    m_layout->setContentsMargins( 5, 5, 5, 5 );
    m_layout->setSpacing( 2 );

    // create a widget
    QWidget *window = new QWidget;
    window->setAttribute( Qt::WA_NoSystemBackground );
    window->setLayout( m_layout );

    // create a scroll Area
    QScrollArea *m_scroll = new QScrollArea();
    m_scroll->setMaximumHeight( m_height - m_headerText->boundingRect().height() - 4*standardPadding() );
    m_scroll->setWidget( window );
    m_scroll->setAttribute( Qt::WA_NoSystemBackground );
    m_scroll->viewport()->setAttribute( Qt::WA_NoSystemBackground );

    m_widget = new QGraphicsProxyWidget( this );
    m_widget->setWidget( m_scroll );

    constraintsEvent();
    
    //Update the applet (render properly the header)
    update();

    // we connect the geometry changed wit(h a setGeom function which will update the video widget geometry
    connect ( this, SIGNAL(geometryChanged()), SLOT( setGeom() ) );

    connectSource( "videoclip" );
    
    connect( dataEngine( "amarok-videoclip" ), SIGNAL( sourceAdded( const QString & ) ),
             this, SLOT( connectSource( const QString & ) ) );

    engineStateChanged(Phonon::PlayingState,Phonon::StoppedState);// kickstart

    // Read config and inform the engine.
    KConfigGroup config = Amarok::config("Videoclip Applet");
    m_youtubeHQ = config.readEntry( "YoutubeHQ", false );
    dataEngine( "amarok-videoclip" )->query( QString( "videoclip:youtubeHQ:" ) + QString().setNum( m_youtubeHQ ) );
    
}

VideoclipApplet::~VideoclipApplet()
{
    DEBUG_BLOCK
   
    delete m_videoWidget;
}

void 
VideoclipApplet::engineNewTrackPlaying()
{
    DEBUG_BLOCK
    // on new track, we expand the applet if not already
    setCollapseOff();
    m_videoWidget->hide();
}

void
VideoclipApplet::engineStateChanged(Phonon::State currentState, Phonon::State oldState)
{
    DEBUG_BLOCK

    debug() << "video old state: " << oldState << " new state: " << currentState;

    if ( currentState == oldState )
        return;

    switch ( currentState )
    {
        // when switching from buffering to to playing, we launch the vid widget
        case Phonon::PlayingState :
        {
            // We need this has when song switching the state will do
            // playing > stopped > playing > loading > buffering > playing

            // --well, not on OS X. there it will go oldState == stopped,
            // newState == playing, after which the track actually starts playing
            // adding StoppedState below makes the videoapplet work for video podcasts.
            // I suggest adding a special case for OS X if this breaks on other platforms - max
            if ( oldState == Phonon::BufferingState || oldState == Phonon::StoppedState )
            {
                debug() <<" video state : playing";

                if ( The::engineController()->phononMediaObject()->hasVideo() )
                {
                    setBusy( false );
                    debug() << " VideoclipApplet | Show VideoWidget";
                    m_widget->hide();
                    m_videoWidget->show();
                    m_videoWidget->activateWindow();
                    Phonon::createPath( const_cast<Phonon::MediaObject*>( The::engineController()->phononMediaObject() ), m_videoWidget );
                    if( m_videoWidget->isActiveWindow() ) {
                        //FIXME dual-screen this seems to still show
                        QContextMenuEvent e( QContextMenuEvent::Other, QPoint() );
                        QApplication::sendEvent( m_videoWidget, &e );
                    }
                }
                else
                {
                    debug() << " VideoclipApplet | Hide VideoWidget";
                    m_videoWidget->hide();
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
            m_videoWidget->hide();
            m_widget->hide();
            break;
        }

        default:
            break;
    }
}

void 
VideoclipApplet::enginePlaybackEnded( int finalPosition, int trackLength, PlaybackEndedReason reason )
{
    Q_UNUSED( finalPosition )
    Q_UNUSED( trackLength )
    Q_UNUSED( reason )

    // On playback ending, we hide everything and collapse
    setBusy( false );
    m_widget->hide();
    m_videoWidget->hide();
    
    setCollapseOn();

}

void 
VideoclipApplet::constraintsEvent( Plasma::Constraints constraints )
{
    Q_UNUSED( constraints );
    prepareGeometryChange();

    // tint the applet size
    m_headerText->setPos( size().width() / 2 - m_headerText->boundingRect().width() / 2, standardPadding() + 3 );
    m_widget->setPos( standardPadding(), m_headerText->pos().y() + m_headerText->boundingRect().height() + standardPadding() );
    m_widget->resize( size().width() - 2 * standardPadding(), size().height() - m_headerText->boundingRect().height() - 2*standardPadding() );
    m_videoWidget->setGeometry( QRect(
        pos().toPoint()+QPoint( standardPadding(), m_headerText->boundingRect().height() + 2.5 * standardPadding() ),
        size().toSize()-QSize( 2 * standardPadding(),  m_headerText->boundingRect().height() + 3.5 * standardPadding() ) ) );

    m_settingsIcon->setPos( size().width() - m_settingsIcon->size().width() - standardPadding(), standardPadding() );
}

void 
VideoclipApplet::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{
    Q_UNUSED( p );
    Q_UNUSED( option );
    Q_UNUSED( contentsRect );
    p->setRenderHint( QPainter::Antialiasing );
    // tint the whole applet
    addGradientToAppletBackground( p );
    // draw rounded rect around title
    drawRoundedRectAroundText( p, m_headerText );
}

void
VideoclipApplet::setGeom( )
{
    updateConstraints();
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
    DEBUG_BLOCK
    Q_UNUSED( name )

    if ( data.empty() )
        return;
    
    if ( !m_videoWidget->isVisible() && !The::engineController()->phononMediaObject()->hasVideo() )
    {
        int width = 130;
        // Properly delete previsouly allocated item
        while ( !m_layoutWidgetList.empty() )
        {
            m_layoutWidgetList.front()->hide();
            m_layout->removeWidget( m_layoutWidgetList.front() );
			delete m_layoutWidgetList.front();
            m_layoutWidgetList.pop_front();            
        }
        
        // if we get a message, show it
        if ( data.contains( "message" ) && data["message"].toString().contains("Fetching"))
        {
            m_headerText->setText( i18n( "Video Clip" ) );
            constraintsEvent();
            update();
            debug() <<" message fetching ";
            m_widget->hide();

			setBusy( true );
        }
		else if ( data.contains( "message" ) )
		{
            //if nothing found, we collapse and inform user
            m_headerText->setText( i18n( "Video Clip " ) + ":" + i18n( " No information found..." ) );
            update();
			setBusy( false );
            m_widget->hide();
            setCollapseOn();
		}
        else if ( data.contains( "item:0" ) )
        {
            // make the animation didn't stop because it was the mees
            resize( size().width(), m_height );
            m_headerText->setText( i18n( "Video Clip" ) );
            update();
            // set collapsed
            // tint the applet
            m_widget->show();

			setBusy(false);
            for (int i=0; i< data.size(); i++ )
            {
                VideoInfo *item = data[ QString ("item:" )+QString().setNum(i) ].value<VideoInfo *>() ;
                if( item->url != "" ) // prevent some weird stuff ...
                {

                    VideoItemButton *vidButton = new VideoItemButton();
                    vidButton->setVideoInfo( item ); 
                    
                    connect ( vidButton, SIGNAL( appendRequested( VideoInfo * ) ), this, SLOT ( appendVideoClip( VideoInfo * ) ) );
                    connect ( vidButton, SIGNAL( queueRequested( VideoInfo* ) ), this, SLOT ( queueVideoClip( VideoInfo * ) ) );
                    connect ( vidButton, SIGNAL( appendPlayRequested( VideoInfo * ) ), this, SLOT ( appendPlayVideoClip( VideoInfo * ) ) );

                    // create link (and resize, no more than 3 lines long)
                    QString title( item->title );
                    if ( title.size() > 45 ) title.resize( 45 );
                    QLabel *link = new QLabel( QString( "<html><body><a href=\"" ) + item->url + QString( "\">" ) + title + QString( "</a>" ) );
                    link->setOpenExternalLinks( true );
                    link->setWordWrap( true );

                    QLabel *duration =  new QLabel( item->duration + QString( "<br>" ) + item->views + QString( " views" ) );

                    KRatingWidget* rating = new KRatingWidget;
                    rating->setRating(( int )( item->rating * 2. ) );
                    rating->setMaximumWidth(( int )(( width / 3 )*2 ) );
                    rating->setMinimumWidth(( int )(( width / 3 )*2 ) );

                    QLabel *webi = new QLabel;
                    if ( item->source == QString( "youtube" ) )
                        webi->setPixmap( *m_pixYoutube );
                    else if ( item->source == QString( "dailymotion" ) )
                        webi->setPixmap( *m_pixDailymotion );
                    else if ( item->source == QString( "vimeo" ) )
                        webi->setPixmap( *m_pixVimeo );


                    QGridLayout *grid = new QGridLayout();
                    grid->setHorizontalSpacing( 5 );
                    grid->setVerticalSpacing( 2 );
                    grid->setRowMinimumHeight( 1, 65 );
                    grid->setColumnStretch( 0, 0 );
                    grid->setColumnStretch( 1, 1 );
                    grid->addWidget( vidButton, 0, 0, 1, -1, Qt::AlignCenter );
                    grid->addWidget( link, 1, 0, 1, -1, Qt::AlignCenter | Qt::AlignTop );
                    grid->addWidget( webi, 2, 0, Qt::AlignCenter );
                    grid->addWidget( duration, 2, 1, Qt::AlignLeft );
                    grid->addWidget( rating, 3, 0, 1, -1, Qt::AlignCenter );

                    // Add The Widget
                    QWidget *widget = new QWidget();
                    widget->setLayout( grid );
                    widget->resize( width, m_height - m_headerText->boundingRect().height() - 2*standardPadding() );
                    widget->setMaximumWidth( width );
                    widget->setMinimumWidth( width );
                    widget->setMinimumHeight( m_height - ( m_headerText->boundingRect().height() + 10 * standardPadding() ) );
                    widget->setMaximumHeight( m_height - ( m_headerText->boundingRect().height() + 10 * standardPadding() ) );
                    m_layout->addWidget( widget, Qt::AlignLeft );
                    m_layoutWidgetList.push_back( widget );

                    if ( i < data.size() - 1 )
                    {
                        QFrame *line = new QFrame();
                        line->setFrameStyle( QFrame::VLine );
                        line->setAutoFillBackground( false );
                        line->setMaximumHeight( m_height - ( m_headerText->boundingRect().height() + 2 * standardPadding() ) );
                        m_layout->addWidget( line, Qt::AlignLeft );
                        m_layoutWidgetList.push_back( line );
                    }
                }
            }
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
        tra->album()->setImage( *info->cover );
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
        tra->album()->setImage( *info->cover );
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
        tra->album()->setImage( *info->cover );
        Meta::TrackPtr track( tra );
        //append to the playlist the newly retrieved
        The::playlistController()->insertOptioned(track , Playlist::AppendAndPlayImmediately );
    }
}

void
VideoclipApplet::videoMenu( QPoint point )
{
    KMenu *men = new KMenu(m_videoWidget);
    if ( !m_videoWidget->isFullScreen() )
    {
        KAction *toggle = new KAction( KIcon( "view-fullscreen" ), i18n( "Enter &fullscreen" ), this );
        men->addAction( toggle );
        connect( toggle, SIGNAL( triggered(bool) ), m_videoWidget, SLOT( exitFullScreen() ) );
    }
    else
    {
        KAction *toggle = new KAction( KIcon( "edit-undo" ), i18n( "E&xit fullscreen" ), this );
        men->addAction( toggle );
        connect( toggle, SIGNAL( triggered(bool) ), m_videoWidget, SLOT( exitFullScreen() ) );
    }   
    men->exec( m_videoWidget->mapToGlobal( point ) );
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

