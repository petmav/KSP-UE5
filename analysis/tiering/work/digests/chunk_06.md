# Review chunk 06 (KSP): 264 files, 39329 lines

## Assembly-CSharp/KSP/FX/Fireworks/FireworkEffectType.cs
- 8 lines; kind=enum; type=FireworkEffectType; bases=-; ancestry=-; ns=KSP.FX.Fireworks
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=4 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/KSP/FX/Fireworks/FireworkFX.cs
- 182 lines; kind=MonoBehaviour; type=FireworkFX; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.FX.Fireworks
- usings: UnityEngine
- unity: GetComponent=8, GameObject=7, Transform=5, Audio=4, Instantiate/Destroy=3, Rigidbody=2; unity_score=79
- ksp refs: GameDatabase=1; fan_in=2 fan_out=3
- methods(5): FixedUpdate, GetRBVelocity, OnCollisionEnter, Setup, playAudioFX
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/KSP/FX/Fireworks/FireworkFXComponent.cs
- 286 lines; kind=MonoBehaviour; type=FireworkFXComponent; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.FX.Fireworks
- usings: UnityEngine
- unity: ParticleSystem=25, Transform=4, Coroutine=4, Lifecycle=3, GameObject=2, Audio=2; unity_score=77
- ksp refs: FlightGlobals=11; fan_in=2 fan_out=5
- methods(11): ActivateBurstPS, Die, Initialize, LateUpdate, OnDestroy, PlaySFXOnParticleDeath, Update, configureParticleSystems, createColorGradient, particleSystemsPlaying, setParticlesCBGravity
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/KSP/FX/Fireworks/FireworkFXDefinition.cs
- 83 lines; kind=class; type=FireworkFXDefinition; bases=-; ancestry=-; ns=KSP.FX.Fireworks
- usings: UnityEngine
- unity: none; unity_score=30
- ksp refs: ConfigNode=2; fan_in=2 fan_out=3
- methods(2): Load, Save
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/KSP/FX/Fireworks/FireworkFXList.cs
- 58 lines; kind=class; type=FireworkFXList; bases=IEnumerable; ancestry=IEnumerable; ns=KSP.FX.Fireworks
- usings: UnityEngine
- unity: none; unity_score=22
- ksp refs: ConfigNode=1; fan_in=1 fan_out=2
- methods(4): Add, Contains, Count, GetEnumerator
- auto suggestion: tier=C subsystem=Rendering

## Assembly-CSharp/KSP/IO/BinaryReader.cs
- 292 lines; kind=class; type=BinaryReader; bases=IDisposable; ancestry=IDisposable; ns=KSP.IO
- usings: -
- unity: none; unity_score=0
- ksp refs: Vessel=1; fan_in=0 fan_out=3
- methods(21): Close, CreateForType, Dispose, PeekChar, Read, ReadBoolean, ReadByte, ReadBytes, ReadChar, ReadChars, ReadDecimal, ReadDouble, ReadInt16, ReadInt32, ReadInt64, ReadSByte, ReadSingle, ReadString, ReadUInt16, ReadUInt32, ReadUInt64
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/KSP/IO/BinaryWriter.cs
- 299 lines; kind=class; type=BinaryWriter; bases=IDisposable; ancestry=IDisposable; ns=KSP.IO
- usings: -
- unity: none; unity_score=0
- ksp refs: Vessel=1; fan_in=0 fan_out=4
- methods(6): Close, CreateForType, Dispose, Flush, Seek, Write
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/KSP/IO/File.cs
- 278 lines; kind=class; type=File; bases=-; ancestry=-; ns=KSP.IO
- usings: -
- unity: none; unity_score=0
- ksp refs: Vessel=15; fan_in=3 fan_out=6
- methods(15): AppendAllText, AppendText, Create, CreateText, Delete, Exists, Open, OpenText, OpenWrite, ReadAllBytes, ReadAllLines, ReadAllText, WriteAllBytes, WriteAllLines, WriteAllText
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/KSP/IO/FileAccess.cs
- 15 lines; kind=enum; type=FileAccess; bases=-; ancestry=-; ns=KSP.IO
- usings: -
- unity: none; unity_score=12
- ksp refs: none; fan_in=2 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/KSP/IO/FileInfo.cs
- 281 lines; kind=class; type=FileInfo; bases=-; ancestry=-; ns=KSP.IO
- usings: -
- unity: none; unity_score=0
- ksp refs: Vessel=3; fan_in=0 fan_out=9
- methods(15): AppendText, CopyTo, Create, CreateForType, CreateText, Decrypt, Delete, Encrypt, MoveTo, Open, OpenRead, OpenText, OpenWrite, Replace, ToString
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/KSP/IO/FileMode.cs
- 15 lines; kind=enum; type=FileMode; bases=-; ancestry=-; ns=KSP.IO
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=3 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/KSP/IO/FileShare.cs
- 18 lines; kind=enum; type=FileShare; bases=-; ancestry=-; ns=KSP.IO
- usings: -
- unity: none; unity_score=12
- ksp refs: none; fan_in=2 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/KSP/IO/FileStream.cs
- 200 lines; kind=class; type=FileStream; bases=IDisposable; ancestry=IDisposable; ns=KSP.IO
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=2
- methods(14): BeginRead, BeginWrite, Dispose, EndRead, EndWrite, Flush, Lock, Read, ReadByte, Seek, SetLength, Unlock, Write, WriteByte
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/KSP/IO/IOException.cs
- 26 lines; kind=class; type=IOException; bases=Exception; ancestry=Exception; ns=KSP.IO
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=5 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/KSP/IO/IOTools.cs
- 24 lines; kind=class; type=IOTools; bases=-; ancestry=-; ns=KSP.IO
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=0 fan_out=1
- methods(2): DeserializeFromBinary, SerializeToBinary
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/KSP/IO/IOUtils.cs
- 165 lines; kind=class; type=IOUtils; bases=-; ancestry=-; ns=KSP.IO
- usings: UnityEngine
- unity: Application.=3; unity_score=7
- ksp refs: Vessel=1, Part=1; fan_in=11 fan_out=9
- methods(12): Cleanup, ConvertAccess, ConvertMode, ConvertSeek, ConvertShare, DeserializeFromBinary, GetFilePathFor, GetFlightDirectories, GetPartPathFor, RemoveFlightData, SerializeToBinary, WrapException
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/KSP/IO/MemoryStream.cs
- 165 lines; kind=class; type=MemoryStream; bases=-; ancestry=-; ns=KSP.IO
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=2
- methods(9): Flush, GetBuffer, Read, ReadByte, Seek, SetLength, ToArray, Write, WriteByte
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/KSP/IO/PluginConfigNode.cs
- 359 lines; kind=class; type=PluginConfigNode; bases=-; ancestry=-; ns=KSP.IO
- usings: UnityEngine
- unity: KeyCode=3, MonoBehaviour=1; unity_score=15
- ksp refs: Vector3d=4; fan_in=1 fan_out=1
- methods(7): GetParent, GetValue, SetValue, StartNamedElement, deserializeXML, handleElement, serializeXML
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/KSP/IO/PluginConfiguration.cs
- 98 lines; kind=class; type=PluginConfiguration; bases=-; ancestry=-; ns=KSP.IO
- usings: -
- unity: none; unity_score=0
- ksp refs: Vessel=1; fan_in=0 fan_out=5
- methods(5): CreateForType, GetValue, SetValue, load, save
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/KSP/IO/SeekOrigin.cs
- 9 lines; kind=enum; type=SeekOrigin; bases=-; ancestry=-; ns=KSP.IO
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=4 fan_out=0
- methods(0): 
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/KSP/IO/TextReader.cs
- 129 lines; kind=class; type=TextReader; bases=IDisposable; ancestry=IDisposable; ns=KSP.IO
- usings: -
- unity: none; unity_score=0
- ksp refs: Vessel=1; fan_in=2 fan_out=3
- methods(8): Close, CreateForType, DiscardBufferedData, Dispose, Peek, Read, ReadLine, ReadToEnd
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/KSP/IO/TextWriter.cs
- 508 lines; kind=class; type=TextWriter; bases=IDisposable; ancestry=IDisposable; ns=KSP.IO
- usings: -
- unity: none; unity_score=0
- ksp refs: Vessel=1; fan_in=2 fan_out=3
- methods(6): Close, CreateForType, Dispose, Flush, Write, WriteLine
- auto suggestion: tier=A subsystem=Save

## Assembly-CSharp/KSP/Localization/FontLangSettings.cs
- 20 lines; kind=class; type=FontLangSettings; bases=-; ancestry=-; ns=KSP.Localization
- usings: TMPro
- unity: TMPro=2; unity_score=48
- ksp refs: none; fan_in=1 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=Localization

## Assembly-CSharp/KSP/Localization/FontSettings.cs
- 82 lines; kind=class; type=FontSettings; bases=-; ancestry=-; ns=KSP.Localization
- usings: TMPro
- unity: TMPro=2; unity_score=25
- ksp refs: none; fan_in=1 fan_out=2
- methods(3): AddSubFont, ChangeLanguage, CleanSubFonts
- auto suggestion: tier=C subsystem=Localization

## Assembly-CSharp/KSP/Localization/Localizer.cs
- 494 lines; kind=class; type=Localizer; bases=-; ancestry=-; ns=KSP.Localization
- usings: CommNet, CompoundParts, Expansions, Expansions.Missions, Expansions.Serenity, FinePrint.Utilities, KSP.UI.Dialogs, KSP.UI.Screens, KSP.UI.Screens.Editor, KSP.UI.Screens.Flight, KSP.UI.Screens.Mapview, KSP.UI.TooltipTypes, KerbNet, Lingoona, ModuleWheels, PreFlightTests, SentinelMission, Strategies, UnityEngine
- unity: none; unity_score=2
- ksp refs: ConfigNode=10, Localizer=4, GameSettings=2, Vessel=1, TimeWarp=1, ProtoVessel=1; fan_in=802 fan_out=131
- methods(20): AddTagValuesForLanguage, AddTagValuesFromNode, AppendMissionTags, Format, FormatWithLingoona, GetLanguageIdFromFile, GetStringByTag, Init, RefreshRuleSet, RefreshTagValues, ReplaceSingleTagIfFound, ReplaceTags, SwitchToLanguage, TranslateBranch, TryGetStringByTag, _CacheLocalStrings, _Format, _SwitchToLanguage, unescapeFormattedString, unescapeStringToLingoona
- auto suggestion: tier=C subsystem=Localization

## Assembly-CSharp/KSP/Testing/TestInfo.cs
- 30 lines; kind=attribute; type=TestInfo; bases=Attribute; ancestry=Attribute; ns=KSP.Testing
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=0
- methods(1): ToString
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/Testing/TestManager.cs
- 71 lines; kind=class; type=TestManager; bases=-; ancestry=-; ns=KSP.Testing
- usings: UnityEngine
- unity: MonoBehaviour=1; unity_score=10
- ksp refs: none; fan_in=3 fan_out=4
- methods(3): AddTest, ClearUnitTests, RunTests
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/Testing/TestResults.cs
- 13 lines; kind=class; type=TestResults; bases=-; ancestry=-; ns=KSP.Testing
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/Testing/TestState.cs
- 13 lines; kind=class; type=TestState; bases=-; ancestry=-; ns=KSP.Testing
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=4 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/Testing/UnitTest.cs
- 127 lines; kind=class; type=UnitTest; bases=-; ancestry=-; ns=KSP.Testing
- usings: UnityEngine
- unity: MonoBehaviour=1; unity_score=6
- ksp refs: Vector3d=4; fan_in=2 fan_out=4
- methods(4): PerformTest, TestStartUp, TestTearDown, assertEquals
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/AlarmClockUIAddPane.cs
- 417 lines; kind=MonoBehaviour; type=AlarmClockUIAddPane; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: KSP.Localization, KSP.UI.Screens, TMPro, UnityEngine, UnityEngine.UI
- unity: SetActive=13, GameObject=13, UnityEngine.UI=10, TMPro=9, Lifecycle=4, MonoBehaviour=1; unity_score=70
- ksp refs: Localizer=4, GameEvents=2, Vessel=1; fan_in=1 fan_out=14
- methods(17): AddAlarm, AddAlarmTypeChanged, AnyTextFieldHasFocus, Awake, BuildDropdownList, CancelAlarm, ClosePane, InputDataChanged, LateUpdate, OnAvailableVesselChanged, OnDestroy, RebuildUI, SetCurrentAlarmValuesFromUI, SetCurrentBaseUIValues, SetupAddAlarm, SetupEditAlarm, Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/AlarmClockUIFrame.cs
- 456 lines; kind=MonoBehaviour; type=AlarmClockUIFrame; bases=GenericAppFrame; ancestry=GenericAppFrame/MonoBehaviour; ns=KSP.UI
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: UnityEngine.UI=12, Transform=6, Lifecycle=4, TMPro=3, Instantiate/Destroy=2, GetComponent=2; unity_score=60
- ksp refs: GameEvents=7, Vessel=4, TimeWarp=3, InputLockManager=3, HighLogic=3, Localizer=2; fan_in=6 fan_out=28
- methods(34): AddAlarmPaneClose, AddAlarmPaneCloseImmediate, AddAlarmPaneOpen, AddInputLock, AnyTextFieldHasFocus, Awake, CreateAlarm, DelayedOpenAddPane, DisableActionButtons, EditAlarm, EnableActionButtons, OnActiveAlarmsToggleChanged, OnActiveVesselToggleChanged, OnAlarmAdded, OnAlarmRemoved, OnAlarmRowSelected, OnAvailableVesselChanged, OnDestroy, OnEdit, OnPointerEnter, OnPointerExit, OnSwitchTo, OnWarpTo, PrintDate, PrintTime, PrintTimeCompact, PrintTimeStampCompact, RebuildList, RemoveInputLock, SetTimeDisplay, Start, ToggleTimeDisplay, Update, UpdateInputLock
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/AlarmUIDisplayMode.cs
- 8 lines; kind=enum; type=AlarmUIDisplayMode; bases=-; ancestry=-; ns=KSP.UI
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=9 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/AnchoredDialog.cs
- 334 lines; kind=MonoBehaviour; type=AnchoredDialog; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: GameObject=13, UnityEngine.UI=7, Transform=6, GetComponent=6, Camera=5, Screen/Cursor=4; unity_score=74
- ksp refs: GameEvents=2; fan_in=4 fan_out=8
- methods(21): AttachEventHooks, Awake, CreatePanel, CreateWindowContent, Disable, GetHover, GetWindowTitle, LateUpdate, OnClickOut, OnDestroy, OnDestroyThis, OnGameSettingsApplied, OnLateUpdate, OnPanelSetupComplete, ReEnable, SetBgPanel, Start, StartThis, Terminate, getUpAxis, setOpacity
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/AnchoredDialogHost.cs
- 23 lines; kind=MonoBehaviour; type=AnchoredDialogHost; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine
- unity: MonoBehaviour=1, Lifecycle=1, Instantiate/Destroy=1, GameObject=1; unity_score=67
- ksp refs: none; fan_in=1 fan_out=1
- methods(1): LateUpdate
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/AppUI_Control.cs
- 42 lines; kind=attribute; type=AppUI_Control; bases=Attribute; ancestry=Attribute; ns=KSP.UI
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=18 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/AppUI_Data.cs
- 112 lines; kind=class; type=AppUI_Data; bases=IConfigNode; ancestry=IConfigNode; ns=KSP.UI
- usings: KSP.UI.Screens, UnityEngine
- unity: none; unity_score=10
- ksp refs: ConfigNode=6; fan_in=7 fan_out=4
- methods(10): CallOnDataChanged, CreateAppUIDataList, CreateInstanceOfAppUIData, Load, OnLoad, OnSave, Save, SetupDataChangeCallback, UIInputPanelDataChanged, UIInputPanelUpdate
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/AppUI_Dropdown.cs
- 7 lines; kind=attribute; type=AppUI_Dropdown; bases=AppUI_Control; ancestry=AppUI_Control/Attribute; ns=KSP.UI
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=4 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/AppUI_DropdownMultiLine.cs
- 6 lines; kind=attribute; type=AppUI_DropdownMultiLine; bases=AppUI_Dropdown; ancestry=AppUI_Dropdown/AppUI_Control/Attribute; ns=KSP.UI
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/AppUI_Heading.cs
- 9 lines; kind=attribute; type=AppUI_Heading; bases=AppUI_Control; ancestry=AppUI_Control/Attribute; ns=KSP.UI
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/AppUI_Input.cs
- 13 lines; kind=attribute; type=AppUI_Input; bases=AppUI_Control; ancestry=AppUI_Control/Attribute; ns=KSP.UI
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/AppUI_InputDateTime.cs
- 11 lines; kind=attribute; type=AppUI_InputDateTime; bases=AppUI_Control; ancestry=AppUI_Control/Attribute; ns=KSP.UI
- usings: KSP.UI.Screens
- unity: none; unity_score=0
- ksp refs: none; fan_in=4 fan_out=2
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/AppUI_InputFloat.cs
- 7 lines; kind=attribute; type=AppUI_InputFloat; bases=AppUI_Input; ancestry=AppUI_Input/AppUI_Control/Attribute; ns=KSP.UI
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/AppUI_Label.cs
- 7 lines; kind=attribute; type=AppUI_Label; bases=AppUI_Control; ancestry=AppUI_Control/Attribute; ns=KSP.UI
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=5 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/AppUI_LabelList.cs
- 11 lines; kind=attribute; type=AppUI_LabelList; bases=AppUI_Label; ancestry=AppUI_Label/AppUI_Control/Attribute; ns=KSP.UI
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/AppUI_RadioBool.cs
- 7 lines; kind=attribute; type=AppUI_RadioBool; bases=AppUI_Control; ancestry=AppUI_Control/Attribute; ns=KSP.UI
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=3 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/AppUI_SliderFloat.cs
- 11 lines; kind=attribute; type=AppUI_SliderFloat; bases=AppUI_Control; ancestry=AppUI_Control/Attribute; ns=KSP.UI
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=2 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/AppUI_SliderFloatMultiLine.cs
- 6 lines; kind=attribute; type=AppUI_SliderFloatMultiLine; bases=AppUI_SliderFloat; ancestry=AppUI_SliderFloat/AppUI_Control/Attribute; ns=KSP.UI
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/AppUI_ToggleBool.cs
- 9 lines; kind=attribute; type=AppUI_ToggleBool; bases=AppUI_Control; ancestry=AppUI_Control/Attribute; ns=KSP.UI
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/BaseCrewAssignmentDialog.cs
- 461 lines; kind=MonoBehaviour; type=BaseCrewAssignmentDialog; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: GetComponent=22, Instantiate/Destroy=4, Lifecycle=2, EventSystems=2, UnityEngine.UI=1, Texture=1; unity_score=56
- ksp refs: ProtoCrewMember=10, KerbalRoster=3, GameDatabase=2; fan_in=6 fan_out=16
- methods(24): AddAvailItem, AddCrewItem, AddCrewItemEmpty, AddCrewListBorder, AddItem, Awake, ButtonClear, ButtonFill, ButtonReset, ClearLists, CreateAvailList, CreateCrewList, DropOnAvailList, DropOnCrewList, GetCurrentCrewRoster, GetManifest, ListItemButtonClick, MoveCrewToAvail, MoveCrewToEmptySeat, OnDestroy, Refresh, RefreshCrewLists, SetCurrentCrewRoster, SetDefaultManifest
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/ButtonColorState.cs
- 17 lines; kind=class; type=ButtonColorState; bases=-; ancestry=-; ns=KSP.UI
- usings: UnityEngine
- unity: none; unity_score=29
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/ButtonSpritesMgr.cs
- 52 lines; kind=MonoBehaviour; type=ButtonSpritesMgr; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine, UnityEngine.UI
- unity: Texture=6, UnityEngine.UI=2, MonoBehaviour=1; unity_score=85
- ksp refs: none; fan_in=2 fan_out=0
- methods(1): SetSpriteSet
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/ButtonState.cs
- 41 lines; kind=class; type=ButtonState; bases=-; ancestry=-; ns=KSP.UI
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: Texture=4, UnityEngine.UI=2, TMPro=1; unity_score=69
- ksp refs: none; fan_in=7 fan_out=1
- methods(1): Setup
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/ButtonStateTooltip.cs
- 12 lines; kind=class; type=ButtonStateTooltip; bases=-; ancestry=-; ns=KSP.UI
- usings: -
- unity: none; unity_score=12
- ksp refs: none; fan_in=1 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/CanvasGroupEventLocker.cs
- 42 lines; kind=MonoBehaviour; type=CanvasGroupEventLocker; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine
- unity: UnityEngine.UI=3, Lifecycle=2, MonoBehaviour=1, GetComponent=1; unity_score=78
- ksp refs: GameEvents=4; fan_in=0 fan_out=1
- methods(4): Awake, Lock, OnDestroy, Unlock
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/CanvasGroupInputLock.cs
- 137 lines; kind=MonoBehaviour; type=CanvasGroupInputLock; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine
- unity: UnityEngine.UI=3, Lifecycle=3, GameObject=2, MonoBehaviour=1, GetComponent=1; unity_score=58
- ksp refs: GameEvents=3, InputLockManager=1; fan_in=1 fan_out=3
- methods(9): Awake, InitializeDefaultMask, OnDestroy, OnInputLocksModified, SetInputMask, SetInputMaskToDefault, Start, ToBits, UpdateLockMask
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/ContentAspectController.cs
- 61 lines; kind=MonoBehaviour; type=ContentAspectController; bases=MonoBehaviour,ILayoutController,ILayoutSelfController; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=4, Transform=2, MonoBehaviour=1; unity_score=70
- ksp refs: none; fan_in=0 fan_out=0
- methods(4): OnRectTransformDimensionsChange, Reset, SetLayoutHorizontal, SetLayoutVertical
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/CrewAssignmentDialog.cs
- 111 lines; kind=MonoBehaviour; type=CrewAssignmentDialog; bases=BaseCrewAssignmentDialog; ancestry=BaseCrewAssignmentDialog/MonoBehaviour; ns=KSP.UI
- usings: PreFlightTests, UnityEngine
- unity: Coroutine=3, Lifecycle=2, GameObject=1; unity_score=47
- ksp refs: HighLogic=6, GameEvents=3, KerbalRoster=2, InputLockManager=2, EditorLogic=2, Game=1; fan_in=6 fan_out=11
- methods(14): Awake, ButtonAstronautComplex, DropOnAvailList, DropOnCrewList, EditorLogicUpdateCrew, GetCurrentCrewRoster, MoveCrewToAvail, MoveCrewToEmptySeat, OnDestroy, RefreshCrewOnDrop, SetCurrentCrewRoster, onOpenACDismiss, onOpenACProceed, onOpenAstronautComplex
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/CrewListItem.cs
- 351 lines; kind=MonoBehaviour; type=CrewListItem; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: Expansions, Expansions.Missions, Expansions.Serenity, KSP.UI.TooltipTypes, TMPro, UnityEngine, UnityEngine.Events, UnityEngine.UI
- unity: UnityEngine.UI=9, TMPro=3, SetActive=3, GameObject=3, GetComponent=2, Texture=1; unity_score=60
- ksp refs: ProtoCrewMember=14, GameDatabase=2, KerbalRoster=1; fan_in=6 fan_out=13
- methods(18): AddButtonInputDelegate, Awake, GetCourage, GetCrewRef, GetName, GetStupidity, OpenHelmetSuitPickerWindow, SetButton, SetButtonEnabled, SetCrewRef, SetKerbal, SetKerbalAsApplicableType, SetLabel, SetName, SetStats, SetTooltip, SetXP, SetupSuitButtons
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/DialogMouseEnterControlLock.cs
- 43 lines; kind=MonoBehaviour; type=DialogMouseEnterControlLock; bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler,IPointerExitHandler; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine, UnityEngine.EventSystems
- unity: EventSystems=4, MonoBehaviour=1, Lifecycle=1; unity_score=75
- ksp refs: InputLockManager=3; fan_in=1 fan_out=2
- methods(6): OnDestroy, OnPointerEnter, OnPointerExit, SetLockName, SetLockType, Setup
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Dialogs/FlightResultsDialog.cs
- 317 lines; kind=MonoBehaviour; type=FlightResultsDialog; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Dialogs
- usings: KSP.Localization, PreFlightTests, TMPro, UnityEngine, UnityEngine.UI
- unity: GameObject=15, SetActive=14, TMPro=12, UnityEngine.UI=8, GetComponent=8, Lifecycle=3; unity_score=82
- ksp refs: Localizer=14, HighLogic=8, Planetarium=3, InputLockManager=2, Vessel=1, GameSettings=1; fan_in=1 fan_out=19
- methods(10): Awake, Close, Display, OnDestroy, SetMissionOutcome, SetupGUI, Update, onLeavingFlight, onLeavingFlightDismiss, onLeavingFlightProceed
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Dialogs/IAccessKerbNet.cs
- 21 lines; kind=interface; type=IAccessKerbNet; bases=-; ancestry=-; ns=KSP.UI.Dialogs
- usings: -
- unity: none; unity_score=0
- ksp refs: Vessel=1, Part=1; fan_in=2 fan_out=2
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Dialogs/KerbNetDialog.cs **MUST-OPEN (>800 lines)**
- 1224 lines; kind=MonoBehaviour; type=KerbNetDialog; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Dialogs
- usings: CommNet, FinePrint, FinePrint.Utilities, KSP.Localization, KSP.UI.TooltipTypes, KerbNet, TMPro, UnityEngine, UnityEngine.UI
- unity: Texture=10, TMPro=10, UnityEngine.UI=9, GameObject=8, SetActive=6, Coroutine=5; unity_score=53
- ksp refs: Vector3d=20, Localizer=14, GameEvents=6, CelestialBody=5, HighLogic=3, GameSettings=3; fan_in=6 fan_out=38
- methods(55): ActivateDisplayMode, AddKnownMode, AutoRefreshRoutine, Awake, CacheLocalStrings, ChangeMapPosition, Close, CreateMapTexture, DeactivateDisplayMode, Display, DrawAnomaliesOnMap, DrawContourLines, FullRefresh, GenerateAssemblyModes, GenerateKnownModes, GenerateResourceModes, GetModeByName, GetScanLatitudeAndLongitude, GetTexture, HasActiveConnection, HasValidAccessor, HasValidDisplayMode, HasVesselControl, NormalizedDistanceFromCenter, OnAutoRefreshChanged, OnDestroy, OnFoVSet, OnModeChanged, OnPartDestroyed, OnRefreshClicked, OnVesselStatusChanged, OnVisibilityChanged, OnWaypointClicked, OnWaypointFieldEndEdit, RandomizeWaypointField, RefreshCustomButton, RefreshDataLabels, RefreshErrorState, RefreshMap, ResetMapPosition, ScanSpaceLocation, SetActivesForError, SetFoVBounds, SetFoVText, SetupAllResourceDisplayModes ...
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Dialogs/WaypointTarget.cs
- 143 lines; kind=MonoBehaviour; type=WaypointTarget; bases=MonoBehaviour,IBeginDragHandler,IEventSystemHandler,IDragHandler,IEndDragHandler; ancestry=MonoBehaviour; ns=KSP.UI.Dialogs
- usings: UnityEngine, UnityEngine.EventSystems
- unity: EventSystems=6, UnityEngine.UI=5, Time.=3, Lifecycle=3, Input.=2, MonoBehaviour=1; unity_score=76
- ksp refs: none; fan_in=1 fan_out=3
- methods(13): Awake, CheckTraverseClick, ChildClampedLocalPosition, ChildClampedWorldPosition, OnBeginDrag, OnDisable, OnDrag, OnEndDrag, Reset, Target, Update, UpdateScale, UpdateTraversal
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/DialogUISelectFile.cs
- 109 lines; kind=MonoBehaviour; type=DialogUISelectFile; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: Instantiate/Destroy=6, UnityEngine.UI=3, Transform=3, TMPro=2, Lifecycle=2, GameObject=2; unity_score=77
- ksp refs: none; fan_in=1 fan_out=4
- methods(9): Awake, Destroy, OnAccept, OnCancel, OnFileSelected, SelectItem, Setup, Start, UpdateFiles
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/DialogUISelectFileItem.cs
- 12 lines; kind=MonoBehaviour; type=DialogUISelectFileItem; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: TMPro, UnityEngine
- unity: TMPro=1, MonoBehaviour=1; unity_score=55
- ksp refs: none; fan_in=1 fan_out=2
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/DragHandler.cs
- 77 lines; kind=MonoBehaviour; type=DragHandler; bases=MonoBehaviour,IInitializePotentialDragHandler,IEventSystemHandler,IBeginDragHandler,IDragHandler,IEndDragHandler; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine, UnityEngine.EventSystems, UnityEngine.Events
- unity: EventSystems=31, MonoBehaviour=1; unity_score=98
- ksp refs: none; fan_in=3 fan_out=0
- methods(7): AddEvents, ClearEvents, OnBeginDrag, OnDrag, OnEndDrag, OnInitializePotentialDrag, RemoveEvents
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/DropHandler.cs
- 22 lines; kind=MonoBehaviour; type=DropHandler; bases=MonoBehaviour,IDropHandler,IEventSystemHandler; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine, UnityEngine.EventSystems, UnityEngine.Events
- unity: EventSystems=6, MonoBehaviour=1; unity_score=89
- ksp refs: none; fan_in=1 fan_out=0
- methods(1): OnDrop
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/EventTriggerForwarder.cs
- 70 lines; kind=MonoBehaviour; type=EventTriggerForwarder; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine, UnityEngine.EventSystems
- unity: EventSystems=19, Transform=4, GameObject=4, MonoBehaviour=1, Lifecycle=1, GetComponent=1; unity_score=97
- ksp refs: none; fan_in=0 fan_out=0
- methods(5): Awake, Drop, PointerEnter, PointerExit, Scroll
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/ExperimentalVersionReadout.cs
- 35 lines; kind=MonoBehaviour; type=ExperimentalVersionReadout; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: Expansions, KSP.Localization, TMPro, UnityEngine
- unity: SetActive=3, GameObject=3, Lifecycle=2, TMPro=1, MonoBehaviour=1; unity_score=86
- ksp refs: GameEvents=2, Localizer=1, GameSettings=1; fan_in=0 fan_out=7
- methods(3): OnDestroy, OnLevelLoaded, Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/GenericAppFrame.cs
- 203 lines; kind=MonoBehaviour; type=GenericAppFrame; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: KSP.UI.Screens, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.Events, UnityEngine.UI
- unity: UnityEngine.UI=8, EventSystems=8, Transform=5, Time.=2, Lifecycle=2, GameObject=2; unity_score=76
- ksp refs: GameSettings=10; fan_in=8 fan_out=7
- methods(14): AddGlobalInputDelegate, AutoScale, Awake, OnBeginDragFooter, OnBeginDragHeader, OnDestroy, OnDragFooter, OnDragHeader, OnEndDragFooter, OnEndDragHeader, Reposition, Setup, UpdateDraggingBounds, UpdateDraggingBoundsAtBottom
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/GenericCascadingList.cs
- 167 lines; kind=MonoBehaviour; type=GenericCascadingList; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: TMPro=13, GetComponent=11, Instantiate/Destroy=8, UnityEngine.UI=2, EventSystems=2, SetActive=1; unity_score=84
- ksp refs: none; fan_in=7 fan_out=6
- methods(11): CreateBody, CreateBodyCollapsable, CreateBodyCollapsable_spacer, CreateBodyKeyValueAutofit, CreateBody_spacer, CreateFooter, CreateHeader, MouseInputCollapsableBody, MouseInputCollapsableBody_spacer, Setup, UpdateBodyCollapsable
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/InteractableCtrlUIStates.cs
- 91 lines; kind=MonoBehaviour; type=InteractableCtrlUIStates; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: SetActive=3, UnityEngine.UI=2, Lifecycle=2, TMPro=1, MonoBehaviour=1; unity_score=73
- ksp refs: none; fan_in=0 fan_out=1
- methods(3): Awake, LateUpdate, SetActive
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/IPinnableTooltipController.cs
- 13 lines; kind=interface; type=IPinnableTooltipController; bases=ITooltipController; ancestry=-; ns=KSP.UI
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=4 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/IRadioButton.cs
- 9 lines; kind=interface; type=IRadioButton; bases=-; ancestry=-; ns=KSP.UI
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=3 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/ITooltipController.cs
- 25 lines; kind=interface; type=ITooltipController; bases=-; ancestry=-; ns=KSP.UI
- usings: UnityEngine
- unity: UnityEngine.UI=1; unity_score=29
- ksp refs: none; fan_in=6 fan_out=0
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/KSPGraphicRaycaster.cs
- 73 lines; kind=class; type=KSPGraphicRaycaster; bases=GraphicRaycaster; ancestry=GraphicRaycaster; ns=KSP.UI
- usings: UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: UnityEngine.UI=2, Raycast/Query=2, GameObject=1, EventSystems=1; unity_score=51
- ksp refs: InputLockManager=1; fan_in=0 fan_out=2
- methods(2): Raycast, UpdateLockMask
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Language/DictionarySerializerHelper.cs
- 22 lines; kind=class; type=DictionarySerializerHelper; bases=-; ancestry=-; ns=KSP.UI.Language
- usings: -
- unity: none; unity_score=12
- ksp refs: none; fan_in=1 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=Localization

## Assembly-CSharp/KSP/UI/Language/ILanguageKey.cs
- 9 lines; kind=interface; type=ILanguageKey; bases=-; ancestry=-; ns=KSP.UI.Language
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=1 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=Localization

## Assembly-CSharp/KSP/UI/Language/Language.cs
- 470 lines; kind=MonoBehaviour; type=Language; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Language
- usings: UnityEngine, UnityEngine.Networking, UnityEngine.UI
- unity: Instantiate/Destroy=11, AssetBundle=11, Coroutine=5, Lifecycle=2, WWW/WebRequest=1, MonoBehaviour=1; unity_score=56
- ksp refs: none; fan_in=5 fan_out=2
- methods(22): ActivateAllFonts, Awake, DeactivateAllFonts, GetLanguage, LanguageName, LoadBundle, LoadLanguage, LoadLanguages, LoadedFonts, LoadedLanguages, OnDestroy, Save, SaveLanguages, SaveUntranslated, SetLanguage, Translate, TranslateWithFormat, UpdateFont, UpdateText, onActivateAllFonts, onLoadLanguages, onSetLanguage
- auto suggestion: tier=C subsystem=Localization

## Assembly-CSharp/KSP/UI/Language/LanguageDefinition.cs
- 211 lines; kind=class; type=LanguageDefinition; bases=-; ancestry=-; ns=KSP.UI.Language
- usings: -
- unity: none; unity_score=2
- ksp refs: none; fan_in=1 fan_out=3
- methods(9): AddTranslation, Deserialize, GetFullPath, MergeInto, RemoveTranslation, Serialize, SerializeUntranslated, TranslateWithFormat, Translation
- auto suggestion: tier=C subsystem=Localization

## Assembly-CSharp/KSP/UI/Language/LanguageText.cs
- 51 lines; kind=MonoBehaviour; type=LanguageText; bases=MonoBehaviour,ILanguageKey; ancestry=MonoBehaviour; ns=KSP.UI.Language
- usings: UnityEngine, UnityEngine.UI
- unity: MonoBehaviour=1, Lifecycle=1, GetComponent=1; unity_score=47
- ksp refs: none; fan_in=0 fan_out=2
- methods(2): Reset, Start
- auto suggestion: tier=C subsystem=Localization

## Assembly-CSharp/KSP/UI/Language/StringExtensions.cs
- 14 lines; kind=static class; type=StringExtensions; bases=-; ancestry=-; ns=KSP.UI.Language
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=0 fan_out=1
- methods(1): Translate
- auto suggestion: tier=C subsystem=Localization

## Assembly-CSharp/KSP/UI/ManeuverToolUIFrame.cs
- 662 lines; kind=MonoBehaviour; type=ManeuverToolUIFrame; bases=GenericAppFrame; ancestry=GenericAppFrame/MonoBehaviour; ns=KSP.UI
- usings: KSP.Localization, KSP.UI.Screens, TMPro, UnityEngine, UnityEngine.UI
- unity: GameObject=20, TMPro=11, UnityEngine.UI=10, Instantiate/Destroy=10, Transform=7, Lifecycle=3; unity_score=68
- ksp refs: Localizer=2; fan_in=3 fan_out=15
- methods(33): ActivatePlanet, AddObjectToVisualizerWindow, AddOnErrorStateChanged, AddOnSidePanelClicked, AddPlanetRecursively, CreatePlanets, DeactivatePlanet, FindPlanet, OnActionDelegate, OnBtnSidePanelClick, OnBtnTopPanelClick, OnCreateManeuverBtn, OnDestroy, OnErrorStateDelegate, OnTimeDisplaySecToggle, OnTimeDisplayUTToggle, RemoveObjectFromVisualizerWindow, RemoveOnErrorStateChanged, RemoveOnSidePanelClicked, SetCalculationState, SetCreateManeuverButton, SetErrorState, SetHeaderText, SetHelpString, SetTopPanelHeader, SetTopPanelState, SetVisualPanelState, Start, TransitionTopPanelImmediate, TransitionVisualPanelImmediate, Update, UpdateDVText, UpdateTimeText
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/MessageSystemAppFrame.cs
- 201 lines; kind=MonoBehaviour; type=MessageSystemAppFrame; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: KSP.Localization, KSP.UI.Screens, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.Events, UnityEngine.UI
- unity: EventSystems=8, UnityEngine.UI=4, Transform=4, Time.=2, TMPro=2, Lifecycle=2; unity_score=72
- ksp refs: GameSettings=10, HighLogic=6, Localizer=1; fan_in=1 fan_out=11
- methods(13): AddGlobalInputDelegate, Awake, OnBeginDragFooter, OnBeginDragHeader, OnDestroy, OnDragFooter, OnDragHeader, OnEndDragFooter, OnEndDragHeader, Reposition, Setup, UpdateDraggingBounds, UpdateDraggingBoundsAtBottom
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/PinnableTooltipController.cs
- 61 lines; kind=MonoBehaviour; type=PinnableTooltipController; bases=TooltipController,IPinnableTooltipController,ITooltipController,IPointerClickHandler,IEventSystemHandler; ancestry=TooltipController/MonoBehaviour; ns=KSP.UI
- usings: UnityEngine.EventSystems
- unity: EventSystems=5; unity_score=65
- ksp refs: none; fan_in=3 fan_out=4
- methods(7): IsPinned, OnPointerClick, OnPointerEnter, OnPointerExit, OnTooltipPinned, OnTooltipUnpinned, Unpin
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/PointerClickAndHoldHandler.cs
- 45 lines; kind=MonoBehaviour; type=PointerClickAndHoldHandler; bases=PointerClickHandler,IPointerExitHandler,IEventSystemHandler; ancestry=PointerClickHandler/MonoBehaviour; ns=KSP.UI
- usings: UnityEngine, UnityEngine.EventSystems
- unity: EventSystems=7, Coroutine=5; unity_score=88
- ksp refs: none; fan_in=6 fan_out=1
- methods(4): OnPointerDown, OnPointerDownHold, OnPointerExit, OnPointerUp
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/PointerClickHandler.cs
- 47 lines; kind=MonoBehaviour; type=PointerClickHandler; bases=MonoBehaviour,IPointerClickHandler,IEventSystemHandler,IPointerDownHandler,IPointerUpHandler; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine, UnityEngine.EventSystems, UnityEngine.Events
- unity: EventSystems=14, MonoBehaviour=1; unity_score=94
- ksp refs: none; fan_in=22 fan_out=0
- methods(3): OnPointerClick, OnPointerDown, OnPointerUp
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/PointerEnterExitHandler.cs
- 44 lines; kind=MonoBehaviour; type=PointerEnterExitHandler; bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler,IPointerExitHandler; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine, UnityEngine.EventSystems, UnityEngine.Events
- unity: EventSystems=16, MonoBehaviour=1; unity_score=98
- ksp refs: none; fan_in=20 fan_out=0
- methods(2): OnPointerEnter, OnPointerExit
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/PrefabSpawner.cs
- 60 lines; kind=MonoBehaviour; type=PrefabSpawner; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine
- unity: GameObject=6, Transform=4, Lifecycle=2, MonoBehaviour=1, Instantiate/Destroy=1; unity_score=82
- ksp refs: none; fan_in=0 fan_out=0
- methods(3): Awake, Spawn, Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/ProgressBar.cs
- 35 lines; kind=MonoBehaviour; type=ProgressBar; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=2, MonoBehaviour=1, Lifecycle=1, GetComponent=1, GameObject=1; unity_score=77
- ksp refs: none; fan_in=1 fan_out=0
- methods(1): Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/RadioButtonGroup.cs
- 77 lines; kind=class; type=RadioButtonGroup; bases=-; ancestry=-; ns=KSP.UI
- usings: UnityEngine
- unity: Transform=1, GameObject=1; unity_score=17
- ksp refs: none; fan_in=2 fan_out=1
- methods(2): GetGroup, GetSelected
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/RectTransformQuadFitter.cs
- 39 lines; kind=MonoBehaviour; type=RectTransformQuadFitter; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine
- unity: Coroutine=4, UnityEngine.UI=1, Transform=1, MonoBehaviour=1, Lifecycle=1; unity_score=79
- ksp refs: none; fan_in=0 fan_out=0
- methods(2): Awake, UpdateDaemon
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/RendererSortOrder.cs
- 27 lines; kind=MonoBehaviour; type=RendererSortOrder; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine
- unity: Renderer=3, MonoBehaviour=1, Lifecycle=1, GetComponent=1; unity_score=84
- ksp refs: none; fan_in=0 fan_out=0
- methods(2): Set, Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Rendering/CameraShaderReplacement.cs
- 34 lines; kind=MonoBehaviour; type=CameraShaderReplacement; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Rendering
- usings: UnityEngine
- unity: Camera=4, GetComponent=2, GameObject=2, Shader=1, MonoBehaviour=1, Lifecycle=1; unity_score=93
- ksp refs: none; fan_in=0 fan_out=0
- methods(3): Reset, SetShader, Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Rendering/InvertSortOrder.cs
- 32 lines; kind=MonoBehaviour; type=InvertSortOrder; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Rendering
- usings: UnityEngine
- unity: Transform=7, UnityEngine.UI=5, MonoBehaviour=1, GetComponent=1, GameObject=1; unity_score=96
- ksp refs: none; fan_in=0 fan_out=0
- methods(2): InvertSortingOrder, RecurseLevel
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Rendering/RTCanvas.cs
- 133 lines; kind=MonoBehaviour; type=RTCanvas; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Rendering
- usings: UniLinq, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=14, GameObject=14, Transform=8, GetComponent=7, Camera=4, Texture=3; unity_score=96
- ksp refs: GameEvents=2, GameSettings=1; fan_in=0 fan_out=2
- methods(7): Awake, FindCanvasesUsingCamera, OnDestroy, SetUIOpacity, Setup, SetupRT, SetupRTCanvas
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/ResizingLayoutElement.cs
- 125 lines; kind=MonoBehaviour; type=ResizingLayoutElement; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=5, Coroutine=4, Time.=2, GetComponent=2, MonoBehaviour=1, Lifecycle=1; unity_score=70
- ksp refs: none; fan_in=3 fan_out=1
- methods(4): Awake, Resize, ReverseResizing, StartResizing
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/ACSceneDespawner.cs
- 20 lines; kind=MonoBehaviour; type=ACSceneDespawner; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=1, MonoBehaviour=1, Lifecycle=1; unity_score=63
- ksp refs: GameEvents=1; fan_in=0 fan_out=1
- methods(2): BtnExit, Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/ACSceneSpawner.cs
- 34 lines; kind=MonoBehaviour; type=ACSceneSpawner; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: UnityEngine
- unity: Lifecycle=2, MonoBehaviour=1; unity_score=56
- ksp refs: GameEvents=4, HighLogic=1; fan_in=0 fan_out=8
- methods(4): Awake, OnDestroy, onACDespawn, onACSpawn
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/ActionGroupsApp.cs
- 230 lines; kind=MonoBehaviour; type=ActionGroupsApp; bases=UIApp; ancestry=UIApp/MonoBehaviour; ns=KSP.UI.Screens
- usings: TMPro, UnityEngine
- unity: Transform=6, GameObject=5, Instantiate/Destroy=3, UnityEngine.UI=2, TMPro=2, SetActive=2; unity_score=62
- ksp refs: Vessel=10, GameEvents=10, FlightGlobals=6, HighLogic=2; fan_in=3 fan_out=15
- methods(19): DisplayApp, GetAppScenes, GetAppScreenPos, HideApp, HideAppFromInput, HideUnpinApp, OnAppAboutToStart, OnAppDestroy, OnAppInitialized, OnVesselChanged, OnVesselDestroy, OnVesselGoOffRails, OnVesselOverrideGroupChanged, OnVesselWasModified, ReadjustPosition, Reposition, SelectNext, SelectPrev, UpdateOverrideGroupText
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/ActionGroupsPanel.cs
- 207 lines; kind=MonoBehaviour; type=ActionGroupsPanel; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: Transform=12, UnityEngine.UI=10, GameObject=10, SetActive=7, Lifecycle=6, Instantiate/Destroy=6; unity_score=86
- ksp refs: GameEvents=6, Vessel=4, FlightGlobals=4, Localizer=3, MapView=2, GameSettings=2; fan_in=3 fan_out=10
- methods(10): Awake, OnDestroy, OnDisable, OnEnable, SelectOverride, SetGroupName, Start, Update, UpdateActionGroups, UpdateButtons
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/ActiveStrategyListItem.cs
- 28 lines; kind=MonoBehaviour; type=ActiveStrategyListItem; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: TMPro=2, UnityEngine.UI=1, Texture=1, MonoBehaviour=1; unity_score=80
- ksp refs: none; fan_in=1 fan_out=1
- methods(1): Setup
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Administration.cs
- 448 lines; kind=MonoBehaviour; type=Administration; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.Localization, Strategies, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: GetComponent=7, GameObject=7, Texture=6, UnityEngine.UI=5, SetActive=5, Instantiate/Destroy=5; unity_score=66
- ksp refs: Localizer=16, HighLogic=4; fan_in=7 fan_out=24
- methods(25): AddActiveStratItem, AddKerbalListItem, AddStrategiesListItem, Awake, BtnInputAccept, CreateActiveStratItem, CreateActiveStratList, CreateStrategiesList, GetStrategyDescription, OnAcceptConfirm, OnCancelConfirm, OnDestroy, OnFalse, OnPopupDismiss, OnSliderCommitmentValueChanged, OnTrue, RedrawPanels, SetEffectSliderValue, SetSelectedStrategy, SetStrategyDescription, Start, UnselectStrategy, UpdateStrategyCount, UpdateStrategyDescription, UpdateStrategyStats
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/AdministrationSceneDespawner.cs
- 20 lines; kind=MonoBehaviour; type=AdministrationSceneDespawner; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=1, MonoBehaviour=1, Lifecycle=1; unity_score=63
- ksp refs: GameEvents=1; fan_in=0 fan_out=1
- methods(2): BtnExit, Start
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/AdministrationSceneSpawner.cs
- 35 lines; kind=MonoBehaviour; type=AdministrationSceneSpawner; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: UnityEngine
- unity: Lifecycle=2, MonoBehaviour=1; unity_score=59
- ksp refs: GameEvents=4, HighLogic=1; fan_in=0 fan_out=8
- methods(4): Awake, OnDestroy, onAdminDespawn, onAdminSpawn
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/AlarmClockApp.cs
- 179 lines; kind=MonoBehaviour; type=AlarmClockApp; bases=UIApp; ancestry=UIApp/MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.Localization, UnityEngine
- unity: Transform=4, GameObject=3, UnityEngine.UI=2, SetActive=2, Lifecycle=2, Instantiate/Destroy=2; unity_score=59
- ksp refs: HighLogic=8, InputLockManager=3, Game=3, Localizer=1, GameEvents=1; fan_in=3 fan_out=12
- methods(16): AnyTextFieldHasFocus, AppFrameHasLock, DisplayApp, EditAlarm, GetAppScenes, GetAppScreenPos, HideApp, OnAppAboutToStart, OnAppDestroy, OnAppInitialized, OnSceneSwitch, RemoveAppFrameInputLock, Reset, Start, TextFieldHasFocus, Update
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/AltimeterSliderButtons.cs
- 325 lines; kind=MonoBehaviour; type=AltimeterSliderButtons; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: Expansions.Missions, KSP.Localization, KSP.UI.TooltipTypes, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: Coroutine=10, Lifecycle=4, Shader=3, GetComponent=3, UnityEngine.UI=2, Transform=2; unity_score=60
- ksp refs: HighLogic=16, GameEvents=10, Localizer=8, Vessel=7, FlightGlobals=7, GameSettings=3; fan_in=0 fan_out=27
- methods(19): Awake, Collapse, Expand, GetInstance, LateUpdate, OnDestroy, OnFlightStarted, OnVesselFocusChanged, OnVesselSituationChange, Start, UnlockRecovery, UnlockReturnToKSC, UpdateForVessel, UpdateUIScale, drawExitWithoutSaveOptions, recoverVessel, returnToSpaceCenter, saveAndExit, setUnlock
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/ApplauncherLayout.cs
- 30 lines; kind=interface; type=ApplauncherLayout; bases=-; ancestry=-; ns=KSP.UI.Screens
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=5, GameObject=1; unity_score=69
- ksp refs: none; fan_in=2 fan_out=2
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/ApplicationLauncher.cs
- 766 lines; kind=MonoBehaviour; type=ApplicationLauncher; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: GameObject=28, SetActive=24, UnityEngine.UI=17, Instantiate/Destroy=8, Transform=6, Coroutine=5; unity_score=69
- ksp refs: GameEvents=30, HighLogic=18, MapView=10, GameSettings=10; fan_in=25 fan_out=14
- methods(46): AddApplication, AddModApplication, AddOnHideCallback, AddOnRepositionCallback, AddOnShowCallback, Awake, BtnDown, BtnUp, Contains, DetermineVisibility, DetermineVisibilityOfAllApps, DisableMutuallyExclusive, EnableMutuallyExclusive, GetAppListSize, GetKnowledgeBaseList, GetModListMaxSize, GetModListSize, Hide, MutuallyExclusiveStockApplistCLICK, MutuallyExclusiveStockApplistHOVER, MutuallyExclusiveStockApplistHOVEROUT, MutuallyExclusiveStockApplistHOVERing, OnDestroy, OnGameSettingsApplied, OnHide, OnReposition, OnSceneLoadRequested, OnSceneLoadedGUIReady, OnShow, RemoveApplication, RemoveModApplication, RemoveOnHideCallback, RemoveOnRepositionCallback, RemoveOnShowCallback, ScaleModList, SetHidden, SetVisible, ShouldBeOnTop, ShouldBeVisible, ShouldItHide, ShouldItShow, Show, SpawnHorizontal, SpawnSimpleLayout, SpawnVertical ...
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/ApplicationLauncherButton.cs
- 365 lines; kind=MonoBehaviour; type=ApplicationLauncherButton; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: Transform=7, Animation=7, Texture=6, EventSystems=6, GameObject=5, UnityEngine.UI=3; unity_score=71
- ksp refs: GameEvents=2; fan_in=13 fan_out=6
- methods(22): AnimCoroutine, Awake, Disable, Enable, GetAnchor, GetAnchorLocal, GetAnchorTopRight, GetAnchorUL, GetAnchorUR, OnClick, OnDestroy, OnFalse, OnHover, OnHoverOut, OnTrue, PlayAnim, SetFalse, SetSprite, SetTexture, SetTrue, Setup, StopAnim
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/AppUIInputPanel.cs
- 141 lines; kind=MonoBehaviour; type=AppUIInputPanel; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: Transform=6, UnityEngine.UI=2, Lifecycle=2, Instantiate/Destroy=2, GameObject=2, TMPro=1; unity_score=68
- ksp refs: none; fan_in=6 fan_out=5
- methods(9): AnyTextFieldHasFocus, DestroyChildren, GetControl, RefreshUI, ReleaseData, SetErrorState, Setup, Start, Update
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/AppUIMember.cs
- 322 lines; kind=MonoBehaviour; type=AppUIMember; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=5, Lifecycle=3, TMPro=2, Transform=1, SetActive=1, MonoBehaviour=1; unity_score=49
- ksp refs: Localizer=1; fan_in=13 fan_out=8
- methods(15): AnyTextFieldHasFocus, Awake, GetValue, OnAwake, OnInitialized, OnRefreshUI, OnStart, OnUpdate, RefreshUI, SetHoverTextTarget, SetValue, Setup, SortByOrder, Start, Update
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/AppUIMemberDateTime.cs
- 279 lines; kind=MonoBehaviour; type=AppUIMemberDateTime; bases=AppUIMember; ancestry=AppUIMember/MonoBehaviour; ns=KSP.UI.Screens
- usings: TMPro, UnityEngine
- unity: TMPro=6, KeyCode=3, Input.=3, SetActive=2, GameObject=2, Lifecycle=1; unity_score=55
- ksp refs: none; fan_in=4 fan_out=7
- methods(8): AnyTextFieldHasFocus, InputEdited, OnDestroy, OnInitialized, OnRefreshUI, OnStart, OnUpdate, SetMode
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/AppUIMemberDropdown.cs
- 160 lines; kind=MonoBehaviour; type=AppUIMemberDropdown; bases=AppUIMember; ancestry=AppUIMember/MonoBehaviour; ns=KSP.UI.Screens
- usings: TMPro, UnityEngine
- unity: TMPro=4, Texture=1, Lifecycle=1; unity_score=48
- ksp refs: none; fan_in=7 fan_out=4
- methods(7): BuildDropDownItemsList, GetIndexOfKey, OnDestroy, OnDropdownChanged, OnInitialized, OnRefreshUI, OnStart
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/AppUIMemberDropdownMultiLine.cs
- 16 lines; kind=MonoBehaviour; type=AppUIMemberDropdownMultiLine; bases=AppUIMemberDropdown; ancestry=AppUIMemberDropdown/AppUIMember/MonoBehaviour; ns=KSP.UI.Screens
- usings: -
- unity: none; unity_score=15
- ksp refs: none; fan_in=0 fan_out=2
- methods(2): OnInitialized, OnRefreshUI
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/AppUIMemberHeading.cs
- 70 lines; kind=MonoBehaviour; type=AppUIMemberHeading; bases=AppUIMember; ancestry=AppUIMember/MonoBehaviour; ns=KSP.UI.Screens
- usings: TMPro, UnityEngine
- unity: UnityEngine.UI=1; unity_score=37
- ksp refs: none; fan_in=0 fan_out=4
- methods(3): AdjustHeadingBackground, OnInitialized, OnRefreshUI
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/AppUIMemberInput.cs
- 103 lines; kind=MonoBehaviour; type=AppUIMemberInput; bases=AppUIMember; ancestry=AppUIMember/MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.Localization, TMPro, UnityEngine
- unity: UnityEngine.UI=4, Transform=2, TMPro=2, SetActive=1, GameObject=1; unity_score=66
- ksp refs: Localizer=1; fan_in=1 fan_out=10
- methods(6): AnyTextFieldHasFocus, InputEdited, OnInitialized, OnRefreshUI, OnStart, RefreshUIInput
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/AppUIMemberInputFloat.cs
- 22 lines; kind=MonoBehaviour; type=AppUIMemberInputFloat; bases=AppUIMemberInput; ancestry=AppUIMemberInput/AppUIMember/MonoBehaviour; ns=KSP.UI.Screens
- usings: -
- unity: none; unity_score=15
- ksp refs: none; fan_in=0 fan_out=3
- methods(2): OnInitialized, RefreshUIInput
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/AppUIMemberLabel.cs
- 39 lines; kind=MonoBehaviour; type=AppUIMemberLabel; bases=AppUIMember; ancestry=AppUIMember/MonoBehaviour; ns=KSP.UI.Screens
- usings: TMPro
- unity: TMPro=1; unity_score=39
- ksp refs: none; fan_in=2 fan_out=5
- methods(2): OnInitialized, OnRefreshUI
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/AppUIMemberLabelList.cs
- 131 lines; kind=MonoBehaviour; type=AppUIMemberLabelList; bases=AppUIMember; ancestry=AppUIMember/MonoBehaviour; ns=KSP.UI.Screens
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: GameObject=4, UnityEngine.UI=2, TMPro=2, SetActive=2, GetComponent=1; unity_score=61
- ksp refs: none; fan_in=1 fan_out=6
- methods(2): OnInitialized, OnRefreshUI
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/AppUIMemberRadioBool.cs
- 58 lines; kind=MonoBehaviour; type=AppUIMemberRadioBool; bases=AppUIMember; ancestry=AppUIMember/MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=1, TMPro=1, Lifecycle=1; unity_score=53
- ksp refs: Localizer=1; fan_in=1 fan_out=4
- methods(4): OnDestroy, OnInitialized, OnRefreshUI, OnTrueToggleChanged
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/AppUIMemberSliderFloat.cs
- 45 lines; kind=MonoBehaviour; type=AppUIMemberSliderFloat; bases=AppUIMember; ancestry=AppUIMember/MonoBehaviour; ns=KSP.UI.Screens
- usings: UnityEngine.UI
- unity: UnityEngine.UI=1, Lifecycle=1; unity_score=46
- ksp refs: none; fan_in=1 fan_out=2
- methods(4): OnDestroy, OnInitialized, OnRefreshUI, OnSliderChanged
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/AppUIMemberSliderFloatMultiLine.cs
- 16 lines; kind=MonoBehaviour; type=AppUIMemberSliderFloatMultiLine; bases=AppUIMemberSliderFloat; ancestry=AppUIMemberSliderFloat/AppUIMember/MonoBehaviour; ns=KSP.UI.Screens
- usings: -
- unity: none; unity_score=15
- ksp refs: none; fan_in=0 fan_out=2
- methods(2): OnInitialized, OnRefreshUI
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/AppUIMemberToggleBool.cs
- 86 lines; kind=MonoBehaviour; type=AppUIMemberToggleBool; bases=AppUIMember; ancestry=AppUIMember/MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=2, TMPro=2, Lifecycle=1; unity_score=58
- ksp refs: Localizer=2; fan_in=0 fan_out=4
- methods(4): OnDestroy, OnInitialized, OnRefreshUI, OnTrueToggleChanged
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/AstronautComplex.cs
- 375 lines; kind=MonoBehaviour; type=AstronautComplex; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: Expansions.Missions, KSP.Localization, TMPro, UnityEngine
- unity: GetComponent=6, TMPro=5, Lifecycle=3, Transform=2, SetActive=2, GameObject=2; unity_score=55
- ksp refs: HighLogic=26, Localizer=22, ProtoCrewMember=21, Game=2, ProtoVessel=1, ProtoPartSnapshot=1; fan_in=5 fan_out=32
- methods(20): AddItem, AddItem_Applicants, AddItem_Assigned, AddItem_Available, AddItem_Kia, Awake, CreateApplicantList, CreateAssignedList, CreateAvailableList, CreateKiaList, HireRecruit, InitiateGUI, OnDestroy, SetApplicantsListUnlocked, Start, UpdateCrewCosts, UpdateCrewCounts, Vbutton, Xbutton_AvailableCrew, Xbutton_MIA
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/BasePartCategorizer.cs
- 385 lines; kind=MonoBehaviour; type=BasePartCategorizer; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.Localization, RUI.Icons.Selectable, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: Coroutine=3, UnityEngine.UI=2, Time.=2, TMPro=1, MonoBehaviour=1, Lifecycle=1; unity_score=38
- ksp refs: AvailablePart=56, Localizer=12, Part=1, GameEvents=1; fan_in=4 fan_out=11
- methods(14): Awake, GeneratePartAutoTags, LoadAutoTags, PartMatchesSearch, SearchField_OnClick, SearchField_OnEndEdit, SearchField_OnValueChange, SearchFilterResult, SearchRoutine, SearchStart, SearchStop, SearchTagSplit, TagMatchType, TermMatchesTag
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/CargoApp.cs
- 50 lines; kind=MonoBehaviour; type=CargoApp; bases=UIApp; ancestry=UIApp/MonoBehaviour; ns=KSP.UI.Screens
- usings: UnityEngine
- unity: Transform=1; unity_score=33
- ksp refs: none; fan_in=0 fan_out=3
- methods(7): DisplayApp, GetAppScenes, GetAppScreenPos, HideApp, OnAppAboutToStart, OnAppDestroy, OnAppInitialized
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/ConstructionApp.cs
- 50 lines; kind=MonoBehaviour; type=ConstructionApp; bases=UIApp; ancestry=UIApp/MonoBehaviour; ns=KSP.UI.Screens
- usings: UnityEngine
- unity: Transform=1; unity_score=33
- ksp refs: none; fan_in=0 fan_out=3
- methods(7): DisplayApp, GetAppScenes, GetAppScreenPos, HideApp, OnAppAboutToStart, OnAppDestroy, OnAppInitialized
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/ContractsApp.cs
- 358 lines; kind=MonoBehaviour; type=ContractsApp; bases=UIApp; ancestry=UIApp/MonoBehaviour; ns=KSP.UI.Screens
- usings: Contracts, KSP.Localization, UnityEngine, UnityEngine.UI
- unity: GameObject=6, Coroutine=5, Transform=4, Instantiate/Destroy=4, SetActive=3, UnityEngine.UI=2; unity_score=56
- ksp refs: Contract=30, GameEvents=18, HighLogic=4, Localizer=3, ContractParameter=3, Game=2; fan_in=1 fan_out=17
- methods(24): AddContract, AddSubParameter, Awake, CreateContractsList, CreateItem, CreateParameterList, DisplayApp, GetAppScenes, GetAppScreenPos, HideApp, OnAppAboutToStart, OnAppDestroy, OnAppInitialized, OnContractsLoaded, RefreshContract, RefreshContractParameter, RefreshContractRequest, RefreshContractRequested, RemoveContract, UpdateDaemon, UpdateLauncherButtonPlayAnim, UpdateLauncherButtonStopAnim, onGamePause, onGameUnPause
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/CraftBrowserDialog.cs **MUST-OPEN (>800 lines)**
- 1544 lines; kind=MonoBehaviour; type=CraftBrowserDialog; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.Localization, KSP.UI.Util, SaveUpgradePipeline, Steamworks, TMPro, UnityEngine, UnityEngine.Networking, UnityEngine.UI
- unity: UnityEngine.UI=43, SetActive=39, GameObject=39, Steam=33, TMPro=16, Analytics=13; unity_score=75
- ksp refs: Localizer=36, HighLogic=5, ConfigNode=5, GameSettings=3, InputLockManager=2, Game=1; fan_in=19 fan_out=43
- methods(66): AddCraftEntryWidget, BuildPlayerCraftList, BuildSteamCraftList, BuildSteamSubscribedCraftList, BuildStockCraftList, CancelledCallback, ClearCraftEntry, ClearCraftList, ClearOldCraftEntries, ClearSelection, ConfirmLoadCraft, Dismiss, FilterUpdateSelectables, GetAllGameFiles, GetMissionCraftFiles, GetPlayerCraftFiles, Hide, HideForLater, InitializeMenuNavigation, LoadSteamItemPreviewURL, OnBtnSteamOverlay, OnDeleteConfirm, OnDestroy, OnDisable, OnEntrySelected, OnSelectionChanged, PromptDeleteFileConfirm, ReDisplay, RemoveCreateCraftEntry, SaveCraftSteamUnsubscribeWarning, SearchOption_OnChanged, SelectConfigNodeCallback, SelectFileCallback, Show, Spawn, Start, SteamItemSubscribed, Update, UpdateCraftListUI, UpdateExplicitNavigation, UpdateSelectables, UpdateSteamListOfCraft, UpdateSteamSubscribedItems, getSteamWorkshopCraft, onBtnSteamItemDetails ...
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/CraftEntry.cs
- 563 lines; kind=MonoBehaviour; type=CraftEntry; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: Expansions, KSP.Localization, Steamworks, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.Networking, UnityEngine.UI
- unity: UnityEngine.UI=13, GameObject=9, TMPro=8, SetActive=6, EventSystems=6, GetComponent=5; unity_score=70
- ksp refs: Localizer=14, ShipConstruct=3, ConfigNode=3, HighLogic=2, Game=1, Funding=1; fan_in=5 fan_out=21
- methods(23): AttachListener, Awake, Create, GetCostTextColorTag, GetDisplayPathFrom, GetSteamFileId, GetStockThumbnailPath, GetThumbURL, GetThumbnail, HidePath, Init, IsExpansion, LoadSteamItemPreviewURL, OnPointerEnter, OnPointerExit, OnScroll, SetDirectoryController, ShowPath, Terminate, UIUpdate, UIUpdateSteamField, UpdateWidgetSize, onValueChanged
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/CraftProfileInfo.cs
- 410 lines; kind=class; type=CraftProfileInfo; bases=IConfigNode; ancestry=IConfigNode; ns=KSP.UI.Screens
- usings: Expansions, Expansions.Missions, Expansions.Missions.Runtime, KSP.Localization, UnityEngine
- unity: none; unity_score=5
- ksp refs: ConfigNode=11, ShipConstruct=6, MissionSystem=6, ResearchAndDevelopment=3, PartLoader=3, Localizer=3; fan_in=8 fan_out=20
- methods(12): CraftHasErrors, GenerateExcludedParts, GetErrorMessage, GetLastWriteTime, GetSFSMD5, GetSaveData, Load, LoadDetailsFromCraftFile, LoadFromMetaFile, PrepareCraftMetaFileLoad, Save, SaveToMetaFile
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/CraftSearch.cs
- 185 lines; kind=MonoBehaviour; type=CraftSearch; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: GameObject=8, GetComponent=6, Coroutine=6, SetActive=4, UnityEngine.UI=3, Transform=3; unity_score=80
- ksp refs: InputLockManager=2, Localizer=1; fan_in=3 fan_out=7
- methods(12): Awake, CraftMatchesSearch, HasFiltered, OnDestroy, OnDisable, SearchField_OnClick, SearchField_OnEndEdit, SearchField_OnValueChange, SearchRoutine, SearchStart, Start, StopSearch
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/CurrencyWidgetsApp.cs
- 111 lines; kind=MonoBehaviour; type=CurrencyWidgetsApp; bases=UIApp; ancestry=UIApp/MonoBehaviour; ns=KSP.UI.Screens
- usings: UnityEngine, UnityEngine.UI
- unity: Transform=10, GameObject=9, UnityEngine.UI=4, Instantiate/Destroy=4, GetComponent=3, SetActive=2; unity_score=89
- ksp refs: ResearchAndDevelopment=2, Reputation=2, Funding=2; fan_in=0 fan_out=8
- methods(9): DespawnWidgets, DisplayApp, GetAppScenes, GetAppScreenPos, HideApp, OnAppAboutToStart, OnAppDestroy, OnAppInitialized, SpawnWidgets
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/AddDebugScreens.cs
- 56 lines; kind=MonoBehaviour; type=AddDebugScreens; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar
- usings: Expansions, UnityEngine
- unity: UnityEngine.UI=1, MonoBehaviour=1, Lifecycle=1; unity_score=51
- ksp refs: none; fan_in=1 fan_out=3
- methods(1): Start
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/DebugScreen.cs
- 386 lines; kind=MonoBehaviour; type=DebugScreen; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar
- usings: Expansions.Missions, KSP.UI.Screens.DebugToolbar.Screens.Cheats, TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=12, GetComponent=11, SetActive=5, GameObject=5, Lifecycle=3, TMPro=1; unity_score=65
- ksp refs: HighLogic=4, InputLockManager=2, GameEvents=2, Game=1; fan_in=2 fan_out=24
- methods(14): AddContentItem, AddContentScreen, AddItem, Awake, GetItem, Hide, OnDestroy, OnExitClick, OnSelectItem, ResetCheats, SelectItem, Show, Update, onGameSceneLoad
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/DebugScreenCheats.cs
- 45 lines; kind=MonoBehaviour; type=DebugScreenCheats; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=11, Lifecycle=2, TMPro=1, MonoBehaviour=1; unity_score=94
- ksp refs: none; fan_in=0 fan_out=2
- methods(2): Awake, OnDestroy
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/DebugScreenConsole.cs
- 347 lines; kind=MonoBehaviour; type=DebugScreenConsole; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar
- usings: TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: Lifecycle=6, Coroutine=4, EventSystems=3, UnityEngine.UI=2, KeyCode=2, Input.=2; unity_score=57
- ksp refs: InputLockManager=2, GameEvents=2, HighLogic=1, GameSettings=1; fan_in=1 fan_out=12
- methods(20): AddConsoleCommand, Awake, FormatLogEntry, GetBracketedColorCode, GetCommand, InitializeLog, OnConsoleCommand, OnDestroy, OnDisable, OnEnable, OnGameSceneLoadRequested, OnInputEndEdit, OnMemoryLogUpdated, OnSubmitclick, ParseInputString, RemoveConsoleCommand, ScrollDown, Start, SubmitCommand, Update
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/DebugScreenInputLocks.cs
- 24 lines; kind=MonoBehaviour; type=DebugScreenInputLocks; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar
- usings: TMPro, UnityEngine
- unity: TMPro=2, Lifecycle=2, MonoBehaviour=1; unity_score=77
- ksp refs: none; fan_in=0 fan_out=2
- methods(2): Awake, OnDestroy
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/DebugScreenScreenToggle.cs
- 54 lines; kind=MonoBehaviour; type=DebugScreenScreenToggle; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar
- usings: UnityEngine, UnityEngine.UI
- unity: SetActive=8, GameObject=2, UnityEngine.UI=1, MonoBehaviour=1, Lifecycle=1; unity_score=82
- ksp refs: none; fan_in=0 fan_out=1
- methods(3): Awake, OnToggleClick, SetShown
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/DebugScreenSpawner.cs
- 87 lines; kind=MonoBehaviour; type=DebugScreenSpawner; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar
- usings: UnityEngine, UnityEngine.SceneManagement
- unity: Lifecycle=4, SceneManager=3, Transform=2, MonoBehaviour=1, KeyCode=1, Instantiate/Destroy=1; unity_score=75
- ksp refs: InputLockManager=1, GameSettings=1; fan_in=2 fan_out=5
- methods(7): Awake, OnDestroy, OnSceneLoaded, ShowDebugScreen, ShowTab, Start, Update
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/ScreenDatabaseAssemblies.cs
- 14 lines; kind=MonoBehaviour; type=ScreenDatabaseAssemblies; bases=ScreenDatabaseList; ancestry=ScreenDatabaseList/MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar
- usings: -
- unity: none; unity_score=15
- ksp refs: none; fan_in=0 fan_out=3
- methods(1): LoadDatabaseListItems
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/ScreenDatabaseAudio.cs
- 14 lines; kind=MonoBehaviour; type=ScreenDatabaseAudio; bases=ScreenDatabaseList; ancestry=ScreenDatabaseList/MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar
- usings: -
- unity: none; unity_score=15
- ksp refs: GameDatabase=2; fan_in=0 fan_out=3
- methods(1): LoadDatabaseListItems
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/ScreenDatabaseConfig.cs
- 17 lines; kind=MonoBehaviour; type=ScreenDatabaseConfig; bases=ScreenDatabaseList; ancestry=ScreenDatabaseList/MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar
- usings: -
- unity: none; unity_score=15
- ksp refs: GameDatabase=1; fan_in=0 fan_out=4
- methods(1): LoadDatabaseListItems
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/ScreenDatabaseList.cs
- 40 lines; kind=MonoBehaviour; type=ScreenDatabaseList; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar
- usings: UnityEngine, UnityEngine.UI
- unity: Lifecycle=2, Coroutine=2, UnityEngine.UI=1, MonoBehaviour=1; unity_score=72
- ksp refs: none; fan_in=5 fan_out=2
- methods(4): Awake, LoadDatabaseListItems, OnEnable, ScrollUp
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/ScreenDatabaseModels.cs
- 14 lines; kind=MonoBehaviour; type=ScreenDatabaseModels; bases=ScreenDatabaseList; ancestry=ScreenDatabaseList/MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar
- usings: -
- unity: none; unity_score=15
- ksp refs: GameDatabase=2; fan_in=0 fan_out=3
- methods(1): LoadDatabaseListItems
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/ScreenDatabaseStrings.cs
- 95 lines; kind=MonoBehaviour; type=ScreenDatabaseStrings; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: TMPro=4, UnityEngine.UI=3, Lifecycle=2, MonoBehaviour=1; unity_score=69
- ksp refs: Localizer=9; fan_in=0 fan_out=4
- methods(4): FormatKeyValue, OnDestroy, OnSearchClicked, Start
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/ScreenDatabaseTextures.cs
- 14 lines; kind=MonoBehaviour; type=ScreenDatabaseTextures; bases=ScreenDatabaseList; ancestry=ScreenDatabaseList/MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar
- usings: -
- unity: none; unity_score=15
- ksp refs: GameDatabase=2; fan_in=0 fan_out=3
- methods(1): LoadDatabaseListItems
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/AllowPartClipping.cs
- 15 lines; kind=MonoBehaviour; type=AllowPartClipping; bases=DebugScreenToggle; ancestry=DebugScreenToggle/MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- usings: -
- unity: none; unity_score=15
- ksp refs: none; fan_in=1 fan_out=3
- methods(2): OnToggleChanged, SetupValues
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/BiomesVisibleInMap.cs
- 47 lines; kind=MonoBehaviour; type=BiomesVisibleInMap; bases=DebugScreenToggle; ancestry=DebugScreenToggle/MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- usings: UnityEngine
- unity: Material=3, Shader=2, Renderer=2, GetComponent=1, GameObject=1; unity_score=83
- ksp refs: FlightGlobals=2, CelestialBody=1; fan_in=1 fan_out=5
- methods(3): OnToggleChanged, SetBiomesVisible, SetupValues
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/CheatsCareer.cs
- 416 lines; kind=MonoBehaviour; type=CheatsCareer; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- usings: KSP.Localization, ModuleWheels, TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=17, TMPro=15, GetComponent=12, Lifecycle=4, MonoBehaviour=1; unity_score=71
- ksp refs: ResearchAndDevelopment=14, Reputation=11, Funding=11, HighLogic=6, GameEvents=6, FlightGlobals=6; fan_in=0 fan_out=22
- methods(31): Awake, FixRepairableParts, IsVesselWithinRange, OnDestroy, OnFundsChanged, OnFundsNegBigClick, OnFundsNegClick, OnFundsPosBigClick, OnFundsPosClick, OnMaxFacilityClick, OnMaxTechClick, OnMaxXPClick, OnRepNegBigClick, OnRepNegClick, OnRepPosBigClick, OnRepPosClick, OnReputationChanged, OnScienceChanged, OnScienceNegBigClick, OnScienceNegClick, OnSciencePosBigClick, OnSciencePosClick, SetFundingActive, SetMaxFacilityActive, SetMaxProgressActive, SetMaxTechActive, SetMaxXPActive, SetReputationActive, SetScienceActive, Start, Update
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/CheatsComets.cs
- 381 lines; kind=MonoBehaviour; type=CheatsComets; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- usings: SentinelMission, TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=25, TMPro=6, SetActive=4, GameObject=4, Lifecycle=3, Time.=2; unity_score=71
- ksp refs: FlightGlobals=4, Vessel=2; fan_in=0 fan_out=7
- methods(30): Awake, OnCollidersClick, OnComaClick, OnDustClick, OnDustMultiplierChange, OnDustSizeMultiplierChange, OnDustTailClick, OnGeyserMultiplierChange, OnGeysersClick, OnInterstellarClick, OnIonMultiplierChange, OnIonTailClick, OnKillClick, OnLongClick, OnMidClick, OnRandomClick, OnReprimeParticles, OnSentinelClick, OnSentinelInterstellarClick, OnSentinelLongClick, OnSentinelMidClick, OnSentinelShortClick, OnShortClick, OnSnowMultiplierChange, OnVesselBackClick, OnVesselForwardClick, SetSelectedSentinelText, Start, Update, UpdateSentinelOptions
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/CheatsObjectThrower.cs
- 193 lines; kind=MonoBehaviour; type=CheatsObjectThrower; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: GameObject=12, Rigidbody=7, UnityEngine.UI=6, TMPro=5, GetComponent=4, Lifecycle=3; unity_score=86
- ksp refs: FlightCamera=5, Localizer=2; fan_in=0 fan_out=5
- methods(10): Awake, OnArmedToggle, OnMassSet, OnObjectNext, OnObjectPrev, OnSizeSet, OnSpeedSet, SetUI, Start, Update
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/DestroyAfterTime.cs
- 24 lines; kind=MonoBehaviour; type=DestroyAfterTime; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- usings: UnityEngine
- unity: Time.=2, Lifecycle=2, GameObject=2, MonoBehaviour=1, Instantiate/Destroy=1; unity_score=80
- ksp refs: none; fan_in=0 fan_out=1
- methods(2): Start, Update
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/HackGravity.cs
- 134 lines; kind=MonoBehaviour; type=HackGravity; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=3, Lifecycle=2, TMPro=1, MonoBehaviour=1; unity_score=50
- ksp refs: FlightGlobals=5, CelestialBody=4; fan_in=1 fan_out=5
- methods(10): Awake, CreateOriginalGeeForces, OnResetClick, OnSliderChanged, OnToggleChanged, SetGravityFactor, SetHack, SetText, SetupValues, Update
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/IgnoreEVAConstructionMassLimit.cs
- 15 lines; kind=MonoBehaviour; type=IgnoreEVAConstructionMassLimit; bases=DebugScreenToggle; ancestry=DebugScreenToggle/MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- usings: -
- unity: none; unity_score=15
- ksp refs: none; fan_in=1 fan_out=3
- methods(2): OnToggleChanged, SetupValues
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/IgnoreKerbalInventoryLimits.cs
- 15 lines; kind=MonoBehaviour; type=IgnoreKerbalInventoryLimits; bases=DebugScreenToggle; ancestry=DebugScreenToggle/MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- usings: -
- unity: none; unity_score=15
- ksp refs: none; fan_in=0 fan_out=3
- methods(2): OnToggleChanged, SetupValues
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/IgnoreMaxTemperature.cs
- 15 lines; kind=MonoBehaviour; type=IgnoreMaxTemperature; bases=DebugScreenToggle; ancestry=DebugScreenToggle/MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- usings: -
- unity: none; unity_score=15
- ksp refs: none; fan_in=1 fan_out=3
- methods(2): OnToggleChanged, SetupValues
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/InfiniteElectricity.cs
- 15 lines; kind=MonoBehaviour; type=InfiniteElectricity; bases=DebugScreenToggle; ancestry=DebugScreenToggle/MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- usings: -
- unity: none; unity_score=15
- ksp refs: none; fan_in=1 fan_out=3
- methods(2): OnToggleChanged, SetupValues
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/InfinitePropellant.cs
- 15 lines; kind=MonoBehaviour; type=InfinitePropellant; bases=DebugScreenToggle; ancestry=DebugScreenToggle/MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- usings: -
- unity: none; unity_score=15
- ksp refs: none; fan_in=1 fan_out=3
- methods(2): OnToggleChanged, SetupValues
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/MaxProgression.cs
- 27 lines; kind=MonoBehaviour; type=MaxProgression; bases=MonoBehaviour,IPointerClickHandler,IEventSystemHandler; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- usings: KSP.Localization, UnityEngine, UnityEngine.EventSystems
- unity: EventSystems=4, MonoBehaviour=1; unity_score=80
- ksp refs: Localizer=2, FlightGlobals=1; fan_in=0 fan_out=6
- methods(1): OnPointerClick
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/MiddleMouseClickSetPosition.cs
- 15 lines; kind=MonoBehaviour; type=MiddleMouseClickSetPosition; bases=DebugScreenToggle; ancestry=DebugScreenToggle/MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- usings: -
- unity: none; unity_score=15
- ksp refs: none; fan_in=1 fan_out=3
- methods(2): OnToggleChanged, SetupValues
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/NoCrashDamage.cs
- 15 lines; kind=MonoBehaviour; type=NoCrashDamage; bases=DebugScreenToggle; ancestry=DebugScreenToggle/MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- usings: -
- unity: none; unity_score=15
- ksp refs: none; fan_in=1 fan_out=3
- methods(2): OnToggleChanged, SetupValues
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/NonStrictPartAttachment.cs
- 15 lines; kind=MonoBehaviour; type=NonStrictPartAttachment; bases=DebugScreenToggle; ancestry=DebugScreenToggle/MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- usings: -
- unity: none; unity_score=15
- ksp refs: none; fan_in=1 fan_out=3
- methods(2): OnToggleChanged, SetupValues
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/PauseOnVesselUnpack.cs
- 15 lines; kind=MonoBehaviour; type=PauseOnVesselUnpack; bases=DebugScreenToggle; ancestry=DebugScreenToggle/MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- usings: -
- unity: none; unity_score=15
- ksp refs: none; fan_in=1 fan_out=3
- methods(2): OnToggleChanged, SetupValues
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/SetOrbit.cs
- 299 lines; kind=MonoBehaviour; type=SetOrbit; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=7, TMPro=3, Lifecycle=3, MonoBehaviour=1; unity_score=49
- ksp refs: FlightGlobals=23, Localizer=13, Vessel=6, CelestialBody=2, FloatingOrigin=1; fan_in=0 fan_out=10
- methods(13): Awake, CheckForErrors, OnBodyBackClick, OnBodyForwardClick, OnDestroy, OnRendezvousClick, OnSetOrbitClick, OnVesselBackClick, OnVesselForwardClick, ResetErrorMsg, SetSelectedBodyText, SetSelectedVesselText, Start
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/SetPosition.cs
- 383 lines; kind=MonoBehaviour; type=SetPosition; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- usings: EdyCommonTools, KSP.Localization, KSP.UI.Screens.Flight, TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=8, Lifecycle=5, GameObject=5, SetActive=4, TMPro=3, Input.=3; unity_score=66
- ksp refs: FlightGlobals=36, Localizer=12, GameEvents=7, Vessel=6, CelestialBody=3, MapView=2; fan_in=0 fan_out=21
- methods(22): Awake, CheckForErrors, CheckForHiddenElements, DisableActiveVesselEaseIn, FormatValues, GetSelectedBodyIndex, GetSugestedAltitude, InitializeValues, OnBodyBackClick, OnBodyForwardClick, OnDestroy, OnEnable, OnSceneExit, OnSetPositionClick, OnVesselChanged, OnVesselSituationChanged, ResetErrorMsg, SetPositionValues, SetSelectedBodyText, Start, Update, UpdateSliderValue
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/StringsMissingKeysToggle.cs
- 17 lines; kind=MonoBehaviour; type=StringsMissingKeysToggle; bases=DebugScreenToggle; ancestry=DebugScreenToggle/MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- usings: KSP.Localization
- unity: none; unity_score=15
- ksp refs: Localizer=2; fan_in=0 fan_out=3
- methods(2): OnToggleChanged, SetupValues
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/StringsOverrideMELock.cs
- 18 lines; kind=MonoBehaviour; type=StringsOverrideMELock; bases=DebugScreenToggle; ancestry=DebugScreenToggle/MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- usings: KSP.Localization
- unity: none; unity_score=15
- ksp refs: Localizer=2, GameEvents=1; fan_in=0 fan_out=4
- methods(2): OnToggleChanged, SetupValues
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/StringsShowKeysToggle.cs
- 17 lines; kind=MonoBehaviour; type=StringsShowKeysToggle; bases=DebugScreenToggle; ancestry=DebugScreenToggle/MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- usings: KSP.Localization
- unity: none; unity_score=15
- ksp refs: Localizer=2; fan_in=0 fan_out=3
- methods(2): OnToggleChanged, SetupValues
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/UnbreakableJoints.cs
- 15 lines; kind=MonoBehaviour; type=UnbreakableJoints; bases=DebugScreenToggle; ancestry=DebugScreenToggle/MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- usings: -
- unity: none; unity_score=15
- ksp refs: none; fan_in=1 fan_out=3
- methods(2): OnToggleChanged, SetupValues
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Contract/ScreenAddContracts.cs
- 92 lines; kind=MonoBehaviour; type=ScreenAddContracts; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Contract
- usings: Contracts, KSP.Localization, UnityEngine
- unity: Lifecycle=3, Instantiate/Destroy=3, Transform=2, UnityEngine.UI=1, MonoBehaviour=1, GameObject=1; unity_score=65
- ksp refs: GameEvents=4, Localizer=1, Game=1, Contract=1, ConfigNode=1; fan_in=0 fan_out=8
- methods(9): ClearItems, CreateError, CreateItem, OnDestroy, OnGameStateCreated, OnGameStateLoaded, RefreshItems, Start, Update
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Contract/ScreenContractExistingItem.cs
- 142 lines; kind=MonoBehaviour; type=ScreenContractExistingItem; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Contract
- usings: Contracts, KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: TMPro=6, SetActive=3, GameObject=3, UnityEngine.UI=2, MonoBehaviour=1, Lifecycle=1; unity_score=68
- ksp refs: Contract=23, Localizer=7; fan_in=1 fan_out=7
- methods(5): OnLeftButtonClicked, OnRightButtonClicked, Setup, SetupError, Start
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Contract/ScreenContractList.cs
- 173 lines; kind=MonoBehaviour; type=ScreenContractList; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Contract
- usings: Contracts, KSP.Localization, UnityEngine
- unity: Lifecycle=3, Instantiate/Destroy=3, Transform=2, UnityEngine.UI=1, MonoBehaviour=1, GameObject=1; unity_score=50
- ksp refs: Contract=25, GameEvents=22, Localizer=5, Game=1, ConfigNode=1; fan_in=1 fan_out=8
- methods(11): ClearItems, CreateError, CreateItem, OnContractsModified, OnDestroy, OnGameStateCreated, OnGameStateLoaded, RefreshItems, SetDirty, Start, Update
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Contract/ScreenContractNewItem.cs
- 94 lines; kind=MonoBehaviour; type=ScreenContractNewItem; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Contract
- usings: Contracts, TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=3, SetActive=3, GameObject=3, TMPro=2, MonoBehaviour=1, Lifecycle=1; unity_score=74
- ksp refs: Contract=6; fan_in=1 fan_out=5
- methods(7): GenerateContract, OnExceptionalClicked, OnSignificantClicked, OnTrivialClicked, Setup, SetupError, Start
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Contract/ScreenContractTools.cs
- 69 lines; kind=MonoBehaviour; type=ScreenContractTools; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Contract
- usings: Contracts, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=4, Lifecycle=2, MonoBehaviour=1; unity_score=66
- ksp refs: Contract=1; fan_in=0 fan_out=3
- methods(7): OnClearCurrentClicked, OnClearFinishedClicked, OnRegenerateCurrentClicked, OnResetWeightsClicked, SetInteractable, Start, Update
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Database/ScreenDatabaseOverview.cs
- 149 lines; kind=MonoBehaviour; type=ScreenDatabaseOverview; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Database
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: TMPro=9, Coroutine=3, UnityEngine.UI=2, Lifecycle=2, MonoBehaviour=1; unity_score=72
- ksp refs: GameDatabase=15, Localizer=4, PartLoader=3, HighLogic=2, GameEvents=2; fan_in=0 fan_out=11
- methods(8): CheckInteractable, FlushCounts, OnDestroy, OnReloadClicked, OnReloadTextsClicked, RecompileAssets, ResetCounts, Start
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Debug/CalcType.cs
- 14 lines; kind=enum; type=CalcType; bases=-; ancestry=-; ns=KSP.UI.Screens.DebugToolbar.Screens.Debug
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=6 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Debug/DeltaVEngineInfoListExtensions.cs
- 26 lines; kind=static class; type=DeltaVEngineInfoListExtensions; bases=-; ancestry=-; ns=KSP.UI.Screens.DebugToolbar.Screens.Debug
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=0 fan_out=3
- methods(1): Get
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Debug/DeltaVPropellantInfoListExtensions.cs
- 26 lines; kind=static class; type=DeltaVPropellantInfoListExtensions; bases=-; ancestry=-; ns=KSP.UI.Screens.DebugToolbar.Screens.Debug
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=0 fan_out=3
- methods(1): Get
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Debug/DeltaVStageInfoListExtensions.cs
- 26 lines; kind=static class; type=DeltaVStageInfoListExtensions; bases=-; ancestry=-; ns=KSP.UI.Screens.DebugToolbar.Screens.Debug
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=0 fan_out=3
- methods(1): Get
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Debug/GraphFPS.cs
- 100 lines; kind=MonoBehaviour; type=GraphFPS; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Debug
- usings: TMPro, UnityEngine, Vectrosity
- unity: Coroutine=5, Lifecycle=4, TMPro=3, UnityEngine.UI=1, MonoBehaviour=1; unity_score=72
- ksp refs: none; fan_in=0 fan_out=3
- methods(7): Awake, OnDisable, OnEnable, Start, UpdateCoroutine, UpdateDisplay, UpdateText
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Debug/Memory.cs
- 91 lines; kind=MonoBehaviour; type=Memory; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Debug
- usings: TMPro, UnityEngine, UnityEngine.Profiling
- unity: TMPro=6, Coroutine=5, Lifecycle=2, MonoBehaviour=1; unity_score=75
- ksp refs: none; fan_in=0 fan_out=2
- methods(5): GetMemoryText, OnDisable, OnEnable, UpdateCoroutine, UpdateDisplay
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Debug/ScreenDeltaVEngineInfo.cs
- 63 lines; kind=MonoBehaviour; type=ScreenDeltaVEngineInfo; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Debug
- usings: KSP.Localization, TMPro, UnityEngine
- unity: TMPro=7, MonoBehaviour=1; unity_score=81
- ksp refs: Localizer=6; fan_in=3 fan_out=6
- methods(1): UpdateData
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Debug/ScreenDeltaVInfo.cs
- 306 lines; kind=MonoBehaviour; type=ScreenDeltaVInfo; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Debug
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.SceneManagement, UnityEngine.UI
- unity: UnityEngine.UI=6, GameObject=6, TMPro=5, SceneManager=4, Lifecycle=4, Instantiate/Destroy=2; unity_score=68
- ksp refs: GameEvents=10, Localizer=9, HighLogic=6, Vessel=4, FlightGlobals=4, GameSettings=3; fan_in=0 fan_out=19
- methods(14): Awake, CacheTypeStrings, ChangeVessel, ClearScreenObjects, FindVessel, OnDestroy, OnLogVerboseToggle, OnSceneLoaded, SetSiblingsUIIndex, Start, Update, onEditorLoad, onEditorPodPicked, onEditorRestoreState
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Debug/ScreenDeltaVPartsListInfo.cs
- 16 lines; kind=MonoBehaviour; type=ScreenDeltaVPartsListInfo; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Debug
- usings: TMPro, UnityEngine
- unity: TMPro=1, MonoBehaviour=1; unity_score=59
- ksp refs: none; fan_in=1 fan_out=2
- methods(1): UpdateData
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Debug/ScreenDeltaVPropellantInfo.cs
- 41 lines; kind=MonoBehaviour; type=ScreenDeltaVPropellantInfo; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Debug
- usings: TMPro, UnityEngine
- unity: TMPro=3, MonoBehaviour=1; unity_score=73
- ksp refs: none; fan_in=2 fan_out=4
- methods(1): UpdateData
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Debug/ScreenDeltaVStageInfo.cs
- 85 lines; kind=MonoBehaviour; type=ScreenDeltaVStageInfo; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Debug
- usings: KSP.Localization, TMPro, UnityEngine
- unity: TMPro=13, MonoBehaviour=1; unity_score=87
- ksp refs: Localizer=6; fan_in=2 fan_out=6
- methods(1): UpdateData
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Debug/ScreenFlightInfo.cs
- 194 lines; kind=MonoBehaviour; type=ScreenFlightInfo; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Debug
- usings: KSP.Localization, TMPro, UnityEngine
- unity: TMPro=16, Lifecycle=5, Time.=4, SetActive=1, MonoBehaviour=1, GameObject=1; unity_score=78
- ksp refs: Localizer=10, FlightGlobals=8, GameEvents=6, Vessel=3, Planetarium=2; fan_in=0 fan_out=8
- methods(13): Awake, FixedUpdate, GetActiveVesselName, LateUpdate, OnDestroy, SetLocation, SetPhysics, SetUT, SetVessel, SetVesselSlopeAngle, Start, onGamePause, onGameUnPause
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Debug/ScreenVesselMassInfo.cs
- 194 lines; kind=MonoBehaviour; type=ScreenVesselMassInfo; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Debug
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.SceneManagement, UnityEngine.UI
- unity: SceneManager=4, GameObject=4, Lifecycle=3, UnityEngine.UI=2, Transform=2, TMPro=2; unity_score=68
- ksp refs: GameEvents=15, ShipConstruct=3, Part=3, EditorLogic=3, Vessel=2, HighLogic=2; fan_in=0 fan_out=13
- methods(13): Awake, ChangeCrew, ChangeShip, ChangeVessel, ClearScreenObjects, FindVessel, OnDestroy, OnSceneLoaded, Start, UpdateList, onEditorLoad, onEditorPodPicked, onEditorRestoreState
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Debug/ScreenVesselMassPartInfo.cs
- 58 lines; kind=MonoBehaviour; type=ScreenVesselMassPartInfo; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Debug
- usings: TMPro, UnityEngine
- unity: TMPro=9, SetActive=1, MonoBehaviour=1, GameObject=1; unity_score=90
- ksp refs: Part=2; fan_in=1 fan_out=3
- methods(1): UpdateData
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Debug/ShowInputLocks.cs
- 37 lines; kind=MonoBehaviour; type=ShowInputLocks; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Debug
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: Lifecycle=2, UnityEngine.UI=1, TMPro=1, MonoBehaviour=1; unity_score=71
- ksp refs: InputLockManager=2; fan_in=0 fan_out=3
- methods(4): Awake, GetText, OnClearClick, Update
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Debug/VersionInfo.cs
- 22 lines; kind=MonoBehaviour; type=VersionInfo; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Debug
- usings: Expansions, KSP.Localization, TMPro, UnityEngine
- unity: TMPro=1, MonoBehaviour=1, Lifecycle=1; unity_score=67
- ksp refs: Localizer=1; fan_in=0 fan_out=5
- methods(1): Start
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Debug/VesselDebugDeltaVEngineInfo.cs
- 131 lines; kind=class; type=VesselDebugDeltaVEngineInfo; bases=-; ancestry=-; ns=KSP.UI.Screens.DebugToolbar.Screens.Debug
- usings: UnityEngine
- unity: GameObject=17, Transform=7, SetActive=4, Instantiate/Destroy=4, GetComponent=3, UnityEngine.UI=2; unity_score=70
- ksp refs: none; fan_in=1 fan_out=7
- methods(5): Create, Destroy, SetActive, SetPropellantUISiblings, Update
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Debug/VesselDebugDeltaVInfo.cs
- 177 lines; kind=class; type=VesselDebugDeltaVInfo; bases=-; ancestry=-; ns=KSP.UI.Screens.DebugToolbar.Screens.Debug
- usings: UnityEngine
- unity: Transform=8, GameObject=8, SetActive=6, Instantiate/Destroy=5, GetComponent=2, UnityEngine.UI=1; unity_score=58
- ksp refs: none; fan_in=1 fan_out=9
- methods(5): Create, Destroy, SetActive, SetSiblingsUIIndex, Update
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/DebugScreenInputDouble.cs
- 50 lines; kind=MonoBehaviour; type=DebugScreenInputDouble; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens
- usings: TMPro, UnityEngine
- unity: TMPro=2, Lifecycle=2, MonoBehaviour=1; unity_score=64
- ksp refs: none; fan_in=2 fan_out=3
- methods(3): Awake, OnInputChanged, Start
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/DebugScreenSlider.cs
- 52 lines; kind=MonoBehaviour; type=DebugScreenSlider; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=1, TMPro=1, MonoBehaviour=1, Lifecycle=1; unity_score=58
- ksp refs: none; fan_in=0 fan_out=2
- methods(5): Awake, OnSliderChanged, SetSlider, SetSliderText, SetupValues
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/DebugScreenToggle.cs
- 58 lines; kind=MonoBehaviour; type=DebugScreenToggle; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=1, TMPro=1, MonoBehaviour=1, Lifecycle=1; unity_score=56
- ksp refs: none; fan_in=16 fan_out=2
- methods(6): Awake, OnToggleChanged, Set, SetToggle, SetToggleText, SetupValues
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/GamePlay/Difficulty.cs
- 67 lines; kind=MonoBehaviour; type=Difficulty; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.GamePlay
- usings: UnityEngine
- unity: Lifecycle=2, MonoBehaviour=1; unity_score=41
- ksp refs: HighLogic=13; fan_in=0 fan_out=4
- methods(2): Start, Update
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/GamePlay/KerbalScreen.cs
- 210 lines; kind=MonoBehaviour; type=KerbalScreen; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.GamePlay
- usings: UnityEngine
- unity: GameObject=5, SetActive=4, Lifecycle=4, Instantiate/Destroy=3, UnityEngine.UI=1, Transform=1; unity_score=58
- ksp refs: GameEvents=10, ProtoCrewMember=9, HighLogic=4, Game=1, ConfigNode=1; fan_in=0 fan_out=7
- methods(15): AddKerbal, Awake, ClearList, CreateItem, OnDestroy, OnGameStateCreated, OnGameStateLoaded, OnKerbalAdded, OnKerbalRemoved, OnKerbalStatusChange, RemoveKerbal, Start, Update, UpdateKerbal, UpdateRoster
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/GamePlay/KerbalScreenItem.cs
- 33 lines; kind=MonoBehaviour; type=KerbalScreenItem; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.GamePlay
- usings: TMPro, UnityEngine
- unity: TMPro=6, MonoBehaviour=1; unity_score=86
- ksp refs: ProtoCrewMember=4; fan_in=1 fan_out=3
- methods(1): Setup
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/GamePlay/ScreenKerbalCreate.cs
- 348 lines; kind=MonoBehaviour; type=ScreenKerbalCreate; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.GamePlay
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=13, TMPro=3, Coroutine=3, Lifecycle=2, MonoBehaviour=1; unity_score=56
- ksp refs: ProtoCrewMember=13, KerbalRoster=12, GameEvents=12, HighLogic=7, Localizer=6, ConfigNode=1; fan_in=0 fan_out=16
- methods(22): AddListeners, CheckForErrors, GameLoaded, InitializeLocals, KerbalsModified, Lock, OnBadassToggle, OnClassToggle, OnCourageLevelSet, OnDestroy, OnExperienceLevelSet, OnGenderToggle, OnNameEdit, OnRandomClicked, OnStupidityLevelSet, OnSubmitClicked, OnVeteranToggle, RefreshControls, SetAttributes, Start, SubmitConfirmation, Unlock
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Mission/ScreenAddMissions.cs
- 91 lines; kind=MonoBehaviour; type=ScreenAddMissions; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Mission
- usings: Expansions.Missions, Expansions.Missions.Runtime, UnityEngine
- unity: Lifecycle=3, Instantiate/Destroy=3, Transform=2, UnityEngine.UI=1, MonoBehaviour=1, GameObject=1; unity_score=65
- ksp refs: GameEvents=4, MissionSystem=3, Game=1, ConfigNode=1; fan_in=0 fan_out=7
- methods(9): ClearItems, CreateError, CreateItem, OnDestroy, OnGameStateCreated, OnGameStateLoaded, RefreshItems, Start, Update
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Mission/ScreenMissionExistingItem.cs
- 65 lines; kind=MonoBehaviour; type=ScreenMissionExistingItem; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Mission
- usings: Expansions.Missions, TMPro, UnityEngine, UnityEngine.UI
- unity: TMPro=6, UnityEngine.UI=2, SetActive=2, GameObject=2, MonoBehaviour=1, Lifecycle=1; unity_score=84
- ksp refs: none; fan_in=2 fan_out=4
- methods(5): OnLeftButtonClicked, OnRightButtonClicked, Setup, SetupError, Start
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Mission/ScreenMissionList.cs
- 140 lines; kind=MonoBehaviour; type=ScreenMissionList; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Mission
- usings: Contracts, Expansions.Missions, Expansions.Missions.Runtime, UnityEngine
- unity: Lifecycle=3, Instantiate/Destroy=3, Transform=2, UnityEngine.UI=1, MonoBehaviour=1, GameObject=1; unity_score=55
- ksp refs: GameEvents=22, Contract=19, MissionSystem=2, Game=1, ConfigNode=1; fan_in=0 fan_out=8
- methods(11): ClearItems, CreateError, CreateItem, OnContractsModified, OnDestroy, OnGameStateCreated, OnGameStateLoaded, RefreshItems, SetDirty, Start, Update
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Mission/ScreenMissionNewItem.cs
- 24 lines; kind=MonoBehaviour; type=ScreenMissionNewItem; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Mission
- usings: TMPro, UnityEngine
- unity: TMPro=2, MonoBehaviour=1, Lifecycle=1; unity_score=72
- ksp refs: none; fan_in=0 fan_out=3
- methods(2): SetupError, Start
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Mission/ScreenMissionTools.cs
- 18 lines; kind=MonoBehaviour; type=ScreenMissionTools; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Mission
- usings: UnityEngine, UnityEngine.UI
- unity: Lifecycle=2, UnityEngine.UI=1, MonoBehaviour=1; unity_score=70
- ksp refs: none; fan_in=0 fan_out=1
- methods(2): Start, Update
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Physics/ScreenAero.cs
- 123 lines; kind=MonoBehaviour; type=ScreenAero; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Physics
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=7, TMPro=4, MonoBehaviour=1, Lifecycle=1; unity_score=70
- ksp refs: PhysicsGlobals=23; fan_in=0 fan_out=4
- methods(9): AddListeners, OnBodyLiftSet, OnDataActionMenusToggle, OnDataGUIToggle, OnForceDisplayScaleSet, OnForcesInFlightToggle, OnGlobalLiftSet, OnLiftDragSet, Start
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Physics/ScreenDrag.cs
- 134 lines; kind=MonoBehaviour; type=ScreenDrag; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Physics
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=8, TMPro=3, MonoBehaviour=1, Lifecycle=1; unity_score=67
- ksp refs: PhysicsGlobals=32; fan_in=0 fan_out=3
- methods(11): AddListeners, CheckInteractable, OnAngularDragSet, OnApplyDragToggle, OnCubeMultiplierSet, OnDragAccelerationToggle, OnDragSpheresToggle, OnGlobalDragSet, OnNonPhysicalCoMToggle, OnNonPhysicalDragToggle, Start
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Physics/ScreenKerbalEVAMaterial.cs
- 175 lines; kind=MonoBehaviour; type=ScreenKerbalEVAMaterial; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Physics
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: TMPro=13, UnityEngine.UI=12, PhysicMaterial=12, MonoBehaviour=1, Lifecycle=1; unity_score=87
- ksp refs: PhysicsGlobals=7; fan_in=0 fan_out=3
- methods(7): AddListeners, ApplyChanges, SlidersUpdated, Start, UpdateBounceCombine, UpdateFrictionCombine, UpdateValuesFromPhysiscsGlobals
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Physics/ScreenPhysics.cs
- 61 lines; kind=MonoBehaviour; type=ScreenPhysics; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Physics
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=4, TMPro=1, MonoBehaviour=1, Lifecycle=1; unity_score=70
- ksp refs: PhysicsGlobals=8, GameSettings=2; fan_in=0 fan_out=4
- methods(5): OnAutostrutVisualizeToggled, OnLegacyOrbitTargetingToggled, OnLoadClicked, OnSaveClicked, Start
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Physics/ScreenThermal.cs
- 211 lines; kind=MonoBehaviour; type=ScreenThermal; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Physics
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=13, TMPro=7, MonoBehaviour=1, Lifecycle=1; unity_score=70
- ksp refs: PhysicsGlobals=43; fan_in=0 fan_out=3
- methods(15): AddListeners, OnConductionEnabledToggle, OnConductionFactorSet, OnConvectionEnabledToggle, OnConvectionFactorSet, OnDataActionMenusToggle, OnDataGUIToggle, OnDebugColorsToggle, OnGenerationFactorSet, OnInsolationFactorSet, OnLuminosityFactorSet, OnNewtonianFactorSet, OnRadiationEnabledToggle, OnRadiationFactorSet, Start
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Resources/Debugging.cs
- 101 lines; kind=MonoBehaviour; type=Debugging; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Resources
- usings: KSP.Localization, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=7, MonoBehaviour=1, Lifecycle=1; unity_score=65
- ksp refs: GameSettings=27, Localizer=2; fan_in=0 fan_out=5
- methods(9): AddListeners, OnErrorToggle, OnExceptionToggle, OnInstantFlushToggle, OnMissingKeysToggle, OnShowLocalizationKeysToggle, OnShowVesselNamingPartsToggle, OnUIMasterPixelPerfectToggle, Start
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Resources/ScreenResources.cs
- 44 lines; kind=MonoBehaviour; type=ScreenResources; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Resources
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=2, MonoBehaviour=1, Lifecycle=1; unity_score=63
- ksp refs: none; fan_in=0 fan_out=2
- methods(4): AddListeners, OnDebugInfoToggle, OnHeatGenerationToggle, Start
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Serenity/ScreenRobotics.cs
- 26 lines; kind=MonoBehaviour; type=ScreenRobotics; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Serenity
- usings: UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=1, MonoBehaviour=1, Lifecycle=1; unity_score=63
- ksp refs: PhysicsGlobals=2; fan_in=0 fan_out=2
- methods(3): AddListeners, OnDataActionMenusToggle, Start
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Serenity/ScreenROCs.cs
- 96 lines; kind=MonoBehaviour; type=ScreenROCs; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Serenity
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=3, Lifecycle=2, TMPro=1, MonoBehaviour=1; unity_score=58
- ksp refs: Localizer=2, GameSettings=2, GameEvents=2; fan_in=0 fan_out=7
- methods(7): AddListeners, OnDestroy, OnROCFinderToggle, OnROCScanPointsToggle, OnROCStatsEnabledToggle, OnStatsChanged, Start
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Serenity/ScreenSerenity.cs
- 16 lines; kind=MonoBehaviour; type=ScreenSerenity; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.DebugToolbar.Screens.Serenity
- usings: UnityEngine
- unity: MonoBehaviour=1, Lifecycle=1; unity_score=50
- ksp refs: none; fan_in=0 fan_out=1
- methods(2): AddListeners, Start
- auto suggestion: tier=C subsystem=Debug

## Assembly-CSharp/KSP/UI/Screens/DirectoryController.cs
- 756 lines; kind=MonoBehaviour; type=DirectoryController; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: Expansions, Expansions.Missions, Expansions.Missions.Runtime, KSP.Localization, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: GameObject=52, Transform=44, GetComponent=26, UnityEngine.UI=15, SetActive=5, Instantiate/Destroy=5; unity_score=78
- ksp refs: HighLogic=15, Game=7, Localizer=5, MissionSystem=2, EditorLogic=1; fan_in=4 fan_out=17
- methods(30): Awake, BuildDirectoryUI, BuildGameNameDirectories, BuildMissionDirectory, BuildMissionDirectoryUI, BuildSteamDirectoryUI, BuildStockDirectoryUI, BuildSubdirectories, CraftListUpdate, CreateDirectory, CreateEditorDirectory, DeselectCurrentDirectory, DirectoryCreatedCallback, DirectoryDeletedCallback, ExpansionDirectories, FindDirectory, InitForCraftSave, OnEnable, OnScroll, OverrideDisplay, PrioritizeCurrentGameDirectory, RestoreDisplay, SelectDirectory, ShowDirectoryTreeForEditor, Update, UpdateAllDirectoryDisplays, UpdateCurrentlySelectedByEditor, UpdateDirectoryControlsBySelected, UpdateDirectoryDisplay, UpdateDirectoryLayout
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Editor/InventoryPartListTooltip.cs
- 216 lines; kind=MonoBehaviour; type=InventoryPartListTooltip; bases=Tooltip,IPointerExitHandler,IEventSystemHandler,IPointerEnterHandler; ancestry=Tooltip/MonoBehaviour; ns=KSP.UI.Screens.Editor
- usings: TMPro, UnityEngine, UnityEngine.EventSystems
- unity: GameObject=10, SetActive=8, UnityEngine.UI=7, EventSystems=6, Transform=5, Coroutine=4; unity_score=83
- ksp refs: AvailablePart=5, GameSettings=2, ProtoPartSnapshot=1, PartModule=1, Part=1; fan_in=2 fan_out=14
- methods(8): CreateInfoWidgets, GetNewTooltipWidget, HidePreviousTooltipWidgets, OnDestroy, RefreshToolTipHeight, Setup, Update, WaitAndRefreshHeight
- auto suggestion: tier=C subsystem=Editor

## Assembly-CSharp/KSP/UI/Screens/Editor/InventoryPartListTooltipController.cs
- 164 lines; kind=MonoBehaviour; type=InventoryPartListTooltipController; bases=PinnableTooltipController; ancestry=PinnableTooltipController/TooltipController/MonoBehaviour; ns=KSP.UI.Screens.Editor
- usings: UnityEngine, UnityEngine.EventSystems
- unity: UnityEngine.UI=5, EventSystems=5, SetActive=3, Transform=2, GameObject=2, Time.=1; unity_score=70
- ksp refs: HighLogic=1, AvailablePart=1; fan_in=3 fan_out=8
- methods(13): CreateTooltip, DestroyTooltip, OnPointerClick, OnPointerEnter, OnPointerExit, OnTooltipAboutToDespawn, OnTooltipAboutToSpawn, OnTooltipDespawned, OnTooltipPinned, OnTooltipSpawned, OnTooltipUnpinned, PinToolTip, Update
- auto suggestion: tier=C subsystem=Editor

## Assembly-CSharp/KSP/UI/Screens/Editor/PartIcon.cs
- 43 lines; kind=class; type=PartIcon; bases=-; ancestry=-; ns=KSP.UI.Screens.Editor
- usings: UnityEngine
- unity: Renderer=3, GameObject=3, GetComponent=1; unity_score=61
- ksp refs: none; fan_in=4 fan_out=0
- methods(1): DestroyIcon
- auto suggestion: tier=B subsystem=Editor

## Assembly-CSharp/KSP/UI/Screens/Editor/PartListTooltip.cs **MUST-OPEN (>800 lines)**
- 812 lines; kind=MonoBehaviour; type=PartListTooltip; bases=Tooltip; ancestry=Tooltip/MonoBehaviour; ns=KSP.UI.Screens.Editor
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: SetActive=28, GameObject=24, Transform=21, TMPro=10, UnityEngine.UI=9, Instantiate/Destroy=6; unity_score=68
- ksp refs: Localizer=26, PartModule=19, AvailablePart=12, Part=5, ResearchAndDevelopment=4, HighLogic=2; fan_in=5 fan_out=26
- methods(22): CacheLocalStrings, CreateExtendedInfo, CreateExtendedUpgradeInfo, DestroyExtendedInfo, DisplayExtendedInfo, GetNewTooltipWidget, GetPrimaryInfo, GetUpgradedPrimaryInfo, HidePreviousTooltipWidgets, OnDestroy, PartStatsDelegate, Setup, SetupGrayout, SetupScaleGauge, SetupUpgradeInfo, Update, UpdateCargoPartModuleInfo, UpdateVariantText, _GenerateStatsText, _GetPartStats, onBtnPurchaseRed, onPurchaseButton
- auto suggestion: tier=C subsystem=Editor

## Assembly-CSharp/KSP/UI/Screens/Editor/PartListTooltipController.cs
- 409 lines; kind=MonoBehaviour; type=PartListTooltipController; bases=PinnableTooltipController; ancestry=PinnableTooltipController/TooltipController/MonoBehaviour; ns=KSP.UI.Screens.Editor
- usings: KSP.Localization, PreFlightTests, UnityEngine, UnityEngine.EventSystems, UnityEngine.Rendering
- unity: Transform=7, Material=6, UnityEngine.UI=5, Shader=4, SetActive=4, EventSystems=4; unity_score=67
- ksp refs: GameEvents=7, HighLogic=4, Localizer=3, InputLockManager=3, AvailablePart=3, ResearchAndDevelopment=1; fan_in=7 fan_out=28
- methods(21): CreateTooltip, DestroyTooltip, GetTooltipHintText, OnButtonVariantToggle, OnEditorDefaultVariantChanged, OnPointerClick, OnPointerEnter, OnTooltipAboutToDespawn, OnTooltipAboutToSpawn, OnTooltipDespawned, OnTooltipPinned, OnTooltipSpawned, OnTooltipUnpinned, SetScreenSpaceMaskMaterials, SetupScreenSpaceMask, onPurchase, onPurchaseDismiss, onPurchaseProceed, onPurchaseUpgrade, onPurchaseUpgradeProceed, onVariantToggle
- auto suggestion: tier=C subsystem=Editor

## Assembly-CSharp/KSP/UI/Screens/Editor/PartListTooltipMasterController.cs
- 117 lines; kind=MonoBehaviour; type=PartListTooltipMasterController; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.Editor
- usings: UnityEngine
- unity: GameObject=4, Camera=4, Texture=3, Lifecycle=2, LayerMask=2, Transform=1; unity_score=76
- ksp refs: InputLockManager=4; fan_in=3 fan_out=5
- methods(6): Awake, CreateThumbnailCamera, HideTooltip, OnDestroy, setHoverLock, setSafeArea
- auto suggestion: tier=C subsystem=Editor

## Assembly-CSharp/KSP/UI/Screens/Editor/PartListTooltipWidget.cs
- 21 lines; kind=MonoBehaviour; type=PartListTooltipWidget; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.Editor
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: TMPro=2, UnityEngine.UI=1, MonoBehaviour=1; unity_score=74
- ksp refs: none; fan_in=3 fan_out=1
- methods(1): Setup
- auto suggestion: tier=C subsystem=Editor

## Assembly-CSharp/KSP/UI/Screens/EditorActionController.cs
- 34 lines; kind=MonoBehaviour; type=EditorActionController; bases=EditorActionGroup_Base; ancestry=EditorActionGroup_Base/UISelectableGridLayoutGroupItem/MonoBehaviour; ns=KSP.UI.Screens
- usings: Expansions.Serenity, KSP.Localization, UnityEngine
- unity: none; unity_score=21
- ksp refs: Localizer=1; fan_in=1 fan_out=5
- methods(1): Setup
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/EditorActionControllerHeader.cs
- 62 lines; kind=MonoBehaviour; type=EditorActionControllerHeader; bases=UISelectableGridLayoutGroupItem; ancestry=UISelectableGridLayoutGroupItem/MonoBehaviour; ns=KSP.UI.Screens
- usings: Expansions.Serenity, TMPro, UnityEngine.UI
- unity: SetActive=6, GameObject=6, TMPro=2, UnityEngine.UI=1; unity_score=83
- ksp refs: HighLogic=1; fan_in=1 fan_out=6
- methods(5): Deselect, EditButtonClicked, InputFieldDone, Select, Setup
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/EditorActionControllerOpenButton.cs
- 26 lines; kind=MonoBehaviour; type=EditorActionControllerOpenButton; bases=UISelectableGridLayoutGroupItem; ancestry=UISelectableGridLayoutGroupItem/MonoBehaviour; ns=KSP.UI.Screens
- usings: Expansions.Serenity, UnityEngine.UI
- unity: UnityEngine.UI=1; unity_score=44
- ksp refs: none; fan_in=1 fan_out=4
- methods(2): OpenButtonClicked, Setup
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/EditorActionGroup.cs
- 37 lines; kind=MonoBehaviour; type=EditorActionGroup; bases=EditorActionGroup_Base; ancestry=EditorActionGroup_Base/UISelectableGridLayoutGroupItem/MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.Localization, UnityEngine
- unity: none; unity_score=20
- ksp refs: Localizer=1; fan_in=1 fan_out=5
- methods(1): Setup
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/EditorActionGroup_Base.cs
- 23 lines; kind=MonoBehaviour; type=EditorActionGroup_Base; bases=UISelectableGridLayoutGroupItem; ancestry=UISelectableGridLayoutGroupItem/MonoBehaviour; ns=KSP.UI.Screens
- usings: TMPro
- unity: TMPro=1; unity_score=44
- ksp refs: none; fan_in=3 fan_out=3
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/EditorActionGroups.cs **MUST-OPEN (>800 lines)**
- 1963 lines; kind=MonoBehaviour; type=EditorActionGroups; bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler,IPointerExitHandler; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: Expansions, Expansions.Serenity, KSP.Localization, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.Events, UnityEngine.UI
- unity: GameObject=43, GetComponent=41, UnityEngine.UI=29, Transform=23, EventSystems=12, KeyCode=6; unity_score=61
- ksp refs: EditorLogic=31, Part=29, HighLogic=21, FlightGlobals=14, GameEvents=10, Localizer=8; fan_in=13 fan_out=44
- methods(59): ActivateInFlightInterface, ActivateInterface, AddActionToGroup, AddToController, AddToGroup, AddToSelection, Awake, CacheCurrentSelectable, ClearSelection, CloseGroup, ConstructActionList, ConstructGroupActionList, ConstructGroupAxisList, ConstructGroupList, ConstructLists, ConstructPartActionList, ConstructPartAxisList, ConstructPartList, CreateGroups_Action, CreateGroups_Axis, CreateGroups_Controller, DeactivateInterface, DectivateInFlightInterface, GetCurrentSelectableColumn, GetCurrentSelectableIndex, GetSelectedParts, GetShipControllers, HasSelectedParts, InitializeMenuNavigation, NavigateColumn, OnControllerAxesOrActionsChanged, OnDestroy, OnEditorUndoRedo, OnPointerEnter, OnPointerExit, RebuildLists, RemoveActionFromGroup, RemoveFromController, RemoveFromGroup, ResetPart, SelectController, SelectGroup, SelectNextColumnItem, SelectionContains, SetColumnLastItemExplicitNavigation ...
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/EditorActionGroupType.cs
- 9 lines; kind=enum; type=EditorActionGroupType; bases=-; ancestry=-; ns=KSP.UI.Screens
- usings: -
- unity: none; unity_score=0
- ksp refs: none; fan_in=5 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/EditorActionOverrideGroup.cs
- 179 lines; kind=MonoBehaviour; type=EditorActionOverrideGroup; bases=UISelectableGridLayoutGroupItem,IPointerClickHandler,IEventSystemHandler,ISubmitHandler; ancestry=UISelectableGridLayoutGroupItem/MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.Localization, TMPro, UnityEngine.EventSystems, UnityEngine.UI
- unity: GameObject=13, SetActive=12, EventSystems=3, TMPro=2, UnityEngine.UI=1, Lifecycle=1; unity_score=76
- ksp refs: InputLockManager=3, Localizer=2; fan_in=2 fan_out=9
- methods(12): CloseGroup, Collapse, Deselect, EditButtonClicked, InputFieldDone, OnDestroy, OnPointerClick, OnSubmit, OpenGroup, Select, SetName, Setup
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/EditorActionOverrideToggle.cs
- 53 lines; kind=MonoBehaviour; type=EditorActionOverrideToggle; bases=UISelectableGridLayoutGroupItem; ancestry=UISelectableGridLayoutGroupItem/MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.Localization, KSP.UI.TooltipTypes, TMPro
- unity: TMPro=1, GetComponent=1; unity_score=42
- ksp refs: Localizer=3; fan_in=1 fan_out=5
- methods(4): OnToggle, OnValueChanged, SetOverrideStateDelegate, Setup
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/EditorActionPartItem.cs
- 379 lines; kind=MonoBehaviour; type=EditorActionPartItem; bases=UISelectableGridLayoutGroupItem,IPointerEnterHandler,IEventSystemHandler,IPointerExitHandler,IPointerClickHandler,ISubmitHandler; ancestry=UISelectableGridLayoutGroupItem/MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.Localization, KSP.UI.TooltipTypes, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: GameObject=15, SetActive=13, EventSystems=7, UnityEngine.UI=4, TMPro=2, GetComponent=2; unity_score=67
- ksp refs: Localizer=8, GameSettings=8, PartModule=3, Part=3; fan_in=1 fan_out=16
- methods(16): GetAxisIncremental, GetAxisInverted, ItemClicked, OnDestroy, OnPointerClick, OnPointerEnter, OnPointerExit, OnSpeedSliderValueChanged, OnSubmit, RefreshAxisField, SetAxisIncremental, SetAxisInverted, Setup, ToggleInvert, ToggleMode, UpdateSliderValue
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/EditorActionPartReset.cs
- 52 lines; kind=MonoBehaviour; type=EditorActionPartReset; bases=UISelectableGridLayoutGroupItem,IPointerEnterHandler,IEventSystemHandler,IPointerExitHandler,IPointerClickHandler,ISubmitHandler; ancestry=UISelectableGridLayoutGroupItem/MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.Localization, TMPro, UnityEngine.EventSystems
- unity: EventSystems=7, TMPro=1; unity_score=80
- ksp refs: Localizer=2; fan_in=1 fan_out=5
- methods(5): OnPointerClick, OnPointerEnter, OnPointerExit, OnSubmit, Setup
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/EditorActionPartSelector.cs
- 215 lines; kind=MonoBehaviour; type=EditorActionPartSelector; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: Highlighting, UnityEngine
- unity: GetComponent=5, GameObject=3, MonoBehaviour=1, Lifecycle=1, Instantiate/Destroy=1; unity_score=44
- ksp refs: Part=21, HighLogic=2, KerbalEVA=1, GameSettings=1; fan_in=4 fan_out=8
- methods(6): CreatePartActions, Deselect, DestroyPartActions, HighLight, LateUpdate, Select
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/EditorActionTitleItem.cs
- 6 lines; kind=MonoBehaviour; type=EditorActionTitleItem; bases=UISelectableGridLayoutGroupItem; ancestry=UISelectableGridLayoutGroupItem/MonoBehaviour; ns=KSP.UI.Screens
- usings: -
- unity: none; unity_score=15
- ksp refs: none; fan_in=0 fan_out=1
- methods(0): 
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/EditorInventoryOnlyIcon.cs
- 81 lines; kind=MonoBehaviour; type=EditorInventoryOnlyIcon; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=8, Transform=7, GameObject=6, SetActive=3, GetComponent=2, Texture=1; unity_score=93
- ksp refs: none; fan_in=3 fan_out=1
- methods(5): Awake, ClearStackImages, SetIconTexture, SetStackAmount, ToggleBackgroundVisibility
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/EditorLaunchPadItem.cs
- 68 lines; kind=MonoBehaviour; type=EditorLaunchPadItem; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=2, Lifecycle=2, TMPro=1, MonoBehaviour=1; unity_score=62
- ksp refs: HighLogic=3, EditorLogic=1; fan_in=1 fan_out=8
- methods(5): Awake, Create, MouseInput_Launch, MouseInput_SelectDefault, OnDestroy
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/EditorPanels.cs
- 140 lines; kind=MonoBehaviour; type=EditorPanels; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.UI.Screens.Editor, UnityEngine, UnityEngine.EventSystems
- unity: Lifecycle=2, MonoBehaviour=1, EventSystems=1; unity_score=35
- ksp refs: EditorLogic=2, GameEvents=1; fan_in=3 fan_out=5
- methods(8): Awake, IsMouseOver, OnDestroy, ShowActionGroups, ShowCargo, ShowCrewAssignment, ShowPartsList, updatePartsListMode
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/EditorPartIcon.cs **MUST-OPEN (>800 lines)**
- 978 lines; kind=MonoBehaviour; type=EditorPartIcon; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: Expansions.Missions.Runtime, KSP.UI.Screens.Editor, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: GameObject=52, SetActive=30, Transform=18, Material=16, UnityEngine.UI=15, GetComponent=10; unity_score=77
- ksp refs: AvailablePart=12, GameEvents=9, HighLogic=7, GameDatabase=5, EditorLogic=5, InputLockManager=3; fan_in=10 fan_out=36
- methods(37): Awake, CheckExperimental, CheckMissionRequired, CleanUpMaterials, Create, CreateMaterialArray, DisableAddButton, DisableDeleteButton, EnableAddButton, EnableDeleteButton, GetCurrentVariant, Highlight, InstantiatePartIcon, LoadPartThumbnail, MouseInput_Add, MouseInput_Delete, MouseInput_PlacePart, MouseInput_PointerEnter, MouseInput_PointerExit, MouseInput_SpawnPart, OnDestroy, OnEditorDefaultVariantChanged, OnEditorPartEvent, OnInventoryPartOnMouseChanged, SetEmptySlot, SetFlagBrowserTexture, SetGrey, SetMatchingIconPrefabMaterial, SetPartColor, Start, ToggleVariant, Unhighlight, UnsetGrey, Update, UpdateStackUI, VariantsAvailable, WaitAndTakePartSnapshot
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/EditorPartList.cs
- 498 lines; kind=MonoBehaviour; type=EditorPartList; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.UI.Screens.Editor, UnityEngine, UnityEngine.UI
- unity: GameObject=20, Transform=17, SetActive=17, Coroutine=12, UnityEngine.UI=7, Instantiate/Destroy=6; unity_score=74
- ksp refs: AvailablePart=36, PartLoader=5, EditorLogic=3, GameEvents=2, HighLogic=1; fan_in=16 fan_out=18
- methods(25): AutoScrollBarFix, AutoScrollBarFix_routine, Awake, ClearAllItems, OnDestroy, OnEditorRestart, OnPartListUpdate, Refresh, RefreshCustomPartList, RefreshPartList, RefreshSearchList, RefreshSubassemblies, RefreshSubassemblyList, RefreshVariants, RefreshVariantsList, RememberScrollbarPosition, RevealPart, ScrollBarFix, SortingCallback, Start, TapBackground, TapIcon, UpdatePartIcon, UpdatePartIcons, UpdateScrollbarPositionDictionary
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/EditorSaveFolderItem.cs
- 83 lines; kind=MonoBehaviour; type=EditorSaveFolderItem; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=2, Lifecycle=2, TMPro=1, MonoBehaviour=1; unity_score=57
- ksp refs: Localizer=1, EditorLogic=1; fan_in=1 fan_out=7
- methods(9): Awake, Create, IsPath, MouseInput_OnClick, MouseInput_SelectDefault, OnDestroy, SaveCraft, SetPath, SetPathDisplay
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/EditorSubassemblyItem.cs
- 93 lines; kind=MonoBehaviour; type=EditorSubassemblyItem; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: TMPro=3, Coroutine=3, UnityEngine.UI=2, MonoBehaviour=1, Lifecycle=1; unity_score=69
- ksp refs: Localizer=8, HighLogic=4, InputLockManager=3, EditorLogic=3, Part=1; fan_in=1 fan_out=14
- methods(8): Awake, Create, DeleteSubassembly, MouseInput_Delete, MouseInput_Spawn, OnDeleteConfirm, OnDismiss, OnDismissCoroutine
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/EditorVariantItem.cs
- 95 lines; kind=MonoBehaviour; type=EditorVariantItem; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens
- usings: TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=4, TMPro=3, Shader=2, Lifecycle=2, MonoBehaviour=1; unity_score=75
- ksp refs: EditorLogic=2, GameEvents=1, AvailablePart=1; fan_in=1 fan_out=7
- methods(6): Awake, Create, MouseInput_ApplyTheme, MouseInput_SetTheme, OnDestroy, SetColor
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/EngineersReport.cs
- 695 lines; kind=MonoBehaviour; type=EngineersReport; bases=UIApp; ancestry=UIApp/MonoBehaviour; ns=KSP.UI.Screens
- usings: Highlighting, KSP.Localization, PreFlightTests, RUI.Algorithms, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: Coroutine=12, Transform=10, GameObject=8, EventSystems=8, TMPro=6, GetComponent=6; unity_score=59
- ksp refs: GameEvents=16, Localizer=14, EditorLogic=7, Part=5, ShipConstruct=3, PartResourceDefinition=2; fan_in=3 fan_out=61
- methods(41): AddTest, CacheLocalStrings, CreateCraftStats, CreateCraftStatsbody, CreateSeveritySelector, CreateStockDesignConcern, Critical_OnFalse, Critical_OnTrue, DisplayApp, GetAppScenes, GetAppScreenPos, GetFormattedTestDescription, GetFormattedTestTitle, HideApp, MouseInput_PointerEnter, MouseInput_PointerExit, Notice_OnFalse, Notice_OnTrue, OnAppAboutToStart, OnAppDestroy, OnAppInitialized, OnCraftModified, OnCraftModified_delayed, OnCrewModified, OnStageAddedDeleted, RemoveTest, RunTest, RunTests, ShouldTest, ShowSeverity, Start, UpdateAndReposition, UpdateContainer, UpdateCraftStatsRoutine, UpdateCratStats, UpdateDesignConcern, UpdateDesignConcernBody, UpdateStateImage, Warning_OnFalse, Warning_OnTrue, onCrewDialogChange
- auto suggestion: tier=C subsystem=UI

## Assembly-CSharp/KSP/UI/Screens/Flight/ActionGroupToggleButton.cs
- 153 lines; kind=MonoBehaviour; type=ActionGroupToggleButton; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.Flight
- usings: UnityEngine, UnityEngine.UI
- unity: Light=2, Lifecycle=2, UnityEngine.UI=1, MonoBehaviour=1, GetComponent=1; unity_score=46
- ksp refs: FlightGlobals=12, InputLockManager=7; fan_in=0 fan_out=5
- methods(3): Awake, LateUpdate, SetToggle
- auto suggestion: tier=B subsystem=FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/AltitudeTumbler.cs
- 188 lines; kind=MonoBehaviour; type=AltitudeTumbler; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.Flight
- usings: UnityEngine, UnityEngine.UI
- unity: Lifecycle=4, Transform=3, Time.=3, Analytics=2, UnityEngine.UI=1, MonoBehaviour=1; unity_score=56
- ksp refs: GameEvents=10, FlightGlobals=7, HighLogic=2, Vessel=1, MapView=1, GameSettings=1; fan_in=1 fan_out=10
- methods(11): Awake, ChangeAltimeterMode, LateUpdate, OnDestroy, OnMapEntered, OnSceneSwitch, OnVesselChange, Reset, SetModeTumbler, SetTumblerPosition, Start
- auto suggestion: tier=B subsystem=FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/Dialogs/CrewHatchDialog.cs
- 181 lines; kind=MonoBehaviour; type=CrewHatchDialog; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.Flight.Dialogs
- usings: KSP.Localization, TMPro, UnityEngine
- unity: Lifecycle=4, Transform=3, TMPro=3, Instantiate/Destroy=3, GameObject=3, GetComponent=2; unity_score=69
- ksp refs: ProtoCrewMember=8, GameEvents=8, Part=4, Localizer=3, Vessel=2, HighLogic=1; fan_in=1 fan_out=15
- methods(15): Awake, ClearList, CreateList, CreatePanelContent, LateUpdate, OnBtnEVA, OnBtnTransfer, OnCameraChange, OnDestroy, OnFlightUIModeChanged, Spawn, Start, Terminate, onVesselChange, onVesselWasModified
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/Dialogs/CrewHatchDialogWidget.cs
- 58 lines; kind=MonoBehaviour; type=CrewHatchDialogWidget; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.Flight.Dialogs
- usings: TMPro, UnityEngine, UnityEngine.Events, UnityEngine.UI
- unity: UnityEngine.UI=3, GameObject=2, TMPro=1, SetActive=1, MonoBehaviour=1, Instantiate/Destroy=1; unity_score=78
- ksp refs: ProtoCrewMember=4; fan_in=1 fan_out=3
- methods(3): BtnInit, Init, Terminate
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/Dialogs/ExperimentsResultDialog.cs
- 430 lines; kind=MonoBehaviour; type=ExperimentsResultDialog; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.Flight.Dialogs
- usings: CommNet, KSP.Localization, KSP.UI.TooltipTypes, TMPro, UnityEngine, UnityEngine.UI
- unity: UnityEngine.UI=14, GameObject=11, TMPro=10, SetActive=10, Lifecycle=3, KeyCode=3; unity_score=77
- ksp refs: GameEvents=15, Localizer=10, Vessel=2, Part=1, HighLogic=1, FlightGlobals=1; fan_in=3 fan_out=20
- methods(25): Awake, Dismiss, DisplayResult, NextPage, OnDestroy, OnGameSceneLoadRequested, OnPagesModified, OnPause, OnUnpause, PrevPage, SetPage, Update, addPage, dismissCurrentPage, onBtnDiscard, onBtnKeep, onBtnLab, onBtnPageNext, onBtnPagePrev, onBtnReset, onBtnTransmit, onPartDie, onVesselFocusChange, onVesselModified, removePage
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/Dialogs/FlagPlaqueDialog.cs
- 56 lines; kind=MonoBehaviour; type=FlagPlaqueDialog; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.Flight.Dialogs
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.UI
- unity: TMPro=2, Instantiate/Destroy=2, UnityEngine.UI=1, Transform=1, MonoBehaviour=1, Lifecycle=1; unity_score=80
- ksp refs: Localizer=1; fan_in=1 fan_out=4
- methods(4): Spawn, Start, Terminate, onBtnClose
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/GeeGauge.cs
- 22 lines; kind=MonoBehaviour; type=GeeGauge; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.Flight
- usings: UnityEngine
- unity: MonoBehaviour=1, Lifecycle=1, GetComponent=1; unity_score=59
- ksp refs: FlightGlobals=2; fan_in=0 fan_out=2
- methods(2): LateUpdate, Reset
- auto suggestion: tier=B subsystem=FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/IVAEVACollapseGroups.cs
- 133 lines; kind=MonoBehaviour; type=IVAEVACollapseGroups; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.Flight
- usings: UnityEngine
- unity: Lifecycle=2, MonoBehaviour=1; unity_score=34
- ksp refs: GameEvents=7, Vessel=2, FlightGlobals=1; fan_in=0 fan_out=7
- methods(7): Awake, FinishedAllTransitionsCallback, OnCameraChange, OnDestroy, UpdatePanels, onVesselChange, onVesselWasModified
- auto suggestion: tier=B subsystem=FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/KerbalPortrait.cs
- 451 lines; kind=MonoBehaviour; type=KerbalPortrait; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.Flight
- usings: KSP.Localization, KSP.UI.TooltipTypes, KSP.UI.Util, TMPro, UnityEngine, UnityEngine.UI
- unity: SetActive=13, Coroutine=10, UnityEngine.UI=7, Material=6, GameObject=5, Lifecycle=3; unity_score=66
- ksp refs: HighLogic=4, Localizer=3, ProtoCrewMember=2, KerbalEVA=2, GameSettings=2, FlightGlobals=2; fan_in=4 fan_out=22
- methods(15): CacheLocalStrings, CanEVA, CanIVA, ClickEVA, ClickIVA, OnBecameInvisible, OnBecameVisible, OnCrewDie, OnDisable, OnEnable, OnOverlayUpdate, OverlayUpdate, RectContainment_OnContainmentChanged, Setup, Update
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/KerbalPortraitGallery.cs **MUST-OPEN (>800 lines)**
- 903 lines; kind=MonoBehaviour; type=KerbalPortraitGallery; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.Flight
- usings: KSP.Localization, TMPro, UnityEngine, UnityEngine.EventSystems, UnityEngine.UI
- unity: GameObject=22, SetActive=19, UnityEngine.UI=12, Transform=7, Coroutine=7, Lifecycle=6; unity_score=62
- ksp refs: GameEvents=13, KerbalEVA=12, FlightGlobals=8, Vessel=4, Localizer=4, ProtoCrewMember=3; fan_in=5 fan_out=16
- methods(35): Awake, CheckInvalidActiveCrew, ClearPortraits, DespawnInactivePortraits, DespawnPortrait, GetMaxGalleryCapacity, GetPortrait, HoverArea_onPointerEnter, HoverArea_onPointerExit, OnCameraChange, OnCommandSeatInteraction, OnDestroy, OnDisable, OnEnable, OnKerbalLevelUp, RegisterActiveCrew, SetActivePortraitsForVessel, SpawnPortrait, Start, StartRefresh, StartReset, ToggleIVAOverlay, UIControlsUpdate, UnregisterActiveCrew, Update, UpdatePortrait, UpdatePortraitScrolling, onBtnAddSlot, onBtnLeft, onBtnRemSlot, onBtnRight, onCrewTransferred, onIVAOverlayDismiss, onIVAOverlayPress, onVesselWasModified
- auto suggestion: tier=C subsystem=FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/LEDLin.cs
- 22 lines; kind=MonoBehaviour; type=LEDLin; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.Flight
- usings: UnityEngine
- unity: MonoBehaviour=1, Lifecycle=1, GetComponent=1; unity_score=59
- ksp refs: FlightGlobals=1; fan_in=0 fan_out=2
- methods(2): LateUpdate, Reset
- auto suggestion: tier=B subsystem=FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/LEDRot.cs
- 22 lines; kind=MonoBehaviour; type=LEDRot; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.Flight
- usings: UnityEngine
- unity: MonoBehaviour=1, Lifecycle=1, GetComponent=1; unity_score=59
- ksp refs: FlightGlobals=1; fan_in=0 fan_out=2
- methods(2): LateUpdate, Reset
- auto suggestion: tier=B subsystem=FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/LightDisplay.cs
- 24 lines; kind=MonoBehaviour; type=LightDisplay; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.Flight
- usings: UnityEngine
- unity: MonoBehaviour=1, Light=1, Lifecycle=1; unity_score=63
- ksp refs: FlightGlobals=4; fan_in=0 fan_out=3
- methods(1): LateUpdate
- auto suggestion: tier=B subsystem=FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/LinearAtmosphereGauge.cs
- 51 lines; kind=MonoBehaviour; type=LinearAtmosphereGauge; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.Flight
- usings: UnityEngine
- unity: MonoBehaviour=1, Lifecycle=1, GetComponent=1; unity_score=47
- ksp refs: FlightGlobals=4, CelestialBody=1; fan_in=0 fan_out=2
- methods(2): LateUpdate, Reset
- auto suggestion: tier=B subsystem=FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/LinearControlGauges.cs
- 101 lines; kind=MonoBehaviour; type=LinearControlGauges; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.Flight
- usings: UnityEngine, UnityEngine.UI
- unity: Lifecycle=3, Input.=2, UnityEngine.UI=1, MonoBehaviour=1; unity_score=55
- ksp refs: FlightGlobals=10, GameEvents=2; fan_in=0 fan_out=3
- methods(4): LateUpdate, OnDestroy, Start, onPrecisionModeToggle
- auto suggestion: tier=B subsystem=FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/MapViewPanelTransition.cs
- 42 lines; kind=MonoBehaviour; type=MapViewPanelTransition; bases=MonoBehaviour; ancestry=MonoBehaviour; ns=KSP.UI.Screens.Flight
- usings: UnityEngine
- unity: Lifecycle=2, GetComponent=2, MonoBehaviour=1; unity_score=64
- ksp refs: GameEvents=4; fan_in=0 fan_out=2
- methods(5): Awake, OnDestroy, Reset, SetModeMapOff, SetModeMapOn
- auto suggestion: tier=B subsystem=FlightUI
