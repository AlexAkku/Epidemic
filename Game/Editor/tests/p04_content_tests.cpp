#include "Epidemic/Game/Editor/content_browser.h"
#include "Epidemic/Game/Editor/png_import.h"
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
int main(){
 try{
    const auto root=fs::path{EPIDEMIC_EDITOR_TEST_DIRECTORY}/"p04-browser-import";
    std::error_code ec;fs::remove_all(root,ec);
    ex::Schema schema;
    auto project=ex::Take(ed::ProjectFolder::Create(root,"Browser Test",ex::Definition(schema),schema.View()));
    fs::create_directories(root/"Content"/"Sprites");
    {std::ofstream out(root/"Content"/"Sprites"/"bad.png",std::ios::binary);out<<"not a png";}
    Check(!ed::ImportProjectPng(*project,"Sprites/bad.png"),"Invalid PNG must fail without registering an asset");
    Check(project->Sources().empty(),"Invalid PNG changed project sources");
    const auto source=root/"Content"/"Sprites"/"notes.txt";
    {std::ofstream out(source);out<<"hello";}
    const auto type=epidemic::runtime::AssetType{ex::Id("editor.test.source")};
    const auto id=ex::Take(project->IndexSource("Sprites/notes.txt",type));
    auto view=ed::BrowseContent(*project,{"Sprites/","NOTES",type});
    Check(view.items.size()==1 && view.items[0].id==id && view.items[0].present,"Browser query lost source identity");
    Check(view.folders.size()==1 && view.folders[0]=="Sprites","Browser folders are missing");
    Check(!ed::RenameContentAsset(*project,id,"../escape.txt"),"Traversal was accepted");
    Check(!ed::RenameContentAsset(*project,id,"Sprites/bad.png"),"Overwrite was accepted");
    ex::Take(ed::RenameContentAsset(*project,id,"Sprites/moved.txt"));
    Check(!fs::exists(source) && fs::exists(root/"Content"/"Sprites"/"moved.txt"),"Move did not modify filesystem");
    view=ed::BrowseContent(*project,{"Sprites/","moved",type});
    Check(view.items.size()==1 && view.items[0].id==id,"Move changed AssetId");
    ex::Take(project->Save());
    project.reset();
    project=ex::Take(ed::ProjectFolder::Open(root,schema.View()));
    view=ed::BrowseContent(*project);
    Check(view.items.size()==1 && view.items[0].id==id && view.items[0].path=="Sprites/moved.txt","Move not persisted");
#ifdef _WIN32
    constexpr unsigned char png[]={137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,1,0,0,0,1,8,6,0,0,0,31,21,196,137,0,0,0,13,73,68,65,84,120,156,99,248,207,192,208,0,0,4,129,1,128,44,85,206,176,0,0,0,0,73,69,78,68,174,66,96,130};
    const auto image=root/"Content/Sprites/real.png";
    {std::ofstream out(image,std::ios::binary);out.write(reinterpret_cast<const char *>(png),sizeof(png));}
    const auto decoded=ex::Take(ed::DecodePng(image));
    Check(decoded.width==1 && decoded.height==1 && decoded.pixels.size()==4,"Real PNG dimensions");
    Check(decoded.pixels[0]==std::byte{0} && decoded.pixels[1]==std::byte{0} && decoded.pixels[2]==std::byte{128} && decoded.pixels[3]==std::byte{128},"PNG premultiplied BGRA alpha");
    const auto texture=ex::Take(ed::ImportProjectPng(*project,"Sprites/real.png"));
    ex::Take(project->Save());project.reset();project=ex::Take(ed::ProjectFolder::Open(root,schema.View()));
    view=ed::BrowseContent(*project,{"Sprites/","real.png",{}});
    Check(view.items.size()==1 && view.items[0].id==texture,"PNG identity survives reopen");
    {std::ofstream out(image,std::ios::binary);out.write(reinterpret_cast<const char *>(png),8);}
    Check(!ed::DecodePng(image),"Truncated PNG rejected");
#endif
    std::cout<<"P04 Content Browser and PNG tests PASS\n";
    return 0;
 }catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
 catch(...){std::fputs("Unexpected content test failure\n",stderr);return 1;}
}
