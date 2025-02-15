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
#include "support/utils/CCUtils.h"
#include "entities.h"
#include "CCLocale.h"
#include "ccMacros.h"
#include "platform/CCImage.h"
#include "CCPinyinUtils.h"
#include "cocos-ext.h"
#include "Unicode.h"
#include "support/codec/hash_bob_jenkins_v2.h"

USING_NS_CC_EXT;
using namespace cocos2d::ui;

NS_CC_BEGIN

CCArray CCUtils::s_tmpArray;

unsigned long CCUtils::nextPOT(unsigned long x) {
    x = x - 1;
    x = x | (x >> 1);
    x = x | (x >> 2);
    x = x | (x >> 4);
    x = x | (x >> 8);
    x = x | (x >>16);
    return x + 1;
}

unsigned char CCUtils::UnitScalarToByte(float x) {
    if (x < 0) {
        return 0;
    }
    if (x >= 1) {
        return 255;
    }
    return (int)(x * (1 << 16)) >> 8;
}

const char* CCUtils::copy(const char* src) {
	if(src == NULL)
		return NULL;
    
	size_t len = strlen(src);
	char* c = (char*)calloc(len + 1, sizeof(char));
	memcpy(c, src, len);
	return c;
}

const char* CCUtils::copy(const char* src, int start, size_t len) {
	if(src == NULL)
		return NULL;
    
	char* c = (char*)calloc(len + 1, sizeof(char));
	memcpy(c, src + start, len);
	return c;
}

void CCUtils::toLowercase(string& s) {
	if(s.empty())
		return;
	
    size_t len = s.length();
	char* buf = new char[len + 1];
	strcpy(buf, s.c_str());
    for(int i = 0; i < len; i++)
		if(buf[i] >= 0x41 && buf[i] <= 0x5A)
			buf[i] += 0x20;
	
	s.copy(buf, len);
	delete buf;
}

bool CCUtils::startsWith(const string& s, const string& sub) {
    return s.find(sub) == 0;
}

bool CCUtils::endsWith(const string& s, const string& sub) {
    return s.rfind(sub) == s.length() - sub.length();
}

void CCUtils::replaceChar(string& s, char c, char sub) {
    size_t len = s.length();
	char* buf = new char[len + 1];
    strcpy(buf, s.c_str());
    
	for(int i = 0; i < len; i++) {
		if(buf[i] == c) {
            buf[i] = sub;
        }
	}
    
    s.copy(buf, len);
    delete buf;
}

string CCUtils::replace(string& s, const string& c, const string& sub) {
    string ret = s;
    size_t pos = ret.length();
    while(true)   {
        if((pos = ret.rfind(c, pos)) != string::npos)
            ret = ret.replace(pos, c.length(), sub);
        else
            break;
    }
    
    return ret;
}

string CCUtils::decodeHtmlEntities(const string& src) {
	char* dest = (char*)calloc(src.length() + 1, sizeof(char));
	decode_html_entities_utf8(dest, src.c_str());
	string decoded = string(dest);
	free(dest);
	return decoded;
}

string CCUtils::getPinyin(const string& s) {
    return CCPinyinUtils::chs2Pinyin(s);
}

int CCUtils::getNumDigits(int num) {
    int d = 1;
    num /= 10;
    while(num > 0) {
        d++;
        num /= 10;
    }
    return d;
}

ssize_t CCUtils::lastDotIndex(const string& path) {
	if(path.empty())
		return -1;
    
	ssize_t len = path.length();
	for(ssize_t i = len - 1; i >= 0; i--) {
		if(path[i] == '.')
			return i;
	}
    
	return -1;
}

ssize_t CCUtils::lastSlashIndex(string path) {
	if(path.empty())
		return -1;
    
	// change slash to windows format
    if(CC_PATH_SEPARATOR != '/')
        replaceChar(path, '/', CC_PATH_SEPARATOR);
    
	// find slash index
	size_t len = path.length();
	int end = len;
	int slash = -1;
	for(int i = len - 1; i >= 0; i--) {
		if(path[i] == CC_PATH_SEPARATOR) {
			if(i == end - 1) {
				end--;
				if(i == 0) {
					slash = 0;
					break;
				}
			} else {
				slash = i;
				break;
			}
		}
	}
    
	// skip extra slash
	if(slash != -1) {
		while(slash >= 1 && path[slash - 1] == CC_PATH_SEPARATOR)
			slash--;
	}
    
	// assign to end
	end = slash;
	if(end == 0)
		end = 1;
    
	return end;
}

string CCUtils::lastPathComponent(const string& path) {
	// change slash to windows format
	if(CC_PATH_SEPARATOR != '/')
		replaceChar((string&)path, '/', CC_PATH_SEPARATOR);
    
	size_t len = path.length();
	int start = 0;
	int end = len;
	for(int i = len - 1; i >= 0; i--) {
		if(path[i] == CC_PATH_SEPARATOR) {
			if(i == end - 1)
				end--;
			else {
				start = i + 1;
				break;
			}
		}
	}
    
	if(end < start)
		return path;
	else
		return path.substr(start, end);
}

string CCUtils::deleteLastPathComponent(const string& path) {
	ssize_t end = lastSlashIndex(path);
	if(end < 0)
		return "";
	else
		return path.substr(0, end);
}

string CCUtils::appendPathComponent(const string& path, const string& component) {
	// change slash to windows format
	if(CC_PATH_SEPARATOR != '/')
		replaceChar((string&)path, '/', CC_PATH_SEPARATOR);
    
	// validating
	if(path.empty()) {
		if(component.empty())
			return "";
		else
			return component;
	} else if(component.empty()) {
		return path;
    }
    
	// allocate a big enough buffer
	// plus 2 because one for slash, one for null terminator
	size_t len = path.length();
	int cLen = component.length();
    char* buf = new char[len + cLen + 2];
    memset(buf, 0, len + cLen + 2);
    
	// copy path
	memcpy(buf, path.c_str(), len);
    
	// take care of slash
	int start = len;
	if(start > 0) {
		if(buf[start - 1] != CC_PATH_SEPARATOR) {
			buf[start++] = CC_PATH_SEPARATOR;
		} else {
			while(start >= 2 && buf[start - 2] == CC_PATH_SEPARATOR)
				start--;
		}
	}
    
	// copy component
	int cStart = 0;
	while(cStart < cLen && component[cStart] == CC_PATH_SEPARATOR)
		cStart++;
	if(cStart > 0 && start == 0)
		cStart--;
	memcpy(buf + start, component.c_str() + cStart, cLen - cStart);
    
    // remove end slash
    int end = start + cLen - cStart - 1;
    while(buf[end] == CC_PATH_SEPARATOR)
        buf[end--] = 0;
    
    string ret = buf;
    delete buf;
	return ret;
}

string CCUtils::deletePathExtension(const string& path) {
    ssize_t end = lastDotIndex(path);
	ssize_t slash = lastSlashIndex((string&)path);
	if(end >= 0) {
		if(end > slash)
			return path.substr(0, end);
		else
			return path;
	} else {
		return path;
	}
}

string CCUtils::getPathExtension(const string& path) {
	ssize_t end = lastDotIndex(path);
	if(end >= 0) {
		return path.substr(end);
	} else {
		return "";
	}
}

string CCUtils::getParentPath(const string& path) {
	if(path.empty())
		return "";
	
	size_t slash = path.rfind('/');
	if(slash == string::npos)
		return "";
	else if(slash == 0)
		return path.substr(0, 1);
	else
		return path.substr(0, slash);
}

bool CCUtils::createIntermediateFolders(const string& path) {
	string parent = getParentPath(path);
	bool exist = isPathExistent(parent);
	bool success = true;
	if(!exist) {
		createIntermediateFolders(parent);
		success = createFolder(parent);
	}
	
	// return success flag
	return success;
}

ccColorHSV CCUtils::ccc32hsv(ccColor3B c) {
	unsigned char min = MIN(c.r, MIN(c.g, c.b));
    unsigned char max = MAX(c.r, MAX(c.g, c.b));
    unsigned char delta = max - min;
	
    // get v
    float v = max / 255.f;
	
    // if input color a gray color?
    if(delta == 0) {
    	return cchsv(0, 0, v);
    }
	
    // get s
    float s = (float)delta / max;
	
    // get h
    float h;
    if(c.r == max) {
        h = (float)(c.g - c.b) / delta;
    } else if (c.g == max) {
        h = 2 + (float)(c.b - c.r) / delta;
    } else { // b == max
        h = 4 + (float)(c.r - c.g) / delta;
    }
	
    // adjust h
    h *= 60;
    if (h < 0) {
        h += 360;
    }
	
    return cchsv(h, s, v);
}

ccColor3B CCUtils::hsv2ccc3(ccColorHSV c) {
	unsigned char s = UnitScalarToByte(c.s);
    unsigned char v = UnitScalarToByte(c.v);
	
    // if s is zero then rgb color is gray
    if (0 == s) {
    	return ccc3(v, v, v);
    }
	
    int hx = (c.h < 0 || c.h >= 360.f) ? 0 : (int)((c.h / 60) * (1 << 16));
    int f = hx & 0xFFFF;
	
    unsigned char v_scale = v + 1;
    unsigned char p = ((255 - s) * v_scale) >> 8;
    unsigned char q = ((255 - (s * f >> 16)) * v_scale) >> 8;
    unsigned char t = ((255 - (s * ((1 << 16) - f) >> 16)) * v_scale) >> 8;
	
    unsigned char r, g, b;
    switch (hx >> 16) {
        case 0:
        	r = v;
        	g = t;
        	b = p;
        	break;
        case 1:
        	r = q;
        	g = v;
        	b = p;
        	break;
        case 2:
        	r = p;
        	g = v;
        	b = t;
        	break;
        case 3:
        	r = p;
        	g = q;
        	b = v;
        	break;
        case 4:
        	r = t;
        	g = p;
        	b = v;
        	break;
        default:
        	r = v;
        	g = p;
        	b = q;
        	break;
    }
	
    return ccc3(r, g, b);
}

CCPoint CCUtils::getOrigin(CCNode* node) {
	if(node->isIgnoreAnchorPointForPosition()) {
		return node->getPosition();
	} else {
		return ccp(node->getPositionX() - node->getAnchorPointInPoints().x,
				   node->getPositionY() - node->getAnchorPointInPoints().y);
	}
}

CCPoint CCUtils::getCenter(CCNode* node) {
	if(node->isIgnoreAnchorPointForPosition()) {
		return ccpAdd(node->getPosition(), ccp(node->getContentSize().width / 2, node->getContentSize().height / 2));
	} else {
		return ccpAdd(ccp(node->getPositionX() - node->getAnchorPointInPoints().x,
						  node->getPositionY() - node->getAnchorPointInPoints().y),
					  ccp(node->getContentSize().width / 2, node->getContentSize().height / 2));
	}
}

CCPoint CCUtils::getLocalCenter(CCNode* node) {
    return ccp(node->getContentSize().width / 2,
               node->getContentSize().height / 2);
}

CCPoint CCUtils::getLocalPoint(CCNode* node, float xpercent, float ypercent) {
	CCSize s = node->getContentSize();
	return ccp(s.width * xpercent, s.height * ypercent);
}

CCPoint CCUtils::getLocalPoint(CCNode* node, CCPoint anchor) {
	CCSize s = node->getContentSize();
	return ccp(s.width * anchor.x, s.height * anchor.y);
}

CCPoint CCUtils::getPoint(CCNode* node, float xpercent, float ypercent) {
	CCPoint origin = getOrigin(node);
	CCSize size = node->getContentSize();
	return ccp(origin.x + size.width * xpercent, origin.y + size.height * ypercent);
}

CCRect CCUtils::getBoundingBoxInWorldSpace(CCNode* node) {
	CCRect r;
	r.origin = CCPointZero;
	r.size = node->getContentSize();
	CCAffineTransform t = node->nodeToWorldTransform();
	r = CCRectApplyAffineTransform(r, t);
	return r;
}

CCRect CCUtils::getCenterRect(CCSpriteFrame* f) {
    const CCSize& size = f->getOriginalSize();
    return CCRectMake(size.width / 2, size.height / 2, 1, 1);
}

CCRect CCUtils::getCenterRect(const string& frameName) {
    CCSpriteFrame* f = CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(frameName.c_str());
    const CCSize& size = f->getOriginalSize();
    return CCRectMake(size.width / 2, size.height / 2, 1, 1);
}

bool CCUtils::containsRect(const CCRect& r1, const CCRect& r2) {
    return r1.getMinX() <= r2.getMinX() &&
        r1.getMaxX() >= r2.getMaxX() &&
        r1.getMinY() <= r2.getMinY() &&
        r1.getMaxY() >= r2.getMaxY();
}

CCScene* CCUtils::getScene(CCNode* n) {
    while(n) {
        CCScene* s = dynamic_cast<CCScene*>(n);
        if(s) {
            return s;
        } else {
            n = n->getParent();
        }
    }
    
    return NULL;
}

bool CCUtils::testSegmentAABB(CCPoint p0, CCPoint p1, ccAABB b) {
	CCPoint c = ccpMult(ccpAdd(b.min, b.max), 0.5f);
	CCPoint e = ccpSub(b.max, c);
	CCPoint m = ccpMult(ccpAdd(p0, p1), 0.5f);
	CCPoint d = ccpSub(p1, m);
	m = ccpSub(m, c);
	
	// Box center-point
	// Box halflength extents
	// Segment midpoint
	// Segment halflength vector
	// Translate box and segment to origin
	// Try world coordinate axes as separating axes
	float adx = fabs(d.x);
	if (fabs(m.x) > e.x + adx)
		return false;
	float ady = fabs(d.y);
	if (fabs(m.y) > e.y + ady)
		return false;
	
	// Add in an epsilon term to counteract arithmetic errors when segment is
	// (near) parallel to a coordinate axis (see text for detail)
	adx += FLT_EPSILON;
	ady += FLT_EPSILON;
	
	// Try cross products of segment direction vector with coordinate axes
	if (fabs(m.x * d.y - m.y * d.x) > e.x * ady + e.y * adx)
		return false;
	
	// No separating axis found; segment must be overlapping AABB
	return true;
}

int CCUtils::binarySearch(int* a, size_t len, int key) {
	int low = 0;
	int high = len - 1;
	
	while(low <= high) {
		int mid = (low + high) >> 1;
		int midVal = a[mid];
		
		if(midVal < key)
			low = mid + 1;
		else if(midVal > key)
			high = mid - 1;
		else
			return mid; // key found
	}
	return -(low + 1); // key not found.
}

CCRect CCUtils::combine(const CCRect& r1, const CCRect& r2) {
    float left = MIN(r1.origin.x, r2.origin.x);
	float right = MAX(r1.origin.x + r1.size.width, r2.origin.x + r2.size.width);
	float bottom = MIN(r1.origin.y, r2.origin.y);
	float top = MAX(r1.origin.y + r1.size.height, r2.origin.y + r2.size.height);
	return CCRectMake(left, bottom, right - left, top - bottom);
}

int64_t CCUtils::currentTimeMillis() {
	struct timeval tv;
	gettimeofday(&tv, (struct timezone *) NULL);
	int64_t when = tv.tv_sec * 1000LL + tv.tv_usec / 1000;
	return when;
}

CCArray& CCUtils::intComponentsOfString(const string& s, const char sep) {
    // returned string list
    s_tmpArray.removeAllObjects();
    
    // quick reject
    if(s.empty()) {
        return s_tmpArray;
    }
    
    // remove head and tailing brace, bracket, parentheses
    size_t start = 0;
    size_t end = s.length() - 1;
    while(start < end) {
        while(s[start] == ' ' && start < end) {
            start++;
        }
        while(s[end] == ' ' && start < end) {
            end--;
        }
        char cs = s[start];
        char ce = s[end];
        if((cs == '{' && ce == '}') || (cs == '[' && ce == ']') || (cs == '(' && ce == ')')) {
            start++;
            end--;
        } else {
            break;
        }
    }
    
    // iterate string
    size_t compStart = start;
    for(size_t i = start; i <= end; i++) {
        char c = s[i];
        if(c == sep) {
            s_tmpArray.addObject(CCInteger::create(atoi(s.substr(compStart, i - compStart).c_str())));
            compStart = i + 1;
        } else if(c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            if(compStart == i) {
                compStart++;
            }
        }
    }
    
    // last comp
    if(compStart <= end) {
        s_tmpArray.addObject(CCInteger::create(atoi(s.substr(compStart, end - compStart + 1).c_str())));
    } else if(s[end] == sep) {
        s_tmpArray.addObject(CCInteger::create(0));
    }
    
    // return
    return s_tmpArray;
}

CCArray& CCUtils::floatComponentsOfString(const string& s, const char sep) {
    // returned list
    s_tmpArray.removeAllObjects();
    
    // quick reject
    if(s.empty()) {
        return s_tmpArray;
    }
    
    // remove head and tailing brace, bracket, parentheses
    size_t start = 0;
    size_t end = s.length() - 1;
    while(start < end) {
        while(s[start] == ' ' && start < end) {
            start++;
        }
        while(s[end] == ' ' && start < end) {
            end--;
        }
        char cs = s[start];
        char ce = s[end];
        if((cs == '{' && ce == '}') || (cs == '[' && ce == ']') || (cs == '(' && ce == ')')) {
            start++;
            end--;
        } else {
            break;
        }
    }
    
    // iterate string
    size_t compStart = start;
    for(size_t i = start; i <= end; i++) {
        char c = s[i];
        if(c == sep) {
            s_tmpArray.addObject(CCFloat::create(atof(s.substr(compStart, i - compStart).c_str())));
            compStart = i + 1;
        } else if(c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            if(compStart == i) {
                compStart++;
            }
        }
    }
    
    // last comp
    if(compStart <= end) {
        s_tmpArray.addObject(CCFloat::create(atof(s.substr(compStart, end - compStart + 1).c_str())));
    } else if(s[end] == sep) {
        s_tmpArray.addObject(CCFloat::create(0));
    }
    
    // return
    return s_tmpArray;
}

CCArray& CCUtils::boolComponentsOfString(const string& s, const char sep) {
    // returned list
    s_tmpArray.removeAllObjects();
    
    // quick reject
    if(s.empty()) {
        return s_tmpArray;
    }
    
    // remove head and tailing brace, bracket, parentheses
    size_t start = 0;
    size_t end = s.length() - 1;
    while(start < end) {
        while(s[start] == ' ' && start < end) {
            start++;
        }
        while(s[end] == ' ' && start < end) {
            end--;
        }
        char cs = s[start];
        char ce = s[end];
        if((cs == '{' && ce == '}') || (cs == '[' && ce == ']') || (cs == '(' && ce == ')')) {
            start++;
            end--;
        } else {
            break;
        }
    }
    
    // iterate string
    size_t compStart = start;
    for(size_t i = start; i <= end; i++) {
        char c = s[i];
        if(c == sep) {
            string sub = s.substr(compStart, i - compStart);
            toLowercase(sub);
            s_tmpArray.addObject(CCBool::create(sub == "y" || sub == "yes" || sub == "true" || atoi(sub.c_str()) > 0));
            compStart = i + 1;
        } else if(c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            if(compStart == i) {
                compStart++;
            }
        }
    }
    
    // last comp
    if(compStart <= end) {
        string sub = s.substr(compStart, end - compStart + 1);
        toLowercase(sub);
        s_tmpArray.addObject(CCBool::create(sub == "y" || sub == "yes" || sub == "true" || atoi(sub.c_str()) > 0));
    } else if(s[end] == sep) {
        s_tmpArray.addObject(CCBool::create(false));
    }
    
    // return
    return s_tmpArray;
}

CCArray& CCUtils::componentsOfString(const string& s, const char sep) {
    // returned list
    s_tmpArray.removeAllObjects();
    
    // quick reject
    if(s.empty()) {
        return s_tmpArray;
    }
    
    // remove head and tailing brace, bracket, parentheses
    size_t start = 0;
    size_t end = s.length() - 1;
    while(start < end) {
        while(s[start] == ' ' && start < end) {
            start++;
        }
        while(s[end] == ' ' && start < end) {
            end--;
        }
        char cs = s[start];
        char ce = s[end];
        if((cs == '{' && ce == '}') || (cs == '[' && ce == ']') || (cs == '(' && ce == ')')) {
            start++;
            end--;
        } else {
            break;
        }
    }
    
    // iterate string
    size_t compStart = start;
    for(size_t i = start; i <= end; i++) {
        char c = s[i];
        if(c == sep) {
            s_tmpArray.addObject(CCString::create(s.substr(compStart, i - compStart)));
            compStart = i + 1;
        } else if(c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            if(compStart == i) {
                compStart++;
            }
        }
    }
    
    // last comp
    // or, if last char is separator, append an empty string
    if(compStart <= end) {
        s_tmpArray.addObject(CCString::create(s.substr(compStart, end - compStart + 1)));
    } else if(s[end] == sep) {
        s_tmpArray.addObject(CCString::create(""));
    }
    
    // return
    return s_tmpArray;
}

string CCUtils::joinString(const CCArray& a, const char sep) {
    string ret;
    CCObject* obj;
    CCARRAY_FOREACH(&a, obj) {
        CCString* ccs = (CCString*)obj;
        if(ret.length() > 0)
            ret += ',';
        ret += ccs->getCString();
    }
    return ret;
}

string CCUtils::joinInt(const CCArray& a, const char sep) {
    string ret;
    CCObject* obj;
    char buf[64];
    CCARRAY_FOREACH(&a, obj) {
        CCInteger* cci = (CCInteger*)obj;
        if(ret.length() > 0)
            ret += ',';
        sprintf(buf, "%d", cci->getValue());
        ret += buf;
    }
    return ret;
}

string CCUtils::joinFloat(const CCArray& a, const char sep) {
    string ret;
    CCObject* obj;
    char buf[64];
    CCARRAY_FOREACH(&a, obj) {
        CCFloat* ccf = (CCFloat*)obj;
        if(ret.length() > 0)
            ret += ',';
        sprintf(buf, "%f", ccf->getValue());
        ret += buf;
    }
    return ret;
}

string CCUtils::joinBool(const CCArray& a, const char sep) {
    string ret;
    CCObject* obj;
    CCARRAY_FOREACH(&a, obj) {
        CCBool* ccb = (CCBool*)obj;
        if(ret.length() > 0)
            ret += ',';
        ret += ccb->getValue() ? "true" : "false";
    }
    return ret;
}

CCPoint CCUtils::ccpFromString(const string& s) {
    CCArray& comp = componentsOfString(s, ',');
    float x = 0, y = 0;
    if(comp.count() > 0) {
        x = atof(((CCString*)comp.objectAtIndex(0))->getCString());
    }
    if(comp.count() > 1) {
        y = atof(((CCString*)comp.objectAtIndex(1))->getCString());
    }
    return ccp(x, y);
}

CCSize CCUtils::ccsFromString(const string& s) {
    CCArray& comp = componentsOfString(s, ',');
    float x = 0, y = 0;
    if(comp.count() > 0) {
        x = atof(((CCString*)comp.objectAtIndex(0))->getCString());
    }
    if(comp.count() > 1) {
        y = atof(((CCString*)comp.objectAtIndex(1))->getCString());
    }
    return CCSizeMake(x, y);
}

CCRect CCUtils::ccrFromString(const string& s) {
    CCArray& comp = componentsOfString(s, ',');
    float x = 0, y = 0, w = 0, h = 0;
    if(comp.count() > 0) {
        x = atof(((CCString*)comp.objectAtIndex(0))->getCString());
    }
    if(comp.count() > 1) {
        y = atof(((CCString*)comp.objectAtIndex(1))->getCString());
    }
    if(comp.count() > 2) {
        w = atof(((CCString*)comp.objectAtIndex(2))->getCString());
    }
    if(comp.count() > 3) {
        h = atof(((CCString*)comp.objectAtIndex(3))->getCString());
    }
    return CCRectMake(x, y, w, h);
}

CCArray& CCUtils::arrayFromString(const string& s) {
    CCArray& comp = componentsOfString(s, ',');
    
    // clear
    CCArray copy;
    copy.addObjectsFromArray(&comp);
    s_tmpArray.removeAllObjects();
    
    // iterator components
    CCObject* obj;
    CCARRAY_FOREACH(&copy, obj) {
        CCString* ccs = (CCString*)obj;
        string cs = ccs->getCString();
        if(cs.length() > 0) {
            if(cs[0] == '\'' || cs[0] == '"') {
                size_t start = 1;
                size_t end = cs.length() - 1;
                if(cs[end] == '\'' || cs[end] == '"') {
                    end--;
                }
                if(end >= start) {
                    s_tmpArray.addObject(CCString::create(cs.substr(start, end - start + 1)));
                } else {
                    s_tmpArray.addObject(CCString::create(""));
                }
            } else {                
                float f = atof(cs.c_str());
                s_tmpArray.addObject(CCFloat::create(f));
            }
        } else {
            s_tmpArray.addObject(CCFloat::create(0));
        }
    }
    
    // return
    return s_tmpArray;
}

string CCUtils::arrayToString(const CCArray& array) {
    string ret = "[";
    CCObject* obj;
    char buf[128]; 
    CCARRAY_FOREACH(&array, obj) {
        CCString* s = dynamic_cast<CCString*>(obj);
        if(s) {
            if(ret.length() > 1)
                ret.append(",");
            ret.append("\"");
            ret.append(s->getCString());
            ret.append("\"");
            continue;
        }
        
        CCInteger* i = dynamic_cast<CCInteger*>(obj);
        if(i) {
            if(ret.length() > 1)
                ret.append(",");
            sprintf(buf, "%d", i->getValue());
            ret.append(buf);
            continue;
        }
        
        CCFloat* f = dynamic_cast<CCFloat*>(obj);
        if(f) {
            if(ret.length() > 1)
                ret.append(",");
            sprintf(buf, "%f", f->getValue());
            ret.append(buf);
            continue;
        }
        
        CCDouble* d = dynamic_cast<CCDouble*>(obj);
        if(d) {
            if(ret.length() > 1)
                ret.append(",");
            sprintf(buf, "%lf", d->getValue());
            ret.append(buf);
            continue;
        }
    }
    
    ret.append("]");
    return ret;
}

void CCUtils::setOpacityRecursively(CCNode* node, int o) {
	CCRGBAProtocol* p = dynamic_cast<CCRGBAProtocol*>(node);
	if(p) {
		p->setOpacity(o);
	}
	
	CCArray* children = node->getChildren();
    int cc = node->getChildrenCount();
    for(int i = 0; i < cc; i++) {
        CCNode* child = (CCNode*)children->objectAtIndex(i);        
        setOpacityRecursively(child, o);
    }
    
    Widget* w = dynamic_cast<Widget*>(node);
    if(w) {
        if(w->getVirtualRenderer()) {
            CCRGBAProtocol* p = dynamic_cast<CCRGBAProtocol*>(w->getVirtualRenderer());
            if(p) {
                p->setOpacity(o);
            }
        }
        
        CCArray* children = w->getNodes();
        int cc = children->count();
        for(int i = 0; i < cc; i++) {
            CCNode* child = (CCNode*)children->objectAtIndex(i);
            setOpacityRecursively(child, o);
        }
    }
}

CCArray* CCUtils::getChildrenByTag(CCNode* parent, int tag) {
	CCArray* ret = CCArray::create();
	CCObject* obj;
	CCARRAY_FOREACH(parent->getChildren(), obj) {
		CCNode* child = (CCNode*)obj;
		if(child->getTag() == tag) {
			ret->addObject(child);
		}
	}
	
	return ret;
}

void CCUtils::removeChildrenByTag(CCNode* parent, int tag) {
	CCObject* obj;
	CCARRAY_FOREACH_REVERSE(parent->getChildren(), obj) {
		CCNode* child = (CCNode*)obj;
		if(child->getTag() == tag) {
			child->removeFromParent();
		}
	}
}

double CCUtils::pround(double x, int precision) {
    double div = pow(10, -precision);
    return floor(x / div + 0.5) * div;
}

double CCUtils::pfloor(double x, int precision) {
    double div = pow(10, -precision);
    return (int)floor(x / div) * div;
}

double CCUtils::pceil(double x, int precision) {
    double div = pow(10, -precision);
    return (int)ceil(x / div) * div;
}

int CCUtils::utf8_to_utf16(int utf8) {
    char16_t u16;
    size_t len = ((utf8 & 0xff000000) == 0) ? 3 : 4;
    int be = htobe32(utf8);
    uint8_t* p = (uint8_t*)&be;
    if(len < 4) {
        p++;
    }
    ::utf8_to_utf16_no_null_terminator(p, len, &u16);
    return u16;
}

int CCUtils::getUTF8Bytes(unsigned char c) {
	int count = 1;
	if(c >= 128) {
		c <<= 1;
		do {
			count++;
			c <<= 1;
		} while(c > 128);
	}
    
	return count;
}

uint32_t CCUtils::hash(const string& s, uint32_t initial) {
    return hashlittle(s.c_str(), s.length(), initial);
}

uint64_t CCUtils::hash2(const string& s, uint32_t initial) {
    uint32_t pc, pb;
    hashlittle2(s.c_str(), s.length(), &pc, &pb);
    uint64_t t = pc;
    t <<= 32;
    t |= pb;
    return t;
}

int CCUtils::strlen8(const char* s) {
	if(!s)
		return 0;
    
	int count = 0;
	char* p = (char*)s;
	while(*p) {
		count++;
		int b = getUTF8Bytes(*p);
		p += b;
	}
    
	return count;
}

CCSize CCUtils::measureString(const char* pText,
                              const char* pFontName,
                              int nSize,
                              int maxWidth,
                              float shadowOffsetX,
                              float shadowOffsetY,
                              float strokeSize,
                              float lineSpacing,
                              float globalImageScaleFactor) {
    return CCImage::measureString(pText,
                                  pFontName,
                                  nSize,
                                  maxWidth,
                                  shadowOffsetX,
                                  shadowOffsetY,
                                  strokeSize,
                                  lineSpacing,
                                  globalImageScaleFactor);
}

string CCUtils::makeScreenshot(CCNode* root, const string& path, bool needStencil) {
	// check extension
	string ext = getPathExtension(path);
	bool png = ext == ".png";
	bool jpg = ext == ".jpg" || ext == ".jpeg";
	if(!png && !jpg)
		jpg = true;
	
	// get render root
	CCNode* renderNode = root;
	CCRect bound;
	if(renderNode)
		bound = getBoundingBoxInWorldSpace(renderNode);
	else
		renderNode = CCDirector::sharedDirector()->getRunningScene();
	
	// render in texture
	CCSize winSize = CCDirector::sharedDirector()->getWinSize();
	CCRenderTexture* screen = CCRenderTexture::create(winSize.width,
													  winSize.height,
													  kCCTexture2DPixelFormat_RGBA8888,
													  needStencil ? GL_DEPTH24_STENCIL8 : 0);
	screen->begin();
	renderNode->visit();
	screen->end();
	
	// save buffer
	screen->saveToFile(path.c_str(), png ? kCCImageFormatPNG : kCCImageFormatJPEG);
	
	// full path
	return CCFileUtils::sharedFileUtils()->getWritablePath() + path;
}

string CCUtils::trim(const string& s) {
	// null checking
	if(s.empty())
		return s;
	
	// trim tail and head
	size_t start = 0;
	size_t end = s.length();
    while(end > 0 && isspace(s.at(end - 1)))
		end--;
    while(start < end && isspace(s.at(start)))
		start++;
	
	return s.substr(start, end - start);
}

string CCUtils::getExternalOrFullPath(const string& path) {
    if(CCFileUtils::sharedFileUtils()->isAbsolutePath(path)) {
        return path;
    } else {
        string ext = externalize(path);
        if(isPathExistent(ext)) {
            return ext;
        } else {
            return CCFileUtils::sharedFileUtils()->fullPathForFilename(path.c_str());
        }
    }
}

NS_CC_END
