//
// AmarokSystray
//
// Author: Stanislav Karchebny <berkus@users.sf.net>, (C) 2003
//
// Copyright: like rest of amaroK
//

#ifndef AMAROKSYSTRAY_H
#define AMAROKSYSTRAY_H

#include <ksystemtray.h>

class KActionCollection;
class QEvent;

//TODO use Amarok namespace

class AmarokSystray : public KSystemTray
{
Q_OBJECT

public:
    AmarokSystray( QWidget *, KActionCollection * );

private:
    bool event( QEvent* );
};

#endif
