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

#include "ColumnContainment.h"

#include "ContextScene.h"
#include "Debug.h"
#include "MainWindow.h"
#include "Svg.h"
#include "SvgTinter.h"
#include "TheInstances.h"
#include "WidgetBackgroundPainter.h"

#include "plasma/theme.h"

#include <KAuthorized>
#include <KMenu>
#include <KTemporaryFile>
#include <KSvgRenderer>
#include <KGlobalSettings>
#include <KIcon>

#include <threadweaver/ThreadWeaver.h>
#include <threadweaver/Job.h>

#include <QAction>
#include <QApplication>
#include <QDesktopWidget>
#include <QGraphicsScene>
#include <QThread>
#include <QTimeLine>



namespace Context
{

void SvgRenderJob::run()
{
    KSvgRenderer renderer( m_file );
    m_image = QImage(  QApplication::desktop()->screenGeometry( QApplication::desktop()->primaryScreen() ).size(), QImage::Format_ARGB32 );
    QPainter painter ( &m_image );
    renderer.render( &painter );
}

ColumnContainment::ColumnContainment( QObject *parent, const QVariantList &args )
    : Context::Containment( parent, args )
    , m_actions( 0 )
    , m_defaultColumnSize( 350 )
    , m_appletBrowserHasBeenKicked ( false )
{
    DEBUG_BLOCK

    setContainmentType( CustomContainment );

    DEBUG_LINE_INFO
    m_columns = new ContextLayout( this );
    m_columns->setColumnWidth( m_defaultColumnSize );
    DEBUG_LINE_INFO
//     m_columns->setSpacing( 3 );
//     m_columns->setContentsMargins(0,0,0,0);
    DEBUG_LINE_INFO

    //HACK alert!

    //m_job = new SvgRenderJob( file.fileName() );
    //the background is not really important for the use of Amarok,
    //and running this in a thread makes startup quite a bit faster
    //consider only starting the thread if QThread::idealThreadCount > 1
    //if the background *has* to be there in time most of the time
    //connect( m_job, SIGNAL( done( ThreadWeaver::Job* ) ), SLOT( jobDone() ) );
    //ThreadWeaver::Weaver::instance()->enqueue( m_job );

    DEBUG_LINE_INFO
    //m_background = new Svg( m_tintedSvg.fileName(), this );
    /*m_logo = new Svg( "widgets/amarok-logo", this );
    m_width = 300; // TODO hardcoding for now, do we want this configurable?
    m_aspectRatio = (qreal)m_logo->size().height() / (qreal)m_logo->size().width();
    m_logo->resize( (int)m_width, (int)( m_width * m_aspectRatio ) );*/
    DEBUG_LINE_INFO

    connect( this, SIGNAL( appletAdded( Plasma::Applet*, const QPointF & ) ), this, SLOT( addApplet( Plasma::Applet*, const QPointF & ) ) );

    DEBUG_LINE_INFO

    m_appletBrowserAction = new QAction(KIcon("list-add-amarok"), i18n("Add applet"), this);
    connect(m_appletBrowserAction, SIGNAL(triggered(bool)), this, SLOT(launchAppletBrowser()));
    // set up default context menu actions
    m_actions = new QList<QAction*>();
    m_actions->append( m_appletBrowserAction );

    m_appletBrowser = new Plasma::AppletBrowser();
    m_appletBrowser->setApplication( "amarok" );
    m_appletBrowser->setContainment( this );
    m_appletBrowser->hide();
    DEBUG_LINE_INFO

    debug() << "Creating ColumnContainment, max size:" << maximumSize();

connect( this, SIGNAL( appletRemoved( Plasma::Applet* ) ), this, SLOT( appletRemoved( Plasma::Applet* ) ) );
    //connect ( Plasma::Theme::self(), SIGNAL( changed() ), this, SLOT( paletteChange() ) );
    connect( KGlobalSettings::self(), SIGNAL(kdisplayPaletteChanged() ), this, SLOT( paletteChange() ) );
}

void ColumnContainment::saveToConfig( KConfig& conf )
{
//     debug() << "number of m_columns:" << m_columns->count();
    for( int i = 0; i < m_columns->count(); i++ )
    {
        Applet *applet = 0;
        //FIXME: Reenable when working, the dynamic_cast causes a crash.
        QGraphicsLayoutItem *item = m_columns->itemAt( i );
        //Q_ASSERT(item);
        applet = dynamic_cast<Plasma::Applet*>( item );
        debug() << "trying to save an applet";
        if( applet != 0 )
        {
            KConfigGroup cg( &conf, QString::number( applet->id() ) );
            debug() << "saving applet" << applet->name();
            cg.writeEntry( "plugin", applet->pluginName() );
        }
    }
    conf.sync();
}

void ColumnContainment::loadConfig( KConfig& conf )
{
    foreach( const QString& group, conf.groupList() )
    {
        KConfigGroup cg( &conf, group );
        QString plugin = cg.readEntry( "plugin", QString() );
//         debug() << "loading applet:" << plugin
//             << QStringList() << group.toUInt();
        if( plugin != QString() )
            Plasma::Containment::addApplet( plugin );
    }
    recalculate();
}

QSizeF ColumnContainment::sizeHint( Qt::SizeHint which, const QSizeF &constraint ) const
{
    return geometry().size();
}

QRectF ColumnContainment::boundingRect() const
{
    return geometry();
}
// call this when the view changes size: e.g. layout needs to be recalculated
void ColumnContainment::updateSize() // SLOT
{
    // HACK HACK HACK i don't know where maximumSize is being set, but SOMETHING is setting it,
    // and is preventing the containment from expanding when it should.
    // so, we manually keep the size high.
    setMaximumSize( QSizeF( 100000, 100000 ) );
    m_columns->setGeometry( scene()->sceneRect() );
    setGeometry( scene()->sceneRect() );
    //debug() << "ColumnContainment updating size to:" << geometry() << "sceneRect is:" << scene()->sceneRect() << "max size is:" << maximumSize();
}

void ColumnContainment::paintInterface(QPainter *painter, const QStyleOptionGraphicsItem *option, const QRect& rect )
{
    Q_UNUSED( option );
    painter->save();

    /*int height = rect.height(); //?
    int width = rect.width();

    int offsetX = MainWindow::self()->contextRectGlobal().x();
    int offsetY = MainWindow::self()->contextRectGlobal().topLeft().y();

    //debug() << "offset: " << offsetX << " x " << offsetY;

    painter->drawPixmap( 0, 0, WidgetBackgroundPainter::instance()->getBackground( "Context", offsetX, offsetY, 0, 0, width, height ) );
*/

    QRectF bounds = The::svgHandler()->getRenderer()->boundsOnElement ( "amarok_logo" );
    double aspectRatio = bounds.width() / bounds.height();

    int logoWidth = 300;
    int logoHeight = ( int )( ( double ) logoWidth / aspectRatio );

    painter->drawPixmap(rect.width() - ( logoWidth + 20 ) , rect.height() - ( logoHeight + 20 ) , The::svgHandler()->renderSvg( "amarok_logo", logoWidth, logoHeight, "amarok_logo" ) );

    painter->restore();

    /*QSize size = m_logo->size();
    QSize pos = rect.size() - size;
    qreal newHeight  = m_aspectRatio * m_width;
    m_logo->resize( QSize( (int)m_width, (int)newHeight ) );
    painter->save();
    m_logo->paint( painter, QRectF( pos.width() - 10.0, pos.height() - 5.0, size.width(), size.height() ) );
    painter->restore();*/
}

Plasma::Applet* ColumnContainment::addApplet( Applet* applet, const QPointF & )
{
//     debug() << "m_columns:" << m_columns;
    m_columns->addItem( applet );

    recalculate();
    return applet;
}

void ColumnContainment::recalculate()
{
    DEBUG_BLOCK
    debug() << "got child item that wants a recalculation";
    m_columns->invalidate();
//    m_columns->relayout();
}

QList<QAction*> ColumnContainment::contextualActions()
{
    return *m_actions;
}

void ColumnContainment::launchAppletBrowser() // SLOT
{
    m_appletBrowser->show();
    if ( !m_appletBrowserHasBeenKicked ) {
        m_appletBrowser->resize( m_appletBrowser->size() + QSize( 1 , 0 ) );
        m_appletBrowserHasBeenKicked = true;
    }
}

/*
bool ColumnContainment::sceneEventFilter(QGraphicsItem *watched, QEvent *event)
{
    Applet *applet = qgraphicsitem_cast<Applet*>(watched);
    //QEvent::GraphicsSceneHoverEnter

    // Otherwise we're watching something we shouldn't be...
    Q_ASSERT(applet!=0);

    return false;
}*/

void ColumnContainment::jobDone()
{
    m_masterImage = m_job->m_image;
    m_job->deleteLater();
    m_job = 0;
    //we should request a redraw of  applet now, but I have now idea how to do that
}


void ColumnContainment::paletteChange()
{
    The::svgHandler()->reTint();
    update( boundingRect() );
}

void
ColumnContainment::appletRemoved( Plasma::Applet* applet )
{
    DEBUG_BLOCK
    QGraphicsLayoutItem* item = dynamic_cast< QGraphicsLayoutItem* >( applet );
    if( item )
        m_columns->removeItem( item );
    else
        debug() << "GOT NON-QGraphicsLayoutItem in APPLETREMOVED";

}


} // Context namespace



#include "ColumnContainment.moc"
