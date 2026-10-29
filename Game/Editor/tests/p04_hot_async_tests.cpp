#include "Epidemic/Game/Editor/content_hot_reload.h"
#include "Epidemic/Game/Editor/png_import.h"
#include "../example/demo_game.h"
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstdio>
#include <stdexcept>
#include <thread>

namespace ed=epidemic::game::editor;
namespace ex=ed::example;
namespace fs=std::filesystem;
void Check(bool yes,const char *text){if(!yes)throw std::runtime_error(text);}
int main(){
#ifdef _WIN32
    try {
        const auto root=fs::path{EPIDEMIC_EDITOR_TEST_DIRECTORY}/"p04-async-image";
        std::error_code ec;fs::remove_all(root,ec);
        ex::Schema schema;
        auto folder=ex::Take(ed::ProjectFolder::Create(root,"Async PNG",ex::Definition(schema),schema.View()));
        fs::create_directories(root/"Content");
        // Known 1x1 RGBA PNG fixture, identical to existing P04 content test.
        constexpr unsigned char red[]={137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,1,0,0,0,1,8,6,0,0,0,31,21,196,137,0,0,0,13,73,68,65,84,120,156,99,248,207,192,208,0,0,4,129,1,128,44,85,206,176,0,0,0,0,73,69,78,68,174,66,96,130};
        constexpr unsigned char blue[]={137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,1,0,0,0,1,8,6,0,0,0,31,21,196,137,0,0,0,13,73,68,65,84,120,156,99,96,96,248,223,0,0,2,131,1,128,67,187,248,3,0,0,0,0,73,69,78,68,174,66,96,130};
        const auto path=root/"Content"/"sprite.png";
        const auto write=[&](auto const& png){std::ofstream out(path,std::ios::binary);out.write(reinterpret_cast<const char*>(png),sizeof(png));Check(out.good(),"Cannot write PNG");};
        write(red);
        const auto id=ex::Take(ed::ImportProjectPng(*folder,"sprite.png"));
        ex::Take(folder->Save());
        const auto before=folder->Sources().front();
        ed::ContentHotReload watch;watch.Reset(*folder);
        write(blue);
        fs::last_write_time(path,fs::file_time_type::clock::now()+std::chrono::seconds{8},ec);
        Check(!ec,"Cannot force updated file mtime");
        bool queued=false,updated=false;
        for(int i=0;i<600&&!updated;++i){
            const auto events=watch.Poll(*folder);
            for(const auto &e:events){
                if(e.id==id && e.kind==ed::HotAssetEventKind::Queued)queued=true;
                if(e.id==id && e.kind==ed::HotAssetEventKind::Updated)updated=true;
                Check(e.kind!=ed::HotAssetEventKind::Failed,"Unexpected decode failure");
            }
            std::this_thread::sleep_for(std::chrono::milliseconds{5});
        }
        Check(queued && updated,"Asynchronous import never completed");
        const auto after=folder->Sources().front();
        Check(after.id==id && after.generation==before.generation+1 && after.cooked_key!=before.cooked_key,
              "Source identity/generation publication broken");
        // Cancellation with an external damage must not advance last-good.
        watch.SetEnabled(false);
        {std::ofstream out(path,std::ios::binary);out<<"broken";}
        Check(folder->Sources().front().cooked_key==after.cooked_key,"Cancel replaced last-good");
        ex::Take(folder->Save());folder.reset();
        folder=ex::Take(ed::ProjectFolder::Open(root,schema.View()));
        Check(folder->Sources().front().cooked_key==after.cooked_key,"Reopen lost cooked key");
        std::cout<<"P04 async PNG watcher PASS\n";
        return 0;
    }catch(const std::exception &e){std::fprintf(stderr,"%s\n",e.what());return 1;}
    catch(...){std::fputs("Unexpected P04 test failure\n",stderr);return 1;}
#else
    return 0; // Windows WIC qualification is registered only on Windows.
#endif
}
