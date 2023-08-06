#ifndef LIST
#define LIST

#include "sublist.h"
#include "popup.h"
#include "iconbutton.h"
#include "../tools/json.h"
#include "../tools/manager.h"

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QPushButton>

class List : public QWidget
{
    Q_OBJECT
public:
    List(Id id, QWidget *p);

    void updateList();
    void changeSublistIndex(Id sublist, size_t index);
    void removeSublist(Id sublist);
    void updateSublist(Id sublist);
    void addSublist(Id sublist);
    void changeElementIndex(Id sublist, Id element, size_t index);
    void removeElement(Id sublist, Id element);
    void updateElement(Id sublist, Id element);
    void addElement(Id sublist, Id element);

    void newElement();

    Id id;

signals:
    void back();
    void deleted();
    void renamed();

public slots:
    void addEmptySublist();

private:
    QVBoxLayout *lay = nullptr;
    QWidget *header = nullptr;
    QHBoxLayout *headerLay = nullptr; 
    QPushButton *backButton = nullptr;
    QLabel *title = nullptr;
    IconButton *editNameButton = nullptr;
    IconButton *deleteButton = nullptr;
    QWidget *sublistsContainer = nullptr;
    QHBoxLayout *sublistsContainerLay = nullptr;
    QPushButton *newSublistButton = nullptr;
    PopUp *popup = nullptr;
    QMap<Id, Sublist *> sublists;

    Sublist *sublist = nullptr;
    QPointF sublistDragOffset;
    Id sublistPressedId = InvalidId;
    bool sublistPressed = false;

    Element *element = nullptr;
    Sublist *elementSublist = nullptr;
    QPointF elementDragOffset;
    Id elementPressedId = InvalidId;
    int elementLayPos;
    bool elementPressed = false;

    void createSublist(QString title, Id sublistId);

    virtual void mousePressEvent(QMouseEvent *e);
    virtual void mouseReleaseEvent(QMouseEvent *e);
    virtual void mouseMoveEvent(QMouseEvent *e);
};

#endif