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

#include "PhotosApplet.h"
#include "PhotosScrollWidget.h"

// Amarok
#include "Amarok.h"
#include "Debug.h"
#include "context/ContextView.h"

// KDE
#include <KAction>
#include <KColorScheme>
#include <KConfigDialog>
#include <Plasma/BusyWidget>
#include <Plasma/IconWidget>
#include <Plasma/Theme>

// Qt
#include <QGraphicsLinearLayout>
#include <QGraphicsProxyWidget>
#include <QGraphicsTextItem>
#include <QGraphicsWidget>

#define DEBUG_PREFIX "PotosApplet"


PhotosApplet::PhotosApplet( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_settingsIcon( 0 )
{
    DEBUG_BLOCK
    setHasConfigurationInterface( true );
}

void 
PhotosApplet::init()
{
    setBackgroundHints( Plasma::Applet::NoBackground );

    // HACK
    m_height = 300;
    
    // Create label
    QFont labelFont;
    labelFont.setPointSize( labelFont.pointSize() + 2 );
    m_headerText = new QGraphicsSimpleTextItem( this );
    m_headerText->setBrush( Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor ) );
    m_headerText->setFont( labelFont );
    m_headerText->setText( i18n( "Photos" ) );

    // Icon
    QAction* settingsAction = new QAction( i18n( "Settings" ), this );
    settingsAction->setIcon( KIcon( "preferences-system" ) );
    settingsAction->setVisible( true );
    settingsAction->setEnabled( true );
    m_settingsIcon = addAction( settingsAction );
    connect( m_settingsIcon, SIGNAL( activated() ), this, SLOT( showConfigurationInterface() ) );

    m_widget = new PhotosScrollWidget( this );

    constraintsEvent();

    connectSource( "photos" );
    connect( dataEngine( "amarok-photos" ), SIGNAL( sourceAdded( const QString & ) ),
             this, SLOT( connectSource( const QString & ) ) );

}

PhotosApplet::~PhotosApplet()
{
    DEBUG_BLOCK
}

void 
PhotosApplet::constraintsEvent( Plasma::Constraints constraints )
{
    Q_UNUSED( constraints );
    prepareGeometryChange();

    m_headerText->setPos( size().width() / 2 - m_headerText->boundingRect().width() / 2, standardPadding() + 3 );
    m_widget->setPos( standardPadding(), m_headerText->pos().y() + m_headerText->boundingRect().height() + standardPadding() );
    m_widget->resize( size().width() - 2 * standardPadding(), size().height() - m_headerText->boundingRect().height() - 2*standardPadding() );

    m_settingsIcon->setPos( size().width() - m_settingsIcon->size().width() - standardPadding(), standardPadding() );
}

void 
PhotosApplet::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
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

QSizeF 
PhotosApplet::sizeHint( Qt::SizeHint which, const QSizeF & constraint ) const
{
    // hardcoding for now
    return QSizeF( QGraphicsWidget::sizeHint( which, constraint ).width(), m_height );
}

void 
PhotosApplet::connectSource( const QString &source )
{
    if ( source == "photos" )
        dataEngine( "amarok-photos" )->connectSource( "photos", this );
}

void 
PhotosApplet::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data ) // SLOT
{
    DEBUG_BLOCK
    Q_UNUSED( name )

    if ( data.empty() )
        return;


    // if we get a message, show it
    if ( data.contains( "message" ) && data["message"].toString().contains("Fetching"))
    {
        m_widget->hide();
        setBusy( true );
    }
    else if ( data.contains( "message" ) )
    {
        m_widget->hide();
        setBusy( false );
    }
    else if ( data.contains( "data" ) )
    {
        // only photos to translate ^^
        m_headerText->setText( i18n( "Photos" ) + QString( " : " ) + data[ "artist" ].toString() );       
        m_widget->setPixmapList( data[ "data" ].value< QList< QPixmap * > >() );
        m_widget->show();
        setBusy(false);
    }
    updateConstraints();
    update();
}

Plasma::IconWidget *
PhotosApplet::addAction( QAction *action )
{
    DEBUG_BLOCK
    if ( !action ) {
        debug() << "ERROR!!! PASSED INVALID ACTION";
        return 0;
    }
    
    Plasma::IconWidget *tool = new Plasma::IconWidget( this );
    tool->setAction( action );
    tool->setText( "" );
    tool->setToolTip( action->text() );
    tool->setDrawBackground( false );
    tool->setOrientation( Qt::Horizontal );
    QSizeF iconSize = tool->sizeFromIconSize( 16 );
    tool->setMinimumSize( iconSize );
    tool->setMaximumSize( iconSize );
    tool->resize( iconSize );
    tool->setZValue( zValue() + 1 );
    
    return tool;
}

void
PhotosApplet::createConfigurationInterface( KConfigDialog *parent )
{
    KConfigGroup configuration = config();
    QWidget *settings = new QWidget;
    ui_Settings.setupUi( settings );

    parent->addPage( settings, i18n( "Photos Settings" ), "preferences-system");

    /*
    // TODO bad, it's done manually ...
    if ( m_youtubeHQ == true )
    ui_Settings.comboBox->( true );
    */
    //connect( ui_Settings, SIGNAL( currentIndexChanged( QString ) ), this, SLOT( switchToLang( QString ) ) );
}



#include "PhotosApplet.moc"

