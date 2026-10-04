#include "Epidemic/Runtime/Support/runtime_support.h"
#include "Epidemic/Platform/iplatform_runtime.h"
#include <algorithm>
#include <bit>
#include <exception>
#include <stdexcept>
#include <tuple>
#include <type_traits>
namespace epidemic::runtime
{
namespace
{
constexpr std::size_t kPackageLimit=64*1024*1024;
constexpr std::size_t kRecordLimit=10000;
void Need(bool condition, const char* reason) { if (!condition) throw std::runtime_error(reason); }
foundation::Error PackageError(std::string_view phase, const std::exception& error)
{ return foundation::Error::Create("runtime.package.invalid","Runtime durable package rejected",std::string(phase)+": "+error.what()); }
std::uint64_t Checksum(std::span<const std::byte> bytes)
{
    std::uint64_t hash=14695981039346656037ull;
    for (auto b:bytes) { hash^=std::to_integer<unsigned char>(b); hash*=1099511628211ull; }
    return hash;
}
class RuntimePackageArchive
{
  public:
    explicit RuntimePackageArchive(std::vector<std::byte>& output):output_(&output) {}
    explicit RuntimePackageArchive(std::span<const std::byte> input):input_(input) {}
    template<class T> void Integer(T& value)
    {
        static_assert(std::is_integral_v<T> && !std::is_same_v<T,bool>);
        using U=std::make_unsigned_t<T>;
        U raw=std::bit_cast<U>(value);
        if (output_)
        {
            Need(output_->size()<=kPackageLimit-sizeof(T),"package size limit");
            for (std::size_t i=0;i<sizeof(T);++i) output_->push_back(std::byte((raw>>(8*i))&255));
        }
        else
        {
            Need(sizeof(T)<=input_.size()-position_,"truncated integer"); raw=0;
            for (std::size_t i=0;i<sizeof(T);++i) raw|=static_cast<U>(std::to_integer<unsigned char>(input_[position_++]))<<(8*i);
            value=std::bit_cast<T>(raw);
        }
    }
    template<class T> void Identity(T& id)
    {
        auto raw=id.Raw(); Integer(raw);
        if constexpr (std::is_constructible_v<T,std::uint64_t>) id=T{raw};
        else id=T{foundation::StringId{raw}};
    }
    template<class E> void Enum(E& value)
    { auto raw=static_cast<std::uint32_t>(value); Integer(raw); value=static_cast<E>(raw); }
    template<class T,class F> void Vector(std::vector<T>& values,F transfer,std::size_t maximum=kRecordLimit,bool nested_zone=false)
    {
        Need(!output_ || values.size()<=maximum,"record count limit");
        auto count=static_cast<std::uint32_t>(values.size()); Integer(count);
        Need(count<=maximum && (output_ || count<=input_.size()-position_),"invalid record count");
        if (nested_zone) { Need(count<=kRecordLimit-zone_records_,"aggregate zone count limit"); zone_records_+=count; }
        if (!output_) values.resize(count);
        for (auto& value:values) transfer(value);
    }
    void Payload(std::vector<std::byte>& value)
    {
        Need(!output_ || value.size()<=1024*1024,"payload size limit");
        auto size=static_cast<std::uint32_t>(value.size()); Integer(size);
        Need(size<=1024*1024,"payload size limit");
        if (output_)
        { Need(size<=kPackageLimit-output_->size(),"package size limit"); output_->insert(output_->end(),value.begin(),value.end()); }
        else
        { Need(size<=input_.size()-position_,"truncated payload"); auto bytes=input_.subspan(position_,size); value.assign(bytes.begin(),bytes.end()); position_+=size; }
    }
    void End() const { Need(output_ || position_==input_.size(),"trailing bytes"); }
  private:
    std::vector<std::byte>* output_=nullptr;
    std::span<const std::byte> input_{};
    std::size_t position_=0;
    std::size_t zone_records_=0;
};
void Transfer(RuntimePackageArchive& a,PersistenceLocation& v) { a.Identity(v.region_id); a.Identity(v.chunk_id); a.Identity(v.location_tag); }
void Transfer(RuntimePackageArchive& a,TombstoneRecord& v)
{ a.Identity(v.persistent_id); a.Integer(v.deleted_game_time.ticks); a.Identity(v.reason); a.Integer(v.revision); }
void Transfer(RuntimePackageArchive& a,LazyRuleRecord& v)
{ a.Identity(v.rule_id); a.Identity(v.target_id); a.Enum(v.kind); a.Enum(v.state); a.Integer(v.created_game_time.ticks); a.Integer(v.evaluate_after_game_time.ticks); a.Integer(v.rule_seed); a.Integer(v.revision); }
void Transfer(RuntimePackageArchive& a,PersistenceSnapshot& v)
{
    a.Integer(v.current_revision);
    a.Vector(v.objects,[&](auto& o){
        a.Identity(o.persistent_id); a.Identity(o.asset_id); a.Enum(o.kind); a.Enum(o.tier); a.Enum(o.state); Transfer(a,o.location);
        a.Identity(o.payload.schema_id); a.Integer(o.payload.schema_version); a.Payload(o.payload.bytes);
        a.Integer(o.created_game_time.ticks); a.Integer(o.last_observed_game_time.ticks); a.Integer(o.protection_flags.value); a.Integer(o.condition_hash); a.Integer(o.revision);
    });
    a.Vector(v.tombstones,[&](auto& o){Transfer(a,o);}); a.Vector(v.lazy_rules,[&](auto& o){Transfer(a,o);});
    a.Vector(v.zone_overrides,[&](auto& o){
        Transfer(a,o.location); a.Vector(o.record_ids,[&](auto& id){a.Identity(id);},kRecordLimit,true);
        a.Vector(o.tombstones,[&](auto& t){Transfer(a,t);},kRecordLimit,true); a.Vector(o.lazy_rules,[&](auto& r){Transfer(a,r);},kRecordLimit,true); a.Integer(o.revision);
    });
}
void Transfer(RuntimePackageArchive& a,TimeCheckpoint& v)
{
    a.Integer(v.now.ticks); a.Integer(v.time_scale.numerator); a.Integer(v.time_scale.denominator);
    auto paused=static_cast<std::uint8_t>(v.paused); a.Integer(paused); Need(paused<=1,"invalid paused flag"); v.paused=paused!=0;
    a.Integer(v.tick_remainder_numerator); a.Integer(v.revision); a.Integer(v.game_ticks_per_real_second);
    a.Integer(v.calendar.hours_per_day); a.Integer(v.calendar.days_per_month); a.Integer(v.calendar.months_per_year);
    a.Vector(v.phase_boundaries,[&](auto& p){a.Integer(p.start_minute); a.Enum(p.phase);},64);
}
void Check(const TimeCheckpoint& value)
{
    Need(value.phase_boundaries.size()<=64,"phase boundary limit");
    TimeOptions options; options.calendar=value.calendar; options.game_ticks_per_real_second=value.game_ticks_per_real_second;
    options.initial_time_scale=value.time_scale; options.phase_boundaries=value.phase_boundaries;
    auto clock=CreateTimeServices(options); Need(bool(clock),"invalid clock configuration");
    Need(bool(clock.Value().runtime->RestoreCheckpoint(value)),"invalid clock continuation");
}
auto LocationKey(const PersistenceLocation& v) { return std::tuple{v.region_id.Raw(),v.chunk_id.Raw(),v.location_tag.Raw()}; }
void Check(const PersistenceSnapshot& value)
{
    Need(value.objects.size()<=kRecordLimit && value.tombstones.size()<=kRecordLimit && value.lazy_rules.size()<=kRecordLimit && value.zone_overrides.size()<=kRecordLimit,"snapshot count limit");
    std::size_t bytes=0;
    for (const auto& o:value.objects) { Need(o.payload.bytes.size()<=1024*1024 && o.payload.bytes.size()<=kPackageLimit-bytes,"snapshot payload limit"); bytes+=o.payload.bytes.size(); }
    std::size_t nested=0;
    for (const auto& z:value.zone_overrides)
    {
        for (auto count:{z.record_ids.size(),z.tombstones.size(),z.lazy_rules.size()})
        { Need(count<=kRecordLimit && count<=kRecordLimit-nested,"aggregate zone count limit"); nested+=count; }
    }
    Need(bool(ValidatePersistenceSnapshot(value)),"invalid persistence snapshot");
}
void Canonical(TimeCheckpoint&) {} // Phase boundaries are configuration identity; preserve reviewed order.
void Canonical(PersistenceSnapshot& v)
{
    const auto object=[](auto& values){std::sort(values.begin(),values.end(),[](const auto& a,const auto& b){return a.persistent_id.Raw()<b.persistent_id.Raw();});};
    const auto rules=[](auto& values){std::sort(values.begin(),values.end(),[](const auto& a,const auto& b){return a.rule_id.Raw()<b.rule_id.Raw();});};
    object(v.objects); object(v.tombstones); rules(v.lazy_rules);
    std::sort(v.zone_overrides.begin(),v.zone_overrides.end(),[](const auto& a,const auto& b){return LocationKey(a.location)<LocationKey(b.location);});
    for (auto& z:v.zone_overrides) { object(z.tombstones); rules(z.lazy_rules); std::sort(z.record_ids.begin(),z.record_ids.end(),[](auto a,auto b){return a.Raw()<b.Raw();}); }
}
template<class T> std::vector<std::byte> Encode(const T& value,std::string_view magic)
{
    Check(value); auto copy=value; Canonical(copy); std::vector<std::byte> bytes;
    for (char c:magic) bytes.push_back(std::byte(static_cast<unsigned char>(c)));
    RuntimePackageArchive writer(bytes); std::uint32_t version=1; writer.Integer(version); Transfer(writer,copy);
    auto hash=Checksum(bytes); writer.Integer(hash); return bytes;
}
template<class T> T Decode(std::span<const std::byte> bytes,std::string_view magic)
{
    Need(bytes.size()>=20 && bytes.size()<=kPackageLimit,"truncated/oversized package");
    for (std::size_t i=0;i<8;++i) Need(bytes[i]==std::byte(static_cast<unsigned char>(magic[i])),"invalid package identity");
    RuntimePackageArchive tail(bytes.last(8)); std::uint64_t hash=0; tail.Integer(hash); Need(hash==Checksum(bytes.first(bytes.size()-8)),"package checksum mismatch");
    RuntimePackageArchive reader(bytes.subspan(8,bytes.size()-16)); std::uint32_t version=0; reader.Integer(version); Need(version==1,"unsupported package version");
    T value{}; Transfer(reader,value); reader.End(); Check(value);
    auto canonical=Encode(value,magic); Need(std::equal(canonical.begin(),canonical.end(),bytes.begin(),bytes.end()),"noncanonical package"); return value;
}
class FilePersistenceBackend final:public IPersistenceBackend
{
  public:
    explicit FilePersistenceBackend(foundation::Path path):path_(std::move(path)) {}
    foundation::Result<PersistenceSnapshot> Load() override
    {
        auto bytes=platform::ReadFilePackage(path_,kPackageLimit);
        if (!bytes) return foundation::Result<PersistenceSnapshot>::Failure(bytes.GetError());
        try { return foundation::Result<PersistenceSnapshot>::Success(Decode<PersistenceSnapshot>(bytes.Value(),"EPIRPER1")); }
        catch (const std::exception& error) { return foundation::Result<PersistenceSnapshot>::Failure(PackageError("persistence.load",error)); }
    }
    foundation::Result<void> CommitSnapshot(const PersistenceSnapshot& snapshot,PersistenceDurability durability) override
    {
        try
        {
            Need(durability==PersistenceDurability::MemoryOnly || durability==PersistenceDurability::SaveRequired || durability==PersistenceDurability::SaveAndFlushRequired,"invalid durability");
            // This file backend always meets the strongest level, including for weaker requests.
            auto bytes=Encode(snapshot,"EPIRPER1"); return platform::CommitFilePackage(path_,bytes,kPackageLimit);
        }
        catch (const std::exception& error) { return foundation::Result<void>::Failure(PackageError("persistence.commit",error)); }
    }
  private:
    foundation::Path path_;
};
}
foundation::Result<void> CommitRuntimeTimeCheckpoint(const foundation::Path& path,const TimeCheckpoint& checkpoint)
{
    try { auto bytes=Encode(checkpoint,"EPIRTIM1"); return platform::CommitFilePackage(path,bytes,kPackageLimit); }
    catch (const std::exception& error) { return foundation::Result<void>::Failure(PackageError("time.commit",error)); }
}
foundation::Result<TimeCheckpoint> LoadRuntimeTimeCheckpoint(const foundation::Path& path)
{
    auto bytes=platform::ReadFilePackage(path,kPackageLimit);
    if (!bytes) return foundation::Result<TimeCheckpoint>::Failure(bytes.GetError());
    try { return foundation::Result<TimeCheckpoint>::Success(Decode<TimeCheckpoint>(bytes.Value(),"EPIRTIM1")); }
    catch (const std::exception& error) { return foundation::Result<TimeCheckpoint>::Failure(PackageError("time.load",error)); }
}
foundation::Result<std::shared_ptr<IPersistenceBackend>> CreateFilePersistenceBackend(const foundation::Path& path)
{
    try
    {
        Need(!path.Empty(),"empty backend path");
        return foundation::Result<std::shared_ptr<IPersistenceBackend>>::Success(std::make_shared<FilePersistenceBackend>(path));
    }
    catch (const std::exception& error) { return foundation::Result<std::shared_ptr<IPersistenceBackend>>::Failure(PackageError("persistence.factory",error)); }
}
}
