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

namespace amaroK {

class Systray : public KSystemTray
{
Q_OBJECT

public:
    Systray( QWidget *, KActionCollection * );

private:
    bool event( QEvent* );
};

}

#endif
