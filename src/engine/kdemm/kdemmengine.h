/***************************************************************************
          kdemmengine.h  -  KDEMM audio interface
                         -------------------
begin                : Jul 04 2004
copyright            : (C) 2004 by Roland Gigler
email                : rolandg@web.de
what                 : interface to KDE Multimedia (KDEMM)
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_KDEMMENGINE_H
#define AMAROK_KDEMMENGINE_H

#include "enginebase.h"

#include <vector>

#include <qguardedptr.h>
#include <qmap.h>
#include <qstringlist.h>
#include <qwidget.h>


class QTimer;
class KURL;

namespace KDE { namespace Multimedia { class SimplePlayer; } }

class KDEMMEngine : public Engine::Base
{
    Q_OBJECT

    public:
        KDEMMEngine();
        ~KDEMMEngine();

        bool init();

        bool initMixer( bool hardware );
        bool canDecode( const KURL& ) const;
        uint position() const;
        Engine::State state() const {return m_state;}
//        std::vector<float>* scope();
//        bool decoderConfigurable();
//        void configureDecoder();
        bool supportsXFade() const     { return false; }

    public slots:
        bool load( const KURL&, bool stream );
        bool play( unsigned int offset = 0);
        void stop();
        void pause();
        void seek( unsigned int ms );
        void setVolumeSW( unsigned int percent );
    private slots:
	void playingTimeout();
    private:
        //void startXfade();
        /////////////////////////////////////////////////////////////////////////////////////
        // ATTRIBUTES
        /////////////////////////////////////////////////////////////////////////////////////
        Engine::State m_state;
        //long m_scopeId;
        //int  m_scopeSize;
        //bool m_xfadeFadeout;
        //float m_xfadeValue;
        //QString m_xfadeCurrent;
        KURL m_url;
	KDE::Multimedia::SimplePlayer *m_player;

	QTimer* m_pPlayingTimer;
};

#endif // AMAROK_KDEMM_ENGINE

