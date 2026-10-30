#include "Epidemic/Game/Editor/content_browser.h"
#include "Epidemic/Game/Editor/png_import.h"
#include "Epidemic/Game/Editor/image_resource.h"
#include "Epidemic/Runtime/Resources/resource_services.h"
#include "../example/demo_game.h"
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <cstdio>
namespace ed=epidemic::game::editor;
namespace ex=ed::example;
namespace fs=std::filesystem;
void Check(bool pass,const char *why){if(!pass)throw std::runtime_error(why);}
void ReadCopiedImage(){
    const auto root=fs::path{EPIDEMIC_EDITOR_TEST_DIRECTORY}/"p04-image-relocated";
    ex::Schema schema;auto project=ex::Take(ed::ProjectFolder::Open(root,schema.View()));
    const auto listing=ed::BrowseContent(*project,{"Sprites/","real.png",{}});
    Check(listing.items.size()==1 && listing.items.front().present,"Copied project source resolves without original root");
    const auto catalog=ex::Take(project->BuildSourceCatalog());
    Check(catalog->Contains(listing.items.front().id),"Copied stable identity resolves through existing Assets");
    ed::ImageResourceLoader loader{root};auto services=ex::Take(epidemic::runtime::CreateResourceServices());
    ex::Take(services.loaders->RegisterLoader(loader));ex::Take(services.loaders->Freeze());
    const auto &source=project->Sources().back();const auto id=ex::Take(loader.Register(source.cooked_key));
    const auto lease=ex::Take(services.manager->RequestLease({id,loader.GetResourceType()}));
    ex::Take(services.manager->ProcessPendingLoads());
    const auto payload=std::dynamic_pointer_cast<const ed::ImagePayload>(services.manager->GetPayload(lease.resource));
    Check(payload && payload->image.pixels[0]==std::byte{128} && payload->image.pixels[2]==std::byte{0},"Cross-process relocated last-good pixels");
    ex::Take(services.manager->Release(lease));
    auto play=ex::Take(project->Play(ex::Options(schema)));
    for(std::uint64_t i=0;i<3;++i){
        const auto sprite=ex::Take(play->ReadSprite(epidemic::runtime::SceneNodeId{42+i}));
        Check(sprite.texture==source.id && sprite.cooked_key==source.cooked_key && sprite.width==1 && sprite.height==1,
            "Cross-process Sprite identity/resource generation");
        Check(sprite.world.position.x==static_cast<float>(static_cast<int>(i)*3-3),"Cross-process Sprite placement transform");
    }
    ex::Take(play->Stop());Check(!play->ReadSprite(epidemic::runtime::SceneNodeId{42}),"Stopped session retained Sprite access");
}
int main(int argc,char **argv){
 try{
    if(argc==2 && std::string_view{argv[1]}=="--read-image"){ReadCopiedImage();return 0;}
    Check(argc==1,"Unexpected content test arguments");
    const auto root=fs::path{EPIDEMIC_EDITOR_TEST_DIRECTORY}/"p04-browser-import";
    std::error_code ec;fs::remove_all(root,ec);
    ex::Schema schema;
    auto definition=ex::Definition(schema);
    auto &scene=std::get<epidemic::game::composition::SceneCompositionDefinition>(definition.documents.front().data);
    scene.scene.nodes.front().local_transform.position.x=-3;
    for(std::uint64_t i=1;i<3;++i){auto node=scene.scene.nodes.front();node.id=epidemic::runtime::SceneNodeId{42+i};node.local_transform.position.x=static_cast<float>(static_cast<int>(i)*3-3);scene.scene.nodes.push_back(node);auto placement=scene.placements.front();placement.node=node.id;scene.placements.push_back(placement);}
    auto project=ex::Take(ed::ProjectFolder::Create(root,"Browser Test",std::move(definition),schema.View()));
    fs::create_directories(root/"Content"/"Sprites");
    {std::ofstream out(root/"Content"/"Sprites"/"bad.png",std::ios::binary);out<<"not a png";}
    Check(!ed::ImportProjectPng(*project,"Sprites/bad.png"),"Invalid PNG must fail without registering an asset");
    Check(project->Sources().empty(),"Invalid PNG changed project sources");
    const auto source=root/"Content"/"Sprites"/"notes.txt";
    {std::ofstream out(source);out<<"hello";}
    const auto type=epidemic::runtime::AssetType{ex::Id("editor.test.source")};
    const auto id=ex::Take(project->IndexSource("Sprites/notes.txt",type));
    epidemic::runtime::graph::GraphDefinition draft;draft.asset_id=epidemic::runtime::AssetId::FromString("editor.content.valid.draft");
    ex::Take(project->Execute({project->Revision(),{ed::CreateDocument{{draft.asset_id,"Legal draft",0,draft}}}}));
    ex::Take(project->Undo());
    const auto before_collision=project->Snapshot();draft.asset_id=id;
    Check(!project->Execute({project->Revision(),{ed::CreateDocument{{id,"Collision",0,draft}}}}) && project->Snapshot()==before_collision,"Source identity cannot be reused by an authoring document");
    auto view=ed::BrowseContent(*project,{"Sprites/","NOTES",type});
    Check(view.items.size()==1 && view.items[0].id==id && view.items[0].present,"Browser query lost source identity");
    Check(view.folders.size()==1 && view.folders[0]=="Sprites","Browser folders are missing");
    Check(!ed::RenameContentAsset(*project,id,"../escape.txt"),"Traversal was accepted");
    Check(!ed::RenameContentAsset(*project,id,"Sprites/bad.png"),"Overwrite was accepted");
    ex::Take(ed::RenameContentAsset(*project,id,"Sprites/moved.txt"));
    Check(!fs::exists(source) && fs::exists(root/"Content"/"Sprites"/"moved.txt"),"Move did not modify filesystem");
    ex::Take(project->UndoAssetMove());Check(fs::exists(source),"Asset move Undo restores source");
    ex::Take(project->RedoAssetMove());Check(!fs::exists(source),"Asset move Redo restores destination");
    view=ed::BrowseContent(*project,{"Sprites/","moved",type});
    Check(view.items.size()==1 && view.items[0].id==id,"Move changed AssetId");
    ex::Take(project->Save());
    project.reset();
    project=ex::Take(ed::ProjectFolder::Open(root,schema.View()));
    view=ed::BrowseContent(*project);
    Check(view.items.size()==1 && view.items[0].id==id && view.items[0].path=="Sprites/moved.txt","Move not persisted");
    ex::Take(project->MoveSource(id,"Sprites/uncommitted.txt"));
    project.reset();project=ex::Take(ed::ProjectFolder::Open(root,schema.View()));
    Check(fs::exists(root/"Content/Sprites/moved.txt") && !fs::exists(root/"Content/Sprites/uncommitted.txt"),"Reopen rolls back uncommitted filesystem move");
#ifdef _WIN32
    constexpr unsigned char png[]={137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,1,0,0,0,1,8,6,0,0,0,31,21,196,137,0,0,0,13,73,68,65,84,120,156,99,248,207,192,208,0,0,4,129,1,128,44,85,206,176,0,0,0,0,73,69,78,68,174,66,96,130};
    const auto image=root/"Content/Sprites/real.png";
    {std::ofstream out(image,std::ios::binary);out.write(reinterpret_cast<const char *>(png),sizeof(png));}
    const auto decoded=ex::Take(ed::DecodePng(image));
    Check(decoded.width==1 && decoded.height==1 && decoded.pixels.size()==4,"Real PNG dimensions");
    Check(decoded.pixels[0]==std::byte{0} && decoded.pixels[1]==std::byte{0} && decoded.pixels[2]==std::byte{128} && decoded.pixels[3]==std::byte{128},"PNG premultiplied BGRA alpha");
    const auto texture=ex::Take(ed::ImportProjectPng(*project,"Sprites/real.png"));
    const auto original=project->Sources().back();
    Check(original.cooked_key.size()==64,"Cooked SHA-256 key");
    ed::ImageResourceLoader loader{root};
    auto resources=ex::Take(epidemic::runtime::CreateResourceServices());
    ex::Take(resources.loaders->RegisterLoader(loader)); ex::Take(resources.loaders->Freeze());
    const auto old_id=ex::Take(loader.Register(original.cooked_key));
    const auto old=ex::Take(resources.manager->RequestLease({old_id,loader.GetResourceType()}));
    ex::Take(resources.manager->ProcessPendingLoads());
    const auto old_pixels=std::dynamic_pointer_cast<const ed::ImagePayload>(resources.manager->GetPayload(old.resource));
    Check(old_pixels && old_pixels->image.pixels==decoded.pixels,"Cooked ResourceLoader/payload round-trip");
    for(std::uint64_t key=1;key<=600;++key){
        auto digest=std::string(64,'0');auto value=key;
        for(std::size_t digit=0;value;++digit){digest[63-digit]="0123456789abcdef"[value%16];value/=16;}
        Check(static_cast<bool>(loader.Register(std::move(digest))),"More than 256 image keys must register");
    }
    Check(loader.RegistrationCount()==601,"Registration count must include all live candidates");
    loader.CollectUnused(*resources.manager);
    Check(loader.RegistrationCount()==1 && resources.manager->IsReady(old.resource),"Collection must retain active old leases only");
    Check(ed::MakeImageThumbnail(old_pixels->image).pixels==decoded.pixels,"Thumbnail alpha matches same backend");
    Check(ex::Take(ed::CookProjectPng(*project,"Sprites/real.png"))==texture && project->Sources().back().generation==original.generation,"Deterministic unchanged reimport");
    constexpr unsigned char blue[]={137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,1,0,0,0,1,8,6,0,0,0,31,21,196,137,0,0,0,13,73,68,65,84,120,156,99,96,96,248,223,0,0,2,131,1,128,67,187,248,3,0,0,0,0,73,69,78,68,174,66,96,130};
    {std::ofstream out(image,std::ios::binary);out.write(reinterpret_cast<const char *>(blue),sizeof(blue));}
    Check(ex::Take(ed::CookProjectPng(*project,"Sprites/real.png"))==texture,"Reimport preserves AssetId");
    const auto updated=project->Sources().back();
    Check(updated.generation==original.generation+1 && updated.cooked_key!=original.cooked_key,"Reimport advances cooked generation");
    const auto new_id=ex::Take(loader.Register(updated.cooked_key));
    const auto next=ex::Take(resources.manager->RequestLease({new_id,loader.GetResourceType()}));
    ex::Take(resources.manager->ProcessPendingLoads());
    const auto new_pixels=std::dynamic_pointer_cast<const ed::ImagePayload>(resources.manager->GetPayload(next.resource));
    Check(new_pixels && new_pixels->image.pixels[0]==std::byte{128} && old_pixels->image.pixels[2]==std::byte{128},"Old/new active leases retain independent pixels");
    ex::Take(resources.manager->Release(old));ex::Take(resources.manager->Release(next));
    loader.CollectUnused(*resources.manager);
    Check(loader.RegistrationCount()==0,"Released image generations must not accumulate loader keys");
    Check(ex::Take(loader.Register(original.cooked_key))==old_id,"Re-registering an evicted digest preserves resource identity");
    std::atomic_bool cancelled{true};
    Check(!ed::CookProjectPng(*project,"Sprites/real.png",&cancelled) && project->Sources().back().cooked_key==updated.cooked_key,"Cancelled candidate retains last-good");
    ex::Take(project->Save());project.reset();project=ex::Take(ed::ProjectFolder::Open(root,schema.View()));
    const auto copied=fs::path{EPIDEMIC_EDITOR_TEST_DIRECTORY}/"p04-image-relocated";
    fs::remove_all(copied,ec);
    view=ed::BrowseContent(*project,{"Sprites/","real.png",{}});
    Check(view.items.size()==1 && view.items[0].id==texture,"PNG identity survives reopen");
    {std::ofstream out(image,std::ios::binary);out.write(reinterpret_cast<const char *>(png),8);}
    Check(!ed::DecodePng(image),"Truncated PNG rejected");
    const auto last_good=project->Sources().back();
    Check(!ed::CookProjectPng(*project,"Sprites/real.png") && project->Sources().back().cooked_key==last_good.cooked_key && project->Sources().back().generation==last_good.generation,"Failed reimport retains cooked identity/generation");
    ex::Take(project->Save());
    project.reset();project=ex::Take(ed::ProjectFolder::Open(root,schema.View()));
    Check(project->Sources().back().cooked_key==last_good.cooked_key,"Last-good survives corrupted source and reopening");
    ex::Take(project->SetPrefabSprite(epidemic::runtime::AssetId::FromString("example.prefab"),texture));
    ex::Take(project->Save());
    fs::create_directories(root/"Content/.imports");
    {std::ofstream staging(root/"Content/.imports/unfinished.png");staging<<"private incomplete source";}
    Check(!project->IndexSource(".IMPORTS/unfinished.png",type),"Private staging path accepted as an asset");
    ex::Take(project->CloneTo(copied)); // Broken source still transports last-good cooked data.
    Check(!fs::exists(copied/"Content/.imports"),"Clone copied private import staging files");
    fs::remove(image); ex::Take(project->Save());
    Check(!ed::BrowseContent(*project,{"Sprites/","real.png",{}}).items.front().present,"Missing source remains visible and saveable");
#endif
    std::cout<<"P04 Content Browser and PNG tests PASS\n";
    return 0;
 }catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
 catch(...){std::fputs("Unexpected content test failure\n",stderr);return 1;}
}
