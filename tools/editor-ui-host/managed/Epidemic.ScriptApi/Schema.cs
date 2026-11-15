namespace Epidemic.ScriptApi;

[AttributeUsage(AttributeTargets.Class)]
public sealed class ScriptComponentAttribute(string id) : Attribute { public string Id { get; } = id; }
[AttributeUsage(AttributeTargets.Property)]
public sealed class ScriptFieldAttribute : Attribute { }
[AttributeUsage(AttributeTargets.Method)]
public sealed class GraphOperationAttribute(string id) : Attribute { public string Id { get; } = id; }
