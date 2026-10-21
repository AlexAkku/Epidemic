// Browser implementation of the Runtime Presentation drawing protocol.
// This stays below Framework and contains no game/content identifiers.
Module.epidemicRender = function(a, strings) {
  const canvas=Module.canvas;
  let ctx=canvas.getContext('2d',{alpha:false}), i=0;
  const layers=[], pool=Module.epidemicLayerPool||(Module.epidemicLayerPool=[]);
  const colors=Module.epidemicColors||(Module.epidemicColors=new Map());
  const layouts=Module.epidemicLayouts||(Module.epidemicLayouts=new Map());
  const v=()=>a[i++], color=n=>{n>>>=0;let c=colors.get(n);if(c===undefined){c='#'+n.toString(16).padStart(6,'0');if(colors.size>=2048)colors.clear();colors.set(n,c);}return c;};
  if(Module.epidemicInspect)Module.epidemicTextRuns=[];
  const style=(c,alpha,stroke=false)=>{if(stroke)ctx.strokeStyle=color(c);else ctx.fillStyle=color(c);const previous=ctx.globalAlpha;ctx.globalAlpha=previous*Math.max(0,Math.min(1,alpha));return previous;};
  function source(id,sx,sy,sw,sh,dx,dy,dw,dh){
    const asset=Module.epidemicImages[id];if(!asset)throw Error('Missing image '+id);
    for(const p of asset){
      const y=Math.max(sy,p.y),end=Math.min(sy+sh,p.y+p.image.height);
      if(end>y&&sw>0&&sh>0&&dw>0&&dh>0)ctx.drawImage(p.image,sx,y-p.y,sw,end-y,dx,dy+(y-sy)*dh/sh,dw,(end-y)*dh/sh);
    }
  }
  function triangle(image,s,d){
    const [p,q,r]=s,[u,w,z]=d;
    const den=(q[0]-p[0])*(r[1]-p[1])-(r[0]-p[0])*(q[1]-p[1]);if(Math.abs(den)<1e-7)return;
    const m1=((w[0]-u[0])*(r[1]-p[1])-(z[0]-u[0])*(q[1]-p[1]))/den;
    const m2=((w[1]-u[1])*(r[1]-p[1])-(z[1]-u[1])*(q[1]-p[1]))/den;
    const m3=((z[0]-u[0])*(q[0]-p[0])-(w[0]-u[0])*(r[0]-p[0]))/den;
    const m4=((z[1]-u[1])*(q[0]-p[0])-(w[1]-u[1])*(r[0]-p[0]))/den;
    ctx.save();ctx.beginPath();ctx.moveTo(...u);ctx.lineTo(...w);ctx.lineTo(...z);ctx.closePath();ctx.clip();
    ctx.transform(m1,m2,m3,m4,u[0]-m1*p[0]-m3*p[1],u[1]-m2*p[0]-m4*p[1]);ctx.drawImage(image,0,0);ctx.restore();
  }
  while(i<a.length){const op=v();
    if(op===0){const w=v(),h=v(),s=v(),ox=v(),oy=v();ctx.reset();ctx.fillStyle='#060d17';ctx.fillRect(0,0,w,h);ctx.setTransform(s,0,0,s,ox,oy);}
    else if(op===1){const m=[v(),v(),v(),v(),v(),v()],alpha=v();ctx.save();ctx.transform(...m);ctx.globalAlpha*=alpha;}
    else if(op===2)ctx.restore();
    else if(op===3){const x=v(),y=v(),w=v(),h=v(),c=v(),alpha=v(),previous=style(c,alpha);ctx.fillRect(x,y,w,h);ctx.globalAlpha=previous;}
    else if(op===4){const x=v(),y=v(),xx=v(),yy=v(),c=v(),t=v(),alpha=v(),previous=style(c,alpha,true);ctx.lineWidth=t;ctx.beginPath();ctx.moveTo(x,y);ctx.lineTo(xx,yy);ctx.stroke();ctx.globalAlpha=previous;}
    else if(op===5||op===6){const x=v(),y=v(),rx=v(),ry=op===6?v():rx,c=v(),t=op===6?v():0,alpha=v(),previous=style(c,alpha,op===6);ctx.beginPath();ctx.ellipse(x,y,Math.max(0,rx),Math.max(0,ry),0,0,Math.PI*2);if(op===5)ctx.fill();else{ctx.lineWidth=t;ctx.stroke();}ctx.globalAlpha=previous;}
    else if(op===7){const x=v(),y=v(),w=v(),h=v(),r=v(),c=v(),alpha=v(),previous=style(c,alpha);ctx.beginPath();ctx.roundRect(x,y,w,h,Math.max(0,Math.min(r,Math.abs(w)/2,Math.abs(h)/2)));ctx.fill();ctx.globalAlpha=previous;}
    else if(op===8){const x=v(),y=v(),w=v(),h=v(),top=v(),bottom=v(),alpha=v(),previous=ctx.globalAlpha;ctx.globalAlpha*=alpha;const g=ctx.createLinearGradient(x,y,x,y+h);g.addColorStop(0,color(top));g.addColorStop(1,color(bottom));ctx.fillStyle=g;ctx.fillRect(x,y,w,h);ctx.globalAlpha=previous;}
    else if(op===9){const x=v(),y=v(),ax=v(),ay=v(),bx=v(),by=v(),ex=v(),ey=v(),c=v(),t=v(),alpha=v(),previous=style(c,alpha,true);ctx.lineWidth=t;ctx.beginPath();ctx.moveTo(x,y);ctx.bezierCurveTo(ax,ay,bx,by,ex,ey);ctx.stroke();ctx.globalAlpha=previous;}
    else if(op===10||op===12){const n=v(),c=v(),alpha=v();if(op===12)ctx.save();const previous=style(c,alpha);ctx.beginPath();for(let k=0;k<n;k++){const x=v(),y=v();if(k)ctx.lineTo(x,y);else ctx.moveTo(x,y);}ctx.closePath();if(op===12)ctx.clip();else{ctx.fill();ctx.globalAlpha=previous;}}
    else if(op===11){const x=v(),y=v(),w=v(),h=v();ctx.save();ctx.beginPath();ctx.rect(x,y,w,h);ctx.clip();}
    else if(op===13){const w=v(),h=v();ctx.save();const g=ctx.createRadialGradient(w/2,h/2,h*.15,w/2,h/2,Math.hypot(w,h)*.55);g.addColorStop(0,'#0000');g.addColorStop(1,'#000b');ctx.fillStyle=g;ctx.fillRect(0,0,w,h);ctx.restore();}
    else if(op===14){const id=v(),sx=v(),sy=v(),sw=v(),sh=v(),x=v(),y=v(),ax=v(),ay=v(),w=v(),h=v(),rotation=v(),alpha=v(),phase=v(),strength=v(),core=v();ctx.save();ctx.globalAlpha*=alpha;ctx.translate(x,y);ctx.rotate(rotation);
      if(strength>0){
        // Continuous displacement mesh: preserve the anchored core, deform outer fragments.
        const asset=Module.epidemicImages[id], scratch=Module.epidemicWarp||(Module.epidemicWarp=new OffscreenCanvas(1,1));
        scratch.width=Math.max(1,sw);scratch.height=Math.max(1,sh);const target=scratch.getContext('2d');
        for(const p of asset){const begin=Math.max(sy,p.y),end=Math.min(sy+sh,p.y+p.image.height);if(end>begin)target.drawImage(p.image,sx,begin-p.y,sw,end-begin,0,begin-sy,sw,end-begin);}
        const point=(u,z)=>{const px=-ax+u*w,py=-ay+z*h,dist=Math.hypot(px,py),weight=Math.max(0,Math.min(1,(dist-core)/Math.max(1,Math.min(w,h)*.45-core)));return [px+strength*weight*Math.sin(phase*1.31+px*.014-py*.006),py+strength*weight*Math.sin(phase*1.57+py*.016+px*.005)];};
        for(let row=0;row<8;row++)for(let col=0;col<8;col++){
          const u=col/8,z=row/8,uu=(col+1)/8,zz=(row+1)/8,p=[u*sw,z*sh],q=[uu*sw,z*sh],r=[uu*sw,zz*sh],s=[u*sw,zz*sh];
          triangle(scratch,[p,q,r],[point(u,z),point(uu,z),point(uu,zz)]);triangle(scratch,[p,r,s],[point(u,z),point(uu,zz),point(u,zz)]);
        }
      }else source(id,sx,sy,sw,sh,-ax,-ay,w,h);ctx.restore();}
    else if(op===15){const id=v(),x=v(),y=v(),w=v(),h=v(),alpha=v();const pages=Module.epidemicImages[id],height=Math.max(...pages.map(p=>p.y+p.image.height));if(Module.epidemicInspect)(Module.epidemicPresentedImages||(Module.epidemicPresentedImages=new Set())).add(id);ctx.save();ctx.globalAlpha*=alpha;source(id,0,0,pages[0].image.width,height,x,y,w,h);ctx.restore();}
    else if(op===16){const x=v(),y=v(),size=v(),c=v(),w=v(),h=v(),center=v(),offset=v(),length=v(),wrap=v();const text=UTF8ToString(strings+offset,length);ctx.save();ctx.fillStyle=color(c);ctx.font=(size<20?'400 ':'700 ')+size+'px "Epidemic UI",sans-serif';ctx.textBaseline='alphabetic';ctx.beginPath();ctx.rect(x,y,w,h);ctx.clip();
      if(Module.epidemicInspect){const m=ctx.getTransform(),p=m.transformPoint(new DOMPoint(x+w/2,y+h/2));Module.epidemicTextRuns.push({text,x:p.x,y:p.y});}
      const key=ctx.font+'|'+w+'|'+wrap+'|'+center+'|'+text;let layout=layouts.get(key);
      if(!layout){let lines=[];for(const paragraph of text.split('\n')){
        if(!wrap||center){lines.push(paragraph);continue;}let line='';
        for(const word of paragraph.split(' ')){const next=line?line+' '+word:word;if(line&&ctx.measureText(next).width>w){lines.push(line);line=word;}else line=next;}lines.push(line);
      }layout=lines.map(line=>{const m=ctx.measureText(line);return {text:line,width:m.width,ascent:m.actualBoundingBoxAscent||size*.72,descent:m.actualBoundingBoxDescent||0};});if(layouts.size>=1024)layouts.delete(layouts.keys().next().value);layouts.set(key,layout);}
      const step=size*1.3,first=layout[0],last=layout[layout.length-1],inkHeight=(layout.length-1)*step+first.ascent+last.descent;
      const baseline=center?y+Math.max(0,(h-inkHeight)/2)+first.ascent:y+Math.max(first.ascent,size*.9);
      layout.forEach((line,k)=>ctx.fillText(line.text,center?x+Math.max(0,(w-line.width)/2):x,baseline+k*step));ctx.restore();}
    else if(op===17){const type=v(),rx=v(),ry=v(),cx=v(),cy=v(),depth=layers.length;let layer=pool[depth];if(!layer||layer.width!==canvas.width||layer.height!==canvas.height)layer=pool[depth]=new OffscreenCanvas(canvas.width,canvas.height);const next=layer.getContext('2d');next.reset();next.setTransform(ctx.getTransform());next.globalAlpha=ctx.globalAlpha;const m=ctx.getTransform();layers.push({ctx,layer,type,rx,ry,cx:m.a*cx+m.c*cy+m.e,cy:m.b*cx+m.d*cy+m.f});ctx=next;}
    else if(op===18){const radius=v(),layer=layers.pop();if(!layer)throw Error('Unbalanced effect');ctx=layer.ctx;ctx.save();ctx.resetTransform();ctx.globalAlpha=1;
      if(layer.type===0){ctx.filter='blur('+radius+'px)';ctx.drawImage(layer.layer,0,0);}
      else if(layer.type===1){ctx.filter='url(#epidemicEdges)';ctx.drawImage(layer.layer,0,0);}
      else {
        const rx=layer.rx*Math.PI/180,ry=layer.ry*Math.PI/180,cx=layer.cx,cy=layer.cy,W=canvas.width,H=canvas.height;
        const project=(x,y)=>{const u=x-cx,z=y-cy,xx=u*Math.cos(ry)+z*Math.sin(rx)*Math.sin(ry),yy=z*Math.cos(rx),depth=-u*Math.sin(ry)+z*Math.sin(rx)*Math.cos(ry),scale=1000/Math.max(100,1000-depth);return[cx+xx*scale,cy+yy*scale];};
        for(let row=0;row<6;row++)for(let col=0;col<10;col++){const x=col*W/10,y=row*H/6,xx=(col+1)*W/10,yy=(row+1)*H/6,p=[x,y],q=[xx,y],r=[xx,yy],s=[x,yy];triangle(layer.layer,[p,q,r],[project(...p),project(...q),project(...r)]);triangle(layer.layer,[p,r,s],[project(...p),project(...r),project(...s)]);}
      }ctx.restore();}
    else throw Error('Unknown Presentation opcode '+op);
  }
  if(layers.length)throw Error('Unclosed Presentation effect');
  Module.epidemicFrames=(Module.epidemicFrames||0)+1;Module.epidemicLastCommands=a.length;
};
