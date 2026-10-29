#include "engine.h"
#include <cassert>
#include <cstdio>
#include <cstring>
static void settle(Engine& g,int option){for(int t=0;t<20000&&g.s.event>=0;++t){g.tick(-1,true);if(g.choice)g.choose(option);if(g.save_requested)g.save_requested=false;assert(!g.error);}assert(g.s.event==-1);}
int main(){
 Engine g;g.start();assert(g.s.event==681);g.tick(-1,false);assert(g.dialogue&&std::strcmp(g.dialogue,"View the opening?")==0);g.tick(-1,true);g.tick(-1,false);assert(g.choice);g.choose(1);settle(g,1);assert(g.s.map==17&&g.s.x==8&&g.s.y==4&&g.s.flags[51]);
 // Every map validates its cell dimensions, tileset, event references and walkability boundaries.
 for(int m=0;m<30;++m){g.load_map(m,0,0);assert(!g.error);assert(g.s.entity_count<=256);assert(!g.passable(-1,0));assert(!g.passable(maps[m].h[4],0));}
 int tested=0,commands=0;
 // Execute both choice paths of every event page with real assets and a real owner where available.
 for(int id=0;id<682;++id)for(int p=0;p<events[id].count;++p)for(int option=0;option<2;++option){
  g.start();int owner=-1;bool found=false;
  for(int m=0;m<30&&!found;++m){g.load_map(m,0,0);for(int i=0;i<g.s.entity_count;++i)if(g.s.entities[i].event==id){owner=i;found=true;break;}}
  for(int f=0;f<256;++f)g.s.flags[f]=option;
  for(int i=0;i<4;++i)g.s.inventory[i]=option?1:0;
  g.start_event(id,p,owner);settle(g,option);assert(g.s.map>=0&&g.s.map<30);assert(g.s.x>=0&&g.s.x<maps[g.s.map].h[4]);assert(g.s.y>=0&&g.s.y<maps[g.s.map].h[5]);
  ++tested;commands+=events[id].pages[p].count;
 }
 // Snapshot/restore must preserve an in-flight dialogue command.
 g.start();g.tick(-1,false);GameState checkpoint=g.s;g.tick(-1,true);g.s=checkpoint;g.restore();g.tick(-1,false);assert(g.dialogue&&std::strcmp(g.dialogue,"View the opening?")==0);
 printf("PASS: 30 maps, %d event-page runs, %d command instances, opening skip, state restore.\n",tested,commands);
}
