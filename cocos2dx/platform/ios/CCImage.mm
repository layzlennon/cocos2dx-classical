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
#import "CCImage.h"
#import "CCFileUtils.h"
#import "CCCommon.h"
#include "support/utils/CCUtils.h"
#import <string>
#include "sprite_nodes/CCSpriteFrame.h"
#import <Foundation/Foundation.h>
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
    #import <UIKit/UIKit.h>
#else
    #define UIImage NSImage
    #define UIFont NSFont
    #import <AppKit/AppKit.h>
#endif
#import <CoreText/CoreText.h>
#include <math.h>
#include "CCDirector.h"
#include "sprite_nodes/CCSpriteFrame.h"
#include "sprite_nodes/CCSpriteFrameCache.h"
#include "textures/CCTextureCache.h"

NS_CC_BEGIN

// tag char
#define TAG_START '['
#define TAG_END ']'

// span type
typedef enum {
    UNKNOWN,
    COLOR,
    FONT,
    SIZE,
    BOLD,
    ITALIC,
    UNDERLINE,
    IMAGE,
    LINK
} SpanType;

/// span
typedef struct Span {
    // span type
    SpanType type;
    
    // close tag?
    bool close;
    
    // pos in plain text
    // for close tag, it is exclusive, i.e., it is next char
    int pos;
    
    // only used for color
    int color;
    int toColor;
    float duration;
    bool transient;
    
    // only used for size
    int fontSize;
    
    // only used for font
    char* fontName;
    
    // only used for image
    // offsetY follow opengl rule
    char* imageName;
    char* plist;
    char* atlas;
    float scaleX;
    float scaleY;
    float width;
    float height;
    float offsetY;
    CCSpriteFrame* frame; // only used when image is in a atlas
    
    // for link tag
    int normalBgColor;
    int selectedBgColor;
} Span;
typedef vector<Span> SpanList;

typedef struct {
    unsigned int height;
    unsigned int width;
    int          realLength;
    int          bitsPerComponent;
    bool         hasAlpha;
    bool         isPremultipliedAlpha;
    bool         hasShadow;
    CGSize       shadowOffset;
    float        shadowBlur;
    unsigned int shadowColor;
    bool         hasStroke;
    float        strokeColorR;
    float        strokeColorG;
    float        strokeColorB;
    float        strokeSize;
    float        tintColorR;
    float        tintColorG;
    float        tintColorB;
    float lineSpacing;
    float globalImageScaleFactor;
    int toCharIndex;
    float elapsedTime;
    unsigned char*  data;
    
    // true indicating only doing measurement
    bool sizeOnly;
    
    // true indicating label has color transition effect, so we need update label continuously
    bool needTime;
} tImageInfo;

////////////////////////////////////////
// run delegate callback

// key is NSString, value is UIImage
static NSMutableDictionary* s_imageMap = [[NSMutableDictionary alloc] init];

static CGFloat getAscent(void* refCon) {
    float ascent = 0;
    Span& span = *(Span*)refCon;
    if(span.frame) {
        const CCSize& size = span.frame->getOriginalSizeInPixels();
        ascent = span.height != 0 ? span.height : (size.height * span.scaleY);
    } else {
        NSString* name = [NSString stringWithCString:span.imageName
                                            encoding:NSUTF8StringEncoding];
        UIImage* image = [s_imageMap objectForKey:name];
        ascent = span.height != 0 ? span.height : (image ? (image.size.height * span.scaleY) : 0);
    }
    
    ascent += span.offsetY;
    return ascent;
}

static CGFloat getDescent(void* refCon) {
    Span& span = *(Span*)refCon;
    return -span.offsetY;
}

static CGFloat getWidth(void* refCon) {
    Span& span = *(Span*)refCon;
    if(span.frame) {
        const CCSize& size = span.frame->getOriginalSizeInPixels();
        return span.width != 0 ? span.width : (size.width * span.scaleX);
    } else {
        NSString* name = [NSString stringWithCString:span.imageName
                                            encoding:NSUTF8StringEncoding];
        UIImage* image = [s_imageMap objectForKey:name];
        return span.width != 0 ? span.width : (image ? (image.size.width * span.scaleX) : 0);
    }
}

static CTRunDelegateCallbacks s_runDelegateCallbacks = {
    kCTRunDelegateVersion1,
    NULL,
    getAscent,
    getDescent,
    getWidth
};

///////////////////////////////////////

// alignment
#define ALIGN_TOP    1
#define ALIGN_CENTER 3
#define ALIGN_BOTTOM 2

// tag parsing state
typedef enum {
    READY,
    START_TAG,
    CLOSE_TAG,
    EQUAL,
    SUCCESS,
    FAIL
} TagParseState;

static int parseColor(NSString* s) {
    int color = 0;
    int len = (int)[s length];
    for(int i = 0; i < len; i++) {
        color <<= 4;
        unichar c = [s characterAtIndex:i];
        if(c >= '0' && c <= '9') {
            color |= c - '0';
        } else if(c >= 'a' && c <= 'f') {
            color |= c - 'a' + 10;
        } else if(c >= 'A' && c <= 'F') {
            color |= c - 'A' + 10;
        }
    }
    
    return color;
}

static int parseColor(const unichar* p, NSUInteger len) {
    int color = 0;
    for(int i = 0; i < len; i++) {
        color <<= 4;
        char c = p[i];
        if(c >= '0' && c <= '9') {
            color |= c - '0';
        } else if(c >= 'a' && c <= 'f') {
            color |= c - 'a' + 10;
        } else if(c >= 'A' && c <= 'F') {
            color |= c - 'A' + 10;
        }
    }
    
    return color;
}

static SpanType checkTagName(unichar* p, NSUInteger start, NSUInteger end) {
    NSUInteger len = end - start;
    switch(len) {
        case 1:
            if(p[start] == 'b') {
                return BOLD;
            } else if(p[start] == 'i') {
                return ITALIC;
            } else if(p[start] == 'u') {
                return UNDERLINE;
            }
            break;
        case 4:
            if(p[start] == 'f' &&
               p[start + 1] == 'o' &&
               p[start + 2] == 'n' &&
               p[start + 3] == 't') {
                return FONT;
            } else if(p[start] == 's' &&
                      p[start + 1] == 'i' &&
                      p[start + 2] == 'z' &&
                      p[start + 3] == 'e') {
                return SIZE;
            } else if(p[start] == 'l' &&
                      p[start + 1] == 'i' &&
                      p[start + 2] == 'n' &&
                      p[start + 3] == 'k') {
                return LINK;
            }
            break;
        case 5:
            if(p[start] == 'c' &&
               p[start + 1] == 'o' &&
               p[start + 2] == 'l' &&
               p[start + 3] == 'o' &&
               p[start + 4] == 'r') {
                return COLOR;
            } else if(p[start] == 'i' &&
                      p[start + 1] == 'm' &&
                      p[start + 2] == 'a' &&
                      p[start + 3] == 'g' &&
                      p[start + 4] == 'e') {
                return IMAGE;
            }
            break;
            
    }
    
    return UNKNOWN;
}

// if parse failed, endTagPos will be len, otherwise it is end tag position
static SpanType checkTag(unichar* p, NSUInteger start, NSUInteger len, NSUInteger* endTagPos, NSUInteger* dataStart, NSUInteger* dataEnd, bool* close) {
    SpanType type = UNKNOWN;
    TagParseState state = READY;
    NSUInteger tagNameStart, tagNameEnd;
    *close = false;
    *endTagPos = len;
    *dataStart = -1;
    NSUInteger i = start;
    while(i < len) {
        switch (state) {
            case READY:
                if(p[i] == TAG_START) {
                    state = START_TAG;
                    tagNameStart = ++i;
                } else {
                    state = FAIL;
                }
                break;
            case START_TAG:
                if((i == start + 1) && p[i] == '/') {
                    state = CLOSE_TAG;
                    *close = true;
                    tagNameStart = ++i;
                } else if(p[i] == '=') {
                    state = EQUAL;
                    tagNameEnd = i++;
                    type = checkTagName(p, tagNameStart, tagNameEnd);
                    *dataStart = i;
                } else if(p[i] == TAG_END) {
                    state = SUCCESS;
                    *endTagPos = i;
                    *dataEnd = i;
                    tagNameEnd = i;
                    if(type == UNKNOWN) {
                        type = checkTagName(p, tagNameStart, tagNameEnd);
                    }
                } else if(p[i] == ' ') {
                    state = EQUAL;
                    tagNameEnd = i++;
                    type = checkTagName(p, tagNameStart, tagNameEnd);
                    *dataStart = i;
                } else {
                    i++;
                }
                break;
            case CLOSE_TAG:
                if(p[i] == TAG_END) {
                    state = SUCCESS;
                    *endTagPos = i;
                    tagNameEnd = i;
                    type = checkTagName(p, tagNameStart, tagNameEnd);
                } else {
                    i++;
                }
                break;
            case EQUAL:
                if(p[i] == TAG_END) {
                    state = SUCCESS;
                    *endTagPos = i;
                    *dataEnd = i;
                } else {
                    i++;
                }
                break;
            default:
                break;
        }
        
        if(state == FAIL || state == SUCCESS)
            break;
    }
    
    if(state != SUCCESS)
        type = UNKNOWN;
    
    return type;
}

static unichar* buildSpan(const char* pText, SpanList& spans, int* outLen, tImageInfo* pInfo) {
    // get unichar of string
    NSString* ns = [NSString stringWithUTF8String:pText];
    NSUInteger uniLen = [ns length];
    unichar* uniText = (unichar*)malloc(uniLen * sizeof(unichar));
    [ns getCharacters:uniText range:NSMakeRange(0, uniLen)];
    
    bool inImageTag = false;
    int plainLen = 0;
    unichar* plain = (unichar*)malloc(sizeof(unichar) * uniLen);
    for(NSUInteger i = 0; i < uniLen; i++) {
        unichar c = uniText[i];
        switch(c) {
            case '\\':
                if(i < uniLen - 1) {
                    unichar cc = uniText[i + 1];
                    if(cc == TAG_START || cc == TAG_END) {
                        plain[plainLen++] = cc;
                        i++;
                    }
                } else {
                    plain[plainLen++] = c;
                }
                break;
            case TAG_START:
            {
                // parse the tag
                Span span;
                NSUInteger endTagPos, dataStart, dataEnd;
                SpanType type = checkTag(uniText, i, uniLen, &endTagPos, &dataStart, &dataEnd, &span.close);
                
                // fill span info
                do {
                    // if type is unknown, discard
                    CC_BREAK_IF(type == UNKNOWN);
                    
                    // you can't embed other tag in image tag, discard
                    CC_BREAK_IF(inImageTag && (!span.close || type != IMAGE));
                    
                    // parse span data
                    span.type = type;
                    span.pos = plainLen;
                    if(span.close) {
                        // special check for image end, use a placeholder for image tag
                        if(inImageTag) {
                            plain[plainLen++] = 0xfffc;
                            span.pos++;
                            inImageTag = false;
                        }
                    } else {
                        switch(span.type) {
                            case COLOR:
                            {
                                NSString* content = [NSString stringWithCharacters:uniText + dataStart
                                                                            length:dataEnd - dataStart];
                                NSArray* parts = [content componentsSeparatedByString:@" "];
                                
                                // color
                                span.color = parseColor([parts objectAtIndex:0]);
                                
                                // set default
                                span.toColor = 0;
                                span.duration = 0;
                                span.transient = false;
                                
                                // parse parameters
                                if([parts count] > 1) {
                                    NSUInteger count = [parts count];
                                    for(NSUInteger i = 1; i < count; i++) {
                                        NSString* part = [parts objectAtIndex:i];
                                        NSArray* pair = [part componentsSeparatedByString:@"="];
                                        if([pair count] == 2) {
                                            NSString* key = [pair objectAtIndex:0];
                                            NSString* value = [pair objectAtIndex:1];
                                            if([@"to" isEqualToString:key]) {
                                                span.toColor = parseColor(value);
                                            } else if([@"duration" isEqualToString:key]) {
                                                span.duration = [value floatValue];
                                                
                                                // if duration > 0, this color transition is effective
                                                if(span.duration > 0)
                                                    pInfo->needTime = true;
                                            } else if([@"transient" isEqualToString:key]) {
                                                span.transient = [@"true" isEqualToString:value];
                                            }
                                        }
                                    }
                                }
                                
                                break;
                            }
                            case FONT:
                            {
                                NSString* font = [NSString stringWithCharacters:uniText + dataStart
                                                                         length:dataEnd - dataStart];
                                const char* cFont = [font cStringUsingEncoding:NSUTF8StringEncoding];
                                size_t len = strlen(cFont);
                                span.fontName = (char*)calloc(sizeof(char), len + 1);
                                strcpy(span.fontName, cFont);
                                break;
                            }
                            case SIZE:
                            {
                                NSString* size = [NSString stringWithCharacters:uniText + dataStart
                                                                         length:dataEnd - dataStart];
                                span.fontSize = [size intValue];
                                break;
                            }
                            case IMAGE:
                            {
                                NSString* name = [NSString stringWithCharacters:uniText + dataStart
                                                                         length:dataEnd - dataStart];
                                NSArray* parts = [name componentsSeparatedByString:@" "];
                                name = [parts objectAtIndex:0];
                                const char* cName = [name cStringUsingEncoding:NSUTF8StringEncoding];
                                size_t len = strlen(cName);
                                span.imageName = (char*)calloc(sizeof(char), len + 1);
                                strcpy(span.imageName, cName);
                                
                                // set default
                                span.scaleX = span.scaleY = 1;
                                span.width = span.height = 0;
                                span.offsetY = 0;
                                span.plist = NULL;
                                span.atlas = NULL;
                                span.frame = NULL;
                                
                                // if has other parts, check
                                if([parts count] > 1) {
                                    NSUInteger count = [parts count];
                                    for(NSUInteger i = 1; i < count; i++) {
                                        NSString* part = [parts objectAtIndex:i];
                                        NSArray* pair = [part componentsSeparatedByString:@"="];
                                        if([pair count] == 2) {
                                            NSString* key = [pair objectAtIndex:0];
                                            NSString* value = [pair objectAtIndex:1];
                                            if([@"scale" isEqualToString:key]) {
                                                span.scaleX = span.scaleY = [value floatValue];
                                            } else if([@"scalex" isEqualToString:key]) {
                                                span.scaleX = [value floatValue];
                                            } else if([@"scaley" isEqualToString:key]) {
                                                span.scaleY = [value floatValue];
                                            } else if([@"w" isEqualToString:key]) {
                                                span.width = [value floatValue];
                                            } else if([@"h" isEqualToString:key]) {
                                                span.height = [value floatValue];
                                            } else if([@"offsety" isEqualToString:key]) {
                                                span.offsetY = [value floatValue];
                                            } else if([@"plist" isEqualToString:key]) {
                                                span.plist = (char*)CCUtils::copy([value cStringUsingEncoding:NSUTF8StringEncoding]);
                                            } else if([@"atlas" isEqualToString:key]) {
                                                span.atlas = (char*)CCUtils::copy([value cStringUsingEncoding:NSUTF8StringEncoding]);
                                            }
                                        }
                                    }
                                }
                                
                                // apply global factor
                                span.scaleX *= pInfo->globalImageScaleFactor;
                                span.scaleY *= pInfo->globalImageScaleFactor;
                                
                                // flag
                                inImageTag = true;
                                
                                break;
                            }
                            case LINK:
                            {
                                NSString* v = [NSString stringWithCharacters:uniText + dataStart
                                                                      length:dataEnd - dataStart];
                                NSArray* parts = [v componentsSeparatedByString:@" "];
                                for(NSString* part in parts) {
                                    NSArray* pair = [part componentsSeparatedByString:@"="];
                                    if([pair count] == 2) {
                                        unichar tmp[32];
                                        NSString* key = [pair objectAtIndex:0];
                                        NSString* value = [pair objectAtIndex:1];
                                        if([@"bg" isEqualToString:key]) {
                                            [value getCharacters:tmp];
                                            span.normalBgColor = parseColor(tmp, [value length]);
                                        } else if([@"bg_click" isEqualToString:key]) {
                                            [value getCharacters:tmp];
                                            span.selectedBgColor = parseColor(tmp, [value length]);
                                        }
                                    }
                                }
                                break;
                            }
                            default:
                                break;
                        }
                    }
                    
                    // add span
                    spans.push_back(span);
                    
                    // set i pos
                    i = endTagPos;
                } while(0);
                
                break;
            }
            case TAG_END:
                // just discard it
                break;
            default:
                // ignore text between image tag
                if(!inImageTag) {
                    plain[plainLen++] = c;
                }
                break;
        }
    }
    
    // return length
    if(outLen)
        *outLen = plainLen;
    
#ifdef COCOS2D_DEBUG
    //	// debug output span info
    //	int i = 0;
    //	for(SpanList::iterator iter = spans.begin(); iter != spans.end(); iter++) {
    //		Span& span = *iter;
    //		CCLOG("span %d: %d, %d, %d", i++, span.type, span.close, span.pos);
    //	}
#endif
    
    // release
    free(uniText);
    
    // return plain str
    return plain;
}

static UIImage* extractFrameFromAtlas(const char* atlasFile, CCSpriteFrame* frame) {
    string path = CCFileUtils::sharedFileUtils()->fullPathForFilename(atlasFile);
    UIImage* frameImage = nil;
    
    // try to load its data
    NSData* nsData = [NSData dataWithContentsOfFile:[NSString stringWithUTF8String:path.c_str()]];
    if(!nsData)
        return frameImage;
    
    // decrypt
    if(gResDecrypt) {
        int decLen;
        const char* dec = (*gResDecrypt)((const char*)[nsData bytes], (int)[nsData length], &decLen);
        if([nsData bytes] != dec) {
            nsData = [NSData dataWithBytes:dec length:decLen];
            free((void*)dec);
        }
    }
    
    // get final image from data
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
    UIImage* image = [UIImage imageWithData:nsData];
#else
    NSImage* image = [[NSImage alloc] initWithData:nsData];
#if !__has_feature(objc_arc)
    // XXX: if uncomment it, cocos2d-x has a weird crash when release pool, so I have
    // to comment it and the image will be leaked
    //	[image autorelease];
#endif
#endif
    
    // draw part of it to get normal frame image
    if(image) {
        // get frame info
        const CCSize& sourceSize = frame->getOriginalSizeInPixels();
        bool rotated = frame->isRotated();
        const CCRect& frameRect = frame->getRectInPixels();
        const CCPoint& offset = frame->getOffsetInPixels();
        
        unsigned char* data = new unsigned char[(int)(sourceSize.width * sourceSize.height * 4)];
        memset(data, 0, (int)(sourceSize.width * sourceSize.height * 4));
        
        // create context
        CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
        CGContextRef ctx = CGBitmapContextCreate(data,
                                                 sourceSize.width,
                                                 sourceSize.height,
                                                 8,
                                                 sourceSize.width * 4,
                                                 colorSpace,
                                                 kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
        
        // start context and setup transform
        CGContextConcatCTM(ctx, CGAffineTransformMakeTranslation(sourceSize.width / 2, sourceSize.height / 2));
        CGContextConcatCTM(ctx, CGAffineTransformMakeTranslation(offset.x, -offset.y));
        if(rotated) {
            CGContextConcatCTM(ctx, CGAffineTransformMakeRotation(CC_DEGREES_TO_RADIANS(90)));
        }
        
        // get partial image
        CGRect rect = CGRectMake(frameRect.origin.x, frameRect.origin.y, frameRect.size.width, frameRect.size.height);
        if(rotated)
            swap(rect.size.width, rect.size.height);
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
        CGImageRef frameCGImage = CGImageCreateWithImageInRect(image.CGImage, rect);
#else
        CGImageRef imageCGImage = [image CGImageForProposedRect:nil
                                                        context:nil
                                                          hints:nil];
        CGImageRef frameCGImage = CGImageCreateWithImageInRect(imageCGImage, rect);
        CGImageRelease(imageCGImage);
#endif
        
        // draw frame
        CGContextDrawImage(ctx,
                           CGRectMake(-rect.size.width / 2, -rect.size.height / 2, rect.size.width, rect.size.height),
                           frameCGImage);
        
        // get final image
        CGImageRef cgImage = CGBitmapContextCreateImage(ctx);
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
        frameImage = [UIImage imageWithCGImage:cgImage];
#else
        frameImage = [[NSImage alloc] initWithCGImage:cgImage size:NSZeroSize];
#if !__has_feature(objc_arc)
        // XXX: if uncomment it, cocos2d-x has a weird crash when release pool, so I have
        // to comment it and the image will be leaked
        //        [frameImage autorelease];
#endif
#endif
        
        // end
        CGImageRelease(frameCGImage);
        CGImageRelease(cgImage);
        CGContextRelease(ctx);
        CGColorSpaceRelease(colorSpace);
        free(data);
    }
    
    return frameImage;
}

static void coverUndisplayedCharacters(CGContextRef context, CTFrameRef frame, int endIndex, float dx, float dy) {
    // get line count and their origin
    CFArrayRef linesArray = CTFrameGetLines(frame);
    CFIndex lineCount = CFArrayGetCount(linesArray);
    CGPoint* origin = (CGPoint*)malloc(sizeof(CGPoint) * lineCount);
    CTFrameGetLineOrigins(frame, CFRangeMake(0, 0), origin);
    
    // find line of endIndex and remove characters after that
    for (CFIndex i = 0; i < lineCount; i++) {
        // get line character range
        CTLineRef line = (CTLineRef)CFArrayGetValueAtIndex(linesArray, i);
        CFRange range = CTLineGetStringRange(line);
        CFIndex end = range.location + range.length;
        if(endIndex >= range.location && endIndex < end) {
            CGFloat startX = CTLineGetOffsetForStringIndex(line, endIndex, NULL) + origin[i].x;
            CGFloat endX = CTLineGetOffsetForStringIndex(line, end, NULL) + origin[i].x;
            CGFloat ascent, descent, leading;
            CTLineGetTypographicBounds(line, &ascent, &descent, &leading);
            CGRect coverBound = CGRectMake(startX + dx, origin[i].y - descent, endX - startX, ascent + descent + dy);
            CGContextClearRect(context, coverBound);
        } else if(endIndex < range.location) {
            CGFloat ascent, descent, leading;
            CGFloat width = CTLineGetTypographicBounds(line, &ascent, &descent, &leading);
            CGRect coverBound = CGRectMake(origin[i].x + dx, origin[i].y - descent, width, ascent + descent + dy);
            CGContextClearRect(context, coverBound);
        }
    }
    
    // free origin
    free(origin);
}

static void renderEmbededImages(CGContextRef context, CTFrameRef frame, unichar* plain, SpanList& spans, vector<CCRect>& imageRects) {
    if([s_imageMap count] > 0) {
        // get line count and their origin
        CFArrayRef linesArray = CTFrameGetLines(frame);
        CFIndex lineCount = CFArrayGetCount(linesArray);
        CGPoint* origin = (CGPoint*)malloc(sizeof(CGPoint) * lineCount);
        CTFrameGetLineOrigins(frame, CFRangeMake(0, 0), origin);
        
        // iterate every line
        for (CFIndex i = 0; i < lineCount; i++) {
            // get line character range
            CTLineRef line = (CTLineRef)CFArrayGetValueAtIndex(linesArray, i);
            CFRange range = CTLineGetStringRange(line);
            
            // find image placeholder
            CFIndex end = range.location + range.length;
            for(CFIndex j = range.location; j < end; j++) {
                // if placeholder matched, render this image
                if(plain[j] == 0xfffc) {
                    // get offset
                    CGFloat offsetX = CTLineGetOffsetForStringIndex(line, j, NULL);
                    
                    // get span, if one image span matched index, draw the image
                    for(SpanList::iterator iter = spans.begin(); iter != spans.end(); iter++) {
                        Span& span = *iter;
                        if(span.type == IMAGE && !span.close && span.pos == j) {
                            UIImage* image = nil;
                            if(span.frame) {
                                image = extractFrameFromAtlas(span.atlas, span.frame);
                            } else {
                                NSString* imageName = [NSString stringWithCString:span.imageName
                                                                         encoding:NSUTF8StringEncoding];
                                image = [s_imageMap objectForKey:imageName];
                            }
                            
                            // if image is got, render, if not, just save rect
                            if(image) {
                                CGRect rect = CGRectMake(offsetX + origin[i].x,
                                                         origin[i].y + span.offsetY,
                                                         span.width != 0 ? span.width : (image.size.width * span.scaleX),
                                                         span.height != 0 ? span.height : (image.size.height * span.scaleY));
                                
                                
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
                                CGContextDrawImage(context, rect, image.CGImage);
#else
                                CGImageRef cgImage = [image CGImageForProposedRect:NULL
                                                                           context:[NSGraphicsContext currentContext]
                                                                             hints:nil];
                                CGContextDrawImage(context, rect, cgImage);
                                CGImageRelease(cgImage);
#endif
                                
                                // save rect
                                imageRects.push_back(CCRectMake(rect.origin.x,
                                                                rect.origin.y,
                                                                rect.size.width,
                                                                rect.size.height));
                            } else {
                                // save rect
                                imageRects.push_back(CCRectMake(offsetX + origin[i].x,
                                                                origin[i].y + span.offsetY,
                                                                span.width,
                                                                span.height));
                            }
                            
                            break;
                        }
                    }
                }
            }
        }
        
        // free origin
        free(origin);
    }
}

static void extractLinkMeta(CTFrameRef frame, SpanList& spans, LinkMetaList& linkMetas) {
    // get line count and their origin
    CFArrayRef linesArray = CTFrameGetLines(frame);
    CFIndex lineCount = CFArrayGetCount(linesArray);
    CGPoint* origin = (CGPoint*)malloc(sizeof(CGPoint) * lineCount);
    CTFrameGetLineOrigins(frame, CFRangeMake(0, 0), origin);
    
    // divide scale factor for origin
    for (CFIndex i = 0; i < lineCount; i++) {
        origin[i].x /= CC_CONTENT_SCALE_FACTOR();
        origin[i].y /= CC_CONTENT_SCALE_FACTOR();
    }
    
    // get range of every line
    CFRange* range = (CFRange*)malloc(sizeof(CFRange) * lineCount);
    for (CFIndex i = 0; i < lineCount; i++) {
        CTLineRef line = (CTLineRef)CFArrayGetValueAtIndex(linesArray, i);
        range[i] = CTLineGetStringRange(line);
    }
    
    // find every link span
    LinkMeta meta;
    int tag = 0;
    CFIndex linkStart, linkEnd;
    CFIndex startLine = 0, endLine = -1;
    for(SpanList::iterator iter = spans.begin(); iter != spans.end(); iter++) {
        Span& span = *iter;
        if(span.type == LINK) {
            if(!span.close) {
                // get start pos
                linkStart = span.pos;
                
                // save color info
                meta.normalBgColor = span.normalBgColor;
                meta.selectedBgColor = span.selectedBgColor;
            } else {
                // remember end pos
                linkEnd = span.pos;
                
                // find out the line for start and end pos
                for (CFIndex i = 0; i < lineCount; i++) {
                    if(linkStart >= range[i].location &&
                       linkStart < range[i].location + range[i].length) {
                        startLine = i;
                    }
                    
                    if(linkEnd >= range[i].location &&
                       linkEnd < range[i].location + range[i].length) {
                        endLine = i;
                        break;
                    }
                }
                
                // in case endLine is not set
                if(endLine == -1) {
                    endLine = startLine;
                }
                
                // get rect area
                while(startLine <= endLine) {
                    CGFloat ascent;
                    CGFloat descent;
                    CFIndex charEnd = linkEnd;
                    if(startLine < endLine)
                        charEnd = range[startLine].location + range[startLine].length;
                    CTLineRef line = (CTLineRef)CFArrayGetValueAtIndex(linesArray, startLine);
                    CGFloat startOffsetX = CTLineGetOffsetForStringIndex(line, linkStart, NULL) / CC_CONTENT_SCALE_FACTOR();
                    CGFloat endOffsetX = CTLineGetOffsetForStringIndex(line, charEnd, NULL) / CC_CONTENT_SCALE_FACTOR();
                    CTLineGetTypographicBounds(line, &ascent, &descent, NULL);
                    ascent /= CC_CONTENT_SCALE_FACTOR();
                    descent /= CC_CONTENT_SCALE_FACTOR();
                    meta.x = origin[startLine].x + startOffsetX;
                    meta.y = origin[startLine].y - descent;
                    meta.width = origin[startLine].x + endOffsetX - meta.x;
                    meta.height = descent + ascent;
                    meta.tag = tag;
                    
                    // push meta
                    linkMetas.push_back(meta);
                    
                    // move line
                    startLine++;
                    
                    // move start
                    linkStart = charEnd;
                }
                
                // increase tag
                tag++;
            }
        }
    }
    
    // free
    free(origin);
    free(range);
}

static void setColorSpan(Span& top, CFMutableAttributedStringRef plainCFAStr, CGColorSpaceRef colorSpace, int start, int end, float elapsedTime) {
    CGFloat comp[] = {
        ((top.color >> 16) & 0xff) / 255.0f,
        ((top.color >> 8) & 0xff) / 255.0f,
        (top.color & 0xff) / 255.0f,
        ((top.color >> 24) & 0xff) / 255.0f
    };
    
    // if has color transition, need calculate color based on dest color and time
    if(top.duration > 0) {
        // dest color
        CGFloat toComp[] = {
            ((top.toColor >> 16) & 0xff) / 255.0f,
            ((top.toColor >> 8) & 0xff) / 255.0f,
            (top.toColor & 0xff) / 255.0f,
            ((top.toColor >> 24) & 0xff) / 255.0f
        };
        
        // if transient, just change color when time's up
        float remaining = fmodf(elapsedTime, top.duration * 2);
        if(top.transient) {
            if(remaining >= top.duration) {
                memcpy(comp, toComp, sizeof(comp));
            }
        } else {
            float percent;
            if(remaining >= top.duration) {
                percent = 1 - (remaining - top.duration) / top.duration;
            } else {
                percent = remaining / top.duration;
            }
            comp[0] = comp[0] * (1 - percent) + toComp[0] * percent;
            comp[1] = comp[1] * (1 - percent) + toComp[1] * percent;
            comp[2] = comp[2] * (1 - percent) + toComp[2] * percent;
            comp[3] = comp[3] * (1 - percent) + toComp[3] * percent;
        }
    }
    
    // set color
    CGColorRef fc = CGColorCreate(colorSpace, comp);
    CFAttributedStringSetAttribute(plainCFAStr,
                                   CFRangeMake(start, end - start),
                                   kCTForegroundColorAttributeName,
                                   fc);
    CGColorRelease(fc);
}

static bool _initWithString(const char * pText, CCImage::ETextAlign eAlign, const char * pFontName, int nSize, tImageInfo* pInfo, CCPoint* outShadowStrokePadding, LinkMetaList* linkMetas, vector<CCRect>* imageRects) {
    bool bRet = false;
    do {
        CC_BREAK_IF(!pText || !pInfo);
        
        // On iOS custom fonts must be listed beforehand in the App info.plist (in order to be usable) and referenced only the by the font family name itself when
        // calling [UIFont fontWithName]. Therefore even if the developer adds 'SomeFont.ttf' or 'fonts/SomeFont.ttf' to the App .plist, the font must
        // be referenced as 'SomeFont' when calling [UIFont fontWithName]. Hence we strip out the folder path components and the extension here in order to get just
        // the font family name itself. This stripping step is required especially for references to user fonts stored in CCB files; CCB files appear to store
        // the '.ttf' extensions when referring to custom fonts.
        NSString* fntName = [NSString stringWithUTF8String:pFontName];
        fntName = [[fntName lastPathComponent] stringByDeletingPathExtension];
        
        // create the font
        if(nSize <= 0)
            nSize = (int)[UIFont systemFontSize];
        UIFont* uiDefaultFont = [UIFont fontWithName:fntName size:nSize];
        CC_BREAK_IF(!uiDefaultFont);
        CTFontRef defaultFont = CTFontCreateWithName((CFStringRef)uiDefaultFont.fontName, nSize, NULL);
        
        // get plain text and extract span list
        SpanList spans;
        int uniLen;
        unichar* plain = buildSpan(pText, spans, &uniLen, pInfo);
        
        // create attributed string
        CFStringRef plainCFStr = CFStringCreateWithCharacters(kCFAllocatorDefault,
                                                              (const UniChar*)plain,
                                                              uniLen);
        CFMutableAttributedStringRef plainCFAStr = CFAttributedStringCreateMutable(kCFAllocatorDefault, 0);
        CFAttributedStringReplaceString(plainCFAStr, CFRangeMake(0, 0), plainCFStr);
        CFIndex plainLen = CFAttributedStringGetLength(plainCFAStr);
        
        // font and color stack
        Span defaultColor;
        defaultColor.type = COLOR;
        defaultColor.color = 0xff000000
        | ((int)(pInfo->tintColorR * 255) << 16)
        | ((int)(pInfo->tintColorG * 255) << 8)
        | (int)(pInfo->tintColorB * 255);
        defaultColor.duration = 0;
        vector<CTFontRef> fontStack;
        vector<Span> colorStack;
        fontStack.push_back(defaultFont);
        colorStack.push_back(defaultColor);
        
        // iterate all spans, install attributes
        int colorStart = 0;
        int fontStart = 0;
        int underlineStart = -1;
        Span* openSpan = NULL;
        CGColorSpaceRef colorSpace  = CGColorSpaceCreateDeviceRGB();
        for(SpanList::iterator iter = spans.begin(); iter != spans.end(); iter++) {
            Span& span = *iter;
            if(span.close) {
                switch(span.type) {
                    case COLOR:
                    {
                        if(span.pos > colorStart) {
                            // set color span
                            Span& top = *colorStack.rbegin();
                            setColorSpan(top, plainCFAStr, colorSpace, colorStart, span.pos, pInfo->elapsedTime);
                            
                            // start need to be reset
                            colorStart = span.pos;
                        }
                        
                        // pop color
                        colorStack.pop_back();
                        
                        break;
                    }
                    case FONT:
                    case SIZE:
                    case BOLD:
                    case ITALIC:
                    {
                        // set previous span
                        CTFontRef font = *fontStack.rbegin();
                        if(span.pos > fontStart && font) {
                            CFAttributedStringSetAttribute(plainCFAStr,
                                                           CFRangeMake(fontStart, plainLen - fontStart),
                                                           kCTFontAttributeName,
                                                           font);
                            fontStart = span.pos;
                        }
                        
                        // pop font
                        fontStack.pop_back();
                        if(font)
                            CFRelease(font);
                        
                        break;
                    }
                    case UNDERLINE:
                    {
                        if(underlineStart > -1) {
                            CFIndex style = kCTUnderlineStyleSingle;
                            CFNumberRef styleNum = CFNumberCreate(kCFAllocatorDefault, kCFNumberCFIndexType, &style);
                            CFAttributedStringSetAttribute(plainCFAStr,
                                                           CFRangeMake(underlineStart, span.pos - underlineStart),
                                                           kCTUnderlineStyleAttributeName,
                                                           styleNum);
                            CFRelease(styleNum);
                            underlineStart = -1;
                        }
                        break;
                    }
                    case IMAGE:
                    {
                        if(openSpan->pos > -1) {
                            do {
                                // register image, if it starts with '/', treat it as an external image
                                // if image name is empty, skip loading
                                // if image is in an atlas, we need load that atlas and get sprite frame, however we don't load atlas image now
                                NSString* imageName = [NSString stringWithCString:span.imageName
                                                                         encoding:NSUTF8StringEncoding];
                                if(span.plist && span.atlas) {
                                    CCSpriteFrameCache* fc = CCSpriteFrameCache::sharedSpriteFrameCache();
                                    if(!fc->spriteFrameByName(span.imageName)) {
                                        if(gResDecrypt) {
                                            // load encryptd data
                                            size_t len;
                                            char* data = (char*)CCFileUtils::sharedFileUtils()->getFileData(span.atlas, "rb", &len);
                                            
                                            // create texture
                                            int decLen;
                                            const char* dec = (*gResDecrypt)(data, (int)len, &decLen);
                                            CCImage* image = new CCImage();
                                            image->initWithImageData((void*)dec, decLen);
                                            CC_SAFE_AUTORELEASE(image);
                                            CCTexture2D* tex = CCTextureCache::sharedTextureCache()->addUIImage(image, span.atlas);
                                            
                                            // add
                                            fc->addSpriteFramesWithFile(span.plist, tex);
                                            
                                            // free
                                            if(dec != data)
                                                free((void*)dec);
                                            free(data);
                                        } else {
                                            fc->addSpriteFramesWithFile(span.plist, span.atlas);
                                        }
                                    }
                                    span.frame = fc->spriteFrameByName(span.imageName);
                                    openSpan->frame = span.frame;
                                    
                                    // just put a dummy object in image map to let it knows there is some embedded image to be rendered
                                    [s_imageMap setObject:[NSNumber numberWithInt:0] forKey:imageName];
                                } else {
                                    if([imageName length] <= 0)
                                        break;
                                    
                                    // get image full path, external path first
                                    string cppFullPath = CCUtils::getExternalOrFullPath([imageName cStringUsingEncoding:NSUTF8StringEncoding]);
                                    NSString* fullPath = [NSString stringWithCString:cppFullPath.c_str() encoding:NSUTF8StringEncoding];
                                    
                                    // check image extension
                                    NSString* pathWithoutExt = [fullPath stringByDeletingPathExtension];
                                    NSString* ext = [fullPath substringFromIndex:[pathWithoutExt length]];
                                    bool png = [ext compare:@".png" options:NSCaseInsensitiveSearch] == NSOrderedSame;
                                    bool jpg = [ext compare:@".jpg" options:NSCaseInsensitiveSearch] == NSOrderedSame ||
                                    [ext compare:@".jpeg" options:NSCaseInsensitiveSearch] == NSOrderedSame;
                                    
                                    // try to load its data
                                    NSData* nsData = [NSData dataWithContentsOfFile:fullPath];
                                    if(!nsData)
                                        break;
                                    
                                    // decrypt
                                    if(gResDecrypt) {
                                        int decLen;
                                        const char* dec = (*gResDecrypt)((const char*)[nsData bytes], (int)[nsData length], &decLen);
                                        if([nsData bytes] != dec) {
                                            nsData = [NSData dataWithBytes:dec length:decLen];
                                            free((void*)dec);
                                        }
                                    }
                                    
                                    // create CGImage from data
                                    CGDataProviderRef imgDataProvider = CGDataProviderCreateWithCFData((CFDataRef)nsData);
                                    CGImageRef cgImage = NULL;
                                    if(png) {
                                        cgImage = CGImageCreateWithPNGDataProvider(imgDataProvider,
                                                                                   NULL,
                                                                                   true,
                                                                                   kCGRenderingIntentDefault);
                                    } else if(jpg) {
                                        cgImage = CGImageCreateWithJPEGDataProvider(imgDataProvider,
                                                                                    NULL,
                                                                                    true,
                                                                                    kCGRenderingIntentDefault);
                                    }
                                    
                                    // create UIImage and register it
                                    if(cgImage) {
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
                                        UIImage* image = [UIImage imageWithCGImage:cgImage];
#else
                                        NSImage* image = [[NSImage alloc] initWithCGImage:cgImage
                                                                                     size:NSZeroSize];
#if !__has_feature(objc_arc)
                                        // XXX: if uncomment it, cocos2d-x has a weird crash when release pool, so I have
                                        // to comment it and the image will be leaked
                                        //                                        [image autorelease];
#endif
#endif
                                        CGImageRelease(cgImage);
                                        [s_imageMap setValue:image forKey:imageName];
                                    }
                                    
                                    // release data provider
                                    CGDataProviderRelease(imgDataProvider);
                                }
                            } while(0);
                            
                            // create a run delegate for image span so that the width and height can be customized
                            CTRunDelegateRef delegate = CTRunDelegateCreate(&s_runDelegateCallbacks, &span);
                            CFAttributedStringSetAttribute(plainCFAStr,
                                                           CFRangeMake(openSpan->pos, span.pos - openSpan->pos),
                                                           kCTRunDelegateAttributeName,
                                                           delegate);
                            CFRelease(delegate);
                        }
                        break;
                    }
                    default:
                        break;
                }
            } else {
                // save open span pointer
                openSpan = &span;
                
                switch(span.type) {
                    case COLOR:
                    {
                        // process previous run
                        if(span.pos > colorStart) {
                            // set color span
                            Span& top = *colorStack.rbegin();
                            setColorSpan(top, plainCFAStr, colorSpace, colorStart, span.pos, pInfo->elapsedTime);
                            
                            // start need to be reset
                            colorStart = span.pos;
                        }
                        
                        // push color
                        colorStack.push_back(span);
                        
                        break;
                    }
                    case FONT:
                    {
                        // set previous span
                        CTFontRef curFont = *fontStack.rbegin();
                        if(span.pos > fontStart) {
                            CFAttributedStringSetAttribute(plainCFAStr,
                                                           CFRangeMake(fontStart, plainLen - fontStart),
                                                           kCTFontAttributeName,
                                                           curFont);
                            fontStart = span.pos;
                        }
                        
                        // create the font
                        NSString* fontName = [NSString stringWithCString:span.fontName
                                                                encoding:NSUTF8StringEncoding];
                        fontName = [[fontName lastPathComponent] stringByDeletingPathExtension];
                        UIFont* uiFont = [UIFont fontWithName:fontName
                                                         size:CTFontGetSize(curFont)];
                        CC_BREAK_IF(!uiFont);
                        CTFontRef font = CTFontCreateWithName((CFStringRef)uiFont.fontName,
                                                              CTFontGetSize(curFont),
                                                              NULL);
                        fontStack.push_back(font);
                        
                        break;
                    }
                    case SIZE:
                    {
                        // set previous span
                        CTFontRef curFont = *fontStack.rbegin();
                        if(span.pos > fontStart) {
                            CFAttributedStringSetAttribute(plainCFAStr,
                                                           CFRangeMake(fontStart, plainLen - fontStart),
                                                           kCTFontAttributeName,
                                                           curFont);
                            fontStart = span.pos;
                        }
                        
                        // push new font
                        CTFontDescriptorRef fd = CTFontCopyFontDescriptor(curFont);
                        CTFontRef font = CTFontCreateCopyWithAttributes(curFont,
                                                                        span.fontSize * CC_CONTENT_SCALE_FACTOR(),
                                                                        NULL,
                                                                        fd);
                        fontStack.push_back(font);
                        CFRelease(fd);
                        
                        break;
                    }
                    case BOLD:
                    {
                        // set previous span
                        CTFontRef curFont = *fontStack.rbegin();
                        if(span.pos > fontStart) {
                            CFAttributedStringSetAttribute(plainCFAStr,
                                                           CFRangeMake(fontStart, plainLen - fontStart),
                                                           kCTFontAttributeName,
                                                           curFont);
                            fontStart = span.pos;
                        }
                        
                        // create new font
                        CTFontRef font = CTFontCreateCopyWithSymbolicTraits(curFont,
                                                                            CTFontGetSize(curFont),
                                                                            NULL,
                                                                            kCTFontBoldTrait,
                                                                            kCTFontBoldTrait);
                        fontStack.push_back(font);
                        
                        break;
                    }
                    case ITALIC:
                    {
                        // set previous span
                        CTFontRef curFont = *fontStack.rbegin();
                        if(span.pos > fontStart) {
                            CFAttributedStringSetAttribute(plainCFAStr,
                                                           CFRangeMake(fontStart, plainLen - fontStart),
                                                           kCTFontAttributeName,
                                                           curFont);
                            fontStart = span.pos;
                        }
                        
                        // create new font
                        CTFontRef font = CTFontCreateCopyWithSymbolicTraits(curFont,
                                                                            CTFontGetSize(curFont),
                                                                            NULL,
                                                                            kCTFontItalicTrait,
                                                                            kCTFontItalicTrait);
                        fontStack.push_back(font);
                        
                        break;
                    }
                    case UNDERLINE:
                    {
                        underlineStart = span.pos;
                        break;
                    }
                    default:
                        break;
                }
            }
        }
        
        // last segment
        if(plainLen > colorStart) {
            Span& top = *colorStack.rbegin();
            setColorSpan(top, plainCFAStr, colorSpace, colorStart, (int)plainLen, pInfo->elapsedTime);
        }
        if(plainLen > fontStart) {
            CTFontRef font = *fontStack.rbegin();
            CFAttributedStringSetAttribute(plainCFAStr,
                                           CFRangeMake(fontStart, plainLen - fontStart),
                                           kCTFontAttributeName,
                                           font);
        }
	       
        // set paragraph style, including line spacing and alignment
        CTTextAlignment alignment = kCTLeftTextAlignment;
        switch(eAlign & 0x0f) {
            case 2:
                alignment = kCTRightTextAlignment;
                break;
            case 3:
                alignment = kCTCenterTextAlignment;
                break;
            default:
                break;
        }
        CTParagraphStyleSetting paraSettings[] = {
            { kCTParagraphStyleSpecifierAlignment, sizeof(alignment), &alignment},
            { kCTParagraphStyleSpecifierLineSpacingAdjustment, sizeof(CGFloat), &CCPointZero.x },
            { kCTParagraphStyleSpecifierMaximumLineSpacing, sizeof(CGFloat), &pInfo->lineSpacing },
            { kCTParagraphStyleSpecifierMinimumLineSpacing, sizeof(CGFloat), &pInfo->lineSpacing }
        };
        CTParagraphStyleRef paraStyle = CTParagraphStyleCreate(paraSettings,
                                                               sizeof(paraSettings) / sizeof(paraSettings[0]));
        CFAttributedStringSetAttribute(plainCFAStr,
                                       CFRangeMake(0, CFAttributedStringGetLength(plainCFAStr)),
                                       kCTParagraphStyleAttributeName,
                                       paraStyle);
        
        // create frame setter
        CTFramesetterRef fs = CTFramesetterCreateWithAttributedString(plainCFAStr);
        
        // constrain size
        CFRange range;
        CGSize constrainSize = CGSizeMake(pInfo->width, pInfo->height);
        CGSize ctConstrainSize = constrainSize;
        if(ctConstrainSize.width <= 0) {
            ctConstrainSize.width = CGFLOAT_MAX;
        }
        if(ctConstrainSize.height <= 0) {
            ctConstrainSize.height = CGFLOAT_MAX;
        }
        CGSize dim = CTFramesetterSuggestFrameSizeWithConstraints(fs,
                                                                  CFRangeMake(0, 0),
                                                                  NULL,
                                                                  ctConstrainSize,
                                                                  &range);
        dim.width = ceilf(dim.width);
        dim.height = ceilf(dim.height);
        
        // text origin offset
        int startY = 0;
        int startX = 0;
        if (constrainSize.height > dim.height) {
            // vertical alignment
            unsigned int vAlignment = (eAlign >> 4) & 0x0F;
            if (vAlignment == ALIGN_TOP) {
                startY = 0;
            } else if (vAlignment == ALIGN_CENTER) {
                startY = (constrainSize.height - dim.height) / 2;
            } else {
                startY = constrainSize.height - dim.height;
            }
        }
        
        // adjust text rect
        if (constrainSize.height > 0 && constrainSize.height < dim.height) {
            dim.height = constrainSize.height;
        }
        
        // create frame
        // no need to create it if measurement mode
        CTFrameRef frame = NULL;
        if(!pInfo->sizeOnly) {
            CGMutablePathRef path = CGPathCreateMutable();
            CGPathAddRect(path, NULL, CGRectMake(0, 0, dim.width, dim.height));
            frame = CTFramesetterCreateFrame(fs,
                                             CFRangeMake(0, 0),
                                             path,
                                             NULL);
            CFRelease(path);
        }
        
        // compute the padding needed by stroke, shift x because we need outer stroke
        float leftPadding = 0;
        float rightPadding = 0;
        float topPadding = 0;
        float bottomPadding = 0;
        if (pInfo->hasStroke) {
            leftPadding = rightPadding = ceilf(pInfo->strokeSize);
            topPadding = bottomPadding = ceilf(pInfo->strokeSize);
        }
        
        // shadow offset is treated as point in cocoa, so we need multiple with native scale
        float nativeScale = [UIScreen mainScreen].scale;
        
        // compute padding needed by shadow, choose the max one
        // however, shadow is a one side so we need add stroke for another side
        if (pInfo->hasShadow) {
            if(pInfo->shadowOffset.width > 0) {
                rightPadding = MAX(rightPadding, pInfo->shadowOffset.width * nativeScale);
            } else {
                leftPadding = MAX(leftPadding, fabs(pInfo->shadowOffset.width * nativeScale));
            }
            if(pInfo->shadowOffset.height > 0) {
                topPadding = MAX(topPadding, pInfo->shadowOffset.height * nativeScale);
            } else {
                bottomPadding = MAX(bottomPadding, fabs(pInfo->shadowOffset.height * nativeScale));
            }
        }
        
        // set drawing offset
        startX += leftPadding;
        startY += bottomPadding;
        
        // add the padding (this could be 0 if no shadow and no stroke)
        dim.width  += leftPadding + rightPadding;
        dim.height += topPadding + bottomPadding;
        
        // ensure dimension is integer, if float, CGBitmapContextCreate will fail
        dim.width = floorf(dim.width);
        dim.height = floorf(dim.height);
        
        // save shadow stroke padding
        if(outShadowStrokePadding) {
            outShadowStrokePadding->x = startX;
            outShadowStrokePadding->y = startY;
        }
        
        // allocate data for bitmap
        // but don't do that if in measurement mode
        unsigned char* data = NULL;
        CGContextRef context = NULL;
        if(!pInfo->sizeOnly) {
            data = new unsigned char[(int)(dim.width * dim.height * 4)];
            memset(data, 0, (int)(dim.width * dim.height * 4));
            
            // create context
            context = CGBitmapContextCreate(data,
                                            dim.width,
                                            dim.height,
                                            8,
                                            dim.width * 4,
                                            colorSpace,
                                            kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
        }
        
        // if fail to create context, break
        // if ok, draw text by core text
        do {
            CC_BREAK_IF(!context);
            
            // store the current context
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
            UIGraphicsPushContext(context);
#else
            [NSGraphicsContext saveGraphicsState];
            NSGraphicsContext* nsCtx = [NSGraphicsContext graphicsContextWithGraphicsPort:context
                                                                                  flipped:YES];
            [NSGraphicsContext setCurrentContext:nsCtx];
#endif
            
            // alow anti-aliasing
            CGContextSetAllowsAntialiasing(context, YES);
            
            // vertical alignment
            CGContextTranslateCTM(context, startX, startY);
            
            // take care of shadow if needed
            if (pInfo->hasShadow) {
                CGSize offset;
                offset.height = pInfo->shadowOffset.height;
                offset.width  = pInfo->shadowOffset.width;
                CGFloat comp[] = {
                    ((pInfo->shadowColor >> 16) & 0xff) / 255.0f,
                    ((pInfo->shadowColor >> 8) & 0xff) / 255.0f,
                    (pInfo->shadowColor & 0xff) / 255.0f,
                    ((pInfo->shadowColor >> 24) & 0xff) / 255.0f
                };
                CGColorRef shadowColor = CGColorCreate(colorSpace, comp);
                CGContextSetShadowWithColor(context, offset, pInfo->shadowBlur, shadowColor);
                CGColorRelease(shadowColor);
            }
            
            // take care of stroke if needed
            // double size stroke because cocoa draw stroke in center, we need a outer stroke
            if (pInfo->hasStroke) {
                CGContextSetTextDrawingMode(context, kCGTextStroke);
                CGContextSetLineJoin(context, kCGLineJoinRound);
                CGContextSetRGBStrokeColor(context, pInfo->strokeColorR, pInfo->strokeColorG, pInfo->strokeColorB, 1);
                CGContextSetLineWidth(context, pInfo->strokeSize * 2);
            }
            
            // draw double sized stroke, if has
            CTFrameDraw(frame, context);
            
            // for stroke, we paint text again to override inner part, so that
            // finally we get a outer stroke effect
            if(pInfo->hasStroke) {
                CGContextSetShadowWithColor(context, CGSizeZero, 0, NULL);
                CGContextSetTextDrawingMode(context, kCGTextFill);
                CTFrameDraw(frame, context);
            }
            
            // if has cached images, try render those images
            renderEmbededImages(context, frame, plain, spans, *imageRects);
            
            // cover undisplayed characters
            if(pInfo->toCharIndex > 0) {
                coverUndisplayedCharacters(context, frame, pInfo->toCharIndex, startX, startY);
            }
            
            // if has link tag, build link info
            if(linkMetas)
                extractLinkMeta(frame, spans, *linkMetas);
            
            // pop the context
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
            UIGraphicsPopContext();
#else
            [NSGraphicsContext restoreGraphicsState];
#endif
        } while(0);
        
        // release
        if(context)
            CGContextRelease(context);
        CGColorSpaceRelease(colorSpace);
        CFRelease(plainCFStr);
        CFRelease(plainCFAStr);
        CFRelease(fs);
        CFRelease(defaultFont);
        if(frame)
            CFRelease(frame);
        CFRelease(paraStyle);
        free(plain);
        for(SpanList::iterator iter = spans.begin(); iter != spans.end(); iter++) {
            Span& span = *iter;
            if(span.type == FONT && !span.close) {
                CC_SAFE_FREE(span.fontName);
            } else if(span.type == IMAGE && !span.close) {
                CC_SAFE_FREE(span.imageName);
                CC_SAFE_FREE(span.plist);
                CC_SAFE_FREE(span.atlas);
            }
        }
        [s_imageMap removeAllObjects];
        
        // output params
        pInfo->data                 = data;
        pInfo->hasAlpha             = true;
        pInfo->isPremultipliedAlpha = true;
        pInfo->bitsPerComponent     = 8;
        pInfo->width                = dim.width;
        pInfo->height               = dim.height;
        pInfo->realLength           = (int)plainLen;
        bRet                        = true;
    } while(0);
    
    return bRet;
}

static bool _initWithImage(CGImageRef cgImage, tImageInfo *pImageinfo)
{
    if(cgImage == NULL) 
    {
        return false;
    }
    
    // get image info
    
    pImageinfo->width = CGImageGetWidth(cgImage);
    pImageinfo->height = CGImageGetHeight(cgImage);
    
    CGImageAlphaInfo info = CGImageGetAlphaInfo(cgImage);
    pImageinfo->hasAlpha = (info == kCGImageAlphaPremultipliedLast) 
                            || (info == kCGImageAlphaPremultipliedFirst) 
                            || (info == kCGImageAlphaLast) 
                            || (info == kCGImageAlphaFirst);
    
    // If OS version < 5.x, add condition to support jpg
    float systemVersion = [[[UIDevice currentDevice] systemVersion] floatValue];
    if(systemVersion < 5.0f)
    {
        pImageinfo->hasAlpha = (pImageinfo->hasAlpha || (info == kCGImageAlphaNoneSkipLast));
    }
    
    CGColorSpaceRef colorSpace = CGImageGetColorSpace(cgImage);
    if (colorSpace)
    {
        if (pImageinfo->hasAlpha)
        {
            info = kCGImageAlphaPremultipliedLast;
            pImageinfo->isPremultipliedAlpha = true;
        }
        else 
        {
            info = kCGImageAlphaNoneSkipLast;
            pImageinfo->isPremultipliedAlpha = false;
        }
    }
    else
    {
        return false;
    }
    
    // change to RGBA8888
    pImageinfo->hasAlpha = true;
    pImageinfo->bitsPerComponent = 8;
    pImageinfo->data = new unsigned char[pImageinfo->width * pImageinfo->height * 4];
    colorSpace = CGColorSpaceCreateDeviceRGB();
    CGContextRef context = CGBitmapContextCreate(pImageinfo->data, 
                                                 pImageinfo->width, 
                                                 pImageinfo->height,
                                                 8, 
                                                 4 * pImageinfo->width, 
                                                 colorSpace, 
                                                 info | kCGBitmapByteOrder32Big);
    
    CGContextClearRect(context, CGRectMake(0, 0, pImageinfo->width, pImageinfo->height));
    //CGContextTranslateCTM(context, 0, 0);
    CGContextDrawImage(context, CGRectMake(0, 0, pImageinfo->width, pImageinfo->height), cgImage);
    
    CGContextRelease(context);
    CFRelease(colorSpace);
  
    return true;
}

static bool _initWithFile(const char* path, tImageInfo *pImageinfo)
{
    CGImageRef                CGImage;    
    UIImage                    *jpg;
    UIImage                    *png;
    bool            ret;
    
    // convert jpg to png before loading the texture
    
    NSString *fullPath = [NSString stringWithUTF8String:path];
    jpg = [[UIImage alloc] initWithContentsOfFile: fullPath];
    png = [[UIImage alloc] initWithData:UIImagePNGRepresentation(jpg)];
    CGImage = png.CGImage;    
    
    ret = _initWithImage(CGImage, pImageinfo);
    
    [png release];
    [jpg release];
    
    return ret;
}


static bool _initWithData(void * pBuffer, int length, tImageInfo *pImageinfo)
{
    bool ret = false;
    
    if (pBuffer) 
    {
        CGImageRef CGImage;
        NSData *data;
        
        data = [NSData dataWithBytes:pBuffer length:length];
        CGImage = [[UIImage imageWithData:data] CGImage];
        
        ret = _initWithImage(CGImage, pImageinfo);
    }
    
    return ret;
}

static CGSize _calculateStringSize(NSString *str, id font, CGSize *constrainSize)
{
    NSArray *listItems = [str componentsSeparatedByString: @"\n"];
    CGSize dim = CGSizeZero;
    CGSize textRect = CGSizeZero;
    textRect.width = constrainSize->width > 0 ? constrainSize->width
                                              : 0x7fffffff;
    textRect.height = constrainSize->height > 0 ? constrainSize->height
                                              : 0x7fffffff;
    
    
    for (NSString *s in listItems)
    {
        CGSize tmp = [s sizeWithFont:font constrainedToSize:textRect];
        
        if (tmp.width > dim.width)
        {
           dim.width = tmp.width; 
        }
        
        dim.height += tmp.height;
    }
    
    dim.width = ceilf(dim.width);
    dim.height = ceilf(dim.height);
    
    return dim;
}

// refer CCImage::ETextAlign
#define ALIGN_TOP    1
#define ALIGN_CENTER 3
#define ALIGN_BOTTOM 2

CCImage::CCImage()
: m_nWidth(0)
, m_nHeight(0)
, m_nBitsPerComponent(0)
, m_pData(0)
, m_bHasAlpha(false)
, m_bPreMulti(false)
, m_shadowStrokePadding(CCPointZero)
, m_realLength(0)
{
    
}

CCImage::~CCImage()
{
    CC_SAFE_DELETE_ARRAY(m_pData);
}

bool CCImage::initWithImageFile(const char * strPath, EImageFormat eImgFmt/* = eFmtPng*/)
{
	bool bRet = false;
    size_t nSize = 0;
    unsigned char* pBuffer = CCFileUtils::sharedFileUtils()->getFileData(
				CCFileUtils::sharedFileUtils()->fullPathForFilename(strPath).c_str(),
				"rb",
				&nSize);
				
    if (pBuffer != NULL && nSize > 0)
    {
        bRet = initWithImageData(pBuffer, nSize, eImgFmt);
    }
    CC_SAFE_DELETE_ARRAY(pBuffer);
    return bRet;
}

bool CCImage::initWithImageFileThreadSafe(const char *fullpath, EImageFormat imageType)
{
    /*
     * CCFileUtils::fullPathFromRelativePath() is not thread-safe, it use autorelease().
     */
    bool bRet = false;
    size_t nSize = 0;
    unsigned char* pBuffer = CCFileUtils::sharedFileUtils()->getFileData(fullpath, "rb", &nSize);
    if (pBuffer != NULL && nSize > 0)
    {
        bRet = initWithImageData(pBuffer, nSize, imageType);
    }
    CC_SAFE_DELETE_ARRAY(pBuffer);
    return bRet;
}

bool CCImage::initWithImageData(void * pData, 
                                int nDataLen, 
                                EImageFormat eFmt,
                                int nWidth,
                                int nHeight,
                                int nBitsPerComponent)
{
    bool bRet = false;
    tImageInfo info = {0};
    
    info.hasShadow = false;
    info.hasStroke = false;
    
    do 
    {
        CC_BREAK_IF(! pData || nDataLen <= 0);
        if (eFmt == kFmtRawData)
        {
            bRet = _initWithRawData(pData, nDataLen, nWidth, nHeight, nBitsPerComponent, false);
        }
        else if (eFmt == kFmtWebp)
        {
            bRet = _initWithWebpData(pData, nDataLen);
        }
        else // init with png or jpg file data
        {
            bRet = _initWithData(pData, nDataLen, &info);
            if (bRet)
            {
                m_nHeight = (short)info.height;
                m_nWidth = (short)info.width;
                m_nBitsPerComponent = info.bitsPerComponent;
                m_bHasAlpha = info.hasAlpha;
                m_bPreMulti = info.isPremultipliedAlpha;
                m_pData = info.data;
            }
        }
    } while (0);
    
    return bRet;
}

bool CCImage::_initWithRawData(void *pData, int nDatalen, int nWidth, int nHeight, int nBitsPerComponent, bool bPreMulti)
{
    bool bRet = false;
    do 
    {
        CC_BREAK_IF(0 == nWidth || 0 == nHeight);

        m_nBitsPerComponent = nBitsPerComponent;
        m_nHeight   = (short)nHeight;
        m_nWidth    = (short)nWidth;
        m_bHasAlpha = true;

        // only RGBA8888 supported
        int nBytesPerComponent = 4;
        int nSize = nHeight * nWidth * nBytesPerComponent;
        m_pData = new unsigned char[nSize];
        CC_BREAK_IF(! m_pData);
        memcpy(m_pData, pData, nSize);

        bRet = true;
    } while (0);
    return bRet;
}

bool CCImage::_initWithJpgData(void *pData, int nDatalen)
{
    assert(0);
	return false;
}

bool CCImage::_initWithPngData(void *pData, int nDatalen)
{
    assert(0);
	return false;
}

bool CCImage::_saveImageToPNG(const char *pszFilePath, bool bIsToRGB)
{
    assert(0);
	return false;
}

bool CCImage::_saveImageToJPG(const char *pszFilePath)
{
    assert(0);
	return false;
}

bool CCImage::initWithString(
                            const char * pText,
                            int         nWidth /* = 0 */,
                            int         nHeight /* = 0 */,
                            ETextAlign eAlignMask /* = kAlignCenter */,
                            const char * pFontName /* = nil */,
                            int         nSize /* = 0 */)
{
    return initWithStringShadowStroke(pText, nWidth, nHeight, eAlignMask , pFontName, nSize);
}

bool CCImage::initWithStringShadowStroke(const char * pText,
                                                       int         nWidth,
                                                       int         nHeight,
                                                       ETextAlign eAlignMask,
                                                       const char * pFontName,
                                                       int         nSize,
                                                       float       textTintR,
                                                       float       textTintG,
                                                       float       textTintB,
                                                       bool shadow,
                                                       float shadowOffsetX,
                                                       float shadowOffsetY,
                                                       unsigned int shadowColor,
                                                       float shadowBlur,
                                                       bool  stroke,
                                                       float strokeR,
                                                       float strokeG,
                                                       float strokeB,
                                                       float strokeSize,
                                                       float lineSpacing,
                                                       float globalImageScaleFactor,
                                                       int toCharIndex,
                                                       float elapsedTime) {
    tImageInfo info = {0};
    info.width                  = nWidth;
    info.height                 = nHeight;
    info.hasShadow              = shadow;
    info.shadowOffset.width     = shadowOffsetX;
    info.shadowOffset.height    = shadowOffsetY;
    info.shadowBlur             = shadowBlur;
    info.shadowColor            = shadowColor;
    info.hasStroke              =  stroke;
    info.strokeColorR           =  strokeR;
    info.strokeColorG           = strokeG;
    info.strokeColorB           = strokeB;
    info.strokeSize             = strokeSize;
    info.tintColorR             = textTintR;
    info.tintColorG             = textTintG;
    info.tintColorB             = textTintB;
    info.lineSpacing            = lineSpacing;
    info.globalImageScaleFactor = globalImageScaleFactor;
    info.toCharIndex            = toCharIndex;
    info.elapsedTime            = elapsedTime;
    info.sizeOnly = false;
    info.needTime = false;
    
    if (!_initWithString(pText, eAlignMask, pFontName, nSize, &info, &m_shadowStrokePadding, &m_linkMetas, &m_imageRects))
    {
        return false;
    }
    m_nHeight = (short)info.height;
    m_nWidth = (short)info.width;
    m_nBitsPerComponent = info.bitsPerComponent;
    m_bHasAlpha = info.hasAlpha;
    m_bPreMulti = info.isPremultipliedAlpha;
    m_pData = info.data;
    m_realLength = info.realLength;
    m_needTime = info.needTime;
    
    return true;
}

CCSize CCImage::measureString(const char* pText,
                              const char* pFontName,
                              int nSize,
                              int maxWidth,
                              float shadowOffsetX,
                              float shadowOffsetY,
                              float strokeSize,
                              float lineSpacing,
                              float globalImageScaleFactor) {
    tImageInfo info = {0};
    info.width                  = maxWidth;
    info.height                 = 0;
    info.hasShadow              = shadowOffsetX != 0 || shadowOffsetY != 0;
    info.shadowOffset.width     = shadowOffsetX;
    info.shadowOffset.height    = shadowOffsetY;
    info.shadowBlur             = 0;
    info.shadowColor            = 0;
    info.hasStroke              = strokeSize != 0;
    info.strokeColorR           = 0;
    info.strokeColorG           = 0;
    info.strokeColorB           = 0;
    info.strokeSize             = strokeSize;
    info.tintColorR             = 0;
    info.tintColorG             = 0;
    info.tintColorB             = 0;
    info.lineSpacing            = lineSpacing;
    info.globalImageScaleFactor = globalImageScaleFactor;
    info.sizeOnly = true;
    
    if (!_initWithString(pText, kAlignCenter, pFontName, nSize, &info, NULL, NULL, NULL)) {
        return CCSizeZero;
    }
    return CCSizeMake(info.width, info.height);
}

bool CCImage::saveToFile(const char *pszFilePath, bool bIsToRGB)
{
    bool saveToPNG = false;
    bool needToCopyPixels = false;
    std::string filePath(pszFilePath);
    if (std::string::npos != filePath.find(".png"))
    {
        saveToPNG = true;
    }
        
    int bitsPerComponent = 8;            
    int bitsPerPixel = m_bHasAlpha ? 32 : 24;
    if ((! saveToPNG) || bIsToRGB)
    {
        bitsPerPixel = 24;
    }            
    
    int bytesPerRow    = (bitsPerPixel/8) * m_nWidth;
    int myDataLength = bytesPerRow * m_nHeight;
    
    unsigned char *pixels    = m_pData;
    
    // The data has alpha channel, and want to save it with an RGB png file,
    // or want to save as jpg,  remove the alpha channel.
    if ((saveToPNG && m_bHasAlpha && bIsToRGB)
       || (! saveToPNG))
    {
        pixels = new unsigned char[myDataLength];
        
        for (int i = 0; i < m_nHeight; ++i)
        {
            for (int j = 0; j < m_nWidth; ++j)
            {
                pixels[(i * m_nWidth + j) * 3] = m_pData[(i * m_nWidth + j) * 4];
                pixels[(i * m_nWidth + j) * 3 + 1] = m_pData[(i * m_nWidth + j) * 4 + 1];
                pixels[(i * m_nWidth + j) * 3 + 2] = m_pData[(i * m_nWidth + j) * 4 + 2];
            }
        }
        
        needToCopyPixels = true;
    }
        
    // make data provider with data.
    CGBitmapInfo bitmapInfo = kCGBitmapByteOrderDefault;
    if (saveToPNG && m_bHasAlpha && (! bIsToRGB))
    {
        bitmapInfo |= kCGImageAlphaPremultipliedLast;
    }
    CGDataProviderRef provider        = CGDataProviderCreateWithData(NULL, pixels, myDataLength, NULL);
    CGColorSpaceRef colorSpaceRef    = CGColorSpaceCreateDeviceRGB();
    CGImageRef iref                    = CGImageCreate(m_nWidth, m_nHeight,
                                                        bitsPerComponent, bitsPerPixel, bytesPerRow,
                                                        colorSpaceRef, bitmapInfo, provider,
                                                        NULL, false,
                                                        kCGRenderingIntentDefault);
        
    UIImage* image                    = [[UIImage alloc] initWithCGImage:iref];
        
    CGImageRelease(iref);    
    CGColorSpaceRelease(colorSpaceRef);
    CGDataProviderRelease(provider);
    
    NSData *data;
                
    if (saveToPNG)
    {
        data = UIImagePNGRepresentation(image);
    }
    else
    {
        data = UIImageJPEGRepresentation(image, 1.0f);
    }
    
    [data writeToFile:[NSString stringWithUTF8String:pszFilePath] atomically:YES];
        
    [image release];
        
    if (needToCopyPixels)
    {
        delete [] pixels;
    }
    
    return true;
}

NS_CC_END

