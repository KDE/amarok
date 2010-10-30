/***************************************************************************
 *   Plasma applet for showing video in the context view.                  *
 *                                                                         *
 *   Copyright (c) 2008 Mark Kretschmann <kretschmann@kde.org              *
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

#include "Video.h"

#include "Amarok.h"
#include "Debug.h"
#include "EngineController.h"
#include "context/ContextView.h"
#include "context/Svg.h"

#include <QPainter>


Video::Video( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
{
    DEBUG_BLOCK

    setHasConfigurationInterface( false );

    Phonon::MediaObject* mediaObject = const_cast<Phonon::MediaObject*>( The::engineController()->phononMediaObject() );
     
    QWidget* view = Context::ContextView::self()->viewport();
    m_videoWidget = new Phonon::VideoWidget();
    m_videoWidget->setParent( view, Qt::SubWindow | Qt::FramelessWindowHint );
    
    setPreferredSize( 400, 500 ); // take up all the current containment space
    
    m_videoWidget->show();

    debug() << "Creating video path.";
    Phonon::Path path = Phonon::createPath( mediaObject, m_videoWidget );

    if( !path.isValid() )
        warning() << "Phonon path is invalid.";

    constraintsEvent();
}

Video::~Video()
{
    DEBUG_BLOCK

    delete m_videoWidget;
} 

void
Video::constraintsEvent( Plasma::Constraints constraints )
{
    Q_UNUSED( constraints );
    prepareGeometryChange();
    m_videoWidget->setGeometry( QRect( pos().toPoint(),  size().toSize() ) );
}

void
Video::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{
    Q_UNUSED( p );
    Q_UNUSED( option );
    Q_UNUSED( contentsRect );
}

QSizeF
Video::effectiveSizeHint( Qt::SizeHint which, const QSizeF & constraint) const
{
    DEBUG_BLOCK
    Q_UNUSED( which )

    return constraint;
}

#include "Video.moc"

