#include "Epidemic/Game/Editor/content_browser.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <set>

namespace epidemic::game::editor {
namespace {
std::string Lower(std::string_view value) {
    std::string out(value);
    std::transform(out.begin(),out.end(),out.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
    return out;
}
}
BrowserListing BrowseContent(const ProjectFolder &project,const BrowserFilter &filter) {
    BrowserListing result;
    const auto query=Lower(filter.search);
    const auto prefix=Lower(filter.path_prefix);
    std::set<std::string> folders;
    for(const auto &asset:project.Sources()) {
        // ProjectFolder only indexes project-relative Content paths.
        if(!asset.relative_path.starts_with("Content/"))continue;
        const auto path=asset.relative_path.substr(8);
        const auto folded=Lower(path);
        if(!prefix.empty() && !folded.starts_with(prefix))continue;
        if(!query.empty() && folded.find(query)==std::string::npos)continue;
        if(filter.type && asset.type!=*filter.type)continue;
        std::error_code error;
        auto resolved=project.ResolveMount("Content",path);
        const bool present=resolved && std::filesystem::is_regular_file(resolved.Value(),error) && !error;
        result.items.push_back({asset.id,asset.type,path,asset.generation,asset.source_hash,present});
        for(auto slash=path.find('/');slash!=std::string::npos;slash=path.find('/',slash+1))folders.insert(path.substr(0,slash));
    }
    std::sort(result.items.begin(),result.items.end(),[](const auto &a,const auto &b){
        if(a.path!=b.path)return a.path<b.path;
        return a.id.Raw()<b.id.Raw();
    });
    result.folders.assign(folders.begin(),folders.end());
    return result;
}
foundation::Result<void> RenameContentAsset(ProjectFolder &project,runtime::AssetId id,std::string_view path) {
    return project.MoveSource(id,path);
}
} // namespace epidemic::game::editor
