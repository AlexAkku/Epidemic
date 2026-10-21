Module.epidemicCreateAudio=async function(){
  const context=new AudioContext(), sfxGain=context.createGain();sfxGain.connect(context.destination);
  const api=Module.epidemicAudio={context,sfxGain,voices:new Set(),buffers:{},music:[],errors:[]};
  await context.resume();
  await Promise.all(Object.entries(Module.epidemicManifest.audio).filter(([,a])=>!a.stream).map(async([id,a])=>{
    const response=await fetch(a.url);if(!response.ok)throw Error('Audio '+response.status+' '+a.url);
    api.buffers[id]=await context.decodeAudioData(await response.arrayBuffer());
  }));
  api.createMusic=()=>{const audio=new Audio();audio.preload='auto';const source=context.createMediaElementSource(audio),gain=context.createGain(),analyser=context.createAnalyser();source.connect(gain);gain.connect(analyser);analyser.connect(context.destination);analyser.fftSize=1024;const m={audio,gain,analyser,fft:new Uint8Array(512),bands:new Float32Array(8),wait:5,once:false,started:false,failed:false,finished:false,active:true};api.music.push(m);return api.music.length-1;};
  api.loadUrl=(id,url,once,owned=false)=>{const m=api.music[id];m.audio.pause();if(m.owned)URL.revokeObjectURL(m.audio.src);m.audio.src=url;m.owned=owned;m.once=once;m.duration=NaN;m.wait=once?0:5;m.started=false;m.failed=false;m.finished=false;m.active=true;m.audio.onerror=()=>{m.failed=true;api.errors.push('Media load '+url);};return 1;};
  api.load=(id,resource,once)=>{const asset=Module.epidemicManifest.audio[resource];if(!asset)return 0;api.loadUrl(id,asset.url,once);api.music[id].duration=asset.duration;return 1;};
  api.state=id=>{const m=api.music[id];if(m.failed)return 5;if(!m.audio.src)return 0;if(m.finished)return 4;if(!m.audio.paused)return 2;return m.started?3:1;};
  api.tick=(id,dt,active,volume)=>{
    const m=api.music[id],audio=m.audio;if(m.failed||!audio.src||m.finished)return;
    if(!active&&!m.once){if(m.active){audio.pause();audio.currentTime=0;m.started=false;m.wait=0;m.bands.fill(0);}m.active=false;return;}
    if(active)m.active=true;
    if(!m.started){m.wait=Math.max(0,m.wait-dt);if(m.wait>0||!active)return;m.started=true;}
    if(!active){audio.pause();m.bands.fill(0);return;}
    if(audio.ended){m.started=false;m.finished=m.once;m.wait=60;m.bands.fill(0);if(!m.once)audio.currentTime=0;return;}
    const duration=audio.duration,time=audio.currentTime;
    const fade=m.once?1:Math.min(Math.max(0,time/10),Number.isFinite(duration)?Math.max(0,(duration-time)/10):1,1);
    m.gain.gain.value=Math.max(0,Math.min(1,volume))*fade;
    if(audio.paused&&!m.playPending){m.playPending=true;audio.play().catch(e=>{m.failed=true;api.errors.push(e.message);}).finally(()=>{m.playPending=false;});}
  };
  api.bands=(id,pointer)=>{const m=api.music[id];m.analyser.getByteFrequencyData(m.fft);const limits=[0,3,6,12,24,48,96,192,512];for(let k=0;k<8;k++){let sum=0;for(let j=limits[k];j<limits[k+1];j++)sum+=m.fft[j];const value=sum/(limits[k+1]-limits[k])/255;m.bands[k]=m.bands[k]*.7+value*.3;HEAPF32[(pointer>>2)+k]=m.bands[k];}};
  api.destroy=id=>{const m=api.music[id];m.audio.pause();if(m.owned)URL.revokeObjectURL(m.audio.src);m.audio.removeAttribute('src');m.audio.load();m.gain.disconnect();};
  api.play=(id,gain,pan)=>{const buffer=api.buffers[id];if(!buffer||api.voices.size>=25)return;const voice=context.createBufferSource(),g=context.createGain(),p=context.createStereoPanner();voice.buffer=buffer;g.gain.value=gain;p.pan.value=pan;voice.connect(g);g.connect(p);p.connect(sfxGain);api.voices.add(voice);voice.onended=()=>{api.voices.delete(voice);voice.disconnect();g.disconnect();p.disconnect();};voice.start();};
  return api;
};
