/****************************************************************************************
 * Copyright (c) 2008 Peter ZHOU <peterzhoulei@gmail.com>                               *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_SCRIPT_CONFIG_H
#define AMAROK_SCRIPT_CONFIG_H

#include <QObject>

class QScriptEngine;

namespace AmarokScript
{
    class AmarokScriptConfig : public QObject
    {
        Q_OBJECT

        public:
            AmarokScriptConfig( const QString& name );
            ~AmarokScriptConfig();

        public slots:
            QVariant readConfig( QString name, QVariant defaultValue ) const;
            QString readConfig( QString name, QString defaultValue ) const;

            void writeConfig( QString name, QVariant content );
            void writeConfig( QString name, QString content );

        private:
            QString m_name;
    };
}

#endif
