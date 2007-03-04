#include "servicemodelitembase.h"

/*QString ServiceModelItemBase::getUrl() {
    return QString();
} 

ServiceModelItemBase *ServiceModelItemBase::child(int row) {
    return m_childItems.value(row);
}

int ServiceModelItemBase::childCount() const {
     return m_childItems.count();
}
*/
ServiceModelItemBase *ServiceModelItemBase::parent() {
    return m_parent;
}

/*QList<ServiceModelItemBase*> ServiceModelItemBase::getChildItems() const {
    return m_childItems;
}


bool ServiceModelItemBase::hasChildren () const {
    return !m_childItems.isEmpty();
}


int ServiceModelItemBase::columnCount() const {
    return 1;
}*/
