-- license:BSD-3-Clause
-- copyright-holders:Jack Li
local exports = {
	name = 'toggle',
	version = '0.0.1',
	description = 'Toggle button plugin',
	license = 'BSD-3-Clause',
	author = { name = 'Jack Li' } }

local toggle = exports

local frame_subscription, stop_subscription

function toggle.startplugin()

	-- List of toggle buttons, each being a table with keys:
	--   'port' - port name of the button being toggled
	--   'mask' - mask of the button field being toggled
	--   'type' - input type of the button being toggled
	--   'key' - input_seq of the keybinding
	--   'key_cfg' - configuration string for the keybinding
	--   'key_pressed' - whether the toggle key is currently being pressed
	--   'button' - reference to ioport_field
	--   'on' - whether the button is toggled on
	local buttons = {}

	local input_manager
	local menu_handler

	local function process_frame()
		local function process_button(button)
			local new_key_pressed = input_manager:seq_pressed(button.key)
			local toggled = new_key_pressed and not button.key_pressed and not manager.ui.menu_active
			button.key_pressed = new_key_pressed
			if toggled then
				button.on = not button.on
			end
			return button.on and 1 or 0
		end

		-- Resolves conflicts between multiple toggle keybindings for the same button.
		local button_states = {}

		for i, button in ipairs(buttons) do
			if button.button then
				local key = button.port .. '\0' .. button.mask .. '.' .. button.type
				local state = button_states[key] or {0, button.button}
				state[1] = process_button(button) | state[1]
				button_states[key] = state
			end
		end
		for i, state in pairs(button_states) do
			if state[1] ~= 0 then
				state[2]:set_value(state[1])
			else
				state[2]:clear_value()
			end
		end
	end

	local function load_settings()
		local loader = require('toggle/toggle_save')
		if loader then
			buttons = loader:load_settings()
		end

		input_manager = manager.machine.input
	end

	local function save_settings()
		local saver = require('toggle/toggle_save')
		if saver then
			saver:save_settings(buttons)
		end

		menu_handler = nil
		input_manager = nil
		buttons = {}
	end

	local function menu_callback(index, event)
		if menu_handler then
			return menu_handler:handle_menu_event(index, event, buttons)
		else
			return false
		end
	end

	local function menu_populate()
		if not menu_handler then
			local status, msg = pcall(function () menu_handler = require('toggle/toggle_menu') end)
			if not status then
				emu.print_error(string.format('Error loading toggle buttons menu: %s', msg))
			end
			if menu_handler then
				menu_handler:init_menu(buttons)
			end
		end
		if menu_handler then
			return menu_handler:populate_menu(buttons)
		else
			return {{_p('plugin-toggle', 'Failed to load toggle buttons menu'), '', 'off'}}
		end
	end

	frame_subscription = emu.add_machine_frame_notifier(process_frame)
	emu.register_prestart(load_settings)
	stop_subscription = emu.add_machine_stop_notifier(save_settings)
	emu.register_menu(menu_callback, menu_populate, _p('plugin-toggle', 'Toggle buttons'))
end

return exports
