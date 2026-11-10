package epidemic.editor;

import java.nio.*;
import java.nio.file.*;
import java.util.*;
import java.util.concurrent.*;
import java.awt.image.BufferedImage;
import javax.imageio.ImageIO;

/** Actual companion/serialized-model integration, never computer automation. */
public final class AssetTests {
    private static final List<Long> polling=new ArrayList<>(),heartbeat=new ArrayList<>();
    private static Bridge.Reply call(Bridge bridge,int op,String...args)throws Exception{return UiTests.result(bridge.call(op,args));}
    private static String id(long value){return Long.toUnsignedString(value);}
    private static Bridge.Reply edit(Bridge bridge,int op,String...args)throws Exception{
        String[] all=new String[args.length+1];all[0]=id(bridge.latest().snapshot().assetRevision());System.arraycopy(args,0,all,1,args.length);return call(bridge,op,all);
    }
    private static Bridge.Snapshot settle(Bridge bridge)throws Exception{
        long limit=System.nanoTime()+TimeUnit.SECONDS.toNanos(20);
        Bridge.Snapshot value;
        do{var reply=call(bridge,26);polling.add(reply.latencyNanos());long tick=System.nanoTime();javax.swing.SwingUtilities.invokeAndWait(()->{});heartbeat.add(System.nanoTime()-tick);value=reply.snapshot();if((value.flags()&16)==0)return value;Thread.sleep(15);}while(System.nanoTime()<limit);
        throw new IllegalStateException("Native import did not settle");
    }
    private static void png(Path file,int argb)throws Exception{
        BufferedImage image=new BufferedImage(260,132,BufferedImage.TYPE_INT_ARGB);for(int y=0;y<image.getHeight();y++)for(int x=0;x<image.getWidth();x++)image.setRGB(x,y,argb);ImageIO.write(image,"png",file.toFile());
    }
    public static void run(Path executable,Path directory)throws Exception{
        Path work=Files.createTempDirectory(directory,"F1-\u041f\u0440\u043e\u0435\u043a\u0442-"),root=work.resolve("project"),source=work.resolve("\u0442\u0435\u043a\u0441\u0442\u0443\u0440\u0430.png");png(source,0xffff0000);
        Path webp=work.resolve("alpha.webp");Files.write(webp,Base64.getDecoder().decode("UklGRhwAAABXRUJQVlA4TBAAAAAvAQAAEA8Q8x/zH4yM6H8A"));
        Path wave=work.resolve("sound.wav");ByteBuffer pcm=ByteBuffer.allocate(76).order(ByteOrder.LITTLE_ENDIAN);pcm.put("RIFF".getBytes(java.nio.charset.StandardCharsets.US_ASCII)).putInt(68).put("WAVEfmt ".getBytes(java.nio.charset.StandardCharsets.US_ASCII)).putInt(16).putShort((short)1).putShort((short)1).putInt(16000).putInt(32000).putShort((short)2).putShort((short)16).put("data".getBytes(java.nio.charset.StandardCharsets.US_ASCII)).putInt(32);Files.write(wave,pcm.array());
        Path font=Path.of(System.getenv("WINDIR"),"Fonts","arial.ttf");UiTests.require(Files.isRegularFile(font),"Windows real SFNT fixture missing");
        List<Path> formats=new ArrayList<>(List.of(source,webp,wave,font));
        try(var installed=Files.list(font.getParent())){installed.filter(path->path.getFileName().toString().toLowerCase(Locale.ROOT).endsWith(".otf")).findFirst().ifPresent(formats::add);}
        System.out.println("F1 actual format fixtures: "+formats.stream().map(path->path.getFileName().toString()).toList());
        long assetId,generation;String cooked,hash;
        try(Bridge bridge=new Bridge(executable,work,s->{})){
            UiTests.result(bridge.connect());call(bridge,3,root.toString(),"F1 assets");call(bridge,27,"0");
            for(Path external:formats){
                int count=bridge.latest().snapshot().assets().size();edit(bridge,18,external.toString(),external.getFileName().toString());
                var after=settle(bridge);UiTests.require(after.assets().size()==count+1,"Format import failed: "+external+" / "+after.assetState().events());
                var imported=after.assets().getLast();call(bridge,9,"6","0",id(imported.id()));
                var selected=bridge.latest().snapshot();UiTests.require(!selected.assets().getLast().cookedKey().isBlank(),"No cooked identity");
                UiTests.require(selected.assets().getLast().metadata().length()>5,"No typed metadata");
                if(external.equals(webp))UiTests.require(selected.assetState().width()==2&&selected.assetState().pixels().endsWith("00000000"),"WebP alpha conversion/preview failed");
                if(external.equals(source)){
                    UiTests.require(selected.assetState().width()>0&&selected.assetState().width()<=128,"Snapshot thumbnail must stay bounded");
                    long version=bridge.latest().generation();
                    for(int divisor:List.of(1,2,4)){
                        var preview=bridge.assetPreview(version,imported.id(),imported.cookedKey(),divisor).get(15,TimeUnit.SECONDS);
                        UiTests.require(preview.getWidth()==260/divisor&&preview.getHeight()==132/divisor&&
                            preview.getRGB(0,0)==0xffff0000,"PNG cooked preview quality "+divisor);
                    }
                    UiTests.require(bridge.latest().snapshot()==selected,"Preview replaced the project snapshot");
                }
                if(external.equals(webp)){
                    var preview=bridge.assetPreview(bridge.latest().generation(),imported.id(),imported.cookedKey(),1).get(15,TimeUnit.SECONDS);
                    UiTests.require(preview.getRGB(preview.getWidth()-1,preview.getHeight()-1)==0,"Full cooked preview lost alpha");
                }
            }
            UiTests.nativeReject(bridge,"editor.import_destination",18,id(bridge.latest().snapshot().assetRevision()),source.toString(),source.getFileName().toString().toUpperCase(Locale.ROOT));
            for(Path external:formats.subList(1,formats.size())){
                var record=bridge.latest().snapshot().assets().stream().filter(a->a.path().equals("Content/"+external.getFileName())).findFirst().orElseThrow();
                Path copied=root.resolve(record.path());Files.writeString(copied,"invalid media");edit(bridge,19,id(record.id()));settle(bridge);
                var failed=bridge.latest().snapshot().assets().stream().filter(a->a.id()==record.id()).findFirst().orElseThrow();UiTests.require(failed.cookedKey().equals(record.cookedKey())&&failed.generation()==record.generation(),"Typed last-good lost for "+external);
                Files.copy(external,copied,StandardCopyOption.REPLACE_EXISTING);edit(bridge,19,id(record.id()));settle(bridge);
                UiTests.require(bridge.latest().snapshot().assetState().events().stream().filter(e->e.id()==record.id()).reduce((a,b)->b).orElseThrow().kind()==4,"Typed recovery failed for "+external);
            }
            call(bridge,6);call(bridge,12,work.resolve("unicode-copy").toString());
            var unicode=bridge.latest().snapshot().assets().getFirst();edit(bridge,22,id(unicode.id()),"0");edit(bridge,23,id(unicode.id()));
            var asset=bridge.latest().snapshot().assets().getFirst();assetId=asset.id();generation=asset.generation();cooked=asset.cookedKey();hash=id(asset.fingerprint());
            call(bridge,9,"6","0",id(assetId));
            javax.swing.SwingUtilities.invokeAndWait(()->{AssetDetails view=new AssetDetails(false,x->{});view.refresh(bridge.latest().snapshot());UiTests.require(view.getComponentCount()==3,"Asset Details lost native preview");});
            call(bridge,15,"1","Sprite owner","registered-components");long prefab=bridge.latest().snapshot().selection().document();edit(bridge,24,id(prefab),id(assetId));
            UiTests.require(bridge.latest().snapshot().assetState().references().stream().anyMatch(r->r.source()==assetId&&r.dependent()==prefab),"Native stable reference missing");
            long copy=call(bridge,35,"duplicate",id(prefab),"Sprite owner copy").snapshot().selection().document();
            UiTests.require(copy!=prefab&&bridge.latest().snapshot().assetState().references().stream()
                .anyMatch(r->r.source()==assetId&&r.dependent()==copy),"Duplicated Prefab lost its sprite");
            call(bridge,35,"delete",id(copy));
            UiTests.require(bridge.latest().snapshot().documents().stream().noneMatch(doc->doc.id()==copy)&&
                bridge.latest().snapshot().assetState().references().stream().noneMatch(r->r.dependent()==copy),"Document deletion left an orphan binding");
            call(bridge,7);
            UiTests.require(bridge.latest().snapshot().assetState().references().stream()
                .anyMatch(r->r.source()==assetId&&r.dependent()==copy),"Undo did not restore document and sprite together");
            call(bridge,8);
            UiTests.nativeReject(bridge,"ui.entry_scene_delete",35,"delete",id(bridge.latest().snapshot().entry()));
            call(bridge,28,"sound");UiTests.require(bridge.latest().snapshot().assets().size()==1&&bridge.latest().snapshot().assetState().totalAssets()==formats.size(),"Native bounded catalog query did not filter");call(bridge,28,"");
            UiTests.require(bridge.latest().snapshot().assetState().sprites().stream().anyMatch(sprite->sprite.document()==prefab&&sprite.key().equals(cooked)),"Native render projection lost sprite binding");
            UiTests.nativeReject(bridge,"editor.import_extension",20,id(bridge.latest().snapshot().assetRevision()),id(assetId),"mismatch.wav");
            String longFolder="Long-"+"x".repeat(100)+"/"+"y".repeat(100);Files.createDirectories(root.resolve("Content").resolve(longFolder));
            edit(bridge,20,id(assetId),longFolder+"/asset.png");UiTests.require(bridge.latest().snapshot().assets().getFirst().path().equals("Content/"+longFolder+"/asset.png"),"Long logical path truncated");
            edit(bridge,20,id(assetId),"renamed.png");UiTests.require(bridge.latest().snapshot().assets().getFirst().id()==assetId,"Move changed AssetId");
            call(bridge,7);call(bridge,8);edit(bridge,21,id(assetId),"duplicate.png");UiTests.require(settle(bridge).assets().size()==formats.size()+1,"Duplicate failed");
            UiTests.nativeReject(bridge,"editor.asset_referenced",22,id(bridge.latest().snapshot().assetRevision()),id(assetId),"0");
            edit(bridge,22,id(assetId),"1");UiTests.require(bridge.latest().snapshot().assets().getFirst().trashed(),"No tombstone");call(bridge,7);call(bridge,8);edit(bridge,23,id(assetId));
            Path copied=root.resolve("Content/renamed.png");png(copied,0xff0000ff);edit(bridge,19,id(assetId));settle(bridge);
            var changed=bridge.latest().snapshot().assets().getFirst();UiTests.require(changed.id()==assetId&&changed.generation()>generation&&!changed.cookedKey().equals(cooked)&&!id(changed.fingerprint()).equals(hash),"Reimport lost identity/digest/generation");
            call(bridge,9,"6","0",id(assetId));String good=bridge.latest().snapshot().assetState().pixels();String key=changed.cookedKey();
            Files.writeString(copied,"malformed PNG");edit(bridge,19,id(assetId));settle(bridge);call(bridge,9,"6","0",id(assetId));
            UiTests.require(bridge.latest().snapshot().assets().getFirst().cookedKey().equals(key)&&bridge.latest().snapshot().assetState().pixels().equals(good),"Invalid source destroyed last-good");
            UiTests.require(bridge.latest().snapshot().assetState().events().stream().anyMatch(e->e.id()==assetId&&e.kind()==3),"Native failed status absent");
            png(copied,0xff00ff00);call(bridge,27,"1");long until=System.nanoTime()+TimeUnit.SECONDS.toNanos(12);
            while(bridge.latest().snapshot().assets().getFirst().cookedKey().equals(key)&&System.nanoTime()<until){call(bridge,26);Thread.sleep(30);}
            UiTests.require(!bridge.latest().snapshot().assets().getFirst().cookedKey().equals(key),"Watcher did not recover generation");
            call(bridge,27,"0");
            edit(bridge,18,source.toString(),"cancel-active.png");call(bridge,26);Thread.sleep(50);call(bridge,25);settle(bridge);
            UiTests.require(bridge.latest().snapshot().assets().size()==formats.size()+1&&bridge.latest().snapshot().assetState().events().stream().anyMatch(e->e.job()>0&&e.kind()==5&&e.source().equals("cancel-active.png")),"Completed but unpublished worker cancellation lost its terminal job outcome");
            edit(bridge,19,id(assetId));call(bridge,25);settle(bridge);UiTests.require(bridge.latest().snapshot().assetState().events().stream().anyMatch(e->e.id()==assetId&&e.kind()==5),"Manual reimport cancellation not observable");
            for(int i=0;i<4;i++)edit(bridge,18,source.toString(),"cancel-"+i+".png");call(bridge,25);settle(bridge);UiTests.require(bridge.latest().snapshot().assets().size()==formats.size()+1,"Cancelled queued imports published assets");
            UiTests.require(bridge.latest().snapshot().assetState().events().stream().anyMatch(e->e.job()>0&&e.kind()==5),"No actual job cancellation outcomes");
            UiTests.nativeReject(bridge,"ui.stale_asset_revision",20,"0",id(assetId),"bad.png");
            UiTests.nativeReject(bridge,"editor.import_destination",18,id(bridge.latest().snapshot().assetRevision()),source.toString(),"../escape.png");
            call(bridge,6);call(bridge,12,work.resolve("clone").toString());call(bridge,10);call(bridge,11);
        }
        for(Path saved:List.of(root,work.resolve("clone"))){try(Bridge bridge=new Bridge(executable,work,s->{})){
            UiTests.result(bridge.connect());call(bridge,2,saved.resolve("Project.epidemicproj").toString());UiTests.require(bridge.latest().snapshot().assets().size()==formats.size()+1&&bridge.latest().snapshot().assets().getFirst().id()==assetId,"Cross-process asset identity/publication lost");call(bridge,10);call(bridge,11);
        }}
        polling.sort(Long::compare);heartbeat.sort(Long::compare);System.out.printf(java.util.Locale.ROOT,"F1 import polling samples=%d p50=%.3fms p95=%.3fms; EDT heartbeat p95=%.3fms%n",polling.size(),polling.get(polling.size()/2)/1e6,polling.get(polling.size()*95/100)/1e6,heartbeat.get(heartbeat.size()*95/100)/1e6);
        System.out.println("PASS F1 PNG/WebP/PCM WAV/SFNT import, typed metadata/alpha thumbnail, stable refs/move/history/duplicate/trash/restore, reimport/watcher/last-good, cancellation, traversal/stale rejection, Save/reopen/clone");
    }
}
