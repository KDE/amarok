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


#include <kapplication.h>
#include <kstandarddirs.h>

#include <qdir.h>
#include <qfileinfo.h>

void Options2::init()
{
    QStringList styleList = kapp->dirs()->findAllResources("data","amarok/themes/*/stylesheet.css", false);
    QStringList sortedList;
    foreach (styleList)
        {
            sortedList.append(QFileInfo( *it ).dir().dirName());
        }
    sortedList.sort();
    foreach(sortedList)
      styleComboBox->insertItem(*it);    
    styleComboBox->setCurrentItem(AmarokConfig::contextBrowserStyleSheet());
}
