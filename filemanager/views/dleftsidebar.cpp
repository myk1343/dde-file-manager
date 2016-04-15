#include "dleftsidebar.h"
#include "dcheckablebutton.h"
#include "dhorizseparator.h"
#include "dscrollbar.h"
#include "windowmanager.h"

#include "../controllers/bookmarkmanager.h"
#include "../../deviceinfo/devicelistener.h"
#include "../../deviceinfo/deviceinfo.h"

#include "../app/global.h"
#include "../app/fmevent.h"
#include "../app/filesignalmanager.h"

#include "dhoverbutton.h"
#include "bmlistwidget.h"
#include "bmlistwidgetitem.h"
#include "dbookmarkitem.h"
#include "dbookmarkscene.h"
#include "dbookmarkitemgroup.h"
#include "dbookmarkrootitem.h"

#include <QVBoxLayout>
#include <QButtonGroup>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QListWidgetItem>
#include <QLabel>


DLeftSideBar::DLeftSideBar(QWidget *parent) : QFrame(parent)
{
    initData();
    initUI();
    initConnect();
}

DLeftSideBar::~DLeftSideBar()
{

}

void DLeftSideBar::initData()
{
    m_iconlist << ":/icons/images/icons/file_normal_16px.svg" //file
               << ":/icons/images/icons/recent_normal_16px.svg" //recent
               << ":/icons/images/icons/home_normal_16px.svg" //home
               << ":/icons/images/icons/desktop_normal_16px.svg" //desktop
               << ":/icons/images/icons/videos_normal_16px.svg" //video
               << ":/icons/images/icons/music_normal_16px.svg" //music
               << ":/icons/images/icons/pictures_normal_16px.svg" //picture
               << ":/icons/images/icons/documents_normal_16px.svg" //document
               << ":/icons/images/icons/download_normal_16px.svg" //download

               << ":/icons/images/icons/trash_normal_16px.svg" //trash
               << ":/icons/images/icons/disk_normal_16px.svg"; //disk

    m_bigIconlist << ":/icons/images/icons/file_normal_22px.svg" //file
                  << ":/icons/images/icons/recent_normal_22px.svg" //recent
                  << ":/icons/images/icons/home_normal_22px.svg" //home
                  << ":/icons/images/icons/desktop_normal_22px.svg" //desktop
                  << ":/icons/images/icons/videos_normal_22px.svg" //video
                  << ":/icons/images/icons/music_normal_22px.svg" //music
                  << ":/icons/images/icons/pictures_normal_22px.svg" //picture
                  << ":/icons/images/icons/documents_normal_22px.svg" //document
                  << ":/icons/images/icons/download_normal_22px.svg" //download

                  << ":/icons/images/icons/trash_normal_22px.svg" //trash
                  << ":/icons/images/icons/disk_normal_22px.svg" //disk
                  << ":/icons/images/icons/favourite_normal.svg"; //bookmarks

    m_bigIconlistChecked << ":/icons/images/icons/file_checked_22px.svg" //file
                         << ":/icons/images/icons/recent_checked_22px.svg" //recent
                         << ":/icons/images/icons/home_checked_22px.svg" //home
                         << ":/icons/images/icons/desktop_checked_22px.svg" //desktop
                         << ":/icons/images/icons/videos_checked_22px.svg" //video
                         << ":/icons/images/icons/music_checked_22px.svg" //music
                         << ":/icons/images/icons/pictures_checked_22px.svg" //picture
                         << ":/icons/images/icons/documents_checked_22px.svg" //document
                         << ":/icons/images/icons/download_checked_22px.svg" //download

                         << ":/icons/images/icons/trash_checked_22px.svg" //trash
                         << ":/icons/images/icons/disk_checked_22px.svg" //disk
                         << ":/icons/images/icons/favourite_checked.svg"; //bookmarks

    m_iconlistChecked << ":/icons/images/icons/file_checked_16px.svg" //file
               << ":/icons/images/icons/recent_checked_16px.svg" //recent
               << ":/icons/images/icons/home_checked_16px.svg" //home
               << ":/icons/images/icons/desktop_checked_16px.svg" //desktop
               << ":/icons/images/icons/videos_checked_16px.svg" //video
               << ":/icons/images/icons/music_checked_16px.svg" //music
               << ":/icons/images/icons/pictures_checked_16px.svg" //picture
               << ":/icons/images/icons/documents_checked_16px.svg" //document
               << ":/icons/images/icons/download_checked_16px.svg" //download

               << ":/icons/images/icons/trash_checked_16px.svg" //trash
               << ":/icons/images/icons/disk_checked_16px.svg"; //disk

    m_nameList << "File" << "Recent" << "Home"  << "Desktop"
               << "Videos" << "Musics" << "Pictures" << "Documents" << "Downloads"
               << "Trash" << "Disks" << "Bookmarks";
    m_navState = true;
    setAcceptDrops(true);
}

void DLeftSideBar::initUI()
{
    initTightNav();
    initNav();
    m_stackedWidget = new QStackedWidget;
    m_stackedWidget->addWidget(m_tightNav);
    m_stackedWidget->addWidget(m_nav);


    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addWidget(m_stackedWidget);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    setLayout(mainLayout);
    m_stackedWidget->setCurrentIndex(1);
    loadDevices();
    loadBookmark();
}

void DLeftSideBar::initConnect()
{
    connect(m_fileButton, &DCheckableButton::released, this, &DLeftSideBar::toTightNav);
    connect(m_tightNavFileButton, &DCheckableButton::released, this, &DLeftSideBar::toNormalNav);
    connect(m_tightScene->getGroup(), &DBookmarkItemGroup::url, this, &DLeftSideBar::handleLocationChanged);
    connect(m_scene->getGroup(), &DBookmarkItemGroup::url, this, &DLeftSideBar::handleLocationChanged);
    connect(m_scene, &DBookmarkScene::dragEntered, this, &DLeftSideBar::doDragEnter);
    connect(m_scene, &DBookmarkScene::dragLeft, this, &DLeftSideBar::doDragLeave);
    connect(m_tightScene, &DBookmarkScene::dragEntered, this, &DLeftSideBar::doDragEnter);
    connect(m_tightScene, &DBookmarkScene::dragLeft, this, &DLeftSideBar::doDragLeave);
    connect(m_scene, &DBookmarkScene::dropped, this, &DLeftSideBar::doDragLeave);
    connect(m_tightScene, &DBookmarkScene::dropped, this, &DLeftSideBar::doDragLeave);
}

void DLeftSideBar::initTightNav()
{
    m_tightNav = new QFrame(this);
    QHBoxLayout * fileButtonLayout = new QHBoxLayout;
    m_tightNavFileButton = new QPushButton("", this);
    m_tightNavFileButton->setObjectName("TightFileButton");
    m_tightNavFileButton->setToolTip("File");
    m_tightNavFileButton->setFixedSize(QSize(22, 22));
    m_tightNavFileButton->setFocusPolicy(Qt::NoFocus);
    fileButtonLayout->addWidget(m_tightNavFileButton);
    fileButtonLayout->setContentsMargins(0, 15, 0, 10);
    fileButtonLayout->setSpacing(0);

    QVBoxLayout * tightNavLayout = new QVBoxLayout;
    tightNavLayout->addLayout(fileButtonLayout);
    tightNavLayout->setContentsMargins(0, 0, 0, 0);
    tightNavLayout->setSpacing(0);
    m_tightNav->setLayout(tightNavLayout);

    QGraphicsView * m_view = new QGraphicsView;
    m_view->setAlignment(Qt::AlignTop);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setVerticalScrollBar(new DScrollBar);
    m_view->setObjectName("Bookmark");
    m_tightScene = new DBookmarkScene;
    m_tightScene->setSceneRect(0, 0, 60, 500);
    m_view->setScene(m_tightScene);

    for(int i = 1; i < m_bigIconlist.size(); i++)
    {
        DBookmarkItem * item = new DBookmarkItem;
        item->boundImageToHover(m_bigIconlistChecked.at(i));
        item->boundImageToPress(m_bigIconlistChecked.at(i));
        item->boundImageToRelease(m_bigIconlist.at(i));
        item->setUrl(getStandardPathbyId(i));
        item->setDefaultItem(true);
        m_tightScene->addItem(item);
    }
    tightNavLayout->addWidget(m_view);
}

void DLeftSideBar::initNav()
{
    m_nav = new QFrame;
    QVBoxLayout* navLayout = new QVBoxLayout;
    m_nav->setLayout(navLayout);
    //add file button
    QHBoxLayout * fileButtonLayout = new QHBoxLayout;
    QLabel * fileLabel = new QLabel(tr("File Manager"));
    fileLabel->setObjectName("FileLabel");
    m_fileButton = new QPushButton("");
    m_fileButton->setObjectName("FileButton");
    m_fileButton->setFixedSize(QSize(16,16));
    m_fileButton->setFocusPolicy(Qt::NoFocus);
    fileButtonLayout->addWidget(m_fileButton);
    fileButtonLayout->addWidget(fileLabel);
    fileButtonLayout->setContentsMargins(17.5, 15, 0, 10);
    fileButtonLayout->setSpacing(8);

    navLayout->addLayout(fileButtonLayout);
    navLayout->setSpacing(0);
    navLayout->setContentsMargins(0, 0, 0, 0);

    QGraphicsView * m_view = new QGraphicsView;
    m_view->setAcceptDrops(true);
    m_view->setVerticalScrollBar(new DScrollBar);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setObjectName("Bookmark");
    m_view->setAlignment(Qt::AlignTop);
    m_scene = new DBookmarkScene;
    m_scene->setSceneRect(0, 0, 200, 500);
    m_view->setScene(m_scene);

    for(int i = 1; i <= 9; i++)
    {
        DBookmarkItem * item = new DBookmarkItem;
        item->boundImageToHover(m_iconlistChecked.at(i));
        item->boundImageToPress(m_iconlistChecked.at(i));
        item->boundImageToRelease(m_iconlist.at(i));
        item->setText(m_nameList.at(i));
        item->setUrl(getStandardPathbyId(i));
        item->setDefaultItem(true);
        m_scene->addItem(item);
    }
    m_scene->addSeparator();
    for(int i = 10; i <m_iconlist.size(); i++)
    {
        DBookmarkItem * item = new DBookmarkItem;
        item->boundImageToHover(m_iconlistChecked.at(i));
        item->boundImageToPress(m_iconlistChecked.at(i));
        item->boundImageToRelease(m_iconlist.at(i));
        item->setText(m_nameList.at(i));
        item->setUrl(getStandardPathbyId(i));
        item->setDefaultItem(true);
        m_scene->addItem(item);
    }
    navLayout->addWidget(m_view);
}

DUrl DLeftSideBar::getStandardPathbyId(int id)
{
    DUrl url;

    switch (id) {
    case 1:
        url = DUrl::fromRecentFile("/");
        break;
    case 2:
        url = DUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::HomeLocation).at(0));
        break;
    case 3:
        url = DUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::DesktopLocation).at(0));
        break;
    case 4:
        url = DUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::MoviesLocation).at(0));
        break;
    case 5:
        url = DUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::MusicLocation).at(0));
        break;
    case 6:
        url = DUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).at(0));
        break;
    case 7:
        url = DUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).at(0));
        break;
    case 8:
        url = DUrl::fromLocalFile(QStandardPaths::standardLocations(QStandardPaths::DownloadLocation).at(0));
        break;
    case 9:
        url = DUrl::fromTrashFile("/");
        break;
    case 10:
        url = DUrl::fromComputerFile("/");
        break;
    case 11:
        url = DUrl::fromBookMarkFile("/");
        break;
    default:
        break;
    }

    return url;
}

void DLeftSideBar::resizeEvent(QResizeEvent *e)
{
    QRect rect = geometry();
    if(rect.width() < 70 && !m_isTight)
        toTightNav();
    else if(rect.width() > 70 && m_isTight)
        toNormalNav();
    QFrame::resizeEvent(e);
}

void DLeftSideBar::handleLocationChanged(const FMEvent &e)
{
    if(e.windowId() != WindowManager::getWindowId(window()))
        return;
    FMEvent event;

    event = e.fileUrl();
    event = FMEvent::FileView;
    event = WindowManager::getWindowId(window());

    emit fileSignalManager->requestChangeCurrentUrl(event);
}

void DLeftSideBar::toTightNav()
{
    m_stackedWidget->setCurrentIndex(0);
    m_isTight = true;
    emit moveSplitter(LEFTSIDEBAR_MIN, 1);
}

void DLeftSideBar::toNormalNav()
{
    qDebug() << "to normal";
    m_stackedWidget->setCurrentIndex(1);
    m_isTight = false;
    emit moveSplitter(LEFTSIDEBAR_NORMAL, 1);
}

void DLeftSideBar::doDragEnter()
{
    setStyleSheet("QFrame#LeftSideBar{\
                  background-color: transparent;\
                  border: 1px solid #2ca7f8\
              }");
}

void DLeftSideBar::doDragLeave()
{
    setStyleSheet("QFrame#LeftSideBar{\
                  background-color: transparent;\
                  border: 1px solid transparent\
    }");
}

void DLeftSideBar::loadBookmark()
{
    QList<BookMark *> m_list = bookmarkManager->getBookmarks();
    if(m_list.size())
        m_scene->addSeparator();
    for(int i = 0; i < m_list.size(); i++)
    {
        BookMark * bm = m_list.at(i);
        DBookmarkItem * item = new DBookmarkItem(bm);
        m_scene->addItem(item);
    }
}

void DLeftSideBar::loadDevices()
{
    deviceListener->update();
}




