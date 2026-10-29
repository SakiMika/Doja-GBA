#pragma once
#include <cstdint>
struct Image { int w,h; const uint8_t* pixels; };
struct Command { uint8_t op; uint16_t size; const uint8_t* data; };
struct Page { int h[16]; int count; const Command* commands; };
struct Event { int count; const Page* pages; };
struct Map { int h[10]; int music; const uint8_t* cells; };
extern const Image images[18][32];
extern const uint16_t palette[256];
extern const Event events[682];
extern const Map maps[30];
extern const uint8_t mapsets[10][17];
extern const uint8_t collision[8][32][5];
extern const uint8_t actor_sprites[6];
extern const char* const actor_names[6];
extern const char* const item_names[4];
extern const int initial_party[3];
extern const int initial_party_count;
extern const int initial_map, initial_x, initial_y, initial_event;
