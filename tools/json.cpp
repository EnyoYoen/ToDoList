#include "json.h"
#include "manager.h"

#include <QJsonArray>

JElement::JElement(QJsonObject j)
    : name(j["name"].toString()), content(j["content"].toString()) 
{}

JElement::JElement(QString m_name, QString m_content) : name(m_name), content(m_content) {}

JElement::JElement() {}

QJsonObject JElement::toJson(Id id)
{
    QJsonObject jelement;
    jelement.insert("id", QJsonValue::fromVariant(id));
    jelement.insert("name", name);
    jelement.insert("content", content);

    return jelement;
}

JSublist::JSublist(QJsonObject j)
    : title(j["title"].toString())
{
    auto jarray = j["elements"].toArray();
    for (Id i = 0 ; i < (Id)jarray.size() ; i++)
        elements.insert(i, JElement(jarray[i].toObject()));
}

JSublist::JSublist(QString m_title) : title(m_title) {}

JSublist::JSublist() {}

QJsonObject JSublist::toJson(Id list, Id id)
{
    QJsonObject jsublist;
    jsublist.insert("id", QJsonValue::fromVariant(id));
    jsublist.insert("title", title);

    QJsonArray jelements;
    for (auto elementId : manager->getElements(list, id))
        jelements.append(manager->getElement(list, id, elementId).toJson(elementId));
    jsublist.insert("elements", jelements);

    return jsublist;
}

JList::JList(QJsonObject j)
    : type((Type)j["type"].toInt()), priority(j["priority"].toInt()), title(j["title"].toString()),
      viewTimestamp(QDateTime::fromSecsSinceEpoch(j["view_timestamp"].toInteger())) 
{
    auto jarray = j["sublists"].toArray();
    for (Id i = 0 ; i < (Id)jarray.size() ; i++)
        sublists.insert(i, JSublist(jarray[i].toObject()));
}

JList::JList(QString m_title, Type m_type, int m_priority) : title(m_title), type(m_type), priority(m_priority) {}

JList::JList() : type(None) {}

QJsonObject JList::toJson(Id id)
{
    QJsonObject jlist;
    jlist.insert("id", QJsonValue::fromVariant(id));
    jlist.insert("type", (int)type);
    jlist.insert("priority", priority);
    jlist.insert("title", title);
    jlist.insert("view_timestamp", viewTimestamp.toSecsSinceEpoch());

    QJsonArray jsublists;
    for (auto sublistId : manager->getSublists(id))
        jsublists.append(manager->getSublist(id, sublistId).toJson(id, sublistId));
    jlist.insert("sublists", jsublists);

    return jlist;
}