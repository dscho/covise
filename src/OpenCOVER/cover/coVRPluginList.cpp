/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

#include <util/common.h>
#include <util/byteswap.h>
#include <util/string_util.h>

#include "coVRPluginList.h"
#include "coVRPluginSupport.h"
#include "coVRPlugin.h"
#include "coVRSelectionManager.h"
#include "RenderObject.h"
#include "coVRMSController.h"

#include <config/CoviseConfig.h>
#include "coVRTui.h"
#include "OpenCOVER.h"
#include "coHud.h"

#ifdef DOTIMING
#include <util/coTimer.h>
#endif

#include "VRViewer.h"
#include "PluginMenu.h"

namespace opencover
{
class coInteractor;
}

using namespace opencover;
using namespace covise;

typedef opencover::coVRPlugin *(coVRPluginInitFunc)();

// do something for all plugins
#define DOALL(something)                                                                                   \
    {                                                                                                      \
        for (PluginMap::iterator plugin_it = m_plugins.begin(); plugin_it != m_plugins.end(); ++plugin_it) \
        {                                                                                                  \
            coVRPlugin *plugin = plugin_it->second;                                                        \
            {                                                                                              \
                something;                                                                                 \
            }                                                                                              \
        }                                                                                                  \
    }

coVRPlugin *coVRPluginList::loadPlugin(const char *name)
{
    if (cover->debugLevel(3))
    {
        if (name)
            fprintf(stderr, "coVRPluginList::loadPlugin %s\n", name);
        else
            fprintf(stderr, "coVRPluginList::loadPlugin name=NULL\n");
    }

    std::string libName = coVRDynLib::libName(name);
    CO_SHLIB_HANDLE handle = coVRDynLib::dlopen(libName);
    if (handle == NULL)
    {
        cerr << "ERROR: could not load shared Library " << libName << endl;
        return NULL;
    }
    coVRPluginInitFunc *initFunc = (coVRPluginInitFunc *)coVRDynLib::dlsym(handle, "coVRPluginInit");
    if (initFunc == NULL)
    {
        coVRDynLib::dlclose(handle);
        cerr << "ERROR: malformed COVER plugin " << name << ", no coVRPluginInit defined" << endl;
        return NULL;
    }
    coVRPlugin *plugin = initFunc();
    if (plugin)
    {
        plugin->handle = handle;
        plugin->setName(name);
    }

    return plugin;
}

coVRPluginList *coVRPluginList::singleton = NULL;
coVRPluginList *coVRPluginList::instance()
{
    if (!singleton)
        singleton = new coVRPluginList();
    return singleton;
}

coVRPluginList::~coVRPluginList()
{
    unloadAllPlugins();
    singleton = NULL;
}

void coVRPluginList::unloadAllPlugins()
{
    if (cover->debugLevel(1))
        cerr << "Unloading plugins:";
    bool wasThreading = VRViewer::instance()->areThreadsRunning();
    if (wasThreading)
        VRViewer::instance()->stopThreading();
    for (PluginMap::iterator it = m_plugins.begin();
         it != m_plugins.end();
         ++it)
    {
        if (cover->debugLevel(1))
            cerr << " " << it->first;
        delete it->second;
    }
    m_plugins.clear();
    if (wasThreading)
        VRViewer::instance()->startThreading();
    if (cover->debugLevel(1))
        cerr << endl;
}

coVRPluginList::coVRPluginList()
{
    singleton = this;
    keyboardPlugin = NULL;
    if (cover->debugLevel(1))
    {
        cerr << "Loading plugins:";
    }

    if (const char *env = getenv("COVER_PLUGINS"))
    {
        std::vector<std::string> plugins = split(env, ':');
        for (size_t i = 0; i < plugins.size(); ++i)
        {
            if (cover->debugLevel(1))
            {
                cerr << " " << plugins[i];
            }
            if (!getPlugin(plugins[i].c_str()))
            {
                if (coVRPlugin *m = loadPlugin(plugins[i].c_str()))
                {
                    m_plugins[plugins[i]] = m; // if init OK, then add new plugin
                }
                else
                {
                    cerr << "plugin " << plugins[i] << " does not have an init function - not loading" << endl;
                }
            }
        }
    }

    coCoviseConfig::ScopeEntries pluginNameEntries = coCoviseConfig::getScopeEntries("COVER.Plugin");
    const char **pluginNames = pluginNameEntries.getValue();
    int i = 0;
    if (pluginNames != NULL)
    {
        while (pluginNames[i])
        {
            bool exists = false;
            char *entryName = new char[strlen(pluginNames[i]) + 100];
            sprintf(entryName, "COVER.Plugin.%s", pluginNames[i]);
            if (coCoviseConfig::isOn(entryName, false))
            {

                if (cover->debugLevel(1))
                {
                    cerr << " " << pluginNames[i];
                }
                coVRPlugin *m = getPlugin(pluginNames[i]);
                if (!m)
                {
                    m = loadPlugin(pluginNames[i]);
                    if (m)
                    {
                        m_plugins[pluginNames[i]] = m; // if init OK, then add new plugin
                    }
                    else
                    {
                        cerr << "plugin " << pluginNames[i] << " does not have an init function - not loading" << endl;
                    }
                }
            }
            if (coCoviseConfig::isOn("menu", entryName, false, &exists))
            {
                coVRPlugin *m = getPlugin(pluginNames[i]);
                PluginMenu::instance()->addEntry(pluginNames[i], m);
            }
            delete[] entryName;

            i++; // skip name
            i++; //skip value (may be NULL)
        }
    }
    if (cover->debugLevel(1))
    {
        cerr << "." << endl;
    }
}

void coVRPluginList::unload(coVRPlugin *plugin)
{
    if (!plugin)
        return;

    bool wasThreading = VRViewer::instance()->areThreadsRunning();
    if (wasThreading)
        VRViewer::instance()->stopThreading();
    if (plugin->destroy())
    {
        m_unloadQueue.push_back(plugin->handle);
        if (m_plugins.erase(plugin->getName()) == 0)
        {
            cerr << "Plugin to unload not found2" << endl;
        }
        delete plugin;
        updateState();
    }
    if (wasThreading)
        VRViewer::instance()->startThreading();
}

void coVRPluginList::unloadQueued()
{
    for (HandleList::iterator it = m_unloadNext.begin(); it != m_unloadNext.end(); ++it)
    {
        coVRDynLib::dlclose(*it);
    }
    m_unloadNext = m_unloadQueue;
    m_unloadQueue.clear();
}

void coVRPluginList::addNode(osg::Node *node, RenderObject *ro, coVRPlugin *addingPlugin)
{
    DOALL(if (plugin != addingPlugin)
              plugin->addNode(node, ro));
}

void coVRPluginList::addObject(RenderObject *baseObj,
                               RenderObject *geomObj, RenderObject *normObj,
                               RenderObject *colorObj, RenderObject *texObj,
                               osg::Group *root,
                               int numCol, int colorBinding, int colorPacking,
                               float *r, float *g, float *b, int *packedCol,
                               int numNormals, int normalBinding,
                               float *xn, float *yn, float *zn,
                               float transparency)
{
    // call addObject for the current plugin in the plugin list
    DOALL(plugin->addObject(baseObj, geomObj, normObj, colorObj, texObj,
                            root, numCol, colorBinding, colorPacking,
                            r, g, b, packedCol,
                            numNormals, normalBinding,
                            xn, yn, zn, transparency));
}

void coVRPluginList::newInteractor(RenderObject *container, coInteractor *it)
{
    DOALL(plugin->newInteractor(container, it));
}

void coVRPluginList::coviseError(const char *error)
{
    DOALL(plugin->coviseError(error));
}

void coVRPluginList::guiToRenderMsg(const char *msg)
{
    DOALL(plugin->guiToRenderMsg(msg));
}

void coVRPluginList::removeObject(const char *objName, bool replaceFlag)
{
    // call deleteObject for the current plugin in the plugin list
    DOALL(plugin->removeObject(objName, replaceFlag));
}

void coVRPluginList::removeNode(osg::Node *node, bool isGroup, osg::Node *realNode)
{
    if (isGroup)
        coVRSelectionManager::instance()->removeNode(node);

    DOALL(plugin->removeNode(node, isGroup, realNode));
}

void coVRPluginList::prepareFrame()
{
#ifdef DOTIMING
    MARK0("COVER calling prepareFrame for all plugins");
#endif
    DOALL(plugin->prepareFrame());
#ifdef DOTIMING
    MARK0("done");
#endif
}

void coVRPluginList::preFrame()
{
#ifdef DOTIMING
    MARK0("COVER calling preFrame for all plugins");
#endif
    unloadQueued();

    DOALL(plugin->preFrame());
#ifdef DOTIMING
    MARK0("done");
#endif
}

void coVRPluginList::setTimestep(int t)
{
    DOALL(plugin->setTimestep(t));
}

void coVRPluginList::postFrame()
{
#ifdef DOTIMING
    MARK0("COVER calling postFrame for all plugins");
#endif

    DOALL(plugin->postFrame());
#ifdef DOTIMING
    MARK0("done");
#endif
}

void coVRPluginList::preDraw(osg::RenderInfo &renderInfo)
{
    DOALL(plugin->preDraw(renderInfo));
}

void coVRPluginList::preSwapBuffers(int windowNumber)
{
    DOALL(plugin->preSwapBuffers(windowNumber));
}

void coVRPluginList::postSwapBuffers(int windowNumber)
{
    DOALL(plugin->postSwapBuffers(windowNumber));
}

void coVRPluginList::param(const char *paramName, bool inMapLoading)
{
    DOALL(plugin->param(paramName, inMapLoading));
}

void coVRPluginList::grabKeyboard(coVRPlugin *p)
{
    keyboardPlugin = p;
}

bool coVRPluginList::key(int type, int keySym, int mod)
{
    if (keyboardPlugin)
    {
        keyboardPlugin->key(type, keySym, mod); // only forward key events to this plugin
    }
    else
    {
        DOALL(plugin->key(type, keySym, mod));
    }

    return true;
}

bool coVRPluginList::userEvent(int mod)
{
    DOALL(plugin->userEvent(mod));
    return true;
}

void coVRPluginList::init()
{
    PluginMenu::instance()->init();
    for (PluginMap::iterator it = m_plugins.begin();
         it != m_plugins.end();)
    {
        OpenCOVER::instance()->hud->setText3(it->first);
        OpenCOVER::instance()->hud->redraw();

        if (!it->second->init())
        {
            cerr << "plugin " << it->second->getName() << " failed to initialise" << endl;
            m_plugins.erase(it++);
        }
        else
        {
            ++it;
        }
    }
    updateState();
}

void coVRPluginList::message(int t, int l, const void *b)
{
    DOALL(plugin->message(t, l, b));
}

coVRPlugin *coVRPluginList::getPlugin(const char *name)
{
    PluginMap::iterator it = m_plugins.find(name);
    if (it == m_plugins.end())
        return NULL;

    return it->second;
}

coVRPlugin *coVRPluginList::addPlugin(const char *name)
{
    coVRPlugin *m = getPlugin(name);
    if (m == NULL)
    {
        m = loadPlugin(name);
        if (m && m->init())
        {
            m_plugins[name] = m; // if init OK, then add new plugin
        }
        else
        {
            cerr << "plugin " << name << " failed to initialise" << endl;
            delete m;
            m = NULL;
        }
        updateState();
    }
    return m;
}

void coVRPluginList::forwardMessage(int len, const void *buf)
{
    int headerSize = 2 * sizeof(int);
    if (len < headerSize)
    {
        cerr << "wrong Message received in coVRPluginList::forwardMessage" << endl;
        return;
    }
    int toWhom = *((int *)buf);
    int type = *(((int *)buf) + 1);
#ifdef BYTESWAP
    byteSwap(toWhom);
    byteSwap(type);
#endif
    if ((toWhom == coVRPluginSupport::TO_ALL)
        || (toWhom == coVRPluginSupport::TO_ALL_OTHERS)
        || (toWhom == coVRPluginSupport::VRML_EVENT))
    {
        message(type, len - headerSize, ((const char *)buf) + headerSize);
    }
    else
    {
        const char *name = (((const char *)buf) + headerSize);
        coVRPlugin *mod = getPlugin(name);
        if (mod)
        {
            int ssize = strlen(name) + 1 + (8 - ((strlen(name) + 1) % 8));
            mod->message(type, len - headerSize - ssize, ((const char *)buf) + headerSize + ssize);
        }
    }
}

void coVRPluginList::requestQuit(bool killSession)
{
    DOALL(plugin->requestQuit(killSession));
}

bool coVRPluginList::sendVisMessage(const Message *msg)
{
    DOALL(if (plugin->sendVisMessage(msg))
          { return true;
          })
    return false;
}

bool coVRPluginList::becomeCollaborativeMaster()
{
    DOALL(if (plugin->becomeCollaborativeMaster())
          { return true;
          })

    return false;
}

covise::Message *coVRPluginList::waitForVisMessage(int messageType)
{
    DOALL(covise::Message *m = plugin->waitForVisMessage(messageType);
          if (m)
          { return m;
    });

    return NULL;
}

bool coVRPluginList::executeAll()
{
    DOALL(if (plugin->executeAll())
          { return true;
          });

    return false;
}

void coVRPluginList::expandBoundingSphere(osg::BoundingSphere &bs)
{
    DOALL(plugin->expandBoundingSphere(bs));
}

void coVRPluginList::updateState()
{
    coVRTui::instance()->updateState();
    PluginMenu::instance()->updateState();
}