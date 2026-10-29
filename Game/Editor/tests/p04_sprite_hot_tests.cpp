#include "Epidemic/Game/Editor/content_hot_reload.h"
#include "Epidemic/Game/Editor/content_dependencies.h"
#include "Epidemic/Game/Editor/image_resource.h"
#include "../example/demo_game.h"
#include <filesystem>
#include <chrono>
#include <thread>
#include <fstream>
#include <iostream>
#include <cstdio>
#include <stdexcept>

namespace ed=epidemic::game::editor;
namespace ex=ed::example;
namespace fs=std::filesystem;
void Check(bool condition,const char *error) {if(!condition)throw std::runtime_error(error);}
int main() {
    try {
        const auto root=fs::path{EPIDEMIC_EDITOR_TEST_DIRECTORY}/"p04-sprite-binding";
        std::error_code ec;fs::remove_all(root,ec);
        ex::Schema schema;
        auto project=ex::Take(ed::ProjectFolder::Create(root,"Sprite Test",ex::Definition(schema),schema.View()));
        fs::create_directories(root/"Content"/"Sprites");
        // Deliberately not a PNG: this test qualifies authoring persistence,
        // not the WIC decoder (covered by EpidemicEditorContentTests).
        {std::ofstream out(root/"Content/Sprites/dummy.png",std::ios::binary);out<<"fake png";}
        const auto type=epidemic::runtime::AssetType{ex::Id("epidemic.texture.png")};
        const auto id=ex::Take(project->IndexSource("Sprites/dummy.png",type,std::string(64,'a')));
        const auto prefab=epidemic::runtime::AssetId::FromString("example.prefab");
        Check(!project->SetPrefabSprite(prefab,{},-1,0,64),"invalid pivot accepted");
        Check(!project->SetPrefabSprite(prefab,id,0.5f,0.5f,0),"invalid PPU accepted");
        Check(!project->SetPrefabSprite(epidemic::runtime::AssetId::FromString("no.prefab"),id),"missing prefab accepted");
        ex::Take(project->SetPrefabSprite(prefab,id,0.25f,0.75f,32));
        Check(project->IsDirty() && project->SpriteBindings().size()==1,"binding not authoring dirty");
        const auto deps=ed::BuildContentDependencyIndex(*project);
        const auto incoming=deps.Incoming(id);
        Check(incoming.size()==1 && incoming.front().dependent==prefab,"typed reverse reference missing");
        ex::Take(project->Save());
        ex::Take(project->SetPrefabSprite(prefab,id,0.75f,0.25f,32));
        ex::Take(project->MoveSource(id,"Sprites/dirty-probe.png"));
        ex::Take(project->UndoAssetMove());
        Check(project->IsDirty(),"Source move Undo erased unsaved Sprite metadata dirty state");
        ex::Take(project->SetPrefabSprite(prefab,id,0.25f,0.75f,32));
        ex::Take(project->Save());
        project.reset();project=ex::Take(ed::ProjectFolder::Open(root,schema.View()));
        const auto &binding=project->SpriteBindings().at(0);
        Check(binding.texture==id && binding.prefab==prefab && binding.pivot_x==0.25f && binding.pivot_y==0.75f && binding.pixels_per_unit==32,
              "sprite binding failed cross-process-compatible archive reopen");
        ex::Take(project->MoveSource(id,"Sprites/renamed.png"));
        Check(ed::BuildContentDependencyIndex(*project).Incoming(id).size()==1,"rename destroyed sprite reference");
        ex::Take(project->Save());
        ed::ContentHotReload monitor;monitor.Reset(*project);
        {std::ofstream out(root/"Content/Sprites/renamed.png",std::ios::binary);out<<"not real PNG, source edited";}
        Check(monitor.Poll(*project).empty(),"hot reload must debounce first observation");
        bool queued=false,failed=false;
        for(int attempt=0;attempt<200 && !failed;++attempt) {
            const auto events=monitor.Poll(*project);
            for(const auto &event:events)if(event.id==id) {
                queued|=event.kind==ed::HotAssetEventKind::Queued;
                failed|=event.kind==ed::HotAssetEventKind::Failed;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds{5});
        }
        Check(queued && failed,"async watcher must queue and eventually report failed PNG cook");
        Check(project->Sources().front().cooked_key==std::string(64,'a'),"failed watcher cook lost last-good key");
        ex::Take(project->SetPrefabSprite(prefab,{}));
        Check(project->SpriteBindings().empty(),"clearing typed slot failed");
        ex::Take(project->Save());
        project.reset();project=ex::Take(ed::ProjectFolder::Open(root,schema.View()));
        Check(project->SpriteBindings().empty(),"cleared typed slot reappeared");
#ifdef _WIN32
        constexpr unsigned char png[]={137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,1,0,0,0,1,8,6,0,0,0,31,21,196,137,0,0,0,13,73,68,65,84,120,156,99,248,207,192,208,0,0,4,129,1,128,44,85,206,176,0,0,0,0,73,69,78,68,174,66,96,130};
        {std::ofstream out(root/"Content/Sprites/sheet.png",std::ios::binary);
            out.write(reinterpret_cast<const char *>(png),sizeof(png));}
        const auto sheet_asset=ex::Take(ed::ImportProjectPng(*project,"Sprites/sheet.png"));
        ed::sheets::Grid grid{1,1,1,1,0,0,0,0};
        auto regions=ed::sheets::BuildGrid(sheet_asset.Raw(),grid);
        Check(static_cast<bool>(regions) && regions.value.size()==1,"sheet grid failed");
        ex::Take(project->SetSpriteSheet(sheet_asset,regions.value,1,1));
        ex::Take(project->SetPrefabSpriteRegion(prefab,sheet_asset,regions.value[0].id));
        ex::Take(project->Save());project.reset();project=ex::Take(ed::ProjectFolder::Open(root,schema.View()));
        Check(project->SpriteSheets().size()==1 && project->SpriteSheets()[0].regions[0].id==regions.value[0].id,
              "sheet metadata changed across Save/Open");
        Check(project->SpriteBindings().size()==1 && project->SpriteBindings()[0].region_id==regions.value[0].id,
              "subasset binding changed across Save/Open");
        ex::Take(project->SetSpriteSheet(sheet_asset,{},1,1));
        Check(project->SpriteSheets()[0].regions[0].missing,"deleted sprite silently rebound");
        ex::Take(project->Save());
#endif
        std::cout<<"P04 sprite binding and dependency/hot-reload tests PASS\n";
        return 0;
    }catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
    catch(...){std::fputs("Unexpected P04 test failure\n",stderr);return 1;}
}
