// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#include "pch.h"
#include "widgets.h"

#include <sstream>

#include "metrics.h"
#include "util.h"
#include "renderer.h"
#include "texture_gl.h"
#include "game_constants.h"
#include "core_data.h"

#include "widget_window.h"

#include "leak_dumper.h"

using Shared::Util::deleteValues;
using Shared::Graphics::Texture;
using namespace Shared::Graphics::Gl;
using Glest::Graphics::Renderer;
using namespace Glest::Global;

namespace Glest { namespace Widgets {

// =====================================================
//  class StaticText
// =====================================================

StaticText::StaticText(Container* parent)
		: Widget(parent) , TextWidget(this)
		, m_shadow(false), m_doubleShadow(false), m_shadowOffset(2) {
	m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::STATIC_WIDGET);
	m_backgroundStyle = g_widgetConfig.getBackgroundStyle(WidgetType::STATIC_WIDGET);
}

StaticText::StaticText(Container* parent, Vec2i pos, Vec2i size)
		: Widget(parent, pos, size), TextWidget(this)
		, m_shadow(false), m_doubleShadow(false), m_shadowOffset(2) {
	m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::STATIC_WIDGET);
	m_backgroundStyle = g_widgetConfig.getBackgroundStyle(WidgetType::STATIC_WIDGET);
}

void StaticText::render() {
	if (!isVisible()) {
		return;
	}
	Widget::renderBgAndBorders();
	if (TextWidget::getText(0) != "") {
		if (m_doubleShadow) {
			TextWidget::renderTextDoubleShadowed(0, m_shadowOffset);
		} else if (m_shadow) {
			TextWidget::renderTextShadowed(0, m_shadowOffset);
		} else {
			TextWidget::renderText();
		}
	}
}

Vec2i StaticText::getMinSize() const {
	Vec2i txtDim = getTextDimensions();
	Vec2i xtra = getBordersAll() + Vec2i(getPadding());
	return txtDim + xtra;
}

Vec2i StaticText::getPrefSize() const {
	return getMinSize();
}

void StaticText::setShadow(const Vec4f &colour, int offset) {
	m_shadow = true;
	TextWidget::setTextShadowColour(colour);
	m_shadowOffset = offset;
}

void StaticText::setDoubleShadow(const Vec4f &colour1, const Vec4f &colour2, int offset) {
	m_doubleShadow = true;
	TextWidget::setTextShadowColours(colour1, colour2);
	m_shadowOffset = offset;
}

// =====================================================
//  class StaticImage
// =====================================================

StaticImage::StaticImage(Container* parent)
		: Widget(parent)
		, ImageWidget(this) {
	m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::STATIC_WIDGET);
	m_backgroundStyle = g_widgetConfig.getBackgroundStyle(WidgetType::STATIC_WIDGET);
}

StaticImage::StaticImage(Container* parent, Vec2i pos, Vec2i size) 
		: Widget(parent, pos, size)
		, ImageWidget(this) {
	m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::STATIC_WIDGET);
	m_backgroundStyle = g_widgetConfig.getBackgroundStyle(WidgetType::STATIC_WIDGET);
}

StaticImage::StaticImage(Container* parent, Vec2i pos, Vec2i size, Texture2D *tex)
		: Widget(parent, pos, size)
		, ImageWidget(this, tex) {
	m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::STATIC_WIDGET);
	m_backgroundStyle = g_widgetConfig.getBackgroundStyle(WidgetType::STATIC_WIDGET);
}

Vec2i StaticImage::getMinSize() const {
	const Pixmap2D *pixmap = getImage()->getPixmap();
	Vec2i imgDim = Vec2i(pixmap->getW(), pixmap->getH());
	Vec2i xtra = getBordersAll() + Vec2i(getPadding());
	return imgDim + xtra;
}

Vec2i StaticImage::getPrefSize() const {
	return getMinSize();
}

// =====================================================
//  class Imageset
// =====================================================

/** Add images with uniform dimensions
  * @param source the source texture
  * @param width the width of a single image
  * @param height the height of a single image
  */
void Imageset::addImages(const Texture2D *source, int width, int height) {
	assert(source != NULL && width > 0 && height > 0);
	///@todo check the dimensions to make sure there will be enough image for all the images
	/// current behaviour cuts some off if not exact? (untested) - hailstone 30Dec2010
	/// maybe check dimensions are power of 2 and then check that maches source dimensions

	const Pixmap2D *sourcePixmap = source->getPixmap();

	// extract a single image from the source pixmap and add the texture to the list.
	// (0,0) is at bottom left but is setup so ordering goes from top left to the right
	// and wraps around to bottom right. Think of it as each row is appended to the previous.
	for (int j = sourcePixmap->getH() - height; j >= 0; j = j - height) {
		for (int i = 0; i < sourcePixmap->getW(); i = i + width) {
			Texture2D *tex = g_renderer.newTexture2D(Graphics::ResourceScope::GLOBAL);
			tex->getPixmap()->copy(i, j, width, height, sourcePixmap); //may throw, should this be handled here? - hailstone 30Dec2010
			tex->setMipmap(false);
			tex->init(Texture::fBilinear);
			addImage(tex);
		}
	}

	assert(hasImage());
}

/// @param ndx must be valid to change default from 0
void Imageset::setDefaultImage(int ndx) {
	if (hasImage(ndx)) {
		m_defaultImage = ndx;
	}
}

/// @param ndx must be valid to change
void Imageset::setActive(int ndx) {
	if (hasImage(ndx)) {
		m_active = ndx;
	}
}

// =====================================================
//	class Animset
// =====================================================

void Animset::update() {
	///@todo timing needs to be fixed - hailstone 2Jan2011
	if (isEnabled()) {
		m_timeElapsed++;
	}

	if (m_timeElapsed > 60) {
		m_timeElapsed = 0;
		++m_currentFrame;
		if (m_currentFrame > m_end) {
			if (m_loop) {
				reset();
			} else {
				stop();
			}
		}
		m_imageset->setActive(m_currentFrame);
	}
}

// =====================================================
//  class Button
// =====================================================

Button::Button(Container* parent)
		: Widget(parent)
		, TextWidget(this)
		, MouseWidget(this)
		, m_hover(false)
		, m_pressed(false)
		, m_doHoverHighlight(true) {
	m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::BUTTON);
	m_backgroundStyle = g_widgetConfig.getBackgroundStyle(WidgetType::BUTTON);
}

Button::Button(Container* parent, Vec2i pos, Vec2i size, bool hoverHighlight)
		: Widget(parent, pos, size)
		, TextWidget(this)
		, MouseWidget(this)
		, m_hover(false)
		, m_pressed(false)
		, m_doHoverHighlight(hoverHighlight) {
	m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::BUTTON);
	m_backgroundStyle = g_widgetConfig.getBackgroundStyle(WidgetType::BUTTON);
}

Vec2i Button::getPrefSize() const {
	Vec2i imgSize(0);
	if (m_borderStyle.m_type == BorderType::TEXTURE) {
		const Texture2D *tex = g_widgetConfig.getTexture(m_borderStyle.m_imageNdx);
		imgSize = Vec2i(tex->getPixmap()->getW(), tex->getPixmap()->getH());
	}
	Vec2i txtSize(getMinSize());
	Vec2i res(std::max(imgSize.x, txtSize.x), std::max(imgSize.y, txtSize.y));
	return res;
}

Vec2i Button::getMinSize() const {
	Vec2i res = TextWidget::getTextDimensions() + getBordersAll() + Vec2i(getPadding());
	res.x = std::min(res.x, 16);
	res.y = std::min(res.y, 16);
	return res;
}

void Button::setSize(const Vec2i &sz) {
	Widget::setSize(sz);
}

bool Button::mouseMove(Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << pos << " )");
	if (isEnabled()) {
		if (m_hover && !isInside(pos)) {
			mouseOut();
		}
		if (!m_hover && isInside(pos)) {
			mouseIn();
		}
	}
	return true;
}

bool Button::mouseDown(MouseButton btn, Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << MouseButtonNames[btn] << ", " << pos << " )");
	if (isEnabled() && btn == MouseButton::LEFT) {
		m_pressed = true;
	}
	return true;
}

bool Button::mouseUp(MouseButton btn, Vec2i pos) {
	WIDGET_LOG( __FUNCTION__ << "( " << MouseButtonNames[btn] << ", " << pos << " )");
	if (isEnabled() && btn == MouseButton::LEFT) {
		if (m_pressed && m_hover) {
			Clicked(this);
		}
		m_pressed = false;
	}
	return true;
}

void Button::render() {
	// render background & borders
	renderBgAndBorders();

	// render hilight
	if (m_doHoverHighlight && m_hover && isEnabled()) {
		float anim = getRootWindow()->getAnim();
		if (anim > 0.5f) {
			anim = 1.f - anim;
		}
		float borderAlpha = 0.1f + anim * 0.5f;
		float centreAlpha = 0.3f + anim;
		Widget::renderHighLight(Vec3f(1.f), centreAlpha, borderAlpha);
	}

	// render label
	if (hasText()) {
		TextWidget::renderText();
	}
}

// =====================================================
//  class CheckBox
// =====================================================

CheckBox::CheckBox(Container* parent)
		: Button(parent), m_checked(false)
		, ImageWidget(this) {
	m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::CHECK_BOX);
	m_backgroundStyle = g_widgetConfig.getBackgroundStyle(WidgetType::CHECK_BOX);
	CoreData &coreData = CoreData::getInstance();
	addImageX(coreData.getCheckBoxCrossTexture(), Vec2i(0), Vec2i(32));
	addImageX(coreData.getCheckBoxTickTexture(), Vec2i(0), Vec2i(32));
	setTextParams(g_lang.get("No"), Vec4f(1.f), coreData.getFTMenuFontNormal(), false);
	addText(g_lang.get("Yes"));
	setSize(getPrefSize());
}

CheckBox::CheckBox(Container* parent, Vec2i pos, Vec2i size)
		: Button(parent, pos, size, false), m_checked(false)
		, ImageWidget(this) {
	m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::CHECK_BOX);
	m_backgroundStyle = g_widgetConfig.getBackgroundStyle(WidgetType::CHECK_BOX);
	CoreData &coreData = CoreData::getInstance();
	addImageX(coreData.getCheckBoxCrossTexture(), Vec2i(0), Vec2i(32));
	addImageX(coreData.getCheckBoxTickTexture(), Vec2i(0), Vec2i(32));
	setTextParams(g_lang.get("No"), Vec4f(1.f), coreData.getFTMenuFontNormal(), false);
	addText(g_lang.get("Yes"));
	int y = int((size.y - getTextFont()->getMetrics()->getHeight()) / 2);
	setTextPos(Vec2i(40, y), 0);
	setTextPos(Vec2i(40, y), 1);
}

void CheckBox::setSize(const Vec2i &sz) {
	// bypass Button::setSize()
	Widget::setSize(sz);
	int y = int((sz.y - getTextFont()->getMetrics()->getHeight()) / 2);
	setTextPos(Vec2i(40, y), 0);
	setTextPos(Vec2i(40, y), 1);
}

Vec2i CheckBox::getMinSize() const {
	Vec2i txtDim = getTextDimensions();
	Vec2i xtra(
		m_borderStyle.m_sizes[Border::LEFT] + m_borderStyle.m_sizes[Border::RIGHT],
		m_borderStyle.m_sizes[Border::TOP] + m_borderStyle.m_sizes[Border::BOTTOM]
	);
	xtra += Vec2i(getPadding());
	Vec2i res = txtDim + xtra + Vec2i(txtDim.y + 2, 0);
	return res;
}

Vec2i CheckBox::getPrefSize() const {
	Vec2i dim = getTextDimensions();
	dim.x += 40;
	if (dim.y < 32) {
		dim.y = 32;
	}
	Vec2i xtra(
		m_borderStyle.m_sizes[Border::LEFT] + m_borderStyle.m_sizes[Border::RIGHT],
		m_borderStyle.m_sizes[Border::TOP] + m_borderStyle.m_sizes[Border::BOTTOM]
	);
	xtra += Vec2i(getPadding());
	return dim + xtra;
}

bool CheckBox::mouseDown(MouseButton btn, Vec2i pos) {
	if (btn == MouseButton::LEFT) {
		m_pressed = true;
		return true;
	}
	return false;
}

bool CheckBox::mouseUp(MouseButton btn, Vec2i pos) {
	if (btn == MouseButton::LEFT) {
		if (m_pressed && m_hover) {
			m_checked = !m_checked;
			Clicked(this);
		}
		m_pressed = false;
		return true;
	}
	return false;
}

void CheckBox::render() {
	// render background
	ImageWidget::renderImage(m_checked ? 1 : 0);

	// render hilight
	if (m_hover) {
		float anim = getRootWindow()->getAnim();
		if (anim > 0.5f) {
			anim = 1.f - anim;
		}
		float borderAlpha = 0.1f + anim * 0.5f;
		float centreAlpha = 0.3f + anim;
		Vec2i offset(m_borderStyle.m_sizes[Border::LEFT], m_borderStyle.m_sizes[Border::BOTTOM]);
		offset += Vec2i(getPadding());
		Widget::renderHighLight(Vec3f(1.f), centreAlpha, borderAlpha, offset, Vec2i(32));
	}

	// render label
	TextWidget::renderText(m_checked ? 1 : 0);
}

// =====================================================
//  class TextBox
// =====================================================

TextBox::TextBox(Container* parent)
		: Widget(parent)
		, MouseWidget(this)
		, KeyboardWidget(this)
		, TextWidget(this)
		, hover(false)
		, focus(false)
		, changed(false) {
	m_normBorderStyle = m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::TEXT_BOX);
	m_focusBorderStyle = g_widgetConfig.getFocusBorderStyle(WidgetType::TEXT_BOX);
	m_backgroundStyle = g_widgetConfig.getBackgroundStyle(WidgetType::TEXT_BOX);
}

TextBox::TextBox(Container* parent, Vec2i pos, Vec2i size)
		: Widget(parent, pos, size)
		, MouseWidget(this)
		, KeyboardWidget(this)
		, TextWidget(this)
		, hover(false)
		, focus(false)
		, changed(false) {
	m_normBorderStyle = m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::TEXT_BOX);
	m_focusBorderStyle = g_widgetConfig.getFocusBorderStyle(WidgetType::TEXT_BOX);
	m_backgroundStyle = g_widgetConfig.getBackgroundStyle(WidgetType::TEXT_BOX);
}

void TextBox::gainFocus() {
	if (isEnabled()) {
		focus = true;
		getRootWindow()->aquireKeyboardFocus(this);
		m_borderStyle = m_focusBorderStyle;
	}
}

bool TextBox::mouseDown(MouseButton btn, Vec2i pos) {
	gainFocus();
	return true;
}

bool TextBox::mouseUp(MouseButton btn, Vec2i pos) {
	return true;
}

bool TextBox::keyDown(Key key) {
	KeyCode code = key.getCode();
	switch (code) {
		case KeyCode::BACK_SPACE: {
			const string &txt = getText();
			if (!txt.empty()) {
				setText(txt.substr(0, txt.size() - 1));
				TextChanged(this);
			}
			return true;
		}
		case KeyCode::RETURN:
			getRootWindow()->releaseKeyboardFocus(this);
			InputEntered(this);
			return true;
		case KeyCode::ESCAPE:
			//getRootWindow()->releaseKeyboardFocus(this);
			//return true;
			/*
		case KeyCode::DELETE_:
		case KeyCode::ARROW_LEFT:
		case KeyCode::ARROW_RIGHT:
		case KeyCode::HOME:
		case KeyCode::END:
		case KeyCode::TAB:
			cout << "KeyDown: [" << KeyCodeNames[code] << "]\n";
			return true;
			*/
		default:
			break;
	}
	return false;
}

bool TextBox::keyUp(Key key) {
	return false;
}

bool TextBox::keyPress(char c) {
	if (c >= 32 && c <= 126) { // 'space' -> 'tilde' [printable ascii char]
		if (!m_inputMask.empty()) {
			if (m_inputMask.find_first_of(c) == string::npos) {
				return true;
			}
		}
		string s(getText());
		setText(s + c);
		TextChanged(this);
		return true;
	}
	return false;
}

void TextBox::lostKeyboardFocus() {
	focus = false;
	m_borderStyle = m_normBorderStyle;
}

void TextBox::render() {
	Widget::renderBgAndBorders();
	TextWidget::renderText();
}

Vec2i TextBox::getMinSize() const {
	Vec2i txtDim = getTextDimensions();
	return Vec2i(200, txtDim.y) + m_borderStyle.getBorderDims();
}

Vec2i TextBox::getPrefSize() const {
	Vec2i dim = getTextDimensions();
	return dim + m_borderStyle.getBorderDims();
}

// =====================================================
//  class Slider
// =====================================================

Slider::Slider(Container* parent, Vec2i pos, Vec2i size, const string &title)
		: Widget(parent, pos, size)
		, MouseWidget(this)
		, ImageWidget(this)
		, TextWidget(this)
		, m_sliderValue(0.f)
		, m_thumbHover(false)
		, m_thumbPressed(false)
		, m_shaftHover(false)
		, m_title(title) {
	m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::SLIDER);
	m_backgroundStyle = g_widgetConfig.getBackgroundStyle(WidgetType::SLIDER);

	const CoreData &coreData = CoreData::getInstance();
	Font *font = coreData.getFTMenuFontNormal();
	addImage(coreData.getButtonSmallTexture());
	setTextParams(m_title, Vec4f(1.f), font, false); // ndx 0
	addText("0 %"); // ndx 1
	
	string maxVal = "100 %";
	Vec2f dims = getTextFont()->getMetrics()->getTextDiminsions(maxVal);
	m_valSize = int(dims.x + 5.f);
	m_shaftStyle.setRaise(WidgetColour::LIGHT_BORDER, WidgetColour::DARK_BORDER);
	m_shaftStyle.setSizes(2);

	recalc();
}

void Slider::recalc() {
	Vec2i size = getSize();	

	int space = size.x - m_valSize;
	m_shaftOffset = int(space * 0.35f);
	m_shaftSize = int(space * 0.60f);

	m_thumbCentre = m_shaftOffset + 5 + int(m_sliderValue * (m_shaftSize - 10));

	space = size.y - (m_borderStyle.m_sizes[Border::TOP] + m_borderStyle.m_sizes[Border::BOTTOM]);
	m_thumbPos = Vec2i(m_thumbCentre - space / 4 + m_borderStyle.m_sizes[Border::LEFT], 
						m_borderStyle.m_sizes[Border::BOTTOM]);
	m_thumbSize = Vec2i(space / 2, space);
	setImageX(0, 0, m_thumbPos, m_thumbSize);

	Vec2f dims = getTextFont()->getMetrics()->getTextDiminsions(m_title);
	m_titlePos = Vec2i(m_shaftOffset / 2 - int(dims.x / 2), size.y / 2 - int(dims.y / 2));

	string sliderString = Conversion::toStr(int(m_sliderValue * 100.f)) + " %";
	setText(sliderString, 1);
	dims = getTextFont()->getMetrics()->getTextDiminsions(sliderString);
	int valWidth = size.x - m_shaftOffset - m_shaftSize;
	m_valuePos = Vec2i(m_shaftOffset + m_shaftSize + (valWidth / 2 - int(dims.x / 2)), size.y / 2 - int(dims.y / 2));
	setTextPos(m_titlePos, 0);
	setTextPos(m_valuePos, 1);
}

bool Slider::mouseMove(Vec2i pos) {
	pos -= getScreenPos();
	if (m_thumbPressed) {
		int x_pos = clamp(pos.x - m_shaftOffset - 5, 0, m_shaftSize - 10);
		float oldVal = m_sliderValue;
		m_sliderValue = x_pos / float(m_shaftSize - 10);
		recalc();
		if (oldVal != m_sliderValue) {
			ValueChanged(this);
		}
		return true;
	}
	if (Vec2i::isInside(pos, m_thumbPos, m_thumbSize)) {
		m_thumbHover = true;
	} else {
		m_thumbHover = false;
	}
	if (!m_thumbHover) {
		if (pos.x >= m_shaftOffset + 3 && pos.x < m_shaftOffset + m_shaftSize - 3) {
			m_shaftHover = true;
		} else {
			m_shaftHover = false;
		}
	} else {
		m_shaftHover = true;
	}
	return true;
}

void Slider::mouseOut() {
	if (!m_thumbPressed) {
		m_shaftHover = m_thumbHover = false;
	}
}

bool Slider::mouseDown(MouseButton btn, Vec2i pos) {
	if (btn != MouseButton::LEFT) {
		return false;
	}
	pos -= getScreenPos();
	if (m_thumbHover || m_shaftHover) {
		m_thumbHover = true;
		m_shaftHover = false;
		m_thumbPressed = true;

		int x_pos = clamp(pos.x - m_shaftOffset - 5, 0, m_shaftSize - 10);
		float oldVal = m_sliderValue;
		m_sliderValue = x_pos / float(m_shaftSize - 10);
		recalc();
		if (oldVal != m_sliderValue) {
			ValueChanged(this);
		}
	}
	return true;
}

bool Slider::mouseUp(MouseButton btn, Vec2i pos) {
	if (btn != MouseButton::LEFT) {
		return false;
	}
	if (m_thumbPressed) {
		m_thumbPressed = false;
	}
	if (isInside(pos)) {
		pos -= getScreenPos();
		if (Vec2i::isInside(pos, m_thumbPos, m_thumbSize)) {
			m_thumbHover = true;
		} else {
			m_thumbHover = false;
		}
		if (!m_thumbHover) {
			if (pos.x >= m_shaftOffset + 3 && pos.x < m_shaftOffset + m_shaftSize - 3) {
				m_shaftHover = true;
			} else {
				m_shaftHover = false;
			}
		} else {
			m_shaftHover = false;
		}

	} else {
		m_shaftHover = m_thumbHover = false;
	}
	return true;
}

void Slider::render() {
	Widget::renderBgAndBorders();
	TextWidget::renderText(0); // label
	TextWidget::renderText(1); // value %

	// slide bar
	Vec2i size = getSize();
	int cy = size.y / 2;
	Vec2i pos(m_shaftOffset + 5, cy - 3);
	Vec2i sz(m_shaftSize - 10, 6);

	Widget::renderBorders(m_shaftStyle, pos, sz);

	// slider thumb
	ImageWidget::renderImage();
	if (m_thumbHover || m_shaftHover) {
		float anim = getRootWindow()->getAnim();
		if (anim > 0.5f) {
			anim = 1.f - anim;
		}
		float borderAlpha = 0.1f + anim * 0.5f;
		float centreAlpha = 0.3f + anim;
		//int offset = getBorderSize() + getPadding();
		Widget::renderHighLight(Vec3f(1.f), centreAlpha, borderAlpha, m_thumbPos, m_thumbSize);
	}
}

// =====================================================
//  class VerticalScrollBar
// =====================================================

VerticalScrollBar::VerticalScrollBar(Container* parent)
		: Widget(parent)
		, ImageWidget(this)
		, MouseWidget(this)
		, hoverPart(0), pressedPart(0)
		//, fullThumb(false), smallThumb(false)
		, shaftOffset(0), shaftHeight(0)
		, thumbOffset(0), thumbSize(0)
		, totalRange(0), availRange(0), lineSize(0)
		, timeCounter(0) {
	init();
}

VerticalScrollBar::VerticalScrollBar(Container* parent, Vec2i pos, Vec2i size)
		: Widget(parent, pos, size)
		, ImageWidget(this)
		, MouseWidget(this)
		, hoverPart(0), pressedPart(0)
		//, fullThumb(false), smallThumb(false)
		, shaftOffset(0), shaftHeight(0)
		, thumbOffset(0), thumbSize(0)
		, totalRange(0), availRange(0), lineSize(0)
		, timeCounter(0) {
	init();
	recalc();
}

VerticalScrollBar::~VerticalScrollBar() {
	getRootWindow()->unregisterUpdate(this);
}

void VerticalScrollBar::init() {
	m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::SCROLL_BAR);
	m_backgroundStyle = g_widgetConfig.getBackgroundStyle(WidgetType::SCROLL_BAR);
	setPadding(0);
	addImage(g_coreData.getVertScrollUpTexture());
	addImage(g_coreData.getVertScrollUpHoverTex());
	addImage(g_coreData.getVertScrollDownTexture());
	addImage(g_coreData.getVertScrollDownHoverTex());
	m_shaftStyle.setEmbed(WidgetColour::LIGHT_BORDER, WidgetColour::DARK_BORDER);
	m_shaftStyle.setSizes(1);
	m_thumbStyle.setRaise(WidgetColour::LIGHT_BORDER, WidgetColour::DARK_BORDER);
	m_thumbStyle.setSizes(1);
}

void VerticalScrollBar::recalc() {
	Vec2i size = getSize();
	Vec2i imgSize = Vec2i(size.x);
	Vec2i upPos(0, 0);
	Vec2i downPos(0, size.y - imgSize.y);
	setImageX(0, 0, upPos, imgSize);
	setImageX(0, 1, upPos, imgSize);
	setImageX(0, 2, downPos, imgSize);
	setImageX(0, 3, downPos, imgSize);
	shaftOffset = imgSize.y;
	shaftHeight = size.y - imgSize.y * 2;
	topOffset = imgSize.y + 1;
	thumbOffset = 0;
	float availRatio = availRange / float(totalRange);
	thumbSize = int(availRatio * (shaftHeight - 2));
}

void VerticalScrollBar::setRanges(int total, int avail, int line) {
	if (total < avail) {
		total = avail;
	}
	totalRange = total;
	availRange = avail;
	lineSize = line;
	recalc();
}

void VerticalScrollBar::setOffset(float percent) {
	const int min = 0;
	const int max = (shaftHeight - 2) - thumbSize;

	thumbOffset = clamp(int(max - percent * max / 100.f), min, max);
	ThumbMoved(this);
}

bool VerticalScrollBar::mouseDown(MouseButton btn, Vec2i pos) {
	Vec2i localPos = pos - getScreenPos();
	pressedPart = hoverPart;
	if (pressedPart == Part::UPPER_SHAFT || pressedPart == Part::LOWER_SHAFT) {
		thumbOffset = clamp(localPos.y - topOffset - thumbSize / 2, 0, (shaftHeight - 2) - thumbSize);
		ThumbMoved(this);
		pressedPart = hoverPart = Part::THUMB;
	} else if (pressedPart == Part::UP_BUTTON || pressedPart == Part::DOWN_BUTTON) {
		getRootWindow()->registerUpdate(this);
		timeCounter = 0;
		moveOnMouseUp = true;
	}
	return true;
}

bool VerticalScrollBar::mouseUp(MouseButton btn, Vec2i pos) {
	Vec2i localPos = pos - getScreenPos();
	Part upPart = partAt(localPos);

	if (pressedPart == Part::UP_BUTTON || pressedPart == Part::DOWN_BUTTON) {
		getRootWindow()->unregisterUpdate(this);
	}
	if (pressedPart == upPart && moveOnMouseUp) {
		if (pressedPart == Part::UP_BUTTON) {
			scrollLine(true);
		} else if (pressedPart == Part::DOWN_BUTTON) {
			scrollLine(false);
		}
	}
	pressedPart = Part::NONE;
	return true;
}

void VerticalScrollBar::scrollLine(bool i_up) {
	thumbOffset += i_up ? -lineSize : lineSize;
	thumbOffset = clamp(thumbOffset, 0, (shaftHeight - 2) - thumbSize);
	ThumbMoved(this);
}

void VerticalScrollBar::update() {
	++timeCounter;
	if (timeCounter % 7 != 0) {
		return;
	}
	scrollLine(pressedPart == Part::UP_BUTTON);
	moveOnMouseUp = false;
}

VerticalScrollBar::Part VerticalScrollBar::partAt(const Vec2i &pos) {
	if (pos.y < shaftOffset) {
		return Part::UP_BUTTON;
	} else if (pos.y > shaftOffset + shaftHeight) {
		return Part::DOWN_BUTTON;
	} else if (pos.y < shaftOffset + thumbOffset) {
		return Part::UPPER_SHAFT;
	} else if (pos.y > shaftOffset + thumbOffset + thumbSize) {
		return Part::LOWER_SHAFT;
	} else {
		return Part::THUMB;
	}
}

bool VerticalScrollBar::mouseMove(Vec2i pos) {
	Vec2i localPos = pos - getScreenPos();
	if (pressedPart == Part::NONE) {
		hoverPart = partAt(localPos);
		//cout << "Part: " << hoverPart << endl;
	} else {
		if (pressedPart == Part::DOWN_BUTTON) {
			if (hoverPart == Part::DOWN_BUTTON && localPos.y >= shaftOffset) {
				hoverPart = Part::NONE;
			} else if (hoverPart == Part::NONE && localPos.y < shaftOffset) {
				hoverPart = Part::DOWN_BUTTON;
			}
		} else if (pressedPart == Part::UP_BUTTON) {
			if (hoverPart == Part::UP_BUTTON && localPos.y <= shaftOffset + shaftHeight) {
				hoverPart = Part::NONE;
			} else if (hoverPart == Part::NONE && localPos.y > shaftOffset + shaftHeight) {
				hoverPart = Part::UP_BUTTON;
			}
		} else if (pressedPart == Part::THUMB) {
			thumbOffset = clamp(localPos.y - topOffset - thumbSize / 2, 0, (shaftHeight - 2) - thumbSize);
			ThumbMoved(this);
		} else {
			// don't care for shaft clicked here
		}
	}
	return true;
}

Vec2i VerticalScrollBar::getPrefSize() const {return Vec2i(-1);}
Vec2i VerticalScrollBar::getMinSize() const {return Vec2i(-1);}

void VerticalScrollBar::render() {
	// buttons
	renderImage((pressedPart == Part::UP_BUTTON) ? 1 : 0);	// up arrow
	renderImage((pressedPart == Part::DOWN_BUTTON) ? 3 : 2); // down arrow
	
	if (hoverPart == Part::UP_BUTTON || hoverPart == Part::DOWN_BUTTON) {
		float anim = getRootWindow()->getAnim();
		if (anim > 0.5f) {
			anim = 1.f - anim;
		}
		float borderAlpha = 0.1f + anim * 0.5f;
		float centreAlpha = 0.3f + anim;

		Vec2i pos(0, hoverPart == Part::UP_BUTTON ? 0 : shaftOffset + shaftHeight);
		Vec2i size(getSize().x);
		renderHighLight(Vec3f(1.f), centreAlpha, borderAlpha, pos, size);
	}

	// shaft
	Vec2i shaftPos(0, shaftOffset);
	Vec2i shaftSize(shaftOffset, shaftHeight);
	renderBorders(m_shaftStyle, shaftPos, shaftSize);
	renderBackground(m_backgroundStyle, shaftPos, shaftSize);

	// thumb
	Vec2i thumbPos(1, topOffset + thumbOffset);
	Vec2i thumbSizev(shaftOffset - 2, thumbSize);
	renderBorders(m_thumbStyle, thumbPos, thumbSizev);
	if (hoverPart == Part::THUMB) {
		renderHighLight(Vec3f(1.f), 0.2f, 0.5f, thumbPos, thumbSizev);
	}
}

// =====================================================
// class CellStrip
// =====================================================

CellStrip::CellStrip(Container *parent, Orientation ld, int cells)
		: Container(parent)
		, m_direction(ld)
		, m_dirty(false) {
	for (int i=0; i < cells; ++i) {
		m_cells.push_back(new WidgetCell(this));
	}
}

CellStrip::CellStrip(Container *parent, Vec2i pos, Vec2i size, Orientation ld, int cells) 
		: Container(parent, pos, size)
		, m_direction(ld)
		, m_dirty(false) {
	for (int i=0; i < cells; ++i) {
		m_cells.push_back(new WidgetCell(this));
	}
}

void CellStrip::addChild(Widget *child) {
	Container::addChild(child);
}

void CellStrip::setPos(const Vec2i &pos) {
	Container::setPos(pos);
	setDirty();
}

void CellStrip::setSize(const Vec2i &sz) {
	Container::setSize(sz);
	setDirty();
}

void CellStrip::render() {
	renderBgAndBorders(false);
	if (m_dirty) {
		layoutCells();
	}
	Container::render();
}

typedef vector<SizeHint>    HintList;
typedef pair<int, int>      CellDim;
typedef vector<CellDim>     CellDimList;

/** splits up an amount of space (single dimension) according to a set of hints
  * @param hints the list of hints, one for each cell
  * @param space the amount of space to split up
  * @param out_res result vector, each entry is an offset and size pair
  * @return the amount of space that is "left over" (not allocated to a cell)
  */
int calculateCellDims(HintList &hints, const int space, CellDimList &out_res) {
	RUNTIME_CHECK_MSG(space > 0, "calculateCellDims(): called with no space.");
	RUNTIME_CHECK_MSG(out_res.empty(), "calculateCellDims(): output vector not empty!");
	if (hints.empty()) {
		return 0; // done ;)
	}
	const int count = hints.size();

	// Pass 1
	// count number of percentage hints and number of them that are 'default', and determine 
	// space for percentage hinted cells (ie, subtract space taken by absolute hints)
	int numPcnt = 0;
	int numDefPcnt = 0;
	int pcntTally = 0;
	int pcntSpace = space;
	foreach_const (HintList, it, hints) {
		if (it->isPercentage()) {
			if (it->getPercentage() >= 0) {
				pcntTally += it->getPercentage();
			} else {
				++numDefPcnt;
			}
			++numPcnt;
		} else {
			pcntSpace -= it->getAbsolute();
		}
	}

	float percent = pcntSpace / 100.f;  // pixels per percent
	int defPcnt;
	if (100 - pcntTally > 0 && numDefPcnt) { // default percentage hints get this much...
		defPcnt = (100 - pcntTally) / numDefPcnt;
	} else {
		defPcnt = 0;
	}

	// Pass 2
	// convert percentages and write cell sizes to output vector
	int offset = 0, size;
	foreach_const (HintList, it, hints) {
		if (it->isPercentage()) {
			if (it->getPercentage() >= 0) {
				size = int(it->getPercentage() * percent);
			} else {
				size = int(defPcnt * percent);
			}
		} else {
			size = it->getAbsolute();
		}
		out_res.push_back(std::make_pair(offset, size));
		offset += size;
	}
	return space - offset;
}

void CellStrip::layoutCells() {
	// collect hints
	HintList     hintList;
	foreach (vector<WidgetCell*>, it, m_cells) {
		hintList.push_back(static_cast<WidgetCell*>(*it)->getSizeHint());
	}
	// determine space available
	int space;
	if (m_direction == Orientation::VERTICAL) {
		space = getHeight() - getBordersVert() - 2 * getPadding();
	} else if (m_direction == Orientation::HORIZONTAL) {
		space = getWidth() - getBordersHoriz() - 2 * getPadding();
	} else {
		throw runtime_error("WidgetStrip has invalid direction.");
	}
	if (space < 1) {
		return;
	}

	// split space according to hints
	CellDimList  resultList;
	int offset = calculateCellDims(hintList, space, resultList) / 2;

	// determine cell width and x-pos OR height and y-pos
	int ppos, psize;
	if (m_direction == Orientation::VERTICAL) {
		ppos = getPadding() + getBorderLeft();
		psize = getWidth() - getPadding() * 2 - getBordersHoriz();
	} else {
		ppos = getPadding() + getBorderTop();
		psize = getHeight() - getPadding() * 2 - getBordersVert();
	}
	// combine results and set cell pos and size
	Vec2i pos, size;
	for (int i=0; i < m_cells.size(); ++i) {
		if (m_direction == Orientation::VERTICAL) {
			pos = Vec2i(ppos, offset + resultList[i].first);
			size = Vec2i(psize, resultList[i].second);
		} else {
			pos = Vec2i(offset + resultList[i].first, ppos);
			size = Vec2i(resultList[i].second, psize);
		}
		m_cells[i]->setPos(pos);
		m_cells[i]->setSize(size);
	}
	m_dirty = false;
}

// =====================================================
//  class Panel
// =====================================================

Panel::Panel(Container* parent)
		: Container(parent)
		, autoLayout(true)
		, layoutOrigin(Origin::CENTRE) {
	setPaddingParams(10, 5);
	m_borderStyle.setNone();
}

Panel::Panel(Container* parent, Vec2i pos, Vec2i sz)
		: Container(parent, pos, sz)
		, autoLayout(true)
		, layoutOrigin(Origin::CENTRE) {
	setPaddingParams(10, 5);
	m_borderStyle.setNone();
}

Panel::Panel(WidgetWindow* window)
		: Container(window) {
}

void Panel::setPaddingParams(int panelPad, int widgetPad) {
	setPadding(panelPad);
	widgetPadding = widgetPad;
}

void Panel::setLayoutParams(bool autoLayout, Orientation dir, Origin origin) {
	assert(
		origin == Origin::CENTRE
		|| ((origin == Origin::FROM_BOTTOM || origin == Origin::FROM_TOP)
			&& dir == Orientation::VERTICAL)
		|| ((origin == Origin::FROM_LEFT || origin == Origin::FROM_RIGHT)
			&& dir == Orientation::HORIZONTAL)
	);
	this->layoutDirection = dir;
	this->layoutOrigin = origin;		
	this->autoLayout = autoLayout;
}

void Panel::layoutChildren() {
	if (!autoLayout || m_children.empty()) {
		return;
	}
	if (layoutDirection == Orientation::VERTICAL) {
		layoutVertical();
	} else if (layoutDirection == Orientation::HORIZONTAL) {
		layoutHorizontal();
	}
}

void Panel::layoutVertical() {
	vector<int> widgetYPos;
	int wh = 0;
	Vec2i size = getSize();
	Vec2i room = size - m_borderStyle.getBorderDims() - Vec2i(getPadding()) * 2;
	foreach (WidgetList, it, m_children) {
		wh +=  + widgetPadding;
		widgetYPos.push_back(wh);
		wh += (*it)->getHeight();
	}
	wh -= widgetPadding;
	
	Vec2i topLeft(m_borderStyle.m_sizes[Border::LEFT] + getPadding(), 
		getPos().y + m_borderStyle.m_sizes[Border::TOP] + getPadding());
	
	int offset;
	switch (layoutOrigin) {
		case Origin::FROM_TOP: offset = m_borderStyle.m_sizes[Border::TOP]; break;
		case Origin::CENTRE: offset = (size.y - wh) / 2; break;
		case Origin::FROM_BOTTOM: offset = size.y - wh - m_borderStyle.m_sizes[Border::TOP]; break;
	}
	int ndx = 0;
	foreach (WidgetList, it, m_children) {
		int ww = (*it)->getWidth();
		int x = topLeft.x + (room.x - ww) / 2;
		int y = offset + widgetYPos[ndx++];
		(*it)->setPos(x, y);
	}
}

void Panel::layoutHorizontal() {
	vector<int> widgetXPos;
	int ww = 0;
	Vec2i size = getSize();
	Vec2i room = size - m_borderStyle.getBorderDims() - Vec2i(getPadding()) * 2;
	foreach (WidgetList, it, m_children) {
		widgetXPos.push_back(ww);
		ww += (*it)->getWidth();
		ww += widgetPadding;
	}
	ww -= widgetPadding;
	
	Vec2i topLeft(getBorderLeft() + getPadding(), size.y - getBorderTop() - getPadding());
	
	int offset;
	switch (layoutOrigin) {
		case Origin::FROM_LEFT: offset = getBorderLeft(); break;
		case Origin::CENTRE: offset = (size.x - ww) / 2; break;
		case Origin::FROM_RIGHT: offset = size.x - ww - getBorderRight(); break;
	}
	int ndx = 0;
	foreach (WidgetList, it, m_children) {
		int wh = (*it)->getHeight();
		int x = offset + widgetXPos[ndx++];
		int y = (room.y - wh) / 2;
		(*it)->setPos(x, y);
	}
}

Vec2i Panel::getMinSize() const {
	///TODO Compositor...
	return Vec2i(-1);
}

Vec2i Panel::getPrefSize() const {
	return Vec2i(-1);
}

void Panel::addChild(Widget* child) {
	Container::addChild(child);
	if (!autoLayout) {
		return;
	}
	Vec2i sz = child->getSize();
	int space_x = getWidth() - m_borderStyle.getHorizBorderDim() - getPadding() * 2;
	if (sz.x > space_x) {
		child->setSize(space_x, sz.y);
	}
	layoutChildren();
}

void Panel::render() {
	assertGl();
	Widget::renderBgAndBorders();
	Vec2i pos = getScreenPos();
	pos.x += getBorderLeft() + getPadding();
	pos.y = g_config.getDisplayHeight() - (pos.y + getHeight())
		  + m_borderStyle.m_sizes[Border::BOTTOM] + getPadding();
	Vec2i size = getSize() - m_borderStyle.getBorderDims() - Vec2i(getPadding() * 2);
	glPushAttrib(GL_SCISSOR_BIT);
		assertGl();
		/*if (glIsEnabled(GL_SCISSOR_TEST)) { ///@todo ? take intersection ?
			Vec4i box;
			glGetIntegerv(GL_SCISSOR_BOX, box.ptr());
		} else */{
			glEnable(GL_SCISSOR_TEST);
		}
		glScissor(pos.x, pos.y, size.w, size.h);
		Container::render();
	glPopAttrib();
	assertGl();
}

// =====================================================
//  class PicturePanel
// =====================================================

Vec2i PicturePanel::getMinSize() const {
	const Pixmap2D *pixmap = getImage()->getPixmap();
	Vec2i imgDim = Vec2i(pixmap->getW(), pixmap->getH());
	Vec2i xtra = m_borderStyle.getBorderDims() + Vec2i(getPadding() * 2);
	return imgDim + xtra;
}

Vec2i PicturePanel::getPrefSize() const {
	return Vec2i(-1);
}

// =====================================================
//  class ListBase
// =====================================================

ListBase::ListBase(Container* parent)
		: Panel(parent)
		, selectedItem(0)
		, selectedIndex(-1)
		, itemFont(0) {
	setPaddingParams(2, 0);
	setAutoLayout(false);
	itemFont = CoreData::getInstance().getFTMenuFontNormal();
}

ListBase::ListBase(Container* parent, Vec2i pos, Vec2i size)
		: Panel(parent, pos, size)
		, selectedItem(0)
		, selectedIndex(-1)
		, itemFont(0) {
	setPaddingParams(2, 0);
	setAutoLayout(false);
	itemFont = CoreData::getInstance().getFTMenuFontNormal();
}

ListBase::ListBase(WidgetWindow* window) 
		: Panel(window)
		, selectedItem(0)
		, selectedIndex(-1) {
	itemFont = CoreData::getInstance().getFTMenuFontNormal();
}

// =====================================================
//  class ListBox
// =====================================================

ListBox::ListBox(Container* parent)
		: ListBase(parent)
		, MouseWidget(this)
		, scrollBar(0)
//		, scrollSetting(ScrollSetting::AUTO)
		, scrollWidth(24) {
	m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::LIST_BOX);
	setPadding(0);
	setAutoLayout(false);
}

ListBox::ListBox(Container* parent, Vec2i pos, Vec2i size) 
		: ListBase(parent, pos, size)
		, MouseWidget(this)
		, scrollBar(0)
//		, scrollSetting(ScrollSetting::AUTO)
		, scrollWidth(24) {
	m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::LIST_BOX);
	setPadding(0);
	setAutoLayout(false);
}

ListBox::ListBox(WidgetWindow* window)
		: ListBase(window)
		, MouseWidget(this)
		, scrollBar(0)
//		, scrollSetting(ScrollSetting::AUTO)
		, scrollWidth(24) {
	m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::LIST_BOX);
	setPadding(0);
	setAutoLayout(false);
}

void ListBox::onSelected(ListBoxItem* item) {
	if (selectedItem != item) {
		if (selectedItem) {
			selectedItem->setSelected(false);
		}
		selectedItem = item;
		selectedItem->setSelected(true);
		for (int i = 0; i < listBoxItems.size(); ++i) {
			if (item == listBoxItems[i]) {
				selectedIndex = i;
				SelectionChanged(this);
				return;
			}
		}
	} else {
		SameSelected(this);
	}
}

void ListBox::addItems(const vector<string> &items) {
	Vec2i sz(getSize().x - 4, int(itemFont->getMetrics()->getHeight()) + 4);
	foreach_const (vector<string>, it, items) {
		ListBoxItem *nItem = new ListBoxItem(this, Vec2i(0), sz, Vec3f(0.25f));
		nItem->setTextParams(*it, Vec4f(1.f), itemFont, true);
		nItem->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
		listBoxItems.push_back(nItem);
		nItem->Selected.connect(this, &ListBox::onSelected);
	}
}

void ListBox::addItem(const string &item) {
	Vec2i sz(getSize().x - getBordersHoriz(), int(itemFont->getMetrics()->getHeight()) + 4);
	ListBoxItem *nItem = new ListBoxItem(this, Vec2i(getBorderLeft(), getBorderBottom()), sz, Vec3f(0.25f));
	nItem->setTextParams(item, Vec4f(1.f), itemFont, true);
	nItem->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
	listBoxItems.push_back(nItem);
	nItem->Selected.connect(this, &ListBox::onSelected);
}

void ListBox::setSize(const Vec2i &sz) {
	Widget::setSize(sz);
	layoutChildren();
}

void ListBox::layoutChildren() {
	if (m_children.empty()) {
		return;
	}
	const int padHoriz = m_borderStyle.getHorizBorderDim() + getPadding() * 2;
	const int padVert = m_borderStyle.getVertBorderDim() + getPadding() * 2;
	Vec2i size = getSize();

	// scrollBar ?
	int totalItemHeight = getPrefHeight() - padVert;
	int clientHeight = size.y - padVert;

	if (totalItemHeight > clientHeight) {
		if (!scrollBar) {
			scrollBar = new VerticalScrollBar(this);
			scrollBar->ThumbMoved.connect(this, &ListBox::onScroll);
		}
		scrollBar->setSize(Vec2i(24, clientHeight));
		scrollBar->setPos(Vec2i(size.x - 24 - m_borderStyle.m_sizes[Border::RIGHT], 
								m_borderStyle.m_sizes[Border::BOTTOM]));
		scrollBar->setRanges(totalItemHeight, clientHeight, 2);
		//cout << "setting scroll ranges total = " << totalItemHeight << ", actual = " << clientHeight << endl;
		//int scrollOffset = scrollBar->getRangeOffset();
		//cout << "range offset = " << scrollOffset << endl;
	} else {
		if (scrollBar) {
			delete scrollBar;
			scrollBar = 0;
		}
	}

	vector<int> widgetYPos;
	int wh = 0;
	Vec2i room = size - m_borderStyle.getBorderDims() - Vec2i(getPadding() * 2);
	if (scrollBar) {
		room.x -= scrollBar->getWidth();
	}
	foreach (WidgetList, it, m_children) {
		if (*it == scrollBar) {
			continue;
		}
		widgetYPos.push_back(wh);
		wh += (*it)->getHeight() + widgetPadding;
	}
	
	Vec2i topLeft(m_borderStyle.m_sizes[Border::LEFT] + getPadding(),
				  m_borderStyle.m_sizes[Border::TOP] + getPadding());

	int ndx = 0;
	yPositions.clear();
	foreach (WidgetList, it, m_children) {
		if (*it == scrollBar) {
			continue;
		}
		int y = topLeft.y + widgetYPos[ndx++];
		(*it)->setPos(topLeft.x, y);
		(*it)->setSize(room.x, (*it)->getHeight());
		static_cast<ListBoxItem*>(*it)->centreText();
		yPositions.push_back(y);
	}
}

void ListBox::onScroll(VerticalScrollBar*) {
	int offset = scrollBar->getRangeOffset();
	//cout << "Scroll offset = " << offset << endl;

	int ndx = 0;
	const int x = m_borderStyle.m_sizes[Border::LEFT] + getPadding();
	foreach (WidgetList, it, m_children) {
		if (*it == scrollBar) {
			continue;
		}
		//Widget* widget = *it;
		(*it)->setPos(x, yPositions[ndx++] - offset);
	}
}

bool ListBox::mouseWheel(Vec2i pos, int z) {
	if (scrollBar) {
		scrollBar->scrollLine(z > 0);
	}
	return true;
}

int ListBox::getPrefHeight(int childCount) {
	int res = m_borderStyle.getVertBorderDim() + getPadding() * 2;
	int iSize = int(g_widgetConfig.getFont(WidgetFont::MENU_NORMAL)->getMetrics()->getHeight()) + 4;
	if (childCount == -1) {
		childCount = m_children.size();//listBoxItems.size();
	}
	res += iSize * childCount;
	if (childCount) {
		res += widgetPadding * (childCount - 1);
	}
	return res;
}

///@todo handle no m_children
Vec2i ListBox::getMinSize() const {
	Vec2i res(0);
	foreach_const (WidgetList, it, m_children) {
		Vec2i ips = (*it)->getPrefSize();
		if (ips.x > res.x) res.x = ips.x;
		if (ips.y > res.y) res.y = ips.y;
	}
	res.y *= 3;
	res += m_borderStyle.getBorderDims() + Vec2i(getPadding() * 2);
	return res;
}

Vec2i ListBox::getPrefSize() const {
	Vec2i res(0);
	foreach_const (WidgetList, it, m_children) {
		Vec2i ips = (*it)->getPrefSize();
		if (ips.x > res.x) res.x = ips.x;
		res.y += ips.y;
	}
	res += m_borderStyle.getBorderDims() + Vec2i(getPadding() * 2);
	return res;
}

void ListBox::setSelected(int index) {
	if (index < 0) {
		if (!selectedItem) {
			assert(selectedIndex == -1);
			return;
		}
		selectedIndex = -1;
		if (selectedItem) {
			selectedItem->setSelected(false);
			selectedItem = 0;
		}
	} else if (index < listBoxItems.size()) {
		if (selectedItem == listBoxItems[index]) {
			return;
		}
		if (selectedItem) {
			selectedItem->setSelected(false);
		}
		selectedItem = listBoxItems[index];
		selectedItem->setSelected(true);
		selectedIndex = index;
	}
	SelectionChanged(this);
}

// =====================================================
//  class ListBoxItem
// =====================================================

ListBoxItem::ListBoxItem(ListBase* parent)
		: StaticText(parent) 
		, MouseWidget(this)
		, selected(false)
		, hover(false)
		, pressed(false) {
	m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::LIST_ITEM);
	m_backgroundStyle = g_widgetConfig.getBackgroundStyle(WidgetType::LIST_ITEM);
}

ListBoxItem::ListBoxItem(ListBase* parent, Vec2i pos, Vec2i sz)
		: StaticText(parent, pos, sz)
		, MouseWidget(this)
		, selected(false)
		, hover(false)
		, pressed(false) {
	m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::LIST_ITEM);
	m_backgroundStyle = g_widgetConfig.getBackgroundStyle(WidgetType::LIST_ITEM);
}

ListBoxItem::ListBoxItem(ListBase* parent, Vec2i pos, Vec2i sz, const Vec3f &bgColour)
		: StaticText(parent, pos, sz)
		, MouseWidget(this)
		, selected(false)
		, hover(false)
		, pressed(false) {
	m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::LIST_ITEM);
	m_backgroundStyle.m_type = BackgroundType::COLOUR;
	m_backgroundStyle.m_colourIndices[0] = g_widgetConfig.getColourIndex(bgColour);
}

Vec2i ListBoxItem::getMinSize() const {
	Vec2i dims = getTextDimensions();
	Vec2i xtra = m_borderStyle.getBorderDims() + Vec2i(getPadding() * 2);
	return dims + xtra;
}

Vec2i ListBoxItem::getPrefSize() const {
	return getMinSize();
}

void ListBoxItem::render() {
	StaticText::render();
	if (isEnabled() && selected) {
		Widget::renderHighLight(Vec3f(1.f), 0.1f, 0.3f);
	}
	assertGl();
}

void ListBoxItem::setBackgroundColour(const Vec3f &colour) {
	m_backgroundStyle.m_type = BackgroundType::COLOUR;
	m_backgroundStyle.m_colourIndices[0] = g_widgetConfig.getColourIndex(colour);
}

void ListBoxItem::mouseIn() {
	if (isEnabled()) {
		m_borderStyle.m_colourIndices[0] = m_borderStyle.m_colourIndices[2];
		hover = true;
	}
}

void ListBoxItem::mouseOut() {
	m_borderStyle.m_colourIndices[0] = m_borderStyle.m_colourIndices[1];
	hover = false;
}

bool ListBoxItem::mouseDown(MouseButton btn, Vec2i pos) {
	if (isEnabled() && btn == MouseButton::LEFT) {
		pressed = true;
		return true;
	}
	return false;
}

bool ListBoxItem::mouseUp(MouseButton btn, Vec2i pos) {
	if (isEnabled() && btn == MouseButton::LEFT) {
		if (hover) {
			Selected(this);
			//static_cast<ListBase*>(parent)->setSelected(this);
		}
		if (hover && pressed) {
			Clicked(this);
		}
		pressed = false;
		return true;
	}
	return false;
}

// =====================================================
//  class DropList
// =====================================================

DropList::DropList(Container* parent)
		: ListBase(parent)
		, MouseWidget(this)
		, floatingList(0)
		, dropBoxHeight(0) {
	m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::DROP_LIST);
	setAutoLayout(false);
	setPadding(0);
	button = new Button(this);
	button->Clicked.connect(this, &DropList::onExpandList);
	selectedItem = new ListBoxItem(this);
	selectedItem->setTextParams("", Vec4f(1.f), itemFont, true);
	selectedItem->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
	selectedItem->Clicked.connect(this, &DropList::onBoxClicked);
}

DropList::DropList(Container* parent, Vec2i pos, Vec2i size) 
		: ListBase(parent, pos, size)
		, MouseWidget(this)
		, floatingList(0)
		, dropBoxHeight(0) {
	m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::DROP_LIST);
	setAutoLayout(false);
	setPadding(0);
	button = new Button(this);
	button->Clicked.connect(this, &DropList::onExpandList);
	selectedItem = new ListBoxItem(this);
	selectedItem->setTextParams("", Vec4f(1.f), itemFont, true);
	selectedItem->setShadow(Vec4f(0.f, 0.f, 0.f, 1.f));
	selectedItem->Clicked.connect(this, &DropList::onBoxClicked);
	layout();
}

Vec2i DropList::getPrefSize() const {
	Vec2i res = selectedItem->getPrefSize();
	res.x += res.y - getBordersVert();
	res += getBordersAll() + Vec2i(getPadding() * 2);
	return  res;
}

Vec2i DropList::getMinSize() const {
	Vec2i res = selectedItem->getMinSize();
	res.x += res.y - getBordersVert();
	res += getBordersAll() + Vec2i(getPadding() * 2);
	return  res;
}

void DropList::setSize(const Vec2i &size) {
	Widget::setSize(size);
	layout();
}

void DropList::layout() {
//	const int borderSize = getBorderSize();
	Vec2i size = getSize();
	int btn_sz = size.y - getBordersVert() - getPadding() * 2;
	button->setSize(Vec2i(btn_sz));
	button->setPos(Vec2i(size.x - btn_sz - getBorderRight() - getPadding(), getBorderBottom() + getPadding()));
	Vec2i liPos(getBorderLeft() + getPadding(), getBorderBottom() + getPadding());
	Vec2i liSz(size.x - btn_sz - getBordersHoriz() - getPadding() * 2, btn_sz);
	selectedItem->setPos(liPos);
	selectedItem->setSize(liSz);
}

bool DropList::mouseWheel(Vec2i pos, int z) {
	if (selectedIndex == -1) {
		return true;
	}
	if (z > 0) {
		if (selectedIndex != 0) {
			setSelected(selectedIndex - 1);
		}
	} else {
		if (selectedIndex != listItems.size() - 1) {
			setSelected(selectedIndex + 1);
		}
	}
	return true;
}

void DropList::addItems(const vector<string> &items) {
	foreach_const (vector<string>, it, items) {
		listItems.push_back(*it);
	}
}

void DropList::addItem(const string &item) {
	listItems.push_back(item);
}

void DropList::clearItems() {
	listItems.clear();
	selectedItem->setText("");
	selectedIndex = -1;
}

void DropList::setSelected(int index) {
	if (index < 0 || index >= listItems.size()) {
		if (selectedIndex == -1) {
			return;
		}
		selectedItem->setText("");
		selectedIndex = -1;
	} else {
		if (selectedIndex == index) {
			return;
		}
		selectedItem->setText(listItems[index]);
		selectedIndex = index;
	}
	SelectionChanged(this);
}

void DropList::setSelected(const string &item) {
	int ndx = -1;
	for (int i=0; i < listItems.size(); ++i) {
		if (listItems[i] == item) {
			ndx = i;
			break;
		}
	}
	if (ndx != -1) {
		setSelected(ndx);
	}
}

void DropList::expandList() {
	floatingList = new ListBox(getRootWindow());
	getRootWindow()->setFloatingWidget(floatingList);
	floatingList->setPaddingParams(0, 0);
	const Vec2i &size = getSize();
	const Vec2i &screenPos = getScreenPos();
	int num = listItems.size();
	int ph = floatingList->getPrefHeight(num);
	int h = dropBoxHeight == 0 ? ph : ph > dropBoxHeight ? dropBoxHeight : ph;

	Vec2i sz(size.x, h);
	Vec2i pos(screenPos.x, screenPos.y);
	floatingList->setPos(pos);
	floatingList->addItems(listItems);
	floatingList->setSize(sz);

	floatingList->setSelected(selectedIndex);
	floatingList->Destroyed.connect(this, &DropList::onListDisposed);
	floatingList->SelectionChanged.connect(this, &DropList::onSelectionMade);
	floatingList->SameSelected.connect(this, &DropList::onSameSelected);
	setVisible(false);
	ListExpanded(this);
}

void DropList::onBoxClicked(ListBoxItem*) {
	if (isEnabled()) {
		expandList();
	}
}

void DropList::onExpandList(Button*) {
	if (isEnabled()) {
		expandList();
	}
}

void DropList::onSelectionMade(ListBase* lb) {
	assert(floatingList == lb);
	setSelected(lb->getSelectedIndex());
	floatingList->Destroyed.disconnect(this);
	onListDisposed(lb);
	getRootWindow()->removeFloatingWidget(lb);
}

void DropList::onSameSelected(ListBase* lb) {
	assert(floatingList == lb);
	floatingList->Destroyed.disconnect(this);
	onListDisposed(lb);
	getRootWindow()->removeFloatingWidget(lb);
}

void DropList::onListDisposed(Widget*) {
	setVisible(true);
	floatingList = 0;
	ListCollapsed(this);
}

// =====================================================
//  class ToolTip
// =====================================================

void ToolTip::init() {
	m_borderStyle = g_widgetConfig.getBorderStyle(WidgetType::TOOL_TIP);
	m_backgroundStyle = g_widgetConfig.getBackgroundStyle(WidgetType::TOOL_TIP);
	TextWidget::setTextParams("", Vec4f(1.f, 1.f, 1.f, 1.f), g_coreData.getFTDisplayFontBig(), false);
	TextWidget::setTextPos(Vec2i(4, 4));
}

ToolTip::ToolTip(Container* parent)
		: StaticText(parent) {
	init();
}

ToolTip::ToolTip(Container* parent, Vec2i pos, Vec2i size)
		: StaticText(parent, pos, size) {
	init();
}

void ToolTip::setText(const string &txt) {
	TextWidget::setText(txt);
	Vec2i dims = TextWidget::getTextDimensions(0);
	dims += Vec2i(8);
	setSize(dims);
}

}}

