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

#include "amarok.h"
#include "amarokconfig.h"
#include "debug.h"
#include "collectiondb.h"



////////////////////////////////////////////////////////////////////////////////
// class Options7
////////////////////////////////////////////////////////////////////////////////

void Options7::atf_clicked( bool checked )
{
    AmarokConfig::setATFJustTurnedOn( checked && !AmarokConfig::advancedTagFeatures() );
}

