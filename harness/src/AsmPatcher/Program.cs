// AsmPatcher: makes a COPY of Assembly-CSharp hostable on .NET Framework outside Unity. Two mechanical edits,
// no KSP method body logic is changed:
//  1. TypeRefs UnityEngine.AnimationCurve / UnityEngine.Gradient (native-backed in Unity) are retargeted to managed
//     ports in KspUnityShim (same member names/signatures).
//  2. Every *direct* call from Assembly-CSharp IL to a Unity InternalCall method (e.g. Component.get_gameObject,
//     Object.get_name) is redirected to a generated static stub that throws NotSupportedException naming the native
//     method. Reason: the .NET Framework JIT binds InternalCall targets when compiling the *caller*, so one such call
//     anywhere in a KSP method makes the whole method un-JIT-able ("ECall methods must be packaged into a system
//     module") even if that line never executes. With the stub, only paths that really reach Unity native code throw.
// Usage: AsmPatcher <in.dll> <out.dll> <shim.dll> [<report.txt>]
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Security.Cryptography;
using Mono.Cecil;
using Mono.Cecil.Cil;

static class Program
{
    static readonly string[] Retarget = { "AnimationCurve", "Gradient" };

    static int Main(string[] args)
    {
        if (args.Length < 3) { Console.Error.WriteLine("usage: AsmPatcher <in.dll> <out.dll> <shim.dll> [report]"); return 2; }
        string inPath = Path.GetFullPath(args[0]), outPath = Path.GetFullPath(args[1]), shimPath = Path.GetFullPath(args[2]);
        string report = args.Length > 3 ? args[3] : null;
        var resolver = new DefaultAssemblyResolver();
        resolver.AddSearchDirectory(Path.GetDirectoryName(inPath));
        bool hasPdb = File.Exists(Path.ChangeExtension(inPath, ".pdb"));
        var rp = new ReaderParameters { AssemblyResolver = resolver, ReadSymbols = hasPdb, InMemory = true };
        var asm = AssemblyDefinition.ReadAssembly(inPath, rp);
        var shim = AssemblyDefinition.ReadAssembly(shimPath, new ReaderParameters { InMemory = true });
        var shimRef = new AssemblyNameReference(shim.Name.Name, shim.Name.Version);
        var mod = asm.MainModule;
        var log = new System.Text.StringBuilder();
        log.AppendLine($"input  {inPath} sha256={Sha(inPath)}");
        log.AppendLine($"shim   {shimPath} ({shim.Name.FullName})");

        // ---- 2. InternalCall call sites -> throwing stubs (done first, while TypeRefs still point at UnityEngine)
        var stubType = new TypeDefinition("KspHarness", "UnityNativeStubs",
            TypeAttributes.Public | TypeAttributes.Abstract | TypeAttributes.Sealed | TypeAttributes.Class, mod.TypeSystem.Object);
        mod.Types.Add(stubType);
        var nse = new TypeReference("System", "NotSupportedException", mod, mod.TypeSystem.CoreLibrary);
        var nseCtor = new MethodReference(".ctor", mod.TypeSystem.Void, nse) { HasThis = true };
        nseCtor.Parameters.Add(new ParameterDefinition(mod.TypeSystem.String));
        var stubs = new Dictionary<string, MethodDefinition>();
        var stubSites = new Dictionary<string, int>();
        int sites = 0, unresolved = 0;
        foreach (var type in mod.GetTypes().ToList())
        {
            if (type == stubType) continue;
            foreach (var m in type.Methods)
            {
                if (!m.HasBody) continue;
                foreach (var ins in m.Body.Instructions)
                {
                    if (ins.OpCode != OpCodes.Call && ins.OpCode != OpCodes.Callvirt) continue;
                    if (!(ins.Operand is MethodReference mr) || mr is GenericInstanceMethod || mr.DeclaringType is GenericInstanceType) continue;
                    string scope = mr.DeclaringType.Scope?.Name ?? "";
                    if (!scope.StartsWith("UnityEngine")) continue;
                    if (mr.DeclaringType.Namespace == "UnityEngine" && Retarget.Contains(mr.DeclaringType.Name)) continue; // ported, not stubbed
                    MethodDefinition md;
                    try { md = mr.Resolve(); } catch { md = null; }
                    if (md == null) { unresolved++; continue; }
                    if (!md.IsInternalCall || md.IsVirtual) continue;
                    string key = mr.FullName;
                    if (!stubs.TryGetValue(key, out var stub))
                    {
                        stub = new MethodDefinition("Stub" + stubs.Count + "_" + mr.DeclaringType.Name + "_" + mr.Name,
                            MethodAttributes.Public | MethodAttributes.Static | MethodAttributes.HideBySig, mr.ReturnType);
                        if (mr.HasThis)
                        {
                            TypeReference self = mr.DeclaringType.IsValueType ? (TypeReference)new ByReferenceType(mr.DeclaringType) : mr.DeclaringType;
                            stub.Parameters.Add(new ParameterDefinition("self", ParameterAttributes.None, self));
                        }
                        foreach (var p in mr.Parameters) stub.Parameters.Add(new ParameterDefinition(p.Name, p.Attributes, p.ParameterType));
                        var il = stub.Body.GetILProcessor();
                        il.Emit(OpCodes.Ldstr, "Unity native code not available outside Unity: " + key);
                        il.Emit(OpCodes.Newobj, nseCtor);
                        il.Emit(OpCodes.Throw);
                        stubType.Methods.Add(stub);
                        stubs[key] = stub;
                    }
                    ins.OpCode = OpCodes.Call;
                    ins.Operand = stub;
                    sites++;
                    stubSites[key] = stubSites.TryGetValue(key, out int c) ? c + 1 : 1;
                }
            }
        }
        log.AppendLine($"InternalCall call sites redirected to throwing stubs: {sites} sites, {stubs.Count} distinct natives, {unresolved} unresolved refs skipped");
        foreach (var kv in stubSites.OrderByDescending(k => k.Value).ThenBy(k => k.Key)) log.AppendLine($"  {kv.Value,5}  {kv.Key}");

        // ---- 1. retarget native-backed Unity types to managed ports
        mod.AssemblyReferences.Add(shimRef);
        int retargeted = 0;
        foreach (var tr in mod.GetTypeReferences().ToList())
        {
            if (tr.Namespace == "UnityEngine" && Retarget.Contains(tr.Name))
            {
                log.AppendLine($"retarget TypeRef {tr.FullName} [{tr.Scope}] -> KspUnityShim.{tr.Name} [{shimRef.Name}]");
                tr.Namespace = "KspUnityShim";
                tr.Scope = shimRef;
                retargeted++;
            }
        }
        int missing = 0;
        foreach (var tn in Retarget)
        {
            string full = "KspUnityShim." + tn;
            var members = mod.GetMemberReferences().Where(m => m.DeclaringType != null && m.DeclaringType.FullName == full)
                .Select(m => m.FullName).Distinct().OrderBy(s => s).ToList();
            log.AppendLine($"member references now bound to {full}: {members.Count}");
            foreach (var m in members) log.AppendLine("  " + m);
            var shimType = shim.MainModule.GetType(full);
            if (shimType == null) { log.AppendLine("  MISSING shim type " + full); missing++; continue; }
            foreach (var mr in mod.GetMemberReferences().Where(m => m.DeclaringType != null && m.DeclaringType.FullName == full))
            {
                bool found = shimType.Methods.Any(md => md.Name == mr.Name && (mr is MethodReference mref ? md.Parameters.Count == mref.Parameters.Count : true))
                             || shimType.Fields.Any(f => f.Name == mr.Name);
                if (!found) { missing++; log.AppendLine("  MISSING in shim: " + mr.FullName); }
            }
        }
        Directory.CreateDirectory(Path.GetDirectoryName(outPath));
        asm.Write(outPath, new WriterParameters { WriteSymbols = hasPdb });
        log.AppendLine($"output {outPath} sha256={Sha(outPath)} typeRefsRetargeted={retargeted} missingMembers={missing}");
        Console.Write(log.ToString());
        if (report != null) File.WriteAllText(report, log.ToString());
        return (retargeted == Retarget.Length && missing == 0) ? 0 : 1;
    }

    static string Sha(string p)
    {
        using var s = File.OpenRead(p);
        return Convert.ToHexString(SHA256.HashData(s)).ToLowerInvariant();
    }
}
