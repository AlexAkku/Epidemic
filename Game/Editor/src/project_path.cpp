#include "Epidemic/Game/Editor/project_path.h"
#include <algorithm>
#include <cctype>
#include <string>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace epidemic::game::editor {
foundation::Result<std::filesystem::path> ResolveProjectPath(const std::filesystem::path &root,std::string_view relative) {
    using Result=foundation::Result<std::filesystem::path>;
    const auto reject=[] { return Result::Failure(foundation::Error::Create("editor.project_path","unsafe project-relative path")); };
    try {
    if(relative.empty() || relative.size()>4096 || relative.front()=='/' || relative.find_first_of("\\:\0",0,3)!=std::string_view::npos) return reject();
    std::error_code error;
    const auto base=std::filesystem::canonical(root,error);
    if(error || !std::filesystem::is_directory(base,error) || error)return reject();
    auto resolved=base;
    for(std::size_t begin=0;begin<relative.size();) {
        const auto end=relative.find('/',begin); const auto part=relative.substr(begin,end==std::string_view::npos?relative.size()-begin:end-begin);
        if(part.empty() || part=="." || part==".." || part.back()=='.' || part.back()==' ')return reject();
        if(std::any_of(part.begin(),part.end(),[](unsigned char c){return c<32 || c=='<' || c=='>' || c=='"' || c=='|' || c=='?' || c=='*';}))return reject();
        auto stem=std::string(part.substr(0,part.find('.')));
        std::transform(stem.begin(),stem.end(),stem.begin(),[](unsigned char c){return static_cast<char>(std::toupper(c));});
        if(stem=="CON" || stem=="PRN" || stem=="AUX" || stem=="NUL" ||
           (stem.size()==4 && (stem.starts_with("COM") || stem.starts_with("LPT")) && stem[3]>='1' && stem[3]<='9'))return reject();
        std::u8string encoded; encoded.reserve(part.size());
        for(unsigned char c:part) encoded.push_back(static_cast<char8_t>(c));
        resolved/=std::filesystem::path{encoded};
#ifdef _WIN32
        // symlink_status alone may report a junction as a directory. Reject
        // every reparse component, not just symbolic-link tags.
        const auto attributes=GetFileAttributesW(resolved.c_str());
        if(attributes!=INVALID_FILE_ATTRIBUTES && (attributes & static_cast<DWORD>(FILE_ATTRIBUTE_REPARSE_POINT))!=0)return reject();
        if(attributes==INVALID_FILE_ATTRIBUTES && GetLastError()!=ERROR_FILE_NOT_FOUND && GetLastError()!=ERROR_PATH_NOT_FOUND)return reject();
#endif
        const auto status=std::filesystem::symlink_status(resolved,error);
        if(error && error!=std::errc::no_such_file_or_directory)return reject();
        error.clear();
        if(std::filesystem::is_symlink(status))return reject();
        if(end==std::string_view::npos)break;
        if(std::filesystem::exists(status) && !std::filesystem::is_directory(status))return reject();
        begin=end+1; if(begin==relative.size())return reject();
    }
    return Result::Success(std::move(resolved));
    } catch(const std::filesystem::filesystem_error &) { return reject(); }
}
}
