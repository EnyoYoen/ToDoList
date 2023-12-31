#include "list.h"

#include "filters.h"
#include "../tools/logger.h"
#include "../tools/manager.h"

List::List(Id m_id, QWidget *p)
    : id(m_id), QWidget(p)
{
    JList jlist = manager->getList(id);

    lay = new QVBoxLayout(this);
    header = new QWidget(this);
    headerLay = new QHBoxLayout(header);
    backButton = new QPushButton(header);
    title = new QLabel(jlist.title, header);
    editNameButton = new IconButton([this]() {
        if (popup)
            popup->deleteLater();
        QList<QStringPair> prompts;
        prompts.append(QStringPair("Name", manager->getList(id).title));
        popup = new PopUp(prompts, "Rename the list", this);
        QObject::connect(popup, &PopUp::result, [this](bool cancelled, QStringList prompts) {
            if (!cancelled && !prompts.isEmpty() && !prompts[0].isEmpty()) {
                title->setText(prompts[0]);

                JList jlist = manager->getList(id);
                jlist.title = prompts[0];
                manager->replaceList(id, jlist);
                emit renamed();
            }
            popup->deleteLater();
            popup = nullptr;
        });
    }, "pen", header);
    deleteButton = new IconButton([this]() {
        if (popup)
            popup->deleteLater();
        popup = new PopUp(QList<QStringPair>(), "Are you sure ?", this);
        QObject::connect(popup, &PopUp::result, [this](bool cancelled, QStringList prompts) {
            if (!cancelled) {
                manager->removeList(id);
                emit deleted();
                deleteLater();
            }
            popup->deleteLater();
            popup = nullptr;
        });
    }, "bin", header);
    sublistsContainer = new QWidget(this);
    sublistsContainerLay = new QHBoxLayout(sublistsContainer);
    newSublistButton = new QPushButton("Add Sublist", this);

    for (auto sublistId : manager->getSublists(id))
        createSublist(QString(), sublistId);
    sublistsContainerLay->setSpacing(10);

    backButton->setFixedSize(40, 40);

    headerLay->addWidget(backButton, 0, Qt::AlignVCenter);
    headerLay->addSpacing(10);
    headerLay->addWidget(title, 0, Qt::AlignVCenter);
    headerLay->addStretch();
    headerLay->addWidget(editNameButton, 0, Qt::AlignVCenter);
    headerLay->addWidget(deleteButton, 0, Qt::AlignVCenter);
    headerLay->setContentsMargins(0, 0, 0, 0);
    header->setFixedHeight(50);
    
    lay->addWidget(header);
    lay->addWidget(sublistsContainer);
    lay->setContentsMargins(5, 0, 5, 5);
    
    newSublistButton->setFixedHeight(30);

    title->setProperty("class", "list-title");
    header->setProperty("class", "list-header");
    backButton->setProperty("class", "list-back-button");
    newSublistButton->setProperty("class", "list-sublist-button");
    setProperty("class", "list");
    setAttribute(Qt::WA_StyledBackground, true);

    ResizeFilter *f = new ResizeFilter([this](QResizeEvent *e) {
        QSize s = e->size();
        newSublistButton->move(s.width() - newSublistButton->width() - 5, s.height() - newSublistButton->height() - 5);
        if (popup)
            popup->setFixedSize(s);
    });
    installEventFilter(f);

    QObject::connect(backButton, &QPushButton::clicked, [this]() { emit back(); });
    QObject::connect(newSublistButton, &QPushButton::clicked, this, &List::addEmptySublist);
}

void List::updateList()
{
    JList list = manager->getList(id);
    title->setText(list.title);
}

void List::changeSublistIndex(Id sublist, size_t index)
{
    sublistsContainerLay->insertWidget(index, sublistsContainerLay->takeAt(sublistsContainerLay->indexOf(sublists[sublist]))->widget());
}

void List::removeSublist(Id sublistId)
{
    Sublist *sublist = sublists[sublistId];
    sublistsContainerLay->removeWidget(sublist);
    sublists.remove(sublistId);
    sublist->deleteLater();
}

void List::updateSublist(Id sublist)
{
    sublists[sublist]->updateSublist();
}

void List::addSublist(Id sublist)
{
    createSublist(QString(), sublist);
}

void List::changeElementIndex(Id sublist, Id element, size_t index)
{
    sublists[sublist]->changeElementIndex(element, index);
}

void List::removeElement(Id sublist, Id element)
{
    sublists[sublist]->removeElement(element);
}   

void List::updateElement(Id sublist, Id element)
{
    sublists[sublist]->updateElement(element);
}

void List::addElement(Id sublist, Id element)
{
    sublists[sublist]->addElement(element);
}

void List::newElement()
{
    if (sublist)
        sublist->newElement();
}

void List::addEmptySublist()
{
    if (popup)
        popup->deleteLater();
    QList<QStringPair> prompts;
    prompts.append(QStringPair("Name", QString()));
    popup = new PopUp(prompts, "Name the sublist", this);
    QObject::connect(popup, &PopUp::result, [this](bool cancelled, QStringList prompts) {
        if (!cancelled && !prompts.isEmpty() && !prompts[0].isEmpty()) {
            createSublist(prompts[0], InvalidId);
        }
        popup->deleteLater();
        popup = nullptr;
    });
}


void List::createSublist(QString title, Id sublistId)
{
    int offset = 0;
    if (sublistId == InvalidId) {
        JSublist jsublist(title);
        sublistId = manager->addSublist(id, jsublist);
        offset = -3;
    }
    Sublist *sublist = new Sublist(id, sublistId, sublistsContainer);
    sublistsContainerLay->insertWidget(sublistsContainerLay->count() + offset, sublist);
    sublists.insert(sublistId, sublist);

    QObject::connect(sublist, &Sublist::renameButtonClicked, [this, sublist, sublistId]() {
        if (popup)
            popup->deleteLater();
        QList<QStringPair> prompts;
        prompts.append(QStringPair("Name", manager->getSublist(id, sublistId).title));
        popup = new PopUp(prompts, "Rename the sublist", this);
        QObject::connect(popup, &PopUp::result, [this, sublist](bool cancelled, QStringList prompts) {
            if (!cancelled && !prompts.isEmpty() && !prompts[0].isEmpty()) {
                sublist->rename(prompts[0]);
            }
            popup->deleteLater();
            popup = nullptr;
        });
    });
    QObject::connect(sublist, &Sublist::deleteButtonClicked, [this, sublist, sublistId]() {
        if (popup)
            popup->deleteLater();
        popup = new PopUp(QList<QStringPair>(), "Are you sure ?", this);
        QObject::connect(popup, &PopUp::result, [this, sublist, sublistId](bool cancelled, QStringList prompts) {
            if (!cancelled) {
                manager->removeSublist(id, sublistId);
                sublistsContainerLay->removeWidget(sublist);
                sublists.remove(sublistId);
                sublist->deleteLater();
            }
            popup->deleteLater();
            popup = nullptr;
        });
    });
    QObject::connect(sublist, &Sublist::popupRequest, [this](QList<QStringPair> prompts, QString title, std::function<void(bool, QStringList)> handler) {
        if (popup)
            popup->deleteLater();
        popup = new PopUp(prompts, title, this);
        QObject::connect(popup, &PopUp::result, [this, handler](bool cancelled, QStringList prompts) {
            handler(cancelled, prompts);
            popup->deleteLater();
            popup = nullptr;
        });
    });

    QObject::connect(sublist, &Sublist::pressed, [this, sublist, sublistId](QPointF offset) {
        this->sublist = sublist;
        sublistDragOffset = offset + sublistsContainer->pos();
        sublistPressedId = sublistId;
        sublistPressed = true;
        sublist->raise();
    });

    QObject::connect(sublist, &Sublist::elementPressed, [this, sublist, sublistId](QPointF offset, QPointF pos, Id elementId, int layPos, Element *m_element) {
        element = m_element;
        elementSublist = sublist;
        elementDragOffset = offset + sublistsContainer->pos();
        elementLayPos = layPos;
        elementPressed = true;
        elementPressedId = elementId;
        sublistPressedId = sublistId;

        QPointF origin = pos - sublistsContainer->pos() - offset;

        elementSublist->lay->removeWidget(element);
        lay->addWidget(element);
        element->setParent(sublistsContainer);
        element->move(origin.x(), origin.y());
        element->raise();
    });
}

void List::mousePressEvent(QMouseEvent *e)
{
    if (sublist)
        sublist->deselect();
    if (elementSublist)
        elementSublist->deselect();
}

void List::mouseReleaseEvent(QMouseEvent *e)
{
    if (elementPressed) {
        // Determine the sublist on which we put the element
        QPoint center = element->pos() + element->contentsRect().center();
        size_t min = -1;
        Sublist *hoveredSublist = nullptr;
        for (qsizetype i = 0 ; i < sublists.size() ; i++) {
            int distance = qAbs(sublists[i]->pos().x() + sublists[i]->contentsRect().center().x() - center.x());
            if (distance < min) {
                min = distance;
                hoveredSublist = sublists[i];
            }
        }

        // Drop the element or replace it 
        if (hoveredSublist->dropElement(elementPressedId, sublistPressedId, element->y() + element->height() / 2, element->height())) {
            element->deleteLater();
        } else {
            lay->removeWidget(element);
            element->setParent(elementSublist);
            elementSublist->lay->insertWidget(elementLayPos, element);
        }

        element = nullptr;
        elementSublist = nullptr;
        elementPressed = false;
        elementPressedId = InvalidId;
        sublistPressedId = InvalidId;
    } else if (sublistPressed) {
        QPoint origin = sublistsContainerLay->itemAt(0)->widget()->pos() + sublistsContainer->pos();
        int sublistWidth = sublist->width();
        int spacing = sublistsContainerLay->spacing();
        int width = (sublistWidth + spacing) / 2;
        int layoutPos = (e->position().x()) / width;

        layoutPos = (layoutPos % 2 ? layoutPos + 1 : layoutPos) / 2;
        if (layoutPos == sublistsContainerLay->indexOf(sublist) + 1) layoutPos--;
        if (layoutPos >= sublistsContainerLay->count()) layoutPos--;
        
        sublistsContainerLay->insertWidget(layoutPos, sublistsContainerLay->takeAt(sublistsContainerLay->indexOf(sublist))->widget());
        manager->setIndexSublist(id, sublistPressedId, layoutPos);

        sublist = nullptr;
        sublistPressedId = InvalidId;
        sublistPressed = false;
    }
}

void List::mouseMoveEvent(QMouseEvent *e)
{
    if (elementPressed) {
        QPointF fpos(e->position() - elementDragOffset);
        element->move(fpos.x(), fpos.y());
    } else if (sublistPressed) {
        sublist->move(e->position().x() - sublistDragOffset.x(), sublist->y());
    }
}