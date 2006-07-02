/*
  Copyright (c) 2005 Gábor Lehel <illissius@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

/**
    While having two equivalent URLs is usually rare, parts of many URLs (the directories, mostly),
    are often equivalent. AtomicURL uses AtomicStrings internally for the separate parts to try and
    reduce memory usage for large numbers of URLs.
**/

#ifndef AMAROK_ATOMICURL_H
#define AMAROK_ATOMICURL_H

#include <qstring.h>
#include "atomicstring.h"
#include "amarok_export.h"

class KURL;

class LIBAMAROK_EXPORT AtomicURL
{
    AtomicString m_beginning;
    AtomicString m_directory;
    QString m_filename;
    QString m_end;

public:
    AtomicURL();

    AtomicURL( const AtomicURL &other );

    AtomicURL( const KURL &url );

    virtual ~AtomicURL();

    AtomicURL &operator=( const AtomicURL &other );

    bool operator==( const AtomicURL &other ) const;

    QString string() const;

    KURL url() const;

    operator KURL() const { return url(); }

    bool isEmpty() const;

    void setPath( const QString &path );

    QString path() const;

    QString fileName() const;

    QString directory() const;
};

#endif
