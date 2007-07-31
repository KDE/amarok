/***************************************************************************
* copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>        *
**************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "LastFmEvents.h"

#include "debug.h"
#include "Theme.h"

#include <QGraphicsTextItem>
#include <QTextDocument>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QPainter>
#include <QVariant>

#include <KDialog>
#include <KLocale>

#define DEBUG_PREFIX "LastFmEvents"

LastFmEvents::LastFmEvents( QObject* parent, const QStringList& args )
    : Context::Applet( parent, args )
    , m_config( 0 )     
    , m_configLayout( 0 )
    , m_friendBox( 0 )
    , m_sysBox( 0 )
    , m_userBox( 0 )
    , m_theme( 0 )
    , m_friendData( 0 )
    , m_userData( 0 )
    , m_sysData( 0 )
    , m_userItem( 0 )
    , m_friendItem( 0 )
    , m_sysItem( 0 )
    , m_friendEnabled( false )
    , m_sysEnabled( false )
    , m_userEnabled( false )
{
    DEBUG_BLOCK
    Context::Theme::self()->setApplication( "amarok" );
    
    debug() << "Loading LastFmEvents applet" << endl;
    setAcceptDrops( false );
    
    setHasConfigurationInterface( true );
    setDrawStandardBackground( false );
    
    KConfigGroup conf = globalConfig();
    m_userEnabled = conf.readEntry( "user", false );
    m_sysEnabled = conf.readEntry( "sys", false );
    m_friendEnabled = conf.readEntry( "friend", false );
    m_size = QSizeF( conf.readEntry( "size" , 400 ), conf.readEntry( "size" , 400 ) );
    
    if( !m_userEnabled && !m_friendEnabled && m_sysEnabled )
        showConfigurationInterface();
    
    if( args.size() > 0 ) // we are being told what position to start at
        setPos( (qreal)args[0].toInt(), (qreal)args[1].toInt() );
    
    m_userData = new QList< QVariantList >();
    m_sysData = new QList< QVariantList >();
    m_friendData = new QList< QVariantList >();
    
    m_theme = new Plasma::Svg( "widgets/lastfm", this );
    m_theme->setContentType( Plasma::Svg::SingleImage );
    m_theme->resize( m_size  );
    
    m_friendItem = new Context::TextWidget( this );
    m_sysItem = new Context::TextWidget( this );
    m_userItem = new Context::TextWidget( this );
    
    dataEngine( "amarok-lastfm" )->connectSource( I18N_NOOP( "sysevents" ), this );
    dataEngine( "amarok-lastfm" )->connectSource( I18N_NOOP( "userevents" ), this );
    dataEngine( "amarok-lastfm" )->connectSource( I18N_NOOP( "friendevents" ), this );
    
    updated( "sysevents", dataEngine( "amarok-lastfm" )->query( "sysevents" ) );
    updated( "userevents", dataEngine( "amarok-lastfm" )->query( "userevents" ) );
    updated( "friendevents", dataEngine( "amarok-lastfm" )->query( "friendevents" ) );

    constraintsUpdated();
    
}

LastFmEvents::~LastFmEvents()
{
    DEBUG_BLOCK
    
    delete m_userData;
    delete m_sysData;
    delete m_friendData;
    delete m_theme;
    delete m_userItem;
    delete m_friendItem;
    delete m_sysItem;
}

void LastFmEvents::geometryChanged()
{
    DEBUG_BLOCK
    prepareGeometryChange();
    
    QRect friendRect = m_theme->elementRect( "friendevents" );
    QRect userRect = m_theme->elementRect( "userevents" );
    QRect sysRect = m_theme->elementRect( "sysevents" );
    
    m_friendItem->setPos( friendRect.x() + 3, friendRect.y() + 3 );
    m_userItem->setPos( userRect.x() + 3, userRect.y() + 3 );
    m_sysItem->setPos( sysRect.x() + 3, sysRect.y() + 3 );
    
    m_friendItem->setTextWidth( friendRect.width() );
    m_userItem->setTextWidth( userRect.width() );
    m_sysItem->setTextWidth( sysRect.width() );
    
    update();
}

void LastFmEvents::updated( const QString& name, const Context::DataEngine::Data& data )
{
    DEBUG_BLOCK
    
    Context::DataEngine::DataIterator iter( data );
    if( m_sysEnabled && name == QString( "sysevents" ) )
    {
        while( iter.hasNext() )
        {
            iter.next();
            const QVariantList event = iter.value().toList();
            if( event.size() == 0 ) continue; // empty event
            m_sysData->append( event );
        }
    }
    if( m_friendEnabled && name == QString( "friendevents" ) )
    {
        while( iter.hasNext() )
        {
            iter.next();
            const QVariantList event = iter.value().toList();
            if( event.size() == 0 ) continue; // empty event
            
            m_friendData->append( event );
        }
    }
    if( m_userEnabled && name == QString( "sysevents" ) )
    {
        while( iter.hasNext() )
        {
            iter.next();
            const QVariantList event = iter.value().toList();
            if( event.size() == 0 ) continue; // empty event
            
            m_sysData->append( event );
        }
    }
    update();
}

void LastFmEvents::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect& contentsRect )
{
    Q_UNUSED( option )
    
    p->setRenderHint(QPainter::SmoothPixmapTransform);
        
    debug() << "painting rect: " << contentsRect << endl;
    m_theme->paint( p, contentsRect );
        
    QString text;
    foreach( QVariantList event, *m_friendData )
        text.append( QString( "%1 - %2<br>" ).arg( event[ 0 ].toString(), event[ 1 ].toString() ) );
    m_friendItem->setText( text );
    
    text.clear();
    foreach( QVariantList event, *m_userData )
        text.append( QString( "%1 - %2<br>" ).arg( event[ 0 ].toString(), event[ 1 ].toString() ) );
    m_userItem->setText( text );
    
    text.clear();
    foreach( QVariantList event, *m_sysData )
        text.append( QString( "%1 - %2<br>" ).arg( event[ 0 ].toString(), event[ 1 ].toString() ) );
    m_sysItem->setText( text );
    
    debug() << "size: " << m_theme->size() << endl;
    debug() << "friend element rect: " << m_theme->elementRect( "friendevents" ) << endl;
    debug() << "user element rect: " << m_theme->elementRect( "userevents" ) << endl;
    debug() << "sys element rect: " << m_theme->elementRect( "sysevents" ) << endl;
    debug() << "top element rect: " << m_theme->elementRect( "top" ) << endl;
    debug() << "bottom element rect: " << m_theme->elementRect( "bottom" ) << endl;
    debug() << "something: " << m_theme->elementRect( "something" ) << endl;
    
    QRectF friendRect = m_theme->elementRect( "friendevents" );
//     friendRect.translate( friendRect.x(), friendRect.y() );
    QRectF userRect = m_theme->elementRect( "userevents" );
//     userRect.translate( userRect.x(), userRect.y() );
    QRectF sysRect = m_theme->elementRect( "sysevents" );
//     sysRect.translate( sysRect.x() , sysRect.y() );

    m_friendItem->setGeometry( friendRect );
    m_userItem->setGeometry( userRect );
    m_sysItem->setGeometry( sysRect );
    
}
    

QSizeF LastFmEvents::contentSize() const 
{
    return m_size;
}

void LastFmEvents::constraintsUpdated()
{
    geometryChanged();
}

void LastFmEvents::showConfigurationInterface()
{
    DEBUG_BLOCK
    if (m_config == 0) 
    {
        m_config = new KDialog;
        m_config->setCaption( i18n( "Configure Last.Fm Events" ) );
        
        QWidget* widget = new QWidget( m_config );
        m_config->setButtons( KDialog::Ok | KDialog::Cancel | KDialog::Apply );
        connect( m_config, SIGNAL(applyClicked()), this, SLOT(configAccepted()) );
        connect( m_config, SIGNAL(okClicked()), this, SLOT(configAccepted()) );
        
        m_configLayout = new QVBoxLayout( widget );
        m_friendBox = new QCheckBox();
        m_friendBox->setText( i18n( "Show Friend Events" ) );
        m_sysBox = new QCheckBox();
        m_sysBox->setText( i18n( "Show Recommended Events" ) );
        m_userBox = new QCheckBox();
        m_userBox->setText( i18n( "Show Your Future Events" ) );
        
        m_configLayout->addWidget( m_friendBox );
        m_configLayout->addWidget( m_sysBox );
        m_configLayout->addWidget( m_userBox );
        
        m_config->setMainWidget( widget );
    }
    
    m_config->show();
}

void LastFmEvents::configAccepted() // SLOT
{
    DEBUG_BLOCK
    debug() << "saving config" << endl;
    KConfigGroup cg = globalConfig();

    m_friendBox->checkState() ? m_friendEnabled = true : m_friendEnabled = false;
    m_sysBox->checkState() ? m_sysEnabled = true : m_sysEnabled = false;
    m_userBox->checkState() ? m_userEnabled = true : m_userEnabled = false;
    
    cg.writeEntry( "friend" , m_friendEnabled );
    cg.writeEntry( "sys" , m_sysEnabled );
    cg.writeEntry( "user" , m_userEnabled );
    
    cg.sync();
    
    constraintsUpdated();
}

#include "LastFmEvents.moc"
