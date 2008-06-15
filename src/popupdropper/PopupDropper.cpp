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

#include <QtDebug>
#include <QAction>
#include <QApplication>
#include <QGraphicsItem>
#include <QGraphicsTextItem>
#include <QGraphicsScene>
#include <QGraphicsSceneDragDropEvent>
#include <QtSvg/QSvgRenderer>
#include <QtSvg/QGraphicsSvgItem>
#include <QPalette>
#include <QTimeLine>
#include <QWidget>

#include "math.h"

#include "PopupDropper.h"
#include "PopupDropper_p.h"
#include "PopupDropperAction.h"
#include "PopupDropperItem.h"

PopupDropperPrivate::PopupDropperPrivate( PopupDropper* parent, bool sa, QWidget* widget )
    : QObject( parent )
    , standalone( sa )
    , widget( widget )
    , scene( 0 )
    , view( 0 )
    , success( false )
    , fade( PopupDropper::FadeInOut )
    , fadeTimer()
    , fadeInTime( 800 )
    , fadeOutTime( 300 )
    , deleteTimer()
    , deleteTimeout( 1000 )
    , closeAtEndOfFade( true )
    , frameMax( 30 )
    , windowColor( 0, 0, 0, 64 )
    , textColor( 255, 255, 255, 255 )
    , file()
    , sharedRenderer( new QSvgRenderer( this ) )
    , horizontalOffset( 30 )
    , itemCount( 0 )
    , totalItems( 0 )
    , pdiItems()
    , overlayLevel( 1 )
    , entered( false )
    , submenuMap()
    , submenu( false )
    , doNotClear( false )
    , q( parent )
{
    scene = new QGraphicsScene( ( sa ? 0 : parent ) );
    view = new PopupDropperView( parent, scene, ( sa ? 0 : widget ) );
    deleteTimer.setSingleShot( true );
    connect( &fadeTimer, SIGNAL( frameChanged(int) ), this, SLOT( fadeTimerFrameChanged(int) ) );
    connect( &fadeTimer, SIGNAL( finished() ), this, SLOT( fadeTimerFinished() ) );
    connect( &deleteTimer, SIGNAL( timeout() ), this, SLOT( deleteTimerFinished() ) );
}

PopupDropperPrivate::~PopupDropperPrivate()
{
}


void PopupDropperPrivate::newSceneView( PopupDropper* pud )
{
    scene->deleteLater();
    scene = new QGraphicsScene( pud );
    view = new PopupDropperView( pud, scene, widget );
}

void PopupDropperPrivate::setParent( QObject* parent )
{
    QObject::setParent( parent );
    q = static_cast<PopupDropper*>( parent );
}

void PopupDropperPrivate::fadeTimerFrameChanged( int frame ) //SLOT
{
    if( fadeTimer.state() == QTimeLine::Running )
    {
        //qDebug() << "Frame: " << frame;
        qreal val = ( frame * 1.0 ) / frameMax;
        QColor color = windowColor;
        int alpha = (int)( color.alpha() * val );
        color.setAlpha( alpha );
        q->setPalette( color, textColor );
    }
}

void PopupDropperPrivate::fadeTimerFinished() //SLOT
{
    //qDebug() << "Timer finished! (at frame " << fadeTimer.currentFrame()  <<")";
    if( fadeTimer.direction() == QTimeLine::Backward && closeAtEndOfFade )
    {
        qDebug() << "subtracting Overlay, closeAtEndOfFade = " << (closeAtEndOfFade?"true":"false");
        view->hide();
        q->subtractOverlay();
    }
    else
        q->setPalette( windowColor, textColor ); 
}

void PopupDropperPrivate::dragEntered()
{
    qDebug() << "PopupDropperPrivate::dragEntered";
}

void PopupDropperPrivate::dragLeft()
{
    qDebug() << "PopupDropperPrivate::dragLeft(), will hide PUD: " << (view->entered()?"true":"false");
    qDebug() << "PUD to be hidden or not hidden = " << q;
    if( view->entered() )
    {
        qDebug() << "View entered, hiding";
        q->hide( PopupDropper::DragLeave );
    }
}

void PopupDropperPrivate::startDeleteTimer()
{
    if( deleteTimeout == 0 )
        return;
    view->setEntered( false );
    qDebug() << "Starting delete timer";
    deleteTimer.start( deleteTimeout  );
}

void PopupDropperPrivate::deleteTimerFinished() //SLOT
{
    qDebug() << "Delete Timer Finished";
    if( !view->entered() )
        q->hide( PopupDropper::DragLeave );
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
    setColors( d->windowColor, d->textColor );
    d->sharedRenderer = new QSvgRenderer( this );
    d->overlayLevel = 1;
    //qDebug() << "Popup Dropper created!";
}

PopupDropper::~PopupDropper()
{
    clear();
    while( d->fadeTimer.state() == QTimeLine::Running )
        QApplication::processEvents();
    //qDebug() << "Popup Dropper destroyed!";
}

int PopupDropper::overlayLevel() const
{
    return d->overlayLevel;
}

void PopupDropper::initOverlay( QWidget* parent, PopupDropperPrivate* priv )
{
    PopupDropperPrivate *pdp = priv ? priv : d;
    pdp->scene->setSceneRect( QRectF( parent->rect() ) );
    pdp->scene->setItemIndexMethod( QGraphicsScene::NoIndex );
    pdp->view->resize( parent->size() );
    pdp->view->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    pdp->view->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    pdp->view->setWindowTitle( "Drop something here." );
    pdp->view->setBackgroundRole( QPalette::Window );
    pdp->view->setAutoFillBackground( true );
    pdp->success = true;
    pdp->fadeTimer.setFrameRange( 0, pdp->frameMax );
}

void PopupDropper::addOverlay()
{
    m_viewStack.push( d );
    PopupDropperPrivate* old_d = d;
    d = new PopupDropperPrivate( this, false, old_d->view );
    initOverlay( old_d->view );
    setColors( d->windowColor, d->textColor );
    d->sharedRenderer = old_d->sharedRenderer;
    d->view->setQuitOnDragLeave( true );
    d->overlayLevel = old_d->overlayLevel + 1;
}

void PopupDropper::addOverlay( PopupDropperPrivate* newD )
{
    qDebug() << "right before push, m_viewStack.size() is " << m_viewStack.size();
    m_viewStack.push( d );
    qDebug() << "right after push, m_viewStack.size() is " << m_viewStack.size();
    PopupDropperPrivate* old_d = d;
    d = newD;
    setColors( d->windowColor, d->textColor );
    d->sharedRenderer = old_d->sharedRenderer;
    d->view->setQuitOnDragLeave( true );
    d->overlayLevel = old_d->overlayLevel + 1;
    qDebug() << "new d d->overlayLevel = " << d->overlayLevel;
    qDebug() << "in PopupDropper " << this;
}

bool PopupDropper::subtractOverlay()
{
    qDebug() << "in PopupDropper " << this;
    qDebug() << "overlay d->overlayLevel = " << d->overlayLevel;
    if( d->overlayLevel == 1 )
        return false;
    PopupDropper::Fading currFadeValue = d->fade;
    d->fade = PopupDropper::NoFade;
    hide( PopupDropper::SubtractingOverlay );
    PopupDropperPrivate* old_d = d;
    qDebug() << "right before pop, m_viewStack.size() is " << m_viewStack.size();
    d = m_viewStack.pop();
    if( !old_d->submenu )
    {
        qDebug() << "not submenu, deleting";
        old_d->deleteLater();
    }
    else
    {
        qDebug() << "not deleting, submenu";
        old_d->fade = currFadeValue;
        old_d->view->resetView();
    }
    d->startDeleteTimer();
    return true;
}

PopupDropperItem* PopupDropper::addSubmenu( PopupDropper** pd, QSvgRenderer *renderer, const QString &elementId, const QString &text )
{
    qDebug() << "addSubmenu, this is " << this << " and passed-in PopupDropper is " << (*pd);
    if( !(*pd) || !renderer )
        return 0;
    PopupDropperPrivate* newD = (*pd)->d;
    newD->submenu = true;
    newD->widget = d->widget;
    newD->setParent( this );
    foreach( PopupDropperItem* item, newD->pdiItems )
        newD->scene->removeItem( item );
    newD->newSceneView( this );
    newD->doNotClear = true;
    initOverlay( d->widget, newD );
    PopupDropperItem* pdi = new PopupDropperItem();
    PopupDropperAction* pda = new PopupDropperAction( renderer, elementId, text, this );
    connect( pda, SIGNAL( hovered() ), this, SLOT( activateSubmenu() ) );
    pdi->setAction( pda );
    pdi->setSubmenuTrigger( true );
    d->submenuMap[pda] = newD;
    delete (*pd);
    (*pd) = 0;
    newD->doNotClear = false;
    qDebug() << "d->submenuMap[pda] = " << d->submenuMap[pda];
    addItem( pdi );
    return pdi;
}

void PopupDropper::activateSubmenu()
{
    qDebug() << "Sender is " << QObject::sender();
    while( d->fadeTimer.state() == QTimeLine::Running && d->fadeTimer.direction() == QTimeLine::Backward )
        QApplication::processEvents();
    if( isHidden() )
        return;
    addOverlay( d->submenuMap[static_cast<PopupDropperAction*>(QObject::sender())] );
    qDebug() << "d->pdiItems.size() = " << d->pdiItems.size() << " for " << d;
    foreach( PopupDropperItem* item, d->pdiItems )
        addItem( item, false, false );
    show();
}

bool PopupDropper::isValid() const
{
    return d->success;
}

bool PopupDropper::standalone() const
{
    return d->standalone;
}

void PopupDropper::show()
{
    if( !d->sharedRenderer )
    {
        qDebug() << "No shared renderer set!";
        return;
    }
    qDebug() << "Showing PopupDropper";
    d->fadeTimer.stop();
    d->fadeTimer.setDirection( QTimeLine::Forward );
    if( ( d->fade == PopupDropper::FadeIn || d->fade == PopupDropper::FadeInOut ) && d->fadeInTime > 0 )
    {
        qDebug() << "Animating!";
        d->fadeTimer.setDuration( d->fadeInTime );
        d->fadeTimer.setCurrentTime( 0 );
        d->fadeTimer.setCurveShape( QTimeLine::EaseOutCurve );
        QColor color = d->windowColor;
        color.setAlpha( 0 );
        setPalette( color, d->textColor );
        //qDebug() << "Timer startFrame = " << d->fadeTimer.startFrame();
        //qDebug() << "Timer endFrame = " << d->fadeTimer.endFrame();
        d->fadeTimer.start();
        //qDebug() << "Timer started";
    }
    d->view->show();
}

//returns immediately!  processEvents() while !isHidden() if you want to wait for it to finish
void PopupDropper::hide( PopupDropper::HideReason reason )
{
    if( isHidden() )
        return;

    switch( reason )
    {
        case PopupDropper::BackgroundChange:
            d->closeAtEndOfFade = false;
            break;
        case PopupDropper::DragLeave:
        case PopupDropper::SubtractingOverlay:
        default:
            d->closeAtEndOfFade = true;
            break;
    }

    if( d->fadeTimer.state() == QTimeLine::Running && d->fadeTimer.direction() == QTimeLine::Backward )
        return;

    //qDebug() << "Hiding PopupDropper";
    bool wasRunning = false;
    if( d->fadeTimer.state() == QTimeLine::Running )
    {
        d->fadeTimer.stop();
        wasRunning = true;
    }
    d->fadeTimer.setDirection( QTimeLine::Backward );
    if( ( d->fade == PopupDropper::FadeOut || d->fade == PopupDropper::FadeInOut ) && d->fadeOutTime > 0 )
    {
        d->fadeTimer.setDuration( d->fadeOutTime );
        if( !wasRunning )
            d->fadeTimer.setCurrentTime( d->fadeOutTime );
        d->fadeTimer.setCurveShape( QTimeLine::LinearCurve );
        //qDebug() << "Timer startFrame = " << d->fadeTimer.startFrame();
        //qDebug() << "Timer endFrame = " << d->fadeTimer.endFrame();
        d->fadeTimer.start();
        //qDebug() << "Timer started";
    }
    else if ( d->closeAtEndOfFade )
    {
        d->view->hide();
        if( reason != PopupDropper::SubtractingOverlay )
            subtractOverlay();
    }
}

bool PopupDropper::isHidden() const
{
    return d->view->isHidden();
}

void PopupDropper::clear()
{
    qDebug() << "Clear happening!";
    while( d->fadeTimer.state() == QTimeLine::Running )
        QApplication::processEvents();  
    d->itemCount = 0;
    do
    {
        if( !d->doNotClear )
        {
            foreach( PopupDropperItem* item, d->pdiItems )
            {
                if( item->submenuTrigger() )
                {
                    qDebug() << "Disconnecting action";
                    disconnect( item->action(), SIGNAL( hovered() ), this, SLOT( activateSubmenu() ) );
                }
                item->deleteLater();
            }
            d->pdiItems.clear();
        }
        qDebug() << "Size of pdiItems is now " << d->pdiItems.size();
        d->view->resetView();
    } while ( subtractOverlay() );
    hide( PopupDropper::DragLeave );
}

bool PopupDropper::isEmpty() const
{
    return d->pdiItems.empty();
}

bool PopupDropper::quitOnDragLeave() const
{
    return d->view->quitOnDragLeave();
}

void PopupDropper::setQuitOnDragLeave( bool quit )
{
    d->view->setQuitOnDragLeave( quit );
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

const QTimeLine* PopupDropper::fadeTimer() const
{
    return &d->fadeTimer;
}

int PopupDropper::deleteTimeout() const
{
    return d->deleteTimeout;
}

void PopupDropper::setDeleteTimeout( int msecs )
{
    d->deleteTimeout = msecs;
}

void PopupDropper::forceUpdate()
{
    d->view->update();
}

void PopupDropper::textUpdated()
{
    //qDebug() << "In textUpdated";
    foreach( PopupDropperItem *pdi, d->pdiItems )
    {
        //qDebug() << "Setting " << pdi->textItem()->toPlainText() << " to " << pdi->action()->text();
        pdi->textItem()->setPlainText( pdi->action()->text() );
    }

    forceUpdate();
}

QColor PopupDropper::windowColor() const
{
    return d->windowColor;
}

void PopupDropper::setWindowColor( const QColor &window )
{
    d->windowColor = window;
    setPalette( window, d->textColor );
}

QColor PopupDropper::textColor() const
{
    return d->textColor;
}

void PopupDropper::setTextColor( const QColor &text )
{
    d->textColor = text;
    setPalette( d->windowColor, text );
}

void PopupDropper::setColors( const QColor &window, const QColor &text )
{
    d->windowColor = window;
    d->textColor = text;
    setPalette( window, text );
}

void PopupDropper::setPalette( const QColor &window, const QColor &text )
{
    QPalette p = d->view->palette();
    p.setColor( QPalette::Window, window );
    d->view->setPalette( p );
    foreach( PopupDropperItem *item, d->pdiItems )
        item->textItem()->setDefaultTextColor( text );
    d->view->update();
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
            qDebug() << "Loaded SVG file!";
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

int PopupDropper::totalItems() const
{
    return d->totalItems;
}

void PopupDropper::setTotalItems( int items )
{
    d->totalItems = items;
}

void PopupDropper::addItem( QGraphicsSvgItem *item, bool useSharedRenderer )
{
    addItem( item, useSharedRenderer, true );
}

void PopupDropper::addItem( QGraphicsSvgItem *item, bool useSharedRenderer, bool appendToList )
{
    qDebug() << "adding item";
    //FIXME: Make separators use something graphical instead of just ignoring them
    PopupDropperItem *pItem = static_cast<PopupDropperItem*>( item );
    if( pItem->action()->isSeparator() )
        return;
    if( useSharedRenderer )
        pItem->setSharedRenderer( d->sharedRenderer );
    d->itemCount++;
    qDebug() << "Checking appendToList";
    if( appendToList )
    {
        d->pdiItems.append( pItem );
        qDebug() << "list is now size " << d->pdiItems.size() << " for " << d;
    }
    if( !pItem->textItem() )
    {
        QGraphicsTextItem *textItem = new QGraphicsTextItem( pItem->text(), pItem );
        textItem->setDefaultTextColor( d->textColor );
        pItem->setTextItem( textItem );
    }
    for( int i = 0; i < d->pdiItems.size(); i++ )
    {
        //qDebug() << "Adjusting item " << i;
        qreal partitionsize = d->scene->height() / d->pdiItems.size(); //gives partition size...now center in this area
        qreal my_min = i * partitionsize;
        qreal my_max = ( i + 1 ) * partitionsize;
        //qDebug() << "my_min = " << my_min << ", my_max = " << my_max;
        qreal vert_center = ( ( my_max - my_min ) / 2 ) + my_min; //gives us our center line...now center the item around it
        qreal item_min = vert_center - ( d->pdiItems.at( i )->boundingRect().height() / 2 );
        //qDebug() << "vert_center = " << vert_center << ", ited->min = " << item_min;
        d->pdiItems.at( i )->setPos( d->horizontalOffset, item_min );
        d->pdiItems.at( i )->reposTextItem();
    }
    d->scene->addItem( pItem );
}

#include "PopupDropper.moc"

