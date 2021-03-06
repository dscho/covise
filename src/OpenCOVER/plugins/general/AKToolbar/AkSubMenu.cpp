/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// CLASS AkSubMenu
//
// This class represents a menu in the new menu system
//
// Initial version: 2003-06-23 [we]
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// (C) 2001 by VirCinity IT Consulting
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Changes:
//

#include "AkSubMenu.h"
#include "AKToolbar.h"

#include <assert.h>

#include <OpenVRUI/coMenuItem.h>
#include <OpenVRUI/coLabelSubMenuToolboxItem.h>
#include <OpenVRUI/coRowMenu.h>
#include <OpenVRUI/coToolboxMenu.h>
#include <OpenVRUI/coSubMenuItem.h>
#include <OpenVRUI/coButtonMenuItem.h>
#include <cover/coVRPluginSupport.h>
#include <OpenVRUI/coCheckboxMenuItem.h>
#include <OpenVRUI/coSliderMenuItem.h>
#include <OpenVRUI/coSliderToolboxItem.h>
#include <OpenVRUI/coPotiMenuItem.h>
#include <OpenVRUI/coPotiToolboxItem.h>
#include <OpenVRUI/coLabel.h>
#include <PluginUtil/coColorBar.h>
#include <cover/coVRMSController.h>
#include <config/CoviseConfig.h>

using covise::coCoviseConfig;

//FIXME
// // called before a frame is drawn
// int markStencil(pfTraverser *, void *)
// {
//    // draw only where stencil bit 1 is not set to 1
//    glStencilFunc(GL_ALWAYS,1,1);
//
//    // and don't touch the buffer
//    glStencilOp(GL_REPLACE,GL_REPLACE,GL_REPLACE);
//
//    return  PFTRAV_CONT;
// }
//
//
// // called before a frame is drawn
// int dontMarkStencil(pfTraverser *, void *)
// {
//    // draw only where stencil bit 1 is not set to 1
//    glStencilFunc(GL_NOTEQUAL,1,1);
//
//    // and don't touch the buffer
//    glStencilOp(GL_KEEP,GL_KEEP,GL_KEEP);
//
//    return  PFTRAV_CONT;
// }
//
//
// // stop isect traversal
// int stopTraversal(pfTraverser *, void *)
// {
//    return  PFTRAV_PRUNE;
// }

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ++  Static Variable initializers
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

const bool AkSubMenu::autoClose_ = coCoviseConfig::isOn("COVER.Plugin.AKToolbar.AutoClose", true);

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ++  Constructor: construct empty menu
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

AkSubMenu::AkSubMenu(const char *name, AkSubMenu *triggerMenu,
                     coButtonMenuItem *triggerItem, coToolboxMenu *toolbar)
{
    if (AKToolbar::debugLevel(2) && coVRMSController::instance()->isMaster())
        fprintf(stderr, "AkSubMenu::AkSubMenu(name=\"%s\", triggerMenu=\"%s\", triggerItem=\"%s\", toolbar)\n",
                name,
                (triggerMenu ? triggerMenu->getName() : "NULL"),
                (triggerItem ? triggerItem->getName() : "NULL"));

    triggerMenu_ = triggerMenu;
    triggerItem_ = triggerItem;
    toolbar_ = toolbar;
    name_ = name;

    //FIXME
    //    cover->getMenuGroup()->setTravFuncs(PFTRAV_DRAW,markStencil,dontMarkStencil);
    //    cover->getObjectsRoot()->setTravFuncs(PFTRAV_ISECT,stopTraversal,NULL);

    // the menu we use
    theMenu_ = new coRowMenu(name, triggerMenu ? triggerMenu->getMenu() : NULL);
    theMenu_->setMenuListener(this); /// this is a FOCUS listener

    // our own toolbar button
    toolbarButton_ = new coLabelSubMenuToolboxItem(name);
    toolbarButton_->setMenuListener(this); /// this is an Interaction listener

    // its "twin" for the RowMenu
    toolbarButtonTwin_ = new coSubMenuItem(name);
    toolbarButtonTwin_->setMenu(theMenu_);
    toolbarButtonTwin_->setTwin(toolbarButton_);

    // make sure we get the button events of the start button
    if (triggerItem_)
    {
        triggerItem_->setMenuListener(this);
        if (AKToolbar::debugLevel(3) && coVRMSController::instance()->isMaster())
        {
            cerr << "Set MenuListener of Trigger \"" << triggerItem_->getName()
                 << "\" to menu \"" << name_ << "\"" << endl;
        }
    }
    else if (AKToolbar::debugLevel(1) && coVRMSController::instance()->isMaster())
    {
        cerr << "NULL triggerItem_ given to \"" << name_ << "\"" << endl;
    }
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ++  Constructor: Clone existing menu
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

AkSubMenu::AkSubMenu(coMenu *cloneMenu, AkSubMenu *triggerMenu,
                     coButtonMenuItem *triggerItem, coToolboxMenu *toolbar)
    : triggerMenu_(triggerMenu)
    , triggerItem_(triggerItem)
    , toolbar_(toolbar)
{
    const char *name = cloneMenu->getName();
    if (!name)
        abort();
    name_ = name;
    if (AKToolbar::debugLevel(2) && coVRMSController::instance()->isMaster())
        fprintf(stderr, "AkSubMenu::AkSubMenu(cloneMenu=\"%s\", \"%s\", \"%s\", toolbar)\n",
                name,
                (triggerMenu ? triggerMenu->getName() : "NULL"),
                (triggerItem ? triggerItem->getName() : "NULL"));
    // the menu we use
    theMenu_ = new coRowMenu(name, triggerMenu ? triggerMenu->getMenu() : NULL);
    theMenu_->setMenuListener(this);

    // sort it? Not implemented in coMenu
    //theMenu_->setSorting(cloneMenu->isSorting());

    // our own toolbar button
    toolbarButton_ = new coLabelSubMenuToolboxItem(name);
    toolbarButton_->setMenuListener(this);

    // its "twin" for the RowMenu
    toolbarButtonTwin_ = new coSubMenuItem(name);
    toolbarButtonTwin_->setMenu(theMenu_);
    toolbarButtonTwin_->setTwin(toolbarButton_);

    // make sure we get the button events of the start button
    if (triggerItem_)
    {
        triggerItem_->setMenuListener(this);
        if (AKToolbar::debugLevel(3) && coVRMSController::instance()->isMaster())
        {
            cerr << "Set MenuListener of Trigger \"" << triggerItem_->getName()
                 << "\" to menu \"" << name_ << "\"" << endl;
        }
    }
    else
    {
        if (AKToolbar::debugLevel(1) && coVRMSController::instance()->isMaster())
        {
            cerr << "NULL triggerItem_ given to \"" << name_ << "\"" << endl;
        }
    }

    /// Clone the Master menu
    coMenuItemVector allItems = cloneMenu->getAllItems();
    int i = 0;
    while (i < allItems.size())
    {
        coMenuItem *menuItem;
        coToolboxMenuItem *toolItem;
        if (copyItem(allItems[i], menuItem, toolItem))
        {
            theMenu_->add(menuItem);
            if (toolItem)
            {
                /// add this to toolbar if button pressed
                menuItem->setMenuListener(this);

                if (AKToolbar::debugLevel(2) && coVRMSController::instance()->isMaster())
                    cerr << "Set MenuListener of Item \"" << menuItem->getName()
                         << "\" to menu \"" << name_ << "\" for inlay" << endl;
                inlayMap_[menuItem] = toolItem;
            }
        }
        else // this is not an error: non-connected coSubMenuItem will be copied later
        {
        }
        ++i;
    }

    // we want to know our master's changes
    monitorMenu(cloneMenu);
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ++  Destructors
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

AkSubMenu::~AkSubMenu()
{
    for (SubMenuMap::iterator it = subMenuMap_.begin();
         it != subMenuMap_.end();
         ++it)
    {
        it->first->setMenuChangeListener(NULL);
        delete it->second;
    }
    subMenuMap_.clear();
    if (triggerItem_)
        triggerMenu_->remove(triggerItem_);
    if (toolbar_)
    {
        //toolbar_->remove(toolbarButton_);

        InlayMap::iterator inlay;
        for (inlay = inlayMap_.begin(); inlay != inlayMap_.end(); ++inlay)
        {
            toolbar_->remove(inlay->second);
        }
    }
    if (triggerItem_)
        delete triggerItem_;
    if (toolbarButton_)
        delete toolbarButton_;
    if (theMenu_)
        delete theMenu_;
    toolbarButtonTwin_->setMenu(NULL);
    delete toolbarButtonTwin_;

    for (MonitoredMenus::iterator it = monitoredMenus_.begin();
         it != monitoredMenus_.end();
         ++it)
    {
        it->first->setMenuChangeListener(NULL);
    }

    for (MonitoredSubMenuItems::iterator it = monitoredSubMenuItems_.begin();
         it != monitoredSubMenuItems_.end();
         ++it)
    {
        it->first->setMenuChangeListener(NULL);
    }
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ++  Operations
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// add an item to this menu
void AkSubMenu::add(coMenuItem *item, int pos)
{
    if (AKToolbar::debugLevel(2) && coVRMSController::instance()->isMaster())
        cerr << "AkSubMenu(" << name_ << ")::add(" << item->getName() << ", pos=" << pos << ")" << endl;
    if (pos >= 0)
        theMenu_->insert(item, pos);
    else
        theMenu_->add(item);

    if (AKToolbar::debugLevel(2) && coVRMSController::instance()->isMaster())
        cerr << "Set MenuListener of Item \"" << item->getName()
             << "\" to menu \"" << name_ << "\" for add, type is "
             << item->getClassName() << endl;

    /// This is a HACK - try to find reasons later
    // Beim EinhÃ¤ngen des Animation-MenÃ¼s wird der Listener sonst falsch gesetzt
    if (0 != strcmp("coButtonMenuItem", item->getClassName()))
        item->setMenuListener(this);
}

// delete an item from this menu
void AkSubMenu::remove(coMenuItem *item)
{
    if (item)
    {
        theMenu_->remove(item);
        if (theMenu_->isVisible())
        {
            toolbarButton_->closeSubmenu();
            toolbarButton_->openSubmenu();
        }
    }
}

/// add this menu to the toolbar
void AkSubMenu::addToToolbar()
{
    if (toolbar_)
        toolbar_->add(toolbarButton_);
}

void AkSubMenu::cleanToolbarRest()
{
    if (toolbar_)
    {
        toolbar_->closeAllOtherMenus(NULL);
        toolbar_->removeAllAfter(toolbarButton_);
    }
}

/// de-activate my menu
void AkSubMenu::deactivate()
{
    toolbarButton_->closeSubmenu();
}

void AkSubMenu::menuEvent(coMenuItem *menuItem)
{
    if (AKToolbar::debugLevel(2) && coVRMSController::instance()->isMaster())
    {
        fprintf(stderr, "AkSubMenu(\"%s\")::menuEvent for \"%s\"\n",
                name_.c_str(), menuItem->getName());

        fprintf(stderr, "Menu is %s\n",
                ((theMenu_->isVisible()) ? "visible" : "invisible"));
    }

    // -----------------------------------------------------
    // our trigger was pressed
    if (menuItem == (coMenuItem *)triggerItem_)
    {
        // remove all items after my parent's button in toolbar
        // and shut my parent's menu
        if (triggerMenu_)
        {
            triggerMenu_->cleanToolbarRest();
            triggerMenu_->deactivate();
        }

        // add my own button to toolbar
        toolbar_->add(toolbarButton_);

        // activate my menu
        toolbarButton_->openSubmenu();

        return;
    }

    // -----------------------------------------------------
    // our Toolbar button was pressed:
    //     - Show/hide Menu (automatic by button)
    else if (menuItem == (coMenuItem *)toolbarButton_)
    {
        // the menu became visible by button action
        if (theMenu_->isVisible())
        {
            // close all buttons behind my one in the toolbar ???? Better not.
            toolbar_->closeAllOtherMenus(toolbarButton_);
        }
        return;
    }

    // -----------------------------------------------------
    // all others: close my menu if the user requested it
    else if (autoClose_)
    {
        toolbarButton_->closeSubmenu();
    }

    // -----------------------------------------------------
    // an item with inlay (e.g. a Slider/Colormap) : show the item and close the menu
    InlayMap::iterator iter = inlayMap_.find(menuItem);
    if (iter != inlayMap_.end())
    {
        toolbar_->removeAllAfter(toolbarButton_);
        toolbar_->add(iter->second);
        toolbarButton_->closeSubmenu();

        // colorbar soll sofort offen sein
        if (strcmp(iter->second->getClassName(), "coLabelSubMenuToolboxItem") == 0)
            ((coLabelSubMenuToolboxItem *)iter->second)->openSubmenu();

        return;
    }
}

////////////  Menu item added to master menu

void AkSubMenu::itemAdded(coMenu *menu, coMenuItem *item, int pos)
{
    if (AKToolbar::plugin)
        AKToolbar::plugin->updatePlugins();

    if (AKToolbar::debugLevel(2) && coVRMSController::instance()->isMaster())
        fprintf(stderr, "Added menu point '%s' type '%s' to menu '%s' in AK menu '%s'\n",
                item->getName(),
                item->getClassName(),
                menu->getName(),
                name_.c_str());

    if (AKToolbar::debugLevel(2) && coVRMSController::instance()->isMaster())
        cerr << "@@@@@@@@@@@@@@@@ " << 1
             << " @@@@@@@@@@@@@@@@ " << item->getName()
             << " @@@@@@@@@@@@@@@@ " << name_ << endl;

    // check if we can simply copy it
    coMenuItem *menuItem; // the item in our Menu
    coToolboxMenuItem *toolItem; // a "inlay" item put into the toolbar
    if (copyItem(item, menuItem, toolItem))
    {

        if (AKToolbar::debugLevel(2) && coVRMSController::instance()->isMaster())
            cerr << "@@@@@@@@@@@@@@@@ " << 2
                 << " @@@@@@@@@@@@@@@@ " << item->getName()
                 << " @@@@@@@@@@@@@@@@ " << name_ << endl;

        add(menuItem, pos);
        if (toolItem) // if an "inlay" is done, we manage it in this map
        {

            if (AKToolbar::debugLevel(2) && coVRMSController::instance()->isMaster())
                cerr << "@@@@@@@@@@@@@@@@ " << 3
                     << " @@@@@@@@@@@@@@@@ " << item->getName()
                     << " @@@@@@@@@@@@@@@@ " << name_ << endl;

            inlayMap_[menuItem] = toolItem;
        }
    }

    else if (AKToolbar::debugLevel(2) && coVRMSController::instance()->isMaster())
        cerr << "@@@@@@@@@@@@@@@@ " << 4
             << " @@@@@@@@@@@@@@@@ " << item->getName()
             << " @@@@@@@@@@@@@@@@ " << name_ << endl;

    //else   // may be correct for non-connected coSubMenuItem
    //{
    //   fprintf(stderr,"AkSubMenu(%s) copy of %s not successful\n",
    //                  getName(),item->getName());
    //}

    // make sure the display is aligned correctly
    if (theMenu_->isVisible())
    {
        toolbarButton_->closeSubmenu();
        toolbarButton_->openSubmenu();
    }
}

void AkSubMenu::itemRemoved(coMenu *, coMenuItem *item)
{
    if (AKToolbar::plugin)
        AKToolbar::plugin->updatePlugins();

    if (AKToolbar::debugLevel(3) && coVRMSController::instance()->isMaster())
        fprintf(stderr, "AkSubMenu(%s): Removed menu point %s(%s)\n",
                name_.c_str(), item->getClassName(), item->getName());

    /// a Sub-Menu was removed - created my own superstructure - DELETE IT!

    if (coSubMenuItem *subitem = dynamic_cast<coSubMenuItem *>(item))
    {
        //       remove(item);

        // dr: das scheint mir falsch im toolbar menue ist ja nicht der pinboard item sondern ein geclonter der gleich heisst
        // remove(item);

        // so sollte es richtig sein
        coMenuItemVector allItems = theMenu_->getAllItems();
        int i = 0;
        while (i < allItems.size())
        {
            //fprintf(stderr,"aktoolbar %s\n", allItems[i]->getName());
            if (strcmp(allItems[i]->getName(), item->getName()) == 0)
            {
                //fprintf(stderr,"found the corresponding trigger item in toolbar and remove it\n");
                remove(allItems[i]);
                break;
            }
            i++;
        }

        // find the corresponding AkSubMenu and delete ir
        coMenu *menu = subitem->getMenu();
        SubMenuMap::iterator currMenu = subMenuMap_.find(menu);
        if (currMenu != subMenuMap_.end())
        {
            AkSubMenu *delMenu = currMenu->second;
            subMenuMap_.erase(currMenu);
            delete delMenu;
        }
    }

    // we are not notified about our button, but about its Twin in the 'normal' menu!
    coMenuItem *twin = item->getTwin();

    // remove from my menu
    if (twin)
    {
        if (strcmp(twin->getClassName(), "coLabelSubMenuToolboxItem") == 0)
        {
            InlayMap::iterator inlay;
            for (inlay = inlayMap_.begin(); inlay != inlayMap_.end(); ++inlay)
            {
                if (inlay->second == twin)
                {
                    remove(inlay->first);
                    toolbar_->remove(inlay->second);
                }
            }
        }
        else
            remove(twin);
    }

    //    coMovableBackgroundMenuItem *movableBackgroundItem = dynamic_cast<coMovableBackgroundMenuItem*>(item);
    //    if (movableBackgroundItem)
    //    {
    //       remove(movableBackgroundItem);
    //    }

    // make sure the display is aligned correctly
    if (theMenu_->isVisible())
    {
        toolbarButton_->closeSubmenu();
        toolbarButton_->openSubmenu();
    }
}

void AkSubMenu::itemRenamed(coMenu *menu, coMenuItem *item)
{
    if (AKToolbar::debugLevel(2) && coVRMSController::instance()->isMaster())
        fprintf(stderr, "AkSubMenu(%s)::itemRenamed: '%s' [%s] in Menu '%s'\n",
                name_.c_str(), item->getName(), item->getClassName(),
                menu->getName());
    coGenericSubMenuItem *genSubMenu = dynamic_cast<coGenericSubMenuItem *>(item);
    if (item && genSubMenu)
    {
        //fprintf(stderr,"item->isOfClassName(coGenericSubMenuItem)\n");

        coMenu *menu = genSubMenu->getMenu();
        SubMenuMap::iterator iter = subMenuMap_.find(menu);
        if (iter != subMenuMap_.end())
        {
            iter->second->rename(item->getName());
            // quick fix for change title of submenu
            // because coRowMenuHandle has no twin and we are not notified if the title changes
            // suggestion: the title of the menu ist the itemName without "..." at the end
            // if the itemName has no points at the end, the title is the itemName
            coRowMenu *row = dynamic_cast<coRowMenu *>(iter->second->getMenu());
            if (row)
            {
                std::string rowname;
                rowname = std::string(item->getName());
                std::string::size_type where = rowname.find_last_of("...");
                if (where != std::string::npos)
                    rowname = rowname.substr(0, where - 2);
                row->updateTitle(rowname.c_str());
            }
        }
    }

    if (coSliderMenuItem *slider = dynamic_cast<coSliderMenuItem *>(item))
    {
        if (slider->getTwin())
            slider->getTwin()->setLabel(slider->getLabel()->getString());
    }
}

/** the subMenuItem controlling this menu was set
@param subMenu pointer to the added submenu
*/
void AkSubMenu::subMenuAdded(coMenu *newMenu, coGenericSubMenuItem *item)
{
    if (AKToolbar::plugin)
        AKToolbar::plugin->updatePlugins();

    if (!item) // actually this item was removed
        return;

    if (AKToolbar::debugLevel(2) && coVRMSController::instance()->isMaster())
        fprintf(stderr, "AkSubMenu(%s)::subMenuAdded(%s,item)\n",
                getName(), newMenu->getName());

    // do not enter same menu twice
    if (subMenuMap_.find(newMenu) != subMenuMap_.end())
        return;

    const char *name = newMenu->getName();

    /// different treatment for Colorbars
    coMenuItemVector allItems = newMenu->getAllItems();
    if ((allItems.size() > 0) && ((strcmp((allItems[0])->getClassName(), "coColorBar") == 0)
                                  || (strcmp((allItems[0])->getClassName(), "coMovableBackgroundMenuItem") == 0)
                                  || (strcmp((allItems[0])->getClassName(), "coPlotMenuItem") == 0)))
    {
        // secure against infinite loop
        subMenuMap_[newMenu] = NULL;

        //if(strcmp((*allItems)->getClassName(),"coSubMenuItem")!=0)
        //   return;
        // ob das so stimmt, muss ich noch checken
        // coMenuItem *button = (coMenuItem*)(*allItems);
        // dr: hast Du wohl nicht mehr gecheckt, tut nicht, deshalb WIEDER:
        coSubMenuItem *button = dynamic_cast<coSubMenuItem *>(item);

        coButtonMenuItem *trigger = new coButtonMenuItem(button->getName());

        add(trigger, -1);

        coLabelSubMenuToolboxItem *inlayButton
            = new coLabelSubMenuToolboxItem(button->getName());
        inlayButton->setMenu(newMenu);
        inlayButton->setTwin(button);
        trigger->setMenuListener(this);
        inlayMap_[trigger] = inlayButton;
    }
    else // normal menu added
    {
        coButtonMenuItem *trigger = new coButtonMenuItem(((coSubMenuItem *)item)->getName());
        add(trigger, -1);
        AkSubMenu *newAkMenu = new AkSubMenu(name, this, trigger, toolbar_);
        subMenuMap_[newMenu] = newAkMenu;
        newMenu->setMenuChangeListener(newAkMenu);
    }
}

/** a new submenu was deleted with this menu as parent
@param subMenu pointer to the deleted submenu
*/
void AkSubMenu::subMenuDeleted(coMenu *submenu)
{
    if (AKToolbar::plugin)
        AKToolbar::plugin->updatePlugins();

    if (AKToolbar::debugLevel(2) && coVRMSController::instance()->isMaster())
        cerr << "AkSubMenu(" << name_ << ")::subMenuDeleted("
             << submenu->getName() << ")" << endl;

    SubMenuMap::iterator iter = subMenuMap_.find(submenu);

    if (iter != subMenuMap_.end())
    {
        delete iter->second;
        subMenuMap_.erase(iter);
    }
}

/// moritor this menu's enevts
void AkSubMenu::monitorMenu(coMenu *menu)
{
    monitoredMenus_[menu] = menu;
    menu->setMenuChangeListener(this);
}

void AkSubMenu::monitorSubMenuItem(coSubMenuItem *item)
{
    monitoredSubMenuItems_[item] = item;
    item->setMenuChangeListener(this);
}

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ++  Attribute request/set functions
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ++  Internally used functions
// +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

bool AkSubMenu::copyItem(coMenuItem *copyitem,
                         coMenuItem *&menuItem,
                         coToolboxMenuItem *&toolItem)
{
    if (AKToolbar::debugLevel(3) && coVRMSController::instance()->isMaster())
        cerr << "AkSubMenu::copyItem(" << copyitem->getName() << ")" << endl;

    menuItem = NULL;
    toolItem = NULL;

    // dynamic_cast does not work on all platforms, so donÂ´t use if not necessary
    // Checkbox item - trivial copy
    if (strcmp(copyitem->getClassName(), "coCheckboxMenuItem") == 0)
    {
        coCheckboxMenuItem *cbItem = (coCheckboxMenuItem *)copyitem;
        coCheckboxMenuItem *myCheckBox
            = new coCheckboxMenuItem(cbItem->getName(), cbItem->getState());
        myCheckBox->setTwin(cbItem);
        myCheckBox->setMenuListener(this);
        menuItem = myCheckBox;
        return true;
    }

    // Button item - trivial copy
    if (strcmp(copyitem->getClassName(), "coButtonMenuItem") == 0)
    {
        coButtonMenuItem *bItem = (coButtonMenuItem *)copyitem;
        coButtonMenuItem *myButton = new coButtonMenuItem(bItem->getName());
        myButton->setTwin(bItem);
        myButton->setMenuListener(this);
        menuItem = myButton;
        return true;
    }

    // Submenu - create e new level
    if (strcmp(copyitem->getClassName(), "coSubMenuItem") == 0)
    {
        coSubMenuItem *sItem = (coSubMenuItem *)copyitem;
        // get the menu of this button
        coMenu *subMenu = sItem->getMenu();

        // menu not yet available - add it later
        if (!subMenu)
        {
            // add the enuChangeListener to the menu as soon as the button is put in
            monitorSubMenuItem(sItem);
            return false;
        }
        else
        {
            // check: Submenu already there?
            if (subMenuMap_.find(subMenu) == subMenuMap_.end())
            {
                // different treatment for ColorBar and DocumentViewer
                coMenuItemVector allItems = subMenu->getAllItems();
                if ((allItems.size() > 0) && ((strcmp((allItems[0])->getClassName(), "coColorBar") == 0)
                                              || (strcmp((allItems[0])->getClassName(), "coMovableBackgroundMenuItem") == 0)
                                              || (strcmp((allItems[0])->getClassName(), "coPlotMenuItem") == 0)))
                {
                    subMenuMap_[subMenu] = NULL;
                    coButtonMenuItem *trigger = new coButtonMenuItem(sItem->getName());
                    trigger->setMenuListener(this);
                    menuItem = trigger;
                    coLabelSubMenuToolboxItem *inlayButton = new coLabelSubMenuToolboxItem(sItem->getName());
                    inlayButton->setMenu(subMenu);
                    inlayButton->setTwin(sItem);
                    inlayMap_[trigger] = inlayButton;
                }
                else
                {
                    // create trigger button
                    coButtonMenuItem *trigger = new coButtonMenuItem(sItem->getName());
                    menuItem = trigger;

                    // create new menu and clone it
                    AkSubMenu *akMenu = new AkSubMenu(subMenu, this, trigger, toolbar_);
                    subMenuMap_[subMenu] = akMenu;
                }
                return true;
            }
            else
                return false; // second try - don't do anything
        }
    }

    // Slider - somewhat trickier
    if (strcmp(copyitem->getClassName(), "coSliderMenuItem") == 0)
    {
        coSliderMenuItem *slItem = (coSliderMenuItem *)copyitem;

        // we create only a button in the menu and attach a slider later
        std::string name(slItem->getName());
        if (name.rfind("...") != name.length() - 3)
            name += "...";
        coButtonMenuItem *myButton = new coButtonMenuItem(name.c_str());
        myButton->setMenuListener(this);
        menuItem = myButton;
        coSliderToolboxItem *tsli = new coSliderToolboxItem(slItem->getLabel()->getString(),
                                                            slItem->getMin(),
                                                            slItem->getMax(),
                                                            slItem->getValue());
        toolItem = tsli;
        slItem->setTwin(tsli);

        return true;
    }

    // Poti - somewhat trickier: we prefer a slider in the toolbar
    if (strcmp(copyitem->getClassName(), "coPotiMenuItem") == 0)
    {

        coPotiMenuItem *poItem = (coPotiMenuItem *)copyitem;
        // we create only a button in the menu and attach a slider later
        std::string name(poItem->getName());
        if (name.rfind("...") != name.length() - 3)
            name += "...";
        coButtonMenuItem *myButton = new coButtonMenuItem(name.c_str());
        myButton->setMenuListener(this);
        menuItem = myButton;

        coSliderToolboxItem *tsli = new coSliderToolboxItem(poItem->getName(),
                                                            poItem->getMin(),
                                                            poItem->getMax(),
                                                            poItem->getValue());
        toolItem = tsli;
        poItem->setTwin(tsli);

        return true;
    }

    if (strcmp(copyitem->getClassName(), "coColorBar") == 0)
    {
        //coColorBar *colItem = (coColorBar*)copyitem;
        if (cover->debugLevel(1))
            cerr << "A Problem occurred: ColorBar called AkSubMenu::copyItem()" << endl;
    }
    return false;
}

void AkSubMenu::rename(const char *newName)
{
    name_ = newName;
    triggerItem_->setName(newName);
    toolbarButton_->setName(newName);
}

void AkSubMenu::focusEvent(bool /*focus*/, coMenu *)
{
    if (cover->debugLevel(5))
        fprintf(stderr, "VRMenu::focusEvent\n");

    //VRPinboard::mainPinboard->lockInteraction = focus;
}
