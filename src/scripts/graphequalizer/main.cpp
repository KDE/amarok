/*
 *   Copyright (C) 2005 by Ian Monroe <ian@monroe.nu>
 *   Released under GPL 2 or later, see COPYING
 */

#include "eqdialog.h"
#include "equalizercanvasview.h"
#include "stdinreader.h"
#include <kapplication.h>
#include <kaboutdata.h>
#include <kdebug.h>
#include <klocale.h>
#include <kcmdlineargs.h>


#include <qcanvas.h>
#include <qlayout.h>

#include <qsocketnotifier.h>
#include <qtextstream.h>


static const char description[] =
    I18N_NOOP("An Amarok Equalizer using a line graph");

static const char version[] = "0.5";

int main(int argc, char **argv)
{
    KAboutData about("Graph Equalizer", I18N_NOOP("Graph Equalizer"), version, description,
             KAboutData::License_GPL, "(C) 2005 Ian Monroe", 0, 0, "ian@monroe.nu");
    about.addAuthor( "Ian Monroe", 0, "ian@monroe.nu" );
    KCmdLineArgs::init( argc, argv, &about );
    KApplication app;
    EqDialog *mainWin = new EqDialog();
//     mainWin = new EqualizerGraph(0,"equalizerdialog");
//     QCanvas canvas;
//     canvas.resize(400, 200);
//     EqualizerCanvasView* eq = new EqualizerCanvasView(&canvas,mainWin,"eqcanvasview");
//     mainWin->getHLayout()->addWidget(eq);
//    app.setMainWidget( mainWin );
    mainWin->show();
    StdinReader* listen = new StdinReader(mainWin, "ioListener");
    CallAmarok* ca = new CallAmarok(mainWin,"ca", mainWin->canvasView,mainWin->preampSlider);
    mainWin->connect(listen, SIGNAL(openWindow()), mainWin, SLOT(show()));
    mainWin->connect(mainWin->canvasView,SIGNAL(eqChanged()),ca, SLOT(updateEq()));
    mainWin->connect(mainWin->preampSlider,SIGNAL(sliderReleased()),ca, SLOT(updateEq()));
    return app.exec();
}

