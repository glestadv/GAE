// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_WIDGETS_WIDGET_BASE_INCLUDED_
#define _GLEST_WIDGETS_WIDGET_BASE_INCLUDED_

#include <string>
#include <vector>
#include <string>

#include "math_util.h"
#include "font.h"
#include "input.h"
#include "texture.h"
#include "text_renderer.h"
#include "sigslot.h"
#include "logger.h"

using namespace Shared::Math;
using namespace Shared::Graphics;
using Glest::Util::Logger;

namespace Glest { namespace Widgets {

class Widget;
class Container;
class WidgetWindow;

using std::string;

#ifndef WIDGET_LOGGING
#	define WIDGET_LOGGING 0
#endif
#if WIDGET_LOGGING
#	define WIDGET_LOG(x) STREAM_LOG(x)
#else
#	define WIDGET_LOG(x)
#endif

#define ASSERT_RANGE(var, size)	assert(var >= 0 && var < size)

// =====================================================
// enum BackgroundStyle
// =====================================================

WRAPPED_ENUM( BackgroundStyle,
	NONE,
	SOLID_COLOUR,
	ALPHA_COLOUR,
	CUSTOM_4_COLOUR,
	CUSTOM_5_COLOUR
);

// =====================================================
// enum BorderStyle - for '3d' borders
// =====================================================

WRAPPED_ENUM( BorderStyle,
	NONE,		/**< Draw nothing */
	RAISE,		/**< Draw a raised widget */
	EMBED,		/**< Draw a lowered widget */
	SOLID,		/**< Draw a solid border */
	CUSTOM		/**< Use colour values from attached ColourValues4 */
	//EMBOSS,		/**< Draw a raised border */
	//ETCH		/**< Draw an etched (lowered) border */
);
/*
struct SideValues {
	int top, right, bottom, left;
};

struct ColourValues4 {
	Vec4f topLeft, topRight, bottomRight, bottomLeft;
};

struct ColourValues5 {
	Vec4f centre, topLeft, topRight, bottomRight, bottomLeft;
};

struct WidgetStyle {
	BorderStyle borderStyle;

	Vec3f borderColour;
	ColourValues4 *customBorderColours;
	
	SideValues borderSizes;
	SideValues paddingSizes;

	BackGroundStyle backgroundStyle;
	Vec3f backgroundColour;
	float backgroundAlpha;
	ColourValues4 *customBackgroundColours4;
	ColourValues5 *customBackgroundColours5;
};
*/

// =====================================================
// class Widget
// =====================================================

class Widget {
	friend class WidgetWindow;
public:
	typedef Widget* Ptr;

private:
	//int id;
	Container* parent;
	WidgetWindow* rootWindow;
	Vec2i	pos, 
			screenPos, 
			size;
	bool	visible;
	bool	enabled;
	float	fade;

	BorderStyle borderStyle;
	Vec3f borderColour;
	int borderSize;
	float bgAlpha;

	int padding;

protected:
	Widget(WidgetWindow* window);
	Widget() {} // dangerous perhaps, but needed to avoid virtual inheritence problems (double inits)

public:
	Widget(Container* parent);
	Widget(Container* parent, Vec2i pos, Vec2i size);
	virtual ~Widget();

	// de-virtualise ??
	// get/is
	virtual Vec2i getScreenPos() const { return screenPos; }
	virtual Container* getParent() const { return parent; }
	virtual WidgetWindow* getRootWindow() const { return rootWindow; }
	virtual Widget::Ptr getWidgetAt(const Vec2i &pos);

	virtual Vec2i getSize() const		{ return size;		 }
	virtual int   getWidth() const		{ return size.x;	 }
	virtual int   getHeight() const		{ return size.y;	 }
	virtual float getFade() const		{ return fade;		 }
	virtual int	  getBorderSize() const	{ return borderSize; }
	virtual int	  getPadding() const	{ return padding;	 }

	// layout helpers
	virtual Vec2i getPrefSize() const = 0; // may return (-1,-1) to indicate 'as big as possible'
	virtual Vec2i getMinSize() const = 0; // may not return (-1,-1)
	virtual Vec2i getMaxSize() const {return Vec2i(-1); } // return (-1,-1) to indicate 'no maximum size'

	virtual bool isVisible() const		{ return visible; }
	virtual bool isInside(const Vec2i &pos) const {
		return pos.southEastOf(screenPos) && pos.northWestOf(screenPos + size);
	}

	virtual bool isEnabled() const	{ return enabled;	}

	// set
	virtual void setEnabled(bool v) { enabled = v;	}
	virtual void setSize(const Vec2i &sz);
	virtual void setPos(const Vec2i &p);
	virtual void setSize(const int x, const int y) { setSize(Vec2i(x,y)); }
	virtual void setPos(const int x, const int y) { setPos(Vec2i(x,y)); }
	virtual void setVisible(bool vis) { visible = vis; }
	virtual void setFade(float v) { fade = v; }
	virtual void setParent(Container* p) { parent = p; }

	void setBorderSize(int sz) { borderSize = sz; }
	void setBorderStyle(BorderStyle style) { borderStyle = style; }
	void setBgAlphaValue(float v) { bgAlpha = v; }
	void setBorderColour(Vec3f colour) { borderColour = colour; }
	void setBorderParams(BorderStyle st, int sz, Vec3f col, float alp);
	void setPadding(int pad) { padding = pad; }

	virtual bool mouseDown(MouseButton btn, Vec2i pos);
	virtual bool mouseUp(MouseButton btn, Vec2i pos);
	virtual bool mouseMove(Vec2i pos);
	virtual bool mouseDoubleClick(MouseButton btn, Vec2i pos);
	virtual bool mouseWheel(Vec2i pos, int z);
	virtual void mouseIn();
	virtual void mouseOut();

	virtual bool keyDown(Key key);
	virtual bool keyUp(Key key);
	virtual bool keyPress(char c);

	virtual void update() {} // must 'register' with WidgetWindow to receive

	virtual void lostKeyboardFocus() {}

	virtual void render() = 0;

	void renderBorders(BorderStyle style, const Vec2i &offset, const Vec2i &size, int borderSize);
	void renderBgAndBorders();
	void renderHighLight(Vec3f colour, float centreAlpha, float borderAlpha, Vec2i offset, Vec2i size);
	void renderHighLight(Vec3f colour, float centreAlpha, float borderAlpha);
	
	virtual string descPosDim();
	virtual string desc() = 0;

	sigslot::signal<Widget::Ptr> Destroyed;

};

class MouseWidget {
	friend class WidgetWindow;
public:
	typedef MouseWidget* Ptr;

private:
	Widget::Ptr me;

public:
	MouseWidget(Widget::Ptr widget);
	~MouseWidget();

private:
	virtual bool EW_mouseDown(MouseButton btn, Vec2i pos)			{ return false; }
	virtual bool EW_mouseUp(MouseButton btn, Vec2i pos)			{ return false; }
	virtual bool EW_mouseMove(Vec2i pos)							{ return false; }
	virtual bool EW_mouseDoubleClick(MouseButton btn, Vec2i pos)	{ return false; }
	virtual bool EW_mouseWheel(Vec2i pos, int z)					{ return false; }

	virtual void EW_mouseIn() {}
	virtual void EW_mouseOut() {}
};

class KeyboardWidget {
	friend class WidgetWindow;
public:
	typedef KeyboardWidget* Ptr;

private:
	Widget::Ptr me;

public:
	KeyboardWidget(Widget::Ptr widget);
	~KeyboardWidget();

private:
	virtual bool EW_keyDown(Key key)	{ return false; }
	virtual bool EW_keyUp(Key key)		{ return false; }
	virtual bool EW_keyPress(char c)	{ return false; }

	virtual void EW_lostKeyboardFocus() {}
};

struct ImageRenderInfo {
	bool hasOffset, hasCustomSize;
	Vec2i offset, size;

	ImageRenderInfo() : hasOffset(false), hasCustomSize(false) {}

	ImageRenderInfo(bool hasOffset, Vec2i offset, bool hasCustomSize, Vec2i size)
		: hasOffset(hasOffset), hasCustomSize(hasCustomSize), offset(offset), size(size) {}
};

// =====================================================
// class ImageWidget
// =====================================================

class ImageWidget /*: public virtual Widget */{
private:
 	typedef vector<Texture2D*> Textures;

	Widget::Ptr me;
	Textures textures;
	vector<ImageRenderInfo> imageInfo;

protected:
	void renderImage(int ndx = 0);

public:
	ImageWidget(Widget::Ptr me);
	ImageWidget(Widget::Ptr me, Texture2D *tex);

	int addImage(Texture2D *tex);
	void setImage(Texture2D *tex, int ndx = 0);
	int addImageX(Texture2D *tex, Vec2i offset, Vec2i sz);
	void setImageX(Texture2D *tex, int ndx, Vec2i offset, Vec2i sz);

	const Texture2D* getImage(int ndx=0) const {
		ASSERT_RANGE(ndx, textures.size());
		return textures[ndx];
	}
};

// =====================================================
// class TextWidget
// =====================================================

class TextWidget /*: public virtual Widget */{
private:
	Widget::Ptr me;
	vector<string> texts;
	Vec4f txtColour;
	Vec4f txtShadowColour;
	Vec2i txtPos;
	const Font *font;
	bool isFreeTypeFont;
	bool centre;

	void renderText(const string &txt, int x, int y, const Vec4f &colour, const Font *font = 0);

protected:
	void renderText(int ndx = 0);
	void renderTextShadowed(int ndx = 0);

public:
	TextWidget(Widget::Ptr me);

	// set
	void setCentre(bool val)	{ centre = val; }
	void setTextParams(const string&, const Vec4f, const Font*, bool ft=false, bool cntr=true);
	int addText(const string &txt);
	void setText(const string &txt, int ndx = 0);
	void setTextColour(const Vec4f &col) { txtColour = col;	 }
	void setTextPos(const Vec2i &pos);
	void setTextFont(const Font *f);

	void centreText(int ndx = 0);
	//void setWidgetSize(const Vec2i &sz);
	//void setWidgetPos(const Vec2i &p);

	// get
	const string& getText(int ndx=0) const	{ return texts[ndx];	}
	const Vec4f& getTextColour() const	 { return txtColour; }
	const Vec2i& getTextPos() const	  { return txtPos; }
	const Font* getTextFont() const { return font; }
	Vec2i getTextDimensions() const;
	bool hasText() const { return !texts.empty(); }
};

// =====================================================
// class Container
// =====================================================

class Container : public /*virtual*/ Widget {
public:
	typedef Container* Ptr;
	typedef vector<Widget::Ptr> WidgetList;

protected:
	WidgetList children;

public:
	Container(Container::Ptr parent);
	Container(Container::Ptr parent, Vec2i pos, Vec2i size);
	Container(WidgetWindow* window);
	virtual ~Container();

	virtual Widget::Ptr getWidgetAt(const Vec2i &pos);

	virtual void addChild(Widget::Ptr child);
	virtual void remChild(Widget::Ptr child);
	virtual void clear();
	virtual void setEnabled(bool v);
	virtual void setFade(float v);
	virtual void render();
};

// =====================================================
// class Layer
// =====================================================

class Layer : public Container {
public:
	typedef Layer* Ptr;

private:
	const string name;
	const int id;

public:
	Layer(WidgetWindow *window, const string &name, int id)
			: Container(window)
			, name(name), id(id) {
		WIDGET_LOG( __FUNCTION__ << endl );
	}

	~Layer() {
		WIDGET_LOG( __FUNCTION__ << endl );
	}

	int getId() const { return id; }
	const string& getName() const { return name; }

	virtual Vec2i getPrefSize() const { return Vec2i(-1); }
	virtual Vec2i getMinSize() const { return Vec2i(-1); }

	virtual string desc() { return string("[Layer '") + name + "':" + descPosDim() + "]"; }
};

}}

#endif
