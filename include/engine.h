#pragma once
#include "assets.h"
struct Entity { int x,y,px,py,dir,event,page,timer; };
struct GameState {
 uint32_t signature,version,checksum;
 uint8_t flags[256]; uint8_t inventory[4];
 int map,x,y,px,py,dir,party[3],party_count;
 int trail_x[3],trail_y[3],trail_dir[3];
 int event,page,pc,owner,portrait,wait,route_index,route_active,music;
 int entity_count; Entity entities[256];
 int frames,flash,flash_r,flash_g,flash_b;
 bool introduction_done;
};
class Engine {
public:
 GameState s{};
 const char* dialogue=nullptr;
 const Command* choice=nullptr;
 bool save_requested=false;
 bool error=false;
 int error_opcode=-1;
 void start();
 void tick(int direction,bool confirm);
 void choose(int option);
 void restore();
 const Page* active() const;
 void start_event(int id,int page,int owner);
 void load_map(int id,int x,int y);
 void refresh();
 bool passable(int x,int y,int ignore=-1) const;
private:
 void script();
 void end_event();
 void move_player(int x,int y);
 bool trigger(int x,int y,int kind,bool solid);
 int skip_branch(int start,int open,int middle,int end) const;
 bool meets(const Page& p) const;
};
