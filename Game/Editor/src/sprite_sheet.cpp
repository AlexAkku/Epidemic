#include "Epidemic/Game/Editor/sprite_sheet.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <limits>
#include <set>

namespace epidemic::game::editor::sheets {
namespace {
constexpr std::size_t kMaxRegions=16384;
constexpr std::size_t kMaxMetadata=std::size_t{4}*1024*1024;
std::string Error(std::string_view reason){return std::string(reason);}
bool Fits(std::uint32_t x,std::uint32_t extent,std::uint32_t limit){return extent!=0 && x<=limit && extent<=limit-x;}
bool ValidName(std::string_view name){
    if(name.empty() || name.size()>128)return false;
    for(unsigned char c:name)if(c<=32 || c>126 || c=='|' || c=='\\')return false;
    return true;
}
bool Valid(const Region &r){return r.x<=4096 && r.y<=4096 && r.width<=4096 && r.height<=4096 && r.id!=0 && ValidName(r.name) && std::isfinite(r.pivot_x) && std::isfinite(r.pivot_y) && r.pivot_x>=0 && r.pivot_x<=1 && r.pivot_y>=0 && r.pivot_y<=1;}
}
std::uint64_t StableId(std::uint64_t parent,std::string_view key) noexcept {
    if(parent==0 || key.empty())return 0;
    std::uint64_t h=14695981039346656037ull;
    for(unsigned i=0;i<8;++i){h^=static_cast<unsigned char>(parent>>(8*i));h*=1099511628211ull;}
    for(unsigned char c:key){h^=c;h*=1099511628211ull;}
    return h?h:1;
}
Result<std::vector<Region>> Validate(std::uint32_t width,std::uint32_t height,std::span<const Region> regions){
    if(width==0 || height==0 || width>4096 || height>4096 || regions.size()>kMaxRegions)return {{},Error("sheet.bounds")};
    std::set<std::uint64_t> ids;std::set<std::string> names;
    for(const auto &r:regions){
        if(!Valid(r) || !ids.insert(r.id).second || !names.insert(r.name).second ||
           (!r.missing && (!Fits(r.x,r.width,width)||!Fits(r.y,r.height,height))))return {{},Error("sheet.region")};
    }
    return {{regions.begin(),regions.end()}, {}};
}
Result<std::vector<Region>> BuildGrid(std::uint64_t parent,const Grid &g,std::string_view prefix){
    if(parent==0 || g.cell_width==0 || g.cell_height==0 || !ValidName(prefix) ||
       g.image_width==0 || g.image_height==0 || g.image_width>4096 || g.image_height>4096 ||
       g.origin_x>=g.image_width || g.origin_y>=g.image_height)return {{},Error("sheet.grid")};
    std::uint64_t stride_x=static_cast<std::uint64_t>(g.cell_width)+g.padding_x;
    std::uint64_t stride_y=static_cast<std::uint64_t>(g.cell_height)+g.padding_y;
    std::vector<Region> out;out.reserve(256);
    for(std::uint64_t y=g.origin_y; y+g.cell_height<=g.image_height; y+=stride_y){
        for(std::uint64_t x=g.origin_x; x+g.cell_width<=g.image_width; x+=stride_x){
            if(out.size()>=kMaxRegions)return {{},Error("sheet.too_many_regions")};
            std::string name=std::string(prefix)+"_"+std::to_string(out.size());
            if(!ValidName(name))return {{},Error("sheet.region_name")};
            out.push_back({StableId(parent,name),std::move(name),static_cast<std::uint32_t>(x),
                 static_cast<std::uint32_t>(y),g.cell_width,g.cell_height,.5f,.5f,false});
        }
    }
    if(out.empty())return {{},Error("sheet.empty")};
    return Validate(g.image_width,g.image_height,out);
}
Result<std::vector<Region>> Reconcile(std::span<const Region> old,std::span<const Region> edited){
    if(old.size()>kMaxRegions || edited.size()>kMaxRegions)return {{},Error("sheet.too_many_regions")};
    std::set<std::uint64_t> ids;std::set<std::string> names;
    std::vector<Region> result;result.reserve(old.size()+edited.size());
    std::set<std::uint64_t> old_ids;std::set<std::string> old_names;
    for(const auto &region:old)if(!Valid(region) || !old_ids.insert(region.id).second || !old_names.insert(region.name).second)
        return {{},Error("sheet.existing_region")};
    for(auto r:edited){
        const auto by_id=std::find_if(old.begin(),old.end(),[&](const Region &o){return o.id==r.id;});
        const auto by_name=std::find_if(old.begin(),old.end(),[&](const Region &o){return o.name==r.name;});
        if(by_id!=old.end()) {
            // An explicit existing identity supports rename; a retired identity
            // may only be carried forward as the same named tombstone.
            if(by_id->missing && (!r.missing || r.name!=by_id->name))return {{},Error("sheet.retired_id")};
            if(by_name!=old.end() && by_name->id!=r.id && by_name->missing)return {{},Error("sheet.retired_name")};
        }else if(by_name!=old.end()) {
            if(by_name->missing)return {{},Error("sheet.retired_name")};
            r.id=by_name->id;
        }
        if(!Valid(r)||!ids.insert(r.id).second||!names.insert(r.name).second)return {{},Error("sheet.region")};
        result.push_back(std::move(r));
    }
    for(auto r:old){
        if(!ids.contains(r.id)){
            r.missing=true;
            if(result.size()>=kMaxRegions || !ids.insert(r.id).second || !names.insert(r.name).second)return {{},Error("sheet.tombstone_limit")};
            result.push_back(std::move(r));
        }
    }
    std::sort(result.begin(),result.end(),[](const Region &a,const Region &b){return a.id<b.id;});
    return {std::move(result),{}};
}
Result<AtlasLayout> Pack(std::span<const Region> regions,std::uint32_t max_w,std::uint32_t max_h,std::uint32_t padding){
    if(max_w==0 || max_h==0 || max_w>4096 || max_h>4096 || padding>64 || regions.size()>kMaxRegions)
        return {{},Error("sheet.atlas_budget")};
    std::vector<Region> sorted;
    for(const auto &r:regions)if(!r.missing){if(!Valid(r)||!r.width||!r.height)return {{},Error("sheet.atlas_region")};sorted.push_back(r);}
    std::sort(sorted.begin(),sorted.end(),[](const Region &a,const Region &b){return a.id<b.id;});
    for(std::size_t i=1;i<sorted.size();++i)if(sorted[i-1].id==sorted[i].id)return {{},Error("sheet.atlas_identity")};
    AtlasLayout atlas;
    std::uint64_t x=0,y=0,row=0;
    for(const auto &r:sorted){
        if(static_cast<std::uint64_t>(r.width)+static_cast<std::uint64_t>(padding)*2>max_w || static_cast<std::uint64_t>(r.height)+static_cast<std::uint64_t>(padding)*2>max_h)
            return {{},Error("sheet.atlas_oversize")};
        if(x+r.width+static_cast<std::uint64_t>(padding)*2>max_w){x=0;y+=row;row=0;}
        if(y+r.height+static_cast<std::uint64_t>(padding)*2>max_h)return {{},Error("sheet.atlas_full")};
        atlas.regions.push_back({r.id,{static_cast<std::uint32_t>(x+padding),static_cast<std::uint32_t>(y+padding),r.width,r.height}});
        x+=r.width+static_cast<std::uint64_t>(padding)*2;
        row=std::max(row,static_cast<std::uint64_t>(r.height)+static_cast<std::uint64_t>(padding)*2);
        atlas.width=std::max(atlas.width,static_cast<std::uint32_t>(x));
        atlas.height=std::max(atlas.height,static_cast<std::uint32_t>(y+row));
    }
    return {std::move(atlas),{}};
}
Result<std::vector<std::byte>> ExtractPbgra(std::span<const std::byte> data,std::uint32_t w,std::uint32_t h,const Region &r){
    if(w==0||h==0||w>4096||h>4096||static_cast<std::uint64_t>(w)*h*4!=data.size()||r.missing||
       !Fits(r.x,r.width,w)||!Fits(r.y,r.height,h))return {{},Error("sheet.extract_bounds")};
    std::vector<std::byte> pixels(static_cast<std::size_t>(r.width)*r.height*4);
    for(std::uint32_t y=0;y<r.height;++y){
       const auto src=(static_cast<std::size_t>(r.y+y)*w+r.x)*4;
       const auto dst=static_cast<std::size_t>(y)*r.width*4;
       std::copy_n(data.begin()+static_cast<std::ptrdiff_t>(src),static_cast<std::ptrdiff_t>(r.width)*4,
                   pixels.begin()+static_cast<std::ptrdiff_t>(dst));
    }
    return {std::move(pixels),{}};
}
Result<std::vector<std::byte>> ComposeAtlasPbgra(
    std::span<const std::byte> source,std::uint32_t source_width,std::uint32_t source_height,
    std::span<const Region> regions,const AtlasLayout &layout){
    if(layout.width==0||layout.height==0||layout.width>4096||layout.height>4096 ||
       static_cast<std::uint64_t>(layout.width)*layout.height*4>=16ULL*1024*1024)
        return {{},Error("sheet.atlas_cooked_limit")};
    std::vector<std::byte> atlas(static_cast<std::size_t>(layout.width)*layout.height*4);
    std::set<std::uint64_t> seen;
    for(const auto &entry:layout.regions){
        if(!seen.insert(entry.id).second)return {{},Error("sheet.atlas_duplicate")};
        const auto region=std::find_if(regions.begin(),regions.end(),[&](const Region &r){return r.id==entry.id;});
        if(region==regions.end()||region->missing || region->width!=entry.rect.width ||
           region->height!=entry.rect.height || !Fits(entry.rect.x,entry.rect.width,layout.width) ||
           !Fits(entry.rect.y,entry.rect.height,layout.height))return {{},Error("sheet.atlas_region")};
        auto cropped=ExtractPbgra(source,source_width,source_height,*region);
        if(!cropped)return {{},cropped.error};
        for(std::uint32_t y=0;y<region->height;++y){
            const auto dst=(static_cast<std::size_t>(entry.rect.y+y)*layout.width+entry.rect.x)*4;
            const auto from=static_cast<std::size_t>(y)*region->width*4;
            std::copy_n(cropped.value.begin()+static_cast<std::ptrdiff_t>(from),
                static_cast<std::ptrdiff_t>(region->width)*4,atlas.begin()+static_cast<std::ptrdiff_t>(dst));
        }
    }
    return {std::move(atlas),{}};
}
Result<std::string> Encode(std::uint64_t parent,std::span<const Region> regions){
    if(!parent || regions.size()>kMaxRegions)return {{},Error("sheet.encode")};
    std::vector<Region> sorted(regions.begin(),regions.end());
    std::sort(sorted.begin(),sorted.end(),[](const Region &a,const Region &b){return a.id<b.id;});
    std::set<std::uint64_t> ids;std::set<std::string> names;
    std::string text="EPIDEMIC_SHEET_1|"+std::to_string(parent)+"\n";
    for(const auto &r:sorted){
        if(!Valid(r)||!ids.insert(r.id).second||!names.insert(r.name).second)return {{},Error("sheet.encode_region")};
        text+=std::to_string(r.id)+"|"+r.name+"|"+std::to_string(r.x)+"|"+std::to_string(r.y)+"|"+
              std::to_string(r.width)+"|"+std::to_string(r.height)+"|"+std::to_string(std::lround(r.pivot_x*1000000.f))+"|"+
              std::to_string(std::lround(r.pivot_y*1000000.f))+"|"+(r.missing?"1":"0")+"\n";
        if(text.size()>kMaxMetadata)return {{},Error("sheet.metadata_limit")};
    }
    return {std::move(text),{}};
}
Result<std::vector<Region>> Decode(std::uint64_t parent,std::string_view text){
    if(!parent||text.size()>kMaxMetadata)return {{},Error("sheet.decode")};
    const auto newline=text.find('\n');
    if(newline==text.npos || text.substr(0,newline)!=("EPIDEMIC_SHEET_1|"+std::to_string(parent)))return {{},Error("sheet.schema")};
    std::vector<Region> out;std::set<std::uint64_t> ids;std::set<std::string> names;
    for(std::size_t at=newline+1;at<text.size();){
        const auto end=text.find('\n',at);if(end==text.npos)return {{},Error("sheet.truncated")};
        const auto line=text.substr(at,end-at);at=end+1;
        if(line.empty())return {{},Error("sheet.empty_record")};
        std::string_view fields[9];std::size_t begin=0;
        for(std::size_t i=0;i<9;++i){const auto sep=line.find('|',begin);
            if(i==8 ? sep!=line.npos:sep==line.npos)return {{},Error("sheet.fields")};
            fields[i]=line.substr(begin,(sep==line.npos?line.size():sep)-begin);
            begin=(sep==line.npos?line.size():sep+1);
        }
        Region r;auto number=[&](std::size_t i,std::uint64_t &v){
            const auto b=fields[i].data();const auto e=b+fields[i].size();
            auto parsed=std::from_chars(b,e,v);return parsed.ec==std::errc{}&&parsed.ptr==e;};
        std::uint64_t values[8]{};
        for(std::size_t i=0;i<8;++i)if(i!=1 && !number(i,values[i]))return {{},Error("sheet.integer")};
        r.id=values[0];r.name=std::string(fields[1]);
        if(values[2]>4096||values[3]>4096||values[4]>4096||values[5]>4096||values[6]>1000000||values[7]>1000000)
            return {{},Error("sheet.bounds")};
        r.x=static_cast<std::uint32_t>(values[2]);r.y=static_cast<std::uint32_t>(values[3]);
        r.width=static_cast<std::uint32_t>(values[4]);r.height=static_cast<std::uint32_t>(values[5]);
        r.pivot_x=static_cast<float>(values[6])/1000000.f;r.pivot_y=static_cast<float>(values[7])/1000000.f;
        if(fields[8]!="0"&&fields[8]!="1")return {{},Error("sheet.tombstone")};r.missing=fields[8]=="1";
        if(!Valid(r)||!ids.insert(r.id).second||!names.insert(r.name).second||out.size()>=kMaxRegions)
            return {{},Error("sheet.region")};
        out.push_back(std::move(r));
    }
    return {std::move(out),{}};
}
} // namespace epidemic::game::editor::sheets
