/*
	Copyright (C) 2008 - 2022
	by Mark de Wever <koraq@xs4all.nl>
	Part of the Battle for Wesnoth Project https://www.wesnoth.org/

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY.

	See the COPYING file for more details.
*/

#define GETTEXT_DOMAIN "wesnoth-lib"

#include "gui/widgets/toggle_button.hpp"

#include "gui/core/register_widget.hpp"
#include "gui/widgets/settings.hpp"
#include "gui/widgets/window.hpp"
#include "gui/core/log.hpp"
#include "gui/core/window_builder/helper.hpp"
#include "sound.hpp"

#include <functional>

#define LOG_SCOPE_HEADER get_control_type() + " [" + id() + "] " + __func__
#define LOG_HEADER LOG_SCOPE_HEADER + ':'

namespace gui2
{

// ------------ WIDGET -----------{

REGISTER_WIDGET(toggle_button)

toggle_button::toggle_button(const implementation::builder_toggle_button& builder)
	: styled_widget(builder, type())
	, state_(ENABLED)
	, state_num_(0)
	, retval_(retval::NONE)
	, icon_name_()
{
	connect_signal<event::MOUSE_ENTER>(std::bind(
			&toggle_button::signal_handler_mouse_enter, this, std::placeholders::_2, std::placeholders::_3));
	connect_signal<event::MOUSE_LEAVE>(std::bind(
			&toggle_button::signal_handler_mouse_leave, this, std::placeholders::_2, std::placeholders::_3));

	connect_signal<event::LEFT_BUTTON_CLICK>(std::bind(
			&toggle_button::signal_handler_left_button_click, this, std::placeholders::_2, std::placeholders::_3));
	connect_signal<event::LEFT_BUTTON_DOUBLE_CLICK>(std::bind(
			&toggle_button::signal_handler_left_button_double_click,
			this,
			std::placeholders::_2,
			std::placeholders::_3));
}

unsigned toggle_button::num_states() const
{
	std::div_t res = std::div(this->config()->state.size(), COUNT);
	assert(res.rem == 0);
	assert(res.quot > 0);
	return res.quot;
}

void toggle_button::set_members(const widget_item& data)
{
	// Inherit
	styled_widget::set_members(data);

	widget_item::const_iterator itor = data.find("icon");
	if(itor != data.end()) {
		set_icon_name(itor->second);
	}
}

void toggle_button::set_active(const bool active)
{
	if(active) {
		set_state(ENABLED);
	} else {
		set_state(DISABLED);
	}
}

bool toggle_button::get_active() const
{
	return state_ != DISABLED;
}

unsigned toggle_button::get_state() const
{
	return state_ +  COUNT * state_num_;
}

void toggle_button::update_canvas()
{
	// Inherit.
	styled_widget::update_canvas();

	// set icon in canvases
	std::vector<canvas>& canvases = styled_widget::get_canvases();
	for(auto & canvas : canvases)
	{
		canvas.set_variable("icon", wfl::variant(icon_name_));
	}

	queue_redraw();
}

void toggle_button::set_value(unsigned selected, bool fire_event)
{
	selected = selected % num_states();
	if(selected == get_value()) {
		return;
	}
	state_num_ = selected;
	queue_redraw();

	// Check for get_window() is here to prevent the callback from
	// being called when the initial value is set.
	if(!get_window()) {
		return;
	}
	if (fire_event) {
		fire(event::NOTIFY_MODIFIED, *this, nullptr);
	}
}

void toggle_button::set_retval(const int retval)
{
	if(retval == retval_) {
		return;
	}

	retval_ = retval;
	set_wants_mouse_left_double_click(retval_ != gui2::retval::NONE);
}

void toggle_button::set_state(const state_t state)
{
	if(state != state_) {
		state_ = state;
		queue_redraw();
	}
}

void toggle_button::signal_handler_mouse_enter(const event::ui_event event,
												bool& handled)
{
	DBG_GUI_E << LOG_HEADER << ' ' << event << ".";
	set_state(FOCUSED);
	handled = true;
}

void toggle_button::signal_handler_mouse_leave(const event::ui_event event,
												bool& handled)
{
	DBG_GUI_E << LOG_HEADER << ' ' << event << ".";
	set_state(ENABLED);
	handled = true;
}

void toggle_button::signal_handler_left_button_click(const event::ui_event event,
													  bool& handled)
{
	DBG_GUI_E << LOG_HEADER << ' ' << event << ".";

	sound::play_UI_sound(settings::sound_toggle_button_click);

	set_value(get_value() + 1, true);

	handled = true;
}

void toggle_button::signal_handler_left_button_double_click(
		const event::ui_event event, bool& handled)
{
	DBG_GUI_E << LOG_HEADER << ' ' << event << ".";

	if(retval_ == retval::NONE) {
		return;
	}

	window* window = get_window();
	assert(window);

	window->set_retval(retval_);

	handled = true;
}

// }---------- DEFINITION ---------{

toggle_button_definition::toggle_button_definition(const config& cfg)
	: styled_widget_definition(cfg)
{
	DBG_GUI_P << "Parsing toggle button " << id;

	load_resolutions<resolution>(cfg);
}

toggle_button_definition::resolution::resolution(const config& cfg)
	: resolution_definition(cfg)
{
	// Note the order should be the same as the enum state_t in
	// toggle_button.hpp.
	for(const auto& c : cfg.child_range("state"))
	{
		state.emplace_back(c.child("enabled"));
		state.emplace_back(c.child("disabled"));
		state.emplace_back(c.child("focused"));
	}
}

// }---------- BUILDER -----------{

namespace implementation
{

builder_toggle_button::builder_toggle_button(const config& cfg)
	: builder_styled_widget(cfg)
	, icon_name_(cfg["icon"])
	, retval_id_(cfg["return_value_id"])
	, retval_(cfg["return_value"])
{
}

std::unique_ptr<widget> builder_toggle_button::build() const
{
	auto widget = std::make_unique<toggle_button>(*this);

	widget->set_icon_name(icon_name_);
	widget->set_retval(get_retval(retval_id_, retval_, id));

	DBG_GUI_G << "Window builder: placed toggle button '" << id
			  << "' with definition '" << definition << "'.";

	return widget;
}

} // namespace implementation

// }------------ END --------------

} // namespace gui2
