#include "todolist.h"
#include "widgets/clickable.h"
#include "widgets/filters.h"
#include "tools/logger.h"
#include "tools/manager.h"

#include <QFile>
#include <QFontDatabase>
#include <QShortcut>

// Replace hasSidebar with a config? (object passed to the module or file)
ToDoList::ToDoList(/*bool hasSidebar, */QWidget *p)
    : QWidget(p)
{
    manager = new Manager();

    lay = new QHBoxLayout(this);
    scrollList = new QScrollArea(this);
    scrollContent = new QWidget(scrollList);
    scrollLay = new QVBoxLayout(scrollContent);
    listsContainer = new QWidget(scrollContent);
    listsLay = new QGridLayout(listsContainer);
    scrollTitle = new QLabel("Lists", scrollContent);
    newListButton = new QPushButton("Add List", this);
        
    scrollList->setWidget(scrollContent);
    scrollList->setWidgetResizable(true);
    scrollList->setAttribute(Qt::WA_StyledBackground, true);

    /*if (hasSidebar)
        createSidebar();*/ // TODO : sidebar

    scrollLay->addSpacing(10);
    scrollLay->addWidget(scrollTitle, 0, Qt::AlignHCenter);
    scrollLay->addWidget(listsContainer, 0, Qt::AlignHCenter);
    scrollLay->addStretch();
    scrollLay->setSpacing(20);
    auto jlists = manager->getLists();
    for (auto id : jlists) {
        createHomeList(id);

        List *list = new List(id, listsContainer);
        list->hide();
        lay->addWidget(list);
        lists.insert(id, list);

        QObject::connect(list, &List::back, this, &ToDoList::openHomePage);
        QObject::connect(list, &List::deleted, [this, id]() {
            homeLists[id]->deleteLater();
            homeLists.remove(id);
            homeListLabels.remove(id);
            lists.remove(id);
            openHomePage();
        });
        QObject::connect(list, &List::renamed, [this, id]() {
            homeListLabels[id]->setText(manager->getList(id).title);
        });
    }

    listsLay->setHorizontalSpacing(5);
    listsLay->setContentsMargins(0, 0, 0, 0);
    scrollList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollLay->setContentsMargins(10, 0, 10, 0);
    lay->addWidget(scrollList);
    lay->setContentsMargins(0, 0, 0, 0);
    
    newListButton->setFixedHeight(30);

    ResizeFilter *f = new ResizeFilter([this](QResizeEvent *e) {
        QSize s = e->size();
        newListButton->move(s.width() - newListButton->width() - 5, s.height() - newListButton->height() - 5);
        if (popup)
            popup->setFixedSize(s);

        updateLists(s.width());
    });
    installEventFilter(f);

    // TODO : Add shortcuts to README 
    QShortcut *addElementShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Plus), this, [this]() {
        if (!homeShown)
            listShown->newElement(); // TODO : not working
    });
    auto addLambda = [this]() {
        if (homeShown) {
            addList();
        } else {
            listShown->addEmptySublist();
        }
    };
    QShortcut *addShortcut = new QShortcut(QKeySequence(Qt::Key_Plus), this, addLambda);
    QShortcut *addCtrlShortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_A), this, addLambda);

    if (QFontDatabase::addApplicationFont(":/whitney.otf")) {
        logger.warn("Can't add application font : whitney");
    }

    scrollList->setProperty("class", "scroll-list");
    scrollContent->setProperty("class", "scroll-list-content");
    scrollTitle->setProperty("class", "scroll-list-title");
    newListButton->setProperty("class", "homepage-list-button");
    setProperty("class", "todolist");

    QFile style(":/style.css");
    style.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(style.readAll());
    setStyleSheet(styleSheet);

    setMinimumSize(800, 600);
    
    QObject::connect(newListButton, &QPushButton::clicked, this, &ToDoList::addList);

    QObject::connect(manager, &Manager::listViewed, [this](Id list, QDateTime timestamp) { /*TODO: sort homelists with timestamps*/ });
    QObject::connect(manager, &Manager::listRemoved, [this](Id list) {
        homeLists[list]->deleteLater();
        homeLists.remove(list);
        homeListLabels.remove(list);
        lists[list]->deleteLater();
        updateLists(scrollList->width());
    });
    QObject::connect(manager, &Manager::listReplaced, [this](Id list) {
        lists[list]->updateList();
        QList<Id> ids;
        for (auto list : lists)
            ids.append(list->id);
        homeListLabels[ids.indexOf(list)]->setText(manager->getList(list).title);
    });
    QObject::connect(manager, &Manager::listAdded, [this](Id listId) {
        createHomeList(listId);
        List *list = new List(listId, this);
        list->hide();
        lay->addWidget(list);
        lists.insert(listId, list);

        QObject::connect(list, &List::back, this, &ToDoList::openHomePage);
        QObject::connect(list, &List::deleted, [this, listId]() {
            homeLists[listId]->deleteLater();
            homeLists.remove(listId);
            homeListLabels.remove(listId);
            lists.remove(listId);
            openHomePage();
            updateLists(scrollList->width());
        });
        QObject::connect(list, &List::renamed, [this, list, listId]() { homeListLabels[listId]->setText(manager->getList(listId).title); });
    });

    QObject::connect(manager, &Manager::sublistIndexChanged, [this](Id list, Id sublist, size_t index) { lists[list]->changeSublistIndex(sublist, index); });
    QObject::connect(manager, &Manager::sublistRemoved, [this](Id list, Id sublist) { lists[list]->removeSublist(sublist); });
    QObject::connect(manager, &Manager::sublistReplaced, [this](Id list, Id sublist) { lists[list]->updateSublist(sublist); });
    QObject::connect(manager, &Manager::sublistAdded, [this](Id list, Id sublist) { lists[list]->addSublist(sublist); });

    QObject::connect(manager, &Manager::elementIndexChanged, [this](Id list, Id sublist, Id element, size_t index) { lists[list]->changeElementIndex(sublist, element, index); });
    QObject::connect(manager, &Manager::elementRemoved, [this](Id list, Id sublist, Id element) { lists[list]->removeElement(sublist, element); });
    QObject::connect(manager, &Manager::elementReplaced, [this](Id list, Id sublist, Id element) { lists[list]->updateElement(sublist, element); });
    QObject::connect(manager, &Manager::elementAdded, [this](Id list, Id sublist, Id element) { lists[list]->addElement(sublist, element); });
}

// id, homelist timestamp

void ToDoList::addList()
{
    if (popup)
        popup->deleteLater();
    QList<QStringPair> prompts;
    prompts.append(QStringPair("Name", QString()));
    popup = new PopUp(prompts, "Name the list", this);
    QObject::connect(popup, &PopUp::result, [this](bool cancelled, QStringList prompts) {
        if (!cancelled && !prompts.isEmpty() && !prompts[0].isEmpty()) {
            size_t i = homeLists.size();
            Id id = manager->addList(JList(prompts[0], JList::Default, 0));
            createHomeList(i);
            
            List *list = new List(i, this);
            list->hide();
            lay->addWidget(list);
            lists.insert(id, list);

            QObject::connect(list, &List::back, this, &ToDoList::openHomePage);
            QObject::connect(list, &List::deleted, [this, list, id]() {
                homeLists[id]->deleteLater();
                homeLists.remove(id);
                homeListLabels.remove(id);
                lists.remove(id);
                openHomePage();
                updateLists(scrollList->width());
            });
            QObject::connect(list, &List::renamed, [this, list, id]() {
                homeListLabels[id]->setText(manager->getList(id).title);
            });
        }
        popup->deleteLater();
        popup = nullptr;
    });
}

void ToDoList::createHomeList(Id id)
{
    Clickable *container = new Clickable([this, id](QMouseEvent *e) { openList(id); }, listsContainer);
    QHBoxLayout *containerLay = new QHBoxLayout(container);
    JList list = manager->getList(id);
    QLabel *listTitle = new QLabel(list.title, container);

    containerLay->addStretch();
    containerLay->addWidget(listTitle, 0, Qt::AlignCenter);
    containerLay->addStretch();

    homeLists.insert(id, container);
    homeListLabels.insert(id, listTitle);
    updateLists(scrollList->width());

    container->setFixedSize(200, 200);
    container->setProperty("class", "scroll-list-element");
    listTitle->setProperty("class", "scroll-list-element-title");
}

void ToDoList::createSidebar()
{
    sidebar = new QWidget(this);
    sidebarLay = new QVBoxLayout(sidebar);
    header = new QWidget(sidebar);
    headerLay = new QHBoxLayout(header);
    sbHider = new QPushButton(header);
    title = new QLabel("To-Do List", header);
    sbContent = new QScrollArea(sidebar);
    sbContainer = new QWidget(sbContent);
    sbContainerLay = new QVBoxLayout(sbContainer);

    sbHider->setProperty("class", "sidebar-hider-opened");
    QObject::connect(sbHider, &QPushButton::clicked, [this]() {
        sbOpen = !sbOpen;
        sbHider->setProperty("class", QString("sidebar-hider-") + (sbOpen ? "opened" : "closed"));
        if (sbOpen) {
            title->show();
            sbContent->show();
        } else {
            title->hide();
            sbContent->hide();
        }
    });

    headerLay->addWidget(title);
    headerLay->addWidget(sbHider);
    sidebarLay->addWidget(header);
    header->setProperty("class", "sidebar-header");

    std::vector<std::function<void()>> recentListsHandlers;
    auto recents = manager->getRecents();
    for (Id i = 0 ; i < (Id)recents.size() ; i++) {
        recentListsHandlers.push_back([this, i]() { openList(i); });
    }
    SidebarTab *recentLists = new SidebarTab("Recent lists", recents, recentListsHandlers, sbContent);
    sbContainerLay->addWidget(recentLists);
    sbContainerLay->addStretch();
    
    sbContent->setWidget(sbContainer);
    sbContent->setWidgetResizable(true);

    sidebarLay->addWidget(sbContent);
    sidebarLay->addStretch();

    sidebar->setProperty("class", "sidebar");
    lay->addWidget(sidebar);
}


void ToDoList::openList(Id id)
{
    QList<Id> ids;
    for (auto list : lists)
        ids.append(list->id);
    scrollList->hide();
    for (auto list : lists) 
        list->hide();
    listShown = lists[ids.indexOf(id)]; 
    listShown->show();
    homeShown = false;

    manager->viewList(id);
}

void ToDoList::openHomePage()
{
    for (auto list : lists) 
        list->hide();
    
    scrollList->show();
    updateLists(scrollList->width());
    homeShown = true;
    listShown = nullptr;
}

void ToDoList::updateLists(int w)
{
    for (int i = 0 ; i < listsLay->count() ; i++)
        listsLay->removeItem(listsLay->itemAt(i));
    int r = 0, c = 0;
    for (auto list : homeLists) {
        listsLay->addWidget(list, r, c);
        c++;
        int threshold = (w - 5*c - 2*10 - (sidebar ? sidebar->width() : 0)) / 200;
        if (threshold < c+1) {
            c = 0;
            r++;
        }
    }
}

void ToDoList::showEvent(QShowEvent *e)
{
    updateLists(scrollList->width());
}

void ToDoList::closeEvent(QCloseEvent *)
{
    manager->~Manager();
}