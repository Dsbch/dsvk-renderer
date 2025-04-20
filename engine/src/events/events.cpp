#include <pch.h>
#include "events.h"

namespace engine
{
	engine::key engine::fromRawMouse(const RAWINPUT* raw)
	{
		if (!raw)
			return unknown;

		USHORT flags = raw->data.mouse.usButtonFlags;

		if (flags & RI_MOUSE_LEFT_BUTTON_DOWN || flags & RI_MOUSE_LEFT_BUTTON_UP)
			return mouse1;

		if (flags & RI_MOUSE_RIGHT_BUTTON_DOWN || flags & RI_MOUSE_RIGHT_BUTTON_UP)
			return mouse2;

		if (flags & RI_MOUSE_MIDDLE_BUTTON_DOWN || flags & RI_MOUSE_MIDDLE_BUTTON_UP)
			return mouse3;

		return unknown;
	}

	engine::key engine::fromRawKeyboard(const RAWINPUT* raw)
	{
		if (!raw)
			return unknown;

		const RAWKEYBOARD& kbd = raw->data.keyboard;

		// You can use kbd.VKey, but note that for certain keys like shift/ctrl/alt,
		// you may want to check kbd.MakeCode + Flags for left/right distinction.
		switch (kbd.VKey)
		{
		case VK_ESCAPE:  return escape;
		case VK_RETURN:  return enter;
		case VK_SPACE:   return space;
		case VK_LEFT:    return left;
		case VK_RIGHT:   return right;
		case VK_UP:      return up;
		case VK_DOWN:    return down;

		case 'A': return a;
		case 'B': return b;
		case 'C': return c;
		case 'D': return d;
		case 'E': return e;
		case 'F': return f;
		case 'G': return g;
		case 'H': return h;
		case 'I': return i;
		case 'J': return j;
		case 'K': return k;
		case 'L': return l;
		case 'M': return m;
		case 'N': return n;
		case 'O': return o;
		case 'P': return p;
		case 'Q': return q;
		case 'R': return r;
		case 'S': return s;
		case 'T': return t;
		case 'U': return u;
		case 'V': return v;
		case 'W': return w;
		case 'X': return x;
		case 'Y': return y;
		case 'Z': return z;

		case '0': return zero;
		case '1': return one;
		case '2': return two;
		case '3': return three;
		case '4': return four;
		case '5': return five;
		case '6': return six;
		case '7': return seven;
		case '8': return eight;
		case '9': return nine;

		default:
			return unknown;
		}
	}

	engine::key engine::fromWinApiMouse(int msg)
	{
		switch (msg)
		{
		case WM_LBUTTONUP: return mouse1;
		case WM_RBUTTONUP: return mouse2;
		case WM_MBUTTONUP: return mouse3;
		case WM_LBUTTONDOWN: return mouse1;
		case WM_RBUTTONDOWN: return mouse2;
		case WM_MBUTTONDOWN: return mouse3;
		default: return unknown;
		}
	}

	engine::key engine::fromWinApiKey(int vkCode)
	{
		switch (vkCode) {
		case VK_ESCAPE:  return escape;
		case VK_RETURN:  return enter;
		case VK_SPACE:   return space;
		case VK_LEFT:    return left;
		case VK_RIGHT:   return right;
		case VK_UP:      return up;
		case VK_DOWN:    return down;

		case 'A': return a;
		case 'B': return b;
		case 'C': return c;
		case 'D': return d;
		case 'E': return e;
		case 'F': return f;
		case 'G': return g;
		case 'H': return h;
		case 'I': return i;
		case 'J': return j;
		case 'K': return k;
		case 'L': return l;
		case 'M': return m;
		case 'N': return n;
		case 'O': return o;
		case 'P': return p;
		case 'Q': return q;
		case 'R': return r;
		case 'S': return s;
		case 'T': return t;
		case 'U': return u;
		case 'V': return v;
		case 'W': return w;
		case 'X': return x;
		case 'Y': return y;
		case 'Z': return z;

		case '0': return zero;
		case '1': return one;
		case '2': return two;
		case '3': return three;
		case '4': return four;
		case '5': return five;
		case '6': return six;
		case '7': return seven;
		case '8': return eight;
		case '9': return nine;

		default: return unknown;
		}
	}
}