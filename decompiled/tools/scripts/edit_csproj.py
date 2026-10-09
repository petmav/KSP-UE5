"""Apply the guide's section 3.2 csproj edits to ILSpy's legacy-format csproj.

- OutputType -> Library, LangVersion 7.3 (ILSpy 8.2 already emits both; asserted/forced here)
- Remove ILSpy's own <Reference> ItemGroup entirely
- After Microsoft.CSharp.targets import: import KSPBuildTools KSPCommon.targets, then the guide's
  reference block (ManagedPath -> KSPBT_ManagedPath, the property name in current KSPBuildTools).
All <Compile Include> items are kept untouched.
"""
import re, sys
path = sys.argv[1]
src = open(path, encoding="utf-8-sig", newline="").read()
nl = "\r\n" if "\r\n" in src else "\n"
n_compile_before = src.count("<Compile Include=")

src = re.sub(r"<OutputType>\w+</OutputType>", "<OutputType>Library</OutputType>", src)
assert "<LangVersion>7.3</LangVersion>" in src

# Remove the ItemGroup whose children are all <Reference> items
m = re.search(r"[ \t]*<ItemGroup>\r?\n(?:[ \t]*<Reference[^>]*/>\r?\n|[ \t]*<Reference[^>]*>\r?\n(?:[ \t]*<HintPath>[^<]*</HintPath>\r?\n)?[ \t]*</Reference>\r?\n)+[ \t]*</ItemGroup>\r?\n", src)
assert m, "reference ItemGroup not found"
removed = m.group(0).count("<Reference ")
src = src[:m.start()] + "  <!-- ItemGroup References was removed entirely here (guide 3.2) -->" + nl + src[m.end():]
assert "<Reference " not in src

anchor = r'<Import Project="$(MSBuildToolsPath)\Microsoft.CSharp.targets" />'
assert src.count(anchor) == 1
block = nl.join([
 anchor,
 r'  <!-- Everything below this was added (guide 3.2) -->',
 r"  <!-- Current KSPBuildTools (1.1.1) submodule import line from its docs; resolves to the same file as the guide's $(SolutionDir)KSPBuildTools\KSPCommon.targets -->",
 r'  <Import Project="$(MSBuildThisFileDirectory)../KSPBuildTools/KSPCommon.targets" />',
 r'  <ItemGroup>',
 r'    <!-- KSPBuildTools adds $(KSPBT_ManagedPath)/Assembly-CSharp.dll; we are building that assembly, so remove it -->',
 r'    <Reference Remove="$(KSPBT_ManagedPath)/Assembly-CSharp.dll" />',
 r'    <Reference Include="KSPTrackIR">',
 r'      <HintPath>$(KSPBT_ManagedPath)\KSPTrackIR.dll</HintPath>',
 r'    </Reference>',
 r'    <Reference Include="Assembly-CSharp-firstpass">',
 r'      <HintPath>$(KSPBT_ManagedPath)\Assembly-CSharp-firstpass.dll</HintPath>',
 r'    </Reference>',
 r'    <Reference Include="System.Core" />',
 r'    <Reference Include="KSPAssets">',
 r'      <HintPath>$(KSPBT_ManagedPath)\KSPAssets.dll</HintPath>',
 r'    </Reference>',
 r'    <Reference Include="Mono.Cecil">',
 r'      <HintPath>$(KSPBT_ManagedPath)\Mono.Cecil.dll</HintPath>',
 r'    </Reference>',
 r'    <Reference Include="Unity.Analytics.StandardEvents">',
 r'      <HintPath>$(KSPBT_ManagedPath)\Unity.Analytics.StandardEvents.dll</HintPath>',
 r'    </Reference>',
 r'    <Reference Include="Ionic.Zip">',
 r'      <HintPath>$(KSPBT_ManagedPath)\Ionic.Zip.dll</HintPath>',
 r'    </Reference>',
 r'    <Reference Include="Unity.Analytics.DataPrivacy">',
 r'      <HintPath>$(KSPBT_ManagedPath)\Unity.Analytics.DataPrivacy.dll</HintPath>',
 r'    </Reference>',
 r'  </ItemGroup>',
])
src = src.replace(anchor, block)
assert src.count("<Compile Include=") == n_compile_before
open(path, "w", encoding="utf-8", newline="").write(src)
print(f"removed {removed} ILSpy references; kept {n_compile_before} Compile items")
