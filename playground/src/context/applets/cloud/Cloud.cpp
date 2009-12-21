/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhn@kde.org>                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "Cloud.h"

#include "Amarok.h"
#include "Debug.h"
#include "context/Svg.h"

#include <QBrush>
#include <QDBusInterface>
#include <QPainter>

Cloud::Cloud( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_runningX( 0.0 )
    , m_runningY( 0.0 )
    , m_currentLineMaxHeight( 0.0 )
    , m_maxFontSize( 30 )
    , m_minFontSize( 3 )
    , m_maxHeightInFirstLine( 0.0 )
    , m_config( 0 )
    , m_configLayout( 0 )
    , m_width( 0 )
    , m_aspectRatio( 0 )
    , m_size( QSizeF() )
    , m_initialized( false )

{
    DEBUG_BLOCK

    setHasConfigurationInterface( false );

    dataEngine( "amarok-cloud" )->connectSource( "cloud", this );

    m_theme = new Context::Svg( this );
    m_theme->setImagePath( "widgets/amarok-cloud" );
    m_theme->setContainsMultipleImages( false );
    m_theme->resize( m_size );
    m_width = globalConfig().readEntry( "width", 500 );

    m_cloudName = new QGraphicsSimpleTextItem( this );
    m_cloudName->setText( "Cloud View ( empty )" );

    //m_cloudName->setBrush( QBrush( Qt::white ) );

    // get natural aspect ratio, so we can keep it on resize
    m_theme->resize();
    m_aspectRatio = (qreal)m_theme->size().height() / (qreal)m_theme->size().width();

    setPreferredSize( m_theme->size() );

    constraintsEvent();
}

Cloud::~Cloud()
{}

void Cloud::constraintsEvent( Plasma::Constraints constraints )
{
    prepareGeometryChange();

    if (constraints & Plasma::SizeConstraint && m_theme) {
        m_theme->resize(size().toSize());
    }

    //make the text as large as possible:
    m_cloudName->setFont( shrinkTextSizeToFit( m_cloudName->text(), m_theme->elementRect( "cloud_name" ) ) );

    //center it
    float textWidth = m_cloudName->boundingRect().width();
    float totalWidth = m_theme->elementRect( "cloud_name" ).width();
    float offsetX =  ( totalWidth - textWidth ) / 2;

    kDebug() << "offset: " << offsetX;

    m_cloudName->setPos( m_theme->elementRect( "cloud_name" ).topLeft() + QPointF ( offsetX, 0 ) );

    drawCloud();

    m_initialized = true;
}

void Cloud::dataUpdated( const QString& name, const Plasma::DataEngine::Data& data )
{
    DEBUG_BLOCK
    Q_UNUSED( name );

    if( data.size() == 0 ) return;

    kDebug() << "got data from engine: " << data[ "cloud_name" ].toString();


    //kDebug() << "got new data" << infoMap;

    m_strings = data[ "cloud_strings" ].toList();
    m_weights = data[ "cloud_weights" ].toList();
    m_actions = data[ "cloud_actions" ].toMap();

    if ( m_initialized ) {
        m_cloudName->setText( data[ "cloud_name" ].toString() );

        float textWidth = m_cloudName->boundingRect().width();
        float totalWidth = m_theme->elementRect( "cloud_name" ).width();
        float offsetX =  ( totalWidth - textWidth ) / 2;

        m_cloudName->setPos( m_theme->elementRect( "cloud_name" ).topLeft() + QPointF ( offsetX, 0 ) );
        
        drawCloud();


    }


}

void Cloud::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{
    Q_UNUSED( option );

    //bail out if there is no room to paint. Prevents crashes and really there is no sense in painting if the
    //context view has been minimized completely
    if ( ( contentsRect.width() < 40 ) || ( contentsRect.height() < 40 ) )
    {
        debug() << "Too little room to paint, hiding all children ( making myself invisible but still painted )!";
        foreach ( QGraphicsItem * childItem, QGraphicsItem::children() )
        {
            childItem->hide();
        }
        return;
    } else
    {
        foreach ( QGraphicsItem * childItem, QGraphicsItem::children () )
        {
            childItem->show();
        }
    }

    p->save();
    m_theme->paint( p, contentsRect/*, "background" */);
    p->restore();
}

bool Cloud::hasHeightForWidth() const
{
    return true;
}

qreal Cloud::heightForWidth(qreal width) const
{
    return width * m_aspectRatio;
}

void Cloud::addText( const QString &text, int weight )
{
    // create the new text item
    CloudTextItem * item = new CloudTextItem ( text, this, 0 );
    //item->setParentItem( this );
    //item->connect ( item, SIGNAL( clicked( QString ) ), receiver, slot );
    QFont font = item->font();
    //font.setPointSize( (int)weight * 1.5 + 10 );
    font.setPointSize( (int)weight + 8 );
    item->setFont( font );

    QRectF itemRect = item->boundingRect();
    QRectF parentRect = m_theme->elementRect( "cloud" );

    // check if item will fit inside cloud at all... if not, just skip it
    // (Does anyone have a better idea how to handle this? )
    if  ( itemRect.width() > parentRect.width() ) {
        delete item;
        return;
    }

    // Check if item will fit on the current line, if not, print current line
    if ( ( itemRect.width() + m_runningX ) > parentRect.width() )
    {
        adjustCurrentLinePos();
        m_runningX = 0;
    }

    m_runningX += itemRect.width();

    m_currentLineItems.append( item );
    m_textItems.append( item );

    connect( item, SIGNAL( clicked(const QString&) ), this, SLOT( cloudItemActivated(const QString&) ) );
}

void Cloud::adjustCurrentLinePos()
{
    if ( m_currentLineItems.isEmpty() ) return;

    int totalWidth = 0;
    int maxHeight = 0;
    int offsetX = 0;
    int offsetY = 0;
    int currentX = 0;

    CloudTextItem * currentItem;
    QRectF currentItemRect;

    //First we run through the list to get the max height of an item
    // and the total width of all items.
    foreach( currentItem, m_currentLineItems )
    {
        //currentItem->setTextWidth ( -1 ); //do not break lines, ever!
        //currentItem->adjustSize();
        currentItemRect = currentItem->boundingRect();
        totalWidth += (int)currentItemRect.width();
        if ( currentItemRect.height() > maxHeight )
            maxHeight = (int)currentItemRect.height();
    }

    if ( m_maxHeightInFirstLine < 1.0 )
        m_maxHeightInFirstLine = maxHeight;

    //wont work with fixed size... should make area scrollable instead TODO
    //do we have enough vertical space for this line? If not, create some!
    /*if ( ( m_runningY + maxHeight ) >  m_theme->elementRect( "cloud" ).height() ) {
        int missingHeight = ( m_runningY + maxHeight ) - m_theme->elementRect( "cloud" ).height();
        setContentRectSize( QSize( m_theme->elementRect( "cloud" ).width(), m_theme->elementRect( "cloud" ).height() + missingHeight ) );
    }*/

    //calc the X offset that makes the line centered horizontally
    offsetX = ( m_theme->elementRect( "cloud" ).width() - totalWidth ) / 2;

    //then remove all items from the list, setting the correct position of each
    // in the process
    while( !m_currentLineItems.isEmpty() )
    {
        currentItem = m_currentLineItems.takeFirst();
        currentItemRect = currentItem->boundingRect();
        offsetY = (int)( (maxHeight - currentItemRect.height()) / 2 );

        //until we get a scroll area, don't print beyound the bottom of the rect
        if ( m_runningY + offsetY + m_maxHeightInFirstLine + currentItemRect.height() >  m_theme->elementRect( "cloud" ).bottomLeft().y() ) {
            m_textItems.removeAll( currentItem );
            delete currentItem;
        } else {
            currentItem->setPos( QPointF( currentX + offsetX, m_runningY + offsetY + m_maxHeightInFirstLine ) );
            currentX += (int)currentItemRect.width();
        }
    }

    m_runningY += maxHeight;
}

CloudTextItem::CloudTextItem( const QString &text, QGraphicsItem * parent, QGraphicsScene * scene )
    : QGraphicsTextItem ( text, parent, scene )
{
    setAcceptsHoverEvents( true );
    m_timeLine = new QTimeLine( 1000, this );
    connect( m_timeLine, SIGNAL( frameChanged( int ) ), this, SLOT( colorFadeSlot( int ) ) );

}

void CloudTextItem::hoverEnterEvent(QGraphicsSceneHoverEvent * event)
{
    Q_UNUSED( event );
   // debug() << "CloudTextItem::hoverEnterEvent!! " << endl;
    m_timeLine->stop();
    m_timeLine->setCurrentTime ( 0 );

    setDefaultTextColor( QColor( 0, 127, 255 ) );
    update();
}

void CloudTextItem::hoverLeaveEvent(QGraphicsSceneHoverEvent * event)
{
    Q_UNUSED( event );

    // Construct a 1-second timeline with a frame range of 0 - 30
    m_timeLine->setFrameRange(0, 30);
    m_timeLine->start();
}

void CloudTextItem::colorFadeSlot( int step )
{
    int colorValue = static_cast<int>(255 - step * 8.5);
    if ( step == 100 ) colorValue = 0;

    setDefaultTextColor( QColor( 0, colorValue / 2, colorValue ) );
    update();
}

void CloudTextItem::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    Q_UNUSED( event )
    debug() << "Mouse clicked!! " << endl;
    emit( clicked( toPlainText() ) );
}

void Cloud::drawCloud()
{
    //clear all and start over
    m_maxHeightInFirstLine = 0.0;

    while ( !m_textItems.isEmpty() )
        delete m_textItems.takeFirst();


    //make the cloud map valuse sane:
    cropAndNormalize( 1, 40 );

    m_runningY = m_theme->elementRect( "cloud_name" ).topLeft().y();

    if ( m_strings.size() == m_weights.size() ) {
        int index = 0;
        foreach( const QVariant &stringVariant, m_strings ) {
            QString string = stringVariant.toString();
            int weight = m_weights.at( index ).toInt();
            index++;

            kDebug() << "Adding string '" << string << "' with weight " << weight;
            addText( string, weight );
        }
        adjustCurrentLinePos();
    }
}

void Cloud::cropAndNormalize( int minCount, int maxCount )
{
    int min = 100000;
    int max = 0;

    foreach( const QVariant &weight, m_weights )
    {
        if ( weight.toInt() < min )
            min = weight.toInt();
        if (  weight.toInt() > max )
            max = weight.toInt();
    }

    if ( min < minCount )
        min = minCount;
    if ( max > maxCount )
        max = maxCount;

    //determine scale factor
    int range = max - min;
    int scaleFactor = range / 10;

    if ( scaleFactor == 0 )
        scaleFactor = 1;

    int index = 0;

    //meh, opimize later if needed...
    QList<QVariant> m_newStrings;
    QList<QVariant> m_newWeights;

    foreach( const QVariant &stringVariant, m_strings ) {
        int weight = m_weights.at( index ).toInt();
        if ( ( weight >= minCount ) && ( weight < maxCount ) ) {
            m_newStrings.append( stringVariant.toString() );
            m_newWeights.append( weight / scaleFactor );
        } else if ( weight > maxCount ) {
            m_newStrings.append( stringVariant.toString() );
            m_newWeights.append( maxCount / scaleFactor );
        }

        index++;
    }

    m_strings = m_newStrings;
    m_weights = m_newWeights;
}

void Cloud::cloudItemActivated( const QString & text )
{
    kDebug() << "cloudItemActivated: '" << text;
    //find the index of this item in the list

    QVariantMap action = m_actions[text].toMap();

    QString component = action["component"].toString();
    QString function = action["function"].toString();
    QString arg1 = action["arg1"].toString();
    QString arg2 = action["arg2"].toString();
    QString arg3 = action["arg3"].toString();
    QString arg4 = action["arg4"].toString();

    kDebug() << component << ", " << function << ", " << arg1 << ", " << arg2 << ", " << arg3 << ", " << arg4;

    if( ( !component.isEmpty() ) && ( !function.isEmpty() ) )
    {
        QDBusInterface interface( "org.kde.amarok", component );

        if ( !arg4.isEmpty() )
            interface.call( function, arg1, arg2, arg3, arg4 );
        else if ( !arg3.isEmpty() )
            interface.call( function, arg1, arg2, arg3 );
        else if ( !arg2.isEmpty() )
            interface.call( function, arg1, arg2 );
        else if ( !arg1.isEmpty() )
            interface.call( function, arg1 );
    }
}

#include "Cloud.moc"


QSizeF Cloud::sizeHint(Qt::SizeHint which, const QSizeF & constraint) const
{
    if( constraint.height() == -1 && constraint.width() > 0 ) // asking height for given width basically
    {
        return QSizeF( constraint.width(), m_aspectRatio * constraint.width() );
    } else
    {
        return constraint;
    }
}

