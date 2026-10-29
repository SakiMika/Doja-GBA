from pathlib import Path
import json,struct,io,zipfile,sys
from PIL import Image
ROOT=Path(__file__).resolve().parents[1]
# Pass the user's original Chapter_1.zip; no source assets are fetched online.
z=zipfile.ZipFile(sys.argv[1]); jar=zipfile.ZipFile(io.BytesIO(z.read(next(n for n in z.namelist() if n.endswith('.jar')))))
sp=z.read(next(n for n in z.namelist() if n.endswith('.sp')))[64:]
class R:
 def __init__(self,d):self.d=d;self.p=0
 def take(self,n):v=self.d[self.p:self.p+n];self.p+=n;assert len(v)==n;return v
 def u8(self):return self.take(1)[0]
 def i8(self):return struct.unpack('b',self.take(1))[0]
 def u16(self):return struct.unpack('>H',self.take(2))[0]
 def i16(self):return struct.unpack('>h',self.take(2))[0]
 def i32(self):return struct.unpack('>i',self.take(4))[0]
def sec(i):
 if i in (7,14):return R(jar.read(f'{i}.bin'))
 off,n=struct.unpack_from('>II',sp,8192+8*i);return R(sp[off:off+n])
out=['#include "assets.h"']
def arr(name,typ,vals):out.append(f'const {typ} {name} = '+'{'+','.join(str(v) for v in vals)+'};')
imgs={}
for section in (11,12,13,14,15):
 r=sec(section)
 for j in range(r.i32()):
  off,size=r.i32(),r.i32();imgs[section,j]=Image.open(io.BytesIO(r.d[off:off+size])).convert('RGBA')
  if section==14:imgs[section,j]=imgs[section,j].resize((160,160),Image.Resampling.NEAREST)
  if section==15:imgs[section,j]=imgs[section,j].resize((42,42),Image.Resampling.NEAREST)
for j in range(12,17):imgs[13,j]=Image.open(io.BytesIO(jar.read(f't{j}.gif'))).convert('RGBA')
for j,n in enumerate(('go.gif','eb.gif')):imgs[17,j]=Image.open(io.BytesIO(jar.read(n))).convert('RGBA')
# One shared RGB palette across maps, actors and portraits; 0=transparent, 1=black, 255=white.
sample=Image.new('RGB',(256,4096));pos=0
for im in imgs.values():
 rgb=Image.new('RGB',im.size);rgb.paste(im,mask=im.getchannel('A'));sample.paste(rgb,(0,pos));pos+=im.height
q=sample.crop((0,0,256,min(pos,4096))).quantize(colors=253)
pal=[0,0,0,0,0,0]+q.getpalette()[:253*3]+[255,255,255]
pim=Image.new('P',(1,1));pim.putpalette(pal)
arr('palette[256]','uint16_t',[(pal[i*3]>>3)|((pal[i*3+1]>>3)<<5)|((pal[i*3+2]>>3)<<10) for i in range(256)])
for (s,j),im in imgs.items():
 raw=bytearray(im.convert('RGB').quantize(palette=pim,dither=Image.Dither.NONE).tobytes());alpha=im.getchannel('A').tobytes()
 for i,a in enumerate(alpha):
  if a<128:raw[i]=0
  elif raw[i]==0:raw[i]=1
 arr(f'img_{s}_{j}[{len(raw)}]','uint8_t',raw)
out.append('const Image images[18][32] = {'+','.join('{'+','.join('{%d,%d,img_%d_%d}'%(imgs[s,j].width,imgs[s,j].height,s,j) if (s,j) in imgs else '{0,0,nullptr}' for j in range(32))+'}' for s in range(18))+'};')
r=sec(7);eventrows=[];opcounts={};pages_total=0
for eid in range(r.i32()):
 pagerows=[]
 for pid in range(r.i16()):
  h=[r.u8() for _ in range(7)]+[r.u16()]+[r.u8() for _ in range(8)]
  n,size=r.i32(),r.i32();offsets=[r.i32() for _ in range(n)];raw=r.take(size);cmdrows=[]
  for ci,off in enumerate(offsets):
   c=R(raw[off:]);op,ln=c.i32(),c.i32();d=c.take(ln);opcounts[op]=opcounts.get(op,0)+1
   if op==0:d=d.decode('cp932').replace('\u3000',' ').encode('ascii')
   name=f'd_{eid}_{pid}_{ci}'
   arr(name+'[]','uint8_t',list(d)+[0]);cmdrows.append('{%d,%d,%s}'%(op,len(d),name))
  cname=f'cmd_{eid}_{pid}'
  out.append(f'const Command {cname}[] = '+'{'+(','.join(cmdrows) if n else '{0,0,nullptr}')+'};')
  pagerows.append('{{'+','.join(map(str,h))+'},'+str(n)+','+cname+'}');pages_total+=1
 out.append(f'const Page pages_{eid}[] = '+'{'+','.join(pagerows)+'};');eventrows.append('{%d,pages_%d}'%(len(pagerows),eid))
assert r.p==len(r.d)
out.append('const Event events[682] = {'+','.join(eventrows)+'};')
r=sec(9);maprows=[]
for i in range(r.i32()):
 h=[r.i16(),r.i16()]+[r.u8() for _ in range(8)];r.take(h[-1]);music=r.u8();cells=r.take(h[4]*h[5]*2)
 arr(f'cells_{i}[]','uint8_t',cells);maprows.append('{{'+','.join(map(str,h))+'},'+str(music)+f',cells_{i}'+'}')
assert r.p==len(r.d)
out.append('const Map maps[30] = {'+','.join(maprows)+'};')
r=sec(8);n=r.i32();out.append('const uint8_t mapsets[10][17] = {'+','.join('{'+','.join(map(str,r.take(17)))+'}' for _ in range(n))+'};')
r=sec(5);n=r.i32();out.append('const uint8_t collision[8][32][5] = {'+','.join('{'+','.join('{'+','.join(map(str,r.take(5)))+'}' for _ in range(32))+'}' for _ in range(n))+'};')
r=sec(0);sprites=[]
for _ in range(r.i32()):
 r.take(12+3+5+16+1+15);n=r.u8();r.take(n*2);sprites.append(r.u8())
assert r.p==len(r.d)
arr('actor_sprites[6]','uint8_t',sprites)
for name,file,count in [('actor_names','actors.txt',6),('item_names','items.txt',4)]:
 names=jar.read(file).decode().splitlines();names+=['Actor']*(count-len(names));out.append(f'const char* const {name}[{count}] = '+'{'+','.join(json.dumps(x) for x in names[:count])+'};')
r=sec(6);r.take(40);n=r.u8();party=[r.i16() for _ in range(n)];r.take(8);ev=r.i32();mp=r.i16();x=r.u8();y=r.u8()
arr('initial_party[3]','int',party+[0]*(3-len(party)))
for name,v in [('initial_party_count',n),('initial_event',ev),('initial_map',mp),('initial_x',x),('initial_y',y)]:out.append(f'const int {name}={v};')
(ROOT/'src/assets.cpp').write_text('\n'.join(out))
(ROOT/'asset_report.json').write_text(json.dumps({'maps':len(maprows),'event_groups':len(eventrows),'event_pages':pages_total,'commands':sum(opcounts.values()),'opcodes':opcounts,'images':len(imgs),'actor_sprites':sprites,'start':[mp,x,y,ev]},indent=2))
print('Generated',len(maprows),'maps,',len(eventrows),'event groups,',len(imgs),'images; initial state',mp,x,y,ev)
