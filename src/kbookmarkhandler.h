/* This file is part of the KDE project
   Copyright (C) xxxx KFile Authors
   Copyright (C) 2002 Anders Lund <anders.lund@lund.tdcadsl.dk>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef _KBOOKMARKHANDLER_H_
#define _KBOOKMARKHANDLER_H_

#include <kbookmarkmanager.h>
#include <kbookmarkmenu.h>

class QTextStream;
class KPopupMenu;

class KBookmarkHandler : public QObject, public KBookmarkOwner
{
    Q_OBJECT

public:
    KBookmarkHandler( QObject *parent, KPopupMenu* );

    // KBookmarkOwner interface:
    virtual void openBookmarkURL( const QString &url ) { emit openUrl( KURL(url) ); }

signals:
    void openUrl( const KURL &url );

private slots:
    void slotNewBookmark( const QString& text, const QCString& url, const QString& additionalInfo );
    void slotNewFolder( const QString& text, bool open, const QString& additionalInfo );
    void newSeparator();
    void endFolder();

private:
    KPopupMenu    *m_menu;
    KBookmarkMenu *m_bookmarkMenu;
    QTextStream   *m_importStream;
};

#endif // _KBOOKMARKHANDLER_H_
