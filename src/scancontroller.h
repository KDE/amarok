/***************************************************************************
 *   Copyright (C) 2003-2005 by The amaroK Developers                      *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#ifndef AMAROK_SCANCONTROLLER_H
#define AMAROK_SCANCONTROLLER_H

#include <qobject.h>
#include <qxml.h>

class DbConnection;
class KProcIO;

/**
 * @author Mark Kretschmann <markey@web.de>
 */
class ScanController : public QObject, public QXmlDefaultHandler
{
    Q_OBJECT

    public:
        ScanController( QObject* parent, QStringList folders );
        ~ScanController();

    private slots:
        void slotReadReady();
        void slotProcessExited();

    private:
        bool startElement( const QString&, const QString &localName, const QString&, const QXmlAttributes &attrs );


        DbConnection* const m_db;
        KProcIO* m_scanner;
        QXmlInputSource m_source;
        QXmlSimpleReader m_reader;
};



#endif // AMAROK_SCANCONTROLLER_H

