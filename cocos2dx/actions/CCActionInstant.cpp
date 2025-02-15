/****************************************************************************
 Copyright (c) 2010-2012 cocos2d-x.org
 Copyright (c) 2008-2010 Ricardo Quesada
 Copyright (c) 2011 Zynga Inc.
 
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

#include "CCActionInstant.h"
#include "base_nodes/CCNode.h"
#include "sprite_nodes/CCSprite.h"
#include "cocoa/CCZone.h"
#include "cocoa/CCBool.h"
#include "cocoa/CCInteger.h"
#include "cocoa/CCFloat.h"
#include "cocoa/CCDouble.h"

NS_CC_BEGIN
//
// InstantAction
//
CCActionInstant::CCActionInstant() {
}

CCObject * CCActionInstant::copyWithZone(CCZone *pZone) {
    CCZone *pNewZone = NULL;
    CCActionInstant *pRet = NULL;

    if (pZone && pZone->m_pCopyObject) {
        pRet = (CCActionInstant*) (pZone->m_pCopyObject);
    } else {
        pRet = new CCActionInstant();
        pZone = pNewZone = new CCZone(pRet);
        CC_SAFE_AUTORELEASE(pRet);
    }

    CCFiniteTimeAction::copyWithZone(pZone);
    CC_SAFE_DELETE(pNewZone);
    return pRet;
}

bool CCActionInstant::isDone() {
    return true;
}

void CCActionInstant::step(float dt) {
    CC_UNUSED_PARAM(dt);
    update(1);
}

void CCActionInstant::update(float time) {
    CC_UNUSED_PARAM(time);
    // nothing
}

CCFiniteTimeAction * CCActionInstant::reverse() {
    return (CCFiniteTimeAction*)copy();
}

//
// Show
//

CCShow* CCShow::create() 
{
    CCShow* pRet = new CCShow();

    if (pRet) {
        CC_SAFE_AUTORELEASE(pRet);
    }

    return pRet;
}

void CCShow::update(float time) {
    CC_UNUSED_PARAM(time);
    m_pTarget->setVisible(true);
}

CCFiniteTimeAction* CCShow::reverse() {
    return (CCFiniteTimeAction*) (CCHide::create());
}

CCObject* CCShow::copyWithZone(CCZone *pZone) {

    CCZone *pNewZone = NULL;
    CCShow *pRet = NULL;
    if (pZone && pZone->m_pCopyObject) {
        pRet = (CCShow*) (pZone->m_pCopyObject);
    } else {
        pRet = new CCShow();
        pZone = pNewZone = new CCZone(pRet);
        CC_SAFE_AUTORELEASE(pRet);
    }

    CCActionInstant::copyWithZone(pZone);
    CC_SAFE_DELETE(pNewZone);
    return pRet;
}

//
// Hide
//
CCHide * CCHide::create() 
{
    CCHide *pRet = new CCHide();

    if (pRet) {
        CC_SAFE_AUTORELEASE(pRet);
    }

    return pRet;
}

void CCHide::update(float time) {
    CC_UNUSED_PARAM(time);
    m_pTarget->setVisible(false);
}

CCFiniteTimeAction *CCHide::reverse() {
    return (CCFiniteTimeAction*) (CCShow::create());
}

CCObject* CCHide::copyWithZone(CCZone *pZone) {
    CCZone *pNewZone = NULL;
    CCHide *pRet = NULL;

    if (pZone && pZone->m_pCopyObject) {
        pRet = (CCHide*) (pZone->m_pCopyObject);
    } else {
        pRet = new CCHide();
        pZone = pNewZone = new CCZone(pRet);
        CC_SAFE_AUTORELEASE(pRet);
    }

    CCActionInstant::copyWithZone(pZone);
    CC_SAFE_DELETE(pNewZone);
    return pRet;
}

//
// ToggleVisibility
//
CCToggleVisibility * CCToggleVisibility::create()
{
    CCToggleVisibility *pRet = new CCToggleVisibility();

    if (pRet)
    {
        CC_SAFE_AUTORELEASE(pRet);
    }

    return pRet;
}

void CCToggleVisibility::update(float time) 
{
    CC_UNUSED_PARAM(time);
    m_pTarget->setVisible(!m_pTarget->isVisible());
}

CCObject* CCToggleVisibility::copyWithZone(CCZone *pZone)
{
    CCZone *pNewZone = NULL;
    CCToggleVisibility *pRet = NULL;

    if (pZone && pZone->m_pCopyObject) {
        pRet = (CCToggleVisibility*) (pZone->m_pCopyObject);
    } else {
        pRet = new CCToggleVisibility();
        pZone = pNewZone = new CCZone(pRet);
        CC_SAFE_AUTORELEASE(pRet);
    }

    CCActionInstant::copyWithZone(pZone);
    CC_SAFE_DELETE(pNewZone);
    return pRet;
}

//
// Remove Self
//
CCRemoveSelf * CCRemoveSelf::create(bool isNeedCleanUp /*= true*/) 
{
	CCRemoveSelf *pRet = new CCRemoveSelf();

	if (pRet && pRet->init(isNeedCleanUp)) {
		CC_SAFE_AUTORELEASE(pRet);
	}

	return pRet;
}

bool CCRemoveSelf::init(bool isNeedCleanUp) {
	m_bIsNeedCleanUp = isNeedCleanUp;
	return true;
}

void CCRemoveSelf::update(float time) {
	CC_UNUSED_PARAM(time);
	m_pTarget->removeFromParentAndCleanup(m_bIsNeedCleanUp);
}

CCFiniteTimeAction *CCRemoveSelf::reverse() {
	return (CCFiniteTimeAction*) (CCRemoveSelf::create(m_bIsNeedCleanUp));
}

CCObject* CCRemoveSelf::copyWithZone(CCZone *pZone) {
	CCZone *pNewZone = NULL;
	CCRemoveSelf *pRet = NULL;

	if (pZone && pZone->m_pCopyObject) {
		pRet = (CCRemoveSelf*) (pZone->m_pCopyObject);
	} else {
		pRet = new CCRemoveSelf();
		pZone = pNewZone = new CCZone(pRet);
        CC_SAFE_AUTORELEASE(pRet);
	}

	CCActionInstant::copyWithZone(pZone);
	pRet->init(m_bIsNeedCleanUp);
	CC_SAFE_DELETE(pNewZone);
	return pRet;
}

//
// FlipX
//

CCFlipX *CCFlipX::create(bool x)
{
    CCFlipX *pRet = new CCFlipX();

    if (pRet && pRet->initWithFlipX(x)) {
        CC_SAFE_AUTORELEASE(pRet);
        return pRet;
    }

    CC_SAFE_DELETE(pRet);
    return NULL;
}

bool CCFlipX::initWithFlipX(bool x) {
    m_bFlipX = x;
    return true;
}

void CCFlipX::update(float time) {
    CC_UNUSED_PARAM(time);
    ((CCSprite*) (m_pTarget))->setFlipX(m_bFlipX);
}

CCFiniteTimeAction* CCFlipX::reverse() {
    return CCFlipX::create(!m_bFlipX);
}

CCObject * CCFlipX::copyWithZone(CCZone *pZone) {
    CCZone *pNewZone = NULL;
    CCFlipX *pRet = NULL;

    if (pZone && pZone->m_pCopyObject) {
        pRet = (CCFlipX*) (pZone->m_pCopyObject);
    } else {
        pRet = new CCFlipX();
        pZone = pNewZone = new CCZone(pRet);
        CC_SAFE_AUTORELEASE(pRet);
    }

    CCActionInstant::copyWithZone(pZone);
    pRet->initWithFlipX(m_bFlipX);
    CC_SAFE_DELETE(pNewZone);
    return pRet;
}

//
// FlipY
//

CCFlipY * CCFlipY::create(bool y)
{
    CCFlipY *pRet = new CCFlipY();

    if (pRet && pRet->initWithFlipY(y)) {
        CC_SAFE_AUTORELEASE(pRet);
        return pRet;
    }

    CC_SAFE_DELETE(pRet);
    return NULL;
}

bool CCFlipY::initWithFlipY(bool y) {
    m_bFlipY = y;
    return true;
}

void CCFlipY::update(float time) {
    CC_UNUSED_PARAM(time);
    ((CCSprite*) (m_pTarget))->setFlipY(m_bFlipY);
}

CCFiniteTimeAction* CCFlipY::reverse() {
    return CCFlipY::create(!m_bFlipY);
}

CCObject* CCFlipY::copyWithZone(CCZone *pZone) {
    CCZone *pNewZone = NULL;
    CCFlipY *pRet = NULL;

    if (pZone && pZone->m_pCopyObject) {
        pRet = (CCFlipY*) (pZone->m_pCopyObject);
    } else {
        pRet = new CCFlipY();
        pZone = pNewZone = new CCZone(pRet);
        CC_SAFE_AUTORELEASE(pRet);
    }

    CCActionInstant::copyWithZone(pZone);
    pRet->initWithFlipY(m_bFlipY);
    CC_SAFE_DELETE(pNewZone);
    return pRet;
}

//
// Place
//

CCPlace* CCPlace::create(const CCPoint& pos)
{
    CCPlace *pRet = new CCPlace();

    if (pRet && pRet->initWithPosition(pos)) {
        CC_SAFE_AUTORELEASE(pRet);
        return pRet;
    }

    CC_SAFE_DELETE(pRet);
    return NULL;
}

bool CCPlace::initWithPosition(const CCPoint& pos) {
    m_tPosition = pos;
    return true;
}

CCObject * CCPlace::copyWithZone(CCZone *pZone) {
    CCZone *pNewZone = NULL;
    CCPlace *pRet = NULL;

    if (pZone && pZone->m_pCopyObject) {
        pRet = (CCPlace*) (pZone->m_pCopyObject);
    } else {
        pRet = new CCPlace();
        pZone = pNewZone = new CCZone(pRet);
        CC_SAFE_AUTORELEASE(pRet);
    }

    CCActionInstant::copyWithZone(pZone);
    pRet->initWithPosition(m_tPosition);
    CC_SAFE_DELETE(pNewZone);
    return pRet;
}

void CCPlace::update(float time) {
    CC_UNUSED_PARAM(time);
    m_pTarget->setPosition(m_tPosition);
}

//
// CallFunc
//
CCCallFunc * CCCallFunc::create(CCObject* pSelectorTarget, SEL_CallFunc selector) 
{
    CCCallFunc *pRet = new CCCallFunc();

    if (pRet && pRet->initWithTarget(pSelectorTarget)) {
        pRet->m_pCallFunc = selector;
        CC_SAFE_AUTORELEASE(pRet);
        return pRet;
    }

    CC_SAFE_DELETE(pRet);
    return NULL;
}

CCCallFunc * CCCallFunc::create(ccScriptFunction nHandler)
{
	CCCallFunc *pRet = new CCCallFunc();

    if (pRet && pRet->initWithScriptTarget(nHandler)) {
        CC_SAFE_AUTORELEASE(pRet);
        return pRet;
    }
    
    CC_SAFE_DELETE(pRet);
    return NULL;
}

bool CCCallFunc::initWithTarget(CCObject* pSelectorTarget) {
    CC_SAFE_RETAIN(pSelectorTarget);
    CC_SAFE_RELEASE(m_pSelectorTarget);
    m_pSelectorTarget = pSelectorTarget;
    return true;
}

bool CCCallFunc::initWithScriptTarget(ccScriptFunction& func) {
    m_nScriptHandler = func;
    return true;
}

CCCallFunc::CCCallFunc()
: m_pSelectorTarget(NULL)
, m_pCallFunc(NULL)
{
    memset(&m_nScriptHandler, 0, sizeof(ccScriptFunction));
}

CCCallFunc::~CCCallFunc(void)
{
    if (m_nScriptHandler.handler) {
        cocos2d::CCScriptEngineManager::sharedManager()->getScriptEngine()->removeScriptHandler(m_nScriptHandler.handler);
    }
    CC_SAFE_RELEASE(m_pSelectorTarget);
}

void CCCallFunc::releaseLoopRetain(CCObject* srcObj) {
    // nothing to do because it doesn't hold anything
}

CCObject * CCCallFunc::copyWithZone(CCZone *pZone) {
    CCZone* pNewZone = NULL;
    CCCallFunc* pRet = NULL;

    if (pZone && pZone->m_pCopyObject) {
        //in case of being called at sub class
        pRet = (CCCallFunc*) (pZone->m_pCopyObject);
    } else {
        pRet = new CCCallFunc();
        pZone = pNewZone = new CCZone(pRet);
        CC_SAFE_AUTORELEASE(pRet);
    }

    CCActionInstant::copyWithZone(pZone);
    pRet->initWithTarget(m_pSelectorTarget);
    pRet->m_pCallFunc = m_pCallFunc;
    if (m_nScriptHandler.handler) {
        pRet->m_nScriptHandler.target = m_nScriptHandler.target;
        pRet->m_nScriptHandler.handler = cocos2d::CCScriptEngineManager::sharedManager()->getScriptEngine()->reallocateScriptHandler(m_nScriptHandler.handler);
    }
    CC_SAFE_DELETE(pNewZone);
    return pRet;
}

void CCCallFunc::update(float time) {
    CC_UNUSED_PARAM(time);
    this->execute();
}

void CCCallFunc::execute() {
    if (m_pCallFunc) {
        (m_pSelectorTarget->*m_pCallFunc)();
    }
	if (m_nScriptHandler.handler) {
		CCScriptEngineManager::sharedManager()->getScriptEngine()->executeCallFuncActionEvent(this);
	}
}

//
// CallFuncN
//
void CCCallFuncN::execute() {
    if (m_pCallFuncN) {
        (m_pSelectorTarget->*m_pCallFuncN)(m_pTarget);
    }
	if (m_nScriptHandler.handler) {
		CCScriptEngineManager::sharedManager()->getScriptEngine()->executeCallFuncActionEvent(this, m_pTarget);
	}
}

CCCallFuncN * CCCallFuncN::create(CCObject* pSelectorTarget, SEL_CallFuncN selector)
{
    CCCallFuncN *pRet = new CCCallFuncN();

    if (pRet && pRet->initWithTarget(pSelectorTarget, selector))
    {
        CC_SAFE_AUTORELEASE(pRet);
        return pRet;
    }

    CC_SAFE_DELETE(pRet);
    return NULL;
}

CCCallFuncN * CCCallFuncN::create(ccScriptFunction nHandler)
{
	CCCallFuncN *pRet = new CCCallFuncN();

	if (pRet) {
		pRet->m_nScriptHandler = nHandler;
		CC_SAFE_AUTORELEASE(pRet);
	}
	else{
		CC_SAFE_DELETE(pRet);
	}
	return pRet;
}

bool CCCallFuncN::initWithTarget(CCObject* pSelectorTarget,
        SEL_CallFuncN selector) {
    if (CCCallFunc::initWithTarget(pSelectorTarget)) {
        m_pCallFuncN = selector;
        return true;
    }

    return false;
}

CCObject * CCCallFuncN::copyWithZone(CCZone* zone) {
    CCZone* pNewZone = NULL;
    CCCallFuncN* pRet = NULL;

    if (zone && zone->m_pCopyObject) {
        //in case of being called at sub class
        pRet = (CCCallFuncN*) (zone->m_pCopyObject);
    } else {
        pRet = new CCCallFuncN();
        zone = pNewZone = new CCZone(pRet);
        CC_SAFE_AUTORELEASE(pRet);
    }

    CCCallFunc::copyWithZone(zone);
    pRet->initWithTarget(m_pSelectorTarget, m_pCallFuncN);
    CC_SAFE_DELETE(pNewZone);
    return pRet;
}

//
// CallFuncND
//

CCCallFuncND * CCCallFuncND::create(CCObject* pSelectorTarget, SEL_CallFuncND selector, void* d)
{
    CCCallFuncND* pRet = new CCCallFuncND();

    if (pRet && pRet->initWithTarget(pSelectorTarget, selector, d)) {
        CC_SAFE_AUTORELEASE(pRet);
        return pRet;
    }

    CC_SAFE_DELETE(pRet);
    return NULL;
}

bool CCCallFuncND::initWithTarget(CCObject* pSelectorTarget,
        SEL_CallFuncND selector, void* d) {
    if (CCCallFunc::initWithTarget(pSelectorTarget)) {
        m_pData = d;
        m_pCallFuncND = selector;
        return true;
    }

    return false;
}

CCObject * CCCallFuncND::copyWithZone(CCZone* zone) {
    CCZone* pNewZone = NULL;
    CCCallFuncND* pRet = NULL;

    if (zone && zone->m_pCopyObject) {
        //in case of being called at sub class
        pRet = (CCCallFuncND*) (zone->m_pCopyObject);
    } else {
        pRet = new CCCallFuncND();
        zone = pNewZone = new CCZone(pRet);
        CC_SAFE_AUTORELEASE(pRet);
    }

    CCCallFunc::copyWithZone(zone);
    pRet->initWithTarget(m_pSelectorTarget, m_pCallFuncND, m_pData);
    CC_SAFE_DELETE(pNewZone);
    return pRet;
}

void CCCallFuncND::execute() {
    if (m_pCallFuncND) {
        (m_pSelectorTarget->*m_pCallFuncND)(m_pTarget, m_pData);
    }
}

//
// CCCallFuncO
//
CCCallFuncO::CCCallFuncO() :
m_pObject(NULL),
m_needRelease(true) {
}

CCCallFuncO::~CCCallFuncO() {
    if(m_needRelease) {
        CC_SAFE_RELEASE(m_pObject);
    }
}

void CCCallFuncO::releaseLoopRetain(CCObject* srcObj) {
    if(m_needRelease && m_pObject == srcObj) {
        CC_SAFE_RELEASE(m_pObject);
        m_needRelease = false;
    }
}

void CCCallFuncO::execute() {
    if (m_pCallFuncO) {
        (m_pSelectorTarget->*m_pCallFuncO)(m_pObject);
    }
    if (m_nScriptHandler.handler) {
        CCScriptEngineManager::sharedManager()->getScriptEngine()->executeCallFuncActionEvent(this, m_pObject);
    }
}

CCCallFuncO * CCCallFuncO::create(CCObject* pSelectorTarget, SEL_CallFuncO selector, CCObject* pObject)
{
    CCCallFuncO *pRet = new CCCallFuncO();

    if (pRet && pRet->initWithTarget(pSelectorTarget, selector, pObject)) {
        CC_SAFE_AUTORELEASE(pRet);
        return pRet;
    }

    CC_SAFE_DELETE(pRet);
    return NULL;
}

CCCallFuncO* CCCallFuncO::create(ccScriptFunction func, CCObject* pObject) {
    CCCallFuncO *pRet = new CCCallFuncO();
    if (pRet && pRet->initWithScriptTarget(func, pObject)) {
        CC_SAFE_AUTORELEASE(pRet);
        return pRet;
    }
    
    CC_SAFE_DELETE(pRet);
    return NULL;
    return pRet;
}

CCCallFuncO * CCCallFuncO::createWithString(ccScriptFunction func, const std::string& s) {
    return CCCallFuncO::create(func, CCString::create(s));
}

CCCallFuncO * CCCallFuncO::createWithBool(ccScriptFunction func, bool flag) {
    return CCCallFuncO::create(func, CCBool::create(flag));
}

CCCallFuncO * CCCallFuncO::createWithInt(ccScriptFunction func, int flag) {
    return CCCallFuncO::create(func, CCInteger::create(flag));
}

CCCallFuncO * CCCallFuncO::createWithFloat(ccScriptFunction func, float flag) {
    return CCCallFuncO::create(func, CCFloat::create(flag));
}

CCCallFuncO * CCCallFuncO::createWithDouble(ccScriptFunction func, double flag) {
    return CCCallFuncO::create(func, CCDouble::create(flag));
}

bool CCCallFuncO::initWithScriptTarget(ccScriptFunction& func, CCObject* pObject) {
    if (CCCallFunc::initWithScriptTarget(func)) {
        m_pObject = pObject;
        CC_SAFE_RETAIN(m_pObject);
        return true;
    }
    
    return false;
}

bool CCCallFuncO::initWithTarget(CCObject* pSelectorTarget,
        SEL_CallFuncO selector, CCObject* pObject) {
    if (CCCallFunc::initWithTarget(pSelectorTarget)) {
        m_pObject = pObject;
        CC_SAFE_RETAIN(m_pObject);

        m_pCallFuncO = selector;
        return true;
    }

    return false;
}

CCObject * CCCallFuncO::copyWithZone(CCZone* zone) {
    CCZone* pNewZone = NULL;
    CCCallFuncO* pRet = NULL;

    if (zone && zone->m_pCopyObject) {
        //in case of being called at sub class
        pRet = (CCCallFuncO*) (zone->m_pCopyObject);
    } else {
        pRet = new CCCallFuncO();
        zone = pNewZone = new CCZone(pRet);
        CC_SAFE_AUTORELEASE(pRet);
    }

    CCCallFunc::copyWithZone(zone);
    pRet->initWithTarget(m_pSelectorTarget, m_pCallFuncO, m_pObject);
    CC_SAFE_DELETE(pNewZone);
    return pRet;
}

NS_CC_END
