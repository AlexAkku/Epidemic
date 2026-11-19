package epidemic.editor;

import java.awt.*;
import javax.swing.*;
import javax.swing.border.EmptyBorder;

public final class Theme {
    public static final Color WINDOW = new Color(0x17171d), TOOL = new Color(0x202028),
        DOCUMENT = new Color(0x191920), ELEVATED = new Color(0x282832), TAB = new Color(0x30303c),
        BORDER = new Color(0x363640), TEXT = new Color(0xf0f0f4), SECONDARY = new Color(0xb5b5c3),
        DISABLED = new Color(0x90909f), ACCENT = new Color(0xa855d5), ERROR = new Color(0xff707c),
        WARNING = new Color(0xf1ba6c), BRAND_FROM = new Color(0x7719a6), BRAND_TO = new Color(0xff0000);
    public static final int PANEL_RADIUS = 10, CONTROL_RADIUS = 6, GAP = 6, UNIT = 4;
    private Theme() { }
    /** Width follows the dock; preferred height includes every wrapped row. */
    public static JPanel actionRows() {
        return new JPanel(new FlowLayout(FlowLayout.LEADING,6,4)) {
            @Override public void setBounds(int x,int y,int width,int height){
                boolean changed=getWidth()!=width;super.setBounds(x,y,width,height);
                if(changed)SwingUtilities.invokeLater(this::revalidate);
            }
            @Override public Dimension getPreferredSize() {
                int width=getWidth();
                if(width<=0&&getParent()!=null)width=getParent().getWidth();
                if(width<=0)width=240;
                Insets in=getInsets();int available=Math.max(1,width-in.left-in.right-12);
                int row=0,height=0,used=0;
                for(Component child:getComponents())if(child.isVisible()){
                    Dimension size=child.getPreferredSize();
                    if(used>0&&used+6+size.width>available){height+=row+4;used=0;row=0;}
                    used+=(used==0?0:6)+size.width;row=Math.max(row,size.height);
                }
                return new Dimension(width,in.top+in.bottom+height+row+8);
            }
            @Override public Dimension getMinimumSize(){return new Dimension(0,getPreferredSize().height);}
        };
    }
    public static JPanel island(LayoutManager layout) {
        JPanel panel = new JPanel(layout) {
            @Override protected void paintComponent(Graphics graphics) {
                Graphics2D g = (Graphics2D)graphics.create();
                try {
                    g.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
                    g.setColor(TOOL); g.fillRoundRect(0, 0, getWidth(), getHeight(), PANEL_RADIUS * 2, PANEL_RADIUS * 2);
                } finally { g.dispose(); }
            }
        };
        panel.setOpaque(false); panel.setBorder(new EmptyBorder(GAP, GAP, GAP, GAP)); return panel;
    }
    public static JLabel label(String text, boolean secondary) {
        JLabel label = new JLabel(text); label.setForeground(secondary ? SECONDARY : TEXT); return label;
    }
    public static JButton button(String label, Runnable action) {
        if(UIManager.getLookAndFeel().getClass().getName().startsWith("com.intellij.")){
            JButton button=new JButton(label);button.addActionListener(event->action.run());return button;
        }
        JButton button = new JButton(label) {
            @Override protected void paintComponent(Graphics graphics) {
                Graphics2D g = (Graphics2D)graphics.create();
                try {
                    g.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
                    g.setColor(getModel().isPressed() ? ELEVATED : getModel().isRollover() ? TAB : TOOL);
                    g.fillRoundRect(0, 0, getWidth(), getHeight(), CONTROL_RADIUS * 2, CONTROL_RADIUS * 2);
                    g.setColor(hasFocus() ? ACCENT : BORDER);
                    g.drawRoundRect(0, 0, getWidth()-1, getHeight()-1, CONTROL_RADIUS * 2, CONTROL_RADIUS * 2);
                } finally { g.dispose(); }
                super.paintComponent(graphics);
            }
        };
        button.setContentAreaFilled(false); button.setBorder(new EmptyBorder(7, 10, 7, 10));
        button.addActionListener(event -> action.run()); return button;
    }
}
