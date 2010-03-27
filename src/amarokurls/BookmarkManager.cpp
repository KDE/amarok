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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "BookmarkManager.h"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"

#include <KApplication>
#include <KDialog>

#include <QHBoxLayout>

BookmarkManager * BookmarkManager::s_instance = 0;


BookmarkManager * BookmarkManager::instance()
{
    if( s_instance == 0 )
        s_instance = new BookmarkManager();

    return s_instance;
}

BookmarkManager::BookmarkManager()
    : QDialog()
{
    // Sets caption and icon correctly (needed e.g. for GNOME)
    kapp->setTopWidget( this );
    setWindowTitle( KDialog::makeStandardCaption( i18n("Bookmark Manager") ) );
    setAttribute( Qt::WA_DeleteOnClose );
    setObjectName( "BookmarkManager" );

    QHBoxLayout *layout = new QHBoxLayout( this );
    m_widget = new BookmarkManagerWidget( this );
    layout->addWidget( m_widget );
    layout->setContentsMargins( 0, 0, 0, 0 );
    setLayout( layout );

    const QSize winSize = Amarok::config( "Bookmark Manager" ).readEntry( "Window Size", QSize( 600, 400 ) );
    resize( winSize );
}

BookmarkManager::~BookmarkManager()
{
    Amarok::config( "Bookmark Manager" ).writeEntry( "Window Size", size() );
    s_instance = 0;
}

void BookmarkManager::showOnce()
{
    DEBUG_BLOCK
    instance()->activateWindow();
    instance()->show();
    instance()->raise();
}

namespace The {

    BookmarkManager* bookmarkManager()
    {
        return BookmarkManager::instance();
    }
}
