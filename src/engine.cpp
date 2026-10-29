#include "engine.h"
#include <cstring>
#include <cstdlib>
static const int dx[]={0,1,0,-1},dy[]={-1,0,1,0};
static int u16(const uint8_t* d){return (d[0]<<8)|d[1];}
const Page* Engine::active() const {return s.event>=0 && s.event<682 ? &events[s.event].pages[s.page] : nullptr;}
bool Engine::meets(const Page& p) const {
 int required=0,met=0;
 if(p.h[1]){required|=1;if(s.flags[p.h[2]])met|=1;}
 if(p.h[3]){required|=2;if(p.h[4]==0 && p.h[5]<4 && s.inventory[p.h[5]])met|=2;}
 if(p.h[6])required|=4; // This chapter has no currency operations.
 return p.h[8]==0 ? (met&required)==required : (met&required)!=0;
}
void Engine::refresh(){
 for(int i=0;i<s.entity_count;++i){auto& e=s.entities[i];int selected=-1;const auto& ev=events[e.event];
  for(int p=ev.count-1;p>=0;--p)if(meets(ev.pages[p])){selected=p;break;}
  if(selected!=e.page){e.page=selected;e.timer=0;if(selected>=0)e.dir=ev.pages[selected].h[12];}
 }
}
void Engine::load_map(int id,int x,int y){
 if(id<0 || id>=30){error=true;return;}
 s.map=id;const auto& m=maps[id];
 if(x<0||y<0||x>=m.h[4]||y>=m.h[5]){error=true;return;}
 s.x=x;s.y=y;s.px=x*16;s.py=y*16;s.dir=2;s.entity_count=0;s.owner=-1;
 for(int yy=0;yy<m.h[5];++yy)for(int xx=0;xx<m.h[4];++xx){int v=m.cells[(yy*m.h[4]+xx)*2+1];if(v!=255){
  if(s.entity_count>=256||m.h[1]+v>=682){error=true;return;}
  s.entities[s.entity_count++]={xx,yy,xx*16,yy*16,2,m.h[1]+v,-1,0};
 }}
 for(int i=0;i<3;++i){s.trail_x[i]=s.px;s.trail_y[i]=s.py;s.trail_dir[i]=2;}
 s.music=m.music-1;refresh();
}
void Engine::start(){
 s={};s.signature=0x43504231;s.version=1;s.event=-1;s.owner=-1;s.portrait=-1;s.music=-1;
 s.party_count=initial_party_count;
 for(int i=0;i<s.party_count;++i)s.party[i]=initial_party[i];
 dialogue=nullptr;choice=nullptr;save_requested=false;error=false;
 load_map(initial_map,initial_x,initial_y);
 if(initial_event>=0){s.introduction_done=true;start_event(initial_event,0,-1);}
}
void Engine::start_event(int id,int page,int owner){
 if(id<0||id>=682||page<0||page>=events[id].count){error=true;return;}
 s.event=id;s.page=page;s.pc=0;s.owner=owner;s.route_active=0;s.route_index=0;s.wait=0;
 dialogue=nullptr;choice=nullptr;
}
void Engine::end_event(){s.event=-1;s.pc=0;s.portrait=-1;s.route_active=0;dialogue=nullptr;choice=nullptr;refresh();}
void Engine::restore(){dialogue=nullptr;choice=nullptr;save_requested=false;error=false;refresh();}
bool Engine::passable(int x,int y,int ignore) const {
 const auto& m=maps[s.map];if(x<0||y<0||x>=m.h[4]||y>=m.h[5])return false;
 int tile=m.cells[(y*m.h[4]+x)*2];int set=mapsets[m.h[0]][0];
 if(tile>=32||set>=8||!collision[set][tile][0])return false;
 for(int i=0;i<s.entity_count;++i){const auto& e=s.entities[i];if(i!=ignore && e.page>=0&&e.x==x&&e.y==y&&events[e.event].pages[e.page].h[15])return false;}
 return true;
}
void Engine::move_player(int x,int y){
 for(int i=2;i>0;--i){s.trail_x[i]=s.trail_x[i-1];s.trail_y[i]=s.trail_y[i-1];s.trail_dir[i]=s.trail_dir[i-1];}
 s.trail_x[0]=s.x*16;s.trail_y[0]=s.y*16;s.trail_dir[0]=s.dir;s.x=x;s.y=y;
}
bool Engine::trigger(int x,int y,int kind,bool solid){
 for(int i=0;i<s.entity_count;++i){auto& e=s.entities[i];if(e.page<0||e.x!=x||e.y!=y)continue;const auto& p=events[e.event].pages[e.page];
  if(p.h[0]==kind && bool(p.h[15])==solid && p.count){if(solid)e.dir=s.dir^2;start_event(e.event,e.page,i);return true;}}
 return false;
}
int Engine::skip_branch(int start,int open,int middle,int end) const {
 auto p=active();int depth=0;
 for(int i=start+1;i<p->count;++i){int op=p->commands[i].op;if(op==open)++depth;else if(op==end){if(depth==0)return i+1;--depth;}else if(op==middle&&depth==0)return i+1;}
 return p->count;
}
void Engine::choose(int option){if(!choice)return;if(option==0)++s.pc;else s.pc=skip_branch(s.pc,2,3,4);choice=nullptr;}
void Engine::script(){
 for(int budget=0;budget<64 && !error;++budget){
  const auto* p=active();if(!p)return;if(s.pc>=p->count){end_event();return;}
  const auto& c=p->commands[s.pc];const auto* d=c.data;
  switch(c.op){
   case 0:dialogue=reinterpret_cast<const char*>(d);return;
   case 1:s.portrait=d[0]?int(int8_t(d[1])):-1;++s.pc;break;
   case 2:choice=&c;return;
   case 3:s.pc=skip_branch(s.pc,2,-1,4);break;
   case 4:case 7:++s.pc;break;
   case 5:{bool value=d[0]==0 ? bool(s.flags[d[1]]) : (d[2]==0 && d[1]<4 && s.inventory[d[1]]);
    bool skip=d[3]==0?value:!value;
    s.pc=skip?skip_branch(s.pc,5,6,7):s.pc+1;break;}
   case 6:s.pc=skip_branch(s.pc,5,-1,7);break;
   case 12:if(d[1]==0&&d[0]<4){int n=s.inventory[d[0]]+(d[2]==0?int(d[3]):-int(d[3]));s.inventory[d[0]]=uint8_t(n<0?0:n>24?24:n);}++s.pc;break;
   case 13:++s.pc;save_requested=true;return;
   case 14:for(int i=d[1];i<=d[2];++i)s.flags[i]=d[0]==1;++s.pc;break;
   case 15:s.music=u16(d)-1;++s.pc;break;
   case 17:++s.pc;load_map(u16(d),d[2],d[3]);s.wait=2;return;
   case 29:{
    if(!s.route_active){s.route_active=1;s.route_index=0;}
    if(s.route_index>=d[2]){s.route_active=0;++s.pc;break;}
    int dir=d[3+s.route_index++];int facing=dir%4;
    if(d[0]==0){s.dir=facing;if(dir<4){int x=s.x+dx[facing],y=s.y+dy[facing];const auto& m=maps[s.map];if(x>=0&&y>=0&&x<m.h[4]&&y<m.h[5]&&(!d[1]||passable(x,y)))move_player(x,y);}}
    else if(d[0]==3&&s.owner>=0&&s.owner<s.entity_count){auto& e=s.entities[s.owner];e.dir=facing;if(dir<4){int x=e.x+dx[facing],y=e.y+dy[facing];const auto& m=maps[s.map];if(x>=0&&y>=0&&x<m.h[4]&&y<m.h[5]&&(!d[1]||passable(x,y,s.owner))){e.x=x;e.y=y;}}}
    s.wait=dir<4?8:1;return;}
   case 30:{int actor=d[1];if(actor>=6){error=true;return;}
    if(d[0]==0){if(s.party_count>1)for(int i=0;i<s.party_count;++i)if(s.party[i]==actor){for(int j=i+1;j<s.party_count;++j)s.party[j-1]=s.party[j];--s.party_count;break;}}
    else if(s.party_count<3){bool found=false;for(int i=0;i<s.party_count;++i)if(s.party[i]==actor)found=true;if(!found)s.party[s.party_count++]=actor;}++s.pc;break;}
   case 32:s.flash=2;s.flash_r=d[0];s.flash_g=d[1];s.flash_b=d[2];++s.pc;s.wait=2;return;
   default:error=true;error_opcode=c.op;return;
  }
 }
}
static int approach(int value,int target){return value<target?value+2:value>target?value-2:value;}
void Engine::tick(int direction,bool confirm){
 if(error)return;++s.frames;if(s.flash>0)--s.flash;
 bool moving=s.px!=s.x*16||s.py!=s.y*16;
 s.px=approach(s.px,s.x*16);s.py=approach(s.py,s.y*16);
 for(int i=0;i<s.entity_count;++i){auto& e=s.entities[i];e.px=approach(e.px,e.x*16);e.py=approach(e.py,e.y*16);}
 if(s.wait>0){--s.wait;return;}
 if(dialogue){if(confirm){dialogue=nullptr;++s.pc;}return;}
 if(choice||save_requested)return;
 if(s.event>=0){script();return;}
 if(moving){if(s.px==s.x*16&&s.py==s.y*16)trigger(s.x,s.y,1,false);return;}
 // Autorun event pages use the same priority as the original map entity order.
 for(int i=0;i<s.entity_count;++i){const auto& e=s.entities[i];if(e.page>=0){const auto& p=events[e.event].pages[e.page];if(p.h[0]==3&&p.count){start_event(e.event,e.page,i);script();return;}}}
 if(confirm){if(trigger(s.x,s.y,0,false))return;if(trigger(s.x+dx[s.dir],s.y+dy[s.dir],0,true))return;}
 if(direction>=0){s.dir=direction;int x=s.x+dx[direction],y=s.y+dy[direction];if(passable(x,y))move_player(x,y);else if(trigger(x,y,1,true))return;}
 // Autonomous random/chasing entities: one tile every second, with collision/contact events.
 for(int i=0;i<s.entity_count;++i){auto& e=s.entities[i];if(e.page<0)continue;const auto& p=events[e.event].pages[e.page];if(!p.h[14]||e.px!=e.x*16||e.py!=e.y*16)continue;
  if(++e.timer<30)continue;e.timer=0;int dir=(s.frames*17+i*31)%4;
  if(p.h[14]==2){int xx=s.x-e.x,yy=s.y-e.y;dir=std::abs(xx)>std::abs(yy)?(xx>0?1:3):(yy>0?2:0);}
  e.dir=dir;int x=e.x+dx[dir],y=e.y+dy[dir];
  if(x==s.x&&y==s.y){if(p.h[0]==2&&p.count){start_event(e.event,e.page,i);return;}continue;}
  if(passable(x,y,i)){e.x=x;e.y=y;}
 }
}
