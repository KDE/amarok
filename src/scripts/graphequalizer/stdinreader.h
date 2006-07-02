#ifndef _STDINREADER_H_
#define _STDINREADER_H_

/*
 *   Copyright (C) 2005 by Ian Monroe <ian@monroe.nu>
 *   Released under GPL 2 or later, see COPYING
 */
#include <qsocketnotifier.h>
#include <qtextstream.h>
#include <kdebug.h>

class StdinReader : public QObject
{
Q_OBJECT    
    public:
    StdinReader(QObject * parent = 0, const char * name = 0)
    :QObject(parent,name)
    {
        QSocketNotifier* streamListener = new QSocketNotifier(0, QSocketNotifier::Read, this, "stdinWatcher");
        connect(streamListener, SIGNAL(activated(int)), this, SLOT(dataRecieved()) );

    }
    ~StdinReader() { }
    signals:
        void openWindow();
    public slots:
    void dataRecieved()
    {
        //separate stdin pointer necesary for OS X for reasons unknown
        FILE * stdin_ptr = stdin;
        QString signal;
        QTextIStream( stdin_ptr ) >> signal;
        if(signal == "configure")
            emit openWindow();
    }
    
    
};

#endif

