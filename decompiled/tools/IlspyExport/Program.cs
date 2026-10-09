// Headless equivalent of ILSpy 8.2 GUI "Save Code" (whole-project export) with:
//   Language C# 7.3 / VS2017.7, "Use new SDK style format" = OFF, "Use nested directories for namespaces" = ON.
// All other DecompilerSettings are left at their defaults (same as a fresh ILSpy 8.2 install).
// Usage: IlspyExport <assembly.dll> <emptyOutDir> <projectName> [--list-only] [--lang 7.3|latest] [refSearchDir...]
//   --lang defaults to 7.3 (the guide setting, used for Squad/third-party code). "latest" = the newest C# version
//   ICSharpCode.Decompiler 8.2 supports (LanguageVersion.Latest); used for Unity/Mono BCL assemblies.
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection.Metadata;
using System.Reflection.PortableExecutable;
using ICSharpCode.Decompiler;
using ICSharpCode.Decompiler.CSharp;
using ICSharpCode.Decompiler.CSharp.ProjectDecompiler;
using ICSharpCode.Decompiler.Metadata;

static class Program
{
    static int Main(string[] args)
    {
        if (args.Length < 3) { Console.Error.WriteLine("usage: IlspyExport <assembly> <outDir> <projectName> [--list-only] [--lang 7.3|latest] [refDir...]"); return 1; }
        string asm = Path.GetFullPath(args[0]);
        string outDir = Path.GetFullPath(args[1]);
        string projName = args[2];
        bool listOnly = false;
        var langVersion = LanguageVersion.CSharp7_3;
        var refDirs = new List<string>();
        var rest = args.Skip(3).ToList();
        for (int i = 0; i < rest.Count; i++)
        {
            if (rest[i] == "--list-only") listOnly = true;
            else if (rest[i] == "--lang" && i + 1 < rest.Count)
            {
                string v = rest[++i];
                if (v == "7.3") langVersion = LanguageVersion.CSharp7_3;
                else if (v == "latest") langVersion = LanguageVersion.Latest;
                else { Console.Error.WriteLine("unknown --lang " + v + " (use 7.3 or latest)"); return 1; }
            }
            else refDirs.Add(rest[i]);
        }

        Console.WriteLine("ICSharpCode.Decompiler " + typeof(WholeProjectDecompiler).Assembly.GetName().Version);
        var module = new PEFile(asm, PEStreamOptions.PrefetchEntireImage);
        string tfm = module.Metadata.DetectTargetFrameworkId();
        Console.WriteLine("Module: " + module.Name + "  TargetFrameworkId: " + tfm);
        var resolver = new UniversalAssemblyResolver(asm, false, tfm);
        foreach (var d in refDirs) resolver.AddSearchDirectory(d);

        int unresolved = 0;
        foreach (var r in module.AssemblyReferences)
        {
            string f = resolver.FindAssemblyFile(r);
            if (f == null) unresolved++;
            Console.WriteLine("  ref " + r.FullName + " -> " + (f ?? "UNRESOLVED"));
        }
        foreach (var res in module.Resources)
            Console.WriteLine("  resource " + res.Name + " (" + res.ResourceType + ")");
        if (listOnly) return unresolved == 0 ? 0 : 3;

        Directory.CreateDirectory(outDir);
        if (Directory.EnumerateFileSystemEntries(outDir).Any()) { Console.Error.WriteLine("Output directory is not empty: " + outDir); return 2; }

        Console.WriteLine("LanguageVersion: " + langVersion + " (" + (int)langVersion + ")");
        var settings = new DecompilerSettings(langVersion) {
            UseSdkStyleProjectFormat = false,
            UseNestedDirectoriesForNamespaces = true,
        };
        var decompiler = new WholeProjectDecompiler(settings, resolver, resolver, null);
        int last = -1;
        decompiler.ProgressIndicator = new Progress<DecompilationProgress>(p => {
            if (p.TotalUnits <= 0) return;
            int pct = (int)(100L * p.UnitsCompleted / p.TotalUnits);
            if (pct / 10 != last / 10) { last = pct; Console.WriteLine($"  {p.Title} {p.UnitsCompleted}/{p.TotalUnits} ({pct}%)"); }
        });
        var sw = System.Diagnostics.Stopwatch.StartNew();
        string projFile = Path.Combine(outDir, projName + ".csproj");
        using (var w = new StreamWriter(projFile))
            decompiler.DecompileProject(module, outDir, w);
        Console.WriteLine("Done in " + sw.Elapsed + " -> " + projFile);
        return 0;
    }
}
