#include "Epidemic/Game/Editor/project_folder.h"
#include "../example/demo_game.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace ed=epidemic::game::editor;
namespace ex=ed::example;
namespace rt=epidemic::runtime;
namespace fs=std::filesystem;
namespace {
void Check(bool test,const char *why){if(!test)throw std::runtime_error(why);}
fs::path Base(){return fs::path{EPIDEMIC_EDITOR_TEST_DIRECTORY}/"p04-folder";}
void Create() {
    std::error_code ec;fs::remove_all(Base(),ec);Check(!ec,"Cannot reset dedicated Temp test directory");
    ex::Schema schema;
    auto folder=ex::Take(ed::ProjectFolder::Create(Base()/"First", "Portable example", ex::Definition(schema),schema.View()));
    Check(folder->Generation()==1 && !folder->IsDirty(),"New project has a committed snapshot");
    const auto prefab=rt::AssetId::FromString("example.prefab");
    ex::Take(folder->Execute({folder->Revision(),{ed::RenameDocument{prefab,"Portable Enemy"}}}));
    Check(folder->IsDirty(),"Editor command dirties folder");
    ex::Take(folder->Save());
    Check(folder->Generation()==2 && !folder->IsDirty(),"Save commits authoring revision");
    {
        std::ofstream file(Base()/"First"/"Content"/"image.png",std::ios::binary);
        file<<"MOCK SOURCE; NOT VALID PNG";
    }
    const auto id=ex::Take(folder->IndexSource("image.png",{ex::Id("editor.source.texture")}));
    Check(id.IsValid() && folder->IsDirty(),"Source identity independent of filename");
    ex::Take(folder->Save());
    Check(folder->Generation()==3 && folder->Sources().size()==1,"Source index committed");
    const auto catalog=ex::Take(folder->BuildSourceCatalog());
    Check(catalog->IsSealed() && catalog->Contains(id),"Fresh sealed Assets catalog");
    Check(ex::Take(folder->ResolveMount("Content","image.png"))==fs::canonical(Base()/"First")/"Content/image.png", "Resolved content mount");
    Check(!folder->ResolveMount("Content","../escape"),"Traversal rejected");
    Check(!ed::ProjectFolder::Open(Base()/"First",schema.View()),"Exclusive process writer lock");
    ex::Take(folder->CloneTo(Base()/"Second"));
    // An imported legacy archive retains retired identity reservations. The
    // source .epidemic must not be overwritten or converted in place.
    auto legacy=ex::Take(ed::EditorProject::Create(Base()/"old.epidemic",ex::Definition(schema),schema.View()));
    const auto graph=rt::AssetId::FromString("example.graph");
    const auto retired=rt::graph::NodeDefinition{ex::Id("p04.retired.node"),ex::Id("native.type"),"Retired",{}};
    ex::Take(legacy->Execute({legacy->Revision(),{ed::AddGraphNode{graph,retired}}}));
    ex::Take(legacy->Undo());
    ex::Take(legacy->Save());
    const auto original_size=fs::file_size(Base()/"old.epidemic");
    auto imported=ex::Take(ed::ProjectFolder::ImportLegacy(Base()/"Imported",Base()/"old.epidemic","Legacy imported",schema.View()));
    Check(imported->Generation()==1 && fs::file_size(Base()/"old.epidemic")==original_size,"Legacy import is copy-only");
    Check(!imported->Execute({imported->Revision(),{ed::AddGraphNode{graph,retired}}}),"Legacy retired ID must survive folder migration");
    std::cout<<"P04 folder create/commit/index/clone/legacy PASS\n";
}
void Read() {
    ex::Schema schema;
    for(const auto *name:{"First","Second"}) {
        auto folder=ex::Take(ed::ProjectFolder::Open(Base()/name,schema.View()));
        Check(folder->Generation()==3 && !folder->IsDirty(),"Cross-process committed generation");
        Check(folder->Snapshot()->Find(rt::AssetId::FromString("example.prefab"))->name=="Portable Enemy", "Authoring preserved");
        Check(folder->Sources().size()==1 && folder->Sources()[0].generation==1,"Stable source index");
        Check(folder->Sources()[0].id==rt::AssetId{ex::Id("epidemic.project.asset/"+std::to_string(ex::Id("example.project").Raw())+"/1")},
              "Source identity does not depend on folder path");
        auto session=ex::Take(folder->Play(ex::Options(schema)));
        ex::Take(session->Stop()); // Check schema can still start after relocation.
    }
    {
        auto imported=ex::Take(ed::ProjectFolder::Open(Base()/"Imported",schema.View()));
        Check(imported->Generation()==1 && !imported->IsDirty(),"Imported legacy folder opens in another process");
    }
    std::cout<<"P04 folder cross-process relocation PASS\n";
}
void Recover() {
    ex::Schema schema;
    const auto root=Base()/"First";
    // Unpublished staged files are not visible: the authoritative commit marker
    // is created last, not guessed from the highest folder name.
    fs::create_directories(root/"ProjectData/Revisions/0000000000000099");
    {std::ofstream ghost(root/"ProjectData/Revisions/0000000000000099/Authoring.epidemic");ghost<<"orphan";}
    {
        auto folder=ex::Take(ed::ProjectFolder::Open(root,schema.View()));
        Check(folder->Generation()==3,"Orphan does not become committed revision");
    }
    // Deliberately corrupt one committed payload; previous generation must be
    // recovered rather than using inconsistent files from different generations.
    {std::ofstream damaged(root/"ProjectData/Revisions/0000000000000003/Authoring.epidemic",std::ios::binary|std::ios::trunc);damaged<<"damaged";}
    auto recovered=ex::Take(ed::ProjectFolder::Open(root,schema.View()));
    Check(recovered->Generation()==2,"Last-good commit recovered");
    Check(recovered->Sources().empty(),"Never mix source manifest from newer revision");
    Check(recovered->Snapshot()->Find(rt::AssetId::FromString("example.prefab"))->name=="Portable Enemy", "Last-good authoring unchanged");
    std::cout<<"P04 folder orphan/corruption fallback PASS\n";
}
}
int main(int argc,char **argv) {
    try {
        if(argc!=2) throw std::runtime_error("Expected --create, --read, or --recover");
        const std::string_view mode=argv[1];
        if(mode=="--create")Create();
        else if(mode=="--read")Read();
        else if(mode=="--recover")Recover();
        else throw std::runtime_error("Unknown test mode");
        return 0;
    } catch(const std::exception &e) {std::cerr<<e.what()<<'\n';return 1;}
}
