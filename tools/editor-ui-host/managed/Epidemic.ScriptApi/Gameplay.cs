using System.Runtime.CompilerServices;
[assembly: InternalsVisibleTo("Epidemic.ScriptWorker")]
namespace Epidemic.ScriptApi;

/// <summary>Calls the native gameplay owner for the current live Entity scope.
/// No time, Entity model, Health implementation or timer runs in managed code.</summary>
public static class Gameplay
{
    [ThreadStatic] internal static Func<string,long,long>? Dispatch;
    public static long Call(string operation, long argument = 0)
        => (Dispatch ?? throw new InvalidOperationException("gameplay.PlayRequired"))(operation, argument);
    public static long Health => Call("gameplay.health.get.v1");
    public static long ApplyDamage(long amount) => Call("gameplay.health.damage.v1", amount);
    public static long Heal(long amount) => Call("gameplay.health.heal.v1", amount);
    public static long Currency => Call("gameplay.economy.get.v1");
    public static long Award(long amount) => Call("gameplay.economy.award.v1", amount);
    public static long Spend(long amount) => Call("gameplay.economy.spend.v1", amount);
    public static long FixedTick => Call("gameplay.time.tick.v1");
    public static long StartTimer(long ticks) => Call("gameplay.time.timer.v1", ticks);
    public static long SpawnPrefab(ulong stableAssetId) => Call("gameplay.entity.spawn.v1", unchecked((long)stableAssetId));
    public static long FireProjectile() => Call("gameplay.projectile.fire.v1");
    public static long Destroy() => Call("gameplay.entity.destroy.v1");
}
