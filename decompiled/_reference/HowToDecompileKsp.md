
Decompiling KSP is technically against the Take2 EULA, but having the KSP Code to reference is an invaluable tool for modding. Hence it's been a bit of an open secret that modders decompile the game to help inform their mod writing. Sadly, nobody has ever fully captured how this is done, so this is what I intend to do here. 
If you wish to read the Take2 Eula before following this guide, you can find it [here](https://www.take2games.com/legal/en-US/).
<details><summary>Notes on Legality</summary>
	
Preface: I am not a lawyer, this is not legal advice, etc. From the research I have done on the topic, I have been able to gather this:

In US and EU legislature it is legal to decompile any program regardless of license restrictions if the purpose of the decompilation is done to enable interoperability ([ref1](https://www.gerrishlegal.com/blog/2020/04/16/2020-4-7-reverse-engineering-when-can-users-lawfully-decompile-software)). In the case of KSP I would say this is not given as KSP does provide a public interface as the modding API. In the US decompilation in this regard seems to be more generally allowed under "Fair Use".
	
In US and EU legilsature it is also legal to decompile any program regardless of license restroctions for the purpose of fixing errors with the program ([ref2](https://www.lexology.com/library/detail.aspx?g=f5b1193c-f423-4f96-bca5-03f5145ecf15)),([ref3](https://bg.legal/en/when-are-you-allowed-to-decompile-software/)). I would argue that KSP modding falls under this ruling, especially if you intend to use the knowledge gained to contribute to a project such as [KSP Community Fixes](https://github.com/KSPModdingLibs/KSPCommunityFixes), which has the explicit purpose of fixing errors preventing the use of the program as it was intended.
	
Furthermore I would argue that if KSP is decompiled for the purpose of modding, this is done in good faith. The code is not published and is only used to develop extensions of the program which **increase** it's sale value, perhaps convincing more people to buy the game. In which case if there was a push to sue for damages, those would be negative. 
 </details>

<details><summary>Personal Comment</summary>
	
Considering that KSP is a completely dead game, and that KSP2 has been abandoned with zero communication, I personally do not think Take2 deserves any respect, seeing how much they have disrespected everyone else. A good mod would also incur no damages to Take2, if anything it would increase sales, of which zero percent ends up with KSP developers, because there are none.
	
![image](https://imgs.xkcd.com/comics/faust_20.png)
 </details>

This guide assumes you have read and followed my [KSP modding starter guide](https://gist.github.com/HB-Stratos/f95fd86264013f82c66e6cd344452dbf).

# 1. Deobfuscate Assembly-CSharp.dll with de4dot
Kerbal Space Program has not only been compiled with Unity, it has also been obfuscated to make decompilaton harder. Thankfully, there are tools around that.
## 1.1 Compiling de4dot
We will be using de4dot as our obfuscator. The project is no longer maintained and you will have to compile it yourself, but it does work well. 
<details> <summary>Side Note</summary>
	
  https://github.com/kant2002/de4dot/ appears to be an active fork of de4dot which offers readily built Releases. I have not tested these and can not vouch for their quality or function, but it may save you some work. Maybe the deobfuscator even does a better job and creates a better decompilation in the end. As of yet, untested.
 </details>
 
- Open Visual Studio (2022) and select "Clone a repository"
- As the repository path, enter https://github.com/de4dot/de4dot and clone the project. 
- On the right sidebar open the solution explorer and open ``de4dot.netframework.sln``
- At the top, select ``Build`` -> ``Batch Build``. Press ``Select All``, and then ``Build``.
![image](https://gist.github.com/user-attachments/assets/0af20381-481d-4af9-9fee-88255916ba53)

## 1.2 Using de4dot
- At the top, select ``Git`` -> ``Open in File Explorer``
- Navigate to ``de4dot/Release/net45/``, then type CMD into the windows explorer adress bar to open a command prompt in this folder.
    - IMPORTANT: Make sure you are in ``de4dot/Release/...`` and NOT in ``de4dot/de4dot/obj/Release/...``, otherwise the exe will not work.
- run ``de4dot`` in the terminal to see if everything is working.
    - Side Note: if you opt to use PowerShell instead of CMD (default of windows terminal), you need to run ``./de4dot`` instead.
- Locate your KSP Mod Developement Install, then navigate to ``KSPRootPath/KSP_x64_Data/Managed``.
- Shift-RightClick ``Assembly-CSharp.dll`` and select "Copy as Path"
- Run this command in the terminal you opened: ``de4dot --dont-rename "<Paste the Assembly-CSharp.dll File url here>"``

You have now deobfuscated KSP, your deobfuscated file was created next to ``Assembly-CSharp.dll`` and is named ``Assembly-CSharp-cleaned.dll``

# 2. Decompile Assembly-CSharp-cleaned.dll with ILSpy
We have deobfuscated KSP now, but it is still in compiled in .dll form. ILSpy is decompiler for .net code with a few other neat tools.
<details> <summary>Side Note</summary>
	
  Other decompilers for .net also exist. I have tested [dnSpy](https://github.com/dnSpyEx/dnSpy/) and [dotpeek](https://www.jetbrains.com/decompiler/download/#section=portable). Both produce better decompilation results when used. However, they also both produce code with thousands of errors that does not compile, at least not on C#7.3 required by Unity 2019.
 </details>
 
# 2.1 Download ILSpy
- Download the latest (non-preview) **selfcontained** Release from https://github.com/icsharpcode/ILSpy/releases/ 
    - All my testing was done with ILSpy 8.2
- Extract the entire zip file and run ``ILSpy.exe``
# 2.2 Using ILSpy
- With ILSpy open, select ``File`` -> ``Open``, then locate your ``Assembly-CSharp-cleaned.dll`` file from earlier and open it.
- Make sure to select Language ``C#`` and Version ``C# 7.3 / VS2017.7`` for KSP 1.12.5
  
  ![image](https://gist.github.com/user-attachments/assets/8b5bfafe-d93c-45b4-b350-82dd972f2082)

- Select ``View`` -> ``Options`` -> ``Decompiler`` and scoll down to ``Project export``. Uncheck "Use new SDK style format for generated projeft files. This makes sure you are generating a project in the usual library format. If you do not do this you will run into a lot of issues trying to integrate KSPBuildTools later.
- In the same menu also check "Use nested directories for namespaces"
- Create a folder for all your decompiled files. This will be your Project folder, so consider creating it where your Visual Studio projects live. By default that is ``User Home/source/Repos/``, where I created ``KSP-Decompiled/Assembly-CSharp"``. The nested folders mirror the structure of normal KSP mods, so I would recommend you use this too. 
- In ILSpy, right-click your newly opened ``Assembly-CSharp-cleaned`` and select ``Save Code``
- Select your project folder ``KSP-Decompiled/Assembly-CSharp`` and select ``Save``
    - **WARNING**: This will look like you are only saving a single csproj file, but in reality ILSpy will use the path you selected to write all decompiled files, and there are hundreds of them. Make sure you have you are saving the code to an empty directory.

You now have a fully decompiled version of KSP, ready to be imported into Visual Studio.

# 3. Importing into Visual Stuido
You should already have Visual Studio Community 2022 installed from my previous guide on how to write a basic mod for KSP. 
## 3.1 Loading the Project
- Open Visual Studio and select ``Open a project or solution``. Find where you saved the code in the previous step and load the ``Assembly-CSharp-cleaned.csproj`` file.
- Select ``File`` -> ``Save All`` and in the popup save your ``Assembly-CSharp-cleaned.sln`` file into the folder **above** ``Assembly-CSharp/``. If you followed my naming, this will be ``User Home/source/Repos/KSP-Decompiled/``. For KSP build tools it will be much more convenient if the solution is not saved in the ``Assembly-CSharp`` Folder.
- Close and restart Visual Studio, and as above select your project, only this time make sure to load the ``.sln`` you just saved instead of the ``.csproj``
- Navigate to ``Git Changes`` on the bottom right and then ``Create Git Repository``. You can make this local or push it to github. Just make sure it is private when pushing to github, otherwise you would be publishing copyrighted code. 
## 3.2 Adding KSPBuildTools
- Locate your project folder (the one with the .sln) in Windows Explorer. You should already have Git installed from the previous tutorial, so right click and select ``Git Bash Here``
- Get the KSP Build Tools install command from https://github.com/KSPModdingLibs/KSPBuildTools . This will likely be ``git submodule add https://github.com/KSPModdingLibs/KSPBuildTools.git`` and run the command in your git bash shell.
- In Visual Studio right click your project (under the solution) in the solution exploerer, and select ``Unload Project``
- Right click again and select ``Edit Project File``
- This file will need several edits: 
    - At the very bottom, under ``<Import Project="$(MSBuildToolsPath)\Microsoft.CSharp.targets" />`` but before ``</Project>`` add the include file for KSPBuildTools. You can find the most up to date version on their github page. At the time of writing that is ``<Import Project="$(SolutionDir)KSPBuildTools\KSPCommon.targets" />``
    - Change the ``<OutputType>`` from ``Exe`` to ``Library``
    - At the time of writing KSP Build Tools brings the references:
```xml
<ItemGroup>
    <Reference Include="$(ManagedPath)\System.dll">
      <Name>System (KSP/Mono)</Name>
      <Private>False</Private>
    </Reference>
    <Reference Include="$(ManagedPath)\mscorlib.dll">
      <Name>System.Core (KSP/Mono)</Name>
      <Private>False</Private>
    </Reference>
    <Reference Include="$(ManagedPath)\System.Xml.dll">
      <Name>System.Xml (KSP/Mono)</Name>
      <Private>False</Private>
    </Reference>
    <Reference Include="$(ManagedPath)\UnityEngine*.dll">
      <Name>UnityEngine</Name>
      <Private>False</Private>
    </Reference>
    <Reference Include="$(ManagedPath)\Assembly-CSharp.dll">
      <Name>Assembly-CSharp</Name>
      <Private>False</Private>
    </Reference>
    <Reference Include="$(ManagedPath)\Assembly-CSharp-firstpass.dll">
      <Name>Assembly-CSharp-firstpass</Name>
      <Private>False</Private>
    </Reference>
  </ItemGroup>
```
Hence we do not need to include any of these ourselves. We however do not want to reference Assembly-CSharp.dll, so we will need to remove that and all double references. We will also change all paths to use the ``$(ManagedPath)`` variable. We must also make sure the new ``Reference`` block comes after importing KSPBuildTools.
After all these changes, your project file should look something like this
```xml
<?xml version="1.0" encoding="utf-8"?>
<Project ToolsVersion="4.0" DefaultTargets="Build" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <PropertyGroup>
    <ProjectGuid>{F7BB68E1-16AC-46C3-9FA6-81435CA41256}</ProjectGuid>
    <ProjectTypeGuids>{FAE04EC0-301F-11D3-BF4B-00C04F79EFBC}</ProjectTypeGuids>
    <Configuration Condition=" '$(Configuration)' == '' ">Debug</Configuration>
    <Platform Condition=" '$(Platform)' == '' ">AnyCPU</Platform>
<!--This was changed-->
    <OutputType>Library</OutputType>    
    <LangVersion>7.3</LangVersion>
    <AssemblyName>Assembly-CSharp</AssemblyName>
    <TargetFrameworkIdentifier>.NETFramework</TargetFrameworkIdentifier>
    <TargetFrameworkVersion>v4.0</TargetFrameworkVersion>
    <WarningLevel>4</WarningLevel>
    <AllowUnsafeBlocks>True</AllowUnsafeBlocks>
  </PropertyGroup>
  <PropertyGroup Condition=" '$(Platform)' == 'AnyCPU' ">
    <PlatformTarget>AnyCPU</PlatformTarget>
  </PropertyGroup>
  <PropertyGroup Condition=" '$(Configuration)' == 'Debug' ">
    <OutputPath>bin\Debug\</OutputPath>
    <DebugSymbols>true</DebugSymbols>
    <DebugType>full</DebugType>
    <Optimize>false</Optimize>
  </PropertyGroup>
  <PropertyGroup Condition=" '$(Configuration)' == 'Release' ">
    <OutputPath>bin\Release\</OutputPath>
    <DebugSymbols>true</DebugSymbols>
    <DebugType>pdbonly</DebugType>
    <Optimize>true</Optimize>
  </PropertyGroup>
<!-- ItemGroup References was removed entirely here -->
  <ItemGroup>
    <Compile Include="AbundanceRequest.cs" />
[... most compiles excluded for brevity, you must keep all of them in your file ...]
    <Compile Include="XSelectable.cs" />
  </ItemGroup>
  <Import Project="$(MSBuildToolsPath)\Microsoft.CSharp.targets" />
<!-- Everything below this was added -->
  <Import Project="$(SolutionDir)KSPBuildTools\KSPCommon.targets" />
	<ItemGroup>
		<Reference Remove="$(ManagedPath)\Assembly-CSharp.dll" />
		<Reference Include="KSPTrackIR">
			<HintPath>$(ManagedPath)\KSPTrackIR.dll</HintPath>
		</Reference>
		<Reference Include="Assembly-CSharp-firstpass">
			<HintPath>$(ManagedPath)\Assembly-CSharp-firstpass.dll</HintPath>
		</Reference>
		<Reference Include="System.Core" />
		<Reference Include="KSPAssets">
			<HintPath>$(ManagedPath)\KSPAssets.dll</HintPath>
		</Reference>
		<Reference Include="Mono.Cecil">
			<HintPath>$(ManagedPath)\Mono.Cecil.dll</HintPath>
		</Reference>
		<Reference Include="Unity.Analytics.StandardEvents">
			<HintPath>$(ManagedPath)\Unity.Analytics.StandardEvents.dll</HintPath>
		</Reference>
		<Reference Include="Ionic.Zip">
			<HintPath>$(ManagedPath)\Ionic.Zip.dll</HintPath>
		</Reference>
		<Reference Include="Unity.Analytics.DataPrivacy">
			<HintPath>$(ManagedPath)\Unity.Analytics.DataPrivacy.dll</HintPath>
		</Reference>
	</ItemGroup>
</Project>

```
With those changes complete, save your ``csproj`` file and reload your project.
- In the solution explorer, double click ``Properties``, then ``Reference Paths`` and add the base path to your KSP Mod Dev Install. Make sure to save your Properties.

## 3.3 Fixing Decompilation Artifacts, First Pass.
- Select ``Build`` -> ``Build Solution``. You will see a few errors we need to fix:
![image](https://gist.github.com/user-attachments/assets/3ddeb2fa-2e9a-4bba-9c92-ac0f115341f1)
    - All errors that say ``[...] explicit method implementation cannot implement [...] because it is an accessor`` can be fixed in the same way: <br>
    Delete the offending method, including the ``[SpecialName]`` attribute above it. 
    ![image](https://gist.github.com/user-attachments/assets/2bde46d4-b9b9-4ec6-8700-9ead2ef1847b)
    While deleting, it can help to use ``Build`` -> ``Rebuild Solution`` every now and then as the error messages can get a bit wonky.       Right clicking on your open Tabs and using "Close all Tabs" also comes in handy.
    - After fixing these issues, only two more remained for me:
    ![image](https://gist.github.com/user-attachments/assets/2dc17e26-ca5b-40cb-bf46-9d58919ea8d6)
    - The error ``'_0003': member names cannot be the same as their enclosing type`` can be fixed by simply deleting the extraneous code. <br>
    ![image](https://gist.github.com/user-attachments/assets/208288b8-2396-464a-92b6-406c835245ff)
    - The error ``'DesignConcernBase' does not implement interface member 'IDesignConcern.testResultChanged'`` requires a bit more attention. <br>
    ![image](https://gist.github.com/user-attachments/assets/368748c5-1ef1-4377-badb-fc81555cc901) <br>
    must be changed to <br>
    ![image](https://gist.github.com/user-attachments/assets/da25c3fd-a8e8-48b8-980b-fb008576522d)<br>
- With that all errors in the console should be gone (until you select rebuild) and the first pass of fixing decompilation errors is complete.
## 3.2 Making it compile 
- There may be one to a few more errors to work out:
	- The error ``Operator '!=' cannot be applied to operands of type 'bool' and 'int'`` can be fixed by changing ``0`` to ``false``<br>
	![image](https://gist.github.com/user-attachments/assets/eb081b57-9c27-461f-816c-ad3106725280)
	- If it occurs, the errors ``Cannot implicitly convert type 'T' to 'Smooth.Algebraics.Option<T>'`` can be fixed like this:
	![image](https://gist.github.com/user-attachments/assets/0db31132-e94a-49f5-ae91-5faa6bfe186b) ![image](https://gist.github.com/user-attachments/assets/b4f9919b-6b56-48ad-a329-4263b2175bd3) <br>
	This error only occurrent when I used ILSpy9.0Preview2 instead of ILSpy8.2
- Unless you got unlucky and more things broke (sorry, you'll be on your own for those), your build should now compile.
- Open the ``Git Changes`` panel again and commit your changes so you have information about how you did this for future reference.
You can now compile a custom ``Assembly-CSharp.dll`` with ``.pdb`` debug information for KSP.

# 4. Load KSP with the custom compiled Assembly
To be able to load KSP with our DLL, we once again need to symlink the DLL into the game folder. However, this time we need to symlink the .dll and .pdb separately as they live in a directory with many other files. 
- Select at the top ``Git`` -> ``Open in File Explorer``, then navigate to ``KSP-Decompiled/GameData/Assembly-CSharp-cleaned``
- Shift-RightClick ``Assembly-CSharp.dll`` and select ``Copy as Path``
- Open an Administrator CMD shell and with ``cd`` navigate to your KSP Mod Dev Installation folder, then ``KspRootPath/KSP_x64_Data/Managed``
- From the Terminal or in File Explorer delete the original ``Assembly-CSharp.dll`` or save it somewhere outside the KSP folder. Also delete / move the ``Assembly-CSharp-cleaned.dll`` we generated earlier.
- In the Terminal, run (making sure to enter the correct path) <br>``mklink Assembly-CSharp.dll "<PathToYourVsRepos>\KSP-Decompiled\GameData\Assembly-CSharp-cleaned\Assembly-CSharp.dll"``. Then, do the same again with the ``.pdb`` file. <br>``mklink Assembly-CSharp.pdb "<PathToYourVsRepos>\KSP-Decompiled\GameData\Assembly-CSharp-cleaned\Assembly-CSharp.pdb"``

Your Assemblies are now correctly linked. As you are using KSPBuildTools, you can now press F5 to launch the game. You can even use ``Debug`` -> ``Attach Unity Debugger`` to debug the game. 

If you are using KSPCommunityFixes, you have to disable a few of it's patches with your custom compiled version. Create a file named ``kspcf.cfg`` in your GameData folder and add the below content. Also make sure you have ModuleManager present to apply the patches, this should have been installed with KSPCF anyways.
```
// these patches don't work with the debug build
@KSP_COMMUNITY_FIXES
{
	@SceneLoadSpeedBoost = false
	@DoubleCurvePreserveTangents = false
	@ModuleIndexingMismatch = false
	@RefundingOnRecovery = false
}
```
Also be aware that when using KSPCF it can occasionally happen that a breakpoint you set in game code will not be hit because KSPCF occasionally uses Harmony patches to redirect stock functions to their improved versions. 
# 5. Outlook
Once again, this is as far as I've gotten. You can now read the KSP code, breakpoint in it and examine how it exactly it works. So far I have not figured out how to combine the decompiled code with a normal Visual Studio mod compiling setup. I will ammend this post if I figure out how

If you want to take the game apart further, there's https://github.com/Perfare/AssetStudio (archived) or https://github.com/aelurum/AssetStudio (active fork). With these tools you can explore and export almost all assets from KSP and inspect how they are made and how scenes are structured. The exported assets will have some significant issues, especially around animations, but for viewing and analyzing they are sufficient. 

I have also heared that there has been some effort to take the decompilation and asset extraction to the next level: Building a fully functional version of KSP in the Unity Editor again. I do not know who has worked on this project, or how far it has come. In my opinion this should definitely be an avenue to explore for the future. A full editor version of KSP would be a very powerful tool. 

### Credits
Kind help provided by:
- JonnyOThan on discord
- Sofiebrink on discord
- Halban on discord for notifying me of the caveat with KSPCF

All screenshots are my own.