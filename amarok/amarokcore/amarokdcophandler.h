#ifndef AMAROK_DCOP_HANDLER_H
#define AMAROK_DCOP_HANDLER_H

#include <qobject.h>
#include "amarokdcopiface.h"

class AmarokDcopHandler : public QObject, virtual public AmarokIface
{
      Q_OBJECT

   public:
      AmarokDcopHandler();
      void setNowPlaying( const QString & );
//      void setTrackTotalTime( int );

   public /* DCOP */ slots:
      virtual void play();
      virtual void stop();
      virtual void next();
      virtual void prev();
      virtual void pause();
      virtual void seek(int s);
      virtual int  trackTotalTime();
      virtual int  trackCurrentTime();
      virtual void addMedia(const KURL &);
      virtual void addMediaList(const KURL::List &);
      virtual QString nowPlaying();
      virtual bool isPlaying();

   private:
      QString m_nowPlaying; /* state for nowPlaying() */
//      int m_trackTotalTime;
};

#endif
