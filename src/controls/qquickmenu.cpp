/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickmenu_p.h"
#include "qquickmenubar_p.h"
#include "qquickmenuitemcontainer_p.h"
#include "qquickmenupopupwindow_p.h"

#include <qdebug.h>
#include <qabstractitemmodel.h>
#include <qcursor.h>
#include <private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformtheme.h>
#include <QtGui/qpa/qplatformmenu.h>
#include <qquickitem.h>
#include <QtQuick/QQuickRenderControl>

QT_BEGIN_NAMESPACE

/*!
  \class QQuickMenu
  \internal
 */

/*!
  \qmltype MenuPrivate
  \instantiates QQuickMenu
  \internal
  \inqmlmodule QtQuick.Controls
 */

/*!
    \qmlproperty list<Object> Menu::items
    \default

    The list of items in the menu.

    \l Menu only accepts objects of type \l Menu, \l MenuItem, and \l MenuSeparator
    as children. It also supports \l Instantiator objects as long as the insertion is
    being done manually using \l insertItem().

    \code
    Menu {
        id: recentFilesMenu

        Instantiator {
            model: recentFilesModel
            MenuItem {
                text: model.fileName
            }
            onObjectAdded: recentFilesMenu.insertItem(index, object)
            onObjectRemoved: recentFilesMenu.removeItem(object)
        }

        MenuSeparator {
            visible: recentFilesModel.count > 0
        }

        MenuItem {
            text: "Clear menu"
            enabled: recentFilesModel.count > 0
            onTriggered: recentFilesModel.clear()
        }
    \endcode

    Note that in this case, the \c index parameter passed to \l insertItem() is relative
    to the position of the \l Instantiator in the menu, as opposed to absolute position
    in the menu.

    \sa MenuItem, MenuSeparator
*/

/*!
    \qmlproperty bool Menu::visible

    Whether the menu should be visible as a submenu of another Menu, or as a menu on a MenuBar.
    Its value defaults to \c true.

    \note This has nothing to do with the actual menu pop-up window being visible. Use
    \l aboutToShow() and \l aboutToHide() if you need to know when the pop-up window will
    be shown or hidden.
*/

/*!
    \qmlproperty enumeration Menu::type

    This property is read-only and constant, and its value is \l {QtQuick.Controls::MenuItem::}{type}.
*/

/*!
    \qmlproperty string Menu::title

    Title for the menu as a submenu or in a menubar.

    Mnemonics are supported by prefixing the shortcut letter with \&.
    For instance, \c "\&File" will bind the \c Alt-F shortcut to the
    \c "File" menu. Note that not all platforms support mnemonics.

    Its value defaults to the empty string.
*/

/*!
    \qmlproperty bool Menu::enabled

    Whether the menu is enabled, and responsive to user interaction as a submenu.
    Its value defaults to \c true.
*/

/*!
    \qmlproperty url Menu::iconSource

    Sets the icon file or resource url for the menu icon as a submenu.
    Defaults to the empty URL.

    \sa iconName
*/

/*!
    \qmlproperty string Menu::iconName

    Sets the icon name for the menu icon. This will pick the icon
    with the given name from the current theme. Only works as a submenu.

    Its value defaults to the empty string.

    \sa iconSource
*/

/*!
    \qmlmethod void Menu::popup()

    Opens this menu under the mouse cursor.
    It can block on some platforms, so test it accordingly.
*/

/*!
    \qmlmethod MenuItem Menu::addItem(string text)

    Adds an item to the menu. Returns the newly created \l MenuItem.

    \sa insertItem()
*/

/*!
    \qmlmethod MenuItem Menu::insertItem(int before, string title)

    Creates and inserts an item with title \c title at the index \c before in the current menu.
    Returns the newly created \l MenuItem.

    \sa addItem()
*/

/*!
    \qmlmethod void Menu::addSeparator()

    Adds a separator to the menu.

    \sa insertSeparator()
*/

/*!
    \qmlmethod void Menu::insertSeparator(int before)

    Creates and inserts a separator at the index \c before in the current menu.

    \sa addSeparator()
*/

/*!
    \qmlmethod Menu Menu::addMenu(string title)
    Adds a submenu to the menu. Returns the newly created \l Menu.

    \sa insertMenu()
*/

/*!
    \qmlmethod MenuItem Menu::insertMenu(int before, string title)

    Creates and inserts a submenu with title \c title at the index \c before in the current menu.
    Returns the newly created \l Menu.

    \sa addMenu()
*/

/*!
    \qmlmethod void Menu::insertItem(int before, object item)

    Inserts the \c item at the index \c before in the current menu.
    In this case, \c item can be either a \l MenuItem, a \l MenuSeparator,
    or a \l Menu.

    \sa removeItem()
*/

/*!
    \qmlmethod void Menu::removeItem(item)

    Removes the \c item from the menu.
    In this case, \c item can be either a \l MenuItem, a \l MenuSeparator,
    or a \l Menu.

    \sa insertItem()
*/


/*!
    \qmlsignal Menu::aboutToShow()
    \since QtQuick.Controls 1.4

    This signal is emitted just before the menu is shown to the user.

    \sa aboutToHide()
*/

/*!
    \qmlsignal Menu::aboutToHide()
    \since QtQuick.Controls 1.4

    This signal is emitted just before the menu is hidden from the user.

    \sa aboutToShow()
*/

QQuickMenu::QQuickMenu(QObject *parent)
    : QQuickMenuText(parent, QQuickMenuItemType::Menu),
      m_itemsCount(0),
      m_selectedIndex(-1),
      m_parentWindow(0),
      m_minimumWidth(0),
      m_popupWindow(0),
      m_menuContentItem(0),
      m_popupVisible(false),
      m_containersCount(0),
      m_xOffset(0),
      m_yOffset(0),
      m_triggerCount(0),
      m_proxy(false)
{
    connect(this, SIGNAL(__textChanged()), this, SIGNAL(titleChanged()));

    m_platformMenu = QGuiApplicationPrivate::platformTheme()->createPlatformMenu();
    if (m_platformMenu) {
        connect(m_platformMenu, SIGNAL(aboutToShow()), this, SIGNAL(aboutToShow()));
        connect(m_platformMenu, SIGNAL(aboutToHide()), this, SLOT(hideMenu()));
        if (platformItem())
            platformItem()->setMenu(m_platformMenu);
    }
    if (const QFont *font = QGuiApplicationPrivate::platformTheme()->font(QPlatformTheme::MenuItemFont))
        m_font = *const_cast<QFont*>(font);
}

QQuickMenu::~QQuickMenu()
{
    while (!m_menuItems.empty()) {
        QQuickMenuBase *item = m_menuItems.takeFirst();
        if (item)
            item->setParentMenu(0);
    }

    if (platformItem())
        platformItem()->setMenu(0);

    delete m_platformMenu;
    m_platformMenu = 0;
}

void QQuickMenu::syncParentMenuBar()
{
    QQuickMenuBar *menubar = qobject_cast<QQuickMenuBar *>(parent());
    if (menubar && menubar->platformMenuBar())
        menubar->platformMenuBar()->syncMenu(m_platformMenu);
}

void QQuickMenu::setVisible(bool v)
{
    QQuickMenuBase::setVisible(v);
    if (m_platformMenu) {
        m_platformMenu->setVisible(v);
        syncParentMenuBar();
    }
}

void QQuickMenu::setEnabled(bool e)
{
    QQuickMenuText::setEnabled(e);
    if (m_platformMenu) {
        m_platformMenu->setEnabled(e);
        syncParentMenuBar();
    }
}

void QQuickMenu::updateText()
{
    if (m_platformMenu)
        m_platformMenu->setText(this->text());
    QQuickMenuText::updateText();
}

void QQuickMenu::setMinimumWidth(int w)
{
    if (w == m_minimumWidth)
        return;

    m_minimumWidth = w;
    if (m_platformMenu)
        m_platformMenu->setMinimumWidth(w);

    emit minimumWidthChanged();
}

void QQuickMenu::setFont(const QFont &arg)
{
    if (arg == m_font)
        return;

    m_font = arg;
    if (m_platformMenu)
        m_platformMenu->setFont(arg);
}

void QQuickMenu::setXOffset(qreal x)
{
    m_xOffset = x;
}

void QQuickMenu::setYOffset(qreal y)
{
    m_yOffset = y;
}

void QQuickMenu::setSelectedIndex(int index)
{
    if (m_selectedIndex == index)
        return;

    m_selectedIndex = index;
    emit __selectedIndexChanged();
}

void QQuickMenu::updateSelectedIndex()
{
    if (QQuickMenuItem *menuItem = qobject_cast<QQuickMenuItem*>(sender())) {
        int index = indexOfMenuItem(menuItem);
        setSelectedIndex(index);
    }
}

QQuickMenuItems QQuickMenu::menuItems()
{
    return QQuickMenuItems(this, 0, &QQuickMenu::append_menuItems, &QQuickMenu::count_menuItems,
                       &QQuickMenu::at_menuItems, &QQuickMenu::clear_menuItems);
}

QQuickWindow *QQuickMenu::findParentWindow()
{
    if (!m_parentWindow) {
        QQuickItem *parentAsItem = qobject_cast<QQuickItem *>(parent());
        m_parentWindow = visualItem() ? visualItem()->window() :    // Menu as menu item case
                         parentAsItem ? parentAsItem->window() : 0; //Menu as context menu/popup case
    }
    return m_parentWindow;
}

void QQuickMenu::popup()
{
    QQuickWindow *quickWindow = findParentWindow();
    QPoint renderOffset;
    QWindow *renderWindow = QQuickRenderControl::renderWindowFor(quickWindow, &renderOffset);
    QWindow *parentWindow = renderWindow ? renderWindow : quickWindow;
    QScreen *screen = parentWindow ? parentWindow->screen() : qGuiApp->primaryScreen();
    QPoint mousePos = QCursor::pos(screen);

    if (mousePos.x() == int(qInf())) {
        // ### fixme: no mouse pos registered. Get pos from touch...
        mousePos = screen->availableGeometry().center();
    }

    if (parentWindow)
        mousePos = parentWindow->mapFromGlobal(mousePos);

    __popup(QRectF(mousePos.x() - renderOffset.x(), mousePos.y() - renderOffset.y(), 0, 0));
}

void QQuickMenu::__popup(const QRectF &targetRect, int atItemIndex, MenuType menuType)
{
    if (popupVisible()) {
        hideMenu();
        // Mac and Windows would normally move the menu under the cursor, so we should not
        // return here. However, very often we want to re-contextualize the menu, and this
        // has to be done at the application level.
        return;
    }

    setPopupVisible(true);

    QQuickMenuBase *atItem = menuItemAtIndex(atItemIndex);

    QQuickWindow *quickWindow = findParentWindow();
    QPoint renderOffset;
    QWindow *renderWindow = QQuickRenderControl::renderWindowFor(quickWindow, &renderOffset);
    QWindow *parentWindow = renderWindow ? renderWindow : quickWindow;
    // parentWindow may not be a QQuickWindow (happens when using QQuickWidget)

    if (m_platformMenu) {
        QRectF globalTargetRect = targetRect.translated(m_xOffset, m_yOffset);
        if (visualItem()) {
            if (qGuiApp->isRightToLeft()) {
                qreal w = qMax(static_cast<qreal>(m_minimumWidth), m_menuContentItem->width());
                globalTargetRect.moveLeft(w - targetRect.x() - targetRect.width());
            }
            globalTargetRect = visualItem()->mapRectToScene(globalTargetRect);
        }
        globalTargetRect.translate(renderOffset);
        m_platformMenu->setMenuType(QPlatformMenu::MenuType(menuType));
        m_platformMenu->showPopup(parentWindow, globalTargetRect.toRect(), atItem ? atItem->platformItem() : 0);
    } else {
        m_popupWindow = new QQuickMenuPopupWindow(this);
        if (visualItem())
            m_popupWindow->setParentItem(visualItem());
        else
            m_popupWindow->setParentWindow(parentWindow, quickWindow);
        m_popupWindow->setPopupContentItem(m_menuContentItem);
        m_popupWindow->setItemAt(atItem ? atItem->visualItem() : 0);

        connect(m_popupWindow, SIGNAL(visibleChanged(bool)), this, SLOT(windowVisibleChanged(bool)));
        connect(m_popupWindow, SIGNAL(geometryChanged()), this, SIGNAL(__popupGeometryChanged()));
        connect(m_popupWindow, SIGNAL(willBeDeletedLater()), this, SLOT(clearPopupWindow()));

        m_popupWindow->setPosition(targetRect.x() + m_xOffset + renderOffset.x(),
                                   targetRect.y() + targetRect.height() + m_yOffset + renderOffset.y());
        emit aboutToShow();
        m_popupWindow->show();
    }
}

void QQuickMenu::setMenuContentItem(QQuickItem *item)
{
    if (m_menuContentItem != item) {
        m_menuContentItem = item;
        emit menuContentItemChanged();
    }
}

void QQuickMenu::setPopupVisible(bool v)
{
    if (m_popupVisible != v) {
        m_popupVisible = v;
        emit popupVisibleChanged();
    }
}

QRect QQuickMenu::popupGeometry() const
{
    if (!m_popupWindow || !m_popupVisible)
        return QRect();

    return m_popupWindow->geometry();
}

void QQuickMenu::prepareItemTrigger(QQuickMenuItem *)
{
    m_triggerCount++;
    __dismissMenu();
}

void QQuickMenu::concludeItemTrigger(QQuickMenuItem *)
{
    if (--m_triggerCount == 0)
        destroyAllMenuPopups();
}

/*!
 * \internal
 * Close this menu's popup window. Emits aboutToHide and sets __popupVisible to false.
 */
void QQuickMenu::hideMenu()
{
    if (m_popupVisible) {
        emit aboutToHide();
        setPopupVisible(false);
    }
    if (m_popupWindow && m_popupWindow->isVisible())
        m_popupWindow->hide();
    m_parentWindow = 0;
}

QQuickMenuPopupWindow *QQuickMenu::topMenuPopup() const
{
    QQuickMenuPopupWindow *topMenuWindow = m_popupWindow;
    while (topMenuWindow) {
        QQuickMenuPopupWindow *pw = qobject_cast<QQuickMenuPopupWindow *>(topMenuWindow->transientParent());
        if (!pw)
            return topMenuWindow;
        topMenuWindow = pw;
    }

    return 0;
}

/*!
 * \internal
 * Dismiss all the menus this menu is attached to, bottom-up.
 * In QQuickPopupWindow, before closing, dismissPopup() emits popupDismissed()
 * which is connected to dismissPopup() on any child popup.
 */
void QQuickMenu::__dismissMenu()
{
    if (m_platformMenu) {
        m_platformMenu->dismiss();
    } else if (QQuickMenuPopupWindow *topPopup = topMenuPopup()) {
        topPopup->dismissPopup();
    }
}

/*!
 * \internal
 * Called when the popup window visible property changes.
 */
void QQuickMenu::windowVisibleChanged(bool v)
{
    if (!v) {
        if (m_popupWindow) {
            QQuickMenuPopupWindow *parentMenuPopup = qobject_cast<QQuickMenuPopupWindow *>(m_popupWindow->transientParent());
            if (parentMenuPopup) {
                parentMenuPopup->setMouseGrabEnabled(true);
                parentMenuPopup->setKeyboardGrabEnabled(true);
            }
        }
        if (m_popupVisible)
            __closeAndDestroy();
    }
}

void QQuickMenu::clearPopupWindow()
{
    m_popupWindow = 0;
    emit __menuPopupDestroyed();
}

void QQuickMenu::destroyMenuPopup()
{
    if (m_triggerCount > 0)
        return;
    if (m_popupWindow)
        m_popupWindow->setToBeDeletedLater();
}

void QQuickMenu::destroyAllMenuPopups() {
    if (m_triggerCount > 0)
        return;
    QQuickMenuPopupWindow *popup = topMenuPopup();
    if (popup)
        popup->setToBeDeletedLater();
}

void QQuickMenu::__closeAndDestroy()
{
    hideMenu();
    destroyMenuPopup();
}

void QQuickMenu::__dismissAndDestroy()
{
    if (m_platformMenu)
        return;

    __dismissMenu();
    destroyAllMenuPopups();
}

void QQuickMenu::itemIndexToListIndex(int itemIndex, int *listIndex, int *containerIndex) const
{
    *listIndex = -1;
    QQuickMenuItemContainer *container = 0;
    while (itemIndex >= 0 && ++*listIndex < m_menuItems.count())
        if ((container = qobject_cast<QQuickMenuItemContainer *>(m_menuItems[*listIndex])))
            itemIndex -= container->items().count();
        else
            --itemIndex;

    if (container)
        *containerIndex = container->items().count() + itemIndex;
    else
        *containerIndex = -1;
}

int QQuickMenu::itemIndexForListIndex(int listIndex) const
{
    int index = 0;
    int i = 0;
    while (i < listIndex && i < m_menuItems.count())
        if (QQuickMenuItemContainer *container = qobject_cast<QQuickMenuItemContainer *>(m_menuItems[i++]))
            index += container->items().count();
        else
            ++index;

    return index;
}

QQuickMenuBase *QQuickMenu::nextMenuItem(QQuickMenu::MenuItemIterator *it) const
{
    if (it->containerIndex != -1) {
        QQuickMenuItemContainer *container = qobject_cast<QQuickMenuItemContainer *>(m_menuItems[it->index]);
        if (++it->containerIndex < container->items().count())
            return container->items()[it->containerIndex];
    }

    if (++it->index < m_menuItems.count()) {
        if (QQuickMenuItemContainer *container = qobject_cast<QQuickMenuItemContainer *>(m_menuItems[it->index])) {
            it->containerIndex = 0;
            return container->items()[0];
        } else {
            it->containerIndex = -1;
            return m_menuItems[it->index];
        }
    }

    return 0;
}

QQuickMenuBase *QQuickMenu::menuItemAtIndex(int index) const
{
    if (0 <= index && index < m_itemsCount) {
        if (!m_containersCount) {
            return m_menuItems[index];
        } else if (m_containersCount == 1 && m_menuItems.count() == 1) {
            QQuickMenuItemContainer *container = qobject_cast<QQuickMenuItemContainer *>(m_menuItems[0]);
            return container->items()[index];
        } else {
            int containerIndex;
            int i;
            itemIndexToListIndex(index, &i, &containerIndex);
            if (containerIndex != -1) {
                QQuickMenuItemContainer *container = qobject_cast<QQuickMenuItemContainer *>(m_menuItems[i]);
                return container->items()[containerIndex];
            } else {
                return m_menuItems[i];
            }
        }
    }

    return 0;
}

bool QQuickMenu::contains(QQuickMenuBase *item)
{
    if (item->container())
        return item->container()->items().contains(item);

    return m_menuItems.contains(item);
}

int QQuickMenu::indexOfMenuItem(QQuickMenuBase *item) const
{
    if (!item)
        return -1;
    if (item->container()) {
        int containerIndex = m_menuItems.indexOf(item->container());
        if (containerIndex == -1)
            return -1;
        int index = item->container()->items().indexOf(item);
        return index == -1 ? -1 : itemIndexForListIndex(containerIndex) + index;
    } else {
        int index = m_menuItems.indexOf(item);
        return index == -1 ? -1 : itemIndexForListIndex(index);
    }
}

QQuickMenuItem *QQuickMenu::addItem(const QString &title)
{
    return insertItem(m_itemsCount, title);
}

QQuickMenuItem *QQuickMenu::insertItem(int index, const QString &title)
{
    QQuickMenuItem *item = new QQuickMenuItem(this);
    item->setText(title);
    insertItem(index, item);
    return item;
}

void QQuickMenu::addSeparator()
{
    insertSeparator(m_itemsCount);
}

void QQuickMenu::insertSeparator(int index)
{
    QQuickMenuSeparator *item = new QQuickMenuSeparator(this);
    insertItem(index, item);
}

void QQuickMenu::insertItem(int index, QQuickMenuBase *menuItem)
{
    if (!menuItem)
        return;
    int itemIndex;
    if (m_containersCount) {
        QQuickMenuItemContainer *container = menuItem->parent() != this ? m_containers[menuItem->parent()] : 0;
        if (container) {
            container->insertItem(index, menuItem);
            itemIndex = itemIndexForListIndex(m_menuItems.indexOf(container)) + index;
        } else {
            itemIndex = itemIndexForListIndex(index);
            m_menuItems.insert(itemIndex, menuItem);
        }
    } else {
        itemIndex = index;
        m_menuItems.insert(index, menuItem);
    }

    setupMenuItem(menuItem, itemIndex);
    emit itemsChanged();
}

void QQuickMenu::removeItem(QQuickMenuBase *menuItem)
{
    if (!menuItem)
        return;
    menuItem->setParentMenu(0);

    QQuickMenuItemContainer *container = menuItem->parent() != this ? m_containers[menuItem->parent()] : 0;
    if (container)
        container->removeItem(menuItem);
    else
        m_menuItems.removeOne(menuItem);

    --m_itemsCount;
    emit itemsChanged();
}

void QQuickMenu::clear()
{
    m_containers.clear();
    m_containersCount = 0;

    // QTBUG-48927: a proxy menu (ApplicationWindowStyle.qml) must not
    // delete its items, because they are owned by the menubar
    if (m_proxy)
        m_menuItems.clear();

    while (!m_menuItems.empty())
        delete m_menuItems.takeFirst();
    m_itemsCount = 0;
}

void QQuickMenu::setupMenuItem(QQuickMenuBase *item, int platformIndex)
{
    item->setParentMenu(this);
    if (m_platformMenu) {
        QPlatformMenuItem *before = 0;
        if (platformIndex != -1)
            before = m_platformMenu->menuItemAt(platformIndex);
        m_platformMenu->insertMenuItem(item->platformItem(), before);
    }
    ++m_itemsCount;
}

void QQuickMenu::append_menuItems(QQuickMenuItems *list, QObject *o)
{
    if (QQuickMenu *menu = qobject_cast<QQuickMenu *>(list->object)) {
        if (QQuickMenuBase *menuItem = qobject_cast<QQuickMenuBase *>(o)) {
            menu->m_menuItems.append(menuItem);
            menu->setupMenuItem(menuItem);
        } else {
            QQuickMenuItemContainer *menuItemContainer = new QQuickMenuItemContainer(menu);
            menu->m_menuItems.append(menuItemContainer);
            menu->m_containers.insert(o, menuItemContainer);
            menuItemContainer->setParentMenu(menu);
            ++menu->m_containersCount;
            foreach (QObject *child, o->children()) {
                if (QQuickMenuBase *item = qobject_cast<QQuickMenuBase *>(child)) {
                    menuItemContainer->insertItem(-1, item);
                    menu->setupMenuItem(item);
                }
            }
        }
    }
}

int QQuickMenu::count_menuItems(QQuickMenuItems *list)
{
    if (QQuickMenu *menu = qobject_cast<QQuickMenu *>(list->object))
        return menu->m_itemsCount;

    return 0;
}

QObject *QQuickMenu::at_menuItems(QQuickMenuItems *list, int index)
{
    if (QQuickMenu *menu = qobject_cast<QQuickMenu *>(list->object))
        return menu->menuItemAtIndex(index);

    return 0;
}

void QQuickMenu::clear_menuItems(QQuickMenuItems *list)
{
    if (QQuickMenu *menu = qobject_cast<QQuickMenu *>(list->object))
        menu->clear();
}

QT_END_NAMESPACE
