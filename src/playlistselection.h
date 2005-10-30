#ifndef PLAYLISTSELECTION_H
#define PLAYLISTSELECTION_H

#include <klistview.h>
#include <qlistview.h>

class NewDynamic;

//this is a widget used in newdynamic.ui
class PlaylistSelection : public KListView
{
 Q_OBJECT
 public:
    PlaylistSelection(QWidget* parent, char* name);
 private:
    void loadChildren(QListViewItem* browserParent, QListViewItem* selectionParent);
};

class ConfigDynamic
{
  public:
    static void dynamicDialog(QWidget* parent);
  private:
    ConfigDynamic::ConfigDynamic() {} //class isn't meant to be an object
    static void ConfigDynamic::addDynamic(NewDynamic* dialog);
};

class SelectionListItem : public QCheckListItem
{
 public:
    SelectionListItem(QListViewItem  * parent, const QString& text, QListViewItem* browserEquivalent);
    SelectionListItem(QCheckListItem * parent, const QString& text, QListViewItem* browserEquivalent);
 protected:
    virtual void stateChange(bool);
 private:
    QListViewItem* m_browserEquivalent;
};
#endif
