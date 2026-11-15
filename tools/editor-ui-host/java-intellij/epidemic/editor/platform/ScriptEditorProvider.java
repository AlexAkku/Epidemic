package epidemic.editor.platform;

import com.intellij.openapi.editor.*;
import com.intellij.openapi.editor.ex.EditorEx;
import com.intellij.openapi.fileEditor.*;
import com.intellij.openapi.fileTypes.*;
import com.intellij.openapi.project.*;
import com.intellij.openapi.util.UserDataHolderBase;
import com.intellij.openapi.vfs.VirtualFile;
import com.intellij.lexer.LexerBase;
import com.intellij.psi.tree.IElementType;
import com.intellij.lang.Language;
import com.intellij.openapi.editor.colors.TextAttributesKey;
import javax.swing.*;
import java.awt.BorderLayout;
import java.beans.PropertyChangeListener;
import java.util.Set;

/** Actual Platform Editor, Document, editor actions, search and Undo stack. */
public final class ScriptEditorProvider implements FileEditorProvider,DumbAware {
    public boolean accept(Project project,VirtualFile file){return file.getUserData(ScriptWorkspace.OWNER)!=null;}
    public String getEditorTypeId(){return "epidemic.script";}
    public FileEditorPolicy getPolicy(){return FileEditorPolicy.HIDE_DEFAULT_EDITOR;}
    public FileEditor createEditor(Project project,VirtualFile file){return new SourceEditor(project,file);}
    static final class SourceEditor extends UserDataHolderBase implements FileEditor,TextEditor {
        final VirtualFile file;final Editor editor;final JPanel component=new JPanel(new BorderLayout());
        final java.beans.PropertyChangeSupport changes=new java.beans.PropertyChangeSupport(this);
        Timer dirtyTimer;boolean modified;final ScriptWorkspace owner;final Object sourceToken;
        SourceEditor(Project project,VirtualFile file){this.file=file;owner=file.getUserData(ScriptWorkspace.OWNER);sourceToken=owner==null?null:owner.editorToken(file);if(owner==null||sourceToken==null)throw new com.intellij.openapi.progress.ProcessCanceledException();var document=FileDocumentManager.getInstance().getDocument(file);if(document==null)throw new IllegalStateException("script.document");
            editor=EditorFactory.getInstance().createEditor(document,project);try{editor.getSettings().setLineNumbersShown(true);editor.getSettings().setUseSoftWraps(false);
            ((EditorEx)editor).setHighlighter(com.intellij.openapi.editor.highlighter.EditorHighlighterFactory.getInstance().createEditorHighlighter(new Highlighter(),editor.getColorsScheme()));
            component.add(owner.editorBar(file),BorderLayout.NORTH);component.add(editor.getComponent());
            project.getService(PlatformSession.class).sourceShortcuts(component,this);
            modified=isModified();dirtyTimer=new Timer(250,event->refreshModified());dirtyTimer.start();
            document.addDocumentListener(new com.intellij.openapi.editor.event.DocumentListener(){public void beforeDocumentChange(com.intellij.openapi.editor.event.DocumentEvent event){owner.beforeEdit(event.getDocument());}public void documentChanged(com.intellij.openapi.editor.event.DocumentEvent event){refreshModified();}},this);
            }catch(RuntimeException|Error error){com.intellij.openapi.util.Disposer.dispose(this);throw error;}
        }
        private void refreshModified(){boolean next=isModified();if(next!=modified){boolean before=modified;modified=next;changes.firePropertyChange(FileEditor.getPropModified(),before,next);}}
        public Editor getEditor(){return editor;}
        public JComponent getComponent(){return component;}
        public JComponent getPreferredFocusedComponent(){return editor.getContentComponent();}
        public String getName(){return "C#";}
        public void setState(FileEditorState state){}
        public boolean isModified(){return FileDocumentManager.getInstance().isDocumentUnsaved(editor.getDocument());}
        public boolean isValid(){return file.isValid()&&!editor.isDisposed();}
        public VirtualFile getFile(){return file;}
        public void addPropertyChangeListener(PropertyChangeListener listener){changes.addPropertyChangeListener(listener);}
        public void removePropertyChangeListener(PropertyChangeListener listener){changes.removePropertyChangeListener(listener);}
        public boolean canNavigateTo(com.intellij.pom.Navigatable target){return target instanceof OpenFileDescriptor;}
        public void navigateTo(com.intellij.pom.Navigatable target){if(target instanceof OpenFileDescriptor descriptor){int offset=descriptor.getOffset();if(offset>=0)editor.getCaretModel().moveToOffset(Math.min(offset,editor.getDocument().getTextLength()));else editor.getCaretModel().moveToLogicalPosition(new LogicalPosition(Math.max(0,Math.min(descriptor.getLine(),editor.getDocument().getLineCount()-1)),Math.max(0,descriptor.getColumn())));editor.getScrollingModel().scrollToCaret(ScrollType.CENTER);}}
        public void dispose(){owner.releasePanel(component);if(dirtyTimer!=null)dirtyTimer.stop();if(!editor.isDisposed())EditorFactory.getInstance().releaseEditor(editor);owner.editorReleased(file,sourceToken);}
    }
    static final Language LANGUAGE=new Language("Epidemic CSharp"){};
    static final IElementType KEYWORD=new IElementType("keyword",LANGUAGE),COMMENT=new IElementType("comment",LANGUAGE),STRING=new IElementType("string",LANGUAGE),NUMBER=new IElementType("number",LANGUAGE),TEXT=new IElementType("text",LANGUAGE);
    static final Set<String> WORDS=Set.of("using","namespace","public","private","protected","internal","sealed","abstract","class","struct","interface","enum","void","static","return","new","if","else","for","foreach","while","switch","case","break","throw","try","catch","finally","long","double","bool","string","int","float","true","false","null","var","get","set","checked","async","await","override","virtual","readonly","const","this","base","typeof");
    static final class Lexer extends LexerBase {
        CharSequence source="";int at,end,stop;IElementType type;
        public void start(CharSequence buffer,int start,int end,int state){source=buffer;at=start;stop=end;scan();}
        void scan(){end=at;if(at>=stop){type=null;return;}char first=source.charAt(end++);type=TEXT;
            if(first=='/'&&end<stop&&(source.charAt(end)=='/'||source.charAt(end)=='*')){boolean block=source.charAt(end++)=='*';type=COMMENT;while(end<stop){if(!block&&source.charAt(end)=='\n')break;if(block&&source.charAt(end)=='*'&&end+1<stop&&source.charAt(end+1)=='/'){end+=2;break;}++end;}}
            else if(first=='"'||first=='\''){type=STRING;while(end<stop){char c=source.charAt(end++);if(c=='\\'&&end<stop)++end;else if(c==first)break;}}
            else if(Character.isJavaIdentifierStart(first)){while(end<stop&&Character.isJavaIdentifierPart(source.charAt(end)))++end;if(WORDS.contains(source.subSequence(at,end).toString()))type=KEYWORD;}
            else if(Character.isDigit(first)){type=NUMBER;while(end<stop&&(Character.isDigit(source.charAt(end))||source.charAt(end)=='.'))++end;}
        }
        public int getState(){return 0;}public IElementType getTokenType(){return type;}public int getTokenStart(){return at;}public int getTokenEnd(){return end;}
        public void advance(){at=end;scan();}public CharSequence getBufferSequence(){return source;}public int getBufferEnd(){return stop;}
    }
    static final class Highlighter extends SyntaxHighlighterBase {
        public com.intellij.lexer.Lexer getHighlightingLexer(){return new Lexer();}
        public TextAttributesKey[] getTokenHighlights(IElementType token){return pack(token==KEYWORD?DefaultLanguageHighlighterColors.KEYWORD:token==COMMENT?DefaultLanguageHighlighterColors.LINE_COMMENT:token==STRING?DefaultLanguageHighlighterColors.STRING:token==NUMBER?DefaultLanguageHighlighterColors.NUMBER:null);}
    }
}
