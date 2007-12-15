/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef CACHEDHTTPJANITOR_H
#define CACHEDHTTPJANITOR_H

#include "UnicornDllExportMacro.h"

#include <QThread>

class UNICORN_DLLEXPORT CachedHttpJanitor : public QThread
{
    Q_OBJECT

    public:
        CachedHttpJanitor( const QString& cacheDir, QObject* parent = 0 );
        ~CachedHttpJanitor();

        void abort() { m_abort = true; }

    private:
        virtual void run();

        QString m_cacheDir;
        bool m_abort;
};

#endif // CACHEDHTTPJANITOR_H
