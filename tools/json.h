#ifndef JSON
#define JSON

#include "def.h"

#include <QString>
#include <QList>
#include <QJsonObject>

class JElement
{
public:
    JElement(QJsonObject j);
    JElement(QString name, QString content);
    JElement();

    QJsonObject toJson(Id id);

    Id unmarshalId;
    QString name;
    QString content;
};

class JSublist
{
public:
    JSublist(QJsonObject j);
    JSublist(QString title);
    JSublist();

    QJsonObject toJson(Id list, Id id);

    Id unmarshalId;
    QString title;
    QMap<Id, JElement> elements;

};

class JList
{
public:
    enum Type
    {
        None,
        Default,
        Calendar,

    };

    enum Diff {
        Title = 1,
        Priority = 2,
        DType = 4,
        Timestamp = 8,
    };

    JList(QJsonObject j);
    JList(QString title, Type type, int priority);
    JList();

    QJsonObject toJson(Id id);

    Type type;
    int priority;
    Id unmarshalId;
    QString title;
    QDateTime viewTimestamp;
    QMap<Id, JSublist> sublists;
};

#endif