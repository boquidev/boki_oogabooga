#define DEV_TESTING 1

enum Texture_id
{
   TEX_BLANK,
	TEX_PLAYER,
   TEX_STONE,
   TEX_WAND,
	TEX_PROJECTILE,
   TEX_SPELL_NULL,
   TEX_SPELL_PLACE_ITEM,
   TEX_SPELL_DESTROY_TILE,
	TEX_SPELL_PROJECTILE,

	TEX_LAST
};

#define NULL_INDEX16 0xffff
#define NULL_INDEX32 0xffffffff

#define KILOBYTES(value) ((value) * 1024LL)
#define MEGABYTES(value) (KILOBYTES(value) * 1024LL)
#define GIGABYTES(value) (MEGABYTES(value) * 1024LL)
#define TERABYTES(value) (GIGABYTES(value) * 1024LL)
#define ZERO_STRUCT(struc) memset(&(struc), 0, sizeof(struc))


typedef struct Memory_arena
{
   union{
	   void* data;
      u8* bytes;
   };
	size_t used;
	size_t size;
}Memory_arena;

void* arena_push_size(Memory_arena* arena, size_t size)
{
   void* result = arena->bytes+arena->used;
   arena->used += size;
   assert(arena->used < arena->size);
   return result;
}
#define arena_push_struct(arena, struct_type) (struct_type*)arena_push_size(arena, sizeof(struct_type))
#define arena_push_structs(arena, struct_type, struct_count) (struct_type*)arena_push_size(arena, struct_count*sizeof(struct_type))

typedef char b8;

typedef struct Int2
{
	int x, y;
}Int2;


typedef Vector2 V2;
typedef Vector3 V3;
typedef Vector4 V4;
typedef Vector4 Color;

typedef union Rect_float
{
	struct{
		f32 x, y, w, h;
	};
	struct{
		V2 pos, size;
	};
}Rect_float;

b8 point_vs_rect_float(V2 p, Rect_float rect)
{
	b8 x_inside = rect.pos.x <=  p.x && p.x  < rect.pos.x + rect.size.x;
	b8 y_inside = rect.pos.y <=  p.y && p.y  < rect.pos.y + rect.size.y;
	return x_inside && y_inside;
}

#define INVENTORY_SIZE 20

typedef struct Inventory{
	b8 is_editable;
	u8 size;
	u8 selected_slot;

	u16 items[INVENTORY_SIZE];
	
}Inventory;

typedef enum Item_id : u16
{
	ITEM_NULL,

	ITEM_PLACEABLE_FIRST,
	ITEM_STONE,
	ITEM_PLACEABLE_LAST,

	ITEM_SPELL_NULL,
	ITEM_SPELL_DESTROY,
	ITEM_SPELL_PROJECTILE,
	ITEM_SPELL_LAST,

	ITEM_WAND,

	ITEM_LAST_ID
}Item_id;

#define MAX_ITEM_STACK 0xff
typedef struct Item
{
	u16 item_count;
	Item_id item_id;

	f32 casting_cd;
	b8 not_cycle_when_casting;

	Inventory inventory;
}Item;

enum Entity_flag : u64
{
   E_RENDER = 0b1,
	E_PICKUP = 0b10,
   E_LAST_FLAG = 0b100,
};

typedef enum Casting_state : u8
{
	CASTING_NULL,
	CASTING_LEFT,
	CASTING_RIGHT,
}Casting_state;

typedef struct Entity
{
   u64 flags;
	struct {
		u16 item;
	};// pickup
	u16 tex_uid;

   V2 pos;

   V2 move_direction;
   f32 movement_speed;
	V2 target_direction;

	f32 casting_cd;
	b8 stop_cycling;
	b8 already_casted;
	Casting_state casting_state;
	
	Inventory inventory;
	u16 unarmed_inventory;
}Entity;


typedef struct Ui_selection
{
	u16 hot

	,pressed
	,clicked

	,pressed2
	,clicked2
	;
}Ui_selection;
Ui_selection NULL_UI_SELECTION = {NULL_INDEX16, NULL_INDEX16, NULL_INDEX16, NULL_INDEX16, NULL_INDEX16};

typedef enum Ui_layout
{
	UI_LAYOUT_DEFAULT,
	UI_LAYOUT_GRID_ROW_MAJOR,


	UI_LAYOUT_LAST,
}Ui_layout;

typedef struct Ui_style
{
	Ui_layout layout;
	u32 line_size;
	f32 cells_border_size;

	string text;
	
	// union {
	// 	struct{
	// 		Int2 pos;
	// 		Int2 size;
	// 	};
	// 	Rect_int rect;
	// };
	union {
		struct{
			V2 pos;
			V2 size;
		};
		Rect_float rect;
	};
	s32 zpos;

	Color color_rect;
	Color color_text;	

	u16 tex_uid;
}Ui_style;
Ui_style STYLE_NULL = {0};

enum Ui_flags
{
	UI_SKIP_RENDERING = 0b1,
	UI_CLICKABLE = 0b10,
	UI_TEXTBOX = 0b100,
	UI_SKIP_RENDERING_CHILDREN = 0b1000,
	UI_SELECTABLE = 0b10000,
	UI_LAST_FLAG = 0b100000
};

typedef struct Ui_widget
{
	u64 flags;

	union{
		struct{
			u16 parent_uid;
			u16 local_uid;
		};
		u32 full_uid;
	};
	// u32 last_frame_uid;
   u16 child_count;

   Ui_style style;
}Ui_widget;

#define MAX_UI_WIDGETS 1000
typedef struct Ui_data
{
	Ui_widget widgets[MAX_UI_WIDGETS];
	V2 global_positions[MAX_UI_WIDGETS];
	int global_zpositions[MAX_UI_WIDGETS];

	u16 current_parent_uid;

	u16 current_widget_uid;
	
	u16 focus;
	Int2 focused_ui_position;

	Ui_selection selection;
	// Ui_uid selected_button; // TODO: maybe add this to the Ui_selection struct
}Ui_data;

u16 do_widget(Ui_data* ui, u32 widget_flags, Ui_style style)
{
	u16 result = ui->current_widget_uid++;
	Ui_widget* new_widget;
	new_widget = &ui->widgets[result];
	new_widget->style = style;
	new_widget->flags = widget_flags;
	new_widget->parent_uid = ui->current_parent_uid;
	new_widget->local_uid = ui->widgets[ui->current_parent_uid].child_count++;

	return result;
}

u16 ui_push(Ui_data* ui, u16 uid)
{
	assert(uid < MAX_UI_WIDGETS);
	ui->current_parent_uid = uid;
	return uid;
}

void ui_pop(Ui_data* ui)
{
	assert(ui->current_parent_uid < MAX_UI_WIDGETS);
	ui->current_parent_uid = ui->widgets[ui->current_parent_uid].parent_uid;
}

#define MAX_ENTITIES 1000

//TODO: maybe reserve the 0 index for the nil_entity and put player in 1?
#define E_PLAYER_INDEX 0

#define WORLD_Y_LENGTH 100
#define WORLD_X_LENGTH 100
#define MAX_ITEMS 0xffff

typedef struct App_data
{
   V2 camera_pos;
   Int2 viewport_size;

   Entity entities[MAX_ENTITIES];
	u8 used_entities[MAX_ENTITIES];
   Ui_data ui;

   u8 world [WORLD_Y_LENGTH][WORLD_X_LENGTH];

   b8 is_menu_opened;

	//indices from 0 to ITEM_LAST_ID are reserved for presets/static items
	u8 used_items[MAX_ITEMS];
	Item items[MAX_ITEMS];
	u16 cursor_item;

	u16 item_id_to_tex_uid [ITEM_LAST_ID];
}App_data;


u16 get_first_available_index(u8* array, u32 arraylen)
{
	assert(arraylen <= 0xffff);
	u16 temp = 0;
	u16 current_i = 0;

	UNTIL(i, arraylen)
	{
		u16 index = (current_i+i)%(u16)arraylen;
		if(!array[index])
		{
			array[index] = 1;
			return index;
		}
	}
	assert(false);
	return NULL_INDEX16;
}

f32 f32_lerp(f32 a, f32 b, f32 t)
{
   return a*(1.0f-t) + b*t;
}

// this is a pseudo perlin noise function
f32 sample_2d_perlin_noise(float* noisemap, 
    int width, int height, int x, int y, 
    u32 max_iterations, //default 8
    int initial_sample_resolution, // default 2 and a multiple of 2
    float influence_step_multiplier // default .5f
    )
{
    float amplitude = 1.0f;

    int step_count = initial_sample_resolution;
    float cumulative_value = 0;
    float cumulative_amplitude = 0;

    UNTIL(current_i, max_iterations)
    {
        int xstep_size = width/step_count;
        int ystep_size = height/step_count;

        if(!xstep_size || !ystep_size) break;

        int ix1 = (x/xstep_size)*xstep_size;
        int iy1 = (y/ystep_size)*ystep_size;
        int ix2 = (((x+xstep_size)/xstep_size)*xstep_size)%width;
        int iy2 = (((y+ystep_size)/ystep_size)*ystep_size)%height;

        float samplex1y1 = noisemap[iy1*width + ix1];
        float samplex2y1 = noisemap[iy1*width + ix2];
        float samplex1y2 = noisemap[iy2*width + ix1];
        float samplex2y2 = noisemap[iy2*width + ix2];

        float tx = ((float)x-ix1)/xstep_size;
        float ty = ((float)y-iy1)/ystep_size;

        float lerped_y1 = f32_lerp(samplex1y1, samplex2y1, tx);
        float lerped_y2 = f32_lerp(samplex1y2, samplex2y2, tx);

        cumulative_value += amplitude * f32_lerp(lerped_y1, lerped_y2, ty);
        cumulative_amplitude += amplitude;
        
        amplitude = influence_step_multiplier*amplitude;
        step_count *= 2;
    }

    return cumulative_value/cumulative_amplitude;
}


V2 size_in_pixels_to_screen(Int2 size, f32 aspect_ratio, Int2 viewport_size)
{
	return v2(2*aspect_ratio*(f32)size.x/viewport_size.x, 2*(f32)size.y/viewport_size.y);
}

Int2 world_pos_to_tile_pos(V2 v, V2 tiles_screen_size)
{
   Int2 result;
   result.x = (s32)((v.x/tiles_screen_size.x)+ WORLD_X_LENGTH/2);
   result.y = (s32)((v.y/tiles_screen_size.y)+ WORLD_Y_LENGTH/2);
   
   return result;
}

V2 tile_to_pos(Int2 tile, V2 tiles_screen_size)
{
	V2 result;
	result.x = (tile.x-(WORLD_X_LENGTH/2)+.5f)*tiles_screen_size.x;
	result.y = (tile.y-(WORLD_Y_LENGTH/2)+.5f)*tiles_screen_size.y;
	return result;
}

void destroy_tile(App_data* app, Int2 tile_pos, V2 tiles_screen_size)
{
	app->world[tile_pos.y][tile_pos.x] = 0;
	u16 e_id = get_first_available_index(app->used_entities, MAX_ENTITIES);
	app->entities[e_id].flags = E_PICKUP|E_RENDER;
	u16 new_item_id = get_first_available_index(app->used_items, MAX_ITEMS);
	app->entities[e_id].item = new_item_id;
	//TODO: tiles will have their own item_id instead of ITEM_STONE
	app->items[new_item_id] = app->items[ITEM_STONE];

	app->entities[e_id].pos = tile_to_pos(tile_pos, tiles_screen_size);
}

string u32_to_string(u32 n, Memory_arena* arena)
{
	u32 i=0;
	string result = {0};
	result.data = (u8*)arena_push_size(arena, 0);
	if(!n) // if number is 0
	{
		*(u8*)arena_push_size(arena, 1) = '0';
		arena_push_size(arena, 1); // 0 ending string
		result.count = 1;
		return result;
	}
	u8 digits = 0;
	u32 temp = n;
	while(temp)
	{
		temp = temp/10;
		i++;
		digits++;
	}
	arena_push_size(arena, digits);
	result.count= i;
	for(;digits; digits--)
	{
		result.data[i-1] = '0' + (n%10);
		n = n/10;
		i--;
	}
	*(u8*)arena_push_size(arena, 1) = 0; // 0 ending string
	return result;
}


u16 get_entity_equipped_item_index(App_data* app, u16 entity_index)
{
	u16 result = app->entities[entity_index].inventory.items[app->entities[entity_index].inventory.selected_slot];
	if(!result)
		result = app->entities[entity_index].unarmed_inventory;
	return result;
}

b8 is_placeable_item(Item_id item_id)
{
	return ITEM_PLACEABLE_FIRST < item_id && item_id < ITEM_PLACEABLE_LAST;
}