#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

struct ACD {
	DWORD id; // 00
	BYTE unk1[0x88];
	DWORD actor_id; // 8C -1 if item is not on ground
	BYTE unk2[0x20];
	// player: 7, monster: 1, item: 2, invalid: -1?
	DWORD type; // B0
	DWORD B4;
	DWORD B8;
	char unk3[0x14];
	// these are 0 if not on ground
	float x; // D0
	float y; // D4
	float z; // D8
	BYTE unk4[0x34];
	DWORD owner_id; // 110 -1 if no owner
	BYTE unk5[0x1BC];
};

struct ACDManager {
	BYTE unk1[0x108];
	int num;
	BYTE unk2[0x3C];
	struct ACD **unit;
};

struct mmap_scene {
	DWORD unk1;
	DWORD id;
	char *name;
	DWORD unk2;
	struct {
		float x;
		float y;
	} start;
	struct {
		float x;
		float y;
	} end;
	DWORD unk3[4];
	DWORD tick;
	DWORD unk4;
	struct mmap_scene *prev;
	struct mmap_scene *next;
};

struct mmap_actor_info {
	DWORD *unk1;
	char *text;
	DWORD *unk2;
};

struct object_manager {
	BYTE unk1[0xD4];
	ACDManager **ACDman; // rather pointer to ACD manager struct which first member is pointer to ACD which has an array of units
};

struct object_manager ****obj_man = (struct object_manager ****) 0x014EBA9C;

//struct ACDManager **man; // 0x0620F17C + 0x0D4
// not static :/
// globl = ***0x014EBA9C + 0xD4
//struct ACDManager ***globl = (ACDManager ***) 0x0620F250;

DWORD patch = 0x00B036AF; // size 9 bytes, jmp patch

DWORD mmap_update_patch = 0x00AFF695; // 5 bytes, call patch
DWORD active_scene_check_patch = 0x00AFF5F2; // 6 bytes, NOPd
DWORD draw_markers_patch = 0x00B019F6; // 6 bytes, jmp patch

BYTE backup[9];

DWORD p_this; // this is not static :/

DWORD minimap_layer = 0x015CD318;

//__stdcall bool (*D3_load_marker)(int unk1, char *marker, DWORD *buf) = 0x0095F0A0;
typedef BOOL __cdecl D3_load_marker_t (int unk1, char *marker, DWORD *buf); D3_load_marker_t *D3_load_marker = (D3_load_marker_t *) 0x0095F0A0;
//__stdcall void (*D3_transform_coords)(float *w_coords, float *m_coords) = 0x00AFF050;
typedef void __stdcall D3_transform_coords_t (float *w_coords, float *m_coords); D3_transform_coords_t *D3_transform_coords = (D3_transform_coords_t *) 0x00AFF050; // __thiscall actually
//__fastcall int (*D3_load_info)(DWORD *buf) = 0x00EB53B0;
typedef int __fastcall D3_get_player_info_t (struct mmap_actor_info *info); D3_get_player_info_t *D3_get_player_info = (D3_get_player_info_t *) 0x00EB53B0;
//__stdcall void (*D3_draw_marker)(int unk1, DWORD *buf, POINT m_coords, DWORD obj, int unk2, void *unk3, DWORD id, int unk4, DWORD unk5) = 0x00B010F0;
typedef void __stdcall D3_draw_marker_t (int unk1, DWORD *buf, float *m_coords, float f, int unk2, struct mmap_actor_info *info, DWORD id, int unk4, DWORD unk5); D3_draw_marker_t *D3_draw_marker = (D3_draw_marker_t *) 0x00B010F0; // __thiscall

typedef DWORD __cdecl D3_get_layer_t (DWORD p_object); D3_get_layer_t *D3_get_layer = (D3_get_layer_t *) 0x00930930;

typedef DWORD __cdecl D3_get_actor_t (DWORD id); D3_get_actor_t *D3_get_actor = (D3_get_actor_t *) 0x00823160; // or thiscall RActors.008230F0

typedef void __cdecl D3_get_actor_info_t (struct mmap_actor_info *info, DWORD actor); D3_get_actor_info_t *D3_get_actor_info = (D3_get_actor_info_t *) 0x00C85930; // size of buf 3 DWORDs, actor is pointer to actor struct

typedef void __cdecl D3_update_minimap_t (struct mmap_scene *scene, float x, float y, float sight_radius, DWORD layer); D3_update_minimap_t *D3_update_minimap = (D3_update_minimap_t *) 0x00AFF2A0;

typedef void __fastcall D3_free_actor_info_t (struct mmap_actor_info *info); D3_free_actor_info_t *D3_free_actor_info = (D3_free_actor_info_t *) 0x00EB5B40;

typedef DWORD __cdecl D3_get_shader_key_t (DWORD arg); D3_get_shader_key_t *D3_get_shader_key = (D3_get_shader_key_t *) 0x00850C20;
// ShaderMap
typedef DWORD __cdecl D3_get_shader_t (DWORD key, DWORD unk1); D3_get_shader_t *D3_get_shader = (D3_get_shader_t *) 0x0097ACB0;

typedef float __stdcall D3_get_attribute_f_t (DWORD id); D3_get_attribute_f_t *D3_get_attribute_f = (D3_get_attribute_f_t *) 0x0089B640; // __thiscall
typedef DWORD __stdcall D3_get_attribute_i_t (DWORD id); D3_get_attribute_i_t *D3_get_attribute_i = (D3_get_attribute_i_t *) 0x0089B700; // __thiscall

typedef DWORD __stdcall D3_get_player_acd_id_t (); D3_get_player_acd_id_t *D3_get_player_acd_id = (D3_get_player_acd_id_t *) 0x00942670;
typedef struct ACD * __stdcall D3_get_acd_t (DWORD id); D3_get_acd_t *D3_get_acd = (D3_get_acd_t *) 0x00812400; // __thiscall

__declspec(naked) struct ACD * __stdcall D3_get_acd_stub (void *t, DWORD id) { // this is ACDManager
	__asm
	{
		pop eax
		pop ecx
		push eax
		jmp D3_get_acd
	}
}

float __declspec(naked) __stdcall D3_get_attribute_f_stub(void *t, DWORD id) {
	__asm
	{
		pop eax
		pop ecx
		push eax
		jmp D3_get_attribute_f
	}
}

DWORD __declspec(naked) __stdcall D3_get_attribute_i_stub(void *t, DWORD id) {
	__asm
	{
		pop eax
		pop ecx
		push eax
		jmp D3_get_attribute_i
	}
}

struct font {
	DWORD id;
	DWORD size;
	float unk1;
	float unk2;
	DWORD unk3; // BOOL
};

struct draw_struct { // 80 bytes, 24 bytes, 64/68 bytes, 32 bytes, I think it's 0x24 = 36 bytes = 9 DWORDs
	char *text;
	DWORD unk[19];
};

typedef struct draw_struct * __stdcall D3_draw_struct_init_t (char *text, DWORD arg2 /* 1 */, DWORD arg3 /* 0 */); D3_draw_struct_init_t *D3_draw_struct_init = (D3_draw_struct_init_t *) 0x009FE380; // __thiscall

__declspec(naked) draw_struct * __stdcall D3_draw_struct_init_stub(void *t, char *, DWORD, DWORD) {
	__asm
	{
		pop eax
		pop ecx
		push eax
		jmp D3_draw_struct_init
	}
}

typedef void __fastcall D3_draw_struct_free_t (struct draw_struct *s); D3_draw_struct_free_t *D3_draw_struct_free = (D3_draw_struct_free_t *) 0x009FD340; // __thiscall

struct draw_info {
	float unk[4];
	struct {
		float width;
		float height;
	} size;
};

typedef void __cdecl D3_draw_text_t (
	struct font *ft,
	//char **text,
	draw_struct *s,
	float *coords,
	DWORD color,
	DWORD *arg5,
	DWORD arg6, // BOOL
	float arg7,
	DWORD arg8, // BOOL
	struct draw_info *info,
	DWORD arg10
	);
D3_draw_text_t *D3_draw_text = (D3_draw_text_t *) 0x0094A530;

typedef void __cdecl D3_get_text_size_t (struct font *ft, struct draw_struct *s, float *size, DWORD arg4, DWORD arg5); D3_get_text_size_t *D3_get_text_size = (D3_get_text_size_t *) 0x0094AE90;

typedef void __cdecl D3_c_convert_to_pixel_t (float *, float *, BOOL rounding); D3_c_convert_to_pixel_t *D3_c_convert_to_pixel = (D3_c_convert_to_pixel_t *) 0x00A67C80;
typedef void __cdecl D3_b_convert_to_pixel_t (float *, float *, BOOL rounding); D3_b_convert_to_pixel_t *D3_b_convert_to_pixel = (D3_b_convert_to_pixel_t *) 0x00A67C00; // that's not what I thought it'd be
typedef void __cdecl D3_c_convert_to_world_t (DWORD *, float *); D3_c_convert_to_world_t *D3_c_convert_to_world = (D3_c_convert_to_world_t *) 0x00A67A40;

typedef void __fastcall D3_draw_2_t (draw_struct *s); D3_draw_2_t *D3_draw_2 = (D3_draw_2_t *) 0x009FD340;

DWORD color_obj = 0x015721F4;
typedef void __stdcall D3_push_color_t (DWORD *color); D3_push_color_t *D3_push_color = (D3_push_color_t *) 0x009495A0; // __thiscall
typedef void __stdcall D3_pop_color_t (DWORD *color); D3_pop_color_t *D3_pop_color = (D3_pop_color_t *) 0x009495C0; // __thiscall
typedef void __stdcall D3_clear_color_t (); D3_clear_color_t *D3_clear_color = (D3_clear_color_t *) 0x00949590; // __thiscall

typedef void __cdecl D3_print_message_t (char *m); D3_print_message_t *D3_print_message = (D3_print_message_t *) 0x00A36450;

typedef void __cdecl D3_draw_rectangle_t (float *coords, DWORD *color, float arg3, DWORD arg4); D3_draw_rectangle_t *D3_draw_rectangle = (D3_draw_rectangle_t *) 0x0099E580;

DWORD **screen_dimension = (DWORD **) 0x014ECDB8;

void __declspec(naked) __stdcall D3_push_color_stub(void *t, DWORD *color) {
	__asm
	{
		pop eax
		pop ecx
		push eax
		jmp D3_push_color
	}
}

void __declspec(naked) __stdcall D3_pop_color_stub(void *t, DWORD *color) {
	__asm
	{
		pop eax
		pop ecx
		push eax
		jmp D3_pop_color
	}
}

void __declspec(naked) __stdcall D3_clear_color_stub(void *t) {
	__asm
	{
		pop eax
		pop ecx
		push eax
		jmp D3_clear_color
	}
}

void get_text_box(float *box, DWORD *coords, struct font *ft, struct draw_struct *s) {
	float w_size[2];
	float p_size[2];
	D3_get_text_size(ft, s, w_size, 0, 0);
	D3_c_convert_to_pixel(w_size, p_size, 1);
	//p_size[0] = ceil(p_size[0]), p_size[1] = ceil(p_size[1]);
	p_size[0]++, p_size[1]++;

	DWORD end[2];
	end[0] = coords[0] + (DWORD) p_size[0], end[1] = coords[1] + (DWORD) p_size[1];

	D3_c_convert_to_world(coords, box);
	D3_c_convert_to_world(end, &box[2]);

	box[2] = box[0] + w_size[0], box[3] = box[1] + w_size[1];
	end[0] = coords[0] + (DWORD) w_size[0];
	end[1] = coords[1] + (DWORD) w_size[1];
	D3_c_convert_to_world(end, &box[2]);
}

void my_draw_text(DWORD x, DWORD y, DWORD color, DWORD ftsize, DWORD ftid, char *text) {
	struct font ft = { ftid, ftsize, 1.0f, 1.0f, 1};
	struct draw_struct s = { text, { 
		0x0171BDC0,
		0x0171BDD0,
		0x00000000,
		strlen(text),
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00020201,
		0x00000000,
		0x00000000,
		0x600DF00D,
		0x00000000,
		0x00000001,
		0x00000000 } };
		DWORD coords[] = {x, y};
		float w_coords[2];
		D3_c_convert_to_world(coords, w_coords);
		// box can be too small by 1 pixel, need to fix that
		//float size[2];
		//D3_get_text_size(&ft, &s, size, 0, 0);
		//float box[] = { w_coords[0], w_coords[1], ceil(w_coords[0]) + ceil(size[0]), ceil(w_coords[1]) + ceil(size[1]) + 5 };
		float box[4];
		get_text_box(box, coords, &ft, &s);
		DWORD arg5 = 0xFF000000;
		D3_push_color_stub((void *) color_obj, &color);
		DWORD c = 0xFFFFFFFF;
		float rec[4];
		D3_b_convert_to_pixel(box, rec, 1);
		//D3_draw_rectangle(rec, &c, 0.9999f, -1);
		D3_draw_text(&ft, &s, w_coords, 0x44A135FF, &arg5, 1, 0.999f, 1, (struct draw_info *) box, 0xFFFFFFFF);
		//D3_draw_text(&ft, &s, w_coords, 0x44A135FF, &arg5, 1, 0.999f, 0, (struct draw_info *) box, 0xFFFFFFFF);
		D3_pop_color_stub((void *) color_obj, &color);
}

void draw_text() {
	/*struct font arg1 = {
		0x00011A9E,
		0x00000010,
		1.0f,
		1.0f,
		0x00000001
	};
	char *s_text = "{c:ffff0000}snoogans{/c}";
	DWORD len = strlen(s_text);
	struct draw_struct s = { s_text, { 
		0x0171BDC0,
		0x0171BDD0,
		0x00000000,
		len,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00020201,
		0x00000000,
		0x00000000,
		0x600DF00D,
		0x00000000,
		0x00000001,
		0x00000000 } };
	//struct draw_struct *p = (struct draw_struct *) 0x24D58780;
	//p->text = "snoogans";
	float coords[] = { 1775.0f - 15.0f, 28.0f };
	DWORD arg5 = 0xFF000000;
	struct draw_info info = { { 1285.667f, 27.0f, 1835.667f, 51.0f }, { 58.333332f, 11.666666f } };
	//D3_get_text_size(&arg1, &s, (float *) &info.size, 0, 0);
	D3_draw_text(&arg1, &s, coords, 0x0000FFFF, &arg5, 0, 0.999f, 1, &info, 0xFFFFFFFF);
	//D3_draw_1(0x015721F4);
	//D3_draw_2(&s);
	//p->text = "ololol";*/

	my_draw_text(20, 20, 0xFFFF00FF, 0x14, 0xCE38, "snoogans!"); // color in ARGB, font 0x11A9C/0x11A9E/0xCE38/0x11A9B
	my_draw_text(20, 40, 0xFF00FF00, 0x14, 0x11A9C, "snoogans!");
	my_draw_text(20, 60, 0xFF0000FF, 0x14, 0x11A9E, "snoogans!");
	my_draw_text(20, 80, 0xFFFF0000, 0x14, 0x11A9B, "snoogans!");
}

#define square(a) ((a) * (a))
#define distance(a, b) ((float) sqrt(square((a).x - (b).x) + square((a).y - (b).y)))

void draw_healthbar(DWORD x, DWORD y, struct ACD *unit) {
	float cur_hp = D3_get_attribute_f_stub(unit, 0xFFFFF066);
	float max_hp = D3_get_attribute_f_stub(unit, 0xFFFFF06F);
	if (cur_hp < max_hp) cur_hp = ceil(cur_hp);
	if (max_hp < 0.00f) return;
	char buf[512];
	sprintf_s(buf, "%i/%i", (DWORD) cur_hp, (DWORD) max_hp);

	struct font ft = {
		0x00011A9E,
		0x00000010,
		1.0f,
		1.0f,
		0x00000001
	};
	DWORD len = strlen(buf);
	struct draw_struct s = { buf, { 
		0x0171BDC0,
		0x0171BDD0,
		0x00000000,
		len,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00000000,
		0x00020201,
		0x00000000,
		0x00000000,
		0x600DF00D,
		0x00000000,
		0x00000001,
		0x00000000 } };

	float p_size[2];
	D3_get_text_size(&ft, &s, p_size, 0, 0);

	float rec[4] = { x, y, x + 50, y + p_size[1] + 6 };
	DWORD c = 0x80000000;
	D3_draw_rectangle(rec, &c, 0.9999f, -1);

	rec[0] += 2;
	rec[1] += 2;
	rec[2] = rec[0] + (cur_hp / max_hp) * (50 - 4);
	rec[3] -= 2;
	c = 0xFF0000FF;
	D3_draw_rectangle(rec, &c, 0.9998f, -1);

	my_draw_text(x + 25 - p_size[0] / 2, y + 3, 0xFFFFFFFF, 0x10, 0x11A9E, buf);
}

struct acd_info {
	DWORD *unk1;
	char *string;
	DWORD unk2;
};

typedef DWORD __cdecl D3_check_gbid_t (struct ACD *); D3_check_gbid_t *D3_check_gbid = (D3_check_gbid_t *) 0x00833730;
typedef acd_info * __cdecl D3_get_item_info_t (struct acd_info *, struct ACD *, DWORD arg3, DWORD arg4); D3_get_item_info_t *D3_get_item_info = (D3_get_item_info_t *) 0x009D9FF0;
typedef acd_info * __cdecl D3_get_monster_info_t (struct acd_info *, struct ACD *, DWORD arg3); D3_get_monster_info_t *D3_get_monster_info = (D3_get_monster_info_t *) 0x00904600;

typedef DWORD * __cdecl D3_get_item_color_t (DWORD *color, struct ACD *item); D3_get_item_color_t *D3_get_item_color = (D3_get_item_color_t *) 0x009D70A0;

typedef DWORD __fastcall D3_get_monster_color_code_t (DWORD arg1); D3_get_monster_color_code_t *D3_get_monster_color_code = (D3_get_monster_color_code_t *) 0x00902C40; // borland fastcall
typedef DWORD * __cdecl D3_get_color_t (DWORD *color, DWORD arg1); D3_get_color_t *D3_get_color = (D3_get_color_t *) 0x00949570;

__declspec(naked) DWORD __fastcall D3_get_monster_color_code_stub(DWORD arg1) {
	__asm
	{
		mov eax, ecx
		jmp D3_get_monster_color_code
	}
}

DWORD * get_monster_color(DWORD *color, struct ACD *monster) {
	DWORD cc = D3_get_monster_color_code_stub(monster->B8);
	return D3_get_color(color, cc);
}

struct ids3 { // sizeof 0xA4
	DWORD unk1[9];
	char *desc[19];
	DWORD unk2[13];
};

typedef DWORD * __stdcall D3_init_ids1_t (DWORD *, struct acd *); D3_init_ids1_t *D3_init_ids1 = (D3_init_ids1_t *) 0x009DAE70; // __thiscall
typedef DWORD * __fastcall D3_init_ids2_t (DWORD *); D3_init_ids2_t *D3_init_ids2 = (D3_init_ids2_t *) 0x00825EF0; // __thiscall
typedef struct ids3 * __fastcall D3_init_ids3_t (struct ids3 *); D3_init_ids3_t *D3_init_ids3 = (D3_init_ids3_t *) 0x009D1690; // __thiscall

__declspec(naked) DWORD * __stdcall D3_init_ids1_stub(void *t, struct ACD *p) {
	__asm
	{
		pop eax
		pop ecx
		push eax
		jmp D3_init_ids1
	}
}

typedef void __cdecl D3_load_ids2_t (DWORD *ids2, struct ACD *item, DWORD *ids1, struct ACD *me, DWORD pos /* 0 */, DWORD arg6 /* 1 */, DWORD arg7 /* 0 */); D3_load_ids2_t *D3_load_ids2 = (D3_load_ids2_t *) 0x009E5BC0;

typedef void __cdecl D3_transform_ids_t (struct ids3 *id, DWORD acd_id, DWORD acd_b4, DWORD *ids2, DWORD arg5 /* 1 */, struct ACD *me); D3_transform_ids_t *D3_transform_ids = (D3_transform_ids_t *) 0x009E63E0;

typedef void __fastcall D3_free_ids2_t (DWORD *); D3_free_ids2_t *D3_free_ids2 = (D3_free_ids2_t *) 0x00825370; // __thiscall

void load_item_description(DWORD *ids2, struct ids3 *id, struct ACD *item) {
	ACDManager *man = *(***obj_man)->ACDman;

	struct ACD *me = D3_get_acd_stub(man, D3_get_player_acd_id());
	if (!me) return;

	DWORD ids1[3]; //, ids2[0x4A];
	D3_init_ids1_stub(ids1, me);
	D3_init_ids2(ids2);
	D3_load_ids2(ids2, item, ids1, me, 0, 1, 0);
	D3_init_ids3(id);
	D3_transform_ids(id, item->id, item->B4, ids2, 1, me);
}

void my_draw_text2(DWORD x, DWORD y, DWORD color, DWORD ftsize, DWORD ftid, struct draw_struct *s) {
	struct font ft = { ftid, ftsize, 1.0f, 1.0f, 1};
		DWORD coords[] = {x, y};
		float w_coords[2];
		D3_c_convert_to_world(coords, w_coords);
		// box can be too small by 1 pixel, need to fix that
		//float size[2];
		//D3_get_text_size(&ft, &s, size, 0, 0);
		//float box[] = { w_coords[0], w_coords[1], ceil(w_coords[0]) + ceil(size[0]), ceil(w_coords[1]) + ceil(size[1]) + 5 };
		float box[4];
		get_text_box(box, coords, &ft, s);
		DWORD arg5 = 0xFF000000;
		D3_push_color_stub((void *) color_obj, &color);
		DWORD c = 0xFFFFFFFF;
		float rec[4];
		D3_b_convert_to_pixel(box, rec, 1);
		//D3_draw_rectangle(rec, &c, 0.9999f, -1);
		D3_draw_text(&ft, s, w_coords, 0x44A135FF, &arg5, 1, 0.999f, 1, (struct draw_info *) box, 0xFFFFFFFF);
		//D3_draw_text(&ft, &s, w_coords, 0x44A135FF, &arg5, 1, 0.999f, 0, (struct draw_info *) box, 0xFFFFFFFF);
		D3_pop_color_stub((void *) color_obj, &color);
}

void dump_item_description(struct ACD *item, char *name) {
	/*static int i = 0;
	static char **items = NULL;
	int j;
	for (j = 0; j < i; j++) {
		if (!strcmp(items[j], name)) break;
	}
	if (!i || j == i) {
		items = (char **) realloc(items, (i + 1) * sizeof(char *));
		items[i++] = _strdup(name);
	} else {
		return;
	}

	if (strstr(name, "Gold") || strstr(name, "Health")) return;

	printf("trying to dump %s\n", name);
	struct ids3 id;
	load_item_description(&id, item);
	for (j = 0; j < 19; j++) {
		if (strlen(id.desc[j])) printf("%s\n", id.desc[j]);
	}
	printf("\n\n");*/

	DWORD ids2[0x4A];
	struct ids3 id;
	load_item_description(ids2, &id, item);
	int i;
	int j = 0;
	char buf[5012];
	memset(buf, 0, 5012);
	strcpy(buf, "{c:ffffffff}Closest item:{/c}\n");
	for (i = 0; i < 19; i++) {
		if (strlen(id.desc[i])) {
			/*j++;
			struct draw_struct s;
			D3_draw_struct_init_stub(&s, id.desc[i], 1, 0);

			my_draw_text2(20, 400 + j * 10, 0xFFFFFFFF, 0x10, 0x11A9E, &s);

			D3_draw_struct_free(&s);*/
			
			strcat(buf, id.desc[i]);
			buf[strlen(buf)] = '\n';
		}
	}
	buf[strlen(buf)-1] = '\0';
	struct draw_struct s;
	D3_draw_struct_init_stub(&s, buf, 1, 0);

	/*float w_size[2];
	struct font ft = { 0x11A9E, 0x10, 1.0f, 1.0f, 1};
	D3_get_text_size(&ft, &s, w_size, 0, 0);
	DWORD coords[2] = { 20, 400 };
	float rec[4];
	float w_rec[4];
	D3_c_convert_to_world(coords, w_rec);
	w_rec[2] = w_rec[0] + w_size[0], w_rec[3] = w_size[1] + w_size[1];
	D3_b_convert_to_pixel(w_rec, rec, 1);

	//float rec[4] = { 20 - 2, 400 - 2, 20 + 2 + w_size[0], 400 + 2 + w_size[1] };
	DWORD c = 0x80000000;
	D3_draw_rectangle(rec, &c, 0.9999f, -1);*/
	
	my_draw_text2(500, 5, 0xFFFFFFFF, 0x10, 0x11A9E, &s);
	
	D3_draw_struct_free(&s);

	D3_free_ids2(ids2);
}

void draw_huds() {
	ACDManager *man = *(***obj_man)->ACDman;

	struct ACD *me = D3_get_acd_stub(man, D3_get_player_acd_id());
	if (!me) return;

	DWORD ft = 0x11A9E, ft_size = 0x10;
	DWORD x = 5, y = 5;
	my_draw_text(x, y, 0xFFFFFFFF, ft_size, ft, "Monsters within sight:");
	y += 15;

	x += 10;
	int i;
	for (i = 0; i < man->num; i++) {
		struct ACD unit = (*(man->unit))[i];
		if (unit.type != 1 || unit.id == (DWORD) -1) continue;
		if (distance(*me, unit) > 65.0f) continue;
		draw_healthbar(x, y, &unit);
		char buf[512];
		struct acd_info info;
		D3_get_monster_info(&info, &unit, 0);
		DWORD level = D3_get_attribute_i_stub(&unit, 0xFFFFF03B);
		sprintf_s(buf, "%s (%i)", (char *) info.string, level);
		D3_free_actor_info((struct mmap_actor_info *) &info);
		DWORD c = 0xFFFFFFFF;
		//get_monster_color(&c, &unit);
		my_draw_text(x + 60, y + 2, c, ft_size, ft, buf);
		y += 20;
	}
	x -= 10;

	y += 30;
	my_draw_text(x, y, 0xFFFFFFFF, ft_size, ft, "Items within sight:");
	y += 15;

	struct ACD closest;
	float max = 1000.0f;
	BOOL found = false;
	x += 10;
	for (i = 0; i < man->num; i++) {
		struct ACD unit = (*(man->unit))[i];
		if (unit.type != 2 || unit.owner_id != (DWORD) -1 || unit.id == (DWORD) -1) continue;
		if (distance(*me, unit) > 65.0f) continue;
		if (distance(*me, unit) < max) {
			closest = unit;
			max = distance(*me, unit);
			found = true;
		}
		char buf[512];
		struct acd_info info;
		D3_get_item_info(&info, &unit, 0, 0);
		sprintf_s(buf, "%s", (char *) info.string);
		D3_free_actor_info((struct mmap_actor_info *) &info);
		DWORD c;
		D3_get_item_color(&c, &unit);
		my_draw_text(x, y, c, ft_size, ft, (char *) buf);
		y += 15;

		//dump_item_description(&unit, info.string);
	}
	x -= 10;



	if (found) dump_item_description(&closest, "bla");
}

void dump_arg1(FILE *f, struct font *ft) {
	fprintf(f, "---\n");
	fprintf(f, "ID: %X\n", ft->id);
	fprintf(f, "size: %i\n", ft->size);
	fprintf(f, "%f\n", ft->unk1);
	fprintf(f, "%f\n", ft->unk2);
	fprintf(f, "%i\n", ft->unk3);
	fprintf(f, "---\n");
}

void dump_arg2(FILE *f, draw_struct *s) { // 64 bytes
	fprintf(f, "---\n");
	fprintf(f, "text: %s\n", s->text);
	int i;
	for (i = 0; i < sizeof(s->unk) / sizeof(DWORD); i++) {
		fprintf(f, "%08X\n", s->unk[i]);
	}
	fprintf(f, "---\n");
}

void dump_arg9(FILE *f, struct draw_info *info) {
	float p_info[4];
	D3_b_convert_to_pixel(info->unk, p_info, 1);
	fprintf(f, "---\n");
	int i;
	for (i = 0; i < 4; i++) {
		fprintf(f, "%f - %f\n", info->unk[i], p_info[i]);
	}
	fprintf(f, "size: %f/%f\n", info->size.width, info->size.height);
	fprintf(f, "---\n");
}
void D3_click_ui(__int64);
void __cdecl draw_text_intercept(struct font *ft, draw_struct *s, float *coords, DWORD color, DWORD *arg5, DWORD arg6, float arg7, DWORD arg8, struct draw_info *info, DWORD arg10) {
	char *old = s->text;
	//s->text = "snoogans";
	//coords[0] = 1777.0f;
	//coords[1] = 28.0f;
	//coords[0] -= 50.0f;
	//coords[1] += 50.0f;
	if (strstr(s->text, "PM")|| strstr(s->text, "AM")) {
		//draw_text();
		draw_huds();
		my_draw_text(8, 712, 0xFF00FFFF, 0x11, 0x11A9E, "feedface.dll");
		D3_draw_text(ft, s, coords, color, arg5, arg6, arg7, arg8, info, arg10);
	} else {
		D3_draw_text(ft, s, coords, color, arg5, arg6, arg7, arg8, info, arg10);
	}
	
	static int ii = 1;
	if (ii) {
		printf("clicking leave game!\n");
		D3_click_ui(6751055690528634054);
		ii = 0;
	}

	return;
	//s->text = old;
	//draw_text();

	static int i = 0;
	static char **text = NULL;
	FILE *f;
	char *mode = i ? "a" : "w";
	if (fopen_s(&f, "C:\\Users\\gonzoj\\Desktop\\D3DrawText.txt", mode)) return;
	int j;
	for (j = 0; j < i; j++) {
		if (!strcmp(text[j], s->text)) break;
	}
	if (!i || j == i) {
		text = (char **) realloc(text, (i + 1) * sizeof(char *));
		text[i++] = _strdup(s->text);
	} else {
		fclose(f);
		return;
	}
	fprintf(f, "############# (%i) %s #############\n", i, s->text);
	dump_arg1(f, ft);
	dump_arg2(f, s);
	float p_coords[2];
	D3_c_convert_to_pixel(coords, p_coords, 1);
	DWORD p_c[2] = { (DWORD) p_coords[0], (DWORD) p_coords[1] };
	float w_coords[2];
	D3_c_convert_to_world(p_c, w_coords); 
	fprintf(f, "x/y: %f/%f - %f/%f - %f/%f\n", coords[0], coords[1], p_coords[0], p_coords[1], w_coords[0], w_coords[1]);
	fprintf(f, "color: %08X\n", color);
	fprintf(f, "%08X\n", *arg5);
	fprintf(f, "%i\n", arg6);
	fprintf(f, "%f\n", arg7);
	fprintf(f, "%i\n", arg8);
	dump_arg9(f, info);
	fprintf(f, "%08X\n", arg10);
	fprintf(f, "\n");
	fclose(f);
}

DWORD draw_text_patch = 0x0099B977;
DWORD draw_text_patch2 = 0x0094A764;
DWORD draw_text_patch3 = 0x0099B926;

void __declspec(naked) draw_text_stub2() {
	__asm
	{
		call draw_text
		pop ebx
		mov esp, ebp
		pop ebp
		ret
	}
}

void __declspec(naked) draw_text_stub() {
	__asm
	{
		call draw_text
		mov esp, ebp
		pop ebp
		retn 8
	}
}

// esi holding actor might be a coincidence actually..
void __declspec(naked) __cdecl D3_load_marker_stub(DWORD actor, int unk1, char *marker, DWORD *buf) {
	__asm
	{
		push esi
		mov esi, DWORD PTR DS:[ESP+0x8]
		mov eax, DWORD PTR DS:[ESP+0xC]
		mov ecx, DWORD PTR DS:[ESP+0x10]
		mov edx, DWORD PTR DS:[ESP+0x14]
		push edx
		push ecx
		push eax
		call D3_load_marker
		add esp, 0xC
		pop esi
		ret
	}
}

void __declspec(naked) __stdcall D3_transform_coords_stub(float *w_coords, float *m_coords) {
	__asm
	{
		mov ecx, p_this
		jmp D3_transform_coords
	}
}

void __declspec(naked) __stdcall D3_draw_marker_stub(int unk1, DWORD *buf, float *m_coords, float f, int unk2, struct mmap_actor_info *info, DWORD id, int unk4, DWORD unk5) {
	__asm
	{
		mov ecx, p_this
		jmp D3_draw_marker
	}
}

void __cdecl update_minimap_intercept(struct mmap_scene *scene, float x, float y, float sight_radius, DWORD layer) {
	float width = abs(scene->start.x - scene->end.x);
	float height = abs(scene->start.y - scene->end.y);
	sight_radius = max(width, height) * sqrt(2.0f);
	D3_update_minimap(scene, 0.0f, 0.0f, sight_radius, layer);
}

void set_actor_info_color(struct mmap_actor_info *info, char *color) {
	if (strstr(info->text, "{c:")) memcpy(info->text + 5, color, strlen(color));
}

typedef DWORD __cdecl D3_get_icon_id_t (char *name); D3_get_icon_id_t *D3_get_icon_id = (D3_get_icon_id_t *) 0x00EB6440;

struct icon { // sizeof 0x4C
	char name[0x44];
	DWORD buf[2];
};

void iterate_marker_icons() {
	FILE *f;
	if (fopen_s(&f, "C:\\Users\\gonzoj\\Desktop\\D3Icons.txt", "w")) return;

	/*char *name[] = {
		"MarkerLocalPlayer",
		"MarkerRedTeam"
	};*/

	// start MarkerLocalPlayer
	// end MarkerQuestion

	//int j;
	//for (j = 0; j < sizeof(name) / sizeof(char *); j++){
	//DWORD id = D3_get_icon_id(name[j]); // 0x8765A43A
	DWORD id = 0x8765A43A;
	DWORD *eax = (DWORD *) 0x014D8730;
	eax = (DWORD *) eax[2];
	DWORD addr = id & 0xFF;
	addr ^= 0x811C9DC5;
	addr *= 0x1000193;
	addr ^= ((id & 0xFF00) >> 8);
	addr *= 0x1000193;
	addr ^= ((id & 0xFF0000) >> 16);
	addr *= 0x1000193;
	addr ^= ((id & 0xFF000000) >> 24);
	addr *= 0x1000193;
	addr &=  0x7F; // [eax + 68]
	eax = (DWORD *) (((DWORD) eax) + 0x28);
	eax = (DWORD *) eax[2];
	DWORD *ptr = (DWORD *) (addr * 4 + ((DWORD) eax));
	addr = *ptr;

	//fprintf(f, "(%s) list at %08X:\n\n", name[j], addr);
	struct icon *icons;
	DWORD *icon;
	for (icon = (DWORD *) addr; icon; icon = (DWORD *) *icon) {
		DWORD *buf = (DWORD *) icon[2];
		if (!strcmp((const char *) icon[2], "MarkerLocalPlayer")) icons = (struct icon *) icon[2];
		//fprintf(f, "%s - %08X %08X\n", icon[2], buf[11], buf[12]);
	}
	//fprintf(f, "\n");
	//}

	int i;
	for (i = 0; 1; i++) {
		fprintf(f, "%i: %s - %08X %08X\n", (i + 1), icons[i].name, icons[i].buf[0], icons[i].buf[1]);
		if (!strcmp(icons[i].name, "MarkerQuestion")) break;
	}

	fclose(f);
}

void draw_all_markers(float *f) {
	float x = 3135.0f;
	float y = 2825.0f;
	float s = 50.0f;
	DWORD buf[2];
	buf[0] = 0x9084;
	struct mmap_actor_info info;
	D3_get_player_info(&info);
	int i = 0, j = 0;
	for (buf[1] = 0; buf[1] < 0x29; buf[1]++) {
		float w[2], m[2];
		w[0] = x + i * s;
		w[1] = y + j * s;
		D3_transform_coords_stub(w, m);
		D3_draw_marker_stub(0x8916, buf, m, *f, 0, &info, 0x77C20006, 0x0FF, 0);
		i++;
	}
}

void dump_acds() {
	FILE *f;
	if (fopen_s(&f, "C:\\Users\\gonzoj\\Desktop\\D3ACDs.txt", "w")) return;
	ACDManager *man = *(***obj_man)->ACDman;
	int i;
	for (i = 0; i < man->num; i++) {
		struct ACD unit = (*(man->unit))[i];
		if (unit.type == (DWORD) -1) continue;
		if (unit.type == 2 && unit.owner_id == 0xFFFFFFFF) {
			char buf[512];
			sprintf_s(buf, "trying to get info for %08X %s", unit.id, unit.unk1);
			D3_print_message(buf);
			struct acd_info info;
			D3_get_item_info(&info, &unit, 1, 0);
			fprintf(f, "%s ", info.string);
			//D3_free_actor_info((mmap_actor_info *) &info);
		}
		fprintf(f, "(%s): type %i (0x%08X)\n", unit.unk1, unit.type, *(man->unit) + i * sizeof(struct ACD));
		int j;
		for (j = 0; j < sizeof(struct ACD) / sizeof(DWORD); j++) {
			fprintf(f, "%08X (+%02X)\n", ((DWORD *)&unit)[j], j * 4);
		}
		fprintf(f, "\n");
	}
	fclose(f);
}

void __fastcall draw_monsters(float *f) {
	//draw_all_markers(f);

	ACDManager *man = *(***obj_man)->ACDman;
	DWORD buf[2];
	int j = 0;
	if (!D3_load_marker(0x01, "MarkerRedTeam", buf)) return;
	int i;
	for (i = 0; i < man->num; i++) {
		float w[2], m[2];
		struct ACD unit = (*(man->unit))[i];
		if (unit.type != 1) continue;
		w[0] = unit.x, w[1] = unit.y;
		D3_transform_coords_stub(w, m);
		struct mmap_actor_info info;
		DWORD actor = D3_get_actor(unit.actor_id);
		if (actor == NULL) continue;
		//D3_load_marker_stub(actor, 0x01, "MarkerFriendlyPlayer", buf);
		D3_get_actor_info(&info, actor);
		set_actor_info_color(&info, "FF0000");
		//buf[1] = j++;
		DWORD shader = D3_get_shader(D3_get_shader_key(0x20B), 1); // 0x8916
		D3_draw_marker_stub(shader, buf, m, *f, 0, &info, unit.id, 0x0FF, 0);
		D3_free_actor_info(&info);
	}
}

void __declspec(naked) patch_stub() {
	__asm 
	{
		mov p_this, edi
		call draw_monsters
		pop edi
		pop esi
		pop ebx
		mov esp, ebp
		pop ebp
		retn 4
	}
}

void __declspec(naked) draw_markers_stub() {
	__asm
	{
		mov p_this, edi
		mov ecx, dword ptr ds:[ebp+8]
		call draw_monsters
		mov esp, ebp
		pop ebp
		retn 8
	}
}

struct _attribute {
	DWORD unk1[2];
	int num;
	DWORD unk2[6];
	char *name;
};

void dump_attributes() {
	FILE *f;
	if (fopen_s(&f, "C:\\Users\\gonzoj\\Desktop\\D3Attributes.txt", "w")) return;
	struct _attribute *atts = (struct _attribute *) 0x014C95C8;
	int i;
	for (i = 0; i < 0x336; i++) {
		fprintf(f, "%s: %i %03X %08X %08X\n", atts[i].name, atts[i].num, atts[i].num, atts[i].num | 0xFFFFF000, atts[i].num | 0xFFFFFC00);
	}
	fclose(f);
}

typedef void D3_dump_ui_map_t (void); D3_dump_ui_map_t *D3_dump_ui_map = (D3_dump_ui_map_t *) 0x00932EF0;

typedef BYTE * __cdecl D3_get_ui_by_id_t (__int64 *id); D3_get_ui_by_id_t *D3_get_ui_by_id = (D3_get_ui_by_id_t *) 0x00930930;
typedef void __cdecl D3_click_handler_t (BYTE *); D3_click_handler_t *D3_click_handler;
typedef void __stdcall D3_ui_action_t (DWORD *id, DWORD *unk1); D3_ui_action_t *D3_ui_action = (D3_ui_action_t *) 0x00B4AFA0; // ui.vtable + 0x20 actually calls this function

void __declspec(naked) __stdcall D3_ui_action_stub(void *t, DWORD *id, DWORD *unk1) {
	__asm
	{
		pop eax
		pop ecx
		push eax
		jmp D3_ui_action
	}
}

typedef void __stdcall D3_ui_click_t (void *t, DWORD *unk, DWORD *action); D3_ui_click_t *D3_ui_click; // 0x00B4B700 for return to game

void __declspec(naked) __stdcall D3_ui_click_stub(void *t, DWORD *unk, DWORD *action) {
	__asm
	{
		pop eax
		pop ecx
		push eax
		jmp D3_ui_click
	}
}

typedef BOOL __cdecl D3_ui_do_t (__int64 *id, DWORD *unk, DWORD *action); D3_ui_do_t *D3_ui_do = (D3_ui_do_t *) 0x00932670;

typedef void __cdecl D3_set_active_ui_t (BYTE *ui); D3_set_active_ui_t *D3_set_active_ui = (D3_set_active_ui_t *) 0x00931770; // vtable + 0x30

typedef void __stdcall D3_bla_t (DWORD unk1, DWORD unk2); D3_bla_t *D3_bla; // __thiscall

void __declspec(naked) __stdcall D3_bla_stub(void *t, DWORD, DWORD) {
	__asm
	{
		pop eax
		pop ecx
		push eax
		jmp D3_bla
	}
}

void D3_click_ui(__int64 id) {
	BYTE *vtable = D3_get_ui_by_id(&id);
	if ((void *) vtable == &id) return;
	BYTE *ui = vtable + 0x30;

	BYTE **omg = (BYTE **) vtable;
	D3_bla = *(D3_bla_t **)&omg[0][0x90];
	//D3_bla_stub(vtable, 2, 0);
	//D3_set_active_ui(ui);
	//D3_bla_stub(vtable, 0, 0);

	D3_click_handler = *(D3_click_handler_t **)&vtable[0x55C];
	if (!D3_click_handler) {
		printf("click handler not set\n");
	} else {
		D3_click_handler(ui);
	}

	//DWORD action = 1;
	//D3_ui_action_stub(vtable, &action, 0);
	//DWORD action = 7;
	//D3_ui_action_stub(vtable, &action, 0);*/

	/*DWORD **vtable = (DWORD **) D3_get_ui_by_id(&id);
	if ((void *) vtable == &id) return;

	D3_ui_click = (D3_ui_click_t *) vtable[0][8];
	printf("D3_ui_click %08X\n", (DWORD) D3_ui_click);

	D3_set_active_ui(((BYTE *) vtable) + 0x30);

	//DWORD action = 4, unk = 1;
	//D3_ui_click_stub((void *) vtable, &unk, &action);
	//action = 5, unk = 1;
	//D3_ui_click_stub((void *) vtable, &unk, &action);
	//action = 6, unk = 1;
	//D3_ui_click_stub((void *) vtable, &unk, &action);
	DWORD action = 7, unk = 1;
	//D3_ui_click_stub((void *) vtable, &unk, &action);
	D3_ui_action_stub(vtable, &action, &unk);*/

	/*action = 5, unk = 1;
	D3_ui_do(&id, &unk, &action);
	action = 6, unk = 1;
	D3_ui_do(&id, &unk, &action);
	action = 7, unk = 1;
	D3_ui_do(&id, &unk, &action);*/
}

//typedef void __cdecl D3_leave_game_t (BYTE *ui); D3_leave_game_t *D3_leave_game = (D3_leave_game_t *) 0x00A87610;
typedef void __stdcall D3_leave_game_t (DWORD); D3_leave_game_t *D3_leave_game = (D3_leave_game_t *) 0x00CF9040; // __thiscall, and it's actually this->vtable + 0x3C
typedef BYTE *** __stdcall D3_get_some_globl_t (); D3_get_some_globl_t *D3_get_some_globl = (D3_get_some_globl_t *) 0x0095E1C0;

void __declspec(naked) __stdcall D3_leave_game_stub(void *t, DWORD arg) {
	__asm {
		pop eax
		pop ecx
		push eax
		jmp D3_leave_game
	}
}

void leave_game() { // will have 10 sec delay in vanilla I guess
	// only works if button has been pressed before
	/*__int64 id = 6751055690528634054;
	BYTE *vtable = D3_get_ui_by_id(&id);
	if ((void *) vtable == &id) return;
	BYTE *ui = vtable + 0x30;
	D3_leave_game(ui);*/

	BYTE ***eax = D3_get_some_globl();
	BYTE **esi = eax[4]; //eax[0x10];
	BYTE *ecx = esi[6]; //esi[0x18];
	D3_leave_game_stub(ecx, 0);
}

void dump_power_ids() {
	BYTE *powers = (BYTE *) 0x01531480;
	DWORD eax = *(DWORD *)&powers[0x48];
	DWORD edx = *(DWORD *)0x14ECEF8;
	eax *= 70;
	eax = edx + eax + 8;
};

float player_sight_radius = 65.0f;

void patch_bytes(BYTE opc, DWORD addr, DWORD func, DWORD len) {
	BYTE *code = (BYTE *) malloc(len);
	memset(code, 0x90, len);
	code[0] = opc;
	if (func) {
		*(DWORD *)&code[1] = (func - (addr + 5));
	}
	DWORD old;
	VirtualProtect((LPVOID) addr, len, PAGE_READWRITE, &old);
	memcpy((void *) addr, code, len);
	VirtualProtect((LPVOID) addr, len, old, &old);
	free(code);
}

#include <WinUser.h>

BOOL APIENTRY DllMain(HMODULE h, DWORD r, LPVOID reserved) {
	if (r == 0x01) {
		// create console
		AllocConsole();
		freopen("CONOUT$", "w", stdout);
		printf("test\n");

		// init
		// remove this
		//p_this = D3_get_layer(minimap_layer);

		// install hooks
		//memcpy(backup, (void *) patch, 9);
		//patch_bytes(0xE9, patch, (DWORD) patch_stub, 9);

		// remove these 3
		//patch_bytes(0xE9, draw_markers_patch, (DWORD) draw_markers_stub, 6);
		//patch_bytes(0xE8, mmap_update_patch, (DWORD) update_minimap_intercept, 5);
		//patch_bytes(0x90, active_scene_check_patch, NULL, 6);

		//patch_bytes(0xE9, draw_text_patch, (DWORD) draw_text_stub, 6);
		//patch_bytes(0xE9, draw_text_patch2, (DWORD) draw_text_stub2, 5);

		// remove this
		patch_bytes(0xE8, draw_text_patch3, (DWORD) draw_text_intercept, 5);

		/*DWORD test = D3_get_layer(minimap_layer);
		char str[512];
		sprintf_s(str, 512, "%08X", &test);
		MessageBoxA(NULL, str, NULL, MB_OK);*/

		//iterate_marker_icons();
		//dump_attributes();

		//D3_dump_ui_map();

		//dump_acds();

		//D3_print_message("[{c:ffffff00}feedface{/c}] successfully injected");

		//D3_click_ui(6751055690528634054); // leave game
		//D3_click_ui(6390307137723323319); // resume game
		//leave_game();
	}

	else if (r == 0x00) {
		// remove hooks
		/*DWORD old;
		VirtualProtect((LPVOID) patch, 9, PAGE_READWRITE, &old);
		memcpy((void *) patch, backup, 9);
		VirtualProtect((LPVOID) patch, 9, old, &old);*/

		fclose(stdout);
		FreeConsole();
	}

	return TRUE;
}