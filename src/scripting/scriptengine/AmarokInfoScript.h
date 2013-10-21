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

class QScriptEngine;

namespace AmarokScript
{
    class AmarokScriptEngine;

    // SCRIPTDOX Amarok.Info
    class InfoScript : public QObject
    {
        Q_OBJECT
        Q_ENUMS( IconSizes )

        public:
            // SCRIPTDOX enum Amarok.Info.IconSizes
            enum IconSizes {
                Small=16,
                SmallMedium=22,
                Medium=32,
                Large=48,
                Huge=64,
                Enormous=128
            };

            InfoScript( const KUrl& scriptUrl, AmarokScriptEngine *engine );

        public slots:

            /**
             * The directory where the script's main.js file is located
             */
            QString scriptPath() const;

            /**
             * Return the location of the specified config
             */
            QString scriptConfigPath( const QString& name ) const;

            /**
             * Return the path to the standard icon with the given name.
             * Icons will be searched in current icon theme and all its base themes.
             * @param name The name of the icon, without extension. If an absolute
             * path is supplied for this parameter, iconPath will return it
             * directly.
             * @param size Search icons whose size is @param size.
             * See Info.IconSizes
             */
            QString iconPath( const QString& name, int size ) const;

            /**
             * The current Amarok version.
             */
            QString version() const;

        private:
            const KUrl m_scriptUrl;
    };

} // namespace AmarokScript

#endif
