/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#include "AmarokDockWidget.h"
#include <QHBoxLayout>

AmarokDockWidget::AmarokDockWidget( const QString & title, QWidget * parent, Qt::WindowFlags flags )
    : QDockWidget( title, parent, flags )
    , m_polished( false )
{
    m_dummyTitleBarWidget = new QWidget( this );
    m_dummyTitleBarWidget->setLayout( new QHBoxLayout ); // HACK avoid warnings in console output, QTBUG-42986
    connect( this, &AmarokDockWidget::visibilityChanged, this, &AmarokDockWidget::slotVisibilityChanged );
}

void AmarokDockWidget::slotVisibilityChanged( bool visible )
{
    if( visible )
        ensurePolish();
}

void AmarokDockWidget::ensurePolish()
{
    if( !m_polished )
    {
        polish();
        m_polished = true;
    }
}

void AmarokDockWidget::setMovable( bool movable )
{
    if( movable )
    {
        const QFlags<QDockWidget::DockWidgetFeature> features = QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetClosable;
        setTitleBarWidget( nullptr );
        setFeatures( features );
    }
    else
    {
        const QFlags<QDockWidget::DockWidgetFeature> features = QDockWidget::NoDockWidgetFeatures;
        setTitleBarWidget( m_dummyTitleBarWidget );
        setFeatures( features );
    }
}

