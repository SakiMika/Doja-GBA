from pathlib import Path
import argparse,io,zipfile,struct,tempfile,subprocess
p=argparse.ArgumentParser(description='Render original embedded MIDI tracks as GBA WAV assets.')
p.add_argument('archive');args=p.parse_args();root=Path(__file__).resolve().parents[1]
z=zipfile.ZipFile(args.archive);sp=z.read(next(n for n in z.namelist() if n.endswith('.sp')))[64:]
o,n=struct.unpack_from('>II',sp,8192+17*8);section=sp[o:o+n];count=struct.unpack_from('>I',section)[0]
with tempfile.TemporaryDirectory() as tmp:
 tmp=Path(tmp);subprocess.run(['javac','--add-exports','java.desktop/com.sun.media.sound=ALL-UNNAMED','-d',str(tmp),str(root/'tools/RenderMidi.java')],check=True)
 for i in range(count):
  o,n=struct.unpack_from('>II',section,4+8*i);data=section[o:o+n];assert data[:4]==b'MThd','Expected MIDI track'
  midi=tmp/f'{i}.mid';midi.write_bytes(data)
  subprocess.run(['java','--add-exports','java.desktop/com.sun.media.sound=ALL-UNNAMED','-cp',str(tmp),'RenderMidi',str(midi),str(root/f'audio/track_{i:02}.wav')],check=True)
print(f'Rendered {count} original MIDI tracks.')
