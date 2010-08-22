// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  GPL V3, see source/licence.txt
// ==============================================================

#ifndef _GLEST_COMPOUND_WIDGETS_INCLUDED_
#define _GLEST_COMPOUND_WIDGETS_INCLUDED_

#include "widgets.h"
#include "game_constants.h"

namespace Glest { namespace Widgets {
using Sim::ControlType;

class PlayerSlotWidget : public Panel, public sigslot::has_slots {
public:
	typedef PlayerSlotWidget* Ptr;
private:
	StaticText::Ptr m_label;
	DropList::Ptr	m_controlList;
	DropList::Ptr	m_factionList;
	DropList::Ptr	m_teamList;
	DropList::Ptr	m_colourList;

public:
	PlayerSlotWidget(Container::Ptr parent, Vec2i pos, Vec2i size);

	void setNameText(const string &name) { m_label->setText(name); }
	
	void setFactionItems(const vector<string> &items) {
		m_factionList->clearItems();
		m_factionList->addItems(items);
	}

	void setSelectedControl(ControlType ct) {
		m_controlList->setSelected(ct);
	}

	void setSelectedFaction(int ndx) {
		m_factionList->setSelected(ndx);
	}

	void setSelectedTeam(int team) {
		assert (team >= -1 && team < GameConstants::maxPlayers);
		m_teamList->setSelected(team);
	}

	void setSelectedColour(int ndx) {
		m_colourList->setSelectedColour(ndx);
	}

	ControlType getControlType() const { return ControlType(m_controlList->getSelectedIndex()); }
	int getSelectedFactionIndex() const { return m_factionList->getSelectedIndex(); }
	int getSelectedTeamIndex() const { return m_teamList->getSelectedIndex(); }
	int getSelectedColourIndex() const { return m_colourList->getSelectedIndex(); }

	sigslot::signal<Ptr> ControlChanged;
	sigslot::signal<Ptr> FactionChanged;
	sigslot::signal<Ptr> TeamChanged;
	sigslot::signal<Ptr> ColourChanged;

private: // re-route signals from DropLists
	void onControlChanged(ListBase::Ptr) {
		ControlChanged(this);
		if (m_controlList->getSelectedIndex() == ControlType::CLOSED) {
			m_factionList->setEnabled(false);
			m_teamList->setEnabled(false);
			m_colourList->setEnabled(false);
		} else {
			m_factionList->setEnabled(true);
			m_teamList->setEnabled(true);
			m_colourList->setEnabled(true);
		}
	}
	void onFactionChanged(ListBase::Ptr) { FactionChanged(this); }
	void onTeamChanged(ListBase::Ptr)	 { TeamChanged(this);	 }
	void onColourChanged(ListBase::Ptr)	 { ColourChanged(this);	 }
};

class OptionContainer : public Container {
public:
	typedef OptionContainer* Ptr;

private:
	StaticText::Ptr m_label;
	Widget::Ptr		m_widget;

	bool	m_abosulteLabelSize;
	int		m_labelSize;

public:
	OptionContainer(Container::Ptr parent, Vec2i pos, Vec2i size, const string &labelText);

	void setLabelWidth(int value, bool absolute);

	///@todo deprecate, over addChild, assume second child is contained widget
	void setWidget(Widget::Ptr widget);
	Widget::Ptr getWidget() { return m_widget; }

	virtual Vec2i getPrefSize() const;
	virtual Vec2i getMinSize() const;

	virtual string desc() { return string("[OptionBox: ") + descPosDim() + "]"; }
};

class ScrollText : public Panel, public MouseWidget, public TextWidget, public sigslot::has_slots {
public:
	typedef ScrollText* Ptr;

private:
	VerticalScrollBar::Ptr m_scrollBar;
	int m_textBase;

public:
	ScrollText(Container::Ptr parent);
	ScrollText(Container::Ptr parent, Vec2i pos, Vec2i size);

	void init();
	void onScroll(VerticalScrollBar::Ptr);
	void setText(const string &txt);

	void render();
};

class TitleBar : public Container, public TextWidget {
public:
	typedef TitleBar* Ptr;

private:
	string		m_title;
	Button::Ptr m_closeButton;

public:
	TitleBar(Container::Ptr parent);
	TitleBar(Container::Ptr parent, Vec2i pos, Vec2i size, string title, bool closeBtn);

	void render();

	virtual Vec2i getPrefSize() const;
	virtual Vec2i getMinSize() const;

	virtual string desc() { return string("[TitleBar: ") + descPosDim() + "]"; }
};

class BasicDialog : public Container, public MouseWidget, public sigslot::has_slots {
public:
	typedef BasicDialog* Ptr;

private:
	TitleBar::Ptr	m_titleBar;
	Widget::Ptr		m_content;
	Button::Ptr		m_button1,
					m_button2;

	int				m_buttonCount;
	bool			m_pressed;
	Vec2i			m_lastPos;

protected:
	BasicDialog(WidgetWindow*);
	void onButtonClicked(Button::Ptr);

protected:
	void setContent(Widget::Ptr content);
	void init(Vec2i pos, Vec2i size, const string &title, const string &btn1, const string &btn2);

public:
	void setTitleText(const string &text);
	void setButtonText(const string &btn1Text, const string &btn2Text = "");

	const string& getTitleText() const { return m_titleBar->getText(); }

	sigslot::signal<Ptr> Button1Clicked,
						 Button2Clicked;

	bool mouseDown(MouseButton btn, Vec2i pos);
	bool mouseMove(Vec2i pos);
	bool mouseUp(MouseButton btn, Vec2i pos);

	void render();
	virtual Vec2i getPrefSize() const { return Vec2i(-1); }
	virtual Vec2i getMinSize() const { return Vec2i(-1); }
	virtual string desc() { return string("[BasicDialog: ") + descPosDim() + "]"; }
};

class MessageDialog : public BasicDialog {
public:
	typedef MessageDialog* Ptr;

private:
	ScrollText::Ptr m_scrollText;

private:
	MessageDialog(WidgetWindow*);

public:
	static MessageDialog::Ptr showDialog(Vec2i pos, Vec2i size, const string &title,
					const string &msg, const string &btn1Text, const string &btn2Text);

	void setMessageText(const string &text);
	const string& getMessageText() const { return m_scrollText->getText(); }

	virtual string desc() { return string("[MessageDialog: ") + descPosDim() + "]"; }
};

class InputDialog : public BasicDialog {
public:
	typedef InputDialog* Ptr;

private:
	StaticText::Ptr	m_label;
	TextBox::Ptr	m_textBox;
	Panel::Ptr		m_panel;

private:
	InputDialog(WidgetWindow*);

	void onInputEntered(TextBox::Ptr);

public:
	static InputDialog::Ptr showDialog(Vec2i pos, Vec2i size, const string &title,
					const string &msg, const string &btn1Text, const string &btn2Text);

	void setMessageText(const string &text);
	void setInputMask(const string &allowMask) { m_textBox->setInputMask(allowMask); }

	const string& getMessageText() const { return m_label->getText(); }
	string getInput() const { return m_textBox->getText(); }

	virtual string desc() { return string("[InputDialog: ") + descPosDim() + "]"; }
};

}} // namespace Glest::Widgets

#endif
