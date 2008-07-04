/******************************************************************************
 * Copyright (C) 2008 Peter ZHOU <peterzhoulei@gmail.com>                     *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License as             *
 * published by the Free Software Foundation; either version 2 of             *
 * the License, or (at your option) any later version.                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
 ******************************************************************************/

#ifndef AMAROK_STATUSBAR_SCRIPT_H
#define AMAROK_STATUSBAR_SCRIPT_H

#include "statusbar/StatusBar.h"

#include <QObject>
#include <QtScript>

namespace Amarok
{

    class AmarokStatusbarScript : public QObject
    {
        Q_OBJECT

        public:
            AmarokStatusbarScript( QScriptEngine* ScriptEngine );
            ~AmarokStatusbarScript();

        public slots:
            void setMainText( const QString &text );
            void setMainTextIcon( QPixmap icon );
            void hideMainTextIcon();
            void resetMainText();
            void longMessage( const QString &text, Amarok::StatusBar::MessageType type = Amarok::StatusBar::Information );
            void longMessageThreadSafe( const QString &text );
            void shortLongMessage( const QString &_short, const QString &_long, Amarok::StatusBar::MessageType type = Amarok::StatusBar::Information );
            void shortMessage( const QString &text, bool longShort = false );
//            void shortMessageThreadSafe( const QString &text ); //what's this?
//            void endProgressOperation();
//            void endProgressOperation( QObject *owner );
//            void setProgress( int steps );
//            void setProgress( const QObject *owner, int steps );
//            void incrementProgress();
//            void toggleProgressWindow( bool show ); //what's this?
//            void abortAllProgressOperations();
        private:

    };
}

#endif
