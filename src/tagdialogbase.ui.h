//Released under GPLv2 or later. (C) Mark Kretschmann <markey@web.de>
//
//This file is a hack to make tagdialogbase.cpp compile with KDE 3.3.
//Designer does not know about KTabWidget and forgets to include its header file
//in the cpp output.

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

#include <ktabwidget.h>
