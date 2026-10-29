#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_regular_bg_item.h"
#include "bn_regular_bg_ptr.h"
#include "bn_regular_bg_tiles_ptr.h"
#include "bn_sram.h"
#include "bn_color.h"
#include "engine.h"
#include "vfont.h"
#include "bn_sound_items.h"
#include "bn_sound_handle.h"
#include "bn_optional.h"
#include <cstring>
#include <cstdio>
#include <algorithm>
namespace {
alignas(4) uint8_t frame_buffers[2][38400] BN_DATA_EWRAM_BSS;
uint8_t* frame=frame_buffers[0];
int frame_index=0;
bn::regular_bg_map_cell cells[1024] BN_DATA_EWRAM_BSS;
bn::color colors[256] BN_DATA_EWRAM_BSS;
Engine game BN_DATA_EWRAM;
GameState saved BN_DATA_EWRAM_BSS;
int menu=-1,selection=0,choice_selection=0,text_ticks=0;
bool title=true;
constexpr int SLOT_COUNT=3,SLOT_STRIDE=9216;
static_assert(sizeof(GameState)<=SLOT_STRIDE && SLOT_COUNT*SLOT_STRIDE<=32768);
struct SlotSummary {bool used;int map;};
SlotSummary slots[SLOT_COUNT]{};
int title_selection=0,slot_selection=0,slot_return=-1,overwrite_selection=1;
const char* last_text=nullptr;
char notice[80]="";int notice_ticks=0;
void pixel(int x,int y,uint8_t c){if(unsigned(x)<240&&unsigned(y)<160)frame[((y>>3)*30+(x>>3))*64+(y&7)*8+(x&7)]=c;}
BN_CODE_IWRAM __attribute__((target("arm"))) void rect(int x,int y,int w,int h,uint8_t c){
 int start=std::max(x,0),end=std::min(x+w,240);
 for(int yy=std::max(y,0);yy<std::min(y+h,160);++yy){int xx=start;while(xx<end){int n=std::min(8-(xx&7),end-xx);uint8_t* dest=frame+((yy>>3)*30+(xx>>3))*64+(yy&7)*8+(xx&7);for(int k=0;k<n;++k)dest[k]=c;xx+=n;}}
}
void box(int x,int y,int w,int h){rect(x,y,w,h,255);rect(x+1,y+1,w-2,h-2,1);}
// Decode UTF-8 so all Vietnamese glyphs in the supplied font are usable.
unsigned next_codepoint(const unsigned char*& p){
 unsigned c=*p++;if(c<128)return c;
 int continuation=c>=0xf0?3:c>=0xe0?2:c>=0xc0?1:0;
 if(!continuation){return '?';}
 c&=(1u<<(6-continuation))-1;
 for(int i=0;i<continuation;++i){if((*p&0xc0)!=0x80)return '?';c=(c<<6)|(*p++&63);}
 return c;
}
int glyph_index(unsigned c){
 if(c>=32&&c<=126)return int(c)-32;
 for(int i=95;i<VFONT_COUNT;++i)if(vfont_codepoints[i]==c)return i;
 return '?'-32;
}
void text(const char* str,int x,int y,int maxw=228,int limit=999){
 int xx=x,yy=y,count=0;const auto* p=reinterpret_cast<const unsigned char*>(str);
 while(*p&&count<limit){unsigned c=next_codepoint(p);
  if(c=='\n'){xx=x;yy+=VFONT_LINE_STEP;continue;}
  int index=glyph_index(c),width=vfont_widths[index];
  if(xx+width>x+maxw){xx=x;yy+=VFONT_LINE_STEP;}
  for(int gy=0;gy<14;++gy){unsigned row=vfont_rows[index][gy];for(int gx=0;gx<8;++gx)if(row&(128u>>gx))pixel(xx+gx,yy+gy,255);}
  xx+=width;++count;
 }
}
BN_CODE_IWRAM __attribute__((target("arm"))) void blit(const Image& im,int x,int y,int sx=0,int sy=0,int w=-1,int h=-1,bool opaque=false){
 if(!im.pixels){return;}
 if(w<0){w=im.w;}
 if(h<0){h=im.h;}
 int x0=std::max(0,-x),y0=std::max(0,-y),x1=std::min(w,240-x),y1=std::min(h,160-y);
 for(int yy=y0;yy<y1;++yy){int iy=sy+yy;if(iy<0||iy>=im.h)continue;int xx=x0;
  while(xx<x1){int dx=x+xx,dy=y+yy;int n=std::min(8-(dx&7),x1-xx);uint8_t* dest=frame+((dy>>3)*30+(dx>>3))*64+(dy&7)*8+(dx&7);
   const uint8_t* src=im.pixels+iy*im.w+sx+xx;
   if(opaque){std::memcpy(dest,src,n);}else{for(int k=0;k<n;++k){if(sx+xx+k>=0&&sx+xx+k<im.w&&src[k]){dest[k]=src[k];}}}
   xx+=n;
  }
 }
}
void scaled(const Image& im,int x,int y,int w,int h){if(!im.pixels)return;if(im.w==w&&im.h==h){blit(im,x,y);return;}for(int yy=0;yy<h;++yy)for(int xx=0;xx<w;++xx){auto c=im.pixels[(yy*im.h/h)*im.w+(xx*im.w/w)];if(c)pixel(x+xx,y+yy,c);}}
uint32_t checksum(const GameState& s){auto* p=(const uint8_t*)&s;uint32_t h=2166136261u;for(unsigned i=12;i<sizeof(s);++i)h=(h^p[i])*16777619u;return h;}
bool valid(const GameState& s){return s.signature==0x43504231 && s.version==1 && s.checksum==checksum(s)&&s.map>=0&&s.map<30&&s.entity_count>=0&&s.entity_count<=256&&s.party_count>0&&s.party_count<=3&&s.event>=-1&&s.event<682;}
void refresh_slots(){
 for(int slot=0;slot<SLOT_COUNT;++slot){bn::sram::read_offset(saved,slot*SLOT_STRIDE);slots[slot]={valid(saved),saved.map};}
}
void open_slots(bool writing,int return_menu){
 refresh_slots();notice_ticks=0;slot_selection=0;slot_return=return_menu;menu=writing?2:3;
}
void save_slot(int slot){
 saved=game.s;saved.checksum=checksum(saved);
 bn::sram::write_offset(saved,slot*SLOT_STRIDE);
 static GameState verify BN_DATA_EWRAM_BSS;
 bn::sram::read_offset(verify,slot*SLOT_STRIDE);
 bool success=valid(verify)&&verify.checksum==saved.checksum;
 slots[slot]={success,verify.map};
 if(success){std::snprintf(notice,sizeof(notice),"Saved to slot %d.",slot+1);menu=-1;}
 else{std::snprintf(notice,sizeof(notice),"Save failed. Please retry.");menu=2;}
 notice_ticks=90;
}
void load_slot(int slot){
 bn::sram::read_offset(saved,slot*SLOT_STRIDE);
 if(valid(saved)){game.s=saved;game.restore();title=false;menu=-1;last_text=nullptr;text_ticks=0;choice_selection=0;}
 else{std::snprintf(notice,sizeof(notice),"Slot %d is empty or invalid.",slot+1);notice_ticks=90;}
}
void world(){const auto& s=game.s;const auto& m=maps[s.map];int camx=s.px+8-120,camy=s.py+8-80;int tileset=mapsets[m.h[0]][0];
 for(int y=0;y<11;++y)for(int x=0;x<16;++x){int tx=(camx>=0?camx/16:(camx-15)/16)+x;int ty=(camy>=0?camy/16:(camy-15)/16)+y;int tile=m.h[8];
  int mx=tx,my=ty;if(m.h[2]&1)mx=(mx%m.h[4]+m.h[4])%m.h[4];if(m.h[2]&2)my=(my%m.h[5]+m.h[5])%m.h[5];
  if(mx>=0&&my>=0&&mx<m.h[4]&&my<m.h[5])tile=m.cells[(my*m.h[4]+mx)*2];
  if(tile>=0&&tile<32)blit(images[11][tileset],tx*16-camx,ty*16-camy,tile%4*16,tile/4*16,16,16,true);
 }
 struct Draw {int y,x,image,dir,pose;};static Draw draw[259] BN_DATA_EWRAM_BSS;int count=0;int anim=(s.frames/8)%4;int pose=anim==0||anim==2?1:anim==1?2:0;
 for(int i=0;i<s.entity_count;++i){const auto& e=s.entities[i];if(e.page<0)continue;const auto& p=events[e.event].pages[e.page];if(!p.h[9])continue;draw[count++]={e.py+16,e.px,mapsets[m.h[0]][1+p.h[11]],e.dir,p.h[10]?pose:p.h[13]};}
 for(int i=s.party_count-1;i>=0;--i){int x=i?s.trail_x[i-1]:s.px;int y=i?s.trail_y[i-1]:s.py;int dir=i?s.trail_dir[i-1]:s.dir;draw[count++]={y+16,x,actor_sprites[s.party[i]],dir,pose};}
 std::sort(draw,draw+count,[](const Draw& a,const Draw& b){return a.y<b.y;});
 for(int i=0;i<count;++i){const auto& d=draw[i];if(d.image>=0&&d.image<32)blit(images[13][d.image],d.x-camx,d.y-camy-32,d.dir%2*48+d.pose*16,d.dir/2*32,16,32);}
}
void render(){
 static uint8_t world_cache[38400] BN_DATA_EWRAM_BSS;
 static int previous_map=-1,previous_frame=-100;
 static int previous_x=-999,previous_y=-999,previous_pc=-999;
 static unsigned previous_actors=0;
 std::memset(frame,1,38400);
 if(title){scaled(images[14][0],40,0,160,160);box(26,102,188,52);text("CORPSE PARTY - CHAPTER 1",36,107);text("New Game",91,121);text("Load",91,138);text(">",77,121+title_selection*17);}
 else{
  unsigned actor_hash=unsigned(game.s.dir+game.s.party_count*4);
  for(int i=0;i<game.s.entity_count;++i){const auto& e=game.s.entities[i];actor_hash=actor_hash*33+unsigned(e.px+e.py*512+e.page*262144+e.dir*1048576);}
  if(previous_actors!=actor_hash||previous_map!=game.s.map||previous_x!=game.s.px||previous_y!=game.s.py||previous_frame/8!=game.s.frames/8||previous_pc!=game.s.pc){
   world();std::memcpy(world_cache,frame,38400);previous_map=game.s.map;previous_x=game.s.px;previous_y=game.s.py;previous_frame=game.s.frames;previous_pc=game.s.pc;previous_actors=actor_hash;
  }else{std::memcpy(frame,world_cache,38400);}
  if(game.s.flash){uint8_t nearest=1;int best=999999;for(int i=1;i<256;++i){int c=palette[i];int dr=((c&31)<<3)-game.s.flash_r,dg=(((c>>5)&31)<<3)-game.s.flash_g,db=(((c>>10)&31)<<3)-game.s.flash_b;int delta=dr*dr+dg*dg+db*db;if(delta<best){best=delta;nearest=i;}}std::memset(frame,nearest,38400);}
  if(game.dialogue){box(2,100,236,58);int x=8,w=224;if(game.s.portrait>=0&&game.s.portrait<32){scaled(images[15][game.s.portrait],5,113,42,42);x=52;w=180;}text(game.dialogue,x,103,w,text_ticks);if(text_ticks>=int(std::strlen(game.dialogue)))text("A",226,143);}
  if(game.choice){box(43,91,154,56);char option[9];for(int i=0;i<2;++i){std::memcpy(option,game.choice->data+i*8,8);option[8]=0;text(option,64,101+i*19);}text(">",51,101+choice_selection*19);}
  if(game.error){box(4,35,232,70);text("ENGINE ERROR - STOPPED",10,42);char b[64];std::snprintf(b,sizeof(b),"Event %d / command %d",game.s.event,game.s.pc);text(b,10,56);std::snprintf(b,sizeof(b),"Opcode: %d",game.error_opcode);text(b,10,70);text("START: menu",10,84);}
 }
 if(menu==0){box(45,19,150,123);const char* options[]={"Resume","Save","Load","Items","Title screen"};for(int i=0;i<5;++i)text(options[i],67,31+i*20);text(">",54,31+selection*20);}
 if(menu==1){box(15,25,210,110);text("ITEMS",25,33);bool any=false;int line=0;for(int i=0;i<4;++i)if(game.s.inventory[i]){char b[48];std::snprintf(b,sizeof(b),"%s x%d",item_names[i],game.s.inventory[i]);text(b,25,51+line++*13);any=true;}if(!any)text("No items.",25,52);text("B: Back",25,119);}
 if(menu==2||menu==3||menu==4){
  box(20,16,200,132);text(menu==3?"LOAD GAME":"SAVE GAME",36,22);
  for(int i=0;i<SLOT_COUNT;++i){char label[48];
   if(slots[i].used)std::snprintf(label,sizeof(label),"Slot %d   Map %02d",i+1,slots[i].map+1);
   else std::snprintf(label,sizeof(label),"Slot %d   Empty",i+1);
   text(label,44,46+i*26);
  }
  text(">",30,46+slot_selection*26);text("A: Select    B: Back",36,127);
 }
 if(menu==4){box(25,47,190,77);text("Overwrite this slot?",38,53);text("Yes",61,91);text("No",145,91);text(">",overwrite_selection?132:48,91);}
 if(notice_ticks){box(8,4,224,19);text(notice,14,8);}
}
}
int main(){
 bn::core::init();bn::core::set_skip_frames(1);for(int i=0;i<256;++i)colors[i]=bn::color(palette[i]&31,(palette[i]>>5)&31,(palette[i]>>10)&31);
 for(int y=0;y<32;++y)for(int x=0;x<32;++x)cells[y*32+x]=(y<20&&x<30)?y*30+x:0;
 bn::regular_bg_item item(bn::span<const bn::tile>((const bn::tile*)frame,1200),bn::span<const bn::color>(colors,256),bn::bpp_mode::BPP_8,cells[0],bn::size(32,32));
 auto bg=item.create_bg(8,48);auto tiles=bg.tiles();refresh_slots();game.start();
 const bn::sound_item tracks[]={bn::sound_items::track_00,bn::sound_items::track_01,bn::sound_items::track_02,bn::sound_items::track_03,bn::sound_items::track_04,bn::sound_items::track_05,bn::sound_items::track_06,bn::sound_items::track_07,bn::sound_items::track_08,bn::sound_items::track_09,bn::sound_items::track_10,bn::sound_items::track_11};
 bn::optional<bn::sound_handle> music;int playing=-1;bool muted=false;
 while(true){
  if(bn::keypad::select_pressed())muted=!muted;
  int track=title||muted?-1:game.s.music;
  if(track!=playing){if(music&&music->active())music->stop();music.reset();playing=track;}
  if(track>=0&&track<12&&(!music||!music->active()))music=tracks[track].play(bn::fixed(0.6));
  bool a=bn::keypad::a_pressed(),b=bn::keypad::b_pressed(),start=bn::keypad::start_pressed();
  if(notice_ticks)--notice_ticks;
  if(menu==2||menu==3){
   if(b){menu=slot_return;}
   else if(bn::keypad::up_pressed()){slot_selection=(slot_selection+SLOT_COUNT-1)%SLOT_COUNT;}
   else if(bn::keypad::down_pressed()){slot_selection=(slot_selection+1)%SLOT_COUNT;}
   else if(a){
    if(menu==3){load_slot(slot_selection);}
    else if(slots[slot_selection].used){menu=4;overwrite_selection=1;}
    else{save_slot(slot_selection);}
   }
  }
  else if(menu==4){
   if(b){menu=2;}
   else if(bn::keypad::left_pressed()||bn::keypad::right_pressed()||bn::keypad::up_pressed()||bn::keypad::down_pressed()){overwrite_selection^=1;}
   else if(a){if(overwrite_selection==0){save_slot(slot_selection);}else{menu=2;}}
  }
  else if(title){
   if(bn::keypad::up_pressed()||bn::keypad::down_pressed()){title_selection^=1;}
   else if(a||start){if(title_selection==0){game.start();title=false;last_text=nullptr;text_ticks=0;choice_selection=0;menu=-1;}else{open_slots(false,-1);}}
  }
  else if(menu>=0){if(menu==1){if(b)menu=0;}else{
   if(b||start)menu=-1;else if(bn::keypad::up_pressed())selection=(selection+4)%5;else if(bn::keypad::down_pressed())selection=(selection+1)%5;
   else if(a){switch(selection){case 0:menu=-1;break;case 1:open_slots(true,0);break;case 2:open_slots(false,0);break;case 3:menu=1;break;case 4:title=true;title_selection=0;menu=-1;break;default:break;}}
  }}
  else if(start){menu=0;selection=0;}
  else if(game.save_requested){game.save_requested=false;open_slots(true,-1);}
  else if(game.choice){if(bn::keypad::up_pressed()||bn::keypad::down_pressed())choice_selection^=1;if(b){game.choose(1);choice_selection=0;}else if(a){game.choose(choice_selection);choice_selection=0;}}
  else{
   int direction=bn::keypad::up_held()?0:bn::keypad::right_held()?1:bn::keypad::down_held()?2:bn::keypad::left_held()?3:-1;
   if(game.dialogue!=last_text){last_text=game.dialogue;text_ticks=0;}
   bool confirm=a;if(game.dialogue){text_ticks+=2;if(confirm&&text_ticks<int(std::strlen(game.dialogue))){text_ticks=999;confirm=false;}}
   game.tick(direction,confirm);
  }
  render();tiles.set_tiles_ref(bn::regular_bg_tiles_item(bn::span<const bn::tile>((const bn::tile*)frame,1200),bn::bpp_mode::BPP_8));tiles.reload_tiles_ref();bn::core::update();frame_index^=1;frame=frame_buffers[frame_index];
 }
}



