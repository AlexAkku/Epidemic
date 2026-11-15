using System.Buffers.Binary;
using System.Reflection;
using System.Runtime.Loader;
using System.Security.Cryptography;
using System.Text;
using Epidemic.ScriptApi;
using Epidemic.ManagedBridge;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;
using System.Reflection.Metadata;
using System.Reflection.Metadata.Ecma335;

// One bounded synchronous invocation stream; process exit is the unload boundary.
static unsafe class Program
{
    const int Limit = 1024 * 1024;
    static readonly Encoding Utf8 = new UTF8Encoding(false, true);
    static void U(Stream s, ulong value, int width = 8) { Span<byte> bytes=stackalloc byte[8]; BinaryPrimitives.WriteUInt64BigEndian(bytes,value); s.Write(bytes[(8-width)..]); }
    static ulong R(Stream s, int width = 8) { Span<byte> bytes=stackalloc byte[8]; bytes.Clear(); s.ReadExactly(bytes[(8-width)..]); return BinaryPrimitives.ReadUInt64BigEndian(bytes); }
    static void Text(Stream s,string value) { var bytes=Utf8.GetBytes(value); if(bytes.Length>Limit)throw new InvalidDataException("script.string_limit"); U(s,(ulong)bytes.Length,4);s.Write(bytes); }
    static string ReadText(Stream s) { var length=R(s,4); if(length>Limit)throw new InvalidDataException("script.string_limit"); byte[] bytes=new byte[(int)length];s.ReadExactly(bytes);return Utf8.GetString(bytes); }
    static IEnumerable<PropertyInfo> Fields(Type type) => type.GetProperties().Where(p=>p.IsDefined(typeof(ScriptFieldAttribute))).OrderBy(p=>p.Name,StringComparer.Ordinal);
    static IEnumerable<MethodInfo> Methods(Type type) => type.GetMethods().Where(m=>m.IsDefined(typeof(GraphOperationAttribute))).OrderBy(m=>m.GetCustomAttribute<GraphOperationAttribute>()!.Id,StringComparer.Ordinal);
    static void Value(Stream output,object? value)
    {
        switch(value) {
            case long integer: U(output,0,1);Text(output,integer.ToString(System.Globalization.CultureInfo.InvariantCulture));break;
            case double real when double.IsFinite(real): U(output,1,1);Text(output,real.ToString("R",System.Globalization.CultureInfo.InvariantCulture));break;
            case bool boolean: U(output,2,1);Text(output,boolean?"true":"false");break;
            case string text when text.Length<=16384: U(output,3,1);Text(output,text);break;
            default: throw new InvalidDataException("script.unsupported_field_type");
        }
    }
    static object ReadValue(Stream stream) {
        ulong kind=R(stream,1);string value=ReadText(stream);var culture=System.Globalization.CultureInfo.InvariantCulture;
        return kind switch {0=>long.Parse(value,culture),1=>double.Parse(value,culture),2=>bool.Parse(value),3=>value,_=>throw new InvalidDataException("script.field_type")};
    }
    static (string Path,ulong Line) Source(MethodInfo method,string assembly,string root) {
        string path="Scripts/"+method.DeclaringType!.Name+".cs";ulong line=1;
        string pdb=System.IO.Path.ChangeExtension(assembly,".pdb");
        if(File.Exists(pdb)){
            using var stream=File.OpenRead(pdb);using var provider=MetadataReaderProvider.FromPortablePdbStream(stream);var reader=provider.GetMetadataReader();
            var info=reader.GetMethodDebugInformation(MetadataTokens.MethodDefinitionHandle(method.MetadataToken&0xffffff));
            var point=info.GetSequencePoints().FirstOrDefault(p=>!p.IsHidden);
            var document=point.Document.IsNil?info.Document:point.Document;
            if(!document.IsNil){string file=reader.GetString(reader.GetDocument(document).Name);string relative=System.IO.Path.GetRelativePath(root,file);
                if(!relative.StartsWith("..",StringComparison.Ordinal)&&!System.IO.Path.IsPathRooted(relative)){path=relative.Replace('\\','/');line=(ulong)point.StartLine;}}
        }
        return(path,line);
    }
    static void ValidateMethod(MethodInfo method) {
        if(method.IsStatic||method.IsGenericMethod||method.ReturnType!=typeof(long)||method.GetParameters() is not [{ParameterType:var argument}]||argument!=typeof(long))
            throw new InvalidDataException("script.operation_abi_requires_instance_i64_i64: "+method.Name);
    }
    sealed class ReflectedCallable(object owner,MethodInfo method) : ManagedCallable {
        protected override long Invoke(long value,NativeContext native)=>(long)method.Invoke(owner,[value])!;
    }
    [UnmanagedCallersOnly(CallConvs=[typeof(CallConvCdecl)])]
    static int NoNative(void* context,long* input,long* output,uint count,uint* completed){*completed=0;return (int)BridgeStatus.NativeFailure;}
    static int Main(string[] args)
    {
        try {
            if(args.Length<3)throw new InvalidDataException("script.arguments");
            if(args[0]=="invoke")Console.SetOut(Console.Error);
            string assembly=Path.GetFullPath(args[1]);
            string fingerprint=Convert.ToHexString(SHA256.HashData(File.ReadAllBytes(assembly))).ToLowerInvariant();
            if(args[0]=="invoke"&&fingerprint!=args[2])throw new InvalidDataException("script.assembly_fingerprint");
            AssemblyLoadContext.Default.Resolving+=(_,name)=>name.Name==typeof(ScriptComponentAttribute).Assembly.GetName().Name?typeof(ScriptComponentAttribute).Assembly:null;
            // Only explicit build/register or invocation executes user constructors; never project-open.
            var loaded=AssemblyLoadContext.Default.LoadFromAssemblyPath(assembly);
            var types=loaded.GetTypes().Where(t=>t.IsDefined(typeof(ScriptComponentAttribute))).OrderBy(t=>t.FullName,StringComparer.Ordinal).ToArray();
            if(types.Length>64)throw new InvalidDataException("script.type_limit");
            if(args[0]=="schema") {
                using var staged=new MemoryStream();U(staged,1,2);Text(staged,args[3]);Text(staged,fingerprint);U(staged,(ulong)types.Length,4);
                var ids=new HashSet<string>(StringComparer.Ordinal);
                foreach(var type in types) {
                    string id=type.GetCustomAttribute<ScriptComponentAttribute>()!.Id;
                    if(!ids.Add(id)||id.Length==0)throw new InvalidDataException("script.duplicate_id");
                    object instance=Activator.CreateInstance(type)??throw new InvalidDataException("script.constructor");
                    Text(staged,id);U(staged,1);var fields=Fields(type).ToArray();if(fields.Length is <1 or >64)throw new InvalidDataException("script.fields");
                    U(staged,(ulong)fields.Length,4);
                    foreach(var property in fields) {
                        if(!property.CanRead||!property.CanWrite||property.GetIndexParameters().Length!=0)throw new InvalidDataException("script.field_access");
                        Text(staged,property.Name);Value(staged,property.GetValue(instance));
                    }
                    var methods=Methods(type).ToArray();if(methods.Length>64)throw new InvalidDataException("script.method_limit");U(staged,(ulong)methods.Length,4);
                    foreach(var method in methods) {
                        ValidateMethod(method);string op=method.GetCustomAttribute<GraphOperationAttribute>()!.Id;
                        if(!ids.Add(op)||op.Length==0)throw new InvalidDataException("script.duplicate_id");
                        Text(staged,op);Text(staged,type.Name+"."+method.Name);Text(staged,type.FullName!);Text(staged,method.Name);
                        var location=Source(method,assembly,args.Length>4?args[4]:Environment.CurrentDirectory);Text(staged,location.Path);U(staged,location.Line);
                    }
                }
                if(staged.Length>Limit)throw new InvalidDataException("script.schema_limit");
                File.WriteAllBytes(args[2],staged.ToArray());return 0;
            }
            if(args[0]!="invoke"||args.Length!=5)throw new InvalidDataException("script.arguments");
            ulong generation=ulong.Parse(args[4],System.Globalization.CultureInfo.InvariantCulture);
            var operations=types.SelectMany(Methods).ToDictionary(m=>m.GetCustomAttribute<GraphOperationAttribute>()!.Id,StringComparer.Ordinal);
            foreach(var method in operations.Values)ValidateMethod(method);
            using var input=Console.OpenStandardInput();using var output=Console.OpenStandardOutput();
            U(output,0x45505343,4);U(output,2,2);U(output,generation);output.Flush();
            NativeApi api=new(){Size=(uint)sizeof(NativeApi),Version=BridgeExports.Version,InvokeBatch=&NoNative};
            var owners=new Dictionary<string,object>(StringComparer.Ordinal);var sessions=new Dictionary<string,ulong>(StringComparer.Ordinal);
            ulong expected=1;
            try { while(true) {
                var requestGeneration=R(input);var request=R(input);var argument=unchecked((long)R(input));
                string operation=ReadText(input),expectedType=ReadText(input),scope=ReadText(input);string error="";long result=0;object? owner=null;Type? target=null;
                var count=R(input,4);if(count>64||operation.Length>256||expectedType.Length>256||scope.Length>256)throw new InvalidDataException("script.request_limit");
                var staged=new List<(string Name,object Value)>();for(ulong index=0;index<count;++index)staged.Add((ReadText(input),ReadValue(input)));
                if(requestGeneration!=generation||request!=expected++)error="script.stale_handle";
                else try {
                    if(!operations.TryGetValue(operation,out var selected))throw new InvalidDataException("script.missing_operation");
                    target=selected.DeclaringType!;if(target.GetCustomAttribute<ScriptComponentAttribute>()!.Id!=expectedType)throw new InvalidDataException("script.type_mismatch");string ownerKey=scope+"|"+target.FullName,sessionKey=ownerKey+"|"+operation;
                    if(!owners.TryGetValue(ownerKey,out owner)){
                        if(owners.Count>=1024)throw new InvalidDataException("script.owner_limit");owner=Activator.CreateInstance(target)!;owners.Add(ownerKey,owner);
                    }
                    var fields=Fields(target).ToDictionary(p=>p.Name,StringComparer.Ordinal);var seen=new HashSet<string>(StringComparer.Ordinal);
                    foreach(var field in staged)if(!seen.Add(field.Name)||!fields.TryGetValue(field.Name,out var property)||property.PropertyType!=field.Value.GetType())throw new InvalidDataException("script.field_type");
                    foreach(var field in staged)fields[field.Name].SetValue(owner,field.Value);
                    if(!sessions.TryGetValue(sessionKey,out ulong handle)){
                        object instance=owner;ulong created=0;int opened=BridgeRuntime.Open(&api,()=>new ReflectedCallable(instance,selected),&created);
                        if(opened!=0)throw new InvalidDataException("script.abi_open: "+opened);handle=created;sessions.Add(sessionKey,handle);
                    }
                    uint completed=0;int code=BridgeRuntime.InvokeBatch(handle,&argument,&result,1,&completed);
                    if(code!=0||completed!=1)throw new InvalidDataException("script.abi_invoke: "+(BridgeStatus)code);
                }catch(Exception failure){error="script.exception: "+(failure.InnerException??failure).Message;}
                using var response=new MemoryStream();U(response,generation);U(response,request);U(response,unchecked((ulong)result));Text(response,error.Length>4096?error[..4096]:error);
                var updated=error.Length==0&&target!=null?Fields(target).ToArray():[];U(response,(ulong)updated.Length,4);foreach(var field in updated){Text(response,field.Name);Value(response,field.GetValue(owner));}
                if(response.Length>Limit)throw new InvalidDataException("script.response_limit");response.Position=0;response.CopyTo(output);output.Flush();
            }}finally{foreach(var handle in sessions.Values)BridgeRuntime.Close(handle);foreach(var owner in owners.Values)if(owner is IDisposable disposable)try{disposable.Dispose();}catch{}}
        } catch(EndOfStreamException) {return 0;}
        catch(Exception error) {Console.Error.WriteLine("script.worker: "+error.Message);return 1;}
    }
}
