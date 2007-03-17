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
//Added by qt3to4:
#include <QByteArray>
//#include <kdebug.h>

void EqDialog::init()
{
 	//kDebug() << "start" << endl;
    Q3Canvas* canvas = new Q3Canvas();
    canvas->resize(400, 200);
    canvasView->setVScrollBarMode(Q3ScrollView::AlwaysOff);
    canvasView->setHScrollBarMode(Q3ScrollView::AlwaysOff);
    canvasView->setCanvas(canvas);
    canvasView->init();
    QByteArray send_data, reply_data;
    QByteArray reply_type;
    //kDebug() << "continue" << endl;
    if(!KApplication::dcopClient()->call("amarok","player","equalizerEnabled()", send_data, reply_type, reply_data,true,1000));
    //kDebug() << "called" << endl;
    QDataStream answer(reply_data, QIODevice::ReadOnly);
    //kDebug() << "answer created" << answer << endl;
    bool eqEnabled;
    answer >> eqEnabled;
    //kDebug() << "eqEnabled set to " << eqEnabled << endl;
    eqGroupBox->setChecked(eqEnabled);
    //kDebug() << "end" << endl;
}


void EqDialog::eqGroupBox_toggled( bool eqEnabled)
{
        QByteArray data;
        QDataStream arg(data, QIODevice::WriteOnly);
        arg << eqEnabled;
        KApplication::dcopClient()->send("amarok", "player", "setEqualizerEnabled(bool)" , data);
}
