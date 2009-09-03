/***************************************************************************
* copyright            : (C) 2007-2008 Leo Franchi <lfranchi@gmail.com>   *
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

#include "Debug.h"
#include "Theme.h"

#include <QGraphicsTextItem>
#include <QTextDocument>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QPainter>
#include <QVariant>
#include <QFontMetrics>

#include <KDialog>
#include <KLocale>

#define DEBUG_PREFIX "LastFmEvents"

LastFmEvents::LastFmEvents( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
{
    DEBUG_BLOCK

    debug() << "Loading LastFmEvents applet";

    setBackgroundHints( Plasma::Applet::DefaultBackground );
    setHasConfigurationInterface( false );
}

LastFmEvents::~LastFmEvents()
{
}

void LastFmEvents::init()
{
    DEBUG_BLOCK
    KConfigGroup conf = globalConfig();

    m_width = conf.readEntry( "width" , 400 );

    m_theme = new Context::Svg( this );
    m_theme->setImagePath( "widgets/amarok-lastfm" );
    m_theme->setContainsMultipleImages( false );
    debug() << "LastFmEvents loaded theme file:" << m_theme->imagePath();

    for( int i = 0; i < 14; i++ ) // create all the items
    {
        m_titles << new QGraphicsSimpleTextItem( this );
        m_dates << new QGraphicsSimpleTextItem( this );
        m_cities << new QGraphicsSimpleTextItem( this );
        const QColor textColor = Plasma::Theme::defaultTheme()->color( Plasma::Theme::TextColor );
        m_titles[ i ]->setBrush( textColor );
        m_dates[ i ]->setBrush( textColor );
        m_cities[ i ]->setBrush( textColor );
    }
    connectSource( "sysevents" );
    connectSource( "userevents" );
    connectSource( "friendevents" );

    connect( dataEngine( "amarok-lastfm" ), SIGNAL( sourceAdded( const QString& ) ), this, SLOT( connectSource( const QString& ) ) );

    // calculate aspect ratio, and resize to desired width
    m_theme->resize();
    m_aspectRatio = (qreal)m_theme->size().height() / (qreal)m_theme->size().width();

    debug() << "setting size to " << m_width;
    m_theme->resize( (int)m_width, (int)m_width );
    setMaximumSize( 10000, 10000 ); // allow effectiveSizeHint to report preferred size without limit

}

void
LastFmEvents::connectSource( const QString &source )
{
        dataEngine( "amarok-lastfm" )->connectSource( source, this );
        dataUpdated( source, dataEngine("amarok-lastfm" )->query( source ) ); // get data initially
}


void LastFmEvents::constraintsEvent( Plasma::Constraints constraints )
{
    DEBUG_BLOCK

    if( !m_theme )
        return;

    prepareGeometryChange();

    if( constraints & Plasma::SizeConstraint )
        m_theme->resize(size().toSize());

    debug() << "resized LastFmEvents svg to " << size().toSize() << ", now re-laying out";
    for( int i = 0; i < 14; i++ ) // go through each row
    {
        QString titleElement = QString( "title%1" ).arg( i );
        QString dateElement = QString( "date%1" ).arg( i );
        QString cityElement = QString( "city%1" ).arg( i );

        m_titles[ i ]->setPos( m_theme->elementRect( titleElement ).topLeft() );
        m_dates[ i ]->setPos( m_theme->elementRect( dateElement ).topLeft() );
        m_cities[ i ]->setPos( m_theme->elementRect( cityElement ).topLeft() );

        m_titles[ i ]->setFont( shrinkTextSizeToFit( m_titles[ i ]->text(), m_theme->elementRect( titleElement ) ) );
        m_dates[ i ]->setFont( shrinkTextSizeToFit( m_dates[ i ]->text(), m_theme->elementRect( dateElement ) ) );
        m_cities[ i ]->setFont( shrinkTextSizeToFit( m_cities[ i ]->text(), m_theme->elementRect( cityElement ) ) );
    }
}

void LastFmEvents::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data )
{
    DEBUG_BLOCK
    //debug() << "got data from engine: " << data;
    Context::DataEngine::DataIterator iter( data );
    if( name == QString( "sysevents" ) )
    {
        int count = 4; // system events are the 5th-10th rows
        while( iter.hasNext() )
        {
            iter.next();
            const QVariantList event = iter.value().toList();
            if( event.size() == 0 || count > 9 ) continue; // empty event, or we are done

            m_titles[ count ]->setFont( shrinkTextSizeToFit( m_titles[ count ]->text(), m_theme->elementRect(  QString( "title%1" ).arg( count ) ) ) );
            m_dates[ count ]->setFont( shrinkTextSizeToFit( m_dates[ count ]->text(), m_theme->elementRect( QString( "date%1" ).arg( count ) ) ) );
            m_cities[ count ]->setFont( shrinkTextSizeToFit( m_cities[ count ]->text(), m_theme->elementRect( QString( "city%1" ).arg( count ) ) ) );

            m_titles[ count ]->setText( truncateTextToFit( event[ 0 ].toString(),
                m_titles[ count ]->font(),
                m_theme->elementRect(  QString( "title%1" ).arg( count ) ) ) );
            m_dates[ count ]->setText( truncateTextToFit( event[ 1 ].toString(),
                m_dates[ count ]->font(),
                m_theme->elementRect(  QString( "date%1" ).arg( count ) ) ) );
            m_cities[ count ]->setText( truncateTextToFit( event[ 3 ].toString(),
                m_cities[ count ]->font(),
                m_theme->elementRect(  QString( "city%1" ).arg( count ) ) ) );


            count++;
        }
    }
    if( name == QString( "friendevents" ) )
    {
        int count = 0; // first 5 rows
        while( iter.hasNext() )
        {
            iter.next();
            const QVariantList event = iter.value().toList();
            if( event.size() == 0 || count > 4) continue; // empty event

            m_titles[ count ]->setFont( shrinkTextSizeToFit( m_titles[ count ]->text(), m_theme->elementRect( QString( "title%1" ).arg( count ) ) ) );
            m_dates[ count ]->setFont( shrinkTextSizeToFit( m_dates[ count ]->text(), m_theme->elementRect( QString( "date%1" ).arg( count ) ) ) );
            m_cities[ count ]->setFont( shrinkTextSizeToFit( m_cities[ count ]->text(), m_theme->elementRect( QString( "city%1" ).arg( count ) ) ) );

            m_titles[ count ]->setText( truncateTextToFit( event[ 0 ].toString(),
                m_titles[ count ]->font(),
                m_theme->elementRect(  QString( "title%1" ).arg( count ) ) ) );
            m_dates[ count ]->setText( truncateTextToFit( event[ 1 ].toString(),
                m_dates[ count ]->font(),
                m_theme->elementRect(  QString( "date%1" ).arg( count ) ) ) );
            m_cities[ count ]->setText( truncateTextToFit( event[ 3 ].toString(),
                m_cities[ count ]->font(),
                m_theme->elementRect(  QString( "city%1" ).arg( count ) ) ) );

            count++;
        }
    }
    if( name == QString( "userevents" ) )
    {
        int count = 10;
        while( iter.hasNext() )
        {
            iter.next();
            const QVariantList event = iter.value().toList();
            if( event.size() == 0 || count > 14) continue; // empty event

            m_titles[ count ]->setFont( shrinkTextSizeToFit( m_titles[ count ]->text(), m_theme->elementRect(  QString( "title%1" ).arg( count ) ) ) );
            m_dates[ count ]->setFont( shrinkTextSizeToFit( m_dates[ count ]->text(), m_theme->elementRect( QString( "date%1" ).arg( count ) ) ) );
            m_cities[ count ]->setFont( shrinkTextSizeToFit( m_cities[ count ]->text(), m_theme->elementRect( QString( "city%1" ).arg( count ) ) ) );

            m_titles[ count ]->setText( truncateTextToFit( event[ 0 ].toString(),
                m_titles[ count ]->font(),
                m_theme->elementRect(  QString( "title%1" ).arg( count ) ) ) );
            m_dates[ count ]->setText( truncateTextToFit( event[ 1 ].toString(),
                m_dates[ count ]->font(),
                m_theme->elementRect(  QString( "date%1" ).arg( count ) ) ) );
            m_cities[ count ]->setText( truncateTextToFit( event[ 3 ].toString(),
                m_cities[ count ]->font(),
                m_theme->elementRect(  QString( "city%1" ).arg( count ) ) ) );

            count++;
        }
    }

    update();
}

QSizeF
LastFmEvents::sizeHint( Qt::SizeHint which, const QSizeF & constraint) const
{
    Q_UNUSED( which );
    DEBUG_BLOCK
    if( constraint.height() == -1 && constraint.width() > 0 ) // asking height for given width basically
    {
        return QSizeF( constraint.width(), m_aspectRatio * constraint.width() );
    } else
    {
        return constraint;
    }
}


void LastFmEvents::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect& contentsRect )
{
//     DEBUG_BLOCK
    Q_UNUSED( option )

    //bail out if there is no room to paint. Prevents crashes and really there is no sense in painting if the
    //context view has been minimized completely
    if ( ( contentsRect.width() < 20 ) || ( contentsRect.height() < 20 ) )
    {
        debug() << "Too little room to paint, hiding all children ( making myself invisible but still painted )!";
        foreach ( QGraphicsItem * childItem, QGraphicsItem::children() ) {
            childItem->hide();
         }
         return;
    } else {
        foreach ( QGraphicsItem * childItem, QGraphicsItem::children () ) {
            childItem->show();
        }
    }

    p->setRenderHint(QPainter::SmoothPixmapTransform);

//     debug() << "painting rect: " << contentsRect << endl;
    m_theme->paint( p, contentsRect );

    for( int i = 0; i < 14; i++ )
    {
        QString titleElement = QString( "title%1" ).arg( i );
        QString dateElement = QString( "date%1" ).arg( i );
        QString cityElement = QString( "city%1" ).arg( i );

        QRectF titleRect = m_theme->elementRect( titleElement );
        QRectF dateRect = m_theme->elementRect( dateElement );
        QRectF cityRect = m_theme->elementRect( cityElement );

        m_titles[ i ]->setPos( titleRect.topLeft() );
        m_dates[ i ]->setPos( dateRect.topLeft() );
        m_cities[ i ]->setPos( cityRect.topLeft() );

    }

}

#include "LastFmEvents.moc"
