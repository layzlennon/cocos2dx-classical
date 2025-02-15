cc = cc or {}

function cc.clampf(value, min_inclusive, max_inclusive)
    -- body
    local temp = 0
    if min_inclusive > max_inclusive then
        temp = min_inclusive
        min_inclusive =  max_inclusive
        max_inclusive = temp
    end

    if value < min_inclusive then
        return min_inclusive
    elseif value < max_inclusive then
        return value
    else
        return max_inclusive
    end
end

--Point
function cc.p(_x,_y)
    if nil == _y then
         return { x = _x.x, y = _x.y }
    else
         return { x = _x, y = _y }
    end
end

function cc.pAdd(pt1,pt2)
    return {x = pt1.x + pt2.x , y = pt1.y + pt2.y }
end

function cc.pSub(pt1,pt2)
    return {x = pt1.x - pt2.x , y = pt1.y - pt2.y }
end

function cc.pMul(pt1,factor)
    return { x = pt1.x * factor , y = pt1.y * factor }
end

function cc.pMidpoint(pt1,pt2)
    return { x = (pt1.x + pt2.x) / 2.0 , y = ( pt1.y + pt2.y) / 2.0 }
end

function cc.pForAngle(a)
    return { x = math.cos(a), y = math.sin(a) }
end

function cc.pGetLength(pt)
    return math.sqrt( pt.x * pt.x + pt.y * pt.y )
end

function cc.pNormalize(pt)
    local length = cc.pGetLength(pt)
    if 0 == length then
        return { x = 1.0,y = 0.0 }
    end

    return { x = pt.x / length, y = pt.y / length }
end

function cc.pCross(self,other)
    return self.x * other.y - self.y * other.x
end

function cc.pDot(self,other)
    return self.x * other.x + self.y * other.y
end

function cc.pToAngle(self)
    return math.atan2(self.y, self.x)
end

function cc.pGetAngle(self,other)
    local a2 = cc.pNormalize(self)
    local b2 = cc.pNormalize(other)
    local angle = math.atan2(cc.pCross(a2, b2), cc.pDot(a2, b2) )
    if math.abs(angle) < 1.192092896e-7 then
        return 0.0
    end

    return angle
end

function cc.pGetDistance(startP,endP)
    return cc.pGetLength(cc.pSub(startP,endP))
end

function cc.pIsLineIntersect(A, B, C, D, s, t)
    if ((A.x == B.x) and (A.y == B.y)) or ((C.x == D.x) and (C.y == D.y))then
        return false, s, t
    end

    local BAx = B.x - A.x
    local BAy = B.y - A.y
    local DCx = D.x - C.x
    local DCy = D.y - C.y
    local ACx = A.x - C.x
    local ACy = A.y - C.y

    local denom = DCy * BAx - DCx * BAy
    s = DCx * ACy - DCy * ACx
    t = BAx * ACy - BAy * ACx

    if (denom == 0) then
        if (s == 0 or t == 0) then
            return true, s , t
        end

        return false, s, t
    end

    s = s / denom
    t = t / denom

    return true,s,t
end

function cc.pPerp(pt)
    return { x = -pt.y, y = pt.x }
end

function cc.RPerp(pt)
    return { x = pt.y,  y = -pt.x }
end

function cc.pProject(pt1, pt2)
    return { x = pt2.x * (cc.pDot(pt1,pt2) / cc.pDot(pt2,pt2)) , y = pt2.y * (cc.pDot(pt1,pt2) / cc.pDot(pt2,pt2)) }
end

function cc.pRotate(pt1, pt2)
    return { x = pt1.x * pt2.x - pt1.y * pt2.y, y = pt1.x * pt2.y + pt1.y * pt2.x }
end

function cc.pUnrotate(pt1, pt2)
    return { x = pt1.x * pt2.x + pt1.y * pt2.y, pt1.y * pt2.x - pt1.x * pt2.y }
end

--Calculates the square length of pt
function cc.pLengthSQ(pt)
    return cc.pDot(pt,pt)
end

function cc.pLength(pt)
    return math.sqrt(cc.pDot(pt,pt))
end

--Calculates the square distance between pt1 and pt2
function cc.pDistanceSQ(pt1,pt2)
    return cc.pLengthSQ(cc.pSub(pt1,pt2))
end

function cc.pGetClampPoint(pt1,pt2,pt3)
    return { x = cc.clampf(pt1.x, pt2.x, pt3.x), y = cc.clampf(pt1.y, pt2.y, pt3.y) }
end

function cc.pFromSize(sz)
    return { x = sz.width, y = sz.height }
end

function cc.pLerp(pt1,pt2,alpha)
    return cc.pAdd(cc.pMul(pt1, 1.0 - alpha), cc.pMul(pt2,alpha) )
end

function cc.pFuzzyEqual(pt1,pt2,variance)
    if (pt1.x - variance <= pt2.x) and (pt2.x <= pt1.x + variance) and (pt1.y - variance <= pt2.y) and (pt2.y <= pt1.y + variance) then
        return true
    else
        return false
    end
end

function cc.pRotateByAngle(pt1, pt2, angle)
    return cc.pAdd(pt2, cc.pRotate( cc.pSub(pt1, pt2),cc.pForAngle(angle)))
end

function cc.pIsSegmentIntersect(pt1,pt2,pt3,pt4)
    local s,t,ret = 0,0,false
    ret,s,t =cc.pIsLineIntersect(pt1, pt2, pt3, pt4,s,t)

    if ret and  s >= 0.0 and s <= 1.0 and t >= 0.0 and t <= 0.0 then
        return true;
    end

    return false
end

function cc.pGetIntersectPoint(pt1,pt2,pt3,pt4)
    local s,t, ret = 0,0,false
    ret,s,t = cc.pIsLineIntersect(pt1,pt2,pt3,pt4,s,t)
    if ret then
        return cc.p(pt1.x + s * (pt2.x - pt1.x), pt1.y + s * (pt2.y - pt1.y))
    else
        return cc.p(0,0)
    end
end

--Size
function cc.size( _width,_height )
    return { width = _width, height = _height }
end

function cc.sizeSub(s1, s2)
    return { width = s1.width - s2.width, height = s1.height - s2.height }
end

function cc.sizeMul(s1, factor)
    return cc.size(s1.width * factor, s1.height * factor)
end

-- Rect
function cc.rect(_x,_y,_width,_height)
    return { x = _x, y = _y, width = _width, height = _height }
end

function cc.rectEqualToRect(rect1,rect2)
    if rect1.x == rect2.x and rect1.y == rect2.y and rect1.width == rect2.width and rect1.height == rect2.height then
        return true
    end

    return false
end

function cc.rectGetMaxX(rect)
    return rect.x + rect.width
end

function cc.rectGetMidX(rect)
    return rect.x + rect.width / 2.0
end

function cc.rectGetMinX(rect)
    return rect.x
end

function cc.rectGetMaxY(rect)
    return rect.y + rect.height
end

function cc.rectGetMidY(rect)
    return rect.y + rect.height / 2.0
end

function cc.rectGetMinY(rect)
    return rect.y
end

function cc.rectContainsPoint( rect, point )
    local ret = false

    if (point.x >= rect.x) and (point.x <= rect.x + rect.width) and
       (point.y >= rect.y) and (point.y <= rect.y + rect.height) then
        ret = true
    end

    return ret
end

function cc.rectIntersectsRect( rect1, rect2 )
    local intersect = not ( rect1.x > rect2.x + rect2.width or
                    rect1.x + rect1.width < rect2.x         or
                    rect1.y > rect2.y + rect2.height        or
                    rect1.y + rect1.height < rect2.y )

    return intersect
end

function cc.rectUnion( rect1, rect2 )
    local rect = cc.rect(0, 0, 0, 0)
    rect.x = math.min(rect1.x, rect2.x)
    rect.y = math.min(rect1.y, rect2.y)
    rect.width = math.max(rect1.x + rect1.width, rect2.x + rect2.width) - rect.x
    rect.height = math.max(rect1.y + rect1.height, rect2.y + rect2.height) - rect.y
    return rect
end

function cc.rectIntersection( rect1, rect2 )
    local intersection = cc.rect(
        math.max(rect1.x, rect2.x),
        math.max(rect1.y, rect2.y),
        0, 0)

    intersection.width = math.min(rect1.x + rect1.width, rect2.x + rect2.width) - intersection.x
    intersection.height = math.min(rect1.y + rect1.height, rect2.y + rect2.height) - intersection.y
    return intersection
end

--Color3B
function cc.c3b( _r,_g,_b )
    return { r = _r, g = _g, b = _b }
end

function cc.i2c3b(num)
    local r = bit.band(bit.rshift(num, 16), 0xff)
    local g = bit.band(bit.rshift(num, 8), 0xff)
    local b = bit.band(num, 0xff)
    return cc.c3b(r, g, b)
end

function cc.i2c4b(num)
    local a = bit.band(bit.rshift(num, 24), 0xff)
    local r = bit.band(bit.rshift(num, 16), 0xff)
    local g = bit.band(bit.rshift(num, 8), 0xff)
    local b = bit.band(num, 0xff)
    return cc.c4b(r, g, b, a)
end

--Color4B
function cc.c4b( _r,_g,_b,_a )
    return { r = _r, g = _g, b = _b, a = _a }
end

--Color4F
function cc.c4f( _r,_g,_b,_a )
    return { r = _r, g = _g, b = _b, a = _a }
end

--Vertex2F
function cc.v2f(_x,_y)
    return { x = _x, y = _y }
end

--Vertex3F
function cc.v3f(_x,_y,_z)
    return { x = _x, y = _y, z = _z }
end

--Tex2F
function cc.t2f(_u,_v)
    return { u = _u, v = _v }
end

--PointSprite
function cc.PointSprite(_pos,_color,_size)
    return { pos = _pos, color = _color, size = _size }
end

--Quad2
function cc.q2(_tl,_tr,_bl,_br)
    return { tl = _tl, tr = _tr, bl = _bl, br = _br }
end

--Quad3
function cc.q3(_tl, _tr, _bl, _br)
    return { tl = _tl, tr = _tr, bl = _bl, br = _br }
end

--V2F_C4B_T2F
function cc.V2F_C4B_T2F(_vertices, _colors, _texCoords)
    return { vertices = _vertices, colors = _colors, texCoords = _texCoords }
end

--V2F_C4F_T2F
function cc.V2F_C4F_T2F(_vertices, _colors, _texCoords)
    return { vertices = _vertices, colors = _colors, texCoords = _texCoords }
end

--V3F_C4B_T2F
function cc.V3F_C4B_T2F(_vertices, _colors, _texCoords)
    return { vertices = _vertices, colors = _colors, texCoords = _texCoords }
end

--V2F_C4B_T2F_Quad
function cc.V2F_C4B_T2F_Quad(_bl, _br, _tl, _tr)
    return { bl = _bl, br = _br, tl = _tl, tr = _tr }
end

--V3F_C4B_T2F_Quad
function cc.V3F_C4B_T2F_Quad(_tl, _bl, _tr, _br)
    return { tl = _tl, bl = _bl, tr = _tr, br = _br }
end

--V2F_C4F_T2F_Quad
function cc.V2F_C4F_T2F_Quad(_bl, _br, _tl, _tr)
    return { bl = _bl, br = _br, tl = _tl, tr = _tr }
end

--T2F_Quad
function cc.T2F_Quad(_bl, _br, _tl, _tr)
    return { bl = _bl, br = _br, tl = _tl, tr = _tr }
end

-- insets
function cc.insets(l, t, r, b)
    return { left = l, top = t, right = r, bottom = b }
end

function cc.vec3(_x, _y, _z)
    return { x = _x, y = _y, z = _z }
end

function cc.vec4(_x, _y, _z, _w)
    return { x = _x, y = _y, z = _z, w = _w }
end

function cc.vec3normalize(vec3)
    local n = vec3.x * vec3.x + vec3.y * vec3.y + vec3.z * vec3.z
    if n == 1.0 then
        return vec3
    end

    n = math.sqrt(n)

    if n < 2e-37 then
        return vec3
    end

    n = 1.0 / n
    return {x = vec3.x * n, y = vec3.y * n, z = vec3.z * n}
end

function cc.safeRelease(obj)
    if obj ~= nil then
        obj:release()
    end
end

function cc.safeRetain(obj)
    if obj ~= nil then
        obj:retain()
    end
end

function cc.safeAutoRelease(obj)
    if obj ~= nil then
        obj:autorelease()
    end
end

function cc.simulateClick(n)
    if tolua.isa(n, "Widget") then
        n:pushDownEvent()
        n:releaseUpEvent()
    elseif tolua.isa(n, "CCMenuItem") then
        local pos = n:convertToWorldSpace(cc.p(n:getContentSize().width / 2,
        n:getContentSize().height / 2))
        local touch = CCTouch:create()
        touch:setTouchInfo(0, pos.x, display.winSize.height - pos.y)
        local menu = n:getParent()
        menu:ccTouchBegan(touch, nil)
        menu:ccTouchEnded(touch, nil)
    elseif tolua.isa(n, "CCTableViewCell") then
        local pos = n:convertToWorldSpace(cc.p(n:getContentSize().width / 2,
        n:getContentSize().height / 2));
        local touch = CCTouch:create()
        touch:setTouchInfo(0, pos.x, display.winSize.height - pos.y)
        local table = n:getParent():getParent()
        table:ccTouchBegan(touch, nil)
        table:ccTouchEnded(touch, nil)
    end
end