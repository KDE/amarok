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

void EqDialog::init()
{
    QCanvas* canvas = new QCanvas();
    canvas->resize(400, 200);
    canvasView->setVScrollBarMode(QScrollView::AlwaysOff);
    canvasView->setHScrollBarMode(QScrollView::AlwaysOff);
    canvasView->setCanvas(canvas);
    canvasView->init();
}
