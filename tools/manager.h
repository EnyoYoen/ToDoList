#ifndef MANAGER
#define MANAGER

#include "json.h"
#include "def.h"

#include <QStringList>
#include <QMap>
#include <QFileSystemWatcher>
#include <QTimer>

class Manager : public QObject
{
    Q_OBJECT
public:
    Manager();
    ~Manager();

    void save(QJsonObject change); // change should contain a field "name" containing the name of the change
                                   // and can contain other fields like "listId", "index", depending on the change 

    void viewList(Id list);
    void removeList(Id list);
    void replaceList(Id listId, JList list);
    Id addList(JList list);
    JList getList(Id list);
    QList<Id> getLists();

    void setIndexSublist(Id list, Id sublist, size_t index);
    void removeSublist(Id list, Id sublist);
    void replaceSublist(Id list, Id sublistId, JSublist sublist);
    Id addSublist(Id list, JSublist sublist);
    JSublist getSublist(Id list, Id sublist);
    QList<Id> getSublists(Id list);

    void setIndexElement(Id list, Id sublist, Id element, size_t index);
    void removeElement(Id list, Id sublist, Id element);
    void replaceElement(Id list, Id sublist, Id elementId, JElement element);
    Id addElement(Id list, Id sublist, JElement element);
    JElement getElement(Id list, Id sublist, Id element);
    QList<Id> getElements(Id list, Id sublist);

    QStringList getRecents();

signals:
    void listViewed(Id list, QDateTime timestamp);
    void listRemoved(Id list);
    void listReplaced(Id list);
    void listAdded(Id list);

    void sublistIndexChanged(Id list, Id sublist, size_t index);
    void sublistRemoved(Id list, Id sublist);
    void sublistReplaced(Id list, Id sublist);
    void sublistAdded(Id list, Id sublist);

    void elementIndexChanged(Id list, Id sublist, Id element, size_t index);
    void elementRemoved(Id list, Id sublist, Id element);
    void elementReplaced(Id list, Id sublist, Id element);
    void elementAdded(Id list, Id sublist, Id element);
    
private:
    void update();
    Id getLowestNewId(QList<Id> ids);

    QMap<Id, JList> lists; // {id: list}
    QMap<Id, QList<Id>> sublistsOrder; // {listId: [sublistId, ...]}
    QMap<Id, QMap<Id, QList<Id>>> elementsOrder; // {listId: {sublistId: [elementId, ...]}}
    QTimer timer;
    QTimer updateTimer;
    QStringList recent;
    QFileSystemWatcher *watcher = nullptr;
    qint64 instances = 1;
    qint64 numberOfChanges = 0;
    bool changing = false;
    static QString filepath;
};

extern Manager *manager;

#endif