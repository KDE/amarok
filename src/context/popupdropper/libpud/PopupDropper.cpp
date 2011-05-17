/***************************************************************************
 *   Copyright (c) 2008  Jeff Mitchell <mitchell@kde.org>                  *
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

#include "PopupDropper.h"
#include "PopupDropper_p.h"
#include "PopupDropperItem.h"

#include <QtDebug>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QtSvg/QSvgRenderer>
#include <QAction>
#include <QMenu>
#include <QPalette>
#include <QTimeLine>
#include <QWidget>

PopupDropperPrivate::PopupDropperPrivate( PopupDropper* parent, bool sa, QWidget* widget )
    : QObject( parent )
    , standalone( sa )
    , widget( widget )
    , scene( 0 )
    , view( 0 )
    , fade( PopupDropper::FadeInOut )
    , fadeHideTimer()
    , fadeShowTimer()
    , fadeInTime( 800 )
    , fadeOutTime( 300 )
    , deleteTimer()
    , deleteTimeout( 1000 )
    , frameMax( 30 )
    , windowColor( 0, 0, 0, 64 )
    , windowBackgroundBrush()
    , baseTextColor( Qt::white )
    , hoveredTextColor( Qt::blue )
    , hoveredBorderPen()
    , hoveredFillBrush()
    , file()
    , sharedRenderer( 0 )
    , horizontalOffset( 30 )
    , pdiItems()
    , overlayLevel( 1 )
    , entered( false )
    , submenuMap()
    , submenu( false )
    , allItems()
    , quitOnDragLeave( false )
    , onTop( true )
    , widgetRect()
    , queuedHide( false )
    , q( parent )
{
    if( widget )
        widgetRect = widget->rect();
	windowBackgroundBrush.setColor( windowColor );
	hoveredBorderPen.setColor( Qt::blue );
    hoveredBorderPen.setWidth( 2 );
    hoveredBorderPen.setStyle( Qt::SolidLine );
	QColor hoveredFillColor = QColor( Qt::blue );
    hoveredFillColor.setAlpha( 32 );
    hoveredFillBrush.setColor( hoveredFillColor );
    hoveredFillBrush.setStyle( Qt::SolidPattern );
    scene = new QGraphicsScene( ( sa ? 0 : parent ) );
    view = new PopupDropperView( parent, scene, ( sa ? 0 : widget ) );
    //qDebug() << "on create, view size = " << view->size();
    deleteTimer.setSingleShot( true );
    fadeHideTimer.setDirection( QTimeLine::Backward );
    connect( &fadeHideTimer, SIGNAL( frameChanged(int) ), this, SLOT( fadeHideTimerFrameChanged(int) ) );
    connect( &fadeShowTimer, SIGNAL( frameChanged(int) ), this, SLOT( fadeShowTimerFrameChanged(int) ) );
    connect( &fadeHideTimer, SIGNAL( finished() ), this, SLOT( fadeHideTimerFinished() ) );
    connect( &fadeShowTimer, SIGNAL( finished() ), this, SLOT( fadeShowTimerFinished() ) );
    connect( &deleteTimer, SIGNAL( timeout() ), this, SLOT( deleteTimerFinished() ) );
}

PopupDropperPrivate::~PopupDropperPrivate()
{
}

void PopupDropperPrivate::newSceneView( PopupDropper* pud )
{
    scene->deleteLater();
    scene = new QGraphicsScene( pud );
    //qDebug() << "new scene width in newSceneView = " << scene->width();
    view = new PopupDropperView( pud, scene, widget );
    //qDebug() << "on create, view size = " << view->size();
}

void PopupDropperPrivate::setParent( QObject* parent )
{
    QObject::setParent( parent );
    q = static_cast<PopupDropper*>( parent );
}

void PopupDropperPrivate::fadeHideTimerFrameChanged( int frame ) //SLOT
{
    if( fadeHideTimer.state() == QTimeLine::Running )
    {
        qreal val = ( frame * 1.0 ) / frameMax;
        QColor color = windowColor;
        int alpha = (int)( color.alpha() * val );
        color.setAlpha( alpha );
        q->setPalette( color );
        foreach( PopupDropperItem* pdi, pdiItems )
            pdi->setSubitemOpacity( val );
    }
}

void PopupDropperPrivate::fadeShowTimerFrameChanged( int frame ) //SLOT
{
    if( fadeShowTimer.state() == QTimeLine::Running )
    {
        qreal val = ( frame * 1.0 ) / frameMax;
        QColor color = windowColor;
        int alpha = (int)( color.alpha() * val );
        color.setAlpha( alpha );
        q->setPalette( color );
        foreach( PopupDropperItem* pdi, pdiItems )
            pdi->setSubitemOpacity( val );
    }
}

void PopupDropperPrivate::fadeHideTimerFinished() //SLOT
{
    view->hide();
    //qDebug() << "Emitting fadeHideFinished in d pointer " << this;
    emit q->fadeHideFinished();
}

void PopupDropperPrivate::fadeShowTimerFinished() //SLOT
{
    q->setPalette( windowColor ); 
    queuedHide = false;
    foreach( PopupDropperItem* pdi, pdiItems )
        pdi->setSubitemOpacity( 1.0 );
}

void PopupDropperPrivate::dragEntered()
{
    //qDebug() << "PopupDropperPrivate::dragEntered";
    q->updateAllOverlays();
}

void PopupDropperPrivate::dragLeft()
{
    //qDebug() << "PopupDropperPrivate::dragLeft()";
    //qDebug() << "PUD to be hidden or not hidden = " << q;
    if( view->entered() && quitOnDragLeave )
    {
        view->setAcceptDrops( false );
        //qDebug() << "View entered, hiding";
        connect( q, SIGNAL( fadeHideFinished() ), q, SLOT( subtractOverlay() ) );
        q->hide();
    }
    q->updateAllOverlays();
}

void PopupDropperPrivate::startDeleteTimer()
{
    if( deleteTimeout == 0 )
        return;
    view->setEntered( false );
    //qDebug() << "Starting delete timer";
    deleteTimer.start( deleteTimeout );
}

void PopupDropperPrivate::deleteTimerFinished() //SLOT
{
    //qDebug() << "Delete Timer Finished";
    if( !view->entered() && quitOnDragLeave )
    {
        connect( q, SIGNAL( fadeHideFinished() ), q, SLOT( subtractOverlay() ) );
        q->hide();
    }
}

void PopupDropperPrivate::reposItems()
{
    qreal partitionsize, my_min, my_max, vert_center, item_min;
    //qDebug() << "allItems.size() = " << allItems.size();
    int counter = 0;
    for( int i = 0; i < allItems.size(); i++ )
    {
        //qDebug() << "item " << i;
        int verticalmargin = 5;
        partitionsize = scene->height() / pdiItems.size(); //gives partition size...now center in this area
        my_min = ( counter * partitionsize ) + verticalmargin;
        my_max = ( ( counter + 1 ) * partitionsize ) - verticalmargin;
        //qDebug() << "my_min = " << my_min << ", my_max = " << my_max;
        vert_center = ( ( my_max - my_min ) / 2 ) + my_min; //gives us our center line...now center the item around it
        PopupDropperItem* pItem = dynamic_cast<PopupDropperItem*>( allItems.at( i ) );
        QGraphicsLineItem* qglItem = 0;
        if( pItem )
        {
            pItem->setPopupDropper( q ); //safety
            //qDebug() << "item " << i << " is a PDI ";
            //If the svgElementRect is too high, resize it to fit
            pItem->svgElementRect().setHeight( my_max - my_min - ( 2 * verticalmargin ) );
            item_min = vert_center - ( pItem->svgElementRect().height() / 2 );
            //qDebug() << "vert_center = " << vert_center << ", ited->min = " << item_min;
            pItem->setPos( 0, my_min );
            pItem->borderRectItem()->setRect( 0 - pItem->borderWidth(), 0, scene->width() + 2*pItem->borderWidth(), my_max - my_min );
            pItem->scaleAndReposSvgItem();
            pItem->reposTextItem();
            pItem->reposHoverFillRects();
            pItem->update();
            //qDebug() << "size of view frame = " << view->size();
            ++counter;
        }
        else if( ( qglItem = dynamic_cast<QGraphicsLineItem*>( allItems.at( i ) ) ) )
        {
            //qDebug() << "item " << i << " is a QGLI";
            qglItem->setLine( horizontalOffset, (my_max-partitionsize), scene->width() - horizontalOffset, (my_max-partitionsize) );
        }
    }
}

bool PopupDropperPrivate::amIOnTop( PopupDropperView* pdv )
{
    if( onTop && pdv == view )
        return true;
    return false;
}

//////////////////////////////////////////////////////////////

PopupDropper::PopupDropper( QWidget *parent, bool standalone )
    : QObject( parent )
    , d( new PopupDropperPrivate( this, standalone, parent ) )
{
    if( !parent )
    {
        parent = d->view;
        d->widget = parent;
    }
    QObject::setParent( parent );
    initOverlay( parent );
    setColors( d->windowColor, d->baseTextColor, d->hoveredTextColor, d->hoveredBorderPen.color(), d->hoveredFillBrush.color() );
    d->sharedRenderer = new QSvgRenderer( this );
    d->overlayLevel = 1;
    //qDebug() << "Popup Dropper created!";
}

PopupDropper::~PopupDropper()
{
    //qDebug() << "Popup Dropper destroyed!";
}

int PopupDropper::overlayLevel() const
{
    return d->overlayLevel;
}

void PopupDropper::initOverlay( QWidget* parent, PopupDropperPrivate* priv )
{
    PopupDropperPrivate *pdp = priv ? priv : d;
    //qDebug() << "PUD Overlay being created, d pointer is " << d;
    pdp->scene->setSceneRect( QRectF( parent->rect() ) );
    //qDebug() << "Scene width = " << pdp->scene->width();
    pdp->scene->setItemIndexMethod( QGraphicsScene::NoIndex );
    pdp->view->setFixedSize( parent->size() );
    pdp->view->setLineWidth( 0 );
    pdp->view->setFrameStyle( QFrame::NoFrame );
    pdp->view->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    pdp->view->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    pdp->view->setBackgroundRole( QPalette::Window );
    pdp->view->setAutoFillBackground( true );
    pdp->fadeHideTimer.setFrameRange( 0, pdp->frameMax );
    pdp->fadeShowTimer.setFrameRange( 0, pdp->frameMax );
}

void PopupDropper::addOverlay()
{
    d->onTop = false;
    m_viewStack.push( d );
    PopupDropperPrivate* old_d = d;
    d = new PopupDropperPrivate( this, false, old_d->view );
    d->sharedRenderer = old_d->sharedRenderer;
    //qDebug() << "Adding overlay: ";
    initOverlay( old_d->view );
    setColors( d->windowColor, d->baseTextColor, d->hoveredTextColor, d->hoveredBorderPen.color(), d->hoveredFillBrush.color() );
    d->quitOnDragLeave = true;
    d->overlayLevel = old_d->overlayLevel + 1;
    old_d->view->deactivateHover();
}

//note: Used when activating a submenu; does not set default colors, should have done that when creating submenu
void PopupDropper::addOverlay( PopupDropperPrivate* newD )
{
    d->onTop = false;
    //qDebug() << "right before push, m_viewStack.size() is " << m_viewStack.size();
    m_viewStack.push( d );
    //qDebug() << "right after push, m_viewStack.size() is " << m_viewStack.size();
    PopupDropperPrivate* old_d = d;
    d = newD;
    d->onTop = true;
    d->sharedRenderer = old_d->sharedRenderer;
    d->quitOnDragLeave = true;
    d->overlayLevel = old_d->overlayLevel + 1;
    //qDebug() << "new d d->overlayLevel = " << d->overlayLevel;
    //qDebug() << "in PopupDropper " << this;
}

//NOTE: just like you have to show the overlay when adding, this function does not hide the overlay; you must do that yourself
//with hide() (which will hide just one level)
bool PopupDropper::subtractOverlay()
{
    //qDebug() << "subtractOverlay, with d pointer " << d;
    disconnect( this, SLOT( subtractOverlay() ) );
    while( !isHidden() && d->fadeHideTimer.state() == QTimeLine::Running )
    {
        //qDebug() << "singleshotting subtractOverlay";
        QTimer::singleShot( 0, this, SLOT( subtractOverlay() ) );
        return false;
    }
    //qDebug() << "in PopupDropper " << this;
    //qDebug() << "overlay d->overlayLevel = " << d->overlayLevel;
    if( d->overlayLevel == 1 )
        return false;
    PopupDropper::Fading currFadeValue = d->fade;
    d->fade = PopupDropper::NoFade;
    d->onTop = false;
    PopupDropperPrivate* old_d = d;
    //qDebug() << "right before pop, m_viewStack.size() is " << m_viewStack.size();
    d = m_viewStack.pop();
    d->onTop = true;
    if( !old_d->submenu )
    {
        //qDebug() << "not submenu, deleting";
        old_d->deleteLater();
    }
    else
    {
        foreach( QGraphicsItem* item, old_d->pdiItems )
            old_d->scene->removeItem( item );
        //qDebug() << "not deleting, submenu";
        old_d->fade = currFadeValue;
        old_d->view->resetView();
    }
    d->startDeleteTimer();
    return true;
}

PopupDropperItem* PopupDropper::addSubmenu( PopupDropper** pd, const QString &text )
{
    //qDebug() << "addSubmenu, this is " << this << " and passed-in PopupDropper is " << (*pd);
    if( !(*pd) )
    {
        qWarning() << "Did not pass in a valid PUD!";
        return 0;
    }
    PopupDropperPrivate* newD = (*pd)->d;
    newD->submenu = true;
    newD->widget = d->widget;
    newD->setParent( this );
    foreach( PopupDropperItem* item, newD->pdiItems )
        newD->scene->removeItem( item );
    newD->newSceneView( this );
    initOverlay( d->widget, newD );
    PopupDropperItem* pdi = new PopupDropperItem();

    QAction* action = new QAction( text, this );

    connect( action, SIGNAL( hovered() ), this, SLOT( activateSubmenu() ) );
    pdi->setAction( action );
    pdi->setSubmenuTrigger( true );
    pdi->setHoverIndicatorShowStyle( PopupDropperItem::OnHover );
    d->submenuMap[action] = newD;
    delete (*pd);
    (*pd) = 0;
    foreach( PopupDropperItem* item, newD->pdiItems )
        item->setPopupDropper( this );
    //qDebug() << "d->submenuMap[pda] = " << d->submenuMap[pda];
    addItem( pdi );
    return pdi;
}

void PopupDropper::activateSubmenu()
{
    //qDebug() << "Sender is " << QObject::sender();
    if( isHidden() || d->fadeHideTimer.state() == QTimeLine::Running )
        return;
    PopupDropperPrivate* oldd = d;
    addOverlay( d->submenuMap[static_cast<QAction*>(QObject::sender())] );
    //qDebug() << "d->pdiItems.size() = " << d->pdiItems.size() << " for " << d;
    foreach( PopupDropperItem* item, d->pdiItems )
        addItem( item, false, false );
    oldd->view->deactivateHover();
    show();
}

bool PopupDropper::addMenu( const QMenu *menu )
{
    Q_UNUSED(menu)
    if( !menu )
        return false;
        
    if( menu->actions().size() == 0 )
        return true;

    PopupDropperItem *pdi = 0;
    foreach( QAction *action, menu->actions() )
    {
        if( !action->menu() )
        {
            pdi = new PopupDropperItem();
            pdi->setAction( action );
            addItem( pdi );
        }
        else
        {
            PopupDropper *pd = new PopupDropper( 0 );
            bool success = pd->addMenu( action->menu() );
            if( success )
                pdi = addSubmenu( &pd, action->text() );
        }
        pdi = 0;
    }
    
    return true;
}

bool PopupDropper::standalone() const
{
    return d->standalone;
}

void PopupDropper::show()
{
    if( !d->sharedRenderer )
    {
        //qDebug() << "No shared renderer set!";
        return;
    }
    if( d->widget && d->widget->rect() != d->widgetRect )
    {
        d->widgetRect = d->widget->rect();
        d->scene->setSceneRect( d->widget->rect() );
        d->view->setFixedSize( d->widget->size() );
        update();
    }
    //qDebug() << "Showing PopupDropper";
    d->fadeShowTimer.stop();
    if( ( d->fade == PopupDropper::FadeIn || d->fade == PopupDropper::FadeInOut ) && d->fadeInTime > 0 )
    {
        //qDebug() << "Animating!";
        d->fadeShowTimer.setDuration( d->fadeInTime );
        d->fadeShowTimer.setCurrentTime( 0 );
        d->fadeShowTimer.setCurveShape( QTimeLine::EaseOutCurve );
        QColor color = d->windowColor;
        color.setAlpha( 0 );
        setPalette( color );
        foreach( PopupDropperItem* pdi, d->pdiItems )
            pdi->setSubitemOpacity( 0.0 );
        d->fadeShowTimer.start();
        //qDebug() << "Timer started";
    }
    d->view->show();
}

void PopupDropper::showAllOverlays()
{
    show();
    for( int i = m_viewStack.size() - 1; i >= 0; --i )
    {
        PopupDropperPrivate* pdi = m_viewStack.at( i );
        if( pdi != d )
            d->view->show();
    }
}

//returns immediately! 
void PopupDropper::hide()
{
    //qDebug() << "PopupDropper::hide entered, d pointer is = " << d;

    //if hide is called and the view is already hidden, it's likely spurious
    if( isHidden() )
    {
        //qDebug() << "ishidden, returning";
        return;
    }

    //queuedHide is to make sure that fadeShowTimerFinished executes before this next hide()
    if( d->fadeShowTimer.state() == QTimeLine::Running )
    {
        //qDebug() << "show timer running, queueing hide";
        d->fadeShowTimer.stop();
        d->queuedHide = true;
        QTimer::singleShot( 0, d, SLOT( fadeShowTimerFinished() ) );
        QTimer::singleShot( 0, this, SLOT( hide() ) );
        return;
    }

    //queuedHide will be set to false from fadeShowTimerFinished...so if this came up first,
    //then wait
    if( d->fadeHideTimer.state() == QTimeLine::Running || d->queuedHide )
    {
        //qDebug() << "hide timer running or queued hide";
        QTimer::singleShot( 0, this, SLOT( hide() ) );
        return;
    }

    if( ( d->fade == PopupDropper::FadeOut || d->fade == PopupDropper::FadeInOut ) && d->fadeOutTime > 0 )
    {
        //qDebug() << "Starting fade out";
        d->fadeHideTimer.setDuration( d->fadeOutTime );
        d->fadeHideTimer.setCurveShape( QTimeLine::LinearCurve );
        d->fadeHideTimer.start();
        //qDebug() << "Timer started";
        return;
    }
    else  //time is zero, or no fade
    {
        //qDebug() << "time is zero, or no fade";
        QTimer::singleShot( 0, d, SLOT( fadeHideTimerFinished() ) );
        return;
    }
}

void PopupDropper::hideAllOverlays()
{
    //qDebug() << "Entered hideAllOverlays";
    connect( this, SIGNAL( fadeHideFinished() ), this, SLOT( slotHideAllOverlays() ) );
    hide();
    //qDebug() << "Leaving hideAllOverlays";
}

void PopupDropper::slotHideAllOverlays()
{
    //qDebug() << "Entered slotHideAllOverlays()";
    disconnect( this, SIGNAL( fadeHideFinished() ), this, SLOT( slotHideAllOverlays() ) );
    //qDebug() << "m_viewStack.size() = " << m_viewStack.size();
    for( int i = m_viewStack.size() - 1; i >= 0; --i )
    {
        PopupDropperPrivate* pdp = m_viewStack.at( i );
        //qDebug() << "checking pdp = " << (QObject*)pdp << ", d is " << (QObject*)d;
        if( pdp != d )
            pdp->view->hide();
    }
    //qDebug() << "Leaving slotHideAllOverlays";
}

void PopupDropper::update()
{
    d->reposItems();
    d->view->update();
}

void PopupDropper::updateAllOverlays()
{
    for( int i = m_viewStack.size() - 1; i >= 0; --i )
    {
        PopupDropperPrivate* pdp = m_viewStack.at( i );
        pdp->view->update();
    }
    d->view->update();
}

bool PopupDropper::isHidden() const
{
    return d->view->isHidden();
}

void PopupDropper::clear()
{
    while( !isHidden() && d->fadeHideTimer.state() == QTimeLine::Running )
    {
        QTimer::singleShot(0, this, SLOT( clear() ) );
        return;
    }
    //qDebug() << "Clear happening!";
    disconnect( this, SLOT( clear() ) );
    do
    {
        foreach( QGraphicsItem* item, d->allItems )
        {
            if( dynamic_cast<PopupDropperItem*>(item) )
            {
                if( dynamic_cast<PopupDropperItem*>(item)->isSubmenuTrigger() )
                {
                    //qDebug() << "Disconnecting action";
                    disconnect( dynamic_cast<PopupDropperItem*>(item)->action(), SIGNAL( hovered() ), this, SLOT( activateSubmenu() ) );
                }
                dynamic_cast<PopupDropperItem*>(item)->deleteLater();
            }
            else
                delete item;
        }
        d->pdiItems.clear();
        d->allItems.clear();
        //qDebug() << "Size of pdiItems is now " << d->pdiItems.size();
        //qDebug() << "Size of allItems is now " << d->allItems.size();
        d->view->hide();
        d->view->resetView();
    } while ( subtractOverlay() );
}

bool PopupDropper::isEmpty( bool allItems ) const
{
    if( allItems )
        return d->allItems.empty();
    else
        return d->pdiItems.empty();
}

bool PopupDropper::quitOnDragLeave() const
{
    return d->quitOnDragLeave;
}

void PopupDropper::setQuitOnDragLeave( bool quit )
{
    d->quitOnDragLeave = quit;
}

int PopupDropper::fadeInTime() const
{
    return d->fadeInTime;
}

void PopupDropper::setFadeInTime( const int msecs )
{
    d->fadeInTime = msecs;
}

int PopupDropper::fadeOutTime() const
{
    return d->fadeOutTime;
}

void PopupDropper::setFadeOutTime( const int msecs )
{
    d->fadeOutTime = msecs;
}

PopupDropper::Fading PopupDropper::fading() const
{
    return d->fade;
}

void PopupDropper::setFading( PopupDropper::Fading fade )
{
    d->fade = fade;
}

const QTimeLine* PopupDropper::fadeHideTimer() const
{
    return &d->fadeHideTimer;
}

const QTimeLine* PopupDropper::fadeShowTimer() const
{
    return &d->fadeShowTimer;
}

int PopupDropper::deleteTimeout() const
{
    return d->deleteTimeout;
}

void PopupDropper::setDeleteTimeout( int msecs )
{
    d->deleteTimeout = msecs;
}

QColor PopupDropper::windowColor() const
{
    return d->windowColor;
}

void PopupDropper::setWindowColor( const QColor &window )
{
    d->windowColor = window;
    setPalette( window );
}

QBrush PopupDropper::windowBackgroundBrush() const
{
    return d->windowBackgroundBrush;
}

void PopupDropper::setWindowBackgroundBrush( const QBrush &window )
{
    d->windowBackgroundBrush = window;
    d->view->setBackgroundBrush( window );
}

QColor PopupDropper::baseTextColor() const
{
    return d->baseTextColor;
}

void PopupDropper::setBaseTextColor( const QColor &text )
{
    d->baseTextColor = text;
    foreach( PopupDropperItem *item, d->pdiItems )
        item->setBaseTextColor( text );
}

QColor PopupDropper::hoveredTextColor() const
{
    return d->hoveredTextColor;
}

void PopupDropper::setHoveredTextColor( const QColor &text )
{
    d->hoveredTextColor = text;
    foreach( PopupDropperItem *item, d->pdiItems )
        item->setHoveredTextColor( text );
}

QPen PopupDropper::hoveredBorderPen() const
{
    return d->hoveredBorderPen;
}

void PopupDropper::setHoveredBorderPen( const QPen &border )
{
    d->hoveredBorderPen = border;
    foreach( PopupDropperItem *item, d->pdiItems )
        item->setHoveredBorderPen( border );
}

QBrush PopupDropper::hoveredFillBrush() const
{
    return d->hoveredFillBrush;
}

void PopupDropper::setHoveredFillBrush( const QBrush &fill )
{
    d->hoveredFillBrush = fill;
    foreach( PopupDropperItem *item, d->pdiItems )
        item->setHoveredFillBrush( fill );
}

void PopupDropper::setColors( const QColor &window, const QColor &baseText, const QColor &hoveredText, const QColor &hoveredBorder, const QColor &hoveredFill )
{
    d->windowColor = window;
    d->baseTextColor = baseText;
    d->hoveredTextColor = hoveredText;
    d->hoveredBorderPen.setColor( hoveredBorder );
    d->hoveredFillBrush.setColor( hoveredFill );
    setPalette( window, baseText, hoveredText, hoveredBorder, hoveredFill );
}

void PopupDropper::setPalette( const QColor &window )
{
    QPalette p = d->view->palette();
    p.setColor( QPalette::Window, window );
    d->view->setPalette( p );
    updateAllOverlays();
}

void PopupDropper::setPalette( const QColor &window, const QColor &baseText, const QColor &hoveredText, const QColor &hoveredBorder, const QColor &hoveredFill )
{
    QPalette p = d->view->palette();
    p.setColor( QPalette::Window, window );
    d->view->setPalette( p );
    QPen pen;
    QBrush brush;
    foreach( PopupDropperItem *item, d->pdiItems )
    {
        item->setBaseTextColor( baseText );
        item->setHoveredTextColor( hoveredText );
        pen = item->hoveredBorderPen();
        pen.setColor( hoveredBorder );
        item->setHoveredBorderPen( pen );
        brush = item->hoveredFillBrush();
        brush.setColor( hoveredFill );
        item->setHoveredFillBrush( brush );
    } 
    updateAllOverlays();
}

QString PopupDropper::windowTitle() const
{
    return d->view->windowTitle();
}

void PopupDropper::setWindowTitle( const QString &title )
{
    d->view->setWindowTitle( title );
    d->view->update();
}

QString PopupDropper::svgFile() const
{
    return d->file;
}

void PopupDropper::setSvgFile( const QString &file )
{
    if( d->sharedRenderer )
    {
        if( !d->sharedRenderer->load( file ) )
            qWarning() << "Could not load SVG file " << file;
        else
        {
            d->file = file;
            //qDebug() << "Loaded SVG file!";
        }
    }
    else
        qWarning() << "No shared renderer!";
}

QSvgRenderer* PopupDropper::svgRenderer()
{
    return d->sharedRenderer;
}

void PopupDropper::setSvgRenderer( QSvgRenderer *renderer )
{
    d->sharedRenderer = renderer;
}

int PopupDropper::horizontalOffset() const
{
    return d->horizontalOffset;
}

void PopupDropper::setHorizontalOffset( int pixels )
{
    d->horizontalOffset = pixels;
}

const QSize PopupDropper::viewSize() const
{
    if( d && d->view )
        return d->view->size();
    else
        return QSize( 0, 0 );
}

void PopupDropper::addItem( PopupDropperItem *item, bool useSharedRenderer )
{
    addItem( item, useSharedRenderer, true );
}

void PopupDropper::addItem( PopupDropperItem *item, bool useSharedRenderer, bool appendToList )
{
    //qDebug() << "adding item";
    //FIXME: Make separators do something graphical instead of just ignoring them
    PopupDropperItem *pItem = static_cast<PopupDropperItem*>( item );
    if( pItem->isSeparator() )
        return;
    if( useSharedRenderer )
        pItem->setSharedRenderer( d->sharedRenderer );
    //qDebug() << "Checking appendToList";
    if( appendToList )
    {
        d->pdiItems.append( pItem );
        d->allItems.append( pItem );
        //qDebug() << "pdiItems list is now size " << d->pdiItems.size() << " for " << d;
        //qDebug() << "allItems list is now size " << d->allItems.size() << " for " << d;
    }
    if( !pItem->textItem() )
    {
        QGraphicsTextItem *textItem = new QGraphicsTextItem( pItem->text(), pItem );
        pItem->setTextItem( textItem );
        if( !pItem->customBaseTextColor() || !pItem->baseTextColor().isValid() )
        {
            pItem->setBaseTextColor( d->baseTextColor );
            //qDebug() << "Using PD's base text color";
        }
        else
        {
            //qDebug() << "Using the item's base text color";
            pItem->textItem()->setDefaultTextColor( pItem->baseTextColor() );
        }
        if( !pItem->customHoveredTextColor() )
            pItem->setHoveredTextColor( d->hoveredTextColor );
    }
    if( !pItem->borderRectItem() )
    {
        QGraphicsRectItem *borderRectItem = new QGraphicsRectItem( pItem );
        borderRectItem->setZValue( -5 );
        pItem->setBorderRectItem( borderRectItem );
        if( !pItem->customHoveredBorderPen() )
            pItem->setHoveredBorderPen( d->hoveredBorderPen );
        if( !pItem->customHoveredFillBrush() )
            pItem->setHoveredFillBrush( d->hoveredFillBrush );
    }
    d->reposItems();
    pItem->setPopupDropper( this );
    d->scene->addItem( pItem );
}

QList<PopupDropperItem*> PopupDropper::items() const
{
    QList<PopupDropperItem*> list;
    foreach( PopupDropperItem *item, d->pdiItems )
        list.append( item );

    return list;
}

//Won't currently work for > 1 level of submenu!
//TODO: Figure out a better way. (Does anything else work > 1 level?)
QList<PopupDropperItem*> PopupDropper::submenuItems( const PopupDropperItem *item ) const
{
    QList<PopupDropperItem*> list;
    if( !item || !item->isSubmenuTrigger() || !d->submenuMap.contains( item->action() ) )
        return list;

    PopupDropperPrivate *pdp = d->submenuMap[item->action()];
    foreach( PopupDropperItem *pdi, pdp->pdiItems )
        list.append( pdi );

    return list;
}

//Goes through and calls the callback on all items, including submenuItems
//which can be adjusted differently by checking isSubmenuTrigger()
void PopupDropper::forEachItem( void callback(void*) )
{
    forEachItemPrivate( d, callback );
}

void PopupDropper::forEachItemPrivate( PopupDropperPrivate *pdp, void callback(void* item) )
{
    foreach( PopupDropperItem *item, pdp->pdiItems )
        callback( item );
    foreach( QAction *action, pdp->submenuMap.keys() )
        forEachItemPrivate( pdp->submenuMap[action], callback );
}

void PopupDropper::addSeparator( PopupDropperItem* separator )
{

    if( !separator )
    {
        //qDebug() << "Action is not a separator!";
        return;
    }

    separator->setSeparator( true );

    if( separator->separatorStyle() == PopupDropperItem::TextSeparator )
    {
        //qDebug() << "Separator style is text";
        addItem( separator );
    }

    //qDebug() << "Separator style is line";
    QPen linePen;
    if( separator && separator->hasLineSeparatorPen() )
        linePen = separator->lineSeparatorPen();
    else
    {
        linePen.setWidth( 2 );
        linePen.setCapStyle( Qt::RoundCap );
        linePen.setStyle( Qt::DotLine );
        linePen.setColor( QColor( 255, 255, 255 ) );
    }

    //qDebug() << "scene width = " << d->scene->width() << ", horizontalOffset = " << d->horizontalOffset;
    //qDebug() << "right side = " << qreal(d->scene->width() - d->horizontalOffset);
    QGraphicsLineItem* lineItem = new QGraphicsLineItem( 0, 0, 0, 0 );
    d->allItems.append( lineItem );
    lineItem->setPen( linePen );
    d->reposItems();
    d->scene->addItem( lineItem );
}

#include "PopupDropper_p.moc"
#include "PopupDropper.moc"

