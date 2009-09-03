/****************************************************************************************
 * Copyright (c) 2002 KFile Authors <kde@kde.org>                                       *
 * Copyright (c) 2002 Anders Lund <anders.lund@lund.tdcadsl.dk>                         *
 * Copyright (c) 2007 Mirko Stocker <me@misto.ch>                                       *
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

#include "kbookmarkhandler.h"

#include <stdio.h>
#include <stdlib.h>

#include <QByteArray>

#include <kbookmarkimporter.h>
#include <kmenu.h>
#include <ksavefile.h>
#include <kstandarddirs.h>
#include <kdiroperator.h>
#include <kaction.h>
#include <kdebug.h>

#include "FileBrowser.h"
#include "MyDirOperator.h"


KBookmarkHandler::KBookmarkHandler( FileBrowser::Widget *parent, KMenu* kpopupmenu )
    : QObject( parent ),
    KBookmarkOwner(),
    mParent( parent ),
    m_menu( kpopupmenu ),
    m_bookmarkMenu( 0 ),
    m_importStream( 0L )
{
  setObjectName( "KBookmarkHandler" );
  if (!m_menu)
    m_menu = new KMenu( parent);

  QString file = KStandardDirs::locate( "data", "amarok/fsbookmarks.xml" );
  if ( file.isEmpty() )
    file = KStandardDirs::locateLocal( "data", "amarok/fsbookmarks.xml" );

  KBookmarkManager *manager = KBookmarkManager::managerForFile( file, "amarok" );
  manager->setUpdate( true );

  m_bookmarkMenu = new KBookmarkMenu( manager, this, m_menu, parent->actionCollection() );
}

KBookmarkHandler::~KBookmarkHandler()
{
  delete m_bookmarkMenu;
}

QString KBookmarkHandler::currentUrl() const
{
  return mParent->dirOperator()->url().url();
}

QString KBookmarkHandler::currentTitle() const
{
  return currentUrl();
}

void KBookmarkHandler::openBookmark( const KBookmark & bm, Qt::MouseButtons, Qt::KeyboardModifiers )
{
  emit openUrl(bm.url());
}

void KBookmarkHandler::slotNewBookmark( const QString& text,
                                        const QByteArray& url,
                                        const QString& additionalInfo )
{
  Q_UNUSED( text )

  *m_importStream << "<bookmark icon=\"" << KMimeType::iconNameForUrl( KUrl( url ) );
  *m_importStream << "\" href=\"" << QString::fromUtf8(url) << "\">\n";
  *m_importStream << "<title>" << (additionalInfo.isEmpty() ? QString::fromUtf8(url) : additionalInfo) << "</title>\n</bookmark>\n";
}

void KBookmarkHandler::slotNewFolder( const QString& text, bool /*open*/,
                                      const QString& /*additionalInfo*/ )
{
  *m_importStream << "<folder icon=\"bookmark_folder\">\n<title=\"";
  *m_importStream << text << "\">\n";
}

void KBookmarkHandler::newSeparator()
{
  *m_importStream << "<separator/>\n";
}

void KBookmarkHandler::endFolder()
{
  *m_importStream << "</folder>\n";
}

#include "kbookmarkhandler.moc"
// kate: space-indent on; indent-width 2; replace-tabs on;

