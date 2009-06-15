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

#ifndef AFTTAGGER_H
#define AFTTAGGER_H

#include <QCoreApplication>
#include <QStringList>
#include <QTime>
#include <QTextStream>

/**
 * @class AFTTagger
 * @short Inserts AFT tags into directories and files
 */

class AFTTagger : public QCoreApplication
{

public:
    AFTTagger( int &argc, char **argv );

    ~AFTTagger() {};

    void processPath( const QString &path );
    QString createCurrentUID( const QString &path );
    QString createV1UID( const QString &path );
    QString upgradeUID( int version, QString currValue );
    void readArgs();
    void displayHelp();

    bool          m_delete;
    bool          m_newid;
    bool          m_quiet;
    bool          m_recurse;
    bool          m_verbose;
    QStringList   m_fileFolderList;
    QTime         m_time;
    QTextStream   m_textStream;

};


#endif // AFTTAGGER_H

