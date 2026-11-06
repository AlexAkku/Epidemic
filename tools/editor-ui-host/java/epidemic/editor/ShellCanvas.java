package epidemic.editor;

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;

/** Navigable presentation canvas. Guides are UI metadata, never authored game objects. */
public final class ShellCanvas extends JPanel {
    private final String id;
    private double zoom = 1;
    private int panX, panY;
    private Point drag;
    public ShellCanvas(String id) {
        this.id = id; setPreferredSize(new Dimension(480,320)); setFocusable(true);
        setToolTipText("Wheel to zoom; middle button to pan. Creation and editing require the indicated F phase.");
        getAccessibleContext().setAccessibleName(id + " planned canvas");
        addMouseWheelListener(event -> { zoom = Math.max(.25,Math.min(4,zoom*Math.pow(1.1,-event.getPreciseWheelRotation()))); repaint(); });
        MouseAdapter pan = new MouseAdapter() {
            @Override public void mousePressed(MouseEvent event) { requestFocusInWindow(); if(SwingUtilities.isMiddleMouseButton(event))drag=event.getPoint(); }
            @Override public void mouseReleased(MouseEvent event) { drag=null; }
            @Override public void mouseDragged(MouseEvent event) {
                if(drag!=null){panX+=event.getX()-drag.x;panY+=event.getY()-drag.y;drag=event.getPoint();repaint();}
            }
        }; addMouseListener(pan);addMouseMotionListener(pan);
        getInputMap(WHEN_FOCUSED).put(KeyStroke.getKeyStroke(KeyEvent.VK_HOME,0),"home");
        getActionMap().put("home",new AbstractAction(){@Override public void actionPerformed(ActionEvent event){zoom=1;panX=panY=0;repaint();}});
    }
    @Override protected void paintComponent(Graphics graphics) {
        Graphics2D g=(Graphics2D)graphics.create();
        try {
            g.setRenderingHint(RenderingHints.KEY_ANTIALIASING,RenderingHints.VALUE_ANTIALIAS_ON);
            g.setColor(Theme.DOCUMENT);g.fillRoundRect(0,0,getWidth(),getHeight(),Theme.PANEL_RADIUS*2,Theme.PANEL_RADIUS*2);
            g.setFont(getFont()==null?new Font("Dialog",Font.PLAIN,13):getFont());
            g.setColor(Theme.BORDER);int step=Math.max(8,(int)(24*zoom));
            for(int x=Math.floorMod(panX,step);x<getWidth();x+=step)for(int y=Math.floorMod(panY,step);y<getHeight();y+=step)g.fillRect(x,y,1,1);
            g.setColor(Theme.SECONDARY);g.drawString("Canvas · " + Math.round(zoom*100) + "% · Home resets view",16,25);
            g.translate(panX,panY);g.scale(zoom,zoom);
            if(id.equals("graph.editor")||id.equals("ui.flow")){
                g.setColor(Theme.TEXT);g.drawString(id.equals("ui.flow")?"Open a UI Flow document · F6":"Open a Graph document · F3",24,72);
                g.setColor(Theme.SECONDARY);g.drawString("Port styles / presentation guide",24,108);
                g.setColor(Theme.ACCENT);g.fillOval(26,126,10,10);g.setColor(Theme.TEXT);g.drawString("Typed data",46,136);
                g.setColor(Theme.WARNING);g.fillRect(26,154,10,10);g.setColor(Theme.TEXT);g.drawString("Control / event",46,164);
                g.setColor(Theme.DISABLED);g.drawString("Port → Scene binding · disabled until F3",24,205);
                g.setStroke(new BasicStroke(1, BasicStroke.CAP_BUTT,BasicStroke.JOIN_MITER,10,new float[]{5,5},0));
                g.drawRoundRect(20,222,Math.max(180,Math.min(350,getWidth()-40)),48,Theme.CONTROL_RADIUS*2,Theme.CONTROL_RADIUS*2);
                g.drawString("Drop preview unavailable",34,251);
            }else if(id.startsWith("animation.")){
                g.setColor(Theme.TEXT);g.drawString("Choose an animation clip · F7",24,72);
                for(int n=0;n<12;n++){int x=24+n*40;g.setColor(Theme.SECONDARY);g.drawString(Integer.toString(n),x,112);g.setColor(Theme.BORDER);g.drawLine(x,122,x,260);}
                g.setColor(Theme.DISABLED);g.drawString("Tracks, keyframes and markers require a clip",24,290);
            }else{
                String label=switch(id){case "ui.canvas"->"Choose a UI document · anchors / layout / preview · F6";
                    case "prefab.canvas"->"Choose a Prefab definition · F2";case "sprite.editor"->"Choose a sprite sheet · regions / pivot / slicing · F7";
                    case "tilemap.editor"->"Choose a tilemap · palette / cells / layers · F7";case "material.editor"->"Choose a material · texture / blend / preview · F7";
                    default->"Choose a document";};
                g.setColor(Theme.TEXT);g.drawString(label,24,72);
                g.setColor(Theme.BORDER);g.drawRoundRect(24,104,Math.max(120,Math.min(340,getWidth()-48)),150,Theme.CONTROL_RADIUS*2,Theme.CONTROL_RADIUS*2);
                g.setColor(Theme.DISABLED);g.drawString("No asset or runtime instance created",38,182);
            }
        }finally{g.dispose();}
    }
}
