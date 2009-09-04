/***************************************************************************
 *   Copyright (C) 2008-2009 Jeff Mitchell <mitchell@kde.org>              *
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

#ifndef AFTUTILITY_H
#define AFTUTILITY_H

#include <QCryptographicHash>
#include <QString>

#include <fileref.h>
#include <tstring.h>

class AFTUtility
{
    public:
        AFTUtility();
        ~AFTUtility() {};
        const QString readEmbeddedUniqueId( const TagLib::FileRef &fileref );
        const TagLib::ByteVector generatedUniqueIdHelper( const TagLib::FileRef &fileref );
        const QString randomUniqueId( QCryptographicHash &md5 );
        const QString readUniqueId( const QString &path );
};
#endif
