/****************************************************************************
Copyright (c) 2010 cocos2d-x.org

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


#include "CCObject.h"
#include "CCAutoreleasePool.h"
#include "ccMacros.h"
#include "script_support/CCScriptSupport.h"
#include "support/profile/CCMemory.h"

NS_CC_BEGIN

CCObject* CCCopying::copyWithZone(CCZone *pZone)
{
    CC_UNUSED_PARAM(pZone);
    CCAssert(0, "not implement");
    return 0;
}

CCObject::CCObject(void)
: m_nLuaID(0)
, m_uReference(1) // when the object is created, the reference count of it is 1
, m_uAutoReleaseCount(0)
, m_nScriptUserDataId(0)
#if CC_CFLAG_MEMORY_TRACKING
, m_tracked(false)
#endif
{
    static unsigned int uObjectCount = 0;

    m_uID = ++uObjectCount;
}

CCObject* CCObject::create() {
    CCObject* o = new CCObject();
    return o->autorelease();
}

CCObject::~CCObject(void)
{
    // for memory debugging
#ifdef CC_CFLAG_MEMORY_TRACKING
    CCMemory::untrackCCObject(this);
#endif
    
    // if the object is managed, we should remove it
    // from pool manager
    if (m_uAutoReleaseCount > 0)
    {
        CCPoolManager::sharedPoolManager()->removeObject(this);
    }

    // script side user data
    if(m_nScriptUserDataId) {
        CCScriptEngineManager::sharedManager()->getScriptEngine()->removeScriptUserData(m_nScriptUserDataId);
        m_nScriptUserDataId = 0;
    }
    
    // if the object is referenced by Lua engine, remove it
    if (m_nLuaID)
    {
        // destructor event of ccobject
        CCScriptEngineManager::sharedManager()->getScriptEngine()->executeObjectDestructor(this);
        
        // remove object from lua registry
        CCScriptEngineManager::sharedManager()->getScriptEngine()->removeScriptObjectByCCObject(this);
    }
    else
    {
        CCScriptEngineProtocol* pEngine = CCScriptEngineManager::sharedManager()->getScriptEngine();
        if (pEngine != NULL && pEngine->getScriptType() == kScriptTypeJavascript)
        {
            pEngine->removeScriptObjectByCCObject(this);
        }
    }
}

CCObject* CCObject::copy()
{
    return copyWithZone(0);
}

void CCObject::release(const char* file, int line)
{
    CCAssert(m_uReference > 0, "reference count should greater than 0");
    --m_uReference;

    // for memory debugging
#ifdef CC_CFLAG_MEMORY_TRACKING
    CCMemory::trackRelease(this, file, line);
#endif
    
    if (m_uReference == 0)
    {
        delete this;
    }
}

void CCObject::retain(const char* file, int line)
{
    CCAssert(m_uReference > 0, "reference count should greater than 0");

    ++m_uReference;
    
    // for memory debugging
#ifdef CC_CFLAG_MEMORY_TRACKING
    CCMemory::trackRetain(this, file, line);
#endif
}

CCObject* CCObject::autorelease(const char* file, int line)
{
    CCPoolManager::sharedPoolManager()->addObject(this);
    
    // for memory debugging
#ifdef CC_CFLAG_MEMORY_TRACKING
    CCMemory::trackAutorelease(this, file, line);
#endif
    
    return this;
}

bool CCObject::isSingleReference(void) const
{
    return m_uReference == 1;
}

unsigned int CCObject::autoReleaseCount(void) const {
    return m_uAutoReleaseCount;
}

unsigned int CCObject::retainCount(void) const
{
    return m_uReference;
}

bool CCObject::isEqual(const CCObject *pObject)
{
    return this == pObject;
}

void CCObject::acceptVisitor(CCDataVisitor &visitor)
{
    visitor.visitObject(this);
}

void CCObject::_setScriptUserData(int dataId) {
    if(m_nScriptUserDataId) {
        CCScriptEngineManager::sharedManager()->getScriptEngine()->removeScriptUserData(m_nScriptUserDataId);
        m_nScriptUserDataId = 0;
    }
    m_nScriptUserDataId = dataId;
}

int CCObject::_getScriptUserData() {
    return m_nScriptUserDataId;
}

NS_CC_END