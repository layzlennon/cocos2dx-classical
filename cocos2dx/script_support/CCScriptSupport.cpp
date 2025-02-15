/****************************************************************************
 Copyright (c) 2010-2012 cocos2d-x.org
 
 http://www.cocos2d-x.org
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ****************************************************************************/

#include "CCScriptSupport.h"
#include "CCScheduler.h"

bool CC_DLL cc_assert_script_compatible(const char *msg)
{
    cocos2d::CCScriptEngineProtocol* pEngine = cocos2d::CCScriptEngineManager::sharedManager()->getScriptEngine();
    if (pEngine && pEngine->handleAssert(msg))
    {
        return true;
    }
    return false;
}

NS_CC_BEGIN

 #if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS) || (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
// #pragma mark -
// #pragma mark CCScriptHandlerEntry
#endif

CCScriptHandlerEntry* CCScriptHandlerEntry::create(ccScriptFunction func)
{
    CCScriptHandlerEntry* entry = new CCScriptHandlerEntry(func);
    CC_SAFE_AUTORELEASE(entry);
    return entry;
}

CCScriptHandlerEntry::~CCScriptHandlerEntry()
{
	if (m_func.handler != 0)
	{
        CCScriptEngineManager::sharedManager()->getScriptEngine()->removeScriptHandler(m_func.handler);
        m_func.handler = 0;
    }
}

#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS) || (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
// #pragma mark -
// #pragma mark CCSchedulerScriptHandlerEntry
#endif

CCSchedulerScriptHandlerEntry* CCSchedulerScriptHandlerEntry::create(ccScriptFunction nHandler, float fInterval, int repeat, float delay, bool bPaused)
{
    CCSchedulerScriptHandlerEntry* pEntry = new CCSchedulerScriptHandlerEntry(nHandler);
    pEntry->init(fInterval, repeat, delay, bPaused);
    CC_SAFE_AUTORELEASE(pEntry);
    return pEntry;
}

bool CCSchedulerScriptHandlerEntry::init(float fInterval, int repeat, float delay, bool bPaused)
{
    m_pTimer = new CCTimer();
    m_pTimer->initWithScriptHandler(m_func, fInterval, repeat, delay);
    CC_SAFE_AUTORELEASE(m_pTimer);
    CC_SAFE_RETAIN(m_pTimer);
    m_bPaused = bPaused;
    LUALOG("[LUA] ADD script schedule: %d, entryID: %d", m_func.handler, m_nEntryId);
    return true;
}

CCSchedulerScriptHandlerEntry::~CCSchedulerScriptHandlerEntry(void)
{
    CC_SAFE_RELEASE(m_pTimer);
    LUALOG("[LUA] DEL script schedule %d, entryID: %d", m_nHandler, m_nEntryId);
}

#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS) || (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
// #pragma mark -
// #pragma mark CCTouchScriptHandlerEntry
#endif

CCTouchScriptHandlerEntry* CCTouchScriptHandlerEntry::create(ccScriptFunction nHandler)
{
    CCTouchScriptHandlerEntry* pEntry = new CCTouchScriptHandlerEntry(nHandler);
    pEntry->init();
    CC_SAFE_AUTORELEASE(pEntry);
    return pEntry;
}

CCTouchScriptHandlerEntry::~CCTouchScriptHandlerEntry(void)
{
    if (m_func.handler != 0)
    {
        CCScriptEngineManager::sharedManager()->getScriptEngine()->removeScriptHandler(m_func.handler);
        LUALOG("[LUA] Remove touch event handler: %d", m_func.handler);
        m_func.handler = 0;
    }
}

bool CCTouchScriptHandlerEntry::init()
{
    return true;
}

#if (CC_TARGET_PLATFORM == CC_PLATFORM_IOS) || (CC_TARGET_PLATFORM == CC_PLATFORM_MAC)
// #pragma mark -
// #pragma mark CCScriptEngineManager
#endif

static CCScriptEngineManager* s_pSharedScriptEngineManager = NULL;


CCScriptEngineManager::~CCScriptEngineManager(void)
{
    removeScriptEngine();
}

void CCScriptEngineManager::setScriptEngine(CCScriptEngineProtocol *pScriptEngine)
{
    removeScriptEngine();
    m_pScriptEngine = pScriptEngine;
}

void CCScriptEngineManager::removeScriptEngine(void)
{
    if (m_pScriptEngine)
    {
        delete m_pScriptEngine;
        m_pScriptEngine = NULL;
    }
}

CCScriptEngineManager* CCScriptEngineManager::sharedManager(void)
{
    if (!s_pSharedScriptEngineManager)
    {
        s_pSharedScriptEngineManager = new CCScriptEngineManager();
    }
    return s_pSharedScriptEngineManager;
}

void CCScriptEngineManager::purgeSharedManager(void)
{
    if (s_pSharedScriptEngineManager)
    {
        delete s_pSharedScriptEngineManager;
        s_pSharedScriptEngineManager = NULL;
    }
}

void CCScriptEngineManager::setScriptDecryptFunc(CC_FILE_DECRYPT_FUNC decFunc) {
    m_decFunc = decFunc;
}

CC_FILE_DECRYPT_FUNC CCScriptEngineManager::getScriptDecryptFunc() {
    return m_decFunc;
}

NS_CC_END
