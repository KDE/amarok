// Maintainer: Max Howell <max.howell@methylblue.com>, (C) 2004
// Copyright:  See COPYING file that comes with this distribution
//
// Description: a popupmenu to control various features of amaroK
//              also provides amaroK's helpMenu

#ifndef AMAROK_ACTIONCLASSES_H
#define AMAROK_ACTIONCLASSES_H

#include "engineobserver.h"
#include <kaction.h>
#include <kactionclasses.h>
#include <kpopupmenu.h>

class QSlider;

class KActionCollection;
class KHelpMenu;


namespace amaroK
{
    class Menu : public KPopupMenu
    {
            Q_OBJECT

        public:
            static const int ID_CONF_DECODER = 103;
            static const int ID_SHOW_VIS_SELECTOR = 104;

            Menu( QWidget *parent );

            static KPopupMenu *helpMenu( QWidget *parent = 0 );

        private slots:
            void slotAboutToShow();
            void slotActivated( int index );

        private:
            static KHelpMenu *HelpMenu;
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
            virtual void engineStateChanged( EngineBase::EngineState );
    };


    class AnalyzerAction : public KAction
    {
        public:
            AnalyzerAction( KActionCollection* );
            virtual int plug( QWidget *, int index = -1 );
    };


    class VolumeAction : public KAction, public EngineObserver
    {
            Q_OBJECT
        
        public:
            VolumeAction( KActionCollection* );
            virtual int plug( QWidget *, int index = -1 );

        private slots:
            void sliderMoved( int value );
            void wheelMoved( int delta );

        private:
            void engineVolumeChanged( int value );

            QSlider* m_slider;
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
                m_function( b );
                KToggleAction::setChecked( b );
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
} /* namespace amaroK */


#endif /* AMAROK_ACTIONCLASSES_H */

