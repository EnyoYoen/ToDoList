#include "manager.h"
#include "logger.h"

#include <QFile>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#ifdef DL
QString Manager::filepath = "build/modules/cache/";
#else
QString Manager::filepath = "./";
#endif

Manager *manager = nullptr;

Manager::Manager()
{
    timer.callOnTimeout(this, &Manager::update);
    timer.setSingleShot(true);
    timer.setInterval(50);

    updateTimer.callOnTimeout(this, &Manager::update);
    updateTimer.setSingleShot(true);
    updateTimer.setInterval(10);

    QFile jsonFile(filepath + "todolists.json");
    jsonFile.open(QIODevice::ReadOnly);

    if (jsonFile.isOpen()) {
        QJsonObject j = QJsonDocument::fromJson(jsonFile.readAll()).object();
        jsonFile.close();

        QJsonArray jlists = j["lists"].toArray();
        for (Id i = 0 ; i < (Id)jlists.count() ; i++) {
            lists.insert(i, JList(jlists[i].toObject()));
            for (auto sublistId : getSublists(i))
                elementsOrder[i].insert(sublistId, getElements(i, sublistId));
            sublistsOrder.insert(i, getSublists(i));
        }
        for (auto recentList : j["recent"].toArray()) {
            recent.append(recentList.toString());
        }
    } else {
        jsonFile.close();
    }

    QFile changesFile(filepath + "changes.json");
    changesFile.open(QIODevice::ReadOnly);

    if (changesFile.isOpen()) {
        QJsonObject changes = QJsonDocument::fromJson(changesFile.readAll()).object();
        changesFile.close();
        if (changes.isEmpty()) {
            changes["instances"] = 1;
            changes["changes"] = QJsonArray();
        } else {
            numberOfChanges = changes["changes"].toArray().size();
            instances = changes["instances"].toInteger() + 1;
            changes["instances"] = instances;
        }
        changesFile.close();
        changesFile.open(QIODevice::WriteOnly);
        changesFile.write(QJsonDocument(changes).toJson(QJsonDocument::Compact));
        changesFile.close();
    } else {
        changesFile.close();

        QJsonObject changes;
        changes["instances"] = 1;
        changes["changes"] = QJsonArray();
        changesFile.open(QIODevice::WriteOnly);
        changesFile.write(QJsonDocument(changes).toJson(QJsonDocument::Compact));
        changesFile.close();
    }

    watcher = new QFileSystemWatcher(QStringList(filepath + "changes.json"));
    QObject::connect(watcher, &QFileSystemWatcher::fileChanged, [this]() {
        if (updateTimer.isActive())
            updateTimer.stop();
        updateTimer.start();
    });
}

Manager::~Manager()
{
    watcher->deleteLater();

    QFile changesFile(filepath + "changes.json");
    changesFile.open(QIODevice::ReadOnly);

    if (changesFile.isOpen()) {
        QJsonObject changes = QJsonDocument::fromJson(changesFile.readAll()).object();
        changesFile.close();
        if (changes.isEmpty()) {
            changes["instances"] = 0;
            changes["changes"] = QJsonArray();
        } else {
            changes["instances"] = instances - 1;
            if (instances == 1) {
                changes["changes"] = QJsonArray();
            }
        }
        changesFile.close();
        changesFile.open(QIODevice::WriteOnly);
        changesFile.write(QJsonDocument(changes).toJson(QJsonDocument::Compact));
        changesFile.close();
    } else {
        changesFile.close();
        logger.error("Can't open changes.json");
    }
}

void Manager::save(QJsonObject change)
{
    QJsonObject jsave;
    QJsonArray jlists;
    QJsonArray jrecent;

    for (auto listId : getLists()) {
        jlists.append(getList(listId).toJson(listId));
    }

    for (auto title : recent) {
        jrecent.append(QJsonValue(title));
    }

    jsave.insert("recent", jrecent);
    jsave.insert("lists", jlists);
    
    QFile jsonFile(filepath + "todolists.json");
    jsonFile.open(QIODevice::WriteOnly);
    jsonFile.write(QJsonDocument(jsave).toJson(QJsonDocument::Compact));
    jsonFile.close();


    if (instances > 1) {
        QFile changesFile(filepath + "changes.json");
        changesFile.open(QIODevice::ReadOnly);

        if (changesFile.isOpen()) {
            QJsonObject changesObj = QJsonDocument::fromJson(changesFile.readAll()).object();
            changesFile.close();
            QJsonArray changes = changesObj["changes"].toArray();
            changes.append(change);
            changesObj["changes"] = changes;
            changing = true;
            changesFile.close();
            changesFile.open(QIODevice::WriteOnly);
            changesFile.write(QJsonDocument(changesObj).toJson(QJsonDocument::Compact));
            changesFile.close();
        } else {
            changesFile.close();
        }
    }
}


void Manager::viewList(Id list)
{
    QDateTime timestamp = QDateTime::currentDateTime();
    lists[list].viewTimestamp = timestamp;

    QJsonObject change;
    change["name"] = "listViewed";
    change["listId"] = QJsonValue::fromVariant(list);
    change["viewTimestamp"] = timestamp.toSecsSinceEpoch();
    save(change);
}

void Manager::removeList(Id list)
{
    lists.remove(list);
    
    QJsonObject change;
    change["name"] = "listRemoved";
    change["listId"] = QJsonValue::fromVariant(list);
    save(change);
}

void Manager::replaceList(Id listId, JList list)
{
    lists.remove(listId);
    lists.insert(listId, list);
    
    QJsonObject change;
    change["name"] = "listReplaced";
    change["listId"] = QJsonValue::fromVariant(listId);
    change["priority"] = list.priority;
    change["type"] = list.type;
    change["title"] = list.title;
    change["viewTimestamp"] = list.viewTimestamp.toSecsSinceEpoch();
    save(change);
}

Id Manager::addList(JList list)
{
    Id id = getLowestNewId(lists.keys());
    lists.insert(id, list);
    
    QJsonObject change;
    change["name"] = "listAdded";
    change["listId"] = QJsonValue::fromVariant(id);
    change["list"] = list.toJson(id);
    save(change);

    return id;
}

JList Manager::getList(Id id)
{
    return lists[id];
}

QList<Id> Manager::getLists()
{
    return lists.keys();
}


void Manager::setIndexSublist(Id list, Id sublist, size_t index)
{
    sublistsOrder[list].insert(index, sublistsOrder[list].takeAt(sublistsOrder[list].indexOf(sublist)));
    
    QJsonObject change;
    change["name"] = "sublistIndexChanged";
    change["listId"] = QJsonValue::fromVariant(list);
    change["sublistId"] = QJsonValue::fromVariant(sublist);
    change["index"] = qint64(index);
    save(change);
}

void Manager::removeSublist(Id list, Id sublist)
{
    lists[list].sublists.remove(sublist);
    sublistsOrder[list].remove(sublist);

    QJsonObject change;
    change["name"] = "sublistRemoved";
    change["listId"] = QJsonValue::fromVariant(list);
    change["sublistId"] = QJsonValue::fromVariant(sublist);
    save(change);
}

void Manager::replaceSublist(Id list, Id sublistId, JSublist sublist)
{
    lists[list].sublists.remove(sublistId);
    lists[list].sublists.insert(sublistId, sublist);
    
    QJsonObject change;
    change["name"] = "sublistReplaced";
    change["listId"] = QJsonValue::fromVariant(list);
    change["sublistId"] = QJsonValue::fromVariant(sublistId);
    change["title"] = sublist.title;
    save(change);
}

Id Manager::addSublist(Id list, JSublist sublist)
{
    auto sublists = lists[list].sublists;
    Id id = getLowestNewId(sublists.keys());
    sublists.insert(id, sublist);
    lists[list].sublists = sublists;
    sublistsOrder[list].append(id);
    
    QJsonObject change;
    change["name"] = "sublistAdded";
    change["listId"] = QJsonValue::fromVariant(list);
    change["sublistId"] = QJsonValue::fromVariant(id);
    change["sublist"] = sublist.toJson(list, id);
    save(change);
    
    return id;
}

JSublist Manager::getSublist(Id list, Id sublist)
{
    return lists[list].sublists[sublist];
}

QList<Id> Manager::getSublists(Id list)
{
    return lists[list].sublists.keys();
}


void Manager::setIndexElement(Id list, Id sublist, Id element, size_t index)
{
    elementsOrder[list][sublist].insert(index, elementsOrder[list][sublist].takeAt(elementsOrder[list][sublist].indexOf(element)));
    
    QJsonObject change;
    change["name"] = "elementIndexChanged";
    change["listId"] = QJsonValue::fromVariant(list);
    change["sublistId"] = QJsonValue::fromVariant(sublist);
    change["elementId"] = QJsonValue::fromVariant(element);
    change["index"] = qint64(index);
    save(change);
}

void Manager::removeElement(Id list, Id sublist, Id element)
{
    lists[list].sublists[sublist].elements.remove(element);
    elementsOrder[list][sublist].removeAll(element);
    
    QJsonObject change;
    change["name"] = "elementRemoved";
    change["listId"] = QJsonValue::fromVariant(list);
    change["sublistId"] = QJsonValue::fromVariant(sublist);
    change["elementId"] = QJsonValue::fromVariant(element);
    save(change);
}

void Manager::replaceElement(Id list, Id sublist, Id elementId, JElement element)
{
    lists[list].sublists[sublist].elements.remove(elementId);
    lists[list].sublists[sublist].elements.insert(elementId, element);
    
    QJsonObject change;
    change["name"] = "elementReplaced";
    change["listId"] = QJsonValue::fromVariant(list);
    change["sublistId"] = QJsonValue::fromVariant(sublist);
    change["elementId"] = QJsonValue::fromVariant(elementId);
    change["title"] = element.name;
    change["content"] = element.content;
    save(change);
}

Id Manager::addElement(Id list, Id sublist, JElement element)
{
    auto elements = lists[list].sublists[sublist].elements;
    Id id = getLowestNewId(elements.keys());
    lists[list].sublists[sublist].elements.insert(id, element);
    elementsOrder[list][sublist].append(id);
    
    QJsonObject change;
    change["name"] = "elementAdded";
    change["listId"] = QJsonValue::fromVariant(list);
    change["sublistId"] = QJsonValue::fromVariant(sublist);
    change["elementId"] = QJsonValue::fromVariant(id);
    change["element"] = element.toJson(id);
    save(change);

    return id;
}

JElement Manager::getElement(Id list, Id sublist, Id element)
{
    return lists[list].sublists[sublist].elements[element];
}

QList<Id> Manager::getElements(Id list, Id sublist)
{
    return lists[list].sublists[sublist].elements.keys();
}


QStringList Manager::getRecents()
{
    return recent;
}

void Manager::update()
{
    if (changing) {
        changing = false;
        return;
    }

    if (timer.isActive())
        return;

    QFile changesFile(filepath + "changes.json");
    changesFile.open(QIODevice::ReadOnly);

    if (changesFile.isOpen()) {
        QByteArray content = changesFile.readAll();
        changesFile.close();
        if (content.size() == 0) {
            timer.start();
            return;
        }
        QJsonObject changesObj = QJsonDocument::fromJson(content).object();

        instances = changesObj["instances"].toInteger();
        
        QJsonArray changes = changesObj["changes"].toArray();
        qint64 changesArraySize = changes.size();
        qint64 remainingChanges = changesArraySize - numberOfChanges;

        for (qint64 i = numberOfChanges ; i < changesArraySize ; i++) {
            QJsonObject change = changes[i].toObject();
            QString changeName = change["name"].toString();

            // List related change
            if (changeName == "listViewed") {
                Id id = change["listId"].toVariant().toULongLong();
                QDateTime timestamp = QDateTime::fromSecsSinceEpoch(change["viewTimestamp"].toInteger());

                lists[id].viewTimestamp = timestamp;

                emit listViewed(id, timestamp);
            } else if (changeName == "listRemoved") {
                Id id = change["listId"].toVariant().toULongLong();

                lists.remove(id);

                emit listRemoved(id);
            } else if (changeName == "listReplaced") {
                Id id = change["listId"].toVariant().toULongLong();

                JList list = lists[id];
                list.priority = change["priority"].toInt();
                list.type = (JList::Type)change["type"].toInt();
                list.title = change["title"].toString();
                list.viewTimestamp = QDateTime::fromSecsSinceEpoch(change["viewTimestamp"].toInteger());
                lists.remove(id);
                lists.insert(id, list);

                emit listReplaced(id);
            } else if (changeName == "listAdded") {
                Id id = change["listId"].toVariant().toULongLong();

                lists.insert(id, JList(change["list"].toObject()));

                emit listAdded(id);
            }
            // Sublist related change
            else if (changeName == "sublistIndexChanged") {
                Id list = change["listId"].toVariant().toULongLong();
                Id sublist = change["sublistId"].toVariant().toULongLong();
                qint64 index = change["index"].toInteger();

                sublistsOrder[list].insert(index, sublistsOrder[list].takeAt(sublistsOrder[list].indexOf(sublist)));

                emit sublistIndexChanged(list, sublist, index);
            } else if (changeName == "sublistRemoved") {
                Id list = change["listId"].toVariant().toULongLong();
                Id sublist = change["sublistId"].toVariant().toULongLong();

                lists[list].sublists.remove(sublist);
                sublistsOrder[list].remove(sublist);

                emit sublistRemoved(list, sublist);
            } else if (changeName == "sublistReplaced") {
                Id list = change["listId"].toVariant().toULongLong();
                Id sublistId = change["sublistId"].toVariant().toULongLong();
                QString title = change["title"].toString();

                JSublist sublist = lists[list].sublists[sublistId];
                sublist.title = title;
                lists[list].sublists.remove(sublistId);
                lists[list].sublists.insert(sublistId, sublist);

                emit sublistReplaced(list, sublistId);
            } else if (changeName == "sublistAdded") {
                Id list = change["listId"].toVariant().toULongLong();
                Id sublistId = change["sublistId"].toVariant().toULongLong();
                JSublist sublist(change["sublist"].toObject());
                
                auto sublists = lists[list].sublists;
                sublists.insert(sublistId, sublist);
                lists[list].sublists = sublists;
                sublistsOrder[list].append(sublistId);

                emit sublistAdded(list, sublistId);
            }
            // Element related change
            else if (changeName == "elementIndexChanged") {
                Id list = change["listId"].toVariant().toULongLong();
                Id sublist = change["sublistId"].toVariant().toULongLong();
                Id element = change["elementId"].toVariant().toULongLong();
                qint64 index = change["index"].toInteger();

                elementsOrder[list][sublist].insert(index, elementsOrder[list][sublist].takeAt(elementsOrder[list][sublist].indexOf(element)));

                emit elementIndexChanged(list, sublist, element, index);
            } else if (changeName == "elementRemoved") {
                Id list = change["listId"].toVariant().toULongLong();
                Id sublist = change["sublistId"].toVariant().toULongLong();
                Id element = change["elementId"].toVariant().toULongLong();

                lists[list].sublists[sublist].elements.remove(element);
                elementsOrder[list][sublist].removeAll(element);

                emit elementRemoved(list, sublist, element);
            } else if (changeName == "elementReplaced") {
                Id list = change["listId"].toVariant().toULongLong();
                Id sublist = change["sublistId"].toVariant().toULongLong();
                Id elementId = change["elementId"].toVariant().toULongLong();
                QString name = change["title"].toString();
                QString content = change["content"].toString();

                JElement element = lists[list].sublists[sublist].elements[elementId];
                element.name = name;
                element.content = content;
                lists[list].sublists[sublist].elements.remove(elementId);
                lists[list].sublists[sublist].elements.insert(elementId, element);

                emit elementReplaced(list, sublist, elementId);
            } else if (changeName == "elementAdded") {
                Id list = change["listId"].toVariant().toULongLong();
                Id sublist = change["sublistId"].toVariant().toULongLong();
                Id elementId = change["elementId"].toVariant().toULongLong();
                JElement element(change["element"].toObject());

                auto elements = lists[list].sublists[sublist].elements;
                lists[list].sublists[sublist].elements.insert(elementId, element);
                elementsOrder[list][sublist].append(elementId);

                emit elementAdded(list, sublist, elementId);
            }
            // Unrecognized change
            else {
                logger.warn("Manager: '" + changeName.toStdString() + "' change not recognized");
            }
        }

        numberOfChanges = changesArraySize;
    } else {
        changesFile.close();
    }
}

// Maybe we can optimize
Id Manager::getLowestNewId(QList<Id> ids)
{
    qsizetype size = ids.size();
    if (size == 0 || ids[0] > 0)
        return 0;

    std::sort(ids.begin(), ids.end());
        
    Id previousId = ids[0];
    qsizetype i = 1;
    while (i < size) {
        if (ids[i] > previousId + 1)
            return previousId + 1;
        previousId++;
        i++;
    }
    return ids.last() + 1;
}