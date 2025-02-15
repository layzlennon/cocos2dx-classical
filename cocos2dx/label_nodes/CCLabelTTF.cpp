/****************************************************************************
 Author: Luma (stubma@gmail.com)
 
 https://github.com/stubma/cocos2dx-classical
 
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
#include "CCLabelTTF.h"
#include "CCDirector.h"
#include "shaders/CCGLProgram.h"
#include "shaders/CCShaderCache.h"
#include "textures/CCTexture2D.h"
#include "CCImage.h"
#include "menu_nodes/CCMenuItemColor.h"
#include "CCLabelTTFLinkStateSynchronizer.h"
#include "textures/CCTextureCache.h"
#include "menu_nodes/CCMenu.h"
#include "actions/CCActionInstant.h"
#include "cocoa/CCPointExtension.h"

NS_CC_BEGIN

#if CC_USE_LA88_LABELS
#define SHADER_PROGRAM kCCShader_PositionTextureColor
#else
#define SHADER_PROGRAM kCCShader_PositionTextureA8Color
#endif

// the start tag of menu item, for link tag
#define START_TAG_LINK_ITEM 0x80000
#define TAG_MENU 0x70000

//
//CCLabelTTF
//
CCLabelTTF::CCLabelTTF() :
m_hAlignment(kCCTextAlignmentCenter),
m_vAlignment(kCCVerticalTextAlignmentTop),
m_pFontName(NULL),
m_fFontSize(0.0),
m_realLength(0),
m_lineSpacing(0),
m_elapsed(0),
m_string(""),
m_updateScheduled(false),
m_shadowEnabled(false),
m_shadowColor(0xff333333),
m_strokeEnabled(false),
m_textFillColor(ccWHITE),
m_globalImageScaleFactor(1),
m_stateListener(NULL),
m_toCharIndex(-1),
m_defaultTarget(NULL),
m_loopFunc(NULL),
m_textChanging(true) {
    m_stateListener = new CCLabelTTFLinkStateSynchronizer(this);
}

CCLabelTTF::~CCLabelTTF() {
    CC_SAFE_DELETE(m_pFontName);
    CC_SAFE_RELEASE(m_loopFunc);
    CC_SAFE_RELEASE(m_defaultTarget);
    
    // release callfunc
    CCMenu* menu = (CCMenu*)getChildByTag(TAG_MENU);
    if(menu) {
        CCObject* obj;
        CCARRAY_FOREACH(menu->getChildren(), obj) {
            CCNode* item = (CCNode*)obj;
            CCObject* data = (CCObject*)item->getUserData();
            CC_SAFE_RELEASE(data);
        }
    }
    
    // release other
    CC_SAFE_RELEASE(m_stateListener);
}

CCLabelTTF * CCLabelTTF::create()
{
    CCLabelTTF * pRet = new CCLabelTTF();
    if (pRet && pRet->init())
    {
        CC_SAFE_AUTORELEASE(pRet);
    }
    else
    {
        CC_SAFE_DELETE(pRet);
    }
    return pRet;
}

CCLabelTTF * CCLabelTTF::createWithFontDefinition(const char *string, ccFontDefinition &textDefinition) {
    CCLabelTTF *pRet = new CCLabelTTF();
    if(pRet && pRet->initWithStringAndTextDefinition(string, textDefinition))
    {
        CC_SAFE_AUTORELEASE(pRet);
        return pRet;
    }
    CC_SAFE_DELETE(pRet);
    return NULL;
}

CCLabelTTF * CCLabelTTF::create(const char *string, const char *fontName, float fontSize)
{
    return CCLabelTTF::create(string, fontName, fontSize,
                                  CCSizeZero, kCCTextAlignmentCenter, kCCVerticalTextAlignmentTop);
}

CCLabelTTF * CCLabelTTF::create(const char *string, const char *fontName, float fontSize,
                                        const CCSize& dimensions, CCTextAlignment hAlignment)
{
    return CCLabelTTF::create(string, fontName, fontSize, dimensions, hAlignment, kCCVerticalTextAlignmentTop);
}

CCLabelTTF* CCLabelTTF::create(const char *string, const char *fontName, float fontSize,
                                       const CCSize &dimensions, CCTextAlignment hAlignment,
                                       CCVerticalTextAlignment vAlignment)
{
    CCLabelTTF *pRet = new CCLabelTTF();
    if(pRet && pRet->initWithString(string, strcmp(fontName, "") == 0 ? "Helvetica" : fontName , fontSize, dimensions, hAlignment, vAlignment))
    {
        CC_SAFE_AUTORELEASE(pRet);
        return pRet;
    }
    CC_SAFE_DELETE(pRet);
    return NULL;
}

bool CCLabelTTF::init()
{
    return this->initWithString("", "Helvetica", 12);
}

bool CCLabelTTF::initWithString(const char *label, const char *fontName, float fontSize,
                                    const CCSize& dimensions, CCTextAlignment alignment)
{
    return this->initWithString(label, fontName, fontSize, dimensions, alignment, kCCVerticalTextAlignmentTop);
}

bool CCLabelTTF::initWithString(const char *label, const char *fontName, float fontSize)
{
    return this->initWithString(label, fontName, fontSize,
                                CCSizeZero, kCCTextAlignmentLeft, kCCVerticalTextAlignmentTop);
}

bool CCLabelTTF::initWithString(const char *string, const char *fontName, float fontSize,
                                    const CCSize &dimensions, CCTextAlignment hAlignment,
                                    CCVerticalTextAlignment vAlignment)
{
    if (CCGradientSprite::init())
    {
        // shader program
        this->setShaderProgram(CCShaderCache::sharedShaderCache()->programForKey(SHADER_PROGRAM));
        
        m_tDimensions = CCSizeMake(dimensions.width, dimensions.height);
        m_hAlignment  = hAlignment;
        m_vAlignment  = vAlignment;
        m_pFontName   = new std::string(fontName);
        m_fFontSize   = fontSize;
        
        this->setString(string);
        
        return true;
    }
    
    return false;
}

bool CCLabelTTF::initWithStringAndTextDefinition(const char *string, ccFontDefinition &textDefinition)
{
    if (CCGradientSprite::init())
    {
        // shader program
        this->setShaderProgram(CCShaderCache::sharedShaderCache()->programForKey(SHADER_PROGRAM));
        
        // prepare everythin needed to render the label
        _updateWithTextDefinition(textDefinition, false);
        
        // set the string
        this->setString(string);
        
        //
        return true;
    }
    else
    {
        return false;
    }
}


void CCLabelTTF::setString(const char *string)
{
    CCAssert(string != NULL, "Invalid string");
    
    if (m_string.compare(string))
    {
        m_string = string;
        m_textChanging = true;
        this->updateTexture();
    }
}

const char* CCLabelTTF::getString(void)
{
    return m_string.c_str();
}

void CCLabelTTF::setText(const char *label) {
    setString(label);
}

const char* CCLabelTTF::getText(void) {
    return getString();
}

const char* CCLabelTTF::description()
{
    return CCString::createWithFormat("<CCLabelTTF | FontName = %s, FontSize = %.1f>", m_pFontName->c_str(), m_fFontSize)->getCString();
}

CCTextAlignment CCLabelTTF::getHorizontalAlignment()
{
    return m_hAlignment;
}

void CCLabelTTF::setHorizontalAlignment(CCTextAlignment alignment)
{
    if (alignment != m_hAlignment)
    {
        m_hAlignment = alignment;
        
        // Force update
        if (m_string.size() > 0)
        {
            this->updateTexture();
        }
    }
}

CCVerticalTextAlignment CCLabelTTF::getVerticalAlignment()
{
    return m_vAlignment;
}

void CCLabelTTF::setVerticalAlignment(CCVerticalTextAlignment verticalAlignment)
{
    if (verticalAlignment != m_vAlignment)
    {
        m_vAlignment = verticalAlignment;
        
        // Force update
        if (m_string.size() > 0)
        {
            this->updateTexture();
        }
    }
}

CCSize CCLabelTTF::getDimensions()
{
    return m_tDimensions;
}

void CCLabelTTF::setDimensions(const CCSize &dim)
{
    if (dim.width != m_tDimensions.width || dim.height != m_tDimensions.height)
    {
        m_tDimensions = dim;
        
        // Force update
        if (m_string.size() > 0)
        {
            this->updateTexture();
        }
    }
}

float CCLabelTTF::getFontSize()
{
    return m_fFontSize;
}

void CCLabelTTF::setFontSize(float fontSize)
{
    if (m_fFontSize != fontSize)
    {
        m_fFontSize = fontSize;
        
        // Force update
        if (m_string.size() > 0)
        {
            this->updateTexture();
        }
    }
}

const char* CCLabelTTF::getFontName()
{
    return m_pFontName->c_str();
}

void CCLabelTTF::setFontName(const char *fontName)
{
    if (m_pFontName->compare(fontName))
    {
        delete m_pFontName;
        m_pFontName = new std::string(fontName);
        
        // Force update
        if (m_string.size() > 0)
        {
            this->updateTexture();
        }
    }
}

void CCLabelTTF::setGradientColor(const ccColor4B& start, const ccColor4B& end, const CCPoint& v) {
    CCGradientSprite::setGradientColor(start, end, v);
}

// Helper
bool CCLabelTTF::updateTexture()
{
    CCTexture2D *tex;
    tex = new CCTexture2D();
    
    if (!tex)
        return false;
    
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS || CC_TARGET_PLATFORM == CC_PLATFORM_MAC
    
    ccFontDefinition texDef = _prepareTextDefinition(true);
    tex->initWithString( m_string.c_str(), &texDef );
    
#else
    
    tex->initWithString( m_string.c_str(),
                        m_pFontName->c_str(),
                        m_fFontSize * CC_CONTENT_SCALE_FACTOR(),
                        CC_SIZE_POINTS_TO_PIXELS(m_tDimensions),
                        m_hAlignment,
                        m_vAlignment);
    
#endif
    
    // set the texture
    this->setTexture(tex);
    
    // release it
    CC_SAFE_RELEASE(tex);
    
    // set the size in the sprite
    CCRect rect =CCRectZero;
    rect.size   = m_pobTexture->getContentSize();
    this->setTextureRect(rect);
    
    // save length
    m_realLength = tex->getRealLength();
    
    // save image rects
    const vector<CCRect>& imageRects = tex->getImageRects();
    m_imageRects.clear();
    m_imageRects.insert(m_imageRects.begin(), imageRects.begin(), imageRects.end());
    for(vector<CCRect>::iterator iter = m_imageRects.begin(); iter != m_imageRects.end(); iter++) {
        CCRect& r = *iter;
        r.origin.x /= CC_CONTENT_SCALE_FACTOR();
        r.origin.y /= CC_CONTENT_SCALE_FACTOR();
        r.size.width /= CC_CONTENT_SCALE_FACTOR();
        r.size.height /= CC_CONTENT_SCALE_FACTOR();
    }
    
    // create link menu
    CCMenu* menu = (CCMenu*)getChildByTag(TAG_MENU);
    const LinkMetaList& linkMetas = tex->getLinkMetas();
    if(!linkMetas.empty() && m_textChanging) {
        // delete old menu if has
        if(menu) {
            menu->removeFromParent();
            menu = NULL;
        }
        
        // now create items for every link
        CCArray* items = CCArray::create();
        for(LinkMetaList::const_iterator iter = linkMetas.begin(); iter != linkMetas.end(); iter++) {
            const LinkMeta& meta = *iter;
            CCMenuItemColor* item = CCMenuItemColor::create(ccc4FromInt(meta.normalBgColor),
                                                            ccc4FromInt(meta.selectedBgColor),
                                                            this,
                                                            menu_selector(CCLabelTTF::onLinkMenuItemClicked));
            item->setTag(START_TAG_LINK_ITEM + meta.tag);
            item->setPosition(ccp(meta.x + meta.width / 2,
                                  meta.y + meta.height / 2));
            item->setContentSize(CCSizeMake(meta.width, meta.height));
            item->setStateListener(m_stateListener);
            items->addObject(item);
        }
        
        // add menu in -1 z order so that it won't override label UI
        CCMenu* newMenu = CCMenu::createWithArray(items);
        newMenu->setPosition(CCPointZero);
        addChild(newMenu, -1, TAG_MENU);
        
        // clear flag
        m_textChanging = false;
    }
    
    // sync shadow stroke padding to link menu
    // the padding must divide scale factor. Because at this time, texture already scaled for label
    menu = (CCMenu*)getChildByTag(TAG_MENU);
    if(menu) {
        const CCPoint& p = tex->getShadowStrokePadding();
        menu->setPosition(ccp(p.x / CC_CONTENT_SCALE_FACTOR(),
                              p.y / CC_CONTENT_SCALE_FACTOR()));
    }
    
    // if has color transition or something else which need time, start to update
    if(tex->isNeedTime()) {
        if(!m_updateScheduled) {
            scheduleUpdate();
            m_updateScheduled = true;
        }
    } else {
        if(m_updateScheduled) {
            unscheduleUpdate();
            m_updateScheduled = false;
        }
    }
    
    //ok
    return true;
}

CCRect CCLabelTTF::getImageBound(int index) {
    if(index < 0 || index >= m_imageRects.size())
        return CCRectZero;
    
    return m_imageRects.at(index);
}

CCRect CCLabelTTF::getImageBoundInParentSpace(int index) {
    CCRect r = getImageBound(index);
    CCAffineTransform t = nodeToParentTransform();
    r = CCRectApplyAffineTransform(r, t);
    return r;
}

CCRect CCLabelTTF::getImageBoundInWorldSpace(int index) {
    CCRect r = getImageBound(index);
    CCAffineTransform t = nodeToWorldTransform();
    r = CCRectApplyAffineTransform(r, t);
    return r;
}

void CCLabelTTF::enableShadow(const CCSize &shadowOffset, unsigned int shadowColor, float shadowBlur, bool updateTexture)
{
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS || CC_TARGET_PLATFORM == CC_PLATFORM_MAC
    
    bool valueChanged = false;
    
    if (false == m_shadowEnabled)
    {
        m_shadowEnabled = true;
        valueChanged    = true;
    }
    
    if ( (m_shadowOffset.width != shadowOffset.width) || (m_shadowOffset.height!=shadowOffset.height) )
    {
        m_shadowOffset.width  = shadowOffset.width;
        m_shadowOffset.height = shadowOffset.height;
        
        valueChanged = true;
    }
    
    if(m_shadowColor != shadowColor) {
        m_shadowColor = shadowColor;
        valueChanged = true;
    }
    
    if (m_shadowBlur    != shadowBlur)
    {
        m_shadowBlur = shadowBlur;
        valueChanged = true;
    }
    
    
    if ( valueChanged && updateTexture )
    {
        this->updateTexture();
    }
    
#else
    CCAssert(false, "Operation is not supported for your platform");
#endif
    
}

void CCLabelTTF::disableShadow(bool updateTexture)
{
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS || CC_TARGET_PLATFORM == CC_PLATFORM_MAC
    
    if (m_shadowEnabled)
    {
        m_shadowEnabled = false;
        
        if (updateTexture)
            this->updateTexture();
        
    }
    
#else
    CCAssert(false, "Operation is not supported for your platform");
#endif
}

void CCLabelTTF::enableStroke(const ccColor3B &strokeColor, float strokeSize, bool updateTexture)
{
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS || CC_TARGET_PLATFORM == CC_PLATFORM_MAC
    bool valueChanged = false;
    
    if(m_strokeEnabled == false)
    {
        m_strokeEnabled = true;
        valueChanged = true;
    }
    
    if ( (m_strokeColor.r != strokeColor.r) || (m_strokeColor.g != strokeColor.g) || (m_strokeColor.b != strokeColor.b) )
    {
        m_strokeColor = strokeColor;
        valueChanged = true;
    }
    
    if (m_strokeSize!=strokeSize)
    {
        m_strokeSize = strokeSize;
        valueChanged = true;
    }
    
    if ( valueChanged && updateTexture )
    {
        this->updateTexture();
    }
    
#else
    CCAssert(false, "Operation is not supported for your platform");
#endif
    
}

void CCLabelTTF::disableStroke(bool updateTexture)
{
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS || CC_TARGET_PLATFORM == CC_PLATFORM_MAC
    
    if (m_strokeEnabled)
    {
        m_strokeEnabled = false;
        
        if (updateTexture)
            this->updateTexture();
    }
    
#else
    CCAssert(false, "Operation is not supported for your platform");
#endif
}

void CCLabelTTF::setColor(const ccColor3B &tintColor)
{
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS || CC_TARGET_PLATFORM == CC_PLATFORM_MAC
    if (m_textFillColor.r != tintColor.r || m_textFillColor.g != tintColor.g || m_textFillColor.b != tintColor.b) {
        m_textFillColor = tintColor;
        updateTexture();
    }
#else
    CCAssert(false, "Operation is not supported for your platform");
#endif
}

void CCLabelTTF::setTextDefinition(ccFontDefinition *theDefinition)
{
    if (theDefinition)
    {
        _updateWithTextDefinition(*theDefinition, true);
    }
}

ccFontDefinition *CCLabelTTF::getTextDefinition()
{
    ccFontDefinition *tempDefinition = new ccFontDefinition;
    *tempDefinition = _prepareTextDefinition(false);
    return tempDefinition;
}

void CCLabelTTF::setGlobalImageScaleFactor(float scale, bool mustUpdateTexture) {
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID || CC_TARGET_PLATFORM == CC_PLATFORM_IOS || CC_TARGET_PLATFORM == CC_PLATFORM_MAC
    if (m_globalImageScaleFactor != scale) {
        m_globalImageScaleFactor = scale;
        
        if (mustUpdateTexture)
            this->updateTexture();
    }
#else
    CCAssert(false, "Operation is not supported for your platform");
#endif
}

void CCLabelTTF::setLineSpacing(float s, bool mustUpdateTexture) {
    m_lineSpacing = s;
    
    // set this flag so that link item can be updated
    m_textChanging = true;
    
    if (mustUpdateTexture)
        this->updateTexture();
}

void CCLabelTTF::_updateWithTextDefinition(ccFontDefinition & textDefinition, bool mustUpdateTexture)
{
    m_tDimensions = CCSizeMake(textDefinition.m_dimensions.width, textDefinition.m_dimensions.height);
    m_hAlignment  = textDefinition.m_alignment;
    m_vAlignment  = textDefinition.m_vertAlignment;
    m_globalImageScaleFactor = textDefinition.m_globalImageScaleFactor;
    m_pFontName   = new std::string(textDefinition.m_fontName);
    m_fFontSize   = textDefinition.m_fontSize;
    m_toCharIndex = textDefinition.m_toCharIndex;
    
    // shadow
    if ( textDefinition.m_shadow.m_shadowEnabled )
    {
        enableShadow(textDefinition.m_shadow.m_shadowOffset, textDefinition.m_shadowColor, false);
    }
    
    // stroke
    if ( textDefinition.m_stroke.m_strokeEnabled )
    {
        enableStroke(textDefinition.m_stroke.m_strokeColor, textDefinition.m_stroke.m_strokeSize, false);
    }
    
    // fill color
    setColor(textDefinition.m_fontFillColor);
    
    if (mustUpdateTexture)
        updateTexture();
}

ccFontDefinition CCLabelTTF::_prepareTextDefinition(bool adjustForResolution)
{
    ccFontDefinition texDef;
    
    texDef.m_globalImageScaleFactor = m_globalImageScaleFactor;
    texDef.m_elapsed = m_elapsed;
    
    if (adjustForResolution)
        texDef.m_fontSize       =  m_fFontSize * CC_CONTENT_SCALE_FACTOR();
    else
        texDef.m_fontSize       =  m_fFontSize;
    
    texDef.m_fontName       = *m_pFontName;
    texDef.m_alignment      =  m_hAlignment;
    texDef.m_vertAlignment  =  m_vAlignment;
    texDef.m_toCharIndex = m_toCharIndex;
    texDef.m_lineSpacing = m_lineSpacing;
    
    if (adjustForResolution)
        texDef.m_dimensions     =  CC_SIZE_POINTS_TO_PIXELS(m_tDimensions);
    else
        texDef.m_dimensions     =  m_tDimensions;
    
    
    // stroke
    if ( m_strokeEnabled )
    {
        texDef.m_stroke.m_strokeEnabled = true;
        texDef.m_stroke.m_strokeColor   = m_strokeColor;
        
        if (adjustForResolution)
            texDef.m_stroke.m_strokeSize = m_strokeSize * CC_CONTENT_SCALE_FACTOR();
        else
            texDef.m_stroke.m_strokeSize = m_strokeSize;
        
        
    }
    else
    {
        texDef.m_stroke.m_strokeEnabled = false;
    }
    
    
    // shadow
    if ( m_shadowEnabled )
    {
        texDef.m_shadow.m_shadowEnabled         = true;
        texDef.m_shadow.m_shadowBlur            = m_shadowBlur;
        texDef.m_shadow.m_shadowOpacity         = m_shadowOpacity;
        texDef.m_shadowColor           = m_shadowColor;
        
        if (adjustForResolution)
            texDef.m_shadow.m_shadowOffset = CC_SIZE_POINTS_TO_PIXELS(m_shadowOffset);
        else
            texDef.m_shadow.m_shadowOffset = m_shadowOffset;
    }
    else
    {
        texDef.m_shadow.m_shadowEnabled = false;
    }
    
    // text tint
    texDef.m_fontFillColor = m_textFillColor;
    
    return texDef;
}

void CCLabelTTF::setLinkTarget(int index, CCCallFunc* func) {
    if(func) {
        char buf[64];
        sprintf(buf, "%d", index);
        m_linkTargets.setObject(func, buf);
        func->releaseLoopRetain(this);
    }
}

void CCLabelTTF::setLinkTargetForAll(CCCallFunc* func) {
    CC_SAFE_RETAIN(func);
    CC_SAFE_RELEASE(m_defaultTarget);
    m_defaultTarget = func;
    if(func) {
        func->releaseLoopRetain(this);
    }
}

void CCLabelTTF::onLinkMenuItemClicked(CCObject* sender) {
    CCMenuItemColor* item = (CCMenuItemColor*)sender;
    int index = item->getTag() - START_TAG_LINK_ITEM;
    char buf[64];
    sprintf(buf, "%d", index);
    CCCallFunc* func = (CCCallFunc*)m_linkTargets.objectForKey(buf);
    if(func)
        func->execute();
    else if(m_defaultTarget)
        m_defaultTarget->execute();
}

void CCLabelTTF::setLinkPriority(int p) {
    // if not found, do nothing
    CCMenu* menu = (CCMenu*)getChildByTag(TAG_MENU);
    if(!menu)
        return;
    
    menu->setTouchPriority(p);
}

void CCLabelTTF::startLoopDisplay(float interval, unsigned int repeat, int delay, CCCallFunc* loopFunc) {
    // save loop func
    CC_SAFE_RETAIN(loopFunc);
    CC_SAFE_RELEASE(m_loopFunc);
    m_loopFunc = loopFunc;
    if(loopFunc) {
        loopFunc->releaseLoopRetain(this);
    }
    
    // init state, because we can't display one char, so at the beginning we hide it
    setVisible(false);
    m_toCharIndex = 0;
    m_repeat = repeat;
    
    // schedule update
    schedule(schedule_selector(CCLabelTTF::displayNextChar), interval, kCCRepeatForever, delay);
}

void CCLabelTTF::stopLoopDisplay() {
    // unschedule
    unschedule(schedule_selector(CCLabelTTF::displayNextChar));
    
    // reset
    setVisible(true);
    setDisplayTo(-1);
    
    // release
    CC_SAFE_RELEASE_NULL(m_loopFunc);
}

void CCLabelTTF::displayNextChar(float delta) {
    // display next
    if(m_toCharIndex < m_realLength) {
        if(m_toCharIndex == -1) {
            m_toCharIndex++;
            setVisible(false);
        } else if(m_toCharIndex == 0) {
            setVisible(true);
            setDisplayTo(m_toCharIndex + 1);
        } else {
            setDisplayTo(m_toCharIndex + 1);
        }
    }
    
    // check complete
    if(m_toCharIndex >= m_realLength) {
        // callback
        if(m_loopFunc)
            m_loopFunc->execute();
        
        // if repeat, reset
        // if not, restore state and release callback
        if(m_repeat == kCCRepeatForever || m_repeat > 0) {
            m_repeat--;
            m_toCharIndex = -1;
        } else {
            unschedule(schedule_selector(CCLabelTTF::displayNextChar));
            setDisplayTo(-1);
            CC_SAFE_RELEASE_NULL(m_loopFunc);
        }
    }
}

void CCLabelTTF::setDisplayTo(int to) {
    if (m_toCharIndex != to) {
        m_toCharIndex = to;
        
        // Force update
        if (m_string.size() > 0) {
            updateTexture();
        }
    }
}

void CCLabelTTF::update(float delta) {
    m_elapsed += delta;
    updateTexture();
}

NS_CC_END
