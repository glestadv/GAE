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
//#include "core_data.h"

#include "widget_window.h"

#include "leak_dumper.h"

using Shared::Util::deleteValues;
using Shared::Graphics::Texture;
using namespace Shared::Graphics::Gl;
using Glest::Graphics::Renderer;
//using namespace Glest::Global;

namespace Glest { namespace Widgets {

// =====================================================
//  class StaticText
// =====================================================

StaticText::StaticText(Container* parent)
		: Widget(parent) , TextWidget(this)
		, m_shadow(false), m_doubleShadow(false), m_shadowOffset(2) {
	setWidgetStyle(WidgetType::STATIC_WIDGET);
}

StaticText::StaticText(Container* parent, Vec2i pos, Vec2i size)
		: Widget(parent, pos, size), TextWidget(this)
		, m_shadow(false), m_doubleShadow(false), m_shadowOffset(2) {
	setWidgetStyle(WidgetType::STATIC_WIDGET);
}

void StaticText::render() {
	if (!isVisible()) {
		return;
	}
	Widget::renderBackground();
	if (TextWidget::getText(0) != "") {
		if (m_doubleShadow) {
			TextWidget::renderTextDoubleShadowed(0, m_shadowOffset);
		} else if (m_shadow) {
			TextWidget::renderTextShadowed(0, m_shadowOffset);
		} else {
			TextWidget::renderText();
		}
	}
	Widget::renderForeground();
}

Vec2i StaticText::getMinSize() const {
	Vec2i txtDim = getTextDimensions();
	Vec2i xtra = getBordersAll();
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
	setWidgetStyle(WidgetType::STATIC_WIDGET);
}

StaticImage::StaticImage(Container* parent, Vec2i pos, Vec2i size) 
		: Widget(parent, pos, size)
		, ImageWidget(this) {
	setWidgetStyle(WidgetType::STATIC_WIDGET);
}

StaticImage::StaticImage(Container* parent, Vec2i pos, Vec2i size, Texture2D *tex)
		: Widget(parent, pos, size)
		, ImageWidget(this, tex) {
	setWidgetStyle(WidgetType::STATIC_WIDGET);
}

Vec2i StaticImage::getMinSize() const {
	const Pixmap2D *pixmap = getImage()->getPixmap();
	Vec2i imgDim = Vec2i(pixmap->getW(), pixmap->getH());
	Vec2i xtra = getBordersAll();
	return imgDim + xtra;
}

Vec2i StaticImage::getPrefSize() const {
	return getMinSize();
}

void StaticImage::render() {
	if (!isVisible()) {
		return;
	}
	Widget::renderBackground();
	ImageWidget::renderImage();
	Widget::renderForeground();
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
		, m_doHoverHighlight(true) {
	setWidgetStyle(WidgetType::BUTTON);
}

Button::Button(Container* parent, Vec2i pos, Vec2i size, bool hoverHighlight)
		: Widget(parent, pos, size)
		, TextWidget(this)
		, MouseWidget(this)
		, m_doHoverHighlight(hoverHighlight) {
	setWidgetStyle(WidgetType::BUTTON);
}

Vec2i Button::getPrefSize() const {
	Vec2i imgSize(0);
	if (m_borderStyle.m_type == BorderType::TEXTURE) {
		const Texture2D *tex = m_rootWindow->getConfig()->getTexture(m_borderStyle.m_imageNdx);
		imgSize = Vec2i(tex->getPixmap()->getW(), tex->getPixmap()->getH());
	}
	Vec2i txtSize(getMinSize());
	Vec2i res(std::max(imgSize.x, txtSize.x), std::max(imgSize.y, txtSize.y));
	return res;
}

Vec2i Button::getMinSize() const {
	Vec2i res = TextWidget::getTextDimensions() + getBordersAll();
	res.x = std::min(res.x, 16);
	res.y = std::min(res.y, 16);
	return res;
}

void Button::setSize(const Vec2i &sz) {
	Widget::setSize(sz);
}

bool Button::mouseMove(Vec2i pos) {
	WIDGET_LOG( descLong() << " : Button::mouseMove( " << pos << " )");
	if (isEnabled()) {
		if (isHovered() && !isInside(pos)) {
			mouseOut();
		}
		if (!isHovered() && isInside(pos)) {
			mouseIn();
		}
	}
	return true;
}

bool Button::mouseDown(MouseButton btn, Vec2i pos) {
	WIDGET_LOG( descLong() << " : Button::mouseDown( " << MouseButtonNames[btn] << ", " << pos << " )");
	if (isEnabled() && btn == MouseButton::LEFT) {
		setFocus(true);
	}
	return true;
}

bool Button::mouseUp(MouseButton btn, Vec2i pos) {
	WIDGET_LOG( descLong() << " : Button::mouseUp( " << MouseButtonNames[btn] << ", " << pos << " )");
	if (isEnabled() && btn == MouseButton::LEFT) {
		if (isFocused() && isHovered()) {
			Clicked(this);
		}
		setFocus(false);
	}
	return true;
}

void Button::render() {
	Widget::renderBackground();
	if (TextWidget::hasText()) {
		TextWidget::renderText();
	}
	Widget::renderForeground();
}

// =====================================================
//  class CheckBox
// =====================================================

CheckBox::CheckBox(Container* parent)
		: Button(parent), m_checked(false)
		, ImageWidget(this) {
	setWidgetStyle(WidgetType::CHECK_BOX);
	setSize(getPrefSize());
}

CheckBox::CheckBox(Container* parent, Vec2i pos, Vec2i size)
		: Button(parent, pos, size, false), m_checked(false)
		, ImageWidget(this) {
	setWidgetStyle(WidgetType::CHECK_BOX);
}

void CheckBox::setSize(const Vec2i &sz) {
	// bypass Button::setSize()
	Widget::setSize(sz);
	//int y = int((sz.y - getTextFont()->getMetrics()->getHeight()) / 2);
	//setTextPos(Vec2i(40, y), 0);
	//setTextPos(Vec2i(40, y), 1);
}

Vec2i CheckBox::getMinSize() const {
	Vec2i txtDim = getTextDimensions();
	Vec2i xtra(
		m_borderStyle.m_sizes[Border::LEFT] + m_borderStyle.m_sizes[Border::RIGHT],
		m_borderStyle.m_sizes[Border::TOP] + m_borderStyle.m_sizes[Border::BOTTOM]
	);
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
	return dim + xtra;
}

bool CheckBox::mouseDown(MouseButton btn, Vec2i pos) {
	if (btn == MouseButton::LEFT) {
		setFocus(true);
		return true;
	}
	return false;
}

bool CheckBox::mouseUp(MouseButton btn, Vec2i pos) {
	if (btn == MouseButton::LEFT) {
		if (isFocused() && isHovered()) {
			m_checked = !m_checked;
			Clicked(this);
		}
		setFocus(false);
		return true;
	}
	return false;
}

// =====================================================
//  class TextBox
// =====================================================

TextBox::TextBox(Container* parent)
		: Widget(parent)
		, MouseWidget(this)
		, KeyboardWidget(this)
		, TextWidget(this)
		, changed(false) {
	setWidgetStyle(WidgetType::TEXT_BOX);
}

TextBox::TextBox(Container* parent, Vec2i pos, Vec2i size)
		: Widget(parent, pos, size)
		, MouseWidget(this)
		, KeyboardWidget(this)
		, TextWidget(this)
		, changed(false) {
	setWidgetStyle(WidgetType::TEXT_BOX);
}

void TextBox::gainFocus() {
	if (isEnabled()) {
		setFocus(true);
		getRootWindow()->aquireKeyboardFocus(this);
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
	setFocus(false);
}

void TextBox::render() {
	Widget::renderBackground();
	TextWidget::renderText();
	Widget::renderForeground();
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
//
//Slider::Slider(Container* parent, Vec2i pos, Vec2i size, const string &title)
//		: Widget(parent, pos, size)
//		, MouseWidget(this)
//		, ImageWidget(this)
//		, TextWidget(this)
//		, m_sliderValue(0.f)
//		, m_thumbHover(false)
//		, m_thumbPressed(false)
//		, m_shaftHover(false)
//		, m_title(title) {
//	setWidgetStyle(WidgetType::SLIDER);
//
//	WidgetConfig &cfg = *m_rootWindow->getConfig();
//	Font *font = cfg.getMenuFont()[FontSize::NORMAL];
//	setTextParams(m_title, Vec4f(1.f), font, false); // ndx 0
//	addText("0 %"); // ndx 1
//	
//	string maxVal = "100 %";
//	Vec2f dims = getTextFont()->getMetrics()->getTextDiminsions(maxVal);
//	m_valSize = int(dims.x + 5.f);
//
//	recalc();
//}
//
//void Slider::recalc() {
//	Vec2i size = getSize();	
//
//	int space = size.x - m_valSize;
//	m_shaftOffset = int(space * 0.35f);
//	m_shaftSize = int(space * 0.60f);
//
//	m_thumbCentre = m_shaftOffset + 5 + int(m_sliderValue * (m_shaftSize - 10));
//
//	space = size.y - (m_borderStyle.m_sizes[Border::TOP] + m_borderStyle.m_sizes[Border::BOTTOM]);
//	m_thumbPos = Vec2i(m_thumbCentre - space / 4 + m_borderStyle.m_sizes[Border::LEFT], 
//						m_borderStyle.m_sizes[Border::BOTTOM]);
//	m_thumbSize = Vec2i(space / 2, space);
//	setImageX(0, 0, m_thumbPos, m_thumbSize);
//
//	Vec2f dims = getTextFont()->getMetrics()->getTextDiminsions(m_title);
//	m_titlePos = Vec2i(m_shaftOffset / 2 - int(dims.x / 2), size.y / 2 - int(dims.y / 2));
//
//	string sliderString = Conversion::toStr(int(m_sliderValue * 100.f)) + " %";
//	setText(sliderString, 1);
//	dims = getTextFont()->getMetrics()->getTextDiminsions(sliderString);
//	int valWidth = size.x - m_shaftOffset - m_shaftSize;
//	m_valuePos = Vec2i(m_shaftOffset + m_shaftSize + (valWidth / 2 - int(dims.x / 2)), size.y / 2 - int(dims.y / 2));
//	setTextPos(m_titlePos, 0);
//	setTextPos(m_valuePos, 1);
//}
//
//bool Slider::mouseMove(Vec2i pos) {
//	pos -= getScreenPos();
//	if (m_thumbPressed) {
//		int x_pos = clamp(pos.x - m_shaftOffset - 5, 0, m_shaftSize - 10);
//		float oldVal = m_sliderValue;
//		m_sliderValue = x_pos / float(m_shaftSize - 10);
//		recalc();
//		if (oldVal != m_sliderValue) {
//			ValueChanged(this);
//		}
//		return true;
//	}
//	if (Vec2i::isInside(pos, m_thumbPos, m_thumbSize)) {
//		m_thumbHover = true;
//	} else {
//		m_thumbHover = false;
//	}
//	if (!m_thumbHover) {
//		if (pos.x >= m_shaftOffset + 3 && pos.x < m_shaftOffset + m_shaftSize - 3) {
//			m_shaftHover = true;
//		} else {
//			m_shaftHover = false;
//		}
//	} else {
//		m_shaftHover = true;
//	}
//	return true;
//}
//
//void Slider::mouseOut() {
//	if (!m_thumbPressed) {
//		m_shaftHover = m_thumbHover = false;
//	}
//}
//
//bool Slider::mouseDown(MouseButton btn, Vec2i pos) {
//	if (btn != MouseButton::LEFT) {
//		return false;
//	}
//	pos -= getScreenPos();
//	if (m_thumbHover || m_shaftHover) {
//		m_thumbHover = true;
//		m_shaftHover = false;
//		m_thumbPressed = true;
//
//		int x_pos = clamp(pos.x - m_shaftOffset - 5, 0, m_shaftSize - 10);
//		float oldVal = m_sliderValue;
//		m_sliderValue = x_pos / float(m_shaftSize - 10);
//		recalc();
//		if (oldVal != m_sliderValue) {
//			ValueChanged(this);
//		}
//	}
//	return true;
//}
//
//bool Slider::mouseUp(MouseButton btn, Vec2i pos) {
//	if (btn != MouseButton::LEFT) {
//		return false;
//	}
//	if (m_thumbPressed) {
//		m_thumbPressed = false;
//	}
//	if (isInside(pos)) {
//		pos -= getScreenPos();
//		if (Vec2i::isInside(pos, m_thumbPos, m_thumbSize)) {
//			m_thumbHover = true;
//		} else {
//			m_thumbHover = false;
//		}
//		if (!m_thumbHover) {
//			if (pos.x >= m_shaftOffset + 3 && pos.x < m_shaftOffset + m_shaftSize - 3) {
//				m_shaftHover = true;
//			} else {
//				m_shaftHover = false;
//			}
//		} else {
//			m_shaftHover = false;
//		}
//
//	} else {
//		m_shaftHover = m_thumbHover = false;
//	}
//	return true;
//}
//
//void Slider::render() {
//	Widget::renderBackground();
//	TextWidget::renderText(0); // label
//	TextWidget::renderText(1); // value %
//
//	// slide bar
//	Vec2i size = getSize();
//	int cy = size.y / 2;
//	Vec2i pos(m_shaftOffset + 5, cy - 3);
//	Vec2i sz(m_shaftSize - 10, 6);
//
//	Widget::renderBorders(m_shaftStyle, pos, sz);
//
//	// slider thumb
//	ImageWidget::renderImage();
//	if (m_thumbHover || m_shaftHover) {
//		float anim = getRootWindow()->getAnim();
//		if (anim > 0.5f) {
//			anim = 1.f - anim;
//		}
//		float borderAlpha = 0.1f + anim * 0.5f;
//		float centreAlpha = 0.3f + anim;
//		//int offset = getBorderSize();
//		int ndx = m_rootWindow->getConfig()->getColourIndex(Vec3f(1.f));
//		Widget::renderHighLight(ndx, centreAlpha, borderAlpha, m_thumbPos, m_thumbSize);
//	}
//	Widget::renderForeground();
//}

// =====================================================
// class CellStrip
// =====================================================

CellStrip::CellStrip(WidgetWindow *window, Orientation ortn, Origin orgn, int cells)
		: Container(window)
		, m_orientation(ortn)
		, m_origin(orgn)
		, m_dirty(false) {
	for (int i=0; i < cells; ++i) {
		m_cells.push_back(new WidgetCell(this));
	}
}

CellStrip::CellStrip(Container *parent, Orientation ortn)
		: Container(parent)
		, m_orientation(ortn)
		, m_origin(Origin::CENTRE)
		, m_dirty(false) {
}

CellStrip::CellStrip(Container *parent, Orientation ortn, int cells)
		: Container(parent)
		, m_orientation(ortn)
		, m_origin(Origin::CENTRE)
		, m_dirty(false) {
	for (int i=0; i < cells; ++i) {
		m_cells.push_back(new WidgetCell(this));
	}
}

CellStrip::CellStrip(Container *parent, Orientation ortn, Origin orgn, int cells)
		: Container(parent)
		, m_orientation(ortn)
		, m_origin(orgn)
		, m_dirty(false) {
	for (int i=0; i < cells; ++i) {
		m_cells.push_back(new WidgetCell(this));
	}
}

CellStrip::CellStrip(Container *parent, Vec2i pos, Vec2i size, Orientation ortn) 
		: Container(parent, pos, size)
		, m_orientation(ortn)
		, m_origin(Origin::CENTRE)
		, m_dirty(false) {
}

CellStrip::CellStrip(Container *parent, Vec2i pos, Vec2i size, Orientation ortn, Origin orgn, int cells) 
		: Container(parent, pos, size)
		, m_orientation(ortn)
		, m_origin(orgn)
		, m_dirty(false) {
	for (int i=0; i < cells; ++i) {
		m_cells.push_back(new WidgetCell(this));
	}
}

void CellStrip::setCustumCell(int ndx, WidgetCell *cell) {
	ASSERT_RANGE(ndx, m_cells.size());
	WidgetCell *old = m_cells[ndx];
	m_cells[ndx] = cell;
	m_children[ndx] = cell;
	delete old;
}

void CellStrip::addChild(Widget *child) {
	Container::addChild(child);
}

void CellStrip::clearCells() {
	foreach (vector<WidgetCell*>, it, m_cells) {
		(*it)->clear();
	}
}

void CellStrip::deleteCells() {
	foreach (vector<WidgetCell*>, it, m_cells) {
		delete *it;
	}
	m_cells.clear();
}

void CellStrip::addCells(int n) {
	for (int i=0; i < n; ++i) {
		m_cells.push_back(new WidgetCell(this));
	}
}

void CellStrip::setPos(const Vec2i &pos) {
	Container::setPos(pos);
	setDirty();
}

void CellStrip::setSize(const Vec2i &sz) {
	Container::setSize(sz);
	//layoutCells();
	setDirty();
}

void CellStrip::render() {
//	renderBackground();
	if (m_dirty) {
		WIDGET_LOG( descShort() << " : CellStrip::render() : dirty, laying out cells." );
		layoutCells();
	}
	//Container::render();
	Widget::render();

	// clip children
	Vec2i pos = getScreenPos();
	pos.x += getBorderLeft();
	pos.y += getBorderTop();
	Vec2i size = getSize() - m_borderStyle.getBorderDims();
	m_rootWindow->pushClipRect(pos, size);

	if (m_orientation == Orientation::VERTICAL) {
		foreach (WidgetList, it, m_children) {
			Widget* widget = *it;
			if (widget->isVisible() && widget->getPos().y < getSize().h
			&& widget->getPos().y + widget->getSize().h >= 0 ) {
				widget->render();
			}
		}
	} else { // Orientation::HORIZONTAL
		assert(m_orientation == Orientation::HORIZONTAL);
		foreach (WidgetList, it, m_children) {
			Widget* widget = *it;
			if (widget->isVisible() && widget->getPos().x < getSize().w
			&& widget->getPos().x + widget->getSize().w >= 0 ) {
				widget->render();
			}
		}
	}
	m_rootWindow->popClipRect();
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
	WIDGET_LOG( descLong() << " : CellStrip::layoutCells()" );
	m_dirty = false;
	// collect hints
	HintList     hintList;
	foreach (vector<WidgetCell*>, it, m_cells) {
		hintList.push_back(static_cast<WidgetCell*>(*it)->getSizeHint());
	}
	// determine space available
	int space;
	if (m_orientation == Orientation::VERTICAL) {
		space = getHeight() - getBordersVert();
	} else if (m_orientation == Orientation::HORIZONTAL) {
		space = getWidth() - getBordersHoriz();
	} else {
		throw runtime_error("WidgetStrip has invalid direction.");
	}
	if (space < 1) {
		return;
	}

	// split space according to hints
	CellDimList  resultList;
	int offset;
	int spare = calculateCellDims(hintList, space, resultList);
	if (m_origin == Origin::FROM_TOP || m_origin == Origin::FROM_LEFT) {
		offset = 0;
	} else if (m_origin == Origin::CENTRE) {
		offset = spare / 2;
	} else {
		offset = space - spare;
	}

	// determine cell width and x-pos OR height and y-pos
	int ppos, psize;
	if (m_orientation == Orientation::VERTICAL) {
		ppos = getBorderLeft();
		psize = getWidth() - getBordersHoriz();
	} else {
		ppos = getBorderTop();
		psize = getHeight() - getBordersVert();
	}
	// combine results and set cell pos and size
	Vec2i pos, size;
	for (int i=0; i < m_cells.size(); ++i) {
		if (m_orientation == Orientation::VERTICAL) {
			pos = Vec2i(ppos, getBorderTop() + offset + resultList[i].first);
			size = Vec2i(psize, resultList[i].second);
		} else {
			pos = Vec2i(getBorderLeft() + offset + resultList[i].first, ppos);
			size = Vec2i(resultList[i].second, psize);
		}
		WIDGET_LOG( descShort() << " : CellStrip::layoutCells(): setting cell " << i 
			<< " [" << m_cells[i]->descId() << "] pos: " << pos << ", size = " << size );
		m_cells[i]->setPos(pos);
		m_cells[i]->setSize(size);
	}
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
	//setPadding(panelPad);
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
	Vec2i room = size - m_borderStyle.getBorderDims();
	foreach (WidgetList, it, m_children) {
		wh +=  + widgetPadding;
		widgetYPos.push_back(wh);
		wh += (*it)->getHeight();
	}
	wh -= widgetPadding;
	
	Vec2i topLeft(m_borderStyle.m_sizes[Border::LEFT], 
		getPos().y + m_borderStyle.m_sizes[Border::TOP]);
	
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
	Vec2i room = size - m_borderStyle.getBorderDims();
	foreach (WidgetList, it, m_children) {
		widgetXPos.push_back(ww);
		ww += (*it)->getWidth();
		ww += widgetPadding;
	}
	ww -= widgetPadding;
	
	Vec2i topLeft(getBorderLeft(), size.y - getBorderTop());
	
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
	int space_x = getWidth() - m_borderStyle.getHorizBorderDim() * 2;
	if (sz.x > space_x) {
		child->setSize(space_x, sz.y);
	}
	layoutChildren();
}

void Panel::render() {
	assertGl();
	Vec2i pos = getScreenPos();
	pos.x += getBorderLeft();
	pos.y += getBorderTop();
	Vec2i size = getSize() - m_borderStyle.getBorderDims();
	m_rootWindow->pushClipRect(pos, size);
	Container::render();
	m_rootWindow->popClipRect();
}

// =====================================================
//  class PicturePanel
// =====================================================

Vec2i PicturePanel::getMinSize() const {
	const Pixmap2D *pixmap = getImage()->getPixmap();
	Vec2i imgDim = Vec2i(pixmap->getW(), pixmap->getH());
	Vec2i xtra = m_borderStyle.getBorderDims();
	return imgDim + xtra;
}

Vec2i PicturePanel::getPrefSize() const {
	return Vec2i(-1);
}

// =====================================================
//  class ToolTip
// =====================================================

void ToolTip::init() {
	setWidgetStyle(WidgetType::TOOL_TIP);
	WidgetConfig &cfg = *m_rootWindow->getConfig();
	Font *font = cfg.getFontSet(m_textStyle.m_fontIndex)[FontSize::BIG];
	TextWidget::setTextParams("", Vec4f(1.f, 1.f, 1.f, 1.f), font, false);
	TextWidget::setTextPos(Vec2i(getBorderLeft(), getBorderTop()));
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

