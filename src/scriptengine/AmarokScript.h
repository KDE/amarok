/****************************************************************************************
 * Copyright (c) 2008 Peter ZHOU <peterzhoulei@gmail.com>                               *
 * Copyright (c) 2008 Mark Kretschmann <kretschmann@kde.org>                            *
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

#ifndef AMAROK_SCRIPT_H
#define AMAROK_SCRIPT_H

#include <QObject>

class QScriptEngine;

namespace AmarokScript
{
    class AmarokScript : public QObject
    {
        Q_OBJECT

        public:
            AmarokScript( const QString& name );
            ~AmarokScript();

            void slotConfigured();

        public slots:
            /** Shuts down Amarok completely. */
            void        quitAmarok();

            /** 
             * Print debug output to the shell. Only printed if amarok is started with --debug.
             * @text The text to print.
             */
            void        debug( const QString& text ) const;

            /**
              * Show an information dialog in Amarok.
              * @text The text to display.
              * @type Type of the dialog. See KMessageBox docs.
              */
            int         alert( const QString& text, const QString& type = "information" ) const;

            /** Signals Amarok that this script has ended. */
            void        end();

            /**
              * Start another Amarok script.
              * @name Name of the script to start.
              */
            bool        runScript( const QString& name ) const;

            /**
              * Stop another Amarok script.
              * @name Name of the script to stop.
              */
            bool        stopScript( const QString& name ) const;

            QStringList listRunningScripts() const;

        //TODO: actaully use this signal
        signals:
            void configured();

        private:
            QString m_name;
    };
}

#endif
