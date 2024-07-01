/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>                            *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/
 
#include "BrowserDock.h"

#include "core/logger/Logger.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "PaletteHandler.h"
#include "widgets/BoxWidget.h"
#include "widgets/HorizontalDivider.h"

#include <QAction>
#include <QIcon>

#include <KLocalizedString>

BrowserDock::BrowserDock( QWidget *parent )
    : AmarokDockWidget( i18n( "&Media Sources" ), parent )
{
    setObjectName( QStringLiteral("Media Sources dock") );
    setAllowedAreas( Qt::AllDockWidgetAreas );

    //we have to create this here as it is used when setting up the
    //categories (unless of course we move that to polish as well...)
    m_mainWidget = new BoxWidget( true, this );
    setWidget( m_mainWidget );
    m_mainWidget->setContentsMargins( 0, 0, 0, 0 );
    m_mainWidget->setFrameShape( QFrame::NoFrame );
    m_mainWidget->setMinimumWidth( 200 );
    m_mainWidget->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Ignored );
    m_mainWidget->setFocus( Qt::ActiveWindowFocusReason );

    m_breadcrumbWidget = new BrowserBreadcrumbWidget( m_mainWidget );
    new HorizontalDivider( m_mainWidget );
    m_categoryList = new BrowserCategoryList( QStringLiteral("root list"), m_mainWidget );
    m_breadcrumbWidget->setRootList( m_categoryList.data() );

    m_messageArea = new BrowserMessageArea( m_mainWidget );
    m_messageArea->setAutoFillBackground( true );
    //TODO: set dynamic height for hidpi displays
    m_messageArea->setFixedHeight( 36 );

    ensurePolish();
}

BrowserDock::~BrowserDock()
{}

void BrowserDock::polish()
{
    m_categoryList->setIcon( QIcon::fromTheme( QStringLiteral("user-home") ) );

    m_categoryList->setMinimumSize( 100, 300 );

    connect( m_breadcrumbWidget, &BrowserBreadcrumbWidget::toHome, this, &BrowserDock::home );

    // Keyboard shortcut for going back one level
    QAction *action = new QAction( QIcon::fromTheme( QStringLiteral("go-up") ), i18n( "Go Up in Media Sources Pane" ),
                                  m_mainWidget );
    Amarok::actionCollection()->addAction( QStringLiteral("browser_previous"), action );
    connect( action, &QAction::triggered, m_categoryList.data(), &BrowserCategoryList::back );
//    action->setShortcut( QKeySequence( Qt::Key_Backspace ) );
    action->setShortcut( Qt::Key_Backspace );

    paletteChanged( palette() );

    connect( The::paletteHandler(), &PaletteHandler::newPalette, this, &BrowserDock::paletteChanged );
}

BrowserCategoryList *BrowserDock::list() const
{
    return m_categoryList.data();
}

void
BrowserDock::navigate( const QString &target )
{
    m_categoryList->navigate( target );
}

void
BrowserDock::home()
{
    m_categoryList->home();
}

void
BrowserDock::paletteChanged( const QPalette &palette )
{
    m_messageArea->setStyleSheet(
                QString( QStringLiteral("QFrame#BrowserMessageArea { border: 1px ridge %1; " \
                         "background-color: %2; color: %3; border-radius: 3px; }" \
                         "QLabel { color: %3; }") )
                        .arg( palette.color( QPalette::Active, QPalette::Window ).name(),
                              palette.color( QPalette::Active, QPalette::Mid ).name(),
                              palette.color( QPalette::Active, QPalette::HighlightedText ).name() )
                );
}

