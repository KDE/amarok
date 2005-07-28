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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _KBOOKMARKHANDLER_H_
#define _KBOOKMARKHANDLER_H_

#include <kbookmarkmanager.h>
#include <qobject.h>

class KDirOperator;
class KPopupMenu;

class KBookmarkHandler : public QObject, public KBookmarkOwner
{
public:
    KBookmarkHandler( KDirOperator *parent, KPopupMenu* );

    /// KBookmarkOwner interface:
    virtual void openBookmarkURL( const QString &url );
    virtual QString currentURL() const;
};

#endif // _KBOOKMARKHANDLER_H_
