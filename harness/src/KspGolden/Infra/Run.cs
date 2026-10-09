using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Security.Cryptography;

namespace KspGolden.Infra
{
    /// <summary>Run-wide settings and provenance shared by every golden file (deterministic: no timestamps).</summary>
    public static class Run
    {
        public static string HarnessRoot;
        public static string InputsDir;   // harness/inputs (copies of KSP-dev files used as inputs)
        public static string GoldenDir;   // output root
        public static string SerializedDir; // golden/serialized (asset extraction output)
        public static readonly List<KeyValuePair<string, object>> CommonProvenance = new List<KeyValuePair<string, object>>();
        public static CaptureLogHandler Log;

        public static string Sha256(string path)
        {
            using (var s = File.OpenRead(path))
            using (var h = SHA256.Create())
                return BitConverter.ToString(h.ComputeHash(s)).Replace("-", "").ToLowerInvariant();
        }

        public static void InitProvenance(string kspIlLabel, string pristineSha)
        {
            var asm = typeof(Orbit).Assembly;
            CommonProvenance.Add(new KeyValuePair<string, object>("ksp_version", "1.12.5 build 03190 (Unity 2019.4.18f1)"));
            CommonProvenance.Add(new KeyValuePair<string, object>("ksp_il", kspIlLabel));
            CommonProvenance.Add(new KeyValuePair<string, object>("assembly_csharp_input_sha256", pristineSha));
            CommonProvenance.Add(new KeyValuePair<string, object>("assembly_csharp_patched_sha256", Sha256(asm.Location)));
            CommonProvenance.Add(new KeyValuePair<string, object>("kspunityshim_sha256", Sha256(typeof(KspUnityShim.AnimationCurve).Assembly.Location)));
            CommonProvenance.Add(new KeyValuePair<string, object>("runtime", ".NET Framework CLR " + Environment.Version + (Environment.Is64BitProcess ? " x64" : " x86")));
            CommonProvenance.Add(new KeyValuePair<string, object>("float_semantics", "IEEE-754 binary32/binary64, SSE2 (RyuJIT x64); libm from the .NET Framework CLR, not Unity's Mono"));
            CommonProvenance.Add(new KeyValuePair<string, object>("culture", System.Threading.Thread.CurrentThread.CurrentCulture.Name));
        }
    }
}
