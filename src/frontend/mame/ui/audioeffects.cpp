// license:BSD-3-Clause
// copyright-holders:Olivier Galibert
/*********************************************************************

    ui/audioeffects.cpp

    Audio effects control

*********************************************************************/

#include "emu.h"
#include "ui/audioeffects.h"

#include "ui/audio_effect_compressor.h"
#include "ui/audio_effect_eq.h"
#include "ui/audio_effect_filter.h"
#include "ui/audio_effect_reverb.h"
#include "ui/ui.h"

#include "audio_effects/aeffect.h"

#include "speaker.h"

#include "osdepend.h"

namespace ui {

menu_audio_effects::menu_audio_effects(mame_ui_manager &mui, render_container &container)
	: menu(mui, container)
{
	set_heading(_("menu-aeffect", "Audio Effects"));
	set_process_flags(PROCESS_LR_REPEAT);
}

menu_audio_effects::~menu_audio_effects()
{
}

float menu_audio_effects::change_f(const float *table, float value, int change)
{
	u32 bi = 0;
	float dt = 1e30;
	u32 index;
	for(index = 0; table[index]; index++) {
		float d1 = value - table[index];
		if(d1 < 0)
			d1 = -d1;
		if(d1 < dt) {
			dt = d1;
			bi = index;
		}
	}
	if((change != -1 || bi != 0) && (change != 1 || bi != index-1))
		bi += change;
	return table[bi];
}

u32 menu_audio_effects::change_u32(const u32 *table, u32 value, int change)
{
	u32 bi = 0;
	s32 dt = 2e9;
	u32 index;
	for(index = 0; table[index]; index++) {
		s32 d1 = value - table[index];
		if(d1 < 0)
			d1 = -d1;
		if(d1 < dt) {
			dt = d1;
			bi = index;
		}
	}
	if((change != -1 || bi != 0) && (change != 1 || bi != index-1))
		bi += change;
	return table[bi];
}

bool menu_audio_effects::handle(event const *ev)
{
	static const float latencies[] = {
		0.0005f, 0.0010f, 0.0025f, 0.0050f, 0.0100f, 0.0250f, 0.0500f, 0
	};

	static const u32 lengths[] = {
		10, 20, 30, 40, 50, 75, 100, 200, 300, 400, 500, 0
	};

	static const u32 phases[] = {
		10, 20, 30, 40, 50, 75, 100, 200, 300, 400, 500, 1000, 0
	};

	if(!ev)
		return false;

	switch(ev->iptkey) {
	case IPT_UI_SELECT: {
		u16 chain = (uintptr_t(ev->itemref)) >> 16;
		u16 entry = (uintptr_t(ev->itemref)) & 0xffff;
		if(entry & 0xf000)
			return false;
		audio_effect *eff = chain == 0xffff ? machine().sound().default_effect_chain()[entry] : machine().sound().effect_chain(chain)[entry];
		switch(eff->type()) {
		case audio_effect::COMPRESSOR:
			menu::stack_push<menu_audio_effect_compressor>(ui(), container(), chain, entry, eff);
			break;

		case audio_effect::EQ:
			menu::stack_push<menu_audio_effect_eq>(ui(), container(), chain, entry, eff);
			break;

		case audio_effect::FILTER:
			menu::stack_push<menu_audio_effect_filter>(ui(), container(), chain, entry, eff);
			break;

		case audio_effect::REVERB:
			menu::stack_push<menu_audio_effect_reverb>(ui(), container(), chain, entry, eff);
			break;
		}
		return true;
	}

	case IPT_UI_CLEAR: {
		switch(uintptr_t(ev->itemref)) {
		case RS_TYPE:
			machine().sound().set_resampler_type(machine().sound().default_resampler_type());
			reset(reset_options::REMEMBER_POSITION);
			return true;

		case RS_LATENCY:
			machine().sound().set_resampler_hq_latency(machine().sound().default_resampler_hq_latency());
			ev->item->set_subtext(format_lat(machine().sound().resampler_hq_latency()));
			ev->item->set_flags(flag_lat());
			return true;

		case RS_LENGTH:
			machine().sound().set_resampler_hq_length(machine().sound().default_resampler_hq_length());
			ev->item->set_subtext(format_u32(machine().sound().resampler_hq_length()));
			ev->item->set_flags(flag_length());
			return true;

		case RS_PHASES:
			machine().sound().set_resampler_hq_phases(machine().sound().default_resampler_hq_phases());
			ev->item->set_subtext(format_u32(machine().sound().resampler_hq_phases()));
			ev->item->set_flags(flag_phases());
			return true;
		}
		break;
	}

	case IPT_UI_LEFT: {
		switch(uintptr_t(ev->itemref)) {
		case RS_TYPE:
			machine().sound().set_resampler_type(sound_manager::RESAMPLER_LOFI);
			reset(reset_options::REMEMBER_POSITION);
			return true;

		case RS_LATENCY:
			machine().sound().set_resampler_hq_latency(change_f(latencies, machine().sound().resampler_hq_latency(), -1));
			ev->item->set_subtext(format_lat(machine().sound().resampler_hq_latency()));
			ev->item->set_flags(flag_lat());
			return true;

		case RS_LENGTH:
			machine().sound().set_resampler_hq_length(change_u32(lengths, machine().sound().resampler_hq_length(), -1));
			ev->item->set_subtext(format_u32(machine().sound().resampler_hq_length()));
			ev->item->set_flags(flag_length());
			return true;

		case RS_PHASES:
			machine().sound().set_resampler_hq_phases(change_u32(phases, machine().sound().resampler_hq_phases(), -1));
			ev->item->set_subtext(format_u32(machine().sound().resampler_hq_phases()));
			ev->item->set_flags(flag_phases());
			return true;
		}
		break;
	}

	case IPT_UI_RIGHT: {
		switch(uintptr_t(ev->itemref)) {
		case RS_TYPE:
			machine().sound().set_resampler_type(sound_manager::RESAMPLER_HQ);
			reset(reset_options::REMEMBER_POSITION);
			return true;

		case RS_LATENCY:
			machine().sound().set_resampler_hq_latency(change_f(latencies, machine().sound().resampler_hq_latency(), 1));
			ev->item->set_subtext(format_lat(machine().sound().resampler_hq_latency()));
			ev->item->set_flags(flag_lat());
			return true;

		case RS_LENGTH:
			machine().sound().set_resampler_hq_length(change_u32(lengths, machine().sound().resampler_hq_length(), 1));
			ev->item->set_subtext(format_u32(machine().sound().resampler_hq_length()));
			ev->item->set_flags(flag_length());
			return true;

		case RS_PHASES:
			machine().sound().set_resampler_hq_phases(change_u32(phases, machine().sound().resampler_hq_phases(), 1));
			ev->item->set_subtext(format_u32(machine().sound().resampler_hq_phases()));
			ev->item->set_flags(flag_phases());
			return true;
		}
		break;
	}
	}

	return false;
}


std::string menu_audio_effects::format_lat(float latency)
{
	return util::string_format(_("menu-aeffect", "%1$3.1f ms"), 1000 * latency);
}

std::string menu_audio_effects::format_u32(u32 val)
{
	return util::string_format(_("menu-aeffect", "%u"), val);
}

u32 menu_audio_effects::flag_type() const
{
	u32 flag = 0;
	u32 type = machine().sound().resampler_type();
	if(type != sound_manager::RESAMPLER_LOFI)
		flag |= FLAG_LEFT_ARROW;
	if(type != sound_manager::RESAMPLER_HQ)
		flag |= FLAG_RIGHT_ARROW;
	return flag;
}

u32 menu_audio_effects::flag_lat() const
{
	u32 flag = 0;
	float latency = machine().sound().resampler_hq_latency();
	if(latency > 0.0005f)
		flag |= FLAG_LEFT_ARROW;
	if(latency < 0.0500f)
		flag |= FLAG_RIGHT_ARROW;
	if(machine().sound().resampler_type() != sound_manager::RESAMPLER_HQ)
		flag |= FLAG_INVERT | FLAG_DISABLE;
	return flag;
}

u32 menu_audio_effects::flag_length() const
{
	u32 flag = 0;
	u32 length = machine().sound().resampler_hq_length();
	if(length > 10)
		flag |= FLAG_LEFT_ARROW;
	if(length < 500)
		flag |= FLAG_RIGHT_ARROW;
	if(machine().sound().resampler_type() != sound_manager::RESAMPLER_HQ)
		flag |= FLAG_INVERT | FLAG_DISABLE;
	return flag;
}

u32 menu_audio_effects::flag_phases() const
{
	u32 flag = 0;
	u32 phases = machine().sound().resampler_hq_phases();
	if(phases > 10)
		flag |= FLAG_LEFT_ARROW;
	if(phases < 1000)
		flag |= FLAG_RIGHT_ARROW;
	if(machine().sound().resampler_type() != sound_manager::RESAMPLER_HQ)
		flag |= FLAG_INVERT | FLAG_DISABLE;
	return flag;
}

void menu_audio_effects::populate()
{
	auto &sound = machine().sound();
	for(s32 chain = 0; chain != sound.effect_chains(); chain++) {
		std::string tag = sound.effect_chain_tag(chain);
		item_append(tag, FLAG_UI_HEADING | FLAG_DISABLE, nullptr);
		auto eff = sound.effect_chain(chain);
		for(u32 e = 0; e != eff.size(); e++)
			item_append(_(audio_effect::effect_names[eff[e]->type()]), 0, (void *)intptr_t((chain << 16) | e));
	}
	item_append(_("menu-aeffect", "Default"), FLAG_UI_HEADING | FLAG_DISABLE, nullptr);
	auto eff = sound.default_effect_chain();
	for(u32 e = 0; e != eff.size(); e++)
		item_append(_("audio-effect", audio_effect::effect_names[eff[e]->type()]), 0, (void *)intptr_t((0xffff << 16) | e));

	item_append(_("menu-aeffect", "Resampler"), FLAG_UI_HEADING | FLAG_DISABLE, nullptr);
	item_append(_("menu-aeffect", "Type"), sound.resampler_type_names(sound.resampler_type()), flag_type(), (void *)RS_TYPE);
	item_append(_("menu-aeffect", "HQ latency"), format_lat(sound.resampler_hq_latency()), flag_lat(), (void *)RS_LATENCY);
	item_append(_("menu-aeffect", "HQ filter max size"), format_u32(sound.resampler_hq_length()), flag_length(), (void *)RS_LENGTH);
	item_append(_("menu-aeffect", "HQ filter max phases"), format_u32(sound.resampler_hq_phases()), flag_phases(), (void *)RS_PHASES);
	item_append(menu_item_type::SEPARATOR);
}

void menu_audio_effects::recompute_metrics(uint32_t width, uint32_t height, float aspect)
{
	menu::recompute_metrics(width, height, aspect);
}

void menu_audio_effects::custom_render(uint32_t flags, void *selectedref, float top, float bottom, float x1, float y1, float x2, float y2)
{
}

void menu_audio_effects::menu_activated()
{
	// scripts or the other form of the menu could have changed something in the mean time
	reset(reset_options::REMEMBER_POSITION);
}

void menu_audio_effects::menu_deactivated()
{
}


} // namespace ui

