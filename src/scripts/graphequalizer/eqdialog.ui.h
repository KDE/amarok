/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you want to add, delete, or rename functions or slots, use
** Qt Designer to update this file, preserving your code.
**
** You should not define a constructor or destructor in this file.
** Instead, write your code in functions called init() and destroy().
** These will automatically be called by the form's constructor and
** destructor.
*****************************************************************************/
#include <dcopclient.h>
#include <kapplication.h>
//#include <kdebug.h>

void EqDialog::init()
{
 	//kdDebug() << "start" << endl;
    QCanvas* canvas = new QCanvas();
    canvas->resize(400, 200);
    canvasView->setVScrollBarMode(QScrollView::AlwaysOff);
    canvasView->setHScrollBarMode(QScrollView::AlwaysOff);
    canvasView->setCanvas(canvas);
    canvasView->init();
    QByteArray send_data, reply_data;
    QCString reply_type;
    //kdDebug() << "continue" << endl;
    if(!KApplication::dcopClient()->call("amarok","player","equalizerEnabled()", send_data, reply_type, reply_data,true,1000));
    //kdDebug() << "called" << endl;
    QDataStream answer(reply_data, IO_ReadOnly);
    //kdDebug() << "answer created" << answer << endl;
    bool eqEnabled;
    answer >> eqEnabled;
    //kdDebug() << "eqEnabled set to " << eqEnabled << endl;
    eqGroupBox->setChecked(eqEnabled);
    //kdDebug() << "end" << endl;
}


void EqDialog::eqGroupBox_toggled( bool eqEnabled)
{
        QByteArray data;
        QDataStream arg(data, IO_WriteOnly);
        arg << eqEnabled;
        KApplication::dcopClient()->send("amarok", "player", "setEqualizerEnabled(bool)" , data);
}
