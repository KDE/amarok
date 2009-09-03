/****************************************************************************************
 * Copyright (c) 2008 Ian Monroe <ian@monroe.nu>                                        *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_INFO_SCRIPT_H
#define AMAROK_INFO_SCRIPT_H

#include <QObject>
#include <KUrl>

class InfoScript : public QObject
{
    Q_OBJECT

    public:
        InfoScript( const KUrl& scriptUrl );
    public slots:
        QString scriptPath() const;
        QString scriptConfigPath( const QString& name ) const;
        QString iconPath( const QString& name, int size ) const;
        QString version() const;

    private:
        const KUrl m_scriptUrl;
};

class IconEnum : public QObject
{
    Q_OBJECT
    Q_ENUMS( StdSizes )
    public:
        IconEnum() : QObject() { }
      enum StdSizes {
          Small=16,
          SmallMedium=22,
          Medium=32,
          Large=48,
          Huge=64,
          Enormous=128
      };
};

#endif
