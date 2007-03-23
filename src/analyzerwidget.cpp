/***************************************************************************
 * copyright     : (C) 2004 Mark Kretschmann <markey@web.de>               *
                   (C) 2007 Dan Meltzer <hydrogen@notyetimplemented.com>   *
 **************************************************************************/

 /***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
//
#include "amarokconfig.h"
#include "analyzerwidget.h"
#include "analyzerbase.h"

#include <klocale.h>

AnalyzerWidget::AnalyzerWidget( QWidget *parent )
    : QWidget( parent )
        , m_child( 0 )
{
    setObjectName(  "AnalyzerWidget" );
    setToolTip( i18n( "Click for more analyzers" ) );
    changeAnalyzer();
}

void
AnalyzerWidget::resizeEvent( QResizeEvent *)
{
    m_child->resize( size() );
}

void AnalyzerWidget::changeAnalyzer()
{
    delete m_child;
    m_child = Analyzer::Factory::createPlaylistAnalyzer( this );
    m_child->setObjectName( "ToolBarAnalyzer" );
    m_child->resize( size() );
    m_child->show();
}

void
AnalyzerWidget::mousePressEvent( QMouseEvent *e)
{
    if( e->button() == Qt::LeftButton ) {
        AmarokConfig::setCurrentPlaylistAnalyzer( AmarokConfig::currentPlaylistAnalyzer() + 1 );
        changeAnalyzer();
    }
}

void
AnalyzerWidget::contextMenuEvent( QContextMenuEvent *e)
{
#if defined HAVE_LIBVISUAL
    KMenu menu;
    menu.addItem( KIcon( Amarok::icon( "visualizations" ) ), i18n("&Visualizations"), Menu::ID_SHOW_VIS_SELECTOR );

    if( menu.exec( mapToGlobal( e->pos() ) ) == Menu::ID_SHOW_VIS_SELECTOR )
        Menu::instance()->slotActivated( Menu::ID_SHOW_VIS_SELECTOR );
#else
    Q_UNUSED(e);
#endif
}

#include "analyzerwidget.moc"
