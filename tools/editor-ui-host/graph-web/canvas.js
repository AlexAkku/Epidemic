import { NodeEditor, ClassicPreset } from 'rete';
import { AreaPlugin, AreaExtensions } from 'rete-area-plugin';
import { ConnectionPlugin, Presets as Connections } from 'rete-connection-plugin';
import { ReactPlugin, Presets as Rendering } from 'rete-react-plugin';
import { createRoot } from 'react-dom/client';

// Rete is a transient projection. Only native snapshots and history are authoritative.
const canvas = document.getElementById('graph');
const status = document.getElementById('status');
const problem = document.getElementById('problem');
const menu = document.getElementById('context');
const mini = document.getElementById('minimap');
const inlineDrafts=new Map();
let dismissedProblem='';
let runtimeFrame;
const editor = new NodeEditor();
const area = new AreaPlugin(canvas);
const renderer = new ReactPlugin({ createRoot });
renderer.addPreset(Rendering.classic.setup());
const connections = new ConnectionPlugin();
connections.addPreset(Connections.classic.setup());
editor.use(area); area.use(connections); area.use(renderer);
let snapshot, rendering = false, pending = false, pendingAction = '', cancelledGesture = false;
let gestureNode, dragging, followers = false, previewFrame, box, boxStart, gestureEpoch=0;
let positions = new Map(), projection = new Map(), selected = new Set(), queue = Promise.resolve();
let cursor = { x: 80, y: 80 }, minimapBounds, lastNativeSelection = '0', reconnectEdge, connectionGesture=false;
function note(text) { status.textContent = text; }
function send(action, ...args) {
    if (!snapshot || rendering || pending || snapshot.readonly) return;
    if(action==='compile'||action==='run')dismissedProblem='';
    pending = true; pendingAction = action; canvas.style.pointerEvents='none';
    window.nativeGraph(JSON.stringify({ nonce: window.graphNonce, revision: ((action==='literal'||action==='inputvalue')?inlineDrafts.get(String(args[0]))?.revision:undefined)??snapshot.revision, action, args: args.map(String) }));
}
const ids = () => [...selected].join(' ');
function nodeElement(id) { return area.nodeViews.get(id)?.element; }
function paintSelection() {
    for (const dto of snapshot?.nodes || []) {
        const element = nodeElement(dto.id);
        if (element) { element.classList.toggle('selected', selected.has(dto.id)); element.classList.toggle('invalid', diagnosticNode() === dto.id); }
    }
    document.getElementById('selection').textContent = `${selected.size} selected · ${snapshot?.nodes.length || 0} nodes`;
}
function select(id, additive = false) {
    if (!additive && !selected.has(id)) selected.clear();
    if (additive && selected.has(id)) selected.delete(id); else if (selected.size < 64) selected.add(id);
    else note('Selection limit: 64 nodes');
    paintSelection();
}
function validate(value) {
    if (!value || !/^\d{1,20}$/.test(value.revision) || !Array.isArray(value.nodes) || value.nodes.length > 1024 ||
        !Array.isArray(value.edges) || value.edges.length > 16384 || !Array.isArray(value.catalogue) || value.catalogue.length > 256) throw Error('Invalid Graph snapshot limits');
    const all = new Set(); let ports = 0;
    const id = text => { if (typeof text !== 'string' || !/^[1-9]\d{0,19}$/.test(text) || all.has(text)) throw Error('Invalid Graph identity'); all.add(text); };
    const endpoints = new Set();
    for (const n of value.nodes) {
        id(n.id);
        if (typeof n.name !== 'string' || n.name.length > 1024 || !Number.isFinite(n.x) || !Number.isFinite(n.y) || Math.abs(n.x) > 1e6 || Math.abs(n.y) > 1e6 || !Array.isArray(n.ports)) throw Error('Invalid Graph node');
        for (const p of n.ports) { id(p.id); endpoints.add(p.id); if (++ports > 8192 || ![0,1,2].includes(p.kind) || ![0,1].includes(p.direction)) throw Error('Invalid Graph port'); }
    }
    if(value.selected!=='0'&&!value.nodes.some(n=>n.id===value.selected))throw Error('Invalid Graph selection');
    for (const e of value.edges) { id(e.id); if (!endpoints.has(e.output) || !endpoints.has(e.input)) throw Error('Missing Graph endpoint'); }
}
window.graphSnapshot = value => {
    try { validate(value); } catch (error) { note(String(error)); return; }
    queue = queue.then(async () => {
        const started=performance.now();
        const conflict=(gestureNode||connectionGesture) && snapshot && (snapshot.revision!==value.revision||value.readonly);
        if (conflict) {
            cancelledGesture = true;connections.drop();clearReconnect();note('Graph changed during drag. Preview cancelled.');
        }
        canvas.style.pointerEvents = 'none'; rendering = true;
        const first = !snapshot, action = pendingAction, oldIds = new Set(projection.keys());
        snapshot = value; pending = false; pendingAction = ''; dragging = undefined;
        const wantedEdges = new Map(value.edges.map(e => [e.id, e]));
        for (const edge of editor.getConnections()) if (!wantedEdges.has(edge.id)) await editor.removeConnection(edge.id);
        const wanted = new Set(value.nodes.map(n => n.id));
        for(const [id] of inlineDrafts)if(!value.nodes.some(n=>n.id===id||n.ports.some(p=>p.id===id)))inlineDrafts.delete(id);
        for (const node of editor.getNodes()) if (!wanted.has(node.id)) { await editor.removeNode(node.id); projection.delete(node.id); selected.delete(node.id); }
        const ports = new Map(), sockets = new Map();
        for (const dto of value.nodes) {
            let node = editor.getNode(dto.id);
            const title = `${dto.markers & 1 ? '[Bookmark] ' : ''}${dto.markers & 2 ? '[Break] ' : ''}${dto.reference ? (dto.referenceAvailable ? '[Ref] ' : '[Missing Reference] ') + dto.referenceLabel : dto.name}${dto.group ? ' · ' + dto.group : ''}`;
            if (!node) {
                node = new ClassicPreset.Node(title); node.id = dto.id;
                for (const p of dto.ports) {
                    const type = p.numeric?"number":`${p.kind}:${p.type}`;
                    if (!sockets.has(type)) sockets.set(type, new ClassicPreset.Socket(type));
                    const socket = sockets.get(type);
                    if (p.direction === 0) node.addInput(p.id, new ClassicPreset.Input(socket, p.name, p.kind !== 0));
                    else node.addOutput(p.id, new ClassicPreset.Output(socket, p.name, true));
                }
                await editor.addNode(node);
            } else if (node.label !== title) { node.label = title; await area.update('node', node.id); }
            for (const p of dto.ports) ports.set(p.id, { node, port: p });
            const old = projection.get(dto.id);
            if (!old || old.x !== dto.x || old.y !== dto.y || cancelledGesture) await area.translate(dto.id, {x:dto.x,y:dto.y});
            const element = nodeElement(dto.id);
            if (element) {
                element.dataset.nodeId = dto.id;
                element.classList.toggle('missing-reference',!!dto.reference&&!dto.referenceAvailable);
                element.title = `${dto.name}\n${dto.comment || ''}\n${dto.ports.map(p => `${p.direction ? 'Out' : 'In'} ${p.name}: ${p.kind === 0 ? ('data ['+p.type+']') : p.kind === 1 ? 'control' : 'i64 event'}${p.kind === 0 && !p.direction ? ', one source required' : ''}`).join('\n')}`;
            }
            if(element)element.dataset.authoringTitle=element.title;
            projection.set(dto.id, dto);
        }
        for (const dto of value.edges) if (!editor.getConnection(dto.id)) {
            const output = ports.get(dto.output), input = ports.get(dto.input);
            const edge = new ClassicPreset.Connection(output.node, dto.output, input.node, dto.input);
            edge.id = dto.id; await editor.addConnection(edge);
        }
        positions = new Map(value.nodes.map(n => [n.id, {x:n.x,y:n.y}]));
        if (action === 'paste' || action === 'duplicate') selected = new Set(value.nodes.filter(n => !oldIds.has(n.id)).map(n => n.id));
        else if(value.error)selected=value.selected!=='0'?new Set([value.selected]):new Set();
        else if (value.selected !== '0' && value.selected !== lastNativeSelection && !selected.has(value.selected)) {
            selected=new Set([value.selected]);const n=value.nodes.find(n=>n.id===value.selected);const k=area.area.transform.k;
            if(n&&!first&&!resumeSocket)await area.area.translate(canvas.clientWidth/2-n.x*k,canvas.clientHeight/2-n.y*k);
        }
        lastNativeSelection = value.selected;
        if (first && value.nodes.length) await AreaExtensions.zoomAt(area, editor.getNodes());
        rendering = false; canvas.style.pointerEvents = ''; paintSelection(); drawMinimap(); showProblem();renderLiterals();paintRuntime();
        if(resumeSocket){const resume=resumeSocket;resumeSocket=undefined;const element=document.querySelector(`[data-port-id="${resume.key}"][data-port-side="${resume.side}"]`);if(element){const r=element.getBoundingClientRect();await connections.pick(new PointerEvent('pointerdown',{clientX:r.left+r.width/2,clientY:r.top+r.height/2}),'down');if(resume.edge)reconnectEdge=resume.edge;}}

        document.querySelectorAll('[data-command]').forEach(button => button.disabled = value.readonly);
        note((conflict ? 'Graph changed during drag. Preview cancelled.' : '') || value.error || value.report?.split('\n')[0] || (value.readonly ? 'Read only during Play' : 'Ctrl-click selects · Shift-drag selects a box · Escape cancels'));
        window.graphMetrics={nodes:value.nodes.length,renderMs:performance.now()-started,
            heapPeak:Math.max(window.graphMetrics?.heapPeak||0,performance.memory?.usedJSHeapSize||0)};
        document.documentElement.dataset.graphReady = value.revision;
    }).catch(error => { rendering = false; pending = false; canvas.style.pointerEvents = ''; note(String(error)); });
};
editor.addPipe(context => {
    if (rendering) return context;
    if(context.type==='connectioncreate'){
        const old=reconnectEdge;clearReconnect();
        if(old)send('reconnect',old,context.data.sourceOutput,context.data.targetInput);
        else send('connect',context.data.sourceOutput,context.data.targetInput);
        return undefined;
    }
    if(context.type==='connectionremove'){
        // ClassicFlow removes an edge while picking it up. Keep that local until drop.
        if(context.data.id===reconnectEdge){const view=area.connectionViews.get(reconnectEdge);if(view)view.element.style.opacity='.25';}
        return undefined;
    }
    if (context.type === 'nodecreate' || context.type === 'noderemove') return undefined;
    return context;
});
function clearReconnect(){
    if(reconnectEdge){const view=area.connectionViews.get(reconnectEdge);if(view)view.element.style.opacity='';}
    reconnectEdge=undefined;
}
connections.addPipe(context=>{
    if(context.type==='connectionpick'){connectionGesture=true;connectionOrigin=context.data.socket;}
    if(context.type==='connectiondrop'){
        connectionGesture=false;
        if(!context.data.created)clearReconnect();
    }
    return context;
});
area.addPipe(context => {
    if(context.type==='render'&&context.data.type==='node')requestAnimationFrame(renderLiterals);
    if(context.type==='render'&&context.data.type==='socket'){context.data.element.dataset.portId=context.data.key;context.data.element.dataset.portSide=context.data.side;}
    if (context.type === 'nodepicked' && !rendering) {
        gestureNode = context.data.id;
        select(gestureNode, ctrl);
    }
    if (!rendering && cancelledGesture && context.type === 'nodetranslate') return undefined;
    if (!rendering && !followers && !cancelledGesture && context.type === 'nodetranslated') {
        dragging = {id:context.data.id,...context.data.position};
        if (!previewFrame && selected.size > 1) previewFrame = requestAnimationFrame(async () => {
            previewFrame = undefined;
            if (!dragging || cancelledGesture || rendering) return;
            const current = dragging, old = positions.get(current.id); if (!old) return;
            const epoch=gestureEpoch;followers = true;
            try { for (const id of selected) if (id !== current.id && positions.has(id)) {
                if(epoch!==gestureEpoch||cancelledGesture||rendering)break;
                const p = positions.get(id); await area.translate(id, {x:p.x+current.x-old.x,y:p.y+current.y-old.y});
            }} finally { followers = false; }
        });
    }
    if (context.type === 'translated' || context.type === 'zoomed') drawMinimap();
    return context;
});
let connectionOrigin, resumeSocket;
let contextDuringConnection=false;
// Keep the Rete capture alive: its generic pointer-up handler otherwise drops
// the preview before Windows dispatches the right-click contextmenu event.
document.addEventListener('pointerdown',event=>{if(event.button===2&&connectionGesture){contextDuringConnection=true;event.preventDefault();event.stopImmediatePropagation();canvas.dispatchEvent(new MouseEvent('contextmenu',{clientX:event.clientX,clientY:event.clientY,bubbles:true,cancelable:true}));}},true);
document.addEventListener('pointerup',event=>{if(event.button===2&&contextDuringConnection){contextDuringConnection=false;event.preventDefault();event.stopImmediatePropagation();}},true);
let ctrl = false;
document.addEventListener('pointerdown', event => {
    const refSocket=event.target.closest('[data-port-id]');
    if(event.altKey&&event.button===0&&refSocket?.dataset.portSide==='input'&&!snapshot?.readonly){
        event.preventDefault();event.stopImmediatePropagation();cancel();
        window.nativeGraph(JSON.stringify({nonce:window.graphNonce,revision:snapshot.revision,action:'reference-start',args:[refSocket.dataset.portId,'0','0']}));return;
    }
    ctrl=event.ctrlKey||event.metaKey;cancelledGesture=false;gestureNode=undefined;
    if(!connectionGesture){
        clearReconnect();const socket=event.target.closest('[data-port-id]');
        if(socket?.dataset.portSide==='input')reconnectEdge=snapshot?.edges.find(e=>e.input===socket.dataset.portId)?.id;
    }
},true);
document.addEventListener('pointerup', () => {
    if (previewFrame) cancelAnimationFrame(previewFrame); previewFrame = undefined;
    if (cancelledGesture) { cancelledGesture = false; dragging = undefined; gestureNode = undefined; return; }
    if(connectionGesture||reconnectEdge){gestureNode=undefined;return;}
    const id = gestureNode; gestureNode = undefined;
    if (!id || rendering || !selected.has(id)) return;
    const old = positions.get(id), now = area.nodeViews.get(id)?.position;
    if (!old || !now) return;
    if (old.x !== now.x || old.y !== now.y) {
        const dx = now.x-old.x, dy = now.y-old.y;
        if (selected.size === 1) send('move',id,now.x,now.y);
        else send('moves',[...selected].map(key => `${key} ${positions.get(key).x+dx} ${positions.get(key).y+dy}`).join('\n'));
    } else send('select',id);
    dragging = undefined;
});
function cancel() {
    resumeSocket=undefined;contextDuringConnection=false;
    ++gestureEpoch;cancelledGesture = true; dragging = undefined; connections.drop();connectionGesture=false;clearReconnect(); menu.hidden = true;
    if(box){box.remove();box=undefined;}
    if (previewFrame) cancelAnimationFrame(previewFrame); previewFrame = undefined;
    queue = queue.then(async () => {
        rendering = true;
        try { for (const [id,p] of positions) await area.translate(id,p); } finally { rendering = false; }
    });
}
function world(x,y) { const r=canvas.getBoundingClientRect(),t=area.area.transform; return {x:(x-r.left-t.x)/t.k,y:(y-r.top-t.y)/t.k}; }
canvas.addEventListener('pointermove',event => { cursor=world(event.clientX,event.clientY); });
canvas.addEventListener('wheel',event=>event.stopPropagation(),{passive:true});
canvas.addEventListener('pointerdown',event => {
    if (event.target.closest('[data-node-id]') || event.button!==0) return;
    if(!event.shiftKey){if(!ctrl){selected.clear();paintSelection();}return;}
    event.stopImmediatePropagation(); boxStart={x:event.clientX,y:event.clientY};
    box=document.createElement('div');box.className='marquee';document.body.append(box);
},true);
window.addEventListener('pointermove',event => {
    if (!box) return;
    Object.assign(box.style,{left:Math.min(boxStart.x,event.clientX)+'px',top:Math.min(boxStart.y,event.clientY)+'px',width:Math.abs(event.clientX-boxStart.x)+'px',height:Math.abs(event.clientY-boxStart.y)+'px'});
});
window.addEventListener('pointerup',() => {
    if (!box) return;const r=box.getBoundingClientRect();selected.clear();
    for(const n of snapshot.nodes){const b=nodeElement(n.id)?.getBoundingClientRect();if(b&&b.left<r.right&&b.right>r.left&&b.top<r.bottom&&b.bottom>r.top&&selected.size<64)selected.add(n.id);}
    box.remove();box=undefined;paintSelection();if(selected.size)send('select',[...selected][0]);
});
window.addEventListener('blur',()=>{ctrl=false;if(gestureNode||box||connectionGesture)cancel();});
document.addEventListener('pointercancel',()=>cancel());
canvas.addEventListener('lostpointercapture',()=>{if(gestureNode)cancel();});
function diagnosticNode() { return /node=(\d+)/.exec(snapshot?.error || snapshot?.report || '')?.[1]; }
async function focusView(id) {
    if(!positions.has(id))return {focused:false,id,reason:'missing position'};
    const current=area.area.transform.k,element=nodeElement(id);
    const root=element?.getBoundingClientRect(),node=element?.querySelector('[data-testid=node]')?.getBoundingClientRect()||root;
    if(!root||!node)return {focused:false,id,reason:'missing DOM'};
    const width=Math.max(1,node.width/current),height=Math.max(1,node.height/current);
    // A narrow document region must still expose both sockets. Centering only
    // the title at a fixed minimum zoom could put the input outside the HWND.
    const k=Math.min(Math.max(0.75,current),Math.max(0.05,(canvas.clientWidth-48)/width),Math.max(0.05,(canvas.clientHeight-48)/height));
    await area.area.zoom(k);
    const bounds=(element.querySelector('[data-testid=node]')||element).getBoundingClientRect(),view=canvas.getBoundingClientRect();
    const transform=area.area.transform;
    await area.area.translate(transform.x+view.left+view.width/2-bounds.left-bounds.width/2,
        transform.y+view.top+view.height/2-bounds.top-bounds.height/2);
    return {focused:true,id,bounds:(element.querySelector('[data-testid=node]')||element).getBoundingClientRect().toJSON()};
}

function focusNode(id) {
    if(!positions.has(id))return;
    selected=new Set([id]);paintSelection();
    void focusView(id).then(()=>send('select',id));
}
function showProblem() {
    problem.replaceChildren();const text=snapshot.error || (!snapshot.report?.startsWith('OK')?snapshot.report:'');
    const key=`${snapshot.revision}:${text}`;problem.hidden=!text||dismissedProblem===key;if(problem.hidden)return;
    const content=document.createElement('div');content.className='problem-content';content.textContent=text;problem.append(content);
    const id=diagnosticNode();if(id){const jump=document.createElement('button');jump.textContent='Go to node';jump.onclick=()=>focusNode(id);content.append(jump);}
    const close=document.createElement('button');close.className='problem-close';close.title='Dismiss diagnostic';close.setAttribute('aria-label','Dismiss diagnostic');
    const svg=document.createElementNS('http://www.w3.org/2000/svg','svg');svg.setAttribute('viewBox','0 0 16 16');svg.setAttribute('aria-hidden','true');
    const path=document.createElementNS(svg.namespaceURI,'path');path.setAttribute('d','M4 4L12 12M12 4L4 12');path.setAttribute('stroke','currentColor');path.setAttribute('stroke-width','1.5');svg.append(path);close.append(svg);
    close.onclick=()=>{dismissedProblem=key;problem.hidden=true;};problem.append(close);
}
for(const event of ['pointerdown','pointerup'])problem.addEventListener(event,e=>e.stopPropagation());
function action(name) {
    if(name==='delete'&&reconnectEdge){const edge=reconnectEdge;connections.drop();clearReconnect();send('disconnect',edge);return;}
    if(name==='fit'){void AreaExtensions.zoomAt(area,editor.getNodes());return;}
    if(name==='reset'){void area.area.zoom(1).then(()=>area.area.translate(40,40));return;}
    if(name==='bookmark'){const nodes=snapshot.nodes.filter(n=>n.markers&1);const index=nodes.findIndex(n=>selected.has(n.id));if(nodes.length)focusNode(nodes[(index+1)%nodes.length].id);return;}
    if(name==='paste'){send('paste',cursor.x,cursor.y);return;}
    if(name==='delete'){if(selected.size)send('deletebulk',ids());return;}
    if(name==='copy'||name==='cut'||name==='duplicate'){if(selected.size)send(name,ids());return;}
    if(name==='group'){if(selected.size)send('group',ids(),`Group ${new Set(snapshot.nodes.map(n=>n.group).filter(Boolean)).size+1}`);return;}
    send(name);
}
for(const b of document.querySelectorAll('[data-action]'))b.onclick=()=>action(b.dataset.action);
for(const b of document.querySelectorAll('[data-command]'))b.onclick=()=>action(b.dataset.command);
canvas.addEventListener('contextmenu',event=>{
    event.preventDefault();event.stopPropagation();cursor=world(event.clientX,event.clientY);menu.replaceChildren();
    const add=(label,callback,enabled=true)=>{const b=document.createElement('button');b.textContent=label;b.disabled=!enabled;b.onclick=()=>{menu.hidden=true;callback();};menu.append(b);};
    const root=document.createElement('details');root.open=connectionGesture;const summary=document.createElement('summary');summary.textContent='Add';root.append(summary);menu.append(root);
    const categories=new Map();
    for(let i=0;i<(snapshot?.catalogue.length||0);++i){const r=snapshot.catalogue[i];if(r.palette===false)continue;
        let group=categories.get(r.category);if(!group){group=document.createElement('details');const title=document.createElement('summary');title.textContent=r.category;group.append(title);categories.set(r.category,group);root.append(group);}
        const button=document.createElement('button');button.textContent=r.name;button.disabled=snapshot.readonly||pending;
        button.onclick=()=>{menu.hidden=true;if(connectionGesture&&connectionOrigin)resumeSocket={...connectionOrigin,edge:reconnectEdge};connections.drop();send('create',i,cursor.x,cursor.y);};group.append(button);
    }
    for(const name of ['copy','cut','paste','duplicate','delete','group','fit','reset','compile','run'])add(name[0].toUpperCase()+name.slice(1),()=>action(name),['fit','reset'].includes(name)||(!snapshot.readonly&&!pending));
    add('Select Group',()=>{const group=projection.get([...selected][0])?.group;if(group){selected=new Set(snapshot.nodes.filter(n=>n.group===group).slice(0,64).map(n=>n.id));paintSelection();if(selected.size)send('select',[...selected][0]);}},selected.size>0);
    for(const edge of snapshot?.edges||[]){
        const output=snapshot.nodes.find(n=>n.ports.some(p=>p.id===edge.output));
        const input=snapshot.nodes.find(n=>n.ports.some(p=>p.id===edge.input));
        if(selected.has(output?.id)||selected.has(input?.id))add(`Disconnect ${output?.name} to ${input?.name}`,()=>{connections.drop();clearReconnect();send('disconnect',edge.id);},!snapshot.readonly&&!pending);
    }
    Object.assign(menu.style,{left:Math.max(4,Math.min(event.clientX,innerWidth-210))+'px',top:Math.max(4,Math.min(event.clientY,innerHeight-400))+'px'});menu.hidden=false;
});
document.addEventListener('pointerdown',e=>{if(e.button===0&&!menu.contains(e.target))menu.hidden=true;},true);
menu.addEventListener('pointerdown',e=>e.stopPropagation());
menu.addEventListener('pointerup',e=>e.stopPropagation());
document.addEventListener('keydown',event=>{
    if(event.target instanceof Element&&event.target.closest('input,textarea,[contenteditable=true]'))return;
    ctrl=event.ctrlKey||event.metaKey;
    if(event.key==='Escape'){cancel();if(box){box.remove();box=undefined;}event.preventDefault();}
    if(event.key==='Delete'){action('delete');event.preventDefault();}
    if(!ctrl)return;
    const key=event.key.toLowerCase();
    if(key==='a'){selected=new Set(snapshot.nodes.slice(0,64).map(n=>n.id));paintSelection();event.preventDefault();return;}
    const map={x:'cut',s:'save',z:event.shiftKey?'redo':'undo',y:'redo',c:'copy',v:'paste',d:'duplicate'};
    if(map[key]){window.graphAction(map[key]);event.preventDefault();}
});
document.addEventListener('keyup',event=>{ctrl=event.ctrlKey||event.metaKey;});
function drawMinimap() {
    if(!snapshot||!mini)return;const ctx=mini.getContext('2d');ctx.clearRect(0,0,mini.width,mini.height);
    if(!snapshot.nodes.length)return;
    const xs=snapshot.nodes.map(n=>n.x),ys=snapshot.nodes.map(n=>n.y);
    const left=Math.min(...xs)-50,top=Math.min(...ys)-50,w=Math.max(...xs)-left+250,h=Math.max(...ys)-top+220;
    minimapBounds={left,top,w,h};ctx.fillStyle='#a7a1d1';
    for(const n of snapshot.nodes)ctx.fillRect((n.x-left)/w*mini.width,(n.y-top)/h*mini.height,Math.max(3,150/w*mini.width),Math.max(3,80/h*mini.height));
}
mini.onclick=e=>{if(!minimapBounds)return;const r=mini.getBoundingClientRect(),b=minimapBounds,t=area.area.transform;
    const x=b.left+(e.clientX-r.left)/r.width*b.w,y=b.top+(e.clientY-r.top)/r.height*b.h;
    void area.area.translate(canvas.clientWidth/2-x*t.k,canvas.clientHeight/2-y*t.k);
};
window.graphFit=()=>AreaExtensions.zoomAt(area,editor.getNodes());
window.graphFocus=focusView;
window.graphGestureState=()=>({connection:connectionGesture,reconnect:reconnectEdge||'',busy:rendering||pending});
window.graphDispose=()=>{if(previewFrame)cancelAnimationFrame(previewFrame);area.destroy();};
window.graphCanvasReady=true;
window.nativeGraph(JSON.stringify({nonce:window.graphNonce,action:'__ready'}));

// Coordinates cross the AWT/JCEF boundary normalized to the actual CSS viewport;
// each HWND keeps its own DPI transform. No saved/window identity enters a ref.
window.graphReferenceHit=(awtX,awtY,width,height)=>{
    const x=awtX*innerWidth/width,y=awtY*innerHeight/height;
    const socket=document.elementFromPoint(x,y)?.closest('[data-port-id]');
    const port=socket?.dataset.portSide==='input'?socket.dataset.portId:'0';
    const rect=canvas.getBoundingClientRect(),t=area.area.transform;
    window.nativeGraph(JSON.stringify({nonce:window.graphNonce,revision:snapshot.revision,action:'reference-hit',args:[port,String((x-rect.left-t.x)/t.k),String((y-rect.top-t.y)/t.k)]}));
};
window.graphReferenceFeedback=text=>note(text||'Alt-drag reference input ↔ Scene / Hierarchy · Escape cancels');

window.graphAction=(name,attempt=0)=>{if(rendering||pending){if(attempt<120)setTimeout(()=>window.graphAction(name,attempt+1),16);else note('Graph is still busy; retry the command.');return;}const active=document.activeElement;if(active?.matches('input,textarea,[contenteditable=true]')){if(['copy','cut','paste'].includes(name))window.nativeGraph(JSON.stringify({nonce:window.graphNonce,revision:snapshot.revision,action:'text-shortcut',args:[name]}));else if(name==='delete')document.execCommand('delete');return;}action(name);};
function renderLiterals(){
    for(const dto of snapshot?.nodes||[]){const element=nodeElement(dto.id);if(!element)continue;
        const recipe=snapshot.catalogue.find(r=>r.type===dto.type);
        const fields=[];
        if(recipe?.operation===0)fields.push({id:dto.id,label:'Value',value:dto.previewDefault,floating:dto.numericType==='f64',action:'inputvalue'});
        for(const port of dto.ports)if(port.defaultValue!==undefined&&!snapshot.edges.some(e=>e.input===port.id))fields.push({id:port.id,label:port.name,value:port.defaultValue,floating:port.floating===true,action:'literal'});
        for(const field of fields){const draft=inlineDrafts.get(field.id);if(draft&&draft.revision!==snapshot.revision){try{if(draft.value===String(field.value)||(field.floating&&/^[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?$/.test(draft.value)&&Number.isFinite(Number(draft.value))&&Object.is(Number(draft.value),Number(field.value)))||(/^[-+]?\d+$/.test(draft.value)&&BigInt(draft.value).toString()===String(field.value)))inlineDrafts.delete(field.id);}catch{}}}
        let panel=element.querySelector('.node-literals');if(!panel){panel=document.createElement('div');panel.className='node-literals';(element.querySelector('[data-testid=node]')||element).append(panel);}
        const signature=JSON.stringify([snapshot.readonly,dto.numeric,dto.numericMode,dto.numericType,fields.map(f=>[f.id,String(f.value)])]);
        if(panel.contains(document.activeElement)&&document.activeElement.matches('input,textarea,[contenteditable=true]')){for(const field of fields){const input=[...panel.querySelectorAll('input')].find(e=>e.dataset.literalId===field.id);if(input)input.dataset.nativeValue=String(field.value);}continue;}if(panel.dataset.signature===signature)continue;panel.dataset.signature=signature;panel.replaceChildren();
        if(dto.numeric){
            const lock=document.createElement('button');lock.className='numeric-lock';lock.disabled=snapshot.readonly;
            lock.setAttribute('aria-label','Numeric type: '+(dto.numericMode===0?'Auto':'Locked '+dto.numericType));
            const icon=document.createElementNS('http://www.w3.org/2000/svg','svg');icon.setAttribute('width','14');icon.setAttribute('height','14');icon.setAttribute('viewBox','0 0 16 16');icon.setAttribute('aria-hidden','true');
            const path=document.createElementNS(icon.namespaceURI,'path');path.setAttribute('d',dto.numericMode===0?'M5 7V4a3 3 0 0 1 6 0M3 7h10v7H3z':'M5 7V4a3 3 0 0 1 6 0v3M3 7h10v7H3z');path.setAttribute('fill','none');path.setAttribute('stroke','currentColor');icon.append(path);lock.append(icon,document.createTextNode((dto.numericMode===0?' Auto · ':' Locked · ')+dto.numericType));
            for(const event of ['pointerdown','pointerup','dblclick'])lock.addEventListener(event,e=>e.stopPropagation());
            lock.onclick=e=>{e.stopPropagation();menu.replaceChildren();
                for(const [mode,title] of ['Auto','Lock i64','Lock f64'].entries()){const choice=document.createElement('button');choice.textContent=title;choice.onclick=()=>{menu.hidden=true;send('numeric',dto.id,mode);};menu.append(choice);}
                const rect=lock.getBoundingClientRect();Object.assign(menu.style,{left:Math.min(rect.left,innerWidth-210)+'px',top:Math.min(rect.bottom,innerHeight-120)+'px'});menu.hidden=false;
            };panel.append(lock);
        }
        for(const field of fields){const label=document.createElement('label');label.textContent=field.label;const input=document.createElement('input');input.type='text';input.inputMode='decimal';input.value=inlineDrafts.get(field.id)?.value??String(field.value);input.dataset.nativeValue=String(field.value);input.dataset.literalId=field.id;input.setAttribute('aria-label',field.label+' numeric literal');input.disabled=snapshot.readonly;
            for(const event of ['pointerdown','pointerup','click','dblclick'])input.addEventListener(event,e=>e.stopPropagation());
            input.oninput=()=>inlineDrafts.set(field.id,{...field,value:input.value,revision:inlineDrafts.get(field.id)?.revision??snapshot.revision});
            input.addEventListener('keydown',e=>{e.stopPropagation();if(e.key==='Escape'){inlineDrafts.delete(field.id);input.value=input.dataset.nativeValue;input.blur();}else if(e.key==='Enter'){input.blur();}});
            input.onchange=()=>{if(input.value!==input.dataset.nativeValue)send(field.action,field.id,input.value);};label.append(input);panel.append(label);
        }
    }
}

window.graphFlushInline=()=>window.nativeGraph(JSON.stringify({nonce:window.graphNonce,action:'flush-inline',args:[...inlineDrafts.values()].map(f=>[f.action,f.id,String(f.value),f.revision])}));

// A separate read-only projection: it never invokes editor/history/nativeGraph.
window.graphRuntime=value=>{
    if(!value||!Array.isArray(value.nodes)||value.nodes.length>64||!Array.isArray(value.breakpoints)||value.breakpoints.length>1024)return;
    runtimeFrame=value;paintRuntime();
};
function paintRuntime(){
    if(!runtimeFrame)return;const frame=runtimeFrame,badge=document.getElementById('runtime-state');
    badge.textContent=`${frame.phase} · Tick ${frame.tick}`;badge.title=`Native session ${frame.session} · generation ${frame.generation} · connection ${frame.connection}`;badge.dataset.session=frame.session;badge.dataset.tick=frame.tick;
    for(const node of snapshot?.nodes||[]){const element=nodeElement(node.id);if(!element)continue;
        const states=frame.nodes.filter(a=>a.node===node.id),active=states.some(a=>['Started','Active','Waiting'].includes(a.state));
        element.classList.toggle('runtime-active',active&&frame.phase!=='Stopped');element.classList.toggle('runtime-failed',states.some(a=>a.state==='Failed'));element.classList.toggle('runtime-breakpoint',frame.breakpoints.includes(node.id));
        const rows=states.map(a=>`Activation ${a.activation}: ${a.state}, instructions=${a.instructions}, native calls=${a.nativeCalls}\n${a.values.map(v=>`${v.port} (${v.type}) = ${v.value}`).join('\n')}`).join('\n');
        element.title=(element.dataset.authoringTitle||'')+(rows?'\nRuntime session '+frame.session+' / tick '+frame.tick+'\n'+rows.slice(0,4096):'');
    }
}
