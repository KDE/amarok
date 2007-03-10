#ifndef AMAROKSERVICEMODELITEMBASE_H
#define AMAROKSERVICEMODELITEMBASE_H 

#include <QString>
#include <QVariant>

class ServiceModelItemBase
{
public:

    virtual QVariant data(int column) const = 0;

    virtual ServiceModelItemBase *child(int row) = 0;
    virtual int childCount() const = 0;
    virtual int columnCount() const = 0;
    virtual int row() const = 0;
    ServiceModelItemBase * parent();
    virtual QList<ServiceModelItemBase*> getChildItems() const = 0;
    virtual bool hasChildren () const = 0;
    virtual QString getUrl() = 0;



protected:
     mutable QList<ServiceModelItemBase*> m_childItems;
     ServiceModelItemBase *m_parent;
};

#endif
