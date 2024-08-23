#define UNTIL(iterator_name, iterations_count) for(u32 iterator_name=0; iterator_name<iterations_count; iterator_name++)

#include "gaem.h"

int entry(int argc, char **argv) 
{
	u32 counter	= 0;
	window.title = STR("Minimal Game Example");
	window.scaled_width = 1280; // We need to set the scaled size if we want to handle system scaling (DPI)
	window.scaled_height = 720; 
	window.x = 200;
	window.y = 90;
	window.clear_color = v4(.5, .5, .5, 1);

	//TODO: for now everything will just shrink and stretch with the window
	Int2 virtual_screen_size = {.x = 320, .y= 180};
	f32 virtual_aspect_ratio = (f32)virtual_screen_size.x/virtual_screen_size.y;
	

	f32 target_dt = 1.0/60.0;
	f32 fixed_dt = target_dt;

	Gfx_Image* textures [TEX_LAST] = {0};

	{
		textures[TEX_BLANK] = load_image_from_disk(fixed_string("data/textures/blank.png"), get_heap_allocator());
		textures[TEX_PLAYER] = load_image_from_disk(fixed_string("data/textures/player.png"), get_heap_allocator());
		textures[TEX_STONE] = load_image_from_disk(fixed_string("data/textures/stone.png"), get_heap_allocator());
		textures[TEX_WAND] = load_image_from_disk(fixed_string("data/textures/wand.png"), get_heap_allocator());
		textures[TEX_PROJECTILE] = load_image_from_disk(fixed_string("data/textures/projectile.png"), get_heap_allocator());
		textures[TEX_SPELL_NULL] = load_image_from_disk(fixed_string("data/textures/spells/spell0000.png"), get_heap_allocator());
		textures[TEX_SPELL_PLACE_ITEM] = load_image_from_disk(fixed_string("data/textures/spells/spell0001.png"), get_heap_allocator());
		textures[TEX_SPELL_DESTROY_TILE] = load_image_from_disk(fixed_string("data/textures/spells/spell0002.png"), get_heap_allocator());
		textures[TEX_SPELL_PROJECTILE] = load_image_from_disk(fixed_string("data/textures/spells/spell0003.png"), get_heap_allocator());

		UNTIL(t, TEX_LAST)
		{
			assert(textures[t], "MISSING TEXTURE");
		}
	}
	
	Gfx_Font *default_font = load_font_from_disk(STR("C:/windows/fonts/arial.ttf"), get_heap_allocator());
	assert(default_font, "Failed loading arial.ttf");

	Memory_arena permanent_arena = {0};
	permanent_arena.size = MEGABYTES(500); 
	permanent_arena.data = (u8*)VirtualAlloc(0, permanent_arena.size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	
	if(!permanent_arena.data){
		if(GetLastError() == ERROR_INVALID_PARAMETER){
			assert(false);// probably compiling in x86 x32 bits and size too big
		}else{
			assert(false);
		}
	}
	
	Memory_arena temp_arena = {0};
	temp_arena.size = MEGABYTES(500); 
	temp_arena.data = (u8*)VirtualAlloc(0, temp_arena.size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	
	if(!temp_arena.data){
		if(GetLastError() == ERROR_INVALID_PARAMETER){
			assert(false);// probably compiling in x86 x32 bits and size too big
		}else{
			assert(false);
		}
	}

	// App_data* app = (App_data*)arena_push_size(&permanent_arena, )
	App_data* app = arena_push_struct(&permanent_arena, App_data);

	b8 is_initialized = 0;


	float64 last_time = os_get_elapsed_seconds();
	while (!window.should_close) 
	{
		// THIS IS (A LOT!!) FASTER THAN MANUALLY CLEARING THE ARENA (at least with /Od in msvc, haven't compared with /O2)
		//TODO: try memset()
		VirtualFree(temp_arena.data, 0, MEM_RELEASE);
		temp_arena.data = (u8*)VirtualAlloc(0, temp_arena.size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		temp_arena.used = 0;

		reset_temporary_storage();
		os_update(); 

		float window_aspect_ratio = (f32)window.width/(f32)window.height;
      V2 tiles_px_size = {8, 8};
		// V2 tiles_screen_size = size_in_pixels_to_screen(tiles_px_size, virtual_aspect_ratio, virtual_screen_size);

		draw_frame.projection = m4_make_orthographic_projection(
			-virtual_screen_size.x*0.5f, virtual_screen_size.x*0.5, 
			-virtual_screen_size.y*0.5f, virtual_screen_size.y*0.5f, 
			-1, 10
		);

		

		if(is_key_just_pressed(KEY_F5) && is_key_down(KEY_SHIFT))
		{
			window.should_close = true;
		}
		V2 cursor_screen_pos  = {
			.x = (virtual_screen_size.x * (input_frame.mouse_x/window.width - 0.5f)),
			.y = (virtual_screen_size.y * (input_frame.mouse_y/window.height - 0.5f))
		};
		V2 cursor_world_pos = v2_add(app->camera_pos, cursor_screen_pos);

		//:UPDATE

		if(!is_initialized || is_key_just_pressed('R'))
		{
			//:CREATE WORLD
			
			f32* noise_texture = arena_push_structs(&temp_arena, f32, WORLD_X_LENGTH*WORLD_Y_LENGTH);
			UNTIL(i, WORLD_X_LENGTH*WORLD_Y_LENGTH)
			{
				noise_texture[i] = get_random_float32_in_range(0.0f, 1.0f);
			}

			UNTIL(y, WORLD_Y_LENGTH)
			{
				UNTIL(x, WORLD_X_LENGTH)
				{
					f32 noise_value = sample_2d_perlin_noise(noise_texture, WORLD_X_LENGTH, WORLD_Y_LENGTH, x, y, 1, WORLD_X_LENGTH/5, .75f);
					{
						// u32 types_count = 2; 
						// app->world[y][x] = (u8)((u8)(noise_value*types_count)*(256/types_count));
						
						u32 center_x = WORLD_X_LENGTH/2;
						u32 center_y = WORLD_Y_LENGTH/2;

						if(noise_value < .5f || (center_x-10 < x && x < center_x+10 && center_y-10 < y && y < center_y+10)){
							app->world[y][x] = 0;
						}else{
							app->world[y][x] = 1;
						}
					}
				}
			}
		}		

		if(!is_initialized)
		{
			is_initialized = true;
			//TODO: i think this is not necessary anymore but need to check
			app->ui.selection = NULL_UI_SELECTION;

			app->item_id_to_tex_uid[ITEM_STONE] = TEX_STONE;
			app->item_id_to_tex_uid[ITEM_WAND] = TEX_WAND;
			app->item_id_to_tex_uid[ITEM_SPELL_NULL] = TEX_SPELL_NULL;
			app->item_id_to_tex_uid[ITEM_SPELL_DESTROY] = TEX_SPELL_DESTROY_TILE;
			app->item_id_to_tex_uid[ITEM_SPELL_PROJECTILE] = TEX_SPELL_PROJECTILE;

			//:INITIALIZE STATIC ITEMS
			UNTIL(id, ITEM_LAST_ID)
			{
				app->items[id].item_id = (Item_id)id;
				app->used_items[id] = true;
				app->items[id].casting_cd = .5f;
				if(id)
					app->items[id].item_count = 1;

				switch(id)
				{
					case ITEM_NULL:
					case ITEM_PLACEABLE_FIRST:
					case ITEM_PLACEABLE_LAST:
					case ITEM_LAST_ID:
					case ITEM_SPELL_LAST: 
					break;
					
					case ITEM_STONE: 
					{
					}
					break;
					case ITEM_SPELL_NULL: 
					case ITEM_SPELL_DESTROY: 
					case ITEM_SPELL_PROJECTILE:
					{
						// this is to make them not stackable
						app->items[id].inventory.is_editable = true;
					}
					break;
					case ITEM_WAND: 
					{
						app->items[id].inventory.is_editable = true;
						app->items[id].inventory.size = 5;
						app->items[id].casting_cd = .5f;
						// TODO: this should be a dynamic inventory
						// app->items[id].inventory.is_editable = true;
					}
					break;
					
					default: assert(false, "unhandled item_id");
					break;
				}
      	}

			//:INITIALIZE PLAYER
			
			app->used_entities[E_PLAYER_INDEX] = true;
			app->entities[E_PLAYER_INDEX].pos = v2(0, 0);
			app->entities[E_PLAYER_INDEX].flags = E_RENDER;
			app->entities[E_PLAYER_INDEX].unarmed_inventory = get_first_available_index(app->used_items, MAX_ITEMS);
			app->entities[E_PLAYER_INDEX].already_casted = true;
			app->entities[E_PLAYER_INDEX].tex_uid = TEX_PLAYER;
			app->items[app->entities[E_PLAYER_INDEX].unarmed_inventory].inventory.is_editable = true;
			app->items[app->entities[E_PLAYER_INDEX].unarmed_inventory].inventory.size = 2;
			app->items[app->entities[E_PLAYER_INDEX].unarmed_inventory].inventory.items[0] = ITEM_SPELL_DESTROY;
			app->items[app->entities[E_PLAYER_INDEX].unarmed_inventory].inventory.items[1] = 0;
			app->items[app->entities[E_PLAYER_INDEX].unarmed_inventory].casting_cd = .5f;
			app->items[app->entities[E_PLAYER_INDEX].unarmed_inventory].not_cycle_when_casting = true;

		}
		if(is_key_just_pressed('T'))
		{
			is_initialized = false;
		}

		int delta_wheel = 0;
		UNTIL(i, input_frame.number_of_events)
		{
			if(input_frame.events[i].kind == INPUT_EVENT_SCROLL)
			{
				delta_wheel = input_frame.events[i].yscroll;
			}
		}
		
		#define INVENTORY_ROWS_LENGTH 20
		
		if(delta_wheel)
		{
			u16 current_item = app->entities[E_PLAYER_INDEX].inventory.items[app->entities[E_PLAYER_INDEX].inventory.selected_slot];
			f32 item_cd = app->items[current_item].casting_cd;
			while(app->items[current_item].inventory.size)
			{
				u16 next_item = app->items[current_item].inventory.items[app->items[current_item].inventory.selected_slot];
				app->items[current_item].inventory.selected_slot = 0;
				current_item = next_item;
			}
   		app->entities[E_PLAYER_INDEX].inventory.selected_slot = (app->entities[E_PLAYER_INDEX].inventory.selected_slot+INVENTORY_ROWS_LENGTH-delta_wheel)%INVENTORY_ROWS_LENGTH;
			app->entities[E_PLAYER_INDEX].casting_cd = item_cd;

			current_item = app->entities[E_PLAYER_INDEX].inventory.items[app->entities[E_PLAYER_INDEX].inventory.selected_slot];
			while(app->items[current_item].inventory.size)
			{
				u16 next_item = app->items[current_item].inventory.items[app->items[current_item].inventory.selected_slot];
				app->items[current_item].inventory.selected_slot = 0;
				current_item = next_item;
			}
		}

		
		// if(is_key_just_pressed(KEY_TAB))
		// {
		// 	app->is_menu_opened = !app->is_menu_opened;
		// }
		app->is_menu_opened = true;

		V2 input_direction = {
			(f32)((is_key_down('D') > 0) - (is_key_down('A') > 0)),
			(f32)((is_key_down('W') > 0) - (is_key_down('S') > 0))
		};
		V2 normalized_input_direction = v2_normalize(input_direction);

		//:UI
		b8 is_mouse_in_ui = 0;

		app->ui.current_widget_uid = 0;
		app->ui.selection.hot = NULL_INDEX16;
		

		UNTIL(i, MAX_UI_WIDGETS)
		{
			if(!(app->ui.widgets[i].flags & UI_SKIP_RENDERING))
			{
				Rect_float widget_rect = {.pos = app->ui.global_positions[i], .size = app->ui.widgets[i].style.size};
				if(point_vs_rect_float(cursor_screen_pos, widget_rect))
				{
					if(app->ui.widgets[i].flags & UI_CLICKABLE)
					{
						app->ui.selection.hot = (u16)i;
					}
					is_mouse_in_ui = true;
				}
			}

			ZERO_STRUCT(app->ui.widgets[i]);
		}

		app->ui.selection.clicked = NULL_INDEX16;
		app->ui.selection.clicked2 = NULL_INDEX16;

		if(is_key_just_pressed(MOUSE_BUTTON_LEFT)){
			app->ui.selection.pressed = app->ui.selection.hot;
		}else if(is_key_just_released(MOUSE_BUTTON_LEFT)){
			if(app->ui.selection.pressed == app->ui.selection.hot){
				app->ui.selection.clicked = app->ui.selection.pressed;
			}
			app->ui.selection.pressed = NULL_INDEX16;
		}
		
		if(is_key_just_pressed(MOUSE_BUTTON_RIGHT)){
			app->ui.selection.pressed2 = app->ui.selection.hot;
		}else if(is_key_just_released(MOUSE_BUTTON_LEFT)){
			if(app->ui.selection.pressed2 == app->ui.selection.hot){
				app->ui.selection.clicked2 = app->ui.selection.pressed2;
			}
			app->ui.selection.pressed2 = NULL_INDEX16;
		}

		//:UPDATING UI

		//:ROOT WIDGET
		ui_push(&app->ui, do_widget(&app->ui, UI_SKIP_RENDERING, STYLE_NULL));
		{
			u32 rows = 1;
			u32 visible_slots_count = INVENTORY_ROWS_LENGTH;

			if(app->is_menu_opened)
			{
				rows = (INVENTORY_SIZE+INVENTORY_ROWS_LENGTH-1)/INVENTORY_ROWS_LENGTH;
				visible_slots_count = INVENTORY_SIZE;
			}

			f32 slots_size = 15;
			
			Ui_style test_style = {0};
			test_style.layout = UI_LAYOUT_GRID_ROW_MAJOR;
			test_style.line_size = INVENTORY_ROWS_LENGTH;
			test_style.cells_border_size = 1.0f;
			test_style.size = (V2){slots_size*INVENTORY_ROWS_LENGTH, slots_size*rows};
			test_style.pos = (V2){-test_style.size.x/2, (virtual_screen_size.y/2)-1 - slots_size*rows};
			test_style.color_rect = (Color){.8f,.8f,.8f,1};
			ui_push(&app->ui, do_widget(&app->ui, 0, test_style));

			UNTIL(i, INVENTORY_SIZE)
			{
				test_style = STYLE_NULL;
				if(i >= visible_slots_count)
				{
					ui_pop(&app->ui);
					do_widget(&app->ui, UI_SKIP_RENDERING, test_style);
					do_widget(&app->ui, UI_SKIP_RENDERING, test_style);
				}
				else
				{

					test_style.color_rect = (Color){.5f, .5f, .5f, 1};
					if(app->entities[E_PLAYER_INDEX].inventory.selected_slot == (s32)i){
						test_style.color_rect = (Color){1, 1, 1, test_style.color_rect.a};
					}
					
					test_style.tex_uid = app->item_id_to_tex_uid[app->items[app->entities[E_PLAYER_INDEX].inventory.items[i]].item_id];
					u16 slot_uid = ui_push(&app->ui, do_widget(&app->ui, UI_CLICKABLE, test_style));
					{
						if(app->ui.selection.clicked == slot_uid)
						{
							u16 current_item = app->entities[E_PLAYER_INDEX].inventory.items[i];
							if(app->is_menu_opened)
							{								
								if(app->items[app->cursor_item].item_id == app->items[current_item].item_id
								&& !app->items[current_item].inventory.is_editable)
								{
									app->items[current_item].item_count += app->items[app->cursor_item].item_count;
									if(app->items[current_item].item_count > MAX_ITEM_STACK)
									{
										int rest = app->items[current_item].item_count - MAX_ITEM_STACK;
										app->items[app->cursor_item].item_count = rest;
										app->items[current_item].item_count = MAX_ITEM_STACK;
									}
									else
									{
										app->used_items[app->cursor_item] = false;
										ZERO_STRUCT(app->items[app->cursor_item]);
										app->cursor_item = 0;
									}
								}
								else
								{
									u16 temp_item = app->cursor_item;
									app->cursor_item = app->entities[E_PLAYER_INDEX].inventory.items[i];
									app->entities[E_PLAYER_INDEX].inventory.items[i] = temp_item;
								}
							}
							else
							{
								assert(i < 0xff);
								app->entities[E_PLAYER_INDEX].inventory.selected_slot = (u8)i;
							}
						}
						
						if(app->items[app->entities[E_PLAYER_INDEX].inventory.items[i]].item_count > 1)
						{
							
							test_style.text = u32_to_string(app->items[app->entities[E_PLAYER_INDEX].inventory.items[i]].item_count, &temp_arena);
						}
						do_widget(&app->ui, UI_SKIP_RENDERING, test_style);

					}
					ui_pop(&app->ui);
				}
			}
			ui_pop(&app->ui);


			//:UI SPELLS INVENTORY
			u16 equipped_item = get_entity_equipped_item_index(app, E_PLAYER_INDEX);
			u16 current_item = equipped_item;

			#define MAX_CASTING_RECURSION 1000
			u16 casting_stack [MAX_CASTING_RECURSION] = {0};
			casting_stack[0] = current_item;
			u16 current_recursion = 0;

			f32 current_y_pos = -virtual_screen_size.y/2;
			while(app->items[current_item].inventory.size > 0)
			{	
				u32 spells_ui_flags = 0;
				if(!app->items[current_item].inventory.size) 
				{
					spells_ui_flags = UI_SKIP_RENDERING;
				}

				test_style = STYLE_NULL;
				test_style.layout = UI_LAYOUT_GRID_ROW_MAJOR;
				test_style.line_size = INVENTORY_ROWS_LENGTH;
				test_style.line_size = app->items[current_item].inventory.size;
				test_style.cells_border_size = 1.0f;
				test_style.size = (V2){slots_size*app->items[current_item].inventory.size, slots_size};
				test_style.color_rect = (Color){.8f,.8f,.8f,1};
				test_style.pos = (V2){(virtual_screen_size.x/2)-test_style.size.x - 1, current_y_pos};
				

				ui_push(&app->ui, do_widget(&app->ui, spells_ui_flags, test_style));
				{
					//TODO: instead of inventory.size use a constant value to keep uid's universal
					UNTIL(spell, app->items[current_item].inventory.size)
					{
						Ui_style temp_style = {0};
						u16 spell_item_uid = app->items[current_item].inventory.items[spell];
						temp_style.tex_uid = app->item_id_to_tex_uid[app->items[spell_item_uid].item_id];
						temp_style.color_rect = (Color){.3f,.3f,.3f,1};
						// if(app->items[app->items[current_item].inventory[spell]].item_count > 1)
						// {
						//    test_style.text = u32_to_string(app->items[current_item].inventory[spell].item_count, memory->temp_arena);
						// }
						if(app->items[current_item].inventory.selected_slot == spell)
						{
							temp_style.color_rect = (Color){1,1,1,1};
						}
						if(app->items[spell_item_uid].item_count > 1)
						{
							temp_style.text = u32_to_string(app->items[spell_item_uid].item_count, &temp_arena);
						}
						if(app->ui.selection.clicked == do_widget(&app->ui, UI_CLICKABLE, temp_style) && app->items[current_item].inventory.is_editable)
						{
							if(app->is_menu_opened && current_item == equipped_item)
							{
								if(app->items[app->cursor_item].item_id == app->items[spell_item_uid].item_id
								&& !app->items[spell_item_uid].inventory.is_editable)
								{
									app->items[spell_item_uid].item_count += app->items[app->cursor_item].item_count;
									if(app->items[spell_item_uid].item_count > MAX_ITEM_STACK)
									{
										int rest = app->items[spell_item_uid].item_count - MAX_ITEM_STACK;
										app->items[app->cursor_item].item_count = rest;
										app->items[spell_item_uid].item_count = MAX_ITEM_STACK;
									}
									else
									{
										app->used_items[app->cursor_item] = false;
										ZERO_STRUCT(app->items[app->cursor_item]);
										app->cursor_item = 0;
									}
								}
								else
								{
									u16 temp_item = app->cursor_item;
									app->cursor_item = spell_item_uid;
									app->items[current_item].inventory.items[spell] = temp_item;
								}
							}
						}
					}
				}ui_pop(&app->ui);

				current_y_pos += test_style.size.y;
				
				current_item = app->items[current_item].inventory.items[app->items[current_item].inventory.selected_slot];
				current_recursion++;
				casting_stack[current_recursion] = current_item;
			}
			

			//:CURSOR ITEM SLOT
			test_style = STYLE_NULL;
			u32 flag = 0;
			if(!(app->is_menu_opened && app->cursor_item))
			{
				flag = UI_SKIP_RENDERING;
			}
			test_style.tex_uid = app->item_id_to_tex_uid[app->items[app->cursor_item].item_id];
			test_style.color_rect = (Color){.99f,.99f,.99f,1};
			test_style.size = (V2){slots_size, slots_size};
			test_style.pos = v2_sub(cursor_screen_pos, v2(.75f*test_style.size.x, .25f*test_style.size.y));
			if(app->cursor_item != NULL_INDEX16 && app->items[app->cursor_item].item_count > 1)
			{
				test_style.text = u32_to_string(app->items[app->cursor_item].item_count, &temp_arena);
			}
			do_widget(&app->ui, flag, test_style);
		}
		ui_pop(&app->ui);


		
		// BUILDING UI LAYOUTS

		UNTIL(i, MAX_UI_WIDGETS)
		{
			u16 parent_uid = app->ui.widgets[i].parent_uid;
			if(app->ui.widgets[parent_uid].style.layout == UI_LAYOUT_DEFAULT)
			{
				app->ui.global_positions[i] = v2_add(app->ui.widgets[i].style.pos, app->ui.global_positions[parent_uid]);
			}
			else if(app->ui.widgets[parent_uid].style.layout == UI_LAYOUT_GRID_ROW_MAJOR)
			{
				f32 border_size = app->ui.widgets[parent_uid].style.cells_border_size;
				int x_index = app->ui.widgets[i].local_uid % app->ui.widgets[parent_uid].style.line_size;
				f32 full_x_size = (app->ui.widgets[parent_uid].style.size.x / app->ui.widgets[parent_uid].style.line_size);
				f32 x_cell_size = full_x_size - (border_size*2);
				f32 x_pos = x_index * full_x_size + (border_size);
				
				int y_index = app->ui.widgets[i].local_uid / app->ui.widgets[parent_uid].style.line_size;
				f32 y_pos = y_index * full_x_size + (border_size);


				V2 parent_upper_left_corner = 
				{
					app->ui.global_positions[parent_uid].x, 
					app->ui.global_positions[parent_uid].y + app->ui.widgets[parent_uid].style.size.y - x_cell_size
				};
				app->ui.global_positions[i] = v2_add(parent_upper_left_corner, v2(x_pos, -y_pos));
				app->ui.widgets[i].style.size = v2(x_cell_size, x_cell_size);
			}
		}

		//:UI END

		Int2 cursor_tile_pos;
		if(!is_mouse_in_ui){
			cursor_tile_pos = world_pos_to_tile_pos(cursor_world_pos, tiles_px_size);
		}else{
			cursor_tile_pos = (Int2){NULL_INDEX16, NULL_INDEX16};
		}

		
		{
			u16 equipped_item = get_entity_equipped_item_index(app, E_PLAYER_INDEX);

			app->entities[E_PLAYER_INDEX].target_direction = v2_normalize(v2_sub(cursor_world_pos, app->entities[E_PLAYER_INDEX].pos));
			if(!is_mouse_in_ui)
			{
				if(is_key_down(MOUSE_BUTTON_LEFT))
				{
					app->entities[E_PLAYER_INDEX].casting_state = CASTING_LEFT;
				}
				if(is_key_down(MOUSE_BUTTON_RIGHT))
				{
					app->entities[E_PLAYER_INDEX].casting_state = CASTING_RIGHT;
					app->items[equipped_item].inventory.selected_slot = app->items[equipped_item].inventory.size - 1;
				}

				#if DEV_TESTING
					if(is_key_just_pressed(KEY_F1) == 1)
					{
						u16 debug_wand_pickup_uid = get_first_available_index(app->used_entities, MAX_ENTITIES);
						app->entities[debug_wand_pickup_uid].flags = E_RENDER|E_PICKUP;
						u16 debug_wand_item_uid = get_first_available_index(app->used_items, MAX_ITEMS);
						app->entities[debug_wand_pickup_uid].item = debug_wand_item_uid;
						app->items[debug_wand_item_uid] = app->items[ITEM_WAND];

						app->entities[debug_wand_pickup_uid].pos = cursor_world_pos;
					}
					if(is_key_just_pressed(KEY_F2) == 1)
					{
						u16 debug_spell = get_first_available_index(app->used_entities, MAX_ENTITIES);
						app->entities[debug_spell].flags = E_RENDER|E_PICKUP;
						u16 debug_wand_item_uid = get_first_available_index(app->used_items, MAX_ITEMS);
						app->entities[debug_spell].item = debug_wand_item_uid;
						app->items[debug_wand_item_uid] = app->items[ITEM_SPELL_DESTROY];

						app->entities[debug_spell].pos = cursor_world_pos;
					}
					if(is_key_just_pressed(KEY_F3) == 1)
					{
						u16 debug_spell = get_first_available_index(app->used_entities, MAX_ENTITIES);
						app->entities[debug_spell].flags = E_RENDER|E_PICKUP;
						u16 debug_wand_item_uid = get_first_available_index(app->used_items, MAX_ITEMS);
						app->entities[debug_spell].item = debug_wand_item_uid;
						app->items[debug_wand_item_uid] = app->items[ITEM_SPELL_PROJECTILE];

						app->entities[debug_spell].pos = cursor_world_pos;
					}
				#endif
			}
		}

		app->entities[E_PLAYER_INDEX].move_direction = normalized_input_direction;
		app->entities[E_PLAYER_INDEX].movement_speed = 40;
		if(is_key_down(KEY_SHIFT) > 0)
		{
			app->entities[E_PLAYER_INDEX].movement_speed = 200.0f;
		}
		
		//:UPDATE ENTITIES
		#define DEFAULT_RANGE (2*tiles_px_size.x)
		
		UNTIL(e, MAX_ENTITIES)
		{
			// f32 x_factor = 160.0f/90;e2_normalize(cursor_world_pos - app->entities[E_PLAYER_INDEX].pos.v2);
			if(app->used_entities[e])
			{
				//TODO: this will be moving across the spells inventory
				u16 equipped_item = get_entity_equipped_item_index(app, e);
				u16 casting_item = equipped_item;
				u16* casting_item_parent_inventory_slot;
				if(equipped_item != app->entities[e].unarmed_inventory){
					casting_item_parent_inventory_slot = &app->entities[e].inventory.items[app->entities[e].inventory.selected_slot];
				}else{
					casting_item_parent_inventory_slot = &app->items[equipped_item].inventory.items[app->items[equipped_item].inventory.selected_slot];
				}

				u16 casting_stack [MAX_CASTING_RECURSION] = {0};
				casting_stack[0] = casting_item;
				u16 current_recursion = 0;
				
				u16 casting_spell = app->items[casting_item].inventory.items[app->items[casting_item].inventory.selected_slot];
				while(app->items[casting_item].inventory.size > 0 && casting_spell)
				{
					casting_item = casting_spell;
					casting_spell = app->items[casting_item].inventory.items[app->items[casting_item].inventory.selected_slot];
					casting_item_parent_inventory_slot = &app->items[casting_item].inventory.items[app->items[casting_item].inventory.selected_slot];
					
					current_recursion++;
					casting_stack[current_recursion] = casting_item;
				}


				if(app->entities[e].casting_state != CASTING_NULL 
				&& !app->entities[e].already_casted)
				{
					app->entities[e].already_casted = true;

					// Item_id spell = app->items[app->items[casting_item].inventory.items[app->items[casting_item].inventory.selected_slot]].item_id;

					if(is_placeable_item(app->items[casting_item].item_id))
					{
						V2 placing_world_pos = v2_add(app->entities[e].pos, v2_mulf(app->entities[e].target_direction, DEFAULT_RANGE));
						Int2 placing_tilemap_pos = world_pos_to_tile_pos(placing_world_pos, tiles_px_size);

						if(!app->world[placing_tilemap_pos.y][placing_tilemap_pos.x])
						{
							//TODO: placeable objects will have their own tile_uid
							app->world[placing_tilemap_pos.y][placing_tilemap_pos.x] = 1;
							app->items[casting_item].item_count -= 1;

							if(app->items[casting_item].item_count == 0)
							{
								app->used_items[casting_item] = 0;
								ZERO_STRUCT(app->items[casting_item]);
								*casting_item_parent_inventory_slot = 0;
							}
						}
					}
					else
					{
						Item_id spell = app->items[casting_item].item_id;
						switch(spell)
						{
							case ITEM_SPELL_NULL:
							break;
							case ITEM_SPELL_DESTROY:
							{
								//TODO: axis aligned 2d ray marching
								V2 current_pos = app->entities[e].pos;
								UNTIL(step, 10)
								{
									V2 test_world_pos = v2_add(current_pos , v2_mulf(app->entities[e].target_direction, (step*DEFAULT_RANGE/10)));
									Int2 test_tilemap_pos = world_pos_to_tile_pos(test_world_pos, tiles_px_size);
									if(app->world[test_tilemap_pos.y][test_tilemap_pos.x])
									{
										destroy_tile(app, test_tilemap_pos, tiles_px_size);
										break;
									}
								}
							}
							break;
							default:
							break;
						}
					}
				}

				//:UPDATING CASTING SPELL

				if(!app->items[casting_item].not_cycle_when_casting // do cycle when casting
				|| (app->entities[e].already_casted))
				{
					app->entities[e].casting_cd -= fixed_dt;
					
					if(app->entities[e].casting_cd <= 0)
					{
						app->entities[e].casting_cd = app->items[casting_item].casting_cd;

						if(app->entities[e].casting_state != CASTING_RIGHT)
						{
							if(app->items[casting_item].inventory.size != 0 && !app->items[casting_item].not_cycle_when_casting)
							{
								app->items[casting_item].inventory.selected_slot = (app->items[casting_item].inventory.selected_slot+1)%app->items[casting_item].inventory.size;
								u16 new_selected_slot = app->items[casting_item].inventory.selected_slot;
								app->items[app->items[casting_item].inventory.items[new_selected_slot]].inventory.selected_slot = 0;
							}

							//:UPDATING SELECTED SLOT
							while(app->items[casting_item].inventory.selected_slot == 0 && casting_item != equipped_item)
							{
								assert(current_recursion > 0);
								current_recursion--;
								casting_item = casting_stack[current_recursion];
								if(app->items[casting_item].inventory.size != 0 && !app->items[casting_item].not_cycle_when_casting)
								{
									app->items[casting_item].inventory.selected_slot = (app->items[casting_item].inventory.selected_slot+1)%app->items[casting_item].inventory.size;
									app->items[app->items[casting_item].inventory.items[app->items[casting_item].inventory.selected_slot]].inventory.selected_slot = 0;
								}
							}
						}
						if(equipped_item == app->entities[e].unarmed_inventory && app->entities[e].casting_state != CASTING_RIGHT)
						{
							app->items[equipped_item].inventory.selected_slot = 0;
						}

						app->entities[e].already_casted = false;
					}
				}
				app->entities[e].casting_state = CASTING_NULL;
				
				UNTIL(e2, MAX_ENTITIES)
				{
					if(app->used_entities[e2] && e != e2)
					{
						//:PICKUP
						if(app->entities[e].flags & E_PICKUP && !(app->entities[e2].flags & E_PICKUP))
						{
							
							f32 distance = v2_length(v2_sub(app->entities[e2].pos, app->entities[e].pos));
							if(distance < tiles_px_size.x)
							{
								u16 first_empty_slot = NULL_INDEX16;
								u16* already_found_stack_item = 0;
								u16 total_count = 0;
								for(u16 slot = INVENTORY_SIZE-1; slot < INVENTORY_SIZE; slot--)
								{
									u16 e2_item_uid = app->entities[e2].inventory.items[slot];

									if(!app->items[app->entities[e].item].inventory.is_editable // if it's editable, it's not stackable
									&& app->items[app->entities[e].item].item_id == app->items[e2_item_uid].item_id
									&& app->items[app->entities[e2].inventory.items[slot]].item_count < MAX_ITEM_STACK)
									{
										already_found_stack_item = &app->entities[e2].inventory.items[slot];
										break;
									}
									if(first_empty_slot == NULL_INDEX16 && app->items[app->entities[e2].inventory.items[slot]].item_id == ITEM_NULL)
									{
										first_empty_slot = (u16)slot;
									}
								}
								if(!already_found_stack_item)
								{// CHECK UNARMED INVENTORY
									UNTIL(slot, app->items[app->entities[e2].unarmed_inventory].inventory.size)
									{
										u16 e2_item_uid = app->items[app->entities[e2].unarmed_inventory].inventory.items[slot];

										if(!app->items[app->entities[e].item].inventory.is_editable // if it's editable, it's not stackable
										&& app->items[app->entities[e].item].item_id == app->items[e2_item_uid].item_id
										&& app->items[app->entities[e2].inventory.items[slot]].item_count < MAX_ITEM_STACK)
										{
											already_found_stack_item = &app->items[app->entities[e2].unarmed_inventory].inventory.items[slot];
											break;
										}
										if(first_empty_slot == NULL_INDEX16 && app->items[app->entities[e2].inventory.items[slot]].item_id == ITEM_NULL)
										{
											first_empty_slot = (u16)slot;
										}
									}
								}
								if(already_found_stack_item)
								{
									u16 e2_item_uid = *already_found_stack_item;
									total_count = app->items[app->entities[e].item].item_count + app->items[e2_item_uid].item_count;

									app->items[e2_item_uid].item_count = min(total_count, MAX_ITEM_STACK);
									
									app->used_items[app->entities[e].item] = 0;
									ZERO_STRUCT(app->items[app->entities[e].item]);
								}
								if(!total_count)
								{
									total_count = app->items[app->entities[e].item].item_count;
									app->entities[e2].inventory.items[first_empty_slot] = app->entities[e].item;
									app->items[app->entities[e].item].item_count = min(total_count, MAX_ITEM_STACK);

									app->entities[e].item = 0;
									if(total_count > MAX_ITEMS)
									{
										u16 new_item_uid = get_first_available_index(app->used_items, MAX_ITEMS);
										app->entities[e].item = new_item_uid;
										app->items[new_item_uid] =  app->items[app->entities[e2].inventory.items[first_empty_slot]];
										app->items[new_item_uid].item_count = total_count - MAX_ITEM_STACK;
									}
								}

								if(total_count < MAX_ITEMS)
								{
									// DESTROY ENTITY
									app->used_entities[e] = 0;
									ZERO_STRUCT(app->entities[e]);
								}
							}
						}
					}
				}


				//:UPDATE ENTITY POSITIONS

				V2 new_pos = v2_add(app->entities[e].pos, (v2_mulf(app->entities[e].move_direction, app->entities[e].movement_speed*fixed_dt)));
				Int2 tile_pos = world_pos_to_tile_pos(new_pos, tiles_px_size);
				
				if(!app->world[tile_pos.y][tile_pos.x]){
					app->entities[e].pos = new_pos;
				}
			}
		}

		app->camera_pos = app->entities[E_PLAYER_INDEX].pos;
		

		//:RENDER

		draw_frame.camera_xform = m4_translate(m4_scalar(1.0f), v3(app->camera_pos.x, app->camera_pos.y, 0));

		//:RENDERING TILEMAP

			
		Int2 camera_tile_pos = world_pos_to_tile_pos(app->camera_pos, tiles_px_size);

		Int2 render_rect_size = {42,26};

		Int2 render_rect_min, render_rect_max;
		render_rect_min.x = clamp(camera_tile_pos.x - render_rect_size.x/2, 0, WORLD_X_LENGTH-render_rect_size.x);
		render_rect_min.y = clamp(camera_tile_pos.y - render_rect_size.y/2, 0, WORLD_Y_LENGTH-render_rect_size.x);

		render_rect_max.x = clamp(camera_tile_pos.x + render_rect_size.x/2, render_rect_size.x, WORLD_X_LENGTH);
		render_rect_max.y = clamp(camera_tile_pos.y + render_rect_size.y/2, render_rect_size.y, WORLD_Y_LENGTH);
		
		{
			V2 placing_world_pos = v2_add(app->entities[E_PLAYER_INDEX].pos, v2_mulf(app->entities[E_PLAYER_INDEX].target_direction, DEFAULT_RANGE));
			Int2 placing_tile_pos = world_pos_to_tile_pos(placing_world_pos, tiles_px_size);
			b8 is_holding_placeable = app->items[app->entities[E_PLAYER_INDEX].inventory.items[app->entities[E_PLAYER_INDEX].inventory.selected_slot]].item_id == ITEM_STONE;

			for(int y=render_rect_max.y-1; y >= render_rect_min.y; y--)
			{
				f32 y_final_pos = ((f32)(y - WORLD_Y_LENGTH/2)*tiles_px_size.y);
				for(int x = render_rect_min.x; x < render_rect_max.x; x++)
				{
					f32 x_final_pos = ((f32)(x - WORLD_X_LENGTH/2)*tiles_px_size.x);
					{
						if(app->world[y][x] || (is_holding_placeable && x==placing_tile_pos.x && y==placing_tile_pos.y))
						{
							Color tile_color = {1,1,1,1};
							// if(x==cursor_tile_pos.x && y==cursor_tile_pos.y){
							if(x==placing_tile_pos.x && y==placing_tile_pos.y){
								tile_color = v4(1,1,0,1);
							}
							draw_image(textures[TEX_STONE], v2(x_final_pos, y_final_pos), v2(textures[TEX_STONE]->width, textures[TEX_STONE]->height), tile_color);
						}
					}
				}
			}
		}

		//:RENDERING ENTITIES
		
		UNTIL(e, MAX_ENTITIES)
		{
			if(app->entities[e].flags & E_RENDER)
			{
				u16 tex_uid;
				if(app->entities[e].flags & E_PICKUP){
					tex_uid = app->item_id_to_tex_uid[app->items[app->entities[e].item].item_id];
				}else{
					tex_uid = app->entities[e].tex_uid;
				}
				
				V2 image_screen_size = {(f32)textures[tex_uid]->width, (f32)textures[tex_uid]->height};
				
				if(app->entities[e].flags & E_PICKUP)
				{
					image_screen_size = v2_mulf(image_screen_size,0.5f);
				}
				V2 image_pos = {
					.x = app->entities[e].pos.x - image_screen_size.x/2,
					.y = app->entities[e].pos.y - image_screen_size.y/2
				};
				draw_image(textures[tex_uid], image_pos, image_screen_size, v4(1,1,1,1) );
			}
		}

		//:RENDERING UI

		draw_frame.camera_xform = m4_scalar(1.0f);
      UNTIL(i, app->ui.current_widget_uid)
      {
         if(app->ui.widgets[i].flags != UI_SKIP_RENDERING)
         {
            
            Color temp_color = app->ui.widgets[i].style.color_rect;
            if(app->ui.selection.pressed == i){
               temp_color = (Color){.9f, .5f, 0, temp_color.a};
            }else if(app->ui.selection.hot == i){
               temp_color = (Color){1,0,1, temp_color.a};
            }
				draw_image(textures[app->ui.widgets[i].style.tex_uid], 
					app->ui.global_positions[i], 
					app->ui.widgets[i].style.size, 
					temp_color
				);
         }

			//:RENDERING UI TEXT
         //TODO: extract this to instancing once i solve the zpos thing
         if(app->ui.widgets[i].style.text.count && app->ui.widgets[i].style.text.data)
         {
            f32 current_x_pos = 0;
            UNTIL(c, app->ui.widgets[i].style.text.count)
            {
               
               u8 character = app->ui.widgets[i].style.text.data[c];
               // request->object3d.texinfo_uid = default_font->texinfo_uids[character-default_font->first_char];
               
               // Tex_info* texinfo;
               // LIST_GET(memory->tex_infos, request->object3d.texinfo_uid, texinfo);
					draw_text(default_font, app->ui.widgets[i].style.text, 20, 
						app->ui.global_positions[i], 
						(V2){0.5f, 0.5f},
						v4(1,1,1,1)
					);
            }
         }
      }
		
		// draw_text(default_font, fixed_string("i am text"), 20, v2(0,0), v2(0.01f, 0.01f), v4(1,1,1,1));

		gfx_update();


		float64 now = os_get_elapsed_seconds();
		f32 dt = now - last_time;
		os_high_precision_sleep((target_dt - dt) * 1000.0);
		last_time = os_get_elapsed_seconds();
	}

	return 0;
}