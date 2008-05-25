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
#include "TheInstances.h"
#include "context/ContextView.h"
#include "context/Svg.h"

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPainter>


Video::Video( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
    , m_config( 0 )
    , m_configLayout( 0 )
    , m_width( 0 )
    , m_aspectRatio( 0 )
    , m_size( QSizeF() )
    , m_initialized( false )
{
    DEBUG_BLOCK

    setHasConfigurationInterface( false );

    m_theme = new Context::Svg( this );
    m_theme->setImagePath( "widgets/amarok-video" );
    m_theme->setContainsMultipleImages( false );
    m_theme->resize( m_size );
    m_width = globalConfig().readEntry( "width", 500 );

    // get natural aspect ratio, so we can keep it on resize
    m_theme->resize();
    m_aspectRatio = (qreal)m_theme->size().height() / (qreal)m_theme->size().width();
    resize( m_width, m_aspectRatio );
    setPreferredSize( m_width, 400 );

    Phonon::MediaObject* mediaObject = const_cast<Phonon::MediaObject*>( The::engineController()->phononMediaObject() );
     
    QWidget* view = Context::ContextView::self()->viewport();

    m_videoWidget = new Phonon::VideoWidget();
    m_videoWidget->setParent( view, Qt::SubWindow | Qt::FramelessWindowHint );
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
EngineNewTrackPlaying()
{
    DEBUG_BLOCK
}

void
Video::constraintsEvent( Plasma::Constraints constraints )
{
    DEBUG_BLOCK

    m_videoWidget->setGeometry( geometry().toRect() );

    prepareGeometryChange();

    if (constraints & Plasma::SizeConstraint && m_theme) {
        m_theme->resize(size().toSize());
    }

    m_initialized = true;
}

void
Video::paintInterface( QPainter *p, const QStyleOptionGraphicsItem *option, const QRect &contentsRect )
{
    Q_UNUSED( option );

    p->save();
    m_theme->paint( p, contentsRect/*, "background" */);
    p->restore();
}

void
Video::showConfigurationInterface()
{}

void
Video::configAccepted() // SLOT
{}

void
Video::resize( qreal newWidth, qreal aspectRatio )
{
    Q_UNUSED( newWidth ); Q_UNUSED( aspectRatio );
}

QSizeF
Video::effectiveSizeHint( Qt::SizeHint which, const QSizeF & constraint) const
{
    DEBUG_BLOCK
    Q_UNUSED( which )

    if( constraint.height() == -1 && constraint.width() > 0 ) // asking height for given width basically
    {
        return QSizeF( m_aspectRatio * constraint.width(), constraint.width() );
    }

    return constraint;
}

bool
Video::hasHeightForWidth() const
{
    return true;
}

qreal
Video::heightForWidth(qreal width) const
{
    return width * m_aspectRatio;
}


#include "Video.moc"
