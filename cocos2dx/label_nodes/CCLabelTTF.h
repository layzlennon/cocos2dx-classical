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
#ifndef __CCLabelTTF__
#define __CCLabelTTF__

#include "sprite_nodes/CCGradientSprite.h"
#include "ccTypes.h"
#include "cocoa/CCDictionary.h"

using namespace std;

NS_CC_BEGIN

class CCTexture2D;
class CCLabelTTFLinkStateSynchronizer;

/**
 * TTF label which support different style in a string. By using a tag description such as:
 * 'Hell[color=ffff0000]o[/color]", the 'o' character will be in red color. If you want
 * a '[' character, use '\' to escape.
 *
 * \par
 * CCLabelTFF won't perform any format validation, so you must ensure the text is
 * correct. If text is wrong, no warning will be shown and final result will be unpredictable.
 *
 * \par
 * supported tags:
 * [color=aarrggbb][/color]: set text color, also can set color transition effect, optional properties:
 *      1. to=aarrggbb: means the color will transit to, by default it is transparent color
 *      2. duration=float: how long does color transition finish, this argument must be set otherwise color transition won't work
 *      3. transient=true/false: change to dest color immediately or gradually, by default it is false
 * [font=font name or path][/font]: set font
 * [size=integer][/size]: set font size
 * [b][/b]: bold font
 * [i][/i]: italic font
 * [u][/u]: underline
 * [image=image path][/image]: embed a image, The text between image tag will be ignored, no matter how long it is.
 *      Other tags should not be embedded inside an image tag, crash may happen if you do that.
 *      If image path starts with a '/', it is an absolute path and no preprocessing.
 *      If image path doesn't start with a '/', it is a relative path and it will be preprocessed by CCUtils::getExternalOrFullPath
 *      Optional attributes: scale, scalex, scaley, w, h, offsety.
 *      In iOS, the absolute path will be redirected to ~/Library, so "/sdcard/a.png" will be "~/Library/sdcard/a.png"
 *			1. w/h: If w is specified, x scale property will be ignored. If h is specified, y scale property will be ignored.
 *			2. scale/scalex/scaley: image scale, if scale x and y is same, you can just set scale
 *			3. offsety: image vertical offset, following opengl rule. So positive value will move image up.
 *			4. plist/atlas: when those two attributes are specified, the image path will be treated as a frame name in atlas plist
 *		The image name can be empty or anything not existent if you just want to add a placeholder
 * [link bg=aarrggbb bg_click=aarrggbb][/link]: set one segment can be clicked as a hyperlink. bg and bg_click are
 *		optional and they will be transparent color if not set. Link tag doesn't add any decoration, you must use other
 *		tag to add visual style for clickable area.
 *
 * \par
 * CCLabelTFF is a subclass of CCGradientSprite, so it also can set a gradient
 * effect.
 *
 * \par
 * Android version will hold a bitmap for encountered atlas, but it only holds last one. If a new atlas image is
 * parsed, previous one will be released and current will be held. For better performance, you should put images which
 * will be embedded in a rich label into one atlas image.
 *
 * \note
 * Currently it only supports iOS and Android, please do it yourself if you want other platform.
 */
class CC_DLL CCLabelTTF : public CCGradientSprite, public CCLabelProtocol {
private:
    /// menu item state listener
    CCLabelTTFLinkStateSynchronizer* m_stateListener;
    
    /// rectangle of every embeded image
    vector<CCRect> m_imageRects;
    
    /// length of unstyled string
    int m_realLength;
    
    /// link target cache
    CCDictionary m_linkTargets;
    
    /// default target for all link items (if no function in link target map)
    CCCallFunc* m_defaultTarget;
    
protected:
    // when link item is clicked
    void onLinkMenuItemClicked(CCObject* sender);
    
public:
    CCLabelTTF();
    virtual ~CCLabelTTF();
    
    /**
     * Creates an label.
     */
    static CCLabelTTF* create();
    
    /**
     * creates a CCLabelTTF with a font name and font size in points
     */
    static CCLabelTTF* create(const char *s, const char *fontName, float fontSize);
    
    /**
     * creates a CCLabelTTF from a fontname, horizontal alignment, dimension in points,  and font size in points.
     */
    static CCLabelTTF* create(const char *s, const char *fontName, float fontSize,
                              const CCSize& dimensions, CCTextAlignment hAlignment);
    
    /**
     * creates a CCLabelTTF from a fontname, alignment, dimension in points and font size in points
     */
    static CCLabelTTF* create(const char *s, const char *fontName, float fontSize,
                              const CCSize& dimensions, CCTextAlignment hAlignment,
                              CCVerticalTextAlignment vAlignment);
    
    /** Create a lable with string and a font definition*/
    static CCLabelTTF* createWithFontDefinition(const char* s, ccFontDefinition &textDefinition);
    
    const char* description();
    
    /** initializes the CCLabelTTF with a font name and font size */
    virtual bool initWithString(const char * s, const char *fontName, float fontSize);
    
    /** initializes the CCLabelTTF with a font name, alignment, dimension and font size */
    virtual bool initWithString(const char * s, const char *fontName, float fontSize,
                        const CCSize& dimensions, CCTextAlignment hAlignment);
    
    /** initializes the CCLabelTTF with a font name, alignment, dimension and font size */
    virtual bool initWithString(const char * s, const char *fontName, float fontSize,
                        const CCSize& dimensions, CCTextAlignment hAlignment,
                        CCVerticalTextAlignment vAlignment);
    
    /** initializes the CCLabelTTF with a font name, alignment, dimension and font size */
    virtual bool initWithStringAndTextDefinition(const char * s, ccFontDefinition &textDefinition);
    
    /** set the text definition used by this label */
    void setTextDefinition(ccFontDefinition *theDefinition);
    
    /** get the text definition used by this label */
    ccFontDefinition * getTextDefinition();
    
    
    
    /** enable or disable shadow for the label */
    void enableShadow(const CCSize &shadowOffset, unsigned int shadowColor, float shadowBlur, bool mustUpdateTexture = true);
    
    /** disable shadow rendering */
    void disableShadow(bool mustUpdateTexture = true);
    
    /** enable or disable stroke */
    void enableStroke(const ccColor3B &strokeColor, float strokeSize, bool mustUpdateTexture = true);
    
    /** disable stroke */
    void disableStroke(bool mustUpdateTexture = true);
    
    /** initializes the CCLabelTTF */
    virtual bool init();
    
    /** changes the string to render
     * @warning Changing the string is as expensive as creating a new CCLabelTTF. To obtain better performance use CCLabelAtlas
     */
    virtual void setString(const char *label);
    virtual const char* getString(void);
    
    /// for interface compatibility
    void setText(const char *label);
    const char* getText(void);
    
    virtual void update(float delta);
    
    CCTextAlignment getHorizontalAlignment();
    void setHorizontalAlignment(CCTextAlignment alignment);
    
    CCVerticalTextAlignment getVerticalAlignment();
    void setVerticalAlignment(CCVerticalTextAlignment verticalAlignment);
    
    CCSize getDimensions();
    void setDimensions(const CCSize &dim);
    
    float getFontSize();
    void setFontSize(float fontSize);
    
    const char* getFontName();
    void setFontName(const char *fontName);
    
    virtual void setColor(const ccColor3B& color3);
    
    virtual void setGradientColor(const ccColor4B& start, const ccColor4B& end, const CCPoint& v);
    
    /**
     * If a rich lable contains link tag, a menu will be created and every link tag is
     * converted to a CCMenuItemColor in menu. Buy by default, the menu item is not connected
     * to action, so you must call this method to assign an action to every link
     *
     * @param index index of link, start from zero, by literal sequence
     * @param func the function will be invoked when a link is clicked
     */
    void setLinkTarget(int index, CCCallFunc* func);
    
    /**
     * It set all link action to same callfunc object. But if you called setLinkTarget to set a special function
     * for one item, the function set by setLinkTarget will take precedence
     *
     * @param func the function will be invoked when any link is clicked
     */
    void setLinkTargetForAll(CCCallFunc* func);
    
    /// set touch event priority of link menu
    void setLinkPriority(int p);
    
    /// get image bound in self space
    CCRect getImageBound(int index);
    
    /// get image bound in parent space
    CCRect getImageBoundInParentSpace(int index);
    
    /// get image bound in world space
    CCRect getImageBoundInWorldSpace(int index);
    
    /// a default scale factor which applies to all images in this label, by default it is 1
    void setGlobalImageScaleFactor(float scale, bool mustUpdateTexture = true);
    float getGlobalImageScaleFactor() { return m_globalImageScaleFactor; }
    
    /// get line spacing
    float getLineSpacing() { return m_lineSpacing; }
    
    /// set line spacing
    void setLineSpacing(float s, bool mustUpdateTexture = true);
    
    /**
     * show label characters one by one, just like talking or dialog mode.
     *
     * @param interval the interval of char display speed
     * @param repeat repeat times, by default it is zero, means no repeat. kCCRepeatForever means repeat forever
     * @param delay delay time before this animation, by default it is zero
     * @param loopFunc function to be called when all characters are displayed, invoked for every loop. by default it is NULL
     */
    void startLoopDisplay(float interval, unsigned int repeat = 0, int delay = 0, CCCallFunc* loopFunc = NULL);
    
    /// stop displaying label char by char, reset to normal state
    void stopLoopDisplay();
    
    /// set the char visible range, from first to specified index, exclusive
    void setDisplayTo(int to);
    
private:
    bool updateTexture();
    
    // update method of startLoopDisplay
    void displayNextChar(float delta);
    
protected:
    
    /** set the text definition for this label */
    void                _updateWithTextDefinition(ccFontDefinition & textDefinition, bool mustUpdateTexture = true);
    ccFontDefinition    _prepareTextDefinition(bool adjustForResolution = false);
    
    /** Dimensions of the label in Points */
    CCSize m_tDimensions;
    /** The alignment of the label */
    CCTextAlignment         m_hAlignment;
    /** The vertical alignment of the label */
    CCVerticalTextAlignment m_vAlignment;
    /** Font name used in the label */
    std::string * m_pFontName;
    /** Font size of the label */
    float m_fFontSize;
    /** label's string */
    std::string m_string;
    
    // displayed letter to index
    int m_toCharIndex;
    
    // line spacing
    int m_lineSpacing;
    
    // repeat flag
    unsigned int m_repeat;
    
    /** font shadow */
    bool    m_shadowEnabled;
    CCSize  m_shadowOffset;
    float   m_shadowOpacity;
    float   m_shadowBlur;
    unsigned int     m_shadowColor;
    
    // default image scale factor
    float m_globalImageScaleFactor;
    
    /** font stroke */
    bool        m_strokeEnabled;
    ccColor3B   m_strokeColor;
    float       m_strokeSize;
    
    /** font tint */
    ccColor3B   m_textFillColor;
    
    // text is changing
    bool m_textChanging;
    
    // callfunc of startLoopDisplay
    CCCallFunc* m_loopFunc;
    
    // elapsed time, used for color transition effect
    float m_elapsed;
    
    // update is scheduled
    bool m_updateScheduled;
};

NS_CC_END

#endif /* defined(__CCLabelTTF__) */
