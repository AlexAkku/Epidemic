package epidemic.editor;

import epidemic.u1.NativeViewport;
import java.awt.*;
import java.util.List;
import java.util.function.Supplier;
import javax.swing.*;
import javax.swing.table.DefaultTableModel;

/** Presentation of existing native diagnostics. Missing providers remain explicitly unavailable. */
public final class RuntimeProfiler extends JPanel {
    private final DefaultTableModel rows=new DefaultTableModel(new String[]{"Metric","Value","Unit","Source / bounded interval"},0){public boolean isCellEditable(int r,int c){return false;}};
    private final Supplier<List<NativeViewport>> surfaces;
    private final java.util.function.BooleanSupplier building;
    public RuntimeProfiler(Supplier<List<NativeViewport>> surfaces,java.util.function.BooleanSupplier building){super(new BorderLayout());this.surfaces=surfaces;this.building=building;var table=new JTable(rows);table.setAutoResizeMode(JTable.AUTO_RESIZE_OFF);for(int c=0;c<4;c++)table.getColumnModel().getColumn(c).setPreferredWidth(c==3?450:160);add(new JLabel("Read-only native diagnostics · sampled 100 ms live / 500 ms stopped"),BorderLayout.NORTH);add(new JScrollPane(table));}
    private void row(String name,double value,String unit,String source){rows.addRow(new Object[]{name,value<0||!Double.isFinite(value)?"Unavailable":String.format(java.util.Locale.ROOT,"%.3f",value),unit,source});}
    public void refresh(Bridge.RuntimeSession frame){
        rows.setRowCount(0);row("Fixed ticks",frame.tick(),"ticks","Native GameClock / immutable RuntimeSnapshot");row("Dropped catch-up ticks",frame.droppedTicks(),"ticks","Native PlaySession bounded accumulator");row("Graph node steps",frame.graphSteps(),"steps","Native debugger commands; does not advance GameClock");for(var metric:frame.metrics())row(metric.name(),metric.value(),metric.unit(),metric.source());
        int index=0;for(var surface:surfaces.get())if(surface.isShowing()){
            double[] stats=surface.frameStatistics();if(stats.length<6)continue;String prefix="HWND "+(++index)+" · ";
            row(prefix+"FPS",stats[0],"frames/s","Native JAWT Renderer submit interval / last256");row(prefix+"Frame p50",stats[1],"ms","Native JAWT Renderer submit interval / last256");row(prefix+"Frame p95",stats[2],"ms","Native JAWT Renderer submit interval / last256");row(prefix+"Width",stats[4],"physical px","Native HWND GetClientRect");row(prefix+"Height",stats[5],"physical px","Native HWND GetClientRect");
        }
        long[] process=NativeViewport.profilerMetrics();if(process.length>=9){row("Native surfaces",process[0],"surfaces","Existing NativeViewport diagnostics");row("UI process working set",process[3],"bytes","Windows PSAPI / UI process");row("Preview residency",process[6],"bytes","Existing native ScenePreview resource accounting");row("Preview textures",process[7],"textures","Existing native ScenePreview resource accounting");}
        row("JCEF renderer RAM",-1,"bytes","Unavailable: renderer Auditor provider not attached; CDP heap measured in standalone smoke");
        row("Managed build queue",building.getAsBoolean()?1:0,"jobs","Existing ScriptWorkspace single-build owner / UI sample");
    }
}
