/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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

#include "BookmarkManager.h"

#include "Debug.h"

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

    QHBoxLayout *layout = new QHBoxLayout();
    m_widget = new BookmarkManagerWidget( 0 );
    layout->addWidget( m_widget );
    setLayout( layout );

    resize( 600, 400 );

}

BookmarkManager::~BookmarkManager()
{
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