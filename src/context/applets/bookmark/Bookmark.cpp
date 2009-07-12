/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@gmail.com>                                  *
 * Copyright (c) 2008 William Viana Soares <vianasw@gmail.com>                          *
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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

    // Properly set the height (width as no importance.)
    resize( 500, 350 );
    
//    QFont labelFont;
//    labelFont.setPointSize( labelFont.pointSize() + 2  );
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

    m_bookmarkWidget->setGeometry( QRectF( standardPadding(), standardPadding(), size().toSize().width() - 2 * standardPadding() , size().toSize().height() - 2 * standardPadding() ) );
}

void Bookmark::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{
    Q_UNUSED( option ); Q_UNUSED(p);

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

    p->setRenderHint( QPainter::Antialiasing );

    // tint the whole applet
    addGradientToAppletBackground( p );
}


void Bookmark::paletteChanged( const QPalette & palette )
{
    Q_UNUSED( palette )
}

#include "Bookmark.moc"


