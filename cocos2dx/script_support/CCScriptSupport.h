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

#ifndef __SCRIPT_SUPPORT_H__
#define __SCRIPT_SUPPORT_H__

#include "platform/CCCommon.h"
#include "platform/CCAccelerometer.h"
#include "touch_dispatcher/CCTouch.h"
#include "cocoa/CCSet.h"
#include <map>
#include <string>
#include <list>
#include "ccTypes.h"

typedef struct lua_State lua_State;

NS_CC_BEGIN

class CCObject;
class CCTimer;
class CCLayer;
class CCMenuItem;
class CCNotificationCenter;
class CCCallFunc;
class CCAcceleration;

namespace ui {
    class Widget;
}

// for script function
typedef struct {
    CCObject* target;
    unsigned int handler;
} ccScriptFunction;

// to collect returned value from script function
typedef void (CCObject::*SEL_ScriptReturnedValueCollector)();
#define valuecollector_selector(_SELECTOR) (SEL_ScriptReturnedValueCollector)(&_SELECTOR)

enum ccScriptType {
    kScriptTypeNone = 0,
    kScriptTypeLua,
    kScriptTypeJavascript
};
/**
 * @js NA
 * @lua NA
 */
class CCScriptHandlerEntry : public CCObject
{
public:
    static CCScriptHandlerEntry* create(ccScriptFunction func);
    ~CCScriptHandlerEntry(void);
    
    ccScriptFunction& getHandler(void) {
        return m_func;
    }
    
    int getEntryId(void) {
        return m_nEntryId;
    }
    
protected:
    CCScriptHandlerEntry(ccScriptFunction func)
    : m_func(func)
    {
        static int newEntryId = 0;
        newEntryId++;
        m_nEntryId = newEntryId;
    }
    
    ccScriptFunction m_func;
    int m_nEntryId;
};

/**
 * @addtogroup script_support
 * @{
 * @js NA
 * @lua NA
 */

class CCSchedulerScriptHandlerEntry : public CCScriptHandlerEntry
{
public:
    // nHandler return by tolua_ref_function(), called from LuaCocos2d.cpp
    static CCSchedulerScriptHandlerEntry* create(ccScriptFunction nHandler, float fInterval, int repeat, float delay, bool bPaused);
    ~CCSchedulerScriptHandlerEntry(void);
    
    cocos2d::CCTimer* getTimer(void) {
        return m_pTimer;
    }
    
    bool isPaused(void) {
        return m_bPaused;
    }
    
    void setPaused(bool flag) {
        m_bPaused = flag;
    }
    
    void markedForDeletion(void) {
        m_bMarkedForDeletion = true;
    }
    
    bool isMarkedForDeletion(void) {
        return m_bMarkedForDeletion;
    }
    
private:
    CCSchedulerScriptHandlerEntry(ccScriptFunction nHandler)
    : CCScriptHandlerEntry(nHandler)
    , m_pTimer(NULL)
    , m_bPaused(false)
    , m_bMarkedForDeletion(false)
    {
    }
    bool init(float fInterval, int repeat, float delay, bool bPaused);
    
    cocos2d::CCTimer*   m_pTimer;
    bool                m_bPaused;
    bool                m_bMarkedForDeletion;
};


/**
 * @js NA
 * @lua NA
 */
class CCTouchScriptHandlerEntry : public CCScriptHandlerEntry
{
public:
    static CCTouchScriptHandlerEntry* create(ccScriptFunction nHandler);
    ~CCTouchScriptHandlerEntry(void);
    
private:
    CCTouchScriptHandlerEntry(ccScriptFunction nHandler)
    : CCScriptHandlerEntry(nHandler)
    {
    }
    bool init();
};


// Don't make CCScriptEngineProtocol inherits from CCObject since setScriptEngine is invoked only once in AppDelegate.cpp,
// It will affect the lifecycle of ScriptCore instance, the autorelease pool will be destroyed before destructing ScriptCore.
// So a crash will appear on Win32 if you click the close button.
/**
 * @js NA
 * @lua NA
 */
class CC_DLL CCScriptEngineProtocol
{
public:
    virtual ~CCScriptEngineProtocol() {};
    
    /** Get script type */
    virtual ccScriptType getScriptType() { return kScriptTypeNone; };

    /** Remove script object. */
    virtual void removeScriptObjectByCCObject(CCObject* pObj) = 0;
    
    /** Remove script side user data */
    virtual void removeScriptUserData(int nRefId) = 0;
    
    /** Remove script function handler, only CCLuaEngine class need to implement this function. */
    virtual void removeScriptHandler(int nHandler) {};
    
    /** Reallocate script function handler, only CCLuaEngine class need to implement this function. */
    virtual int reallocateScriptHandler(int nHandler) { return -1;}
    
    /** check if two script function handlers point to same function */
    virtual bool isScriptFunctionSame(int handler1, int handler2) = 0;
    
    /**
     @brief Execute script code contained in the given string.
     @param codes holding the valid script code that should be executed.
     @return 0 if the string is executed correctly.
     @return other if the string is executed wrongly.
     */
    virtual int executeString(const char* codes) = 0;
    
    /**
     @brief Execute a script file.
     @param filename String object holding the filename of the script file that is to be executed
     */
    virtual int executeScriptFile(const char* filename) = 0;
    
    /**
     @brief Execute a scripted global function.
     @brief The function should not take any parameters and should return an integer.
     @param functionName String object holding the name of the function, in the global script environment, that is to be executed.
     @return The integer value returned from the script function.
     */
    virtual int executeGlobalFunction(const char* functionName) = 0;
    
    /** execute a callfun event */
    virtual int executeCallFuncActionEvent(CCCallFunc* pAction, CCObject* pTarget = NULL) = 0;
    /** execute a schedule function */
    virtual int executeSchedule(ccScriptFunction& func, float dt) = 0;
    
    /** functions for executing touch event */
    virtual int executeLayerTouchesEvent(CCLayer* pLayer, const char* pEventName, CCSet *pTouches) = 0;
    virtual int executeLayerTouchEvent(CCLayer* pLayer, const char* pEventName, CCTouch *pTouch) = 0;

    /** execute a accelerometer event */
    virtual int executeAccelerometerEvent(ccScriptFunction& func, CCAcceleration* pAccelerationValue) = 0;
    
    /** 
     * function for common event
     * @param func script funcation, if target is not null, it will be passed as first argument
     * @param pEventName event name
     */
    virtual int executeEvent(ccScriptFunction& func, const char* pEventName, CCObject* collector = NULL, SEL_ScriptReturnedValueCollector sel = NULL) = 0;
    
    /** 
     * function for c++ call back lua funtion 
     */
    virtual int executeEventWithArgs(ccScriptFunction& func, CCArray* pArgs, CCObject* collector = NULL, SEL_ScriptReturnedValueCollector sel = NULL) = 0;
    
    /// notify object destructor
    virtual void executeObjectDestructor(CCObject* obj) = 0;

    /** called by CCAssert to allow scripting engine to handle failed assertions
     * @return true if the assert was handled by the script engine, false otherwise.
     */
    virtual bool handleAssert(const char *msg) = 0;
    
    /**
     *
     */
    enum ConfigType
    {
        NONE,
        COCOSTUDIO,
    };
    virtual bool parseConfig(ConfigType type, const std::string& str) = 0;
};

/**
 CCScriptEngineManager is a singleton which holds an object instance of CCScriptEngineProtocl
 It helps cocos2d-x and the user code to find back LuaEngine object
 @since v0.99.5-x-0.8.5
 @js NA
 @lua NA
 */
class CC_DLL CCScriptEngineManager
{
public:
    ~CCScriptEngineManager(void);
    
    CCScriptEngineProtocol* getScriptEngine(void) {
        return m_pScriptEngine;
    }
    void setScriptEngine(CCScriptEngineProtocol *pScriptEngine);
    void removeScriptEngine(void);
    
    static CCScriptEngineManager* sharedManager(void);
    static void purgeSharedManager(void);
    
    /** set a decrypt function to decrypt script file,
     * or NULL to clear it
     */
    void setScriptDecryptFunc(CC_FILE_DECRYPT_FUNC decFunc);
    
    /// get script file decrypt function
    CC_FILE_DECRYPT_FUNC getScriptDecryptFunc();
    
private:
    CCScriptEngineManager(void)
    : m_pScriptEngine(NULL)
    , m_decFunc(NULL)
    {
    }
    
    CCScriptEngineProtocol *m_pScriptEngine;
    
    /// decrypt function
    CC_FILE_DECRYPT_FUNC m_decFunc;
};

// end of script_support group
/// @}

NS_CC_END

#endif // __SCRIPT_SUPPORT_H__
