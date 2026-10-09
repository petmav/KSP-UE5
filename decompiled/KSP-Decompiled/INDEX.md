# INDEX - every assembly shipped with KSP 1.12.5 (build 03190, Unity 2019.4.18f1)

Generated 2026-10-08 by `tools\scripts\make_index.py` from `logs\decompile-all-*.csv` (written by `tools\scripts\decompile_all.ps1`) plus live counts of the decompiled trees. 
Source install: `C:\Projects\KSP-UE5\decompiled\KSP-dev\` (robocopy of the Steam install; the file lists are identical apart from KSP-dev's symlinked `Assembly-CSharp.dll/.pdb`, `KSP.log` and `settings.cfg`).

- **Decompiler:** ICSharpCode.Decompiler 8.2.0.7535 via `tools\IlspyExport` (legacy csproj, nested namespace directories). C# 7.3 for Squad and third-party assemblies, C# latest (= 11.0 in 8.2) for Unity and Mono BCL assemblies.
- **Obfuscated:** `de4dot -d` (v3.2.0) on every managed file. Only the game's `Assembly-CSharp.dll` has a known obfuscator (Crypto Obfuscator); de4dot reported "Unknown Obfuscator" (= none detected) for all others. For the game's Assembly-CSharp the stock original (`backup\Assembly-CSharp.dll`) was probed, since the Managed copy is now a symlink to our build.
- **Lines** = newline count over the `.cs` files (`wc -l`). **Buildable**: only `Assembly-CSharp\` is built (0 errors); everything else is read-only reference source and was never compiled.
- Paths in the *Decompiled path* column are relative to `KSP-Decompiled\`.
- **Reproduce** (tools live outside this repo, in `C:\Projects\KSP-UE5\decompiled\tools\`): `powershell -NoProfile -ExecutionPolicy Bypass -File tools\scripts\decompile_all.ps1 [-Group Unity,Mono,ThirdParty,Launcher,Expansions] [-Force]`, then the same script with `-Inventory`, then `python tools\scripts\make_index.py`. The earlier Squad decompilations (Assembly-CSharp, firstpass, KSPAssets*, KSPTrackIR, KSPSteamCtrlr) are documented in `FIXES.md` / `..\STATUS.md`.
- **Totals:** 188 .dll/.exe files in the install: 180 managed, 8 native. 180 managed assemblies decompiled, 22,901 .cs files, 3,261,699 lines.

| Section | Files (.dll/.exe) | Managed | Native | Decompiled | .cs files | Lines |
|---|---|---|---|---|---|---|
| Game: `KSP_x64_Data\Managed\` | 90 | 90 | 0 | 90 | 13,020 | 1,899,258 |
| Game: `GameData\` plugins | 1 | 1 | 0 | 1 | 7 | 481 |
| Launcher: `Launcher_Data\Managed\` | 89 | 89 | 0 | 89 | 9,874 | 1,361,960 |
| Native (not .NET) | 8 | 0 | 8 | 0 | 0 | 0 |

| Origin | Assemblies decompiled | .cs files | Lines |
|---|---|---|---|
| Squad | 8 | 3,891 | 698,905 |
| third-party | 4 | 435 | 61,358 |
| Mono | 37 | 13,565 | 1,986,490 |
| Unity | 131 | 5,010 | 514,946 |

60 of the launcher's managed DLLs are byte-identical (sha256) to the game's copies; they were still decompiled separately under `Launcher\` (git stores identical blobs once).

No DLC: `GameData\SquadExpansion\` does not exist in KSP-dev or in the Steam install (checked read-only), so there are no expansion plugin DLLs and no `Expansions\` output directory. The Making History / Breaking Ground *code* is compiled into Assembly-CSharp (`Assembly-CSharp\Expansions\`). `GameData\Squad\Plugins\` contains only KSPSteamCtrlr.dll.

## Game: `KSP_x64_Data\Managed\`

| Assembly | Origin | Version | Obfuscated | Decompiled path | .cs files | Lines | Buildable | Notes |
|---|---|---|---|---|---|---|---|---|
| Assembly-CSharp | Squad | 0.0.0.0 | y (Crypto Obfuscator) | `Assembly-CSharp\` | 3,299 | 627,747 | **yes** | C# 7.3; de4dot --dont-rename first; 23 compile fixes (FIXES.md); built DLL is symlinked into KSP-dev |
| Assembly-CSharp-firstpass | Squad | 0.0.0.0 | n | `Assembly-CSharp-firstpass\` | 413 | 33,052 | no | C# 7.3; Squad-shipped, mostly third-party content (Steamworks.NET, Vectrosity, ...) |
| KSPAssets | Squad | 1.5.0.0 | n | `KSPAssets\` | 11 | 2,502 | no | C# 7.3 |
| KSPAssets.XmlSerializers | Squad | 1.4.0.0 | n | `KSPAssets.XmlSerializers\` | 11 | 1,769 | no | C# 7.3 |
| KSPTrackIR | Squad | 1.0.0.0 | n | `KSPTrackIR\` | 3 | 481 | no | C# 7.3 |
| Ionic.Zip | third-party | 1.9.1.8 | n | `ThirdParty\Ionic.Zip\` | 96 | 21,445 | no | C# 7.3 |
| Mono.Cecil | third-party | 0.9.6.0 | n | `ThirdParty\Mono.Cecil\` | 235 | 26,649 | no | C# 7.3 |
| TDx.TDxInput | third-party | 1.1.0.0 | n | `ThirdParty\TDx.TDxInput\` | 35 | 1,367 | no | C# 7.3 |
| I18N | Mono | 2.0.5.0 | n | `Mono\I18N\` | 8 | 991 | no | C# latest (11.0) |
| I18N.CJK | Mono | 2.0.5.0 | n | `Mono\I18N.CJK\` | 37 | 3,653 | no | C# latest (11.0) |
| I18N.MidEast | Mono | 2.0.5.0 | n | `Mono\I18N.MidEast\` | 16 | 2,968 | no | C# latest (11.0) |
| I18N.Other | Mono | 2.0.5.0 | n | `Mono\I18N.Other\` | 39 | 2,949 | no | C# latest (11.0) |
| I18N.Rare | Mono | 2.0.5.0 | n | `Mono\I18N.Rare\` | 76 | 29,213 | no | C# latest (11.0) |
| I18N.West | Mono | 2.0.5.0 | n | `Mono\I18N.West\` | 32 | 8,108 | no | C# latest (11.0) |
| Mono.Posix | Mono | 4.0.0.0 | n | `Mono\Mono.Posix\` | 158 | 16,952 | no | C# latest (11.0) |
| Mono.Security | Mono | 4.0.0.0 | n | `Mono\Mono.Security\` | 175 | 27,284 | no | C# latest (11.0) |
| mscorlib | Mono | 4.0.0.0 | n | `Mono\mscorlib\` | 2,292 | 310,085 | no | C# latest (11.0) |
| System | Mono | 4.0.0.0 | n | `Mono\System\` | 1,566 | 191,144 | no | C# latest (11.0) |
| System.Configuration | Mono | 4.0.0.0 | n | `Mono\System.Configuration\` | 139 | 9,793 | no | C# latest (11.0) |
| System.Core | Mono | 4.0.0.0 | n | `Mono\System.Core\` | 587 | 72,829 | no | C# latest (11.0) |
| System.Security | Mono | 4.0.0.0 | n | `Mono\System.Security\` | 137 | 13,626 | no | C# latest (11.0) |
| System.Xml | Mono | 4.0.0.0 | n | `Mono\System.Xml\` | 1,080 | 244,097 | no | C# latest (11.0) |
| Unity.Analytics.DataPrivacy | Unity | 0.0.0.0 | n | `Unity\Unity.Analytics.DataPrivacy\` | 3 | 168 | no | C# latest (11.0) |
| Unity.Analytics.StandardEvents | Unity | 1.0.0.0 | n | `Unity\Unity.Analytics.StandardEvents\` | 52 | 1,762 | no | C# latest (11.0) |
| Unity.Analytics.Tracker | Unity | 1.0.0.0 | n | `Unity\Unity.Analytics.Tracker\` | 20 | 1,209 | no | C# latest (11.0) |
| Unity.RenderPipelines.Core.Runtime | Unity | 0.0.0.0 | n | `Unity\Unity.RenderPipelines.Core.Runtime\` | 174 | 12,953 | no | C# latest (11.0) |
| Unity.RenderPipelines.Core.ShaderLibrary | Unity | 0.0.0.0 | n | `Unity\Unity.RenderPipelines.Core.ShaderLibrary\` | 2 | 8 | no | C# latest (11.0) |
| Unity.RenderPipelines.ShaderGraph.ShaderGraphLibrary | Unity | 0.0.0.0 | n | `Unity\Unity.RenderPipelines.ShaderGraph.ShaderGraphLibrary\` | 2 | 8 | no | C# latest (11.0) |
| Unity.Timeline | Unity | 1.0.0.0 | n | `Unity\Unity.Timeline\` | 80 | 7,936 | no | C# latest (11.0) |
| UnityEngine | Unity | 0.0.0.0 | n | `Unity\UnityEngine\` | 24 | 2,267 | no | C# latest (11.0) |
| UnityEngine.AccessibilityModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.AccessibilityModule\` | 2 | 258 | no | C# latest (11.0) |
| UnityEngine.AIModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.AIModule\` | 32 | 2,650 | no | C# latest (11.0) |
| UnityEngine.AndroidJNIModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.AndroidJNIModule\` | 14 | 4,021 | no | C# latest (11.0) |
| UnityEngine.AnimationModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.AnimationModule\` | 100 | 9,681 | no | C# latest (11.0) |
| UnityEngine.ARModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.ARModule\` | 6 | 374 | no | C# latest (11.0) |
| UnityEngine.AssetBundleModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.AssetBundleModule\` | 11 | 852 | no | C# latest (11.0) |
| UnityEngine.AudioModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.AudioModule\` | 44 | 3,079 | no | C# latest (11.0) |
| UnityEngine.ClothModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.ClothModule\` | 4 | 565 | no | C# latest (11.0) |
| UnityEngine.ClusterInputModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.ClusterInputModule\` | 3 | 214 | no | C# latest (11.0) |
| UnityEngine.ClusterRendererModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.ClusterRendererModule\` | 2 | 167 | no | C# latest (11.0) |
| UnityEngine.CoreModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.CoreModule\` | 732 | 58,564 | no | C# latest (11.0) |
| UnityEngine.CrashReportingModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.CrashReportingModule\` | 2 | 178 | no | C# latest (11.0) |
| UnityEngine.DirectorModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.DirectorModule\` | 2 | 385 | no | C# latest (11.0) |
| UnityEngine.DSPGraphModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.DSPGraphModule\` | 10 | 599 | no | C# latest (11.0) |
| UnityEngine.GameCenterModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.GameCenterModule\` | 21 | 1,191 | no | C# latest (11.0) |
| UnityEngine.GridModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.GridModule\` | 3 | 430 | no | C# latest (11.0) |
| UnityEngine.HotReloadModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.HotReloadModule\` | 1 | 139 | no | C# latest (11.0) |
| UnityEngine.ImageConversionModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.ImageConversionModule\` | 2 | 266 | no | C# latest (11.0) |
| UnityEngine.IMGUIModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.IMGUIModule\` | 41 | 10,834 | no | C# latest (11.0) |
| UnityEngine.InputLegacyModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.InputLegacyModule\` | 15 | 1,320 | no | C# latest (11.0) |
| UnityEngine.InputModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.InputModule\` | 7 | 336 | no | C# latest (11.0) |
| UnityEngine.JSONSerializeModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.JSONSerializeModule\` | 2 | 214 | no | C# latest (11.0) |
| UnityEngine.LocalizationModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.LocalizationModule\` | 2 | 185 | no | C# latest (11.0) |
| UnityEngine.ParticleSystemModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.ParticleSystemModule\` | 58 | 10,686 | no | C# latest (11.0) |
| UnityEngine.PerformanceReportingModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.PerformanceReportingModule\` | 2 | 163 | no | C# latest (11.0) |
| UnityEngine.Physics2DModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.Physics2DModule\` | 53 | 6,508 | no | C# latest (11.0) |
| UnityEngine.PhysicsModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.PhysicsModule\` | 46 | 5,279 | no | C# latest (11.0) |
| UnityEngine.ProfilerModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.ProfilerModule\` | 1 | 139 | no | C# latest (11.0) |
| UnityEngine.ScreenCaptureModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.ScreenCaptureModule\` | 2 | 194 | no | C# latest (11.0) |
| UnityEngine.SharedInternalsModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.SharedInternalsModule\` | 40 | 813 | no | C# latest (11.0) |
| UnityEngine.SpriteMaskModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.SpriteMaskModule\` | 2 | 223 | no | C# latest (11.0) |
| UnityEngine.SpriteShapeModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.SpriteShapeModule\` | 9 | 454 | no | C# latest (11.0) |
| UnityEngine.StreamingModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.StreamingModule\` | 2 | 165 | no | C# latest (11.0) |
| UnityEngine.SubstanceModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.SubstanceModule\` | 9 | 564 | no | C# latest (11.0) |
| UnityEngine.SubsystemsModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.SubsystemsModule\` | 13 | 602 | no | C# latest (11.0) |
| UnityEngine.TerrainModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.TerrainModule\` | 21 | 3,803 | no | C# latest (11.0) |
| UnityEngine.TerrainPhysicsModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.TerrainPhysicsModule\` | 2 | 172 | no | C# latest (11.0) |
| UnityEngine.TextCoreModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.TextCoreModule\` | 73 | 12,153 | no | C# latest (11.0) |
| UnityEngine.TextRenderingModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.TextRenderingModule\` | 16 | 1,359 | no | C# latest (11.0) |
| UnityEngine.TilemapModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.TilemapModule\` | 12 | 1,303 | no | C# latest (11.0) |
| UnityEngine.TLSModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.TLSModule\` | 1 | 139 | no | C# latest (11.0) |
| UnityEngine.UI | Unity | 1.0.0.0 | n | `Unity\UnityEngine.UI\` | 114 | 18,695 | no | C# latest (11.0) |
| UnityEngine.UIElementsModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.UIElementsModule\` | 460 | 45,285 | no | C# latest (11.0) |
| UnityEngine.UIModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.UIModule\` | 9 | 919 | no | C# latest (11.0) |
| UnityEngine.UmbraModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.UmbraModule\` | 1 | 139 | no | C# latest (11.0) |
| UnityEngine.UNETModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.UNETModule\` | 44 | 3,529 | no | C# latest (11.0) |
| UnityEngine.UnityAnalyticsModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.UnityAnalyticsModule\` | 12 | 1,734 | no | C# latest (11.0) |
| UnityEngine.UnityConnectModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.UnityConnectModule\` | 3 | 258 | no | C# latest (11.0) |
| UnityEngine.UnityTestProtocolModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.UnityTestProtocolModule\` | 1 | 139 | no | C# latest (11.0) |
| UnityEngine.UnityWebRequestAssetBundleModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.UnityWebRequestAssetBundleModule\` | 3 | 273 | no | C# latest (11.0) |
| UnityEngine.UnityWebRequestAudioModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.UnityWebRequestAudioModule\` | 4 | 281 | no | C# latest (11.0) |
| UnityEngine.UnityWebRequestModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.UnityWebRequestModule\` | 17 | 2,463 | no | C# latest (11.0) |
| UnityEngine.UnityWebRequestTextureModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.UnityWebRequestTextureModule\` | 3 | 235 | no | C# latest (11.0) |
| UnityEngine.UnityWebRequestWWWModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.UnityWebRequestWWWModule\` | 4 | 627 | no | C# latest (11.0) |
| UnityEngine.VehiclesModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.VehiclesModule\` | 3 | 456 | no | C# latest (11.0) |
| UnityEngine.VFXModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.VFXModule\` | 23 | 1,980 | no | C# latest (11.0) |
| UnityEngine.VideoModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.VideoModule\` | 17 | 1,287 | no | C# latest (11.0) |
| UnityEngine.VRModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.VRModule\` | 39 | 1,864 | no | C# latest (11.0) |
| UnityEngine.WindModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.WindModule\` | 3 | 202 | no | C# latest (11.0) |
| UnityEngine.XRModule | Unity | 0.0.0.0 | n | `Unity\UnityEngine.XRModule\` | 38 | 2,656 | no | C# latest (11.0) |

## Game: `GameData\` plugins

| Assembly | Origin | Version | Obfuscated | Decompiled path | .cs files | Lines | Buildable | Notes |
|---|---|---|---|---|---|---|---|---|
| KSPSteamCtrlr | Squad | 0.0.1.35 | n | `KSPSteamCtrlr\` | 7 | 481 | no | C# 7.3 |

## Launcher: `Launcher_Data\Managed\`

| Assembly | Origin | Version | Obfuscated | Decompiled path | .cs files | Lines | Buildable | Notes |
|---|---|---|---|---|---|---|---|---|
| Assembly-CSharp | Squad | 0.0.0.0 | n | `Launcher\Assembly-CSharp\` | 23 | 3,163 | no | C# 7.3; Squad launcher app (news, settings, graphics profiles); readable names |
| Assembly-CSharp-firstpass | Squad | 0.0.0.0 | n | `Launcher\Assembly-CSharp-firstpass\` | 124 | 29,710 | no | C# 7.3; EZ GUI / SpriteManager 2 UI library (third-party content) |
| ICSharpCode.SharpZipLib | third-party | 2.84.0.0 | n | `Launcher\ICSharpCode.SharpZipLib\` | 69 | 11,897 | no | C# 7.3 |
| Mono.Security | Mono | 4.0.0.0 | n | `Launcher\Mono.Security\` | 175 | 27,242 | no | C# latest (11.0) |
| mscorlib | Mono | 4.0.0.0 | n | `Launcher\mscorlib\` | 2,263 | 296,832 | no | C# latest (11.0) |
| netstandard | Mono | 2.0.0.0 | n | `Launcher\netstandard\` | 1 | 2,455 | no | C# latest (11.0) |
| System | Mono | 4.0.0.0 | n | `Launcher\System\` | 1,413 | 161,534 | no | C# latest (11.0) |
| System.ComponentModel.Composition | Mono | 4.0.0.0 | n | `Launcher\System.ComponentModel.Composition\` | 149 | 14,456 | no | C# latest (11.0) |
| System.Configuration | Mono | 4.0.0.0 | n | `Launcher\System.Configuration\` | 111 | 5,384 | no | C# latest (11.0) |
| System.Core | Mono | 4.0.0.0 | n | `Launcher\System.Core\` | 679 | 72,714 | no | C# latest (11.0) |
| System.Data | Mono | 4.0.0.0 | n | `Launcher\System.Data\` | 658 | 149,194 | no | C# latest (11.0) |
| System.Diagnostics.StackTrace | Mono | 4.1.1.0 | n | `Launcher\System.Diagnostics.StackTrace\` | 8 | 163 | no | C# latest (11.0) |
| System.Drawing | Mono | 4.0.0.0 | n | `Launcher\System.Drawing\` | 183 | 17,015 | no | C# latest (11.0) |
| System.EnterpriseServices | Mono | 4.0.0.0 | n | `Launcher\System.EnterpriseServices\` | 106 | 3,301 | no | C# latest (11.0) |
| System.Globalization.Extensions | Mono | 4.1.1.0 | n | `Launcher\System.Globalization.Extensions\` | 5 | 150 | no | C# latest (11.0) |
| System.IO.Compression | Mono | 4.0.0.0 | n | `Launcher\System.IO.Compression\` | 42 | 6,073 | no | C# latest (11.0) |
| System.IO.Compression.FileSystem | Mono | 4.0.0.0 | n | `Launcher\System.IO.Compression.FileSystem\` | 10 | 784 | no | C# latest (11.0) |
| System.Net.Http | Mono | 4.0.0.0 | n | `Launcher\System.Net.Http\` | 59 | 7,681 | no | C# latest (11.0) |
| System.Numerics | Mono | 4.0.0.0 | n | `Launcher\System.Numerics\` | 22 | 9,694 | no | C# latest (11.0) |
| System.Runtime.Serialization | Mono | 4.0.0.0 | n | `Launcher\System.Runtime.Serialization\` | 293 | 65,133 | no | C# latest (11.0) |
| System.Runtime.Serialization.Xml | Mono | 4.1.1.0 | n | `Launcher\System.Runtime.Serialization.Xml\` | 3 | 117 | no | C# latest (11.0) |
| System.ServiceModel.Internals | Mono | 0.0.0.0 | n | `Launcher\System.ServiceModel.Internals\` | 83 | 14,655 | no | C# latest (11.0) |
| System.Transactions | Mono | 4.0.0.0 | n | `Launcher\System.Transactions\` | 49 | 1,927 | no | C# latest (11.0) |
| System.Xml | Mono | 4.0.0.0 | n | `Launcher\System.Xml\` | 851 | 185,929 | no | C# latest (11.0) |
| System.Xml.Linq | Mono | 4.0.0.0 | n | `Launcher\System.Xml.Linq\` | 58 | 10,324 | no | C# latest (11.0) |
| System.Xml.XPath.XDocument | Mono | 4.1.1.0 | n | `Launcher\System.Xml.XPath.XDocument\` | 2 | 41 | no | C# latest (11.0) |
| Unity.TextMeshPro | Unity | 0.0.0.0 | n | `Launcher\Unity.TextMeshPro\` | 113 | 29,933 | no | C# latest (11.0) |
| Unity.Timeline | Unity | 1.0.0.0 | n | `Launcher\Unity.Timeline\` | 80 | 7,949 | no | C# latest (11.0) |
| UnityEngine | Unity | 0.0.0.0 | n | `Launcher\UnityEngine\` | 24 | 2,267 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.AccessibilityModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.AccessibilityModule\` | 2 | 258 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.AIModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.AIModule\` | 32 | 2,650 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.AndroidJNIModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.AndroidJNIModule\` | 14 | 4,021 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.AnimationModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.AnimationModule\` | 100 | 9,681 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.ARModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.ARModule\` | 6 | 374 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.AssetBundleModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.AssetBundleModule\` | 11 | 852 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.AudioModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.AudioModule\` | 44 | 3,079 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.ClothModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.ClothModule\` | 4 | 565 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.ClusterInputModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.ClusterInputModule\` | 3 | 214 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.ClusterRendererModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.ClusterRendererModule\` | 2 | 167 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.CoreModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.CoreModule\` | 732 | 58,564 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.CrashReportingModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.CrashReportingModule\` | 2 | 178 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.DirectorModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.DirectorModule\` | 2 | 385 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.DSPGraphModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.DSPGraphModule\` | 10 | 599 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.GameCenterModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.GameCenterModule\` | 21 | 1,191 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.GridModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.GridModule\` | 3 | 430 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.HotReloadModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.HotReloadModule\` | 1 | 139 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.ImageConversionModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.ImageConversionModule\` | 2 | 266 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.IMGUIModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.IMGUIModule\` | 41 | 10,834 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.InputLegacyModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.InputLegacyModule\` | 15 | 1,320 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.InputModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.InputModule\` | 7 | 336 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.JSONSerializeModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.JSONSerializeModule\` | 2 | 214 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.LocalizationModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.LocalizationModule\` | 2 | 185 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.ParticleSystemModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.ParticleSystemModule\` | 58 | 10,686 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.PerformanceReportingModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.PerformanceReportingModule\` | 2 | 163 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.Physics2DModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.Physics2DModule\` | 53 | 6,508 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.PhysicsModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.PhysicsModule\` | 46 | 5,279 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.ProfilerModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.ProfilerModule\` | 1 | 139 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.ScreenCaptureModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.ScreenCaptureModule\` | 2 | 194 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.SharedInternalsModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.SharedInternalsModule\` | 40 | 813 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.SpriteMaskModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.SpriteMaskModule\` | 2 | 223 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.SpriteShapeModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.SpriteShapeModule\` | 9 | 454 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.StreamingModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.StreamingModule\` | 2 | 165 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.SubstanceModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.SubstanceModule\` | 9 | 564 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.SubsystemsModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.SubsystemsModule\` | 13 | 602 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.TerrainModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.TerrainModule\` | 21 | 3,803 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.TerrainPhysicsModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.TerrainPhysicsModule\` | 2 | 172 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.TextCoreModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.TextCoreModule\` | 73 | 12,153 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.TextRenderingModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.TextRenderingModule\` | 16 | 1,359 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.TilemapModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.TilemapModule\` | 12 | 1,303 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.TLSModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.TLSModule\` | 1 | 139 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.UI | Unity | 1.0.0.0 | n | `Launcher\UnityEngine.UI\` | 114 | 18,695 | no | C# latest (11.0) |
| UnityEngine.UIElementsModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.UIElementsModule\` | 460 | 45,285 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.UIModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.UIModule\` | 9 | 919 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.UmbraModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.UmbraModule\` | 1 | 139 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.UNETModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.UNETModule\` | 44 | 3,529 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.UnityAnalyticsModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.UnityAnalyticsModule\` | 12 | 1,734 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.UnityConnectModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.UnityConnectModule\` | 3 | 258 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.UnityTestProtocolModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.UnityTestProtocolModule\` | 1 | 139 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.UnityWebRequestAssetBundleModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.UnityWebRequestAssetBundleModule\` | 3 | 273 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.UnityWebRequestAudioModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.UnityWebRequestAudioModule\` | 4 | 281 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.UnityWebRequestModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.UnityWebRequestModule\` | 17 | 2,463 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.UnityWebRequestTextureModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.UnityWebRequestTextureModule\` | 3 | 235 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.UnityWebRequestWWWModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.UnityWebRequestWWWModule\` | 4 | 627 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.VehiclesModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.VehiclesModule\` | 3 | 456 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.VFXModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.VFXModule\` | 23 | 1,980 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.VideoModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.VideoModule\` | 17 | 1,287 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.VRModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.VRModule\` | 39 | 1,864 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.WindModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.WindModule\` | 3 | 202 | no | C# latest (11.0); byte-identical to game copy |
| UnityEngine.XRModule | Unity | 0.0.0.0 | n | `Launcher\UnityEngine.XRModule\` | 38 | 2,656 | no | C# latest (11.0); byte-identical to game copy |

## Native (not .NET)

| Assembly | Origin | Version | Obfuscated | Decompiled path | .cs files | Lines | Buildable | Notes |
|---|---|---|---|---|---|---|---|---|
| KSP_x64.exe | native | - | - | native, not decompilable | - | - | n/a | Unity player executable (game) |
| KSP_x64_Data\Plugins\x86_64\lingoona_grammar_kerbal.dll | native | - | - | native, not decompilable | - | - | n/a | Lingoona grammar engine (localization), used via Lingoona wrapper in firstpass |
| KSP_x64_Data\Plugins\x86_64\steam_api64.dll | native | - | - | native, not decompilable | - | - | n/a | Steamworks SDK (used via Steamworks.NET in Assembly-CSharp-firstpass) |
| Launcher.exe | native | - | - | native, not decompilable | - | - | n/a | Unity player executable (launcher) |
| MonoBleedingEdge\EmbedRuntime\mono-2.0-bdwgc.dll | native | - | - | native, not decompilable | - | - | n/a | Mono runtime (MonoBleedingEdge, Boehm GC) |
| MonoBleedingEdge\EmbedRuntime\MonoPosixHelper.dll | native | - | - | native, not decompilable | - | - | n/a | native helper for Mono.Posix |
| UnityCrashHandler64.exe | native | - | - | native, not decompilable | - | - | n/a | Unity crash handler |
| UnityPlayer.dll | native | - | - | native, not decompilable | - | - | n/a | Unity 2019.4.18f1 engine runtime (C++) |

## Where to find things (core gameplay code for a UE5 port)

All paths are relative to `KSP-Decompiled\`. KSP's own gameplay classes are almost all in the **global namespace**, so they sit directly in `Assembly-CSharp\` (1,394 files at the top level); sub-namespaces map to sub-directories. Line counts are for the current (fixed, buildable) tree.

| Topic | Path | Files | Lines |
|---|---|---|---|
| Part (physics object, part tree) | `Assembly-CSharp\Part.cs` | 1 | 9,023 |
| Vessel | `Assembly-CSharp\Vessel.cs` | 1 | 4,664 |
| PartModule base | `Assembly-CSharp\PartModule.cs` | 1 | 1,256 |
| PartJoint (ConfigurableJoint wrapper) | `Assembly-CSharp\PartJoint.cs` | 1 | 500 |
| Orbit (Keplerian elements, propagation) | `Assembly-CSharp\Orbit.cs` | 1 | 1,854 |
| OrbitDriver (rails / physics sync) | `Assembly-CSharp\OrbitDriver.cs` | 1 | 363 |
| PatchedConicSolver | `Assembly-CSharp\PatchedConicSolver.cs` | 1 | 377 |
| Krakensbane (velocity frame shift) | `Assembly-CSharp\Krakensbane.cs` | 1 | 209 |
| FloatingOrigin | `Assembly-CSharp\FloatingOrigin.cs` | 1 | 368 |
| FlightIntegrator (aero, thermal, drag) | `Assembly-CSharp\FlightIntegrator.cs` | 1 | 2,776 |
| TimeWarp | `Assembly-CSharp\TimeWarp.cs` | 1 | 865 |
| PQS (procedural quad-sphere terrain) | `Assembly-CSharp\PQS.cs` | 1 | 3,168 |
| CelestialBody | `Assembly-CSharp\CelestialBody.cs` | 1 | 1,231 |
| FlightGlobals | `Assembly-CSharp\FlightGlobals.cs` | 1 | 2,003 |
| ConfigNode (cfg parser/serializer) | `Assembly-CSharp\ConfigNode.cs` | 1 | 4,344 |
| GameEvents | `Assembly-CSharp\GameEvents.cs` | 1 | 1,058 |
| ShipConstruct | `Assembly-CSharp\ShipConstruct.cs` | 1 | 971 |
| EditorLogic (VAB/SPH) | `Assembly-CSharp\EditorLogic.cs` | 1 | 5,490 |
| PQS mods (terrain generators) | `Assembly-CSharp\PQSMod*.cs` (66 files, global namespace) | 66 | 6,400 |
| other PQS* support classes (PQSCity, PQSLandControl, quads, cache, ...) | `Assembly-CSharp\PQS*.cs` minus the above | 15 | 4,665 |
| Engines | `Assembly-CSharp\ModuleEngines.cs` | 1 | 2,217 |
| Engines | `Assembly-CSharp\ModuleEnginesFX.cs` | 1 | 191 |
| Wheels (base) | `Assembly-CSharp\ModuleWheelBase.cs` | 1 | 1,125 |
| Wheels (submodules, namespace ModuleWheels) | `Assembly-CSharp\ModuleWheels\` | 14 | 3,463 |

| Namespace / subsystem | Path | Files | Lines | Notes |
|---|---|---|---|---|
| CommNet (relay network) | `Assembly-CSharp\CommNet\` | 33 | 3,980 | Squad |
| Contracts | `Assembly-CSharp\Contracts\` | 64 | 13,117 | Squad |
| FinePrint (stock contract types) | `Assembly-CSharp\FinePrint\` | 70 | 19,909 | Squad; originally a mod by Arsonide |
| Expansions (all) | `Assembly-CSharp\Expansions\` | 482 | 83,623 | Squad DLC code compiled into the base game |
| Expansions\Missions = Making History | `Assembly-CSharp\Expansions\Missions\` | 441 | 66,585 | mission editor/runtime |
| Expansions\Serenity = Breaking Ground | `Assembly-CSharp\Expansions\Serenity\` | 37 | 16,197 | robotics, deployed science |
| KSP\UI | `Assembly-CSharp\KSP\UI\` | 497 | 76,371 | Squad UI |
| VehiclePhysics | `Assembly-CSharp\VehiclePhysics\` | 99 | 17,083 | third-party embedded: Edy's Vehicle Physics (drivetrain, tire/suspension model); used by KSPWheelController.cs, ModuleWheelBase.cs, Vessel.cs |
| EdyCommonTools | `Assembly-CSharp\EdyCommonTools\` | 44 | 6,962 | third-party embedded: Edy's common tools |
| LibNoise | `Assembly-CSharp\LibNoise\` | 45 | 2,762 | third-party embedded: LibNoise (C# port), used by PQS |
| TMPro | `Assembly-CSharp\TMPro\` | 121 | 27,094 | third-party embedded: TextMesh Pro |
| Smooth | `Assembly-CSharp\Smooth\` | 104 | 11,868 | third-party embedded: Smooth.Foundations (Smooth.Algebraics, Smooth.Slinq, ...) |
| SoftMasking | `Assembly-CSharp\SoftMasking\` | 22 | 2,427 | third-party embedded: Soft Mask for Unity UI |
| UniLinq | `Assembly-CSharp\UniLinq\` | 13 | 3,642 | third-party embedded: LINQ reimplementation (AOT-safe) |
| Highlighting | `Assembly-CSharp\Highlighting\` | 2 | 1,580 | third-party embedded: Highlighting System |

Unity-side API that the gameplay code calls (physics, joints, transforms) is under `Unity\`, e.g. `Unity\UnityEngine.PhysicsModule\UnityEngine\` (Rigidbody, ConfigurableJoint, Physics, colliders), `Unity\UnityEngine.VehiclesModule\UnityEngine\WheelCollider.cs` and `Unity\UnityEngine.CoreModule\UnityEngine\` (Transform, Vector3, Quaternion). KSP's double-precision math types are its own: `Assembly-CSharp\Vector3d.cs`, `Assembly-CSharp\Matrix4x4D.cs`, `Assembly-CSharp\UnityEngine\QuaternionD.cs` (declared in namespace UnityEngine inside Assembly-CSharp), `Assembly-CSharp\Planetarium.cs` (inertial frame, UT). Note these Unity assemblies are thin managed bindings: most members are `extern` / `[MethodImpl(MethodImplOptions.InternalCall)]` into the native engine (UnityPlayer.dll, incl. PhysX), so the decompiled Unity code documents the API surface, not the engine behaviour.
