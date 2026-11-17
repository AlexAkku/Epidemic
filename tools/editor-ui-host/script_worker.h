#pragma once
#include "Epidemic/Runtime/Graph/graph_compiler.h"
#include <Windows.h>
#include <filesystem>
#include <chrono>
#include <thread>
#include <string>
#include <stdexcept>
#include <bit>
#include <vector>
#include <charconv>
#include <cmath>
#include "Epidemic/GameFramework/Integration/components.h"

namespace epidemic::game::editor::scripts {
// Owned handles only. No shell, inherited handles restricted to the two pipes.
class Worker final : public runtime::graph::INativeInvocation {
    HANDLE process_=nullptr, input_=nullptr, output_=nullptr, job_=nullptr;
    std::uint64_t generation_=0, request_=0;
    std::string operation_;
    std::chrono::steady_clock::time_point deadline_;
    static std::wstring Wide(const std::string &value){
        const auto count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,value.data(),static_cast<int>(value.size()),nullptr,0);
        if(count<=0)throw std::runtime_error("script.utf8");std::wstring result(static_cast<std::size_t>(count),L' ');
        MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,value.data(),static_cast<int>(value.size()),result.data(),count);return result;
    }
    static std::wstring Quote(const std::wstring &text) {
        std::wstring result=L"\"";std::size_t slashes=0;
        for(auto ch:text){if(ch==L'\\'){++slashes;continue;}if(ch==L'\"')result.append(slashes*2+1,L'\\');else result.append(slashes,L'\\');slashes=0;result+=ch;}
        result.append(slashes*2,L'\\');return result+L"\"";
    }
    template<unsigned Width=8> void Write(std::uint64_t value) {
        constexpr unsigned width=Width;
        unsigned char bytes[8];for(unsigned i=0;i<8;++i)bytes[7-i]=static_cast<unsigned char>(value>>(i*8));
        DWORD count=0;if(!WriteFile(input_,bytes+8-width,width,&count,nullptr)||count!=width)throw std::runtime_error("script.worker_write");
    }
    std::uint64_t Read(unsigned width=8) {
        std::uint64_t result=0;
        for(unsigned i=0;i<width;++i){
            DWORD ready=0;
            while(true){if(!PeekNamedPipe(output_,nullptr,0,nullptr,&ready,nullptr))throw std::runtime_error("script.worker_closed");if(ready)break;
                if(std::chrono::steady_clock::now()>deadline_)throw std::runtime_error("script.worker_timeout");std::this_thread::sleep_for(std::chrono::milliseconds(1));}
            unsigned char byte=0;DWORD count=0;if(!ReadFile(output_,&byte,1,&count,nullptr)||count!=1)throw std::runtime_error("script.worker_closed");result=(result<<8u)|static_cast<std::uint64_t>(byte);
        }return result;
    }
    void Text(const std::string &value){Write<4>(value.size());DWORD count=0;if(!value.empty()&&(!WriteFile(input_,value.data(),static_cast<DWORD>(value.size()),&count,nullptr)||count!=value.size()))throw std::runtime_error("script.worker_write");}
    std::string Text(){const auto length=Read(4);if(length>16384)throw std::runtime_error("script.string_limit");std::string result;result.reserve(static_cast<std::size_t>(length));for(std::uint64_t i=0;i<length;++i)result+=static_cast<char>(Read(1));return result;}
  public:
    using Fields=std::vector<std::pair<std::string,gameplay::composition::PropertyValue>>;
    Worker(const std::filesystem::path &worker,const std::filesystem::path &assembly,const std::string &hash,const std::string &operation,std::uint64_t generation):generation_(generation),operation_(operation){
        deadline_=std::chrono::steady_clock::now()+std::chrono::seconds(3);
        HANDLE child_input=nullptr,child_output=nullptr,child_error=nullptr;
        try {
            SECURITY_ATTRIBUTES security{sizeof(SECURITY_ATTRIBUTES),nullptr,TRUE};
            if(!CreatePipe(&child_input,&input_,&security,0)||!CreatePipe(&output_,&child_output,&security,0))throw std::runtime_error("script.pipe");
            child_error=CreateFileW(L"NUL",GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,&security,OPEN_EXISTING,0,nullptr);
            if(child_error==INVALID_HANDLE_VALUE){child_error=nullptr;throw std::runtime_error("script.stderr");}
            SetHandleInformation(input_,HANDLE_FLAG_INHERIT,0);SetHandleInformation(output_,HANDLE_FLAG_INHERIT,0);
            wchar_t program_files[32768];if(!GetEnvironmentVariableW(L"ProgramFiles",program_files,32768))throw std::runtime_error("script.dotnet_missing");
            const auto dotnet=std::filesystem::path(program_files)/L"dotnet/dotnet.exe";
            if(!std::filesystem::is_regular_file(dotnet)||!std::filesystem::is_regular_file(worker)||!std::filesystem::is_regular_file(assembly))throw std::runtime_error("script.managed_unavailable");
            std::wstring command=Quote(dotnet.wstring())+L" "+Quote(worker.wstring())+L" invoke "+Quote(assembly.wstring())+L" "+Quote(Wide(hash))+L" "+Quote(Wide(operation))+L" "+std::to_wstring(generation);
            SIZE_T size=0;InitializeProcThreadAttributeList(nullptr,1,0,&size);std::vector<std::byte> storage(size);
            auto *attributes=reinterpret_cast<PPROC_THREAD_ATTRIBUTE_LIST>(storage.data());
            if(!InitializeProcThreadAttributeList(attributes,1,0,&size))throw std::runtime_error("script.process_attributes");
            HANDLE handles[]{child_input,child_output,child_error};
            STARTUPINFOEXW startup{};startup.StartupInfo.cb=sizeof(startup);startup.StartupInfo.dwFlags=STARTF_USESTDHANDLES;
            startup.StartupInfo.hStdInput=child_input;startup.StartupInfo.hStdOutput=child_output;startup.StartupInfo.hStdError=child_error;startup.lpAttributeList=attributes;
            PROCESS_INFORMATION process{};
            const bool configured=UpdateProcThreadAttribute(attributes,0,PROC_THREAD_ATTRIBUTE_HANDLE_LIST,static_cast<void *>(handles),sizeof(handles),nullptr,nullptr)!=FALSE;
            const bool created=configured&&CreateProcessW(dotnet.c_str(),command.data(),nullptr,nullptr,TRUE,CREATE_SUSPENDED|CREATE_NO_WINDOW|EXTENDED_STARTUPINFO_PRESENT,nullptr,assembly.parent_path().c_str(),&startup.StartupInfo,&process);
            DeleteProcThreadAttributeList(attributes);
            if(!created)throw std::runtime_error("script.process_start");process_=process.hProcess;
            job_=CreateJobObjectW(nullptr,nullptr);JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE|JOB_OBJECT_LIMIT_PROCESS_MEMORY;limits.ProcessMemoryLimit=512ULL*1024*1024;
            if(!job_||!SetInformationJobObject(job_,JobObjectExtendedLimitInformation,&limits,sizeof(limits))||!AssignProcessToJobObject(job_,process_)){
                TerminateProcess(process_,1);CloseHandle(process.hThread);throw std::runtime_error("script.process_job");}
            ResumeThread(process.hThread);CloseHandle(process.hThread);CloseHandle(child_error);child_error=nullptr;
            CloseHandle(child_input);child_input=nullptr;CloseHandle(child_output);child_output=nullptr;
            if(Read(4)!=0x45505343||Read(2)!=3||Read()!=generation_)throw std::runtime_error("script.worker_handshake");
        }catch(...){if(child_input)CloseHandle(child_input);if(child_output)CloseHandle(child_output);if(child_error)CloseHandle(child_error);Stop();throw;}
    }
    Worker(const Worker&)=delete;Worker&operator=(const Worker&)=delete;
    ~Worker() override {Stop();}
    void Stop() noexcept {
        if(input_){CloseHandle(input_);input_=nullptr;}
        if(process_){if(WaitForSingleObject(process_,500)==WAIT_TIMEOUT){TerminateProcess(process_,1);WaitForSingleObject(process_,1000);}CloseHandle(process_);process_=nullptr;}
        if(output_){CloseHandle(output_);output_=nullptr;}
        if(job_){CloseHandle(job_);job_=nullptr;}
    }
    foundation::Result<std::int64_t> Invoke(std::int64_t argument) override {Fields fields;return InvokeFields(argument,fields);}
    foundation::Result<std::int64_t> InvokeFields(std::int64_t argument,Fields &fields) {return InvokeFor(operation_,"","preview",argument,fields);}
    using GameplayCall=std::function<foundation::Result<std::int64_t>(std::string_view,std::int64_t)>;
    foundation::Result<std::int64_t> InvokeFor(const std::string &operation,const std::string &type,const std::string &scope,std::int64_t argument,Fields &fields,GameplayCall gameplay={}) {
        try {
            if(!process_)throw std::runtime_error("script.invalid_handle");
            deadline_=std::chrono::steady_clock::now()+std::chrono::seconds(3);
            Write(generation_);Write(++request_);Write(static_cast<std::uint64_t>(argument));Text(operation);Text(type);Text(scope);Write<4>(fields.size());
            for(const auto &[name,value]:fields){Text(name);Write<1>(value.index());Text(std::visit([](const auto &v)->std::string{using T=std::decay_t<decltype(v)>;if constexpr(std::is_same_v<T,std::string>)return v;else if constexpr(std::is_same_v<T,bool>)return v?"true":"false";else {char buffer[64];const auto parsed=std::to_chars(buffer,buffer+64,v);if(parsed.ec!=std::errc{})throw std::runtime_error("script.field_encoding");return {buffer,parsed.ptr};}},value));}
            auto response=Read();unsigned calls=0;
            while(response==0x45504750434C5633ULL){
                if(++calls>256||Read()!=generation_||Read()!=request_)throw std::runtime_error("script.gameplay_budget_or_generation");
                const auto id=Text();const auto arg=std::bit_cast<std::int64_t>(Read());
                auto result=gameplay?gameplay(id,arg):foundation::Result<std::int64_t>::Failure(foundation::Error::Create("gameplay.PlayRequired","No live scope"));
                Write(result?static_cast<std::uint64_t>(result.Value()):0);Text(result?"":result.GetError().code);response=Read();
            }
            if(response!=generation_||Read()!=request_)throw std::runtime_error("script.stale_handle");
            const auto value=std::bit_cast<std::int64_t>(Read());const auto error=Text();
            const auto count=Read(4);if(count>64)throw std::runtime_error("script.field_limit");Fields updated;
            for(std::uint64_t i=0;i<count;++i){auto name=Text();const auto kind=Read(1);const auto text=Text();gameplay::composition::PropertyValue field;
                if(kind==0){std::int64_t number=0;const auto parsed=std::from_chars(text.data(),text.data()+text.size(),number);if(parsed.ec!=std::errc{}||parsed.ptr!=text.data()+text.size())throw std::runtime_error("script.field_type");field=number;}
                else if(kind==1){double number=0;const auto parsed=std::from_chars(text.data(),text.data()+text.size(),number);if(parsed.ec!=std::errc{}||parsed.ptr!=text.data()+text.size()||!std::isfinite(number))throw std::runtime_error("script.field_type");field=number;}
                else if(kind==2){if(text!="true"&&text!="false")throw std::runtime_error("script.field_type");field=text=="true";}
                else if(kind==3)field=text;else throw std::runtime_error("script.field_type");updated.emplace_back(std::move(name),std::move(field));}
            fields=std::move(updated);
            if(!error.empty())throw std::runtime_error(error);
            return foundation::Result<std::int64_t>::Success(value);
        }catch(const std::exception &error){Stop();return foundation::Result<std::int64_t>::Failure(foundation::Error::Create("script.invocation_failed",error.what()));}
    }
};
// One lazy worker per Play (or isolated preview), shared by all native call sites.
class SharedWorker final {
    std::filesystem::path worker_,assembly_;std::string hash_;std::uint64_t generation_;
    std::unique_ptr<Worker> active_;
    bool closed_=false,failed_=false;
    std::uint64_t scopes_=0;
  public:
    SharedWorker(std::filesystem::path worker,std::filesystem::path assembly,std::string hash,std::uint64_t generation):worker_(std::move(worker)),assembly_(std::move(assembly)),hash_(std::move(hash)),generation_(generation){}
    void Stop()noexcept{closed_=true;active_.reset();}
    bool Failed()const noexcept{return failed_;}
    std::string Scope(){if(scopes_>=4096)throw std::runtime_error("script.scope_limit");return "preview:"+std::to_string(++scopes_);}
    foundation::Result<std::int64_t> Invoke(const std::string &operation,const std::string &type,const std::string &scope,std::int64_t argument,Worker::Fields &fields,Worker::GameplayCall gameplay={}){
        try{if(closed_||failed_)throw std::runtime_error("script.invalid_handle");if(!active_)active_=std::make_unique<Worker>(worker_,assembly_,hash_,"*",generation_);auto result=active_->InvokeFor(operation,type,scope,argument,fields,std::move(gameplay));if(!result){failed_=true;Stop();}return result;}
        catch(const std::exception &error){failed_=true;Stop();return foundation::Result<std::int64_t>::Failure(foundation::Error::Create("script.worker_start",error.what()));}
    }
};
class Factory final : public runtime::graph::INativeCallableFactory {
    std::shared_ptr<SharedWorker> worker_;std::string operation_,type_;
    class Invocation final: public runtime::graph::INativeInvocation {
        std::shared_ptr<SharedWorker> worker_;std::string operation_,type_,scope_;
      public:
        Invocation(std::shared_ptr<SharedWorker> worker,std::string operation,std::string type):worker_(std::move(worker)),operation_(std::move(operation)),type_(std::move(type)),scope_(worker_->Scope()){}
        foundation::Result<std::int64_t> Invoke(std::int64_t argument)override{Worker::Fields fields;return worker_->Invoke(operation_,type_,scope_,argument,fields);}
    };
  public:
    Factory(std::shared_ptr<SharedWorker> worker,std::string operation,std::string type):worker_(std::move(worker)),operation_(std::move(operation)),type_(std::move(type)){}
    foundation::Result<std::unique_ptr<runtime::graph::INativeInvocation>> CreateInvocation()const override {
        using Result=foundation::Result<std::unique_ptr<runtime::graph::INativeInvocation>>;
        try{return Result::Success(std::make_unique<Invocation>(worker_,operation_,type_));}catch(const std::exception &error){return Result::Failure(foundation::Error::Create("script.invocation",error.what()));}
    }
};
}
