import { NodeEditor, ClassicPreset } from 'rete';
import { AreaPlugin, AreaExtensions } from 'rete-area-plugin';
import { ConnectionPlugin, Presets as Connections } from 'rete-connection-plugin';
import { ReactPlugin, Presets as Rendering } from 'rete-react-plugin';
import { createRoot } from 'react-dom/client';

// This is a projection. Every committed mutation is published by the native owner.
const editor = new NodeEditor();
const area = new AreaPlugin(document.getElementById('graph'));
const renderer = new ReactPlugin({ createRoot });
renderer.addPreset(Rendering.classic.setup());
const connections = new ConnectionPlugin();
connections.addPreset(Connections.classic.setup());
editor.use(area); area.use(connections); area.use(renderer);
let snapshot, rendering = false, pending = false, cancelledGesture = false, dragging, positions = new Map();
const canvas = document.getElementById('graph');
let queue = Promise.resolve();
const status = document.getElementById('status');
function send(action, ...args) {
    if (!snapshot || rendering || pending || snapshot.readonly) return;
    pending = true;
    window.nativeGraph(JSON.stringify({ nonce: window.graphNonce, revision: snapshot.revision,
        action, args: args.map(String) }));
}
window.graphSnapshot = value => {
    queue = queue.then(async () => {
        canvas.style.pointerEvents = 'none';
        rendering = true; pending = false; dragging = undefined; snapshot = value;
        status.textContent = value.error || (value.readonly ? 'Read only during Play' : 'Drag ports to connect · Escape cancels · Ctrl+S saves');
        const previous = editor.getNodes().map(n => n.id).join();
        await editor.clear(); positions = new Map();
        const ports = new Map(), sockets = new Map();
        for (const dto of value.nodes) {
            const node = new ClassicPreset.Node(dto.name); node.id = dto.id;
            for (const port of dto.ports) {
                const type = `${port.kind}:${port.type}`;
                if (!sockets.has(type)) sockets.set(type, new ClassicPreset.Socket(type));
                const socket = sockets.get(type);
                if (port.direction === 0) node.addInput(port.id, new ClassicPreset.Input(socket, port.name));
                else node.addOutput(port.id, new ClassicPreset.Output(socket, port.name));
                ports.set(port.id, { node, port });
            }
            await editor.addNode(node);
            positions.set(node.id, { x: dto.x, y: dto.y });
            await area.translate(node.id, { x: dto.x, y: dto.y });
        }
        for (const dto of value.edges) {
            const output = ports.get(dto.output), input = ports.get(dto.input);
            if (!output || !input) throw new Error('Native edge endpoint missing');
            const edge = new ClassicPreset.Connection(output.node, dto.output, input.node, dto.input);
            edge.id = dto.id; await editor.addConnection(edge);
        }
        if (!previous && value.nodes.length) await AreaExtensions.zoomAt(area, editor.getNodes());
        rendering = false; canvas.style.pointerEvents = '';
        document.documentElement.dataset.graphReady = value.revision;
    }).catch(error => { rendering = false; canvas.style.pointerEvents = ''; status.textContent = String(error); });
};
editor.addPipe(context => {
    if (rendering) return context;
    if (context.type === 'connectioncreate') {
        send('connect', context.data.sourceOutput, context.data.targetInput);
        return undefined;
    }
    if (context.type === 'connectionremove') {
        send('disconnect', context.data.id); return undefined;
    }
    if (context.type === 'noderemove' || context.type === 'nodecreate') return undefined;
    return context;
});
area.addPipe(context => {
    if (!rendering && cancelledGesture && context.type === 'nodetranslate') return undefined;
    if (!rendering && !cancelledGesture && context.type === 'nodetranslated') dragging = { id: context.data.id, ...context.data.position };
    return context;
});
document.addEventListener('pointerdown', () => { cancelledGesture = false; });
document.addEventListener('pointerup', () => {
    if (cancelledGesture) { cancelledGesture = false; dragging = undefined; return; }
    if (!dragging || rendering) return;
    const value = dragging; dragging = undefined;
    const old = positions.get(value.id);
    if (old && (old.x !== value.x || old.y !== value.y)) send('move', value.id, value.x, value.y);
});
document.addEventListener('keydown', event => {
    if (event.key === 'Escape') {
        cancelledGesture = true; dragging = undefined; connections.drop();
        if (snapshot) window.graphSnapshot(snapshot);
        event.preventDefault();
    }
    if ((event.ctrlKey || event.metaKey) && ['s', 'z', 'y'].includes(event.key.toLowerCase())) {
        send(event.key.toLowerCase() === 's' ? 'save' : event.key.toLowerCase() === 'y' || event.shiftKey ? 'redo' : 'undo');
        event.preventDefault();
    }
});
for (const button of document.querySelectorAll('[data-create]'))
    button.addEventListener('click', () => send('create', button.dataset.create, 80 + (snapshot?.nodes.length || 0) * 40, 100));
window.graphDispose = () => area.destroy();
window.graphCanvasReady = true;

window.nativeGraph(JSON.stringify({ nonce: window.graphNonce, action: '__ready' }));
