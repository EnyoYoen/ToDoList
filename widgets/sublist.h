#ifndef SUBLIST
#define SUBLIST

#include "../tools/json.h"
#include "element.h"
#include "iconbutton.h"

#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

class Sublist : public QLabel
{
    Q_OBJECT
public:
    Sublist(Id list, Id sublist, QWidget *p);

    void rename(QString name);
    void deselect();
    bool dropElement(Id elementId, Id fromSublist, int y, int elementHeight);

    QVBoxLayout *lay = nullptr; 

signals:
    void renameButtonClicked();
    void deleteButtonClicked();
    void popupRequest(QList<QStringPair> prompts, QString title, std::function<void(bool, QStringList)> handler);
    void pressed(QPointF offset);
    void elementPressed(QPointF offset, QPointF pos, Id elementId, int layPos, Element *element);

public slots:
    void newElement();

private:
    QLabel *title = nullptr;
    IconButton *renameButton = nullptr;
    IconButton *deleteButton = nullptr;
    QScrollArea *elementsScroll = nullptr;
    QWidget *elementsContainer = nullptr;
    QVBoxLayout *elementsLay = nullptr;
    QPushButton *addButton = nullptr;
    QList<Element *> elements;
    Id list, sublist;
    bool press = false;

    void createElement(QString name, QString content, Id elementId, int index = -1);

    virtual void mousePressEvent(QMouseEvent *e);
};

#endif