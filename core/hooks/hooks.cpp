#pragma once
#include "..//menu/menu.hpp"
#include "../../dependencies/common_includes.hpp"
#include <algorithm>

void hooks::initialize( ) {
	client_hook.setup(interfaces::client);
	client_hook.hook_index(37, frame_stage_notify);

	clientmode_hook.setup(interfaces::clientmode );
	clientmode_hook.hook_index(24, create_move);
	clientmode_hook.hook_index(35, viewmodel_fov);

	panel_hook.setup(interfaces::panel);
	panel_hook.hook_index( 41, paint_traverse);

	renderview_hook.setup(interfaces::render_view);
	renderview_hook.hook_index(9, scene_end);

	surface_hook.setup(interfaces::surface);
	surface_hook.hook_index(67, lock_cursor);

	wndproc_original = reinterpret_cast< WNDPROC >( SetWindowLongPtrA( FindWindow( "Valve001", NULL ), GWL_WNDPROC, reinterpret_cast< LONG >( wndproc ) ) );

	interfaces::console->get_convar("crosshair")->set_value(1);
	interfaces::console->get_convar("viewmodel_fov")->callbacks.set_size(false);
	interfaces::console->get_convar("viewmodel_offset_x")->callbacks.set_size(false);
	interfaces::console->get_convar("viewmodel_offset_y")->callbacks.set_size(false);
	interfaces::console->get_convar("viewmodel_offset_z")->callbacks.set_size(false);
	interfaces::console->get_convar("mat_postprocess_enable")->set_value(0);

	render::setup_fonts( );

	zgui::functions.draw_line = render::line;
	zgui::functions.draw_rect = render::rect;
	zgui::functions.draw_filled_rect = render::filled_rect;
	zgui::functions.draw_text = render::text;
	zgui::functions.get_text_size = render::get_text_size;
	zgui::functions.get_frametime = render::get_frametime;
}

void hooks::shutdown( ) {
	client_hook.release( );
	clientmode_hook.release( );
	panel_hook.release( );
	renderview_hook.release( );
	surface_hook.release();

	SetWindowLongPtrA( FindWindow( "Valve001", NULL ), GWL_WNDPROC, reinterpret_cast< LONG >( wndproc_original ) );
}

bool __stdcall hooks::create_move( float frame_time, c_usercmd* cmd ) {
	static auto original_fn = reinterpret_cast<create_move_fn>(clientmode_hook.get_original(24));
	auto local_player = reinterpret_cast<player_t*>(interfaces::entity_list->get_client_entity(interfaces::engine->get_local_player()));
	original_fn(interfaces::clientmode, frame_time, cmd);//Apparently a fix [pasted from https://github.com/designer1337/aristois-legit]
	
	if ( !cmd || !cmd->command_number )
		return false;

	if ( !interfaces::entity_list->get_client_entity( interfaces::engine->get_local_player( ) ) )
		return false;

	// misc
	movement.do_bhop(cmd);

	// clamping movement
	cmd->forward_move = std::clamp(cmd->forward_move, -450.0f, 450.0f);
	cmd->side_move = std::clamp(cmd->side_move, -450.0f, 450.0f);
	cmd->up_move = std::clamp(cmd->up_move, -320.f, 320.f);

	// clamping angles
	cmd->view_angles.x = std::clamp(cmd->view_angles.x, -89.0f, 89.0f);
	cmd->view_angles.y = std::clamp(cmd->view_angles.y, -180.0f, 180.0f);
	cmd->view_angles.z = 0.0f;

	return false;
}

float __stdcall hooks::viewmodel_fov() {
	auto local_player = reinterpret_cast<player_t*>(interfaces::entity_list->get_client_entity(interfaces::engine->get_local_player()));

	if (local_player && local_player->is_alive())
		return 68.f + set.misc.viewmodel_fov;
	else
		return 68.f;
}

void __stdcall hooks::frame_stage_notify( int frame_stage ) {
	reinterpret_cast< frame_stage_notify_fn >( client_hook.get_original( 37 ) )( interfaces::client, frame_stage );
}

void __stdcall hooks::paint_traverse( unsigned int panel, bool force_repaint, bool allow_force ) {
	std::string panel_name = interfaces::panel->get_panel_name( panel );

	reinterpret_cast< paint_traverse_fn >( panel_hook.get_original( 41 ) )( interfaces::panel, panel, force_repaint, allow_force );

	static unsigned int _panel = 0;
	static bool once = false;

	if ( !once ) {
		const char* panel_char = interfaces::panel->get_panel_name( panel );
		if ( strstr( panel_char, "MatSystemTopPanel" ) ) {
			_panel = panel;
			once = true;
		}
	}
	else if ( _panel == panel ) {
		g_menu.render( );
	}
}

void __stdcall hooks::scene_end( ) {
	reinterpret_cast< scene_end_fn >( renderview_hook.get_original( 9 ) )( interfaces::render_view );
}

void __stdcall hooks::lock_cursor() {
	static auto original_fn = reinterpret_cast<lock_cursor_fn>(surface_hook.get_original(67));

	if (set.menu.menu_opened) {
		interfaces::surface->unlock_cursor();
		return;
	}

	original_fn(interfaces::surface);
}

LRESULT __stdcall hooks::wndproc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
	static bool pressed = false;

	if (!pressed && GetAsyncKeyState(set.menu.menu_toggle_key)) {
		pressed = true;
	}
	else if (pressed && !GetAsyncKeyState(set.menu.menu_toggle_key)) {
		pressed = false;

		set.menu.menu_opened = !set.menu.menu_opened;
	}

	if (set.menu.menu_opened) {
		interfaces::inputsystem->enable_input(false);

	}
	else if (!set.menu.menu_opened) {
		interfaces::inputsystem->enable_input(true);
	}

	if (set.menu.menu_opened)
		return true;

	return CallWindowProcA(wndproc_original, hwnd, message, wparam, lparam);
}
