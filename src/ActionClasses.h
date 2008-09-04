/***************************************************************************
 *   Copyright (C) 2004 by Max Howell <max.howell@methylblue.com>          *
 *   Copyright (C) 2008 by Mark Kretschmann <kretschmann@kde.org>          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_ACTIONCLASSES_H
#define AMAROK_ACTIONCLASSES_H

#include "EngineObserver.h"
#include "widgets/SliderWidget.h"

#include <KAction>
#include <KMenu>
#include <KToggleAction>
#include <KSelectAction>

#include <QPointer>

class KActionCollection;
class KHelpMenu;


namespace Amarok
{
    class Menu : public KMenu
    {
        Q_OBJECT
        public:
            Menu();
            static Menu *instance();
            static KMenu *helpMenu( QWidget *parent = 0 );

        private:
            static KHelpMenu  *s_helpMenu;
    };

    class MenuAction : public KAction
    {
        public:
            MenuAction( KActionCollection*, QObject* );
    };

    class PlayPauseAction : public KToggleAction, public EngineObserver
    {
        public:
            PlayPauseAction( KActionCollection*, QObject* );
            virtual void engineStateChanged( Phonon::State, Phonon::State = Phonon::StoppedState );
    };

    class ToggleAction : public KToggleAction
    {
        public:
            ToggleAction( const QString &text, void ( *f ) ( bool ), KActionCollection* const ac, const char *name, QObject *parent );

            virtual void setChecked( bool b );

            virtual void setEnabled( bool b );

        private:
            void ( *m_function ) ( bool );
    };

    class SelectAction : public KSelectAction
    {
        Q_OBJECT

        public:
            SelectAction( const QString &text, void ( *f ) ( int ), KActionCollection* const ac, const char *name, QObject *parent );

            virtual void setCurrentItem( int n );
            virtual void setEnabled( bool b );
            virtual void setIcons( QStringList icons );
            virtual QString currentText() const;
            QStringList icons() const;
            QString currentIcon() const;

        protected slots:
            virtual void actionTriggered( QAction *a );

        private:
            void ( *m_function ) ( int );
            QStringList m_icons;
    };

    class RandomAction : public SelectAction
    {
        public:
            RandomAction( KActionCollection *ac, QObject* );
            virtual void setCurrentItem( int n );
    };

    class FavorAction : public SelectAction
    {
        public:
            FavorAction( KActionCollection *ac, QObject* );
    };

    class RepeatAction : public SelectAction
    {
        public:
            RepeatAction( KActionCollection *ac, QObject* );
    };

    class BurnMenu : public KMenu
    {
        Q_OBJECT

        public:
            BurnMenu();
            static KMenu *instance();

        private slots:
            void slotBurnCurrentPlaylist();
            void slotBurnSelectedTracks();
    };


    class BurnMenuAction : public KAction
    {
        public:
            BurnMenuAction( KActionCollection*, QObject* );
            virtual QWidget* createWidget( QWidget* );
    };

    class StopAction : public KAction, public EngineObserver
    {
        public:
            StopAction( KActionCollection*, QObject* );
            virtual void engineStateChanged( Phonon::State, Phonon::State = Phonon::StoppedState );
    };

} /* namespace Amarok */


#endif /* AMAROK_ACTIONCLASSES_H */

