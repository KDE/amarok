/*****************************************************************************
 * copyright            : (C) 2008 Alejandro Wainzinger <aikawarauzni@gmail.com>          *
*
 *****************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "MediaDevicesApplet.h"

#include <QGraphicsLinearLayout>
#include <QPainter>
#include <QFontMetrics>
#include <QSizeF>
 
#include <plasma/svg.h>
#include <plasma/theme.h>

#include <KStandardDirs>
 
MediaDevicesApplet::MediaDevicesApplet(QObject *parent, const QVariantList &args)
    : Context::Applet(parent, args),
    m_icon(0),
    m_connect(0),
    m_disconnect(0)
{
    //m_svg.setImagePath("amarok/images/pud_items.svg");
    // this will get us the standard applet background, for free!
    setBackgroundHints(DefaultBackground);

    // get path where we pull svg's from

    QString svgPath = KStandardDirs::locate( "data", "amarok/images/pud_items.svg" );

    // iPod (device) icon setup
    
    m_icon = new Plasma::Icon( this );
    m_icon->setSvg( svgPath, "device" );

    // Connect Icon

    m_connect = new Plasma::Icon( this );
    m_connect->setSvg( svgPath, "append" );

    // Disconnect Icon

    m_disconnect = new Plasma::Icon( this );
    m_disconnect->setSvg( svgPath, "delete" );

}
 
 
MediaDevicesApplet::~MediaDevicesApplet()
{
    if (hasFailedToLaunch()) {
        // Do some cleanup here
    } else {
        // Save settings
    }
}
 
void MediaDevicesApplet::init()
{
    // layout and ui

    // set icons sizes

    QSizeF iconSize = m_icon->sizeFromIconSize(32);
    
    m_icon->setMinimumSize( iconSize );
    m_icon->setMaximumSize( iconSize );

    m_connect->setMinimumSize( iconSize );
    m_connect->setMaximumSize( iconSize );

    m_disconnect->setMinimumSize( iconSize );
    m_disconnect->setMaximumSize( iconSize );

    // set layout

    m_layout = new QGraphicsLinearLayout( Qt::Horizontal );
    setLayout( m_layout );
    m_layout->setSpacing( 0 );

    // add items to layout

    m_layout->addItem( m_icon );
    m_layout->addItem( m_connect );
    m_layout->addItem( m_disconnect );
} 
 
 
void MediaDevicesApplet::paintInterface(QPainter *p,
        const QStyleOptionGraphicsItem *option, const QRect &contentsRect)
{
    p->setRenderHint(QPainter::SmoothPixmapTransform);
    p->setRenderHint(QPainter::Antialiasing);
 
    // Now we draw the applet, starting with our svg

    // We place the icon and text
//    p->drawPixmap(7, 0, m_icon.pixmap((int)contentsRect.width(),(int)contentsRect.width()-14));
    /*
    p->save();
    p->setPen(Qt::white);
    p->drawText(contentsRect,
                Qt::AlignBottom | Qt::AlignHCenter,
                "Hello Plasmoid!");
    
    p->restore();
    */




}

QSizeF 
MediaDevicesApplet::sizeHint( Qt::SizeHint which, const QSizeF & constraint) const
{
    Q_UNUSED( which )

            if( constraint.height() == -1 && constraint.width() > 0 ) // asking height for given width basically
            return QSizeF( constraint.width(), 150 );
//         return QSizeF( constraint.width(), m_aspectRatio * constraint.width() );

    return constraint;
}
 
#include "MediaDevicesApplet.moc"
