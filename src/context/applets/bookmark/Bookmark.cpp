/*****************************************************************************
 * copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>          *
 *                      : (C) 2008 William Viana Soares <vianasw@gmail.com>  *
 *                  : (C) 2008 Nikolaj Hald Nielsen <nhnFreespiri@gmail.com> *
 *****************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "Bookmark.h"

#include "Amarok.h"
#include "AmarokUrl.h"
#include "NavigationUrlGenerator.h"
#include "Debug.h"
#include "EngineController.h"
#include "context/popupdropper/libpud/PopupDropperAction.h"
#include "meta/MetaUtility.h"
#include "PaletteHandler.h"
#include "SvgHandler.h"

#include <plasma/theme.h>

#include <KApplication>
#include <KColorScheme>
#include <KIcon>
#include <KMessageBox>

#include <QCheckBox>
#include <QFont>
#include <QLabel>
#include <QPainter>
#include <QSpinBox>
#include <QVBoxLayout>


Bookmark::Bookmark( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
{
    setHasConfigurationInterface( false );
}

Bookmark::~Bookmark()
{
    delete m_bookmarkWidget;
    m_bookmarkWidget = 0;
}

void Bookmark::init()
{
    DEBUG_BLOCK
    QFont labelFont;
    labelFont.setPointSize( labelFont.pointSize() + 1  );
    QBrush brush = KColorScheme( QPalette::Active ).foreground( KColorScheme::NormalText );

    m_bookmarkWidget = new BookmarkManagerWidgetProxy( this );
    
    connect( The::paletteHandler(), SIGNAL( newPalette( const QPalette& ) ), SLOT(  paletteChanged( const QPalette &  ) ) );
}

void Bookmark::constraintsEvent( Plasma::Constraints constraints )
{
    Q_UNUSED( constraints )
    DEBUG_BLOCK

    prepareGeometryChange();

    /*if( constraints & Plasma::SizeConstraint )
         m_theme->resize(size().toSize());*/

    m_bookmarkWidget->setGeometry( QRectF( 10, 10, size().toSize().width() - 20 , size().toSize().height() - 20 ) );
}

QSizeF 
Bookmark::sizeHint( Qt::SizeHint which, const QSizeF & constraint) const
{
    Q_UNUSED( which )

    if( constraint.height() == -1 && constraint.width() > 0 ) // asking height for given width basically
        return QSizeF( constraint.width(), 350 );

    return QGraphicsWidget::sizeHint( which, constraint );
}

void Bookmark::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{
    Q_UNUSED( option );

    //bail out if there is no room to paint. Prevents crashes and really there is no sense in painting if the
    //context view has been minimized completely
    if( ( contentsRect.width() < 20 ) || ( contentsRect.height() < 20 ) )
    {
        foreach ( QGraphicsItem * childItem, QGraphicsItem::children() )
            childItem->hide();
        return;
    }
    else
    {
        foreach ( QGraphicsItem * childItem, QGraphicsItem::children () )
            childItem->show();
    }
}


void Bookmark::paletteChanged( const QPalette & palette )
{
    Q_UNUSED( palette )
}

#include "Bookmark.moc"


