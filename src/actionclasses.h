// Maintainer: Max Howell <max.howell@methylblue.com>, (C) 2004
// Copyright:  See COPYING file that comes with this distribution
//
// Description: a popupmenu to control various features of amaroK
//              also provides amaroK's helpMenu

#ifndef AMAROK_ACTIONCLASSES_H
#define AMAROK_ACTIONCLASSES_H

#include "engineobserver.h"
#include "sliderwidget.h"

#include <kaction.h>
#include <kactionclasses.h>
#include <kpopupmenu.h>
#include <qguardedptr.h>

class KActionCollection;
class KHelpMenu;


namespace amaroK
{
    class Menu : public KPopupMenu
    {
        Q_OBJECT
        public:
            static Menu *instance();
            static KPopupMenu *helpMenu( QWidget *parent = 0 );

            enum MenuIds {
                ID_CONF_DECODER,
                ID_SHOW_VIS_SELECTOR,
                ID_SHOW_COVER_MANAGER,
                ID_SHOW_EFFECTS,
                ID_CONFIGURE_EQUALIZER,
                ID_SHOW_WIZARD
            };

        public slots:
            void slotActivated( int index );

        private slots:
            void slotAboutToShow();

        private:
            Menu();

            static KHelpMenu  *s_helpMenu;
    };


    class MenuAction : public KAction
    {
        public:
            MenuAction( KActionCollection* );
            virtual int plug( QWidget*, int index = -1 );
    };


    class PlayPauseAction : public KAction, public EngineObserver
    {
        public:
            PlayPauseAction( KActionCollection* );
            ~PlayPauseAction();
            virtual void engineStateChanged( Engine::State );
    };


    class AnalyzerAction : public KAction
    {
        public:
            AnalyzerAction( KActionCollection* );
            virtual int plug( QWidget *, int index = -1 );
    };


    class VolumeAction : public KAction, public EngineObserver
    {
        public:
            VolumeAction( KActionCollection* );
            ~VolumeAction();

            virtual int plug( QWidget *, int index = -1 );

        private:
            void engineVolumeChanged( int value );

            QGuardedPtr<amaroK::Slider> m_slider;
    };


    class ToggleAction : public KToggleAction
    {
        public:
            ToggleAction( const QString &text, void ( *f ) ( bool ), KActionCollection* const ac, const char *name )
                    : KToggleAction( text, 0, ac, name )
                    , m_function( f )
            {}

            virtual void setChecked( bool b )
            {
                const bool announce = b != isChecked();

                m_function( b );
                KToggleAction::setChecked( b );
                if( announce ) emit toggled( b ); //KToggleAction doesn't do this for us. How gay!
            }

        private:
            void ( *m_function ) ( bool );
    };


    class RandomAction : public ToggleAction
    {
        public:
            RandomAction( KActionCollection *ac );
    };


    class RepeatTrackAction : public ToggleAction
    {
        public:
            RepeatTrackAction( KActionCollection *ac );
    };


    class RepeatPlaylistAction : public ToggleAction
    {
        public:
            RepeatPlaylistAction( KActionCollection *ac );
    };

    class BurnMenu : public KPopupMenu
    {
            Q_OBJECT

        public:
            enum MenuIds {
                CURRENT_PLAYLIST,
                SELECTED_TRACKS
            };

            static KPopupMenu *instance();

        private slots:
            void slotAboutToShow();
            void slotActivated( int index );

        private:
            BurnMenu();
    };


    class BurnMenuAction : public KAction
    {
        public:
            BurnMenuAction( KActionCollection* );
            virtual int plug( QWidget*, int index = -1 );
    };

} /* namespace amaroK */


#endif /* AMAROK_ACTIONCLASSES_H */

