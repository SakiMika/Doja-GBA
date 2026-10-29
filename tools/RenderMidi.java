import com.sun.media.sound.AudioSynthesizer;
import javax.sound.midi.*;
import javax.sound.sampled.*;
import java.io.*;
import java.util.*;
class RenderMidi {
 public static void main(String[] args)throws Exception {
 Sequence seq=MidiSystem.getSequence(new File(args[0]));
 AudioSynthesizer synth=(AudioSynthesizer)MidiSystem.getSynthesizer();
 AudioFormat fmt=new AudioFormat(11025,16,1,true,false);
 Map<String,Object> options=new HashMap<>();options.put("interpolation","linear");options.put("max polyphony",64);
 AudioInputStream stream=synth.openStream(fmt,options);
 Receiver receiver=synth.getReceiver();
 java.util.List<MidiEvent> events=new ArrayList<>();for(Track t:seq.getTracks())for(int i=0;i<t.size();i++)events.add(t.get(i));
 events.sort(Comparator.comparingLong(MidiEvent::getTick));long tick=0,time=0;int tempo=500000;
 for(MidiEvent e:events){long dt=e.getTick()-tick;time+=dt*tempo/seq.getResolution();tick=e.getTick();MidiMessage m=e.getMessage();if(m instanceof MetaMessage mm){if(mm.getType()==0x51){byte[] d=mm.getData();tempo=((d[0]&255)<<16)|((d[1]&255)<<8)|(d[2]&255);}}else receiver.send(m,time);}
 long frames=(long)((time/1000000.0+1)*11025);
 AudioInputStream limited=new AudioInputStream(stream,fmt,frames);
 AudioFormat out=new AudioFormat(11025,8,1,false,false);
 AudioSystem.write(AudioSystem.getAudioInputStream(out,limited),AudioFileFormat.Type.WAVE,new File(args[1]));synth.close();
 System.out.println(args[0]+" seconds="+time/1000000.0);
 }
}
