# child_3: 681 files, 87632 lines

## Assembly-CSharp/KSP/FX/Fireworks/FireworkEffectType.cs
- L=8 kind=enum type=FireworkEffectType bases=- anc=- ns=KSP.FX.Fireworks
- unity(0): none | ksp: none | fan_in=4
- methods(0): 
- auto: C / Rendering

## Assembly-CSharp/KSP/FX/Fireworks/FireworkFX.cs
- L=182 kind=MonoBehaviour type=FireworkFX bases=MonoBehaviour anc=MonoBehaviour ns=KSP.FX.Fireworks
- unity(79): GetComponent=8, GameObject=7, Transform=5, Audio=4, Instantiate/Destroy=3, Rigidbody=2 | ksp: GameDatabase=1 | fan_in=2
- methods(5): FixedUpdate, GetRBVelocity, OnCollisionEnter, Setup, playAudioFX
- auto: C / Rendering

## Assembly-CSharp/KSP/FX/Fireworks/FireworkFXComponent.cs
- L=286 kind=MonoBehaviour type=FireworkFXComponent bases=MonoBehaviour anc=MonoBehaviour ns=KSP.FX.Fireworks
- unity(77): ParticleSystem=25, Transform=4, Coroutine=4, Lifecycle=3, GameObject=2, Audio=2 | ksp: FlightGlobals=11 | fan_in=2
- methods(11): ActivateBurstPS, Die, Initialize, LateUpdate, OnDestroy, PlaySFXOnParticleDeath, Update, configureParticleSystems, createColorGradient, particleSystemsPlaying, setParticlesCBGravity
- auto: C / Rendering

## Assembly-CSharp/KSP/FX/Fireworks/FireworkFXDefinition.cs
- L=83 kind=class type=FireworkFXDefinition bases=- anc=- ns=KSP.FX.Fireworks
- unity(30): none | ksp: ConfigNode=2 | fan_in=2
- methods(2): Load, Save
- auto: C / Rendering

## Assembly-CSharp/KSP/FX/Fireworks/FireworkFXList.cs
- L=58 kind=class type=FireworkFXList bases=IEnumerable anc=IEnumerable ns=KSP.FX.Fireworks
- unity(22): none | ksp: ConfigNode=1 | fan_in=1
- methods(4): Add, Contains, Count, GetEnumerator
- auto: C / Rendering

## Assembly-CSharp/KSP/IO/BinaryReader.cs
- L=292 kind=class type=BinaryReader bases=IDisposable anc=IDisposable ns=KSP.IO
- unity(0): none | ksp: Vessel=1 | fan_in=0
- methods(21): Close, CreateForType, Dispose, PeekChar, Read, ReadBoolean, ReadByte, ReadBytes, ReadChar, ReadChars, ReadDecimal, ReadDouble, ReadInt16, ReadInt32, ReadInt64, ReadSByte, ReadSingle, ReadString, ReadUInt16, ReadUInt32, ReadUInt64
- auto: A / Save

## Assembly-CSharp/KSP/IO/BinaryWriter.cs
- L=299 kind=class type=BinaryWriter bases=IDisposable anc=IDisposable ns=KSP.IO
- unity(0): none | ksp: Vessel=1 | fan_in=0
- methods(6): Close, CreateForType, Dispose, Flush, Seek, Write
- auto: A / Save

## Assembly-CSharp/KSP/IO/File.cs
- L=278 kind=class type=File bases=- anc=- ns=KSP.IO
- unity(0): none | ksp: Vessel=15 | fan_in=3
- methods(15): AppendAllText, AppendText, Create, CreateText, Delete, Exists, Open, OpenText, OpenWrite, ReadAllBytes, ReadAllLines, ReadAllText, WriteAllBytes, WriteAllLines, WriteAllText
- auto: A / Save

## Assembly-CSharp/KSP/IO/FileAccess.cs
- L=15 kind=enum type=FileAccess bases=- anc=- ns=KSP.IO
- unity(12): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / Save

## Assembly-CSharp/KSP/IO/FileInfo.cs
- L=281 kind=class type=FileInfo bases=- anc=- ns=KSP.IO
- unity(0): none | ksp: Vessel=3 | fan_in=0
- methods(15): AppendText, CopyTo, Create, CreateForType, CreateText, Decrypt, Delete, Encrypt, MoveTo, Open, OpenRead, OpenText, OpenWrite, Replace, ToString
- auto: A / Save

## Assembly-CSharp/KSP/IO/FileMode.cs
- L=15 kind=enum type=FileMode bases=- anc=- ns=KSP.IO
- unity(0): none | ksp: none | fan_in=3
- methods(0): 
- auto: A / Save

## Assembly-CSharp/KSP/IO/FileShare.cs
- L=18 kind=enum type=FileShare bases=- anc=- ns=KSP.IO
- unity(12): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / Save

## Assembly-CSharp/KSP/IO/FileStream.cs
- L=200 kind=class type=FileStream bases=IDisposable anc=IDisposable ns=KSP.IO
- unity(0): none | ksp: none | fan_in=2
- methods(14): BeginRead, BeginWrite, Dispose, EndRead, EndWrite, Flush, Lock, Read, ReadByte, Seek, SetLength, Unlock, Write, WriteByte
- auto: A / Save

## Assembly-CSharp/KSP/IO/IOException.cs
- L=26 kind=class type=IOException bases=Exception anc=Exception ns=KSP.IO
- unity(0): none | ksp: none | fan_in=5
- methods(0): 
- auto: A / Save

## Assembly-CSharp/KSP/IO/IOTools.cs
- L=24 kind=class type=IOTools bases=- anc=- ns=KSP.IO
- unity(0): none | ksp: none | fan_in=0
- methods(2): DeserializeFromBinary, SerializeToBinary
- auto: A / Save

## Assembly-CSharp/KSP/IO/IOUtils.cs
- L=165 kind=class type=IOUtils bases=- anc=- ns=KSP.IO
- unity(7): Application.=3 | ksp: Vessel=1, Part=1 | fan_in=11
- methods(12): Cleanup, ConvertAccess, ConvertMode, ConvertSeek, ConvertShare, DeserializeFromBinary, GetFilePathFor, GetFlightDirectories, GetPartPathFor, RemoveFlightData, SerializeToBinary, WrapException
- auto: A / Save

## Assembly-CSharp/KSP/IO/MemoryStream.cs
- L=165 kind=class type=MemoryStream bases=- anc=- ns=KSP.IO
- unity(0): none | ksp: none | fan_in=2
- methods(9): Flush, GetBuffer, Read, ReadByte, Seek, SetLength, ToArray, Write, WriteByte
- auto: A / Save

## Assembly-CSharp/KSP/IO/PluginConfigNode.cs
- L=359 kind=class type=PluginConfigNode bases=- anc=- ns=KSP.IO
- unity(15): KeyCode=3, MonoBehaviour=1 | ksp: Vector3d=4 | fan_in=1
- methods(7): GetParent, GetValue, SetValue, StartNamedElement, deserializeXML, handleElement, serializeXML
- auto: A / Save

## Assembly-CSharp/KSP/IO/PluginConfiguration.cs
- L=98 kind=class type=PluginConfiguration bases=- anc=- ns=KSP.IO
- unity(0): none | ksp: Vessel=1 | fan_in=0
- methods(5): CreateForType, GetValue, SetValue, load, save
- auto: A / Save

## Assembly-CSharp/KSP/IO/SeekOrigin.cs
- L=9 kind=enum type=SeekOrigin bases=- anc=- ns=KSP.IO
- unity(0): none | ksp: none | fan_in=4
- methods(0): 
- auto: A / Save

## Assembly-CSharp/KSP/IO/TextReader.cs
- L=129 kind=class type=TextReader bases=IDisposable anc=IDisposable ns=KSP.IO
- unity(0): none | ksp: Vessel=1 | fan_in=2
- methods(8): Close, CreateForType, DiscardBufferedData, Dispose, Peek, Read, ReadLine, ReadToEnd
- auto: A / Save

## Assembly-CSharp/KSP/IO/TextWriter.cs
- L=508 kind=class type=TextWriter bases=IDisposable anc=IDisposable ns=KSP.IO
- unity(0): none | ksp: Vessel=1 | fan_in=2
- methods(6): Close, CreateForType, Dispose, Flush, Write, WriteLine
- auto: A / Save

## Assembly-CSharp/KSP/Localization/FontLangSettings.cs
- L=20 kind=class type=FontLangSettings bases=- anc=- ns=KSP.Localization
- unity(48): TMPro=2 | ksp: none | fan_in=1
- methods(0): 
- auto: C / Localization

## Assembly-CSharp/KSP/Localization/FontSettings.cs
- L=82 kind=class type=FontSettings bases=- anc=- ns=KSP.Localization
- unity(25): TMPro=2 | ksp: none | fan_in=1
- methods(3): AddSubFont, ChangeLanguage, CleanSubFonts
- auto: C / Localization

## Assembly-CSharp/KSP/Localization/Localizer.cs
- L=494 kind=class type=Localizer bases=- anc=- ns=KSP.Localization
- unity(2): none | ksp: ConfigNode=10, Localizer=4, GameSettings=2, Vessel=1, TimeWarp=1 | fan_in=802
- methods(20): AddTagValuesForLanguage, AddTagValuesFromNode, AppendMissionTags, Format, FormatWithLingoona, GetLanguageIdFromFile, GetStringByTag, Init, RefreshRuleSet, RefreshTagValues, ReplaceSingleTagIfFound, ReplaceTags, SwitchToLanguage, TranslateBranch, TryGetStringByTag, _CacheLocalStrings, _Format, _SwitchToLanguage, unescapeFormattedString, unescapeStringToLingoona
- auto: C / Localization

## Assembly-CSharp/KSP/Testing/TestInfo.cs
- L=30 kind=attribute type=TestInfo bases=Attribute anc=Attribute ns=KSP.Testing
- unity(0): none | ksp: none | fan_in=2
- methods(1): ToString
- auto: C / Debug

## Assembly-CSharp/KSP/Testing/TestManager.cs
- L=71 kind=class type=TestManager bases=- anc=- ns=KSP.Testing
- unity(10): MonoBehaviour=1 | ksp: none | fan_in=3
- methods(3): AddTest, ClearUnitTests, RunTests
- auto: C / Debug

## Assembly-CSharp/KSP/Testing/TestResults.cs
- L=13 kind=class type=TestResults bases=- anc=- ns=KSP.Testing
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: C / Debug

## Assembly-CSharp/KSP/Testing/TestState.cs
- L=13 kind=class type=TestState bases=- anc=- ns=KSP.Testing
- unity(0): none | ksp: none | fan_in=4
- methods(0): 
- auto: C / Debug

## Assembly-CSharp/KSP/Testing/UnitTest.cs
- L=127 kind=class type=UnitTest bases=- anc=- ns=KSP.Testing
- unity(6): MonoBehaviour=1 | ksp: Vector3d=4 | fan_in=2
- methods(4): PerformTest, TestStartUp, TestTearDown, assertEquals
- auto: C / Debug

## Assembly-CSharp/KSP/UI/AlarmClockUIAddPane.cs
- L=417 kind=MonoBehaviour type=AlarmClockUIAddPane bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(70): SetActive=13, GameObject=13, UnityEngine.UI=10, TMPro=9, Lifecycle=4, MonoBehaviour=1 | ksp: Localizer=4, GameEvents=2, Vessel=1 | fan_in=1
- methods(17): AddAlarm, AddAlarmTypeChanged, AnyTextFieldHasFocus, Awake, BuildDropdownList, CancelAlarm, ClosePane, InputDataChanged, LateUpdate, OnAvailableVesselChanged, OnDestroy, RebuildUI, SetCurrentAlarmValuesFromUI, SetCurrentBaseUIValues, SetupAddAlarm, SetupEditAlarm, Start
- auto: C / UI

## Assembly-CSharp/KSP/UI/AlarmClockUIFrame.cs
- L=456 kind=MonoBehaviour type=AlarmClockUIFrame bases=GenericAppFrame anc=GenericAppFrame/MonoBehaviour ns=KSP.UI
- unity(60): UnityEngine.UI=12, Transform=6, Lifecycle=4, TMPro=3, Instantiate/Destroy=2, GetComponent=2 | ksp: GameEvents=7, Vessel=4, TimeWarp=3, InputLockManager=3, HighLogic=3 | fan_in=6
- methods(34): AddAlarmPaneClose, AddAlarmPaneCloseImmediate, AddAlarmPaneOpen, AddInputLock, AnyTextFieldHasFocus, Awake, CreateAlarm, DelayedOpenAddPane, DisableActionButtons, EditAlarm, EnableActionButtons, OnActiveAlarmsToggleChanged, OnActiveVesselToggleChanged, OnAlarmAdded, OnAlarmRemoved, OnAlarmRowSelected, OnAvailableVesselChanged, OnDestroy, OnEdit, OnPointerEnter, OnPointerExit, OnSwitchTo, OnWarpTo, PrintDate ...
- auto: C / UI

## Assembly-CSharp/KSP/UI/AlarmUIDisplayMode.cs
- L=8 kind=enum type=AlarmUIDisplayMode bases=- anc=- ns=KSP.UI
- unity(0): none | ksp: none | fan_in=9
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/AnchoredDialog.cs
- L=334 kind=MonoBehaviour type=AnchoredDialog bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(74): GameObject=13, UnityEngine.UI=7, Transform=6, GetComponent=6, Camera=5, Screen/Cursor=4 | ksp: GameEvents=2 | fan_in=4
- methods(21): AttachEventHooks, Awake, CreatePanel, CreateWindowContent, Disable, GetHover, GetWindowTitle, LateUpdate, OnClickOut, OnDestroy, OnDestroyThis, OnGameSettingsApplied, OnLateUpdate, OnPanelSetupComplete, ReEnable, SetBgPanel, Start, StartThis, Terminate, getUpAxis, setOpacity
- auto: C / UI

## Assembly-CSharp/KSP/UI/AnchoredDialogHost.cs
- L=23 kind=MonoBehaviour type=AnchoredDialogHost bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(67): MonoBehaviour=1, Lifecycle=1, Instantiate/Destroy=1, GameObject=1 | ksp: none | fan_in=1
- methods(1): LateUpdate
- auto: C / UI

## Assembly-CSharp/KSP/UI/AppUI_Control.cs
- L=42 kind=attribute type=AppUI_Control bases=Attribute anc=Attribute ns=KSP.UI
- unity(0): none | ksp: none | fan_in=18
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/AppUI_Data.cs
- L=112 kind=class type=AppUI_Data bases=IConfigNode anc=IConfigNode ns=KSP.UI
- unity(10): none | ksp: ConfigNode=6 | fan_in=7
- methods(10): CallOnDataChanged, CreateAppUIDataList, CreateInstanceOfAppUIData, Load, OnLoad, OnSave, Save, SetupDataChangeCallback, UIInputPanelDataChanged, UIInputPanelUpdate
- auto: C / UI

## Assembly-CSharp/KSP/UI/AppUI_Dropdown.cs
- L=7 kind=attribute type=AppUI_Dropdown bases=AppUI_Control anc=AppUI_Control/Attribute ns=KSP.UI
- unity(0): none | ksp: none | fan_in=4
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/AppUI_DropdownMultiLine.cs
- L=6 kind=attribute type=AppUI_DropdownMultiLine bases=AppUI_Dropdown anc=AppUI_Dropdown/AppUI_Control/Attribute ns=KSP.UI
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/AppUI_Heading.cs
- L=9 kind=attribute type=AppUI_Heading bases=AppUI_Control anc=AppUI_Control/Attribute ns=KSP.UI
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/AppUI_Input.cs
- L=13 kind=attribute type=AppUI_Input bases=AppUI_Control anc=AppUI_Control/Attribute ns=KSP.UI
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/AppUI_InputDateTime.cs
- L=11 kind=attribute type=AppUI_InputDateTime bases=AppUI_Control anc=AppUI_Control/Attribute ns=KSP.UI
- unity(0): none | ksp: none | fan_in=4
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/AppUI_InputFloat.cs
- L=7 kind=attribute type=AppUI_InputFloat bases=AppUI_Input anc=AppUI_Input/AppUI_Control/Attribute ns=KSP.UI
- unity(0): none | ksp: none | fan_in=1
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/AppUI_Label.cs
- L=7 kind=attribute type=AppUI_Label bases=AppUI_Control anc=AppUI_Control/Attribute ns=KSP.UI
- unity(0): none | ksp: none | fan_in=5
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/AppUI_LabelList.cs
- L=11 kind=attribute type=AppUI_LabelList bases=AppUI_Label anc=AppUI_Label/AppUI_Control/Attribute ns=KSP.UI
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/AppUI_RadioBool.cs
- L=7 kind=attribute type=AppUI_RadioBool bases=AppUI_Control anc=AppUI_Control/Attribute ns=KSP.UI
- unity(0): none | ksp: none | fan_in=3
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/AppUI_SliderFloat.cs
- L=11 kind=attribute type=AppUI_SliderFloat bases=AppUI_Control anc=AppUI_Control/Attribute ns=KSP.UI
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/AppUI_SliderFloatMultiLine.cs
- L=6 kind=attribute type=AppUI_SliderFloatMultiLine bases=AppUI_SliderFloat anc=AppUI_SliderFloat/AppUI_Control/Attribute ns=KSP.UI
- unity(0): none | ksp: none | fan_in=1
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/AppUI_ToggleBool.cs
- L=9 kind=attribute type=AppUI_ToggleBool bases=AppUI_Control anc=AppUI_Control/Attribute ns=KSP.UI
- unity(0): none | ksp: none | fan_in=1
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/BaseCrewAssignmentDialog.cs
- L=461 kind=MonoBehaviour type=BaseCrewAssignmentDialog bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(56): GetComponent=22, Instantiate/Destroy=4, Lifecycle=2, EventSystems=2, UnityEngine.UI=1, Texture=1 | ksp: ProtoCrewMember=10, KerbalRoster=3, GameDatabase=2 | fan_in=6
- methods(24): AddAvailItem, AddCrewItem, AddCrewItemEmpty, AddCrewListBorder, AddItem, Awake, ButtonClear, ButtonFill, ButtonReset, ClearLists, CreateAvailList, CreateCrewList, DropOnAvailList, DropOnCrewList, GetCurrentCrewRoster, GetManifest, ListItemButtonClick, MoveCrewToAvail, MoveCrewToEmptySeat, OnDestroy, Refresh, RefreshCrewLists, SetCurrentCrewRoster, SetDefaultManifest
- auto: C / UI

## Assembly-CSharp/KSP/UI/ButtonColorState.cs
- L=17 kind=class type=ButtonColorState bases=- anc=- ns=KSP.UI
- unity(29): none | ksp: none | fan_in=1
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/ButtonSpritesMgr.cs
- L=52 kind=MonoBehaviour type=ButtonSpritesMgr bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(85): Texture=6, UnityEngine.UI=2, MonoBehaviour=1 | ksp: none | fan_in=2
- methods(1): SetSpriteSet
- auto: C / UI

## Assembly-CSharp/KSP/UI/ButtonState.cs
- L=41 kind=class type=ButtonState bases=- anc=- ns=KSP.UI
- unity(69): Texture=4, UnityEngine.UI=2, TMPro=1 | ksp: none | fan_in=7
- methods(1): Setup
- auto: C / UI

## Assembly-CSharp/KSP/UI/ButtonStateTooltip.cs
- L=12 kind=class type=ButtonStateTooltip bases=- anc=- ns=KSP.UI
- unity(12): none | ksp: none | fan_in=1
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/CanvasGroupEventLocker.cs
- L=42 kind=MonoBehaviour type=CanvasGroupEventLocker bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(78): UnityEngine.UI=3, Lifecycle=2, MonoBehaviour=1, GetComponent=1 | ksp: GameEvents=4 | fan_in=0
- methods(4): Awake, Lock, OnDestroy, Unlock
- auto: C / UI

## Assembly-CSharp/KSP/UI/CanvasGroupInputLock.cs
- L=137 kind=MonoBehaviour type=CanvasGroupInputLock bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(58): UnityEngine.UI=3, Lifecycle=3, GameObject=2, MonoBehaviour=1, GetComponent=1 | ksp: GameEvents=3, InputLockManager=1 | fan_in=1
- methods(9): Awake, InitializeDefaultMask, OnDestroy, OnInputLocksModified, SetInputMask, SetInputMaskToDefault, Start, ToBits, UpdateLockMask
- auto: C / UI

## Assembly-CSharp/KSP/UI/ContentAspectController.cs
- L=61 kind=MonoBehaviour type=ContentAspectController bases=MonoBehaviour,ILayoutController,ILayoutSelfController anc=MonoBehaviour ns=KSP.UI
- unity(70): UnityEngine.UI=4, Transform=2, MonoBehaviour=1 | ksp: none | fan_in=0
- methods(4): OnRectTransformDimensionsChange, Reset, SetLayoutHorizontal, SetLayoutVertical
- auto: C / UI

## Assembly-CSharp/KSP/UI/CrewAssignmentDialog.cs
- L=111 kind=MonoBehaviour type=CrewAssignmentDialog bases=BaseCrewAssignmentDialog anc=BaseCrewAssignmentDialog/MonoBehaviour ns=KSP.UI
- unity(47): Coroutine=3, Lifecycle=2, GameObject=1 | ksp: HighLogic=6, GameEvents=3, KerbalRoster=2, InputLockManager=2, EditorLogic=2 | fan_in=6
- methods(14): Awake, ButtonAstronautComplex, DropOnAvailList, DropOnCrewList, EditorLogicUpdateCrew, GetCurrentCrewRoster, MoveCrewToAvail, MoveCrewToEmptySeat, OnDestroy, RefreshCrewOnDrop, SetCurrentCrewRoster, onOpenACDismiss, onOpenACProceed, onOpenAstronautComplex
- auto: C / UI

## Assembly-CSharp/KSP/UI/CrewListItem.cs
- L=351 kind=MonoBehaviour type=CrewListItem bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(60): UnityEngine.UI=9, TMPro=3, SetActive=3, GameObject=3, GetComponent=2, Texture=1 | ksp: ProtoCrewMember=14, GameDatabase=2, KerbalRoster=1 | fan_in=6
- methods(18): AddButtonInputDelegate, Awake, GetCourage, GetCrewRef, GetName, GetStupidity, OpenHelmetSuitPickerWindow, SetButton, SetButtonEnabled, SetCrewRef, SetKerbal, SetKerbalAsApplicableType, SetLabel, SetName, SetStats, SetTooltip, SetXP, SetupSuitButtons
- auto: C / UI

## Assembly-CSharp/KSP/UI/DialogMouseEnterControlLock.cs
- L=43 kind=MonoBehaviour type=DialogMouseEnterControlLock bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler anc=MonoBehaviour ns=KSP.UI
- unity(75): EventSystems=4, MonoBehaviour=1, Lifecycle=1 | ksp: InputLockManager=3 | fan_in=1
- methods(6): OnDestroy, OnPointerEnter, OnPointerExit, SetLockName, SetLockType, Setup
- auto: C / UI

## Assembly-CSharp/KSP/UI/Dialogs/FlightResultsDialog.cs
- L=317 kind=MonoBehaviour type=FlightResultsDialog bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Dialogs
- unity(82): GameObject=15, SetActive=14, TMPro=12, UnityEngine.UI=8, GetComponent=8, Lifecycle=3 | ksp: Localizer=14, HighLogic=8, Planetarium=3, InputLockManager=2, Vessel=1 | fan_in=1
- methods(10): Awake, Close, Display, OnDestroy, SetMissionOutcome, SetupGUI, Update, onLeavingFlight, onLeavingFlightDismiss, onLeavingFlightProceed
- auto: C / UI

## Assembly-CSharp/KSP/UI/Dialogs/IAccessKerbNet.cs
- L=21 kind=interface type=IAccessKerbNet bases=- anc=- ns=KSP.UI.Dialogs
- unity(0): none | ksp: Vessel=1, Part=1 | fan_in=2
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/Dialogs/WaypointTarget.cs
- L=143 kind=MonoBehaviour type=WaypointTarget bases=MonoBehaviour,IBeginDragHandler,IEventSystemHandler anc=MonoBehaviour ns=KSP.UI.Dialogs
- unity(76): EventSystems=6, UnityEngine.UI=5, Time.=3, Lifecycle=3, Input.=2, MonoBehaviour=1 | ksp: none | fan_in=1
- methods(13): Awake, CheckTraverseClick, ChildClampedLocalPosition, ChildClampedWorldPosition, OnBeginDrag, OnDisable, OnDrag, OnEndDrag, Reset, Target, Update, UpdateScale, UpdateTraversal
- auto: C / UI

## Assembly-CSharp/KSP/UI/DialogUISelectFile.cs
- L=109 kind=MonoBehaviour type=DialogUISelectFile bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(77): Instantiate/Destroy=6, UnityEngine.UI=3, Transform=3, TMPro=2, Lifecycle=2, GameObject=2 | ksp: none | fan_in=1
- methods(9): Awake, Destroy, OnAccept, OnCancel, OnFileSelected, SelectItem, Setup, Start, UpdateFiles
- auto: C / UI

## Assembly-CSharp/KSP/UI/DialogUISelectFileItem.cs
- L=12 kind=MonoBehaviour type=DialogUISelectFileItem bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(55): TMPro=1, MonoBehaviour=1 | ksp: none | fan_in=1
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/DragHandler.cs
- L=77 kind=MonoBehaviour type=DragHandler bases=MonoBehaviour,IInitializePotentialDragHandler,IEventSystemHandler anc=MonoBehaviour ns=KSP.UI
- unity(98): EventSystems=31, MonoBehaviour=1 | ksp: none | fan_in=3
- methods(7): AddEvents, ClearEvents, OnBeginDrag, OnDrag, OnEndDrag, OnInitializePotentialDrag, RemoveEvents
- auto: C / UI

## Assembly-CSharp/KSP/UI/DropHandler.cs
- L=22 kind=MonoBehaviour type=DropHandler bases=MonoBehaviour,IDropHandler,IEventSystemHandler anc=MonoBehaviour ns=KSP.UI
- unity(89): EventSystems=6, MonoBehaviour=1 | ksp: none | fan_in=1
- methods(1): OnDrop
- auto: C / UI

## Assembly-CSharp/KSP/UI/EventTriggerForwarder.cs
- L=70 kind=MonoBehaviour type=EventTriggerForwarder bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(97): EventSystems=19, Transform=4, GameObject=4, MonoBehaviour=1, Lifecycle=1, GetComponent=1 | ksp: none | fan_in=0
- methods(5): Awake, Drop, PointerEnter, PointerExit, Scroll
- auto: C / UI

## Assembly-CSharp/KSP/UI/ExperimentalVersionReadout.cs
- L=35 kind=MonoBehaviour type=ExperimentalVersionReadout bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(86): SetActive=3, GameObject=3, Lifecycle=2, TMPro=1, MonoBehaviour=1 | ksp: GameEvents=2, Localizer=1, GameSettings=1 | fan_in=0
- methods(3): OnDestroy, OnLevelLoaded, Start
- auto: C / UI

## Assembly-CSharp/KSP/UI/GenericAppFrame.cs
- L=203 kind=MonoBehaviour type=GenericAppFrame bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(76): UnityEngine.UI=8, EventSystems=8, Transform=5, Time.=2, Lifecycle=2, GameObject=2 | ksp: GameSettings=10 | fan_in=8
- methods(14): AddGlobalInputDelegate, AutoScale, Awake, OnBeginDragFooter, OnBeginDragHeader, OnDestroy, OnDragFooter, OnDragHeader, OnEndDragFooter, OnEndDragHeader, Reposition, Setup, UpdateDraggingBounds, UpdateDraggingBoundsAtBottom
- auto: C / UI

## Assembly-CSharp/KSP/UI/GenericCascadingList.cs
- L=167 kind=MonoBehaviour type=GenericCascadingList bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(84): TMPro=13, GetComponent=11, Instantiate/Destroy=8, UnityEngine.UI=2, EventSystems=2, SetActive=1 | ksp: none | fan_in=7
- methods(11): CreateBody, CreateBodyCollapsable, CreateBodyCollapsable_spacer, CreateBodyKeyValueAutofit, CreateBody_spacer, CreateFooter, CreateHeader, MouseInputCollapsableBody, MouseInputCollapsableBody_spacer, Setup, UpdateBodyCollapsable
- auto: C / UI

## Assembly-CSharp/KSP/UI/InteractableCtrlUIStates.cs
- L=91 kind=MonoBehaviour type=InteractableCtrlUIStates bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(73): SetActive=3, UnityEngine.UI=2, Lifecycle=2, TMPro=1, MonoBehaviour=1 | ksp: none | fan_in=0
- methods(3): Awake, LateUpdate, SetActive
- auto: C / UI

## Assembly-CSharp/KSP/UI/IPinnableTooltipController.cs
- L=13 kind=interface type=IPinnableTooltipController bases=ITooltipController anc=- ns=KSP.UI
- unity(0): none | ksp: none | fan_in=4
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/IRadioButton.cs
- L=9 kind=interface type=IRadioButton bases=- anc=- ns=KSP.UI
- unity(0): none | ksp: none | fan_in=3
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/ITooltipController.cs
- L=25 kind=interface type=ITooltipController bases=- anc=- ns=KSP.UI
- unity(29): UnityEngine.UI=1 | ksp: none | fan_in=6
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/KSPGraphicRaycaster.cs
- L=73 kind=class type=KSPGraphicRaycaster bases=GraphicRaycaster anc=GraphicRaycaster ns=KSP.UI
- unity(51): UnityEngine.UI=2, Raycast/Query=2, GameObject=1, EventSystems=1 | ksp: InputLockManager=1 | fan_in=0
- methods(2): Raycast, UpdateLockMask
- auto: C / UI

## Assembly-CSharp/KSP/UI/Language/DictionarySerializerHelper.cs
- L=22 kind=class type=DictionarySerializerHelper bases=- anc=- ns=KSP.UI.Language
- unity(12): none | ksp: none | fan_in=1
- methods(0): 
- auto: C / Localization

## Assembly-CSharp/KSP/UI/Language/ILanguageKey.cs
- L=9 kind=interface type=ILanguageKey bases=- anc=- ns=KSP.UI.Language
- unity(0): none | ksp: none | fan_in=1
- methods(0): 
- auto: C / Localization

## Assembly-CSharp/KSP/UI/Language/Language.cs
- L=470 kind=MonoBehaviour type=Language bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Language
- unity(56): Instantiate/Destroy=11, AssetBundle=11, Coroutine=5, Lifecycle=2, WWW/WebRequest=1, MonoBehaviour=1 | ksp: none | fan_in=5
- methods(22): ActivateAllFonts, Awake, DeactivateAllFonts, GetLanguage, LanguageName, LoadBundle, LoadLanguage, LoadLanguages, LoadedFonts, LoadedLanguages, OnDestroy, Save, SaveLanguages, SaveUntranslated, SetLanguage, Translate, TranslateWithFormat, UpdateFont, UpdateText, onActivateAllFonts, onLoadLanguages, onSetLanguage
- auto: C / Localization

## Assembly-CSharp/KSP/UI/Language/LanguageDefinition.cs
- L=211 kind=class type=LanguageDefinition bases=- anc=- ns=KSP.UI.Language
- unity(2): none | ksp: none | fan_in=1
- methods(9): AddTranslation, Deserialize, GetFullPath, MergeInto, RemoveTranslation, Serialize, SerializeUntranslated, TranslateWithFormat, Translation
- auto: C / Localization

## Assembly-CSharp/KSP/UI/Language/LanguageText.cs
- L=51 kind=MonoBehaviour type=LanguageText bases=MonoBehaviour,ILanguageKey anc=MonoBehaviour ns=KSP.UI.Language
- unity(47): MonoBehaviour=1, Lifecycle=1, GetComponent=1 | ksp: none | fan_in=0
- methods(2): Reset, Start
- auto: C / Localization

## Assembly-CSharp/KSP/UI/Language/StringExtensions.cs
- L=14 kind=static class type=StringExtensions bases=- anc=- ns=KSP.UI.Language
- unity(0): none | ksp: none | fan_in=0
- methods(1): Translate
- auto: C / Localization

## Assembly-CSharp/KSP/UI/ManeuverToolUIFrame.cs
- L=662 kind=MonoBehaviour type=ManeuverToolUIFrame bases=GenericAppFrame anc=GenericAppFrame/MonoBehaviour ns=KSP.UI
- unity(68): GameObject=20, TMPro=11, UnityEngine.UI=10, Instantiate/Destroy=10, Transform=7, Lifecycle=3 | ksp: Localizer=2 | fan_in=3
- methods(33): ActivatePlanet, AddObjectToVisualizerWindow, AddOnErrorStateChanged, AddOnSidePanelClicked, AddPlanetRecursively, CreatePlanets, DeactivatePlanet, FindPlanet, OnActionDelegate, OnBtnSidePanelClick, OnBtnTopPanelClick, OnCreateManeuverBtn, OnDestroy, OnErrorStateDelegate, OnTimeDisplaySecToggle, OnTimeDisplayUTToggle, RemoveObjectFromVisualizerWindow, RemoveOnErrorStateChanged, RemoveOnSidePanelClicked, SetCalculationState, SetCreateManeuverButton, SetErrorState, SetHeaderText, SetHelpString ...
- auto: C / UI

## Assembly-CSharp/KSP/UI/MessageSystemAppFrame.cs
- L=201 kind=MonoBehaviour type=MessageSystemAppFrame bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(72): EventSystems=8, UnityEngine.UI=4, Transform=4, Time.=2, TMPro=2, Lifecycle=2 | ksp: GameSettings=10, HighLogic=6, Localizer=1 | fan_in=1
- methods(13): AddGlobalInputDelegate, Awake, OnBeginDragFooter, OnBeginDragHeader, OnDestroy, OnDragFooter, OnDragHeader, OnEndDragFooter, OnEndDragHeader, Reposition, Setup, UpdateDraggingBounds, UpdateDraggingBoundsAtBottom
- auto: C / UI

## Assembly-CSharp/KSP/UI/PinnableTooltipController.cs
- L=61 kind=MonoBehaviour type=PinnableTooltipController bases=TooltipController,IPinnableTooltipController,ITooltipController anc=TooltipController/MonoBehaviour ns=KSP.UI
- unity(65): EventSystems=5 | ksp: none | fan_in=3
- methods(7): IsPinned, OnPointerClick, OnPointerEnter, OnPointerExit, OnTooltipPinned, OnTooltipUnpinned, Unpin
- auto: C / UI

## Assembly-CSharp/KSP/UI/PointerClickAndHoldHandler.cs
- L=45 kind=MonoBehaviour type=PointerClickAndHoldHandler bases=PointerClickHandler,IPointerExitHandler,IEventSystemHandler anc=PointerClickHandler/MonoBehaviour ns=KSP.UI
- unity(88): EventSystems=7, Coroutine=5 | ksp: none | fan_in=6
- methods(4): OnPointerDown, OnPointerDownHold, OnPointerExit, OnPointerUp
- auto: C / UI

## Assembly-CSharp/KSP/UI/PointerClickHandler.cs
- L=47 kind=MonoBehaviour type=PointerClickHandler bases=MonoBehaviour,IPointerClickHandler,IEventSystemHandler anc=MonoBehaviour ns=KSP.UI
- unity(94): EventSystems=14, MonoBehaviour=1 | ksp: none | fan_in=22
- methods(3): OnPointerClick, OnPointerDown, OnPointerUp
- auto: C / UI

## Assembly-CSharp/KSP/UI/PointerEnterExitHandler.cs
- L=44 kind=MonoBehaviour type=PointerEnterExitHandler bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler anc=MonoBehaviour ns=KSP.UI
- unity(98): EventSystems=16, MonoBehaviour=1 | ksp: none | fan_in=20
- methods(2): OnPointerEnter, OnPointerExit
- auto: C / UI

## Assembly-CSharp/KSP/UI/PrefabSpawner.cs
- L=60 kind=MonoBehaviour type=PrefabSpawner bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(82): GameObject=6, Transform=4, Lifecycle=2, MonoBehaviour=1, Instantiate/Destroy=1 | ksp: none | fan_in=0
- methods(3): Awake, Spawn, Start
- auto: C / UI

## Assembly-CSharp/KSP/UI/ProgressBar.cs
- L=35 kind=MonoBehaviour type=ProgressBar bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(77): UnityEngine.UI=2, MonoBehaviour=1, Lifecycle=1, GetComponent=1, GameObject=1 | ksp: none | fan_in=1
- methods(1): Start
- auto: C / UI

## Assembly-CSharp/KSP/UI/RadioButtonGroup.cs
- L=77 kind=class type=RadioButtonGroup bases=- anc=- ns=KSP.UI
- unity(17): Transform=1, GameObject=1 | ksp: none | fan_in=2
- methods(2): GetGroup, GetSelected
- auto: C / UI

## Assembly-CSharp/KSP/UI/RectTransformQuadFitter.cs
- L=39 kind=MonoBehaviour type=RectTransformQuadFitter bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(79): Coroutine=4, UnityEngine.UI=1, Transform=1, MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=0
- methods(2): Awake, UpdateDaemon
- auto: C / UI

## Assembly-CSharp/KSP/UI/RendererSortOrder.cs
- L=27 kind=MonoBehaviour type=RendererSortOrder bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(84): Renderer=3, MonoBehaviour=1, Lifecycle=1, GetComponent=1 | ksp: none | fan_in=0
- methods(2): Set, Start
- auto: C / UI

## Assembly-CSharp/KSP/UI/Rendering/CameraShaderReplacement.cs
- L=34 kind=MonoBehaviour type=CameraShaderReplacement bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Rendering
- unity(93): Camera=4, GetComponent=2, GameObject=2, Shader=1, MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=0
- methods(3): Reset, SetShader, Start
- auto: C / UI

## Assembly-CSharp/KSP/UI/Rendering/InvertSortOrder.cs
- L=32 kind=MonoBehaviour type=InvertSortOrder bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Rendering
- unity(96): Transform=7, UnityEngine.UI=5, MonoBehaviour=1, GetComponent=1, GameObject=1 | ksp: none | fan_in=0
- methods(2): InvertSortingOrder, RecurseLevel
- auto: C / UI

## Assembly-CSharp/KSP/UI/Rendering/RTCanvas.cs
- L=133 kind=MonoBehaviour type=RTCanvas bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Rendering
- unity(96): UnityEngine.UI=14, GameObject=14, Transform=8, GetComponent=7, Camera=4, Texture=3 | ksp: GameEvents=2, GameSettings=1 | fan_in=0
- methods(7): Awake, FindCanvasesUsingCamera, OnDestroy, SetUIOpacity, Setup, SetupRT, SetupRTCanvas
- auto: C / UI

## Assembly-CSharp/KSP/UI/ResizingLayoutElement.cs
- L=125 kind=MonoBehaviour type=ResizingLayoutElement bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(70): UnityEngine.UI=5, Coroutine=4, Time.=2, GetComponent=2, MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=3
- methods(4): Awake, Resize, ReverseResizing, StartResizing
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/ACSceneDespawner.cs
- L=20 kind=MonoBehaviour type=ACSceneDespawner bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(63): UnityEngine.UI=1, MonoBehaviour=1, Lifecycle=1 | ksp: GameEvents=1 | fan_in=0
- methods(2): BtnExit, Start
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/ACSceneSpawner.cs
- L=34 kind=MonoBehaviour type=ACSceneSpawner bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(56): Lifecycle=2, MonoBehaviour=1 | ksp: GameEvents=4, HighLogic=1 | fan_in=0
- methods(4): Awake, OnDestroy, onACDespawn, onACSpawn
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/ActionGroupsApp.cs
- L=230 kind=MonoBehaviour type=ActionGroupsApp bases=UIApp anc=UIApp/MonoBehaviour ns=KSP.UI.Screens
- unity(62): Transform=6, GameObject=5, Instantiate/Destroy=3, UnityEngine.UI=2, TMPro=2, SetActive=2 | ksp: Vessel=10, GameEvents=10, FlightGlobals=6, HighLogic=2 | fan_in=3
- methods(19): DisplayApp, GetAppScenes, GetAppScreenPos, HideApp, HideAppFromInput, HideUnpinApp, OnAppAboutToStart, OnAppDestroy, OnAppInitialized, OnVesselChanged, OnVesselDestroy, OnVesselGoOffRails, OnVesselOverrideGroupChanged, OnVesselWasModified, ReadjustPosition, Reposition, SelectNext, SelectPrev, UpdateOverrideGroupText
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/ActionGroupsPanel.cs
- L=207 kind=MonoBehaviour type=ActionGroupsPanel bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(86): Transform=12, UnityEngine.UI=10, GameObject=10, SetActive=7, Lifecycle=6, Instantiate/Destroy=6 | ksp: GameEvents=6, Vessel=4, FlightGlobals=4, Localizer=3, MapView=2 | fan_in=3
- methods(10): Awake, OnDestroy, OnDisable, OnEnable, SelectOverride, SetGroupName, Start, Update, UpdateActionGroups, UpdateButtons
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/ActiveStrategyListItem.cs
- L=28 kind=MonoBehaviour type=ActiveStrategyListItem bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(80): TMPro=2, UnityEngine.UI=1, Texture=1, MonoBehaviour=1 | ksp: none | fan_in=1
- methods(1): Setup
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Administration.cs
- L=448 kind=MonoBehaviour type=Administration bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(66): GetComponent=7, GameObject=7, Texture=6, UnityEngine.UI=5, SetActive=5, Instantiate/Destroy=5 | ksp: Localizer=16, HighLogic=4 | fan_in=7
- methods(25): AddActiveStratItem, AddKerbalListItem, AddStrategiesListItem, Awake, BtnInputAccept, CreateActiveStratItem, CreateActiveStratList, CreateStrategiesList, GetStrategyDescription, OnAcceptConfirm, OnCancelConfirm, OnDestroy, OnFalse, OnPopupDismiss, OnSliderCommitmentValueChanged, OnTrue, RedrawPanels, SetEffectSliderValue, SetSelectedStrategy, SetStrategyDescription, Start, UnselectStrategy, UpdateStrategyCount, UpdateStrategyDescription ...
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/AdministrationSceneDespawner.cs
- L=20 kind=MonoBehaviour type=AdministrationSceneDespawner bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(63): UnityEngine.UI=1, MonoBehaviour=1, Lifecycle=1 | ksp: GameEvents=1 | fan_in=0
- methods(2): BtnExit, Start
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/AdministrationSceneSpawner.cs
- L=35 kind=MonoBehaviour type=AdministrationSceneSpawner bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(59): Lifecycle=2, MonoBehaviour=1 | ksp: GameEvents=4, HighLogic=1 | fan_in=0
- methods(4): Awake, OnDestroy, onAdminDespawn, onAdminSpawn
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/AlarmClockApp.cs
- L=179 kind=MonoBehaviour type=AlarmClockApp bases=UIApp anc=UIApp/MonoBehaviour ns=KSP.UI.Screens
- unity(59): Transform=4, GameObject=3, UnityEngine.UI=2, SetActive=2, Lifecycle=2, Instantiate/Destroy=2 | ksp: HighLogic=8, InputLockManager=3, Game=3, Localizer=1, GameEvents=1 | fan_in=3
- methods(16): AnyTextFieldHasFocus, AppFrameHasLock, DisplayApp, EditAlarm, GetAppScenes, GetAppScreenPos, HideApp, OnAppAboutToStart, OnAppDestroy, OnAppInitialized, OnSceneSwitch, RemoveAppFrameInputLock, Reset, Start, TextFieldHasFocus, Update
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/AltimeterSliderButtons.cs
- L=325 kind=MonoBehaviour type=AltimeterSliderButtons bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(60): Coroutine=10, Lifecycle=4, Shader=3, GetComponent=3, UnityEngine.UI=2, Transform=2 | ksp: HighLogic=16, GameEvents=10, Localizer=8, Vessel=7, FlightGlobals=7 | fan_in=0
- methods(19): Awake, Collapse, Expand, GetInstance, LateUpdate, OnDestroy, OnFlightStarted, OnVesselFocusChanged, OnVesselSituationChange, Start, UnlockRecovery, UnlockReturnToKSC, UpdateForVessel, UpdateUIScale, drawExitWithoutSaveOptions, recoverVessel, returnToSpaceCenter, saveAndExit, setUnlock
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/ApplauncherLayout.cs
- L=30 kind=interface type=ApplauncherLayout bases=- anc=- ns=KSP.UI.Screens
- unity(69): UnityEngine.UI=5, GameObject=1 | ksp: none | fan_in=2
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/ApplicationLauncher.cs
- L=766 kind=MonoBehaviour type=ApplicationLauncher bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(69): GameObject=28, SetActive=24, UnityEngine.UI=17, Instantiate/Destroy=8, Transform=6, Coroutine=5 | ksp: GameEvents=30, HighLogic=18, MapView=10, GameSettings=10 | fan_in=25
- methods(46): AddApplication, AddModApplication, AddOnHideCallback, AddOnRepositionCallback, AddOnShowCallback, Awake, BtnDown, BtnUp, Contains, DetermineVisibility, DetermineVisibilityOfAllApps, DisableMutuallyExclusive, EnableMutuallyExclusive, GetAppListSize, GetKnowledgeBaseList, GetModListMaxSize, GetModListSize, Hide, MutuallyExclusiveStockApplistCLICK, MutuallyExclusiveStockApplistHOVER, MutuallyExclusiveStockApplistHOVEROUT, MutuallyExclusiveStockApplistHOVERing, OnDestroy, OnGameSettingsApplied ...
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/ApplicationLauncherButton.cs
- L=365 kind=MonoBehaviour type=ApplicationLauncherButton bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(71): Transform=7, Animation=7, Texture=6, EventSystems=6, GameObject=5, UnityEngine.UI=3 | ksp: GameEvents=2 | fan_in=13
- methods(22): AnimCoroutine, Awake, Disable, Enable, GetAnchor, GetAnchorLocal, GetAnchorTopRight, GetAnchorUL, GetAnchorUR, OnClick, OnDestroy, OnFalse, OnHover, OnHoverOut, OnTrue, PlayAnim, SetFalse, SetSprite, SetTexture, SetTrue, Setup, StopAnim
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/AppUIInputPanel.cs
- L=141 kind=MonoBehaviour type=AppUIInputPanel bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(68): Transform=6, UnityEngine.UI=2, Lifecycle=2, Instantiate/Destroy=2, GameObject=2, TMPro=1 | ksp: none | fan_in=6
- methods(9): AnyTextFieldHasFocus, DestroyChildren, GetControl, RefreshUI, ReleaseData, SetErrorState, Setup, Start, Update
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/AppUIMember.cs
- L=322 kind=MonoBehaviour type=AppUIMember bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(49): UnityEngine.UI=5, Lifecycle=3, TMPro=2, Transform=1, SetActive=1, MonoBehaviour=1 | ksp: Localizer=1 | fan_in=13
- methods(15): AnyTextFieldHasFocus, Awake, GetValue, OnAwake, OnInitialized, OnRefreshUI, OnStart, OnUpdate, RefreshUI, SetHoverTextTarget, SetValue, Setup, SortByOrder, Start, Update
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/AppUIMemberDateTime.cs
- L=279 kind=MonoBehaviour type=AppUIMemberDateTime bases=AppUIMember anc=AppUIMember/MonoBehaviour ns=KSP.UI.Screens
- unity(55): TMPro=6, KeyCode=3, Input.=3, SetActive=2, GameObject=2, Lifecycle=1 | ksp: none | fan_in=4
- methods(8): AnyTextFieldHasFocus, InputEdited, OnDestroy, OnInitialized, OnRefreshUI, OnStart, OnUpdate, SetMode
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/AppUIMemberDropdown.cs
- L=160 kind=MonoBehaviour type=AppUIMemberDropdown bases=AppUIMember anc=AppUIMember/MonoBehaviour ns=KSP.UI.Screens
- unity(48): TMPro=4, Texture=1, Lifecycle=1 | ksp: none | fan_in=7
- methods(7): BuildDropDownItemsList, GetIndexOfKey, OnDestroy, OnDropdownChanged, OnInitialized, OnRefreshUI, OnStart
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/AppUIMemberDropdownMultiLine.cs
- L=16 kind=MonoBehaviour type=AppUIMemberDropdownMultiLine bases=AppUIMemberDropdown anc=AppUIMemberDropdown/AppUIMember/MonoBehaviour ns=KSP.UI.Screens
- unity(15): none | ksp: none | fan_in=0
- methods(2): OnInitialized, OnRefreshUI
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/AppUIMemberHeading.cs
- L=70 kind=MonoBehaviour type=AppUIMemberHeading bases=AppUIMember anc=AppUIMember/MonoBehaviour ns=KSP.UI.Screens
- unity(37): UnityEngine.UI=1 | ksp: none | fan_in=0
- methods(3): AdjustHeadingBackground, OnInitialized, OnRefreshUI
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/AppUIMemberInput.cs
- L=103 kind=MonoBehaviour type=AppUIMemberInput bases=AppUIMember anc=AppUIMember/MonoBehaviour ns=KSP.UI.Screens
- unity(66): UnityEngine.UI=4, Transform=2, TMPro=2, SetActive=1, GameObject=1 | ksp: Localizer=1 | fan_in=1
- methods(6): AnyTextFieldHasFocus, InputEdited, OnInitialized, OnRefreshUI, OnStart, RefreshUIInput
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/AppUIMemberInputFloat.cs
- L=22 kind=MonoBehaviour type=AppUIMemberInputFloat bases=AppUIMemberInput anc=AppUIMemberInput/AppUIMember/MonoBehaviour ns=KSP.UI.Screens
- unity(15): none | ksp: none | fan_in=0
- methods(2): OnInitialized, RefreshUIInput
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/AppUIMemberLabel.cs
- L=39 kind=MonoBehaviour type=AppUIMemberLabel bases=AppUIMember anc=AppUIMember/MonoBehaviour ns=KSP.UI.Screens
- unity(39): TMPro=1 | ksp: none | fan_in=2
- methods(2): OnInitialized, OnRefreshUI
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/AppUIMemberLabelList.cs
- L=131 kind=MonoBehaviour type=AppUIMemberLabelList bases=AppUIMember anc=AppUIMember/MonoBehaviour ns=KSP.UI.Screens
- unity(61): GameObject=4, UnityEngine.UI=2, TMPro=2, SetActive=2, GetComponent=1 | ksp: none | fan_in=1
- methods(2): OnInitialized, OnRefreshUI
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/AppUIMemberRadioBool.cs
- L=58 kind=MonoBehaviour type=AppUIMemberRadioBool bases=AppUIMember anc=AppUIMember/MonoBehaviour ns=KSP.UI.Screens
- unity(53): UnityEngine.UI=1, TMPro=1, Lifecycle=1 | ksp: Localizer=1 | fan_in=1
- methods(4): OnDestroy, OnInitialized, OnRefreshUI, OnTrueToggleChanged
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/AppUIMemberSliderFloat.cs
- L=45 kind=MonoBehaviour type=AppUIMemberSliderFloat bases=AppUIMember anc=AppUIMember/MonoBehaviour ns=KSP.UI.Screens
- unity(46): UnityEngine.UI=1, Lifecycle=1 | ksp: none | fan_in=1
- methods(4): OnDestroy, OnInitialized, OnRefreshUI, OnSliderChanged
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/AppUIMemberSliderFloatMultiLine.cs
- L=16 kind=MonoBehaviour type=AppUIMemberSliderFloatMultiLine bases=AppUIMemberSliderFloat anc=AppUIMemberSliderFloat/AppUIMember/MonoBehaviour ns=KSP.UI.Screens
- unity(15): none | ksp: none | fan_in=0
- methods(2): OnInitialized, OnRefreshUI
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/AppUIMemberToggleBool.cs
- L=86 kind=MonoBehaviour type=AppUIMemberToggleBool bases=AppUIMember anc=AppUIMember/MonoBehaviour ns=KSP.UI.Screens
- unity(58): UnityEngine.UI=2, TMPro=2, Lifecycle=1 | ksp: Localizer=2 | fan_in=0
- methods(4): OnDestroy, OnInitialized, OnRefreshUI, OnTrueToggleChanged
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/AstronautComplex.cs
- L=375 kind=MonoBehaviour type=AstronautComplex bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(55): GetComponent=6, TMPro=5, Lifecycle=3, Transform=2, SetActive=2, GameObject=2 | ksp: HighLogic=26, Localizer=22, ProtoCrewMember=21, Game=2, ProtoVessel=1 | fan_in=5
- methods(20): AddItem, AddItem_Applicants, AddItem_Assigned, AddItem_Available, AddItem_Kia, Awake, CreateApplicantList, CreateAssignedList, CreateAvailableList, CreateKiaList, HireRecruit, InitiateGUI, OnDestroy, SetApplicantsListUnlocked, Start, UpdateCrewCosts, UpdateCrewCounts, Vbutton, Xbutton_AvailableCrew, Xbutton_MIA
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/BasePartCategorizer.cs
- L=385 kind=MonoBehaviour type=BasePartCategorizer bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(38): Coroutine=3, UnityEngine.UI=2, Time.=2, TMPro=1, MonoBehaviour=1, Lifecycle=1 | ksp: AvailablePart=56, Localizer=12, Part=1, GameEvents=1 | fan_in=4
- methods(14): Awake, GeneratePartAutoTags, LoadAutoTags, PartMatchesSearch, SearchField_OnClick, SearchField_OnEndEdit, SearchField_OnValueChange, SearchFilterResult, SearchRoutine, SearchStart, SearchStop, SearchTagSplit, TagMatchType, TermMatchesTag
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/CargoApp.cs
- L=50 kind=MonoBehaviour type=CargoApp bases=UIApp anc=UIApp/MonoBehaviour ns=KSP.UI.Screens
- unity(33): Transform=1 | ksp: none | fan_in=0
- methods(7): DisplayApp, GetAppScenes, GetAppScreenPos, HideApp, OnAppAboutToStart, OnAppDestroy, OnAppInitialized
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/ConstructionApp.cs
- L=50 kind=MonoBehaviour type=ConstructionApp bases=UIApp anc=UIApp/MonoBehaviour ns=KSP.UI.Screens
- unity(33): Transform=1 | ksp: none | fan_in=0
- methods(7): DisplayApp, GetAppScenes, GetAppScreenPos, HideApp, OnAppAboutToStart, OnAppDestroy, OnAppInitialized
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/ContractsApp.cs
- L=358 kind=MonoBehaviour type=ContractsApp bases=UIApp anc=UIApp/MonoBehaviour ns=KSP.UI.Screens
- unity(56): GameObject=6, Coroutine=5, Transform=4, Instantiate/Destroy=4, SetActive=3, UnityEngine.UI=2 | ksp: Contract=30, GameEvents=18, HighLogic=4, Localizer=3, ContractParameter=3 | fan_in=1
- methods(24): AddContract, AddSubParameter, Awake, CreateContractsList, CreateItem, CreateParameterList, DisplayApp, GetAppScenes, GetAppScreenPos, HideApp, OnAppAboutToStart, OnAppDestroy, OnAppInitialized, OnContractsLoaded, RefreshContract, RefreshContractParameter, RefreshContractRequest, RefreshContractRequested, RemoveContract, UpdateDaemon, UpdateLauncherButtonPlayAnim, UpdateLauncherButtonStopAnim, onGamePause, onGameUnPause
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/CraftEntry.cs
- L=563 kind=MonoBehaviour type=CraftEntry bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(70): UnityEngine.UI=13, GameObject=9, TMPro=8, SetActive=6, EventSystems=6, GetComponent=5 | ksp: Localizer=14, ShipConstruct=3, ConfigNode=3, HighLogic=2, Game=1 | fan_in=5
- methods(23): AttachListener, Awake, Create, GetCostTextColorTag, GetDisplayPathFrom, GetSteamFileId, GetStockThumbnailPath, GetThumbURL, GetThumbnail, HidePath, Init, IsExpansion, LoadSteamItemPreviewURL, OnPointerEnter, OnPointerExit, OnScroll, SetDirectoryController, ShowPath, Terminate, UIUpdate, UIUpdateSteamField, UpdateWidgetSize, onValueChanged
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/CraftProfileInfo.cs
- L=410 kind=class type=CraftProfileInfo bases=IConfigNode anc=IConfigNode ns=KSP.UI.Screens
- unity(5): none | ksp: ConfigNode=11, ShipConstruct=6, MissionSystem=6, ResearchAndDevelopment=3, PartLoader=3 | fan_in=8
- methods(12): CraftHasErrors, GenerateExcludedParts, GetErrorMessage, GetLastWriteTime, GetSFSMD5, GetSaveData, Load, LoadDetailsFromCraftFile, LoadFromMetaFile, PrepareCraftMetaFileLoad, Save, SaveToMetaFile
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/CraftSearch.cs
- L=185 kind=MonoBehaviour type=CraftSearch bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(80): GameObject=8, GetComponent=6, Coroutine=6, SetActive=4, UnityEngine.UI=3, Transform=3 | ksp: InputLockManager=2, Localizer=1 | fan_in=3
- methods(12): Awake, CraftMatchesSearch, HasFiltered, OnDestroy, OnDisable, SearchField_OnClick, SearchField_OnEndEdit, SearchField_OnValueChange, SearchRoutine, SearchStart, Start, StopSearch
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/CurrencyWidgetsApp.cs
- L=111 kind=MonoBehaviour type=CurrencyWidgetsApp bases=UIApp anc=UIApp/MonoBehaviour ns=KSP.UI.Screens
- unity(89): Transform=10, GameObject=9, UnityEngine.UI=4, Instantiate/Destroy=4, GetComponent=3, SetActive=2 | ksp: ResearchAndDevelopment=2, Reputation=2, Funding=2 | fan_in=0
- methods(9): DespawnWidgets, DisplayApp, GetAppScenes, GetAppScreenPos, HideApp, OnAppAboutToStart, OnAppDestroy, OnAppInitialized, SpawnWidgets
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/AddDebugScreens.cs
- L=56 kind=MonoBehaviour type=AddDebugScreens bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar
- unity(51): UnityEngine.UI=1, MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=1
- methods(1): Start
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/DebugScreen.cs
- L=386 kind=MonoBehaviour type=DebugScreen bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar
- unity(65): UnityEngine.UI=12, GetComponent=11, SetActive=5, GameObject=5, Lifecycle=3, TMPro=1 | ksp: HighLogic=4, InputLockManager=2, GameEvents=2, Game=1 | fan_in=2
- methods(14): AddContentItem, AddContentScreen, AddItem, Awake, GetItem, Hide, OnDestroy, OnExitClick, OnSelectItem, ResetCheats, SelectItem, Show, Update, onGameSceneLoad
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/DebugScreenCheats.cs
- L=45 kind=MonoBehaviour type=DebugScreenCheats bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar
- unity(94): UnityEngine.UI=11, Lifecycle=2, TMPro=1, MonoBehaviour=1 | ksp: none | fan_in=0
- methods(2): Awake, OnDestroy
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/DebugScreenConsole.cs
- L=347 kind=MonoBehaviour type=DebugScreenConsole bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar
- unity(57): Lifecycle=6, Coroutine=4, EventSystems=3, UnityEngine.UI=2, KeyCode=2, Input.=2 | ksp: InputLockManager=2, GameEvents=2, HighLogic=1, GameSettings=1 | fan_in=1
- methods(20): AddConsoleCommand, Awake, FormatLogEntry, GetBracketedColorCode, GetCommand, InitializeLog, OnConsoleCommand, OnDestroy, OnDisable, OnEnable, OnGameSceneLoadRequested, OnInputEndEdit, OnMemoryLogUpdated, OnSubmitclick, ParseInputString, RemoveConsoleCommand, ScrollDown, Start, SubmitCommand, Update
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/DebugScreenInputLocks.cs
- L=24 kind=MonoBehaviour type=DebugScreenInputLocks bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar
- unity(77): TMPro=2, Lifecycle=2, MonoBehaviour=1 | ksp: none | fan_in=0
- methods(2): Awake, OnDestroy
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/DebugScreenScreenToggle.cs
- L=54 kind=MonoBehaviour type=DebugScreenScreenToggle bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar
- unity(82): SetActive=8, GameObject=2, UnityEngine.UI=1, MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=0
- methods(3): Awake, OnToggleClick, SetShown
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/DebugScreenSpawner.cs
- L=87 kind=MonoBehaviour type=DebugScreenSpawner bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar
- unity(75): Lifecycle=4, SceneManager=3, Transform=2, MonoBehaviour=1, KeyCode=1, Instantiate/Destroy=1 | ksp: InputLockManager=1, GameSettings=1 | fan_in=2
- methods(7): Awake, OnDestroy, OnSceneLoaded, ShowDebugScreen, ShowTab, Start, Update
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/ScreenDatabaseAssemblies.cs
- L=14 kind=MonoBehaviour type=ScreenDatabaseAssemblies bases=ScreenDatabaseList anc=ScreenDatabaseList/MonoBehaviour ns=KSP.UI.Screens.DebugToolbar
- unity(15): none | ksp: none | fan_in=0
- methods(1): LoadDatabaseListItems
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/ScreenDatabaseAudio.cs
- L=14 kind=MonoBehaviour type=ScreenDatabaseAudio bases=ScreenDatabaseList anc=ScreenDatabaseList/MonoBehaviour ns=KSP.UI.Screens.DebugToolbar
- unity(15): none | ksp: GameDatabase=2 | fan_in=0
- methods(1): LoadDatabaseListItems
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/ScreenDatabaseConfig.cs
- L=17 kind=MonoBehaviour type=ScreenDatabaseConfig bases=ScreenDatabaseList anc=ScreenDatabaseList/MonoBehaviour ns=KSP.UI.Screens.DebugToolbar
- unity(15): none | ksp: GameDatabase=1 | fan_in=0
- methods(1): LoadDatabaseListItems
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/ScreenDatabaseList.cs
- L=40 kind=MonoBehaviour type=ScreenDatabaseList bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar
- unity(72): Lifecycle=2, Coroutine=2, UnityEngine.UI=1, MonoBehaviour=1 | ksp: none | fan_in=5
- methods(4): Awake, LoadDatabaseListItems, OnEnable, ScrollUp
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/ScreenDatabaseModels.cs
- L=14 kind=MonoBehaviour type=ScreenDatabaseModels bases=ScreenDatabaseList anc=ScreenDatabaseList/MonoBehaviour ns=KSP.UI.Screens.DebugToolbar
- unity(15): none | ksp: GameDatabase=2 | fan_in=0
- methods(1): LoadDatabaseListItems
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/ScreenDatabaseStrings.cs
- L=95 kind=MonoBehaviour type=ScreenDatabaseStrings bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar
- unity(69): TMPro=4, UnityEngine.UI=3, Lifecycle=2, MonoBehaviour=1 | ksp: Localizer=9 | fan_in=0
- methods(4): FormatKeyValue, OnDestroy, OnSearchClicked, Start
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/ScreenDatabaseTextures.cs
- L=14 kind=MonoBehaviour type=ScreenDatabaseTextures bases=ScreenDatabaseList anc=ScreenDatabaseList/MonoBehaviour ns=KSP.UI.Screens.DebugToolbar
- unity(15): none | ksp: GameDatabase=2 | fan_in=0
- methods(1): LoadDatabaseListItems
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/AllowPartClipping.cs
- L=15 kind=MonoBehaviour type=AllowPartClipping bases=DebugScreenToggle anc=DebugScreenToggle/MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- unity(15): none | ksp: none | fan_in=1
- methods(2): OnToggleChanged, SetupValues
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/BiomesVisibleInMap.cs
- L=47 kind=MonoBehaviour type=BiomesVisibleInMap bases=DebugScreenToggle anc=DebugScreenToggle/MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- unity(83): Material=3, Shader=2, Renderer=2, GetComponent=1, GameObject=1 | ksp: FlightGlobals=2, CelestialBody=1 | fan_in=1
- methods(3): OnToggleChanged, SetBiomesVisible, SetupValues
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/CheatsCareer.cs
- L=416 kind=MonoBehaviour type=CheatsCareer bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- unity(71): UnityEngine.UI=17, TMPro=15, GetComponent=12, Lifecycle=4, MonoBehaviour=1 | ksp: ResearchAndDevelopment=14, Reputation=11, Funding=11, HighLogic=6, GameEvents=6 | fan_in=0
- methods(31): Awake, FixRepairableParts, IsVesselWithinRange, OnDestroy, OnFundsChanged, OnFundsNegBigClick, OnFundsNegClick, OnFundsPosBigClick, OnFundsPosClick, OnMaxFacilityClick, OnMaxTechClick, OnMaxXPClick, OnRepNegBigClick, OnRepNegClick, OnRepPosBigClick, OnRepPosClick, OnReputationChanged, OnScienceChanged, OnScienceNegBigClick, OnScienceNegClick, OnSciencePosBigClick, OnSciencePosClick, SetFundingActive, SetMaxFacilityActive ...
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/CheatsComets.cs
- L=381 kind=MonoBehaviour type=CheatsComets bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- unity(71): UnityEngine.UI=25, TMPro=6, SetActive=4, GameObject=4, Lifecycle=3, Time.=2 | ksp: FlightGlobals=4, Vessel=2 | fan_in=0
- methods(30): Awake, OnCollidersClick, OnComaClick, OnDustClick, OnDustMultiplierChange, OnDustSizeMultiplierChange, OnDustTailClick, OnGeyserMultiplierChange, OnGeysersClick, OnInterstellarClick, OnIonMultiplierChange, OnIonTailClick, OnKillClick, OnLongClick, OnMidClick, OnRandomClick, OnReprimeParticles, OnSentinelClick, OnSentinelInterstellarClick, OnSentinelLongClick, OnSentinelMidClick, OnSentinelShortClick, OnShortClick, OnSnowMultiplierChange ...
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/CheatsObjectThrower.cs
- L=193 kind=MonoBehaviour type=CheatsObjectThrower bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- unity(86): GameObject=12, Rigidbody=7, UnityEngine.UI=6, TMPro=5, GetComponent=4, Lifecycle=3 | ksp: FlightCamera=5, Localizer=2 | fan_in=0
- methods(10): Awake, OnArmedToggle, OnMassSet, OnObjectNext, OnObjectPrev, OnSizeSet, OnSpeedSet, SetUI, Start, Update
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/DestroyAfterTime.cs
- L=24 kind=MonoBehaviour type=DestroyAfterTime bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- unity(80): Time.=2, Lifecycle=2, GameObject=2, MonoBehaviour=1, Instantiate/Destroy=1 | ksp: none | fan_in=0
- methods(2): Start, Update
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/HackGravity.cs
- L=134 kind=MonoBehaviour type=HackGravity bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- unity(50): UnityEngine.UI=3, Lifecycle=2, TMPro=1, MonoBehaviour=1 | ksp: FlightGlobals=5, CelestialBody=4 | fan_in=1
- methods(10): Awake, CreateOriginalGeeForces, OnResetClick, OnSliderChanged, OnToggleChanged, SetGravityFactor, SetHack, SetText, SetupValues, Update
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/IgnoreEVAConstructionMassLimit.cs
- L=15 kind=MonoBehaviour type=IgnoreEVAConstructionMassLimit bases=DebugScreenToggle anc=DebugScreenToggle/MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- unity(15): none | ksp: none | fan_in=1
- methods(2): OnToggleChanged, SetupValues
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/IgnoreKerbalInventoryLimits.cs
- L=15 kind=MonoBehaviour type=IgnoreKerbalInventoryLimits bases=DebugScreenToggle anc=DebugScreenToggle/MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- unity(15): none | ksp: none | fan_in=0
- methods(2): OnToggleChanged, SetupValues
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/IgnoreMaxTemperature.cs
- L=15 kind=MonoBehaviour type=IgnoreMaxTemperature bases=DebugScreenToggle anc=DebugScreenToggle/MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- unity(15): none | ksp: none | fan_in=1
- methods(2): OnToggleChanged, SetupValues
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/InfiniteElectricity.cs
- L=15 kind=MonoBehaviour type=InfiniteElectricity bases=DebugScreenToggle anc=DebugScreenToggle/MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- unity(15): none | ksp: none | fan_in=1
- methods(2): OnToggleChanged, SetupValues
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/InfinitePropellant.cs
- L=15 kind=MonoBehaviour type=InfinitePropellant bases=DebugScreenToggle anc=DebugScreenToggle/MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- unity(15): none | ksp: none | fan_in=1
- methods(2): OnToggleChanged, SetupValues
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/MaxProgression.cs
- L=27 kind=MonoBehaviour type=MaxProgression bases=MonoBehaviour,IPointerClickHandler,IEventSystemHandler anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- unity(80): EventSystems=4, MonoBehaviour=1 | ksp: Localizer=2, FlightGlobals=1 | fan_in=0
- methods(1): OnPointerClick
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/MiddleMouseClickSetPosition.cs
- L=15 kind=MonoBehaviour type=MiddleMouseClickSetPosition bases=DebugScreenToggle anc=DebugScreenToggle/MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- unity(15): none | ksp: none | fan_in=1
- methods(2): OnToggleChanged, SetupValues
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/NoCrashDamage.cs
- L=15 kind=MonoBehaviour type=NoCrashDamage bases=DebugScreenToggle anc=DebugScreenToggle/MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- unity(15): none | ksp: none | fan_in=1
- methods(2): OnToggleChanged, SetupValues
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/NonStrictPartAttachment.cs
- L=15 kind=MonoBehaviour type=NonStrictPartAttachment bases=DebugScreenToggle anc=DebugScreenToggle/MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- unity(15): none | ksp: none | fan_in=1
- methods(2): OnToggleChanged, SetupValues
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/PauseOnVesselUnpack.cs
- L=15 kind=MonoBehaviour type=PauseOnVesselUnpack bases=DebugScreenToggle anc=DebugScreenToggle/MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- unity(15): none | ksp: none | fan_in=1
- methods(2): OnToggleChanged, SetupValues
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/SetOrbit.cs
- L=299 kind=MonoBehaviour type=SetOrbit bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- unity(49): UnityEngine.UI=7, TMPro=3, Lifecycle=3, MonoBehaviour=1 | ksp: FlightGlobals=23, Localizer=13, Vessel=6, CelestialBody=2, FloatingOrigin=1 | fan_in=0
- methods(13): Awake, CheckForErrors, OnBodyBackClick, OnBodyForwardClick, OnDestroy, OnRendezvousClick, OnSetOrbitClick, OnVesselBackClick, OnVesselForwardClick, ResetErrorMsg, SetSelectedBodyText, SetSelectedVesselText, Start
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/SetPosition.cs
- L=383 kind=MonoBehaviour type=SetPosition bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- unity(66): UnityEngine.UI=8, Lifecycle=5, GameObject=5, SetActive=4, TMPro=3, Input.=3 | ksp: FlightGlobals=36, Localizer=12, GameEvents=7, Vessel=6, CelestialBody=3 | fan_in=0
- methods(22): Awake, CheckForErrors, CheckForHiddenElements, DisableActiveVesselEaseIn, FormatValues, GetSelectedBodyIndex, GetSugestedAltitude, InitializeValues, OnBodyBackClick, OnBodyForwardClick, OnDestroy, OnEnable, OnSceneExit, OnSetPositionClick, OnVesselChanged, OnVesselSituationChanged, ResetErrorMsg, SetPositionValues, SetSelectedBodyText, Start, Update, UpdateSliderValue
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/StringsMissingKeysToggle.cs
- L=17 kind=MonoBehaviour type=StringsMissingKeysToggle bases=DebugScreenToggle anc=DebugScreenToggle/MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- unity(15): none | ksp: Localizer=2 | fan_in=0
- methods(2): OnToggleChanged, SetupValues
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/StringsOverrideMELock.cs
- L=18 kind=MonoBehaviour type=StringsOverrideMELock bases=DebugScreenToggle anc=DebugScreenToggle/MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- unity(15): none | ksp: Localizer=2, GameEvents=1 | fan_in=0
- methods(2): OnToggleChanged, SetupValues
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/StringsShowKeysToggle.cs
- L=17 kind=MonoBehaviour type=StringsShowKeysToggle bases=DebugScreenToggle anc=DebugScreenToggle/MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- unity(15): none | ksp: Localizer=2 | fan_in=0
- methods(2): OnToggleChanged, SetupValues
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Cheats/UnbreakableJoints.cs
- L=15 kind=MonoBehaviour type=UnbreakableJoints bases=DebugScreenToggle anc=DebugScreenToggle/MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Cheats
- unity(15): none | ksp: none | fan_in=1
- methods(2): OnToggleChanged, SetupValues
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Contract/ScreenAddContracts.cs
- L=92 kind=MonoBehaviour type=ScreenAddContracts bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Contract
- unity(65): Lifecycle=3, Instantiate/Destroy=3, Transform=2, UnityEngine.UI=1, MonoBehaviour=1, GameObject=1 | ksp: GameEvents=4, Localizer=1, Game=1, Contract=1, ConfigNode=1 | fan_in=0
- methods(9): ClearItems, CreateError, CreateItem, OnDestroy, OnGameStateCreated, OnGameStateLoaded, RefreshItems, Start, Update
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Contract/ScreenContractExistingItem.cs
- L=142 kind=MonoBehaviour type=ScreenContractExistingItem bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Contract
- unity(68): TMPro=6, SetActive=3, GameObject=3, UnityEngine.UI=2, MonoBehaviour=1, Lifecycle=1 | ksp: Contract=23, Localizer=7 | fan_in=1
- methods(5): OnLeftButtonClicked, OnRightButtonClicked, Setup, SetupError, Start
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Contract/ScreenContractList.cs
- L=173 kind=MonoBehaviour type=ScreenContractList bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Contract
- unity(50): Lifecycle=3, Instantiate/Destroy=3, Transform=2, UnityEngine.UI=1, MonoBehaviour=1, GameObject=1 | ksp: Contract=25, GameEvents=22, Localizer=5, Game=1, ConfigNode=1 | fan_in=1
- methods(11): ClearItems, CreateError, CreateItem, OnContractsModified, OnDestroy, OnGameStateCreated, OnGameStateLoaded, RefreshItems, SetDirty, Start, Update
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Contract/ScreenContractNewItem.cs
- L=94 kind=MonoBehaviour type=ScreenContractNewItem bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Contract
- unity(74): UnityEngine.UI=3, SetActive=3, GameObject=3, TMPro=2, MonoBehaviour=1, Lifecycle=1 | ksp: Contract=6 | fan_in=1
- methods(7): GenerateContract, OnExceptionalClicked, OnSignificantClicked, OnTrivialClicked, Setup, SetupError, Start
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Contract/ScreenContractTools.cs
- L=69 kind=MonoBehaviour type=ScreenContractTools bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Contract
- unity(66): UnityEngine.UI=4, Lifecycle=2, MonoBehaviour=1 | ksp: Contract=1 | fan_in=0
- methods(7): OnClearCurrentClicked, OnClearFinishedClicked, OnRegenerateCurrentClicked, OnResetWeightsClicked, SetInteractable, Start, Update
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Database/ScreenDatabaseOverview.cs
- L=149 kind=MonoBehaviour type=ScreenDatabaseOverview bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Database
- unity(72): TMPro=9, Coroutine=3, UnityEngine.UI=2, Lifecycle=2, MonoBehaviour=1 | ksp: GameDatabase=15, Localizer=4, PartLoader=3, HighLogic=2, GameEvents=2 | fan_in=0
- methods(8): CheckInteractable, FlushCounts, OnDestroy, OnReloadClicked, OnReloadTextsClicked, RecompileAssets, ResetCounts, Start
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Debug/CalcType.cs
- L=14 kind=enum type=CalcType bases=- anc=- ns=KSP.UI.Screens.DebugToolbar.Screens.Debug
- unity(0): none | ksp: none | fan_in=6
- methods(0): 
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Debug/DeltaVEngineInfoListExtensions.cs
- L=26 kind=static class type=DeltaVEngineInfoListExtensions bases=- anc=- ns=KSP.UI.Screens.DebugToolbar.Screens.Debug
- unity(0): none | ksp: none | fan_in=0
- methods(1): Get
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Debug/DeltaVPropellantInfoListExtensions.cs
- L=26 kind=static class type=DeltaVPropellantInfoListExtensions bases=- anc=- ns=KSP.UI.Screens.DebugToolbar.Screens.Debug
- unity(0): none | ksp: none | fan_in=0
- methods(1): Get
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Debug/DeltaVStageInfoListExtensions.cs
- L=26 kind=static class type=DeltaVStageInfoListExtensions bases=- anc=- ns=KSP.UI.Screens.DebugToolbar.Screens.Debug
- unity(0): none | ksp: none | fan_in=0
- methods(1): Get
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Debug/GraphFPS.cs
- L=100 kind=MonoBehaviour type=GraphFPS bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Debug
- unity(72): Coroutine=5, Lifecycle=4, TMPro=3, UnityEngine.UI=1, MonoBehaviour=1 | ksp: none | fan_in=0
- methods(7): Awake, OnDisable, OnEnable, Start, UpdateCoroutine, UpdateDisplay, UpdateText
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Debug/Memory.cs
- L=91 kind=MonoBehaviour type=Memory bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Debug
- unity(75): TMPro=6, Coroutine=5, Lifecycle=2, MonoBehaviour=1 | ksp: none | fan_in=0
- methods(5): GetMemoryText, OnDisable, OnEnable, UpdateCoroutine, UpdateDisplay
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Debug/ScreenDeltaVEngineInfo.cs
- L=63 kind=MonoBehaviour type=ScreenDeltaVEngineInfo bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Debug
- unity(81): TMPro=7, MonoBehaviour=1 | ksp: Localizer=6 | fan_in=3
- methods(1): UpdateData
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Debug/ScreenDeltaVInfo.cs
- L=306 kind=MonoBehaviour type=ScreenDeltaVInfo bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Debug
- unity(68): UnityEngine.UI=6, GameObject=6, TMPro=5, SceneManager=4, Lifecycle=4, Instantiate/Destroy=2 | ksp: GameEvents=10, Localizer=9, HighLogic=6, Vessel=4, FlightGlobals=4 | fan_in=0
- methods(14): Awake, CacheTypeStrings, ChangeVessel, ClearScreenObjects, FindVessel, OnDestroy, OnLogVerboseToggle, OnSceneLoaded, SetSiblingsUIIndex, Start, Update, onEditorLoad, onEditorPodPicked, onEditorRestoreState
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Debug/ScreenDeltaVPartsListInfo.cs
- L=16 kind=MonoBehaviour type=ScreenDeltaVPartsListInfo bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Debug
- unity(59): TMPro=1, MonoBehaviour=1 | ksp: none | fan_in=1
- methods(1): UpdateData
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Debug/ScreenDeltaVPropellantInfo.cs
- L=41 kind=MonoBehaviour type=ScreenDeltaVPropellantInfo bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Debug
- unity(73): TMPro=3, MonoBehaviour=1 | ksp: none | fan_in=2
- methods(1): UpdateData
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Debug/ScreenDeltaVStageInfo.cs
- L=85 kind=MonoBehaviour type=ScreenDeltaVStageInfo bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Debug
- unity(87): TMPro=13, MonoBehaviour=1 | ksp: Localizer=6 | fan_in=2
- methods(1): UpdateData
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Debug/ScreenFlightInfo.cs
- L=194 kind=MonoBehaviour type=ScreenFlightInfo bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Debug
- unity(78): TMPro=16, Lifecycle=5, Time.=4, SetActive=1, MonoBehaviour=1, GameObject=1 | ksp: Localizer=10, FlightGlobals=8, GameEvents=6, Vessel=3, Planetarium=2 | fan_in=0
- methods(13): Awake, FixedUpdate, GetActiveVesselName, LateUpdate, OnDestroy, SetLocation, SetPhysics, SetUT, SetVessel, SetVesselSlopeAngle, Start, onGamePause, onGameUnPause
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Debug/ScreenVesselMassInfo.cs
- L=194 kind=MonoBehaviour type=ScreenVesselMassInfo bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Debug
- unity(68): SceneManager=4, GameObject=4, Lifecycle=3, UnityEngine.UI=2, Transform=2, TMPro=2 | ksp: GameEvents=15, ShipConstruct=3, Part=3, EditorLogic=3, Vessel=2 | fan_in=0
- methods(13): Awake, ChangeCrew, ChangeShip, ChangeVessel, ClearScreenObjects, FindVessel, OnDestroy, OnSceneLoaded, Start, UpdateList, onEditorLoad, onEditorPodPicked, onEditorRestoreState
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Debug/ScreenVesselMassPartInfo.cs
- L=58 kind=MonoBehaviour type=ScreenVesselMassPartInfo bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Debug
- unity(90): TMPro=9, SetActive=1, MonoBehaviour=1, GameObject=1 | ksp: Part=2 | fan_in=1
- methods(1): UpdateData
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Debug/ShowInputLocks.cs
- L=37 kind=MonoBehaviour type=ShowInputLocks bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Debug
- unity(71): Lifecycle=2, UnityEngine.UI=1, TMPro=1, MonoBehaviour=1 | ksp: InputLockManager=2 | fan_in=0
- methods(4): Awake, GetText, OnClearClick, Update
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Debug/VersionInfo.cs
- L=22 kind=MonoBehaviour type=VersionInfo bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Debug
- unity(67): TMPro=1, MonoBehaviour=1, Lifecycle=1 | ksp: Localizer=1 | fan_in=0
- methods(1): Start
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Debug/VesselDebugDeltaVEngineInfo.cs
- L=131 kind=class type=VesselDebugDeltaVEngineInfo bases=- anc=- ns=KSP.UI.Screens.DebugToolbar.Screens.Debug
- unity(70): GameObject=17, Transform=7, SetActive=4, Instantiate/Destroy=4, GetComponent=3, UnityEngine.UI=2 | ksp: none | fan_in=1
- methods(5): Create, Destroy, SetActive, SetPropellantUISiblings, Update
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Debug/VesselDebugDeltaVInfo.cs
- L=177 kind=class type=VesselDebugDeltaVInfo bases=- anc=- ns=KSP.UI.Screens.DebugToolbar.Screens.Debug
- unity(58): Transform=8, GameObject=8, SetActive=6, Instantiate/Destroy=5, GetComponent=2, UnityEngine.UI=1 | ksp: none | fan_in=1
- methods(5): Create, Destroy, SetActive, SetSiblingsUIIndex, Update
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/DebugScreenInputDouble.cs
- L=50 kind=MonoBehaviour type=DebugScreenInputDouble bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens
- unity(64): TMPro=2, Lifecycle=2, MonoBehaviour=1 | ksp: none | fan_in=2
- methods(3): Awake, OnInputChanged, Start
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/DebugScreenSlider.cs
- L=52 kind=MonoBehaviour type=DebugScreenSlider bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens
- unity(58): UnityEngine.UI=1, TMPro=1, MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=0
- methods(5): Awake, OnSliderChanged, SetSlider, SetSliderText, SetupValues
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/DebugScreenToggle.cs
- L=58 kind=MonoBehaviour type=DebugScreenToggle bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens
- unity(56): UnityEngine.UI=1, TMPro=1, MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=16
- methods(6): Awake, OnToggleChanged, Set, SetToggle, SetToggleText, SetupValues
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/GamePlay/Difficulty.cs
- L=67 kind=MonoBehaviour type=Difficulty bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.GamePlay
- unity(41): Lifecycle=2, MonoBehaviour=1 | ksp: HighLogic=13 | fan_in=0
- methods(2): Start, Update
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/GamePlay/KerbalScreen.cs
- L=210 kind=MonoBehaviour type=KerbalScreen bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.GamePlay
- unity(58): GameObject=5, SetActive=4, Lifecycle=4, Instantiate/Destroy=3, UnityEngine.UI=1, Transform=1 | ksp: GameEvents=10, ProtoCrewMember=9, HighLogic=4, Game=1, ConfigNode=1 | fan_in=0
- methods(15): AddKerbal, Awake, ClearList, CreateItem, OnDestroy, OnGameStateCreated, OnGameStateLoaded, OnKerbalAdded, OnKerbalRemoved, OnKerbalStatusChange, RemoveKerbal, Start, Update, UpdateKerbal, UpdateRoster
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/GamePlay/KerbalScreenItem.cs
- L=33 kind=MonoBehaviour type=KerbalScreenItem bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.GamePlay
- unity(86): TMPro=6, MonoBehaviour=1 | ksp: ProtoCrewMember=4 | fan_in=1
- methods(1): Setup
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/GamePlay/ScreenKerbalCreate.cs
- L=348 kind=MonoBehaviour type=ScreenKerbalCreate bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.GamePlay
- unity(56): UnityEngine.UI=13, TMPro=3, Coroutine=3, Lifecycle=2, MonoBehaviour=1 | ksp: ProtoCrewMember=13, KerbalRoster=12, GameEvents=12, HighLogic=7, Localizer=6 | fan_in=0
- methods(22): AddListeners, CheckForErrors, GameLoaded, InitializeLocals, KerbalsModified, Lock, OnBadassToggle, OnClassToggle, OnCourageLevelSet, OnDestroy, OnExperienceLevelSet, OnGenderToggle, OnNameEdit, OnRandomClicked, OnStupidityLevelSet, OnSubmitClicked, OnVeteranToggle, RefreshControls, SetAttributes, Start, SubmitConfirmation, Unlock
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Mission/ScreenAddMissions.cs
- L=91 kind=MonoBehaviour type=ScreenAddMissions bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Mission
- unity(65): Lifecycle=3, Instantiate/Destroy=3, Transform=2, UnityEngine.UI=1, MonoBehaviour=1, GameObject=1 | ksp: GameEvents=4, MissionSystem=3, Game=1, ConfigNode=1 | fan_in=0
- methods(9): ClearItems, CreateError, CreateItem, OnDestroy, OnGameStateCreated, OnGameStateLoaded, RefreshItems, Start, Update
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Mission/ScreenMissionExistingItem.cs
- L=65 kind=MonoBehaviour type=ScreenMissionExistingItem bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Mission
- unity(84): TMPro=6, UnityEngine.UI=2, SetActive=2, GameObject=2, MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=2
- methods(5): OnLeftButtonClicked, OnRightButtonClicked, Setup, SetupError, Start
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Mission/ScreenMissionList.cs
- L=140 kind=MonoBehaviour type=ScreenMissionList bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Mission
- unity(55): Lifecycle=3, Instantiate/Destroy=3, Transform=2, UnityEngine.UI=1, MonoBehaviour=1, GameObject=1 | ksp: GameEvents=22, Contract=19, MissionSystem=2, Game=1, ConfigNode=1 | fan_in=0
- methods(11): ClearItems, CreateError, CreateItem, OnContractsModified, OnDestroy, OnGameStateCreated, OnGameStateLoaded, RefreshItems, SetDirty, Start, Update
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Mission/ScreenMissionNewItem.cs
- L=24 kind=MonoBehaviour type=ScreenMissionNewItem bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Mission
- unity(72): TMPro=2, MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=0
- methods(2): SetupError, Start
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Mission/ScreenMissionTools.cs
- L=18 kind=MonoBehaviour type=ScreenMissionTools bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Mission
- unity(70): Lifecycle=2, UnityEngine.UI=1, MonoBehaviour=1 | ksp: none | fan_in=0
- methods(2): Start, Update
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Physics/ScreenAero.cs
- L=123 kind=MonoBehaviour type=ScreenAero bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Physics
- unity(70): UnityEngine.UI=7, TMPro=4, MonoBehaviour=1, Lifecycle=1 | ksp: PhysicsGlobals=23 | fan_in=0
- methods(9): AddListeners, OnBodyLiftSet, OnDataActionMenusToggle, OnDataGUIToggle, OnForceDisplayScaleSet, OnForcesInFlightToggle, OnGlobalLiftSet, OnLiftDragSet, Start
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Physics/ScreenDrag.cs
- L=134 kind=MonoBehaviour type=ScreenDrag bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Physics
- unity(67): UnityEngine.UI=8, TMPro=3, MonoBehaviour=1, Lifecycle=1 | ksp: PhysicsGlobals=32 | fan_in=0
- methods(11): AddListeners, CheckInteractable, OnAngularDragSet, OnApplyDragToggle, OnCubeMultiplierSet, OnDragAccelerationToggle, OnDragSpheresToggle, OnGlobalDragSet, OnNonPhysicalCoMToggle, OnNonPhysicalDragToggle, Start
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Physics/ScreenKerbalEVAMaterial.cs
- L=175 kind=MonoBehaviour type=ScreenKerbalEVAMaterial bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Physics
- unity(87): TMPro=13, UnityEngine.UI=12, PhysicMaterial=12, MonoBehaviour=1, Lifecycle=1 | ksp: PhysicsGlobals=7 | fan_in=0
- methods(7): AddListeners, ApplyChanges, SlidersUpdated, Start, UpdateBounceCombine, UpdateFrictionCombine, UpdateValuesFromPhysiscsGlobals
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Physics/ScreenPhysics.cs
- L=61 kind=MonoBehaviour type=ScreenPhysics bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Physics
- unity(70): UnityEngine.UI=4, TMPro=1, MonoBehaviour=1, Lifecycle=1 | ksp: PhysicsGlobals=8, GameSettings=2 | fan_in=0
- methods(5): OnAutostrutVisualizeToggled, OnLegacyOrbitTargetingToggled, OnLoadClicked, OnSaveClicked, Start
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Physics/ScreenThermal.cs
- L=211 kind=MonoBehaviour type=ScreenThermal bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Physics
- unity(70): UnityEngine.UI=13, TMPro=7, MonoBehaviour=1, Lifecycle=1 | ksp: PhysicsGlobals=43 | fan_in=0
- methods(15): AddListeners, OnConductionEnabledToggle, OnConductionFactorSet, OnConvectionEnabledToggle, OnConvectionFactorSet, OnDataActionMenusToggle, OnDataGUIToggle, OnDebugColorsToggle, OnGenerationFactorSet, OnInsolationFactorSet, OnLuminosityFactorSet, OnNewtonianFactorSet, OnRadiationEnabledToggle, OnRadiationFactorSet, Start
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Resources/Debugging.cs
- L=101 kind=MonoBehaviour type=Debugging bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Resources
- unity(65): UnityEngine.UI=7, MonoBehaviour=1, Lifecycle=1 | ksp: GameSettings=27, Localizer=2 | fan_in=0
- methods(9): AddListeners, OnErrorToggle, OnExceptionToggle, OnInstantFlushToggle, OnMissingKeysToggle, OnShowLocalizationKeysToggle, OnShowVesselNamingPartsToggle, OnUIMasterPixelPerfectToggle, Start
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Resources/ScreenResources.cs
- L=44 kind=MonoBehaviour type=ScreenResources bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Resources
- unity(63): UnityEngine.UI=2, MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=0
- methods(4): AddListeners, OnDebugInfoToggle, OnHeatGenerationToggle, Start
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Serenity/ScreenRobotics.cs
- L=26 kind=MonoBehaviour type=ScreenRobotics bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Serenity
- unity(63): UnityEngine.UI=1, MonoBehaviour=1, Lifecycle=1 | ksp: PhysicsGlobals=2 | fan_in=0
- methods(3): AddListeners, OnDataActionMenusToggle, Start
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Serenity/ScreenROCs.cs
- L=96 kind=MonoBehaviour type=ScreenROCs bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Serenity
- unity(58): UnityEngine.UI=3, Lifecycle=2, TMPro=1, MonoBehaviour=1 | ksp: Localizer=2, GameSettings=2, GameEvents=2 | fan_in=0
- methods(7): AddListeners, OnDestroy, OnROCFinderToggle, OnROCScanPointsToggle, OnROCStatsEnabledToggle, OnStatsChanged, Start
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DebugToolbar/Screens/Serenity/ScreenSerenity.cs
- L=16 kind=MonoBehaviour type=ScreenSerenity bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.DebugToolbar.Screens.Serenity
- unity(50): MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=0
- methods(2): AddListeners, Start
- auto: C / Debug

## Assembly-CSharp/KSP/UI/Screens/DirectoryController.cs
- L=756 kind=MonoBehaviour type=DirectoryController bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(78): GameObject=52, Transform=44, GetComponent=26, UnityEngine.UI=15, SetActive=5, Instantiate/Destroy=5 | ksp: HighLogic=15, Game=7, Localizer=5, MissionSystem=2, EditorLogic=1 | fan_in=4
- methods(30): Awake, BuildDirectoryUI, BuildGameNameDirectories, BuildMissionDirectory, BuildMissionDirectoryUI, BuildSteamDirectoryUI, BuildStockDirectoryUI, BuildSubdirectories, CraftListUpdate, CreateDirectory, CreateEditorDirectory, DeselectCurrentDirectory, DirectoryCreatedCallback, DirectoryDeletedCallback, ExpansionDirectories, FindDirectory, InitForCraftSave, OnEnable, OnScroll, OverrideDisplay, PrioritizeCurrentGameDirectory, RestoreDisplay, SelectDirectory, ShowDirectoryTreeForEditor ...
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Editor/InventoryPartListTooltip.cs
- L=216 kind=MonoBehaviour type=InventoryPartListTooltip bases=Tooltip,IPointerExitHandler,IEventSystemHandler anc=Tooltip/MonoBehaviour ns=KSP.UI.Screens.Editor
- unity(83): GameObject=10, SetActive=8, UnityEngine.UI=7, EventSystems=6, Transform=5, Coroutine=4 | ksp: AvailablePart=5, GameSettings=2, ProtoPartSnapshot=1, PartModule=1, Part=1 | fan_in=2
- methods(8): CreateInfoWidgets, GetNewTooltipWidget, HidePreviousTooltipWidgets, OnDestroy, RefreshToolTipHeight, Setup, Update, WaitAndRefreshHeight
- auto: C / Editor

## Assembly-CSharp/KSP/UI/Screens/Editor/InventoryPartListTooltipController.cs
- L=164 kind=MonoBehaviour type=InventoryPartListTooltipController bases=PinnableTooltipController anc=PinnableTooltipController/TooltipController/MonoBehaviour ns=KSP.UI.Screens.Editor
- unity(70): UnityEngine.UI=5, EventSystems=5, SetActive=3, Transform=2, GameObject=2, Time.=1 | ksp: HighLogic=1, AvailablePart=1 | fan_in=3
- methods(13): CreateTooltip, DestroyTooltip, OnPointerClick, OnPointerEnter, OnPointerExit, OnTooltipAboutToDespawn, OnTooltipAboutToSpawn, OnTooltipDespawned, OnTooltipPinned, OnTooltipSpawned, OnTooltipUnpinned, PinToolTip, Update
- auto: C / Editor

## Assembly-CSharp/KSP/UI/Screens/Editor/PartIcon.cs
- L=43 kind=class type=PartIcon bases=- anc=- ns=KSP.UI.Screens.Editor
- unity(61): Renderer=3, GameObject=3, GetComponent=1 | ksp: none | fan_in=4
- methods(1): DestroyIcon
- auto: B / Editor

## Assembly-CSharp/KSP/UI/Screens/Editor/PartListTooltipController.cs
- L=409 kind=MonoBehaviour type=PartListTooltipController bases=PinnableTooltipController anc=PinnableTooltipController/TooltipController/MonoBehaviour ns=KSP.UI.Screens.Editor
- unity(67): Transform=7, Material=6, UnityEngine.UI=5, Shader=4, SetActive=4, EventSystems=4 | ksp: GameEvents=7, HighLogic=4, Localizer=3, InputLockManager=3, AvailablePart=3 | fan_in=7
- methods(21): CreateTooltip, DestroyTooltip, GetTooltipHintText, OnButtonVariantToggle, OnEditorDefaultVariantChanged, OnPointerClick, OnPointerEnter, OnTooltipAboutToDespawn, OnTooltipAboutToSpawn, OnTooltipDespawned, OnTooltipPinned, OnTooltipSpawned, OnTooltipUnpinned, SetScreenSpaceMaskMaterials, SetupScreenSpaceMask, onPurchase, onPurchaseDismiss, onPurchaseProceed, onPurchaseUpgrade, onPurchaseUpgradeProceed, onVariantToggle
- auto: C / Editor

## Assembly-CSharp/KSP/UI/Screens/Editor/PartListTooltipMasterController.cs
- L=117 kind=MonoBehaviour type=PartListTooltipMasterController bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.Editor
- unity(76): GameObject=4, Camera=4, Texture=3, Lifecycle=2, LayerMask=2, Transform=1 | ksp: InputLockManager=4 | fan_in=3
- methods(6): Awake, CreateThumbnailCamera, HideTooltip, OnDestroy, setHoverLock, setSafeArea
- auto: C / Editor

## Assembly-CSharp/KSP/UI/Screens/Editor/PartListTooltipWidget.cs
- L=21 kind=MonoBehaviour type=PartListTooltipWidget bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.Editor
- unity(74): TMPro=2, UnityEngine.UI=1, MonoBehaviour=1 | ksp: none | fan_in=3
- methods(1): Setup
- auto: C / Editor

## Assembly-CSharp/KSP/UI/Screens/EditorActionController.cs
- L=34 kind=MonoBehaviour type=EditorActionController bases=EditorActionGroup_Base anc=EditorActionGroup_Base/UISelectableGridLayoutGroupItem/MonoBehaviour ns=KSP.UI.Screens
- unity(21): none | ksp: Localizer=1 | fan_in=1
- methods(1): Setup
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/EditorActionControllerHeader.cs
- L=62 kind=MonoBehaviour type=EditorActionControllerHeader bases=UISelectableGridLayoutGroupItem anc=UISelectableGridLayoutGroupItem/MonoBehaviour ns=KSP.UI.Screens
- unity(83): SetActive=6, GameObject=6, TMPro=2, UnityEngine.UI=1 | ksp: HighLogic=1 | fan_in=1
- methods(5): Deselect, EditButtonClicked, InputFieldDone, Select, Setup
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/EditorActionControllerOpenButton.cs
- L=26 kind=MonoBehaviour type=EditorActionControllerOpenButton bases=UISelectableGridLayoutGroupItem anc=UISelectableGridLayoutGroupItem/MonoBehaviour ns=KSP.UI.Screens
- unity(44): UnityEngine.UI=1 | ksp: none | fan_in=1
- methods(2): OpenButtonClicked, Setup
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/EditorActionGroup.cs
- L=37 kind=MonoBehaviour type=EditorActionGroup bases=EditorActionGroup_Base anc=EditorActionGroup_Base/UISelectableGridLayoutGroupItem/MonoBehaviour ns=KSP.UI.Screens
- unity(20): none | ksp: Localizer=1 | fan_in=1
- methods(1): Setup
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/EditorActionGroup_Base.cs
- L=23 kind=MonoBehaviour type=EditorActionGroup_Base bases=UISelectableGridLayoutGroupItem anc=UISelectableGridLayoutGroupItem/MonoBehaviour ns=KSP.UI.Screens
- unity(44): TMPro=1 | ksp: none | fan_in=3
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/EditorActionGroupType.cs
- L=9 kind=enum type=EditorActionGroupType bases=- anc=- ns=KSP.UI.Screens
- unity(0): none | ksp: none | fan_in=5
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/EditorActionOverrideGroup.cs
- L=179 kind=MonoBehaviour type=EditorActionOverrideGroup bases=UISelectableGridLayoutGroupItem,IPointerClickHandler,IEventSystemHandler anc=UISelectableGridLayoutGroupItem/MonoBehaviour ns=KSP.UI.Screens
- unity(76): GameObject=13, SetActive=12, EventSystems=3, TMPro=2, UnityEngine.UI=1, Lifecycle=1 | ksp: InputLockManager=3, Localizer=2 | fan_in=2
- methods(12): CloseGroup, Collapse, Deselect, EditButtonClicked, InputFieldDone, OnDestroy, OnPointerClick, OnSubmit, OpenGroup, Select, SetName, Setup
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/EditorActionOverrideToggle.cs
- L=53 kind=MonoBehaviour type=EditorActionOverrideToggle bases=UISelectableGridLayoutGroupItem anc=UISelectableGridLayoutGroupItem/MonoBehaviour ns=KSP.UI.Screens
- unity(42): TMPro=1, GetComponent=1 | ksp: Localizer=3 | fan_in=1
- methods(4): OnToggle, OnValueChanged, SetOverrideStateDelegate, Setup
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/EditorActionPartItem.cs
- L=379 kind=MonoBehaviour type=EditorActionPartItem bases=UISelectableGridLayoutGroupItem,IPointerEnterHandler,IEventSystemHandler anc=UISelectableGridLayoutGroupItem/MonoBehaviour ns=KSP.UI.Screens
- unity(67): GameObject=15, SetActive=13, EventSystems=7, UnityEngine.UI=4, TMPro=2, GetComponent=2 | ksp: Localizer=8, GameSettings=8, PartModule=3, Part=3 | fan_in=1
- methods(16): GetAxisIncremental, GetAxisInverted, ItemClicked, OnDestroy, OnPointerClick, OnPointerEnter, OnPointerExit, OnSpeedSliderValueChanged, OnSubmit, RefreshAxisField, SetAxisIncremental, SetAxisInverted, Setup, ToggleInvert, ToggleMode, UpdateSliderValue
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/EditorActionPartReset.cs
- L=52 kind=MonoBehaviour type=EditorActionPartReset bases=UISelectableGridLayoutGroupItem,IPointerEnterHandler,IEventSystemHandler anc=UISelectableGridLayoutGroupItem/MonoBehaviour ns=KSP.UI.Screens
- unity(80): EventSystems=7, TMPro=1 | ksp: Localizer=2 | fan_in=1
- methods(5): OnPointerClick, OnPointerEnter, OnPointerExit, OnSubmit, Setup
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/EditorActionPartSelector.cs
- L=215 kind=MonoBehaviour type=EditorActionPartSelector bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(44): GetComponent=5, GameObject=3, MonoBehaviour=1, Lifecycle=1, Instantiate/Destroy=1 | ksp: Part=21, HighLogic=2, KerbalEVA=1, GameSettings=1 | fan_in=4
- methods(6): CreatePartActions, Deselect, DestroyPartActions, HighLight, LateUpdate, Select
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/EditorActionTitleItem.cs
- L=6 kind=MonoBehaviour type=EditorActionTitleItem bases=UISelectableGridLayoutGroupItem anc=UISelectableGridLayoutGroupItem/MonoBehaviour ns=KSP.UI.Screens
- unity(15): none | ksp: none | fan_in=0
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/EditorInventoryOnlyIcon.cs
- L=81 kind=MonoBehaviour type=EditorInventoryOnlyIcon bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(93): UnityEngine.UI=8, Transform=7, GameObject=6, SetActive=3, GetComponent=2, Texture=1 | ksp: none | fan_in=3
- methods(5): Awake, ClearStackImages, SetIconTexture, SetStackAmount, ToggleBackgroundVisibility
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/EditorLaunchPadItem.cs
- L=68 kind=MonoBehaviour type=EditorLaunchPadItem bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(62): UnityEngine.UI=2, Lifecycle=2, TMPro=1, MonoBehaviour=1 | ksp: HighLogic=3, EditorLogic=1 | fan_in=1
- methods(5): Awake, Create, MouseInput_Launch, MouseInput_SelectDefault, OnDestroy
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/EditorPanels.cs
- L=140 kind=MonoBehaviour type=EditorPanels bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(35): Lifecycle=2, MonoBehaviour=1, EventSystems=1 | ksp: EditorLogic=2, GameEvents=1 | fan_in=3
- methods(8): Awake, IsMouseOver, OnDestroy, ShowActionGroups, ShowCargo, ShowCrewAssignment, ShowPartsList, updatePartsListMode
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/EditorPartList.cs
- L=498 kind=MonoBehaviour type=EditorPartList bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(74): GameObject=20, Transform=17, SetActive=17, Coroutine=12, UnityEngine.UI=7, Instantiate/Destroy=6 | ksp: AvailablePart=36, PartLoader=5, EditorLogic=3, GameEvents=2, HighLogic=1 | fan_in=16
- methods(25): AutoScrollBarFix, AutoScrollBarFix_routine, Awake, ClearAllItems, OnDestroy, OnEditorRestart, OnPartListUpdate, Refresh, RefreshCustomPartList, RefreshPartList, RefreshSearchList, RefreshSubassemblies, RefreshSubassemblyList, RefreshVariants, RefreshVariantsList, RememberScrollbarPosition, RevealPart, ScrollBarFix, SortingCallback, Start, TapBackground, TapIcon, UpdatePartIcon, UpdatePartIcons ...
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/EditorSaveFolderItem.cs
- L=83 kind=MonoBehaviour type=EditorSaveFolderItem bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(57): UnityEngine.UI=2, Lifecycle=2, TMPro=1, MonoBehaviour=1 | ksp: Localizer=1, EditorLogic=1 | fan_in=1
- methods(9): Awake, Create, IsPath, MouseInput_OnClick, MouseInput_SelectDefault, OnDestroy, SaveCraft, SetPath, SetPathDisplay
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/EditorSubassemblyItem.cs
- L=93 kind=MonoBehaviour type=EditorSubassemblyItem bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(69): TMPro=3, Coroutine=3, UnityEngine.UI=2, MonoBehaviour=1, Lifecycle=1 | ksp: Localizer=8, HighLogic=4, InputLockManager=3, EditorLogic=3, Part=1 | fan_in=1
- methods(8): Awake, Create, DeleteSubassembly, MouseInput_Delete, MouseInput_Spawn, OnDeleteConfirm, OnDismiss, OnDismissCoroutine
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/EditorVariantItem.cs
- L=95 kind=MonoBehaviour type=EditorVariantItem bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(75): UnityEngine.UI=4, TMPro=3, Shader=2, Lifecycle=2, MonoBehaviour=1 | ksp: EditorLogic=2, GameEvents=1, AvailablePart=1 | fan_in=1
- methods(6): Awake, Create, MouseInput_ApplyTheme, MouseInput_SetTheme, OnDestroy, SetColor
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/EngineersReport.cs
- L=695 kind=MonoBehaviour type=EngineersReport bases=UIApp anc=UIApp/MonoBehaviour ns=KSP.UI.Screens
- unity(59): Coroutine=12, Transform=10, GameObject=8, EventSystems=8, TMPro=6, GetComponent=6 | ksp: GameEvents=16, Localizer=14, EditorLogic=7, Part=5, ShipConstruct=3 | fan_in=3
- methods(41): AddTest, CacheLocalStrings, CreateCraftStats, CreateCraftStatsbody, CreateSeveritySelector, CreateStockDesignConcern, Critical_OnFalse, Critical_OnTrue, DisplayApp, GetAppScenes, GetAppScreenPos, GetFormattedTestDescription, GetFormattedTestTitle, HideApp, MouseInput_PointerEnter, MouseInput_PointerExit, Notice_OnFalse, Notice_OnTrue, OnAppAboutToStart, OnAppDestroy, OnAppInitialized, OnCraftModified, OnCraftModified_delayed, OnCrewModified ...
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Flight/ActionGroupToggleButton.cs
- L=153 kind=MonoBehaviour type=ActionGroupToggleButton bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.Flight
- unity(46): Light=2, Lifecycle=2, UnityEngine.UI=1, MonoBehaviour=1, GetComponent=1 | ksp: FlightGlobals=12, InputLockManager=7 | fan_in=0
- methods(3): Awake, LateUpdate, SetToggle
- auto: B / FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/AltitudeTumbler.cs
- L=188 kind=MonoBehaviour type=AltitudeTumbler bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.Flight
- unity(56): Lifecycle=4, Transform=3, Time.=3, Analytics=2, UnityEngine.UI=1, MonoBehaviour=1 | ksp: GameEvents=10, FlightGlobals=7, HighLogic=2, Vessel=1, MapView=1 | fan_in=1
- methods(11): Awake, ChangeAltimeterMode, LateUpdate, OnDestroy, OnMapEntered, OnSceneSwitch, OnVesselChange, Reset, SetModeTumbler, SetTumblerPosition, Start
- auto: B / FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/Dialogs/CrewHatchDialog.cs
- L=181 kind=MonoBehaviour type=CrewHatchDialog bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.Flight.Dialogs
- unity(69): Lifecycle=4, Transform=3, TMPro=3, Instantiate/Destroy=3, GameObject=3, GetComponent=2 | ksp: ProtoCrewMember=8, GameEvents=8, Part=4, Localizer=3, Vessel=2 | fan_in=1
- methods(15): Awake, ClearList, CreateList, CreatePanelContent, LateUpdate, OnBtnEVA, OnBtnTransfer, OnCameraChange, OnDestroy, OnFlightUIModeChanged, Spawn, Start, Terminate, onVesselChange, onVesselWasModified
- auto: C / FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/Dialogs/CrewHatchDialogWidget.cs
- L=58 kind=MonoBehaviour type=CrewHatchDialogWidget bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.Flight.Dialogs
- unity(78): UnityEngine.UI=3, GameObject=2, TMPro=1, SetActive=1, MonoBehaviour=1, Instantiate/Destroy=1 | ksp: ProtoCrewMember=4 | fan_in=1
- methods(3): BtnInit, Init, Terminate
- auto: C / FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/Dialogs/ExperimentsResultDialog.cs
- L=430 kind=MonoBehaviour type=ExperimentsResultDialog bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.Flight.Dialogs
- unity(77): UnityEngine.UI=14, GameObject=11, TMPro=10, SetActive=10, Lifecycle=3, KeyCode=3 | ksp: GameEvents=15, Localizer=10, Vessel=2, Part=1, HighLogic=1 | fan_in=3
- methods(25): Awake, Dismiss, DisplayResult, NextPage, OnDestroy, OnGameSceneLoadRequested, OnPagesModified, OnPause, OnUnpause, PrevPage, SetPage, Update, addPage, dismissCurrentPage, onBtnDiscard, onBtnKeep, onBtnLab, onBtnPageNext, onBtnPagePrev, onBtnReset, onBtnTransmit, onPartDie, onVesselFocusChange, onVesselModified ...
- auto: C / FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/Dialogs/FlagPlaqueDialog.cs
- L=56 kind=MonoBehaviour type=FlagPlaqueDialog bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.Flight.Dialogs
- unity(80): TMPro=2, Instantiate/Destroy=2, UnityEngine.UI=1, Transform=1, MonoBehaviour=1, Lifecycle=1 | ksp: Localizer=1 | fan_in=1
- methods(4): Spawn, Start, Terminate, onBtnClose
- auto: C / FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/GeeGauge.cs
- L=22 kind=MonoBehaviour type=GeeGauge bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.Flight
- unity(59): MonoBehaviour=1, Lifecycle=1, GetComponent=1 | ksp: FlightGlobals=2 | fan_in=0
- methods(2): LateUpdate, Reset
- auto: B / FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/IVAEVACollapseGroups.cs
- L=133 kind=MonoBehaviour type=IVAEVACollapseGroups bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.Flight
- unity(34): Lifecycle=2, MonoBehaviour=1 | ksp: GameEvents=7, Vessel=2, FlightGlobals=1 | fan_in=0
- methods(7): Awake, FinishedAllTransitionsCallback, OnCameraChange, OnDestroy, UpdatePanels, onVesselChange, onVesselWasModified
- auto: B / FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/KerbalPortrait.cs
- L=451 kind=MonoBehaviour type=KerbalPortrait bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.Flight
- unity(66): SetActive=13, Coroutine=10, UnityEngine.UI=7, Material=6, GameObject=5, Lifecycle=3 | ksp: HighLogic=4, Localizer=3, ProtoCrewMember=2, KerbalEVA=2, GameSettings=2 | fan_in=4
- methods(15): CacheLocalStrings, CanEVA, CanIVA, ClickEVA, ClickIVA, OnBecameInvisible, OnBecameVisible, OnCrewDie, OnDisable, OnEnable, OnOverlayUpdate, OverlayUpdate, RectContainment_OnContainmentChanged, Setup, Update
- auto: C / FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/LEDLin.cs
- L=22 kind=MonoBehaviour type=LEDLin bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.Flight
- unity(59): MonoBehaviour=1, Lifecycle=1, GetComponent=1 | ksp: FlightGlobals=1 | fan_in=0
- methods(2): LateUpdate, Reset
- auto: B / FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/LEDRot.cs
- L=22 kind=MonoBehaviour type=LEDRot bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.Flight
- unity(59): MonoBehaviour=1, Lifecycle=1, GetComponent=1 | ksp: FlightGlobals=1 | fan_in=0
- methods(2): LateUpdate, Reset
- auto: B / FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/LightDisplay.cs
- L=24 kind=MonoBehaviour type=LightDisplay bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.Flight
- unity(63): MonoBehaviour=1, Light=1, Lifecycle=1 | ksp: FlightGlobals=4 | fan_in=0
- methods(1): LateUpdate
- auto: B / FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/LinearAtmosphereGauge.cs
- L=51 kind=MonoBehaviour type=LinearAtmosphereGauge bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.Flight
- unity(47): MonoBehaviour=1, Lifecycle=1, GetComponent=1 | ksp: FlightGlobals=4, CelestialBody=1 | fan_in=0
- methods(2): LateUpdate, Reset
- auto: B / FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/LinearControlGauges.cs
- L=101 kind=MonoBehaviour type=LinearControlGauges bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.Flight
- unity(55): Lifecycle=3, Input.=2, UnityEngine.UI=1, MonoBehaviour=1 | ksp: FlightGlobals=10, GameEvents=2 | fan_in=0
- methods(4): LateUpdate, OnDestroy, Start, onPrecisionModeToggle
- auto: B / FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/MapViewPanelTransition.cs
- L=42 kind=MonoBehaviour type=MapViewPanelTransition bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.Flight
- unity(64): Lifecycle=2, GetComponent=2, MonoBehaviour=1 | ksp: GameEvents=4 | fan_in=0
- methods(5): Awake, OnDestroy, Reset, SetModeMapOff, SetModeMapOn
- auto: B / FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/METDisplay.cs
- L=103 kind=MonoBehaviour type=METDisplay bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.Flight
- unity(63): Time.=4, TMPro=3, Lifecycle=3, MonoBehaviour=1, GetComponent=1 | ksp: GameEvents=4, Planetarium=1, Localizer=1, FlightGlobals=1 | fan_in=1
- methods(9): CacheLocalStrings, LateUpdate, OnDestroy, Reset, SetTimeMode, Start, ToggleTimeMode, onGamePause, onGameUnPause
- auto: C / FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/NavBall.cs
- L=248 kind=MonoBehaviour type=NavBall bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.Flight
- unity(90): SetActive=24, GameObject=24, Transform=17, Texture=4, UnityEngine.UI=2, Lifecycle=2 | ksp: FlightGlobals=29, FlightCamera=4, Vessel=2, Orbit=2, GameDatabase=2 | fan_in=3
- methods(6): ClearWaypoint, DrawOrbitalCues, SetVectorAlphaTint, SetWaypoint, Start, Update
- auto: C / FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/NavBallBurnVector.cs
- L=645 kind=MonoBehaviour type=NavBallBurnVector bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.Flight
- unity(77): GameObject=39, SetActive=38, Transform=22, UnityEngine.UI=6, TMPro=6, Lifecycle=4 | ksp: FlightGlobals=23, GameSettings=22, Localizer=11, Planetarium=7, GameEvents=4 | fan_in=1
- methods(20): AutoStepNodes, Awake, CacheLocalStrings, CalculateBurnTime, ClearStageMarkers, LateUpdate, OnDestroy, OnDismissButton, OnVesselSwitch, OnWarpToButton, RemoveRemainingStageMarkers, SetAdvancedMode, Start, UpdateBurnUIText, UpdateDVStageMarkers, UpdateNavballBurnVectors, btnPercentMinusClick, btnPercentPlusClick, navBallIsExpanded, onGameSettingsApplied
- auto: C / FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/NavBallToggle.cs
- L=136 kind=MonoBehaviour type=NavBallToggle bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.Flight
- unity(47): Lifecycle=3, UnityEngine.UI=1, MonoBehaviour=1, IMGUI=1 | ksp: InputLockManager=7, MapView=3, GameEvents=3, GameSettings=2 | fan_in=3
- methods(9): Awake, DisableManeuverMode, EnableManeuverMode, OnDestroy, OnMapEntered, OnMapExited, OnNavBallToggle, TogglePanel, Update
- auto: B / FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/RCSDisplay.cs
- L=44 kind=MonoBehaviour type=RCSDisplay bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.Flight
- unity(63): Lifecycle=2, GetComponent=2, MonoBehaviour=1 | ksp: FlightGlobals=4, KerbalEVA=1 | fan_in=0
- methods(4): LateUpdate, Reset, SetNewState, Start
- auto: B / FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/SASDisplay.cs
- L=63 kind=MonoBehaviour type=SASDisplay bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.Flight
- unity(54): Lifecycle=2, GetComponent=2, MonoBehaviour=1 | ksp: FlightGlobals=2, Vessel=1, GameSettings=1 | fan_in=0
- methods(4): LateUpdate, Reset, SetNewState, Start
- auto: B / FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/SpeedDisplay.cs
- L=81 kind=MonoBehaviour type=SpeedDisplay bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.Flight
- unity(63): Lifecycle=4, TMPro=2, UnityEngine.UI=1, MonoBehaviour=1 | ksp: FlightGlobals=8, GameEvents=2, Orbit=1, Localizer=1 | fan_in=0
- methods(6): Awake, LateUpdate, OnChangeMode, OnDestroy, OnSetSpeedMode, Start
- auto: C / FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/StageTumbler.cs
- L=32 kind=MonoBehaviour type=StageTumbler bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.Flight
- unity(79): Transform=3, Lifecycle=2, MonoBehaviour=1, GetComponent=1 | ksp: GameSettings=3, FlightGlobals=1 | fan_in=0
- methods(4): Awake, LateUpdate, Reset, SetTumblerPosition
- auto: B / FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/StagingLED.cs
- L=50 kind=MonoBehaviour type=StagingLED bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.Flight
- unity(59): Shader=2, MonoBehaviour=1, Lifecycle=1 | ksp: InputLockManager=1, FlightGlobals=1 | fan_in=0
- methods(1): LateUpdate
- auto: B / FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/TelemetryUpdate.cs
- L=356 kind=MonoBehaviour type=TelemetryUpdate bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.Flight
- unity(76): Texture=25, SetActive=9, GameObject=9, UnityEngine.UI=6, Lifecycle=4, Time.=1 | ksp: Localizer=18, Vessel=2, MapView=1, FlightGlobals=1 | fan_in=1
- methods(10): Awake, CacheLocalStrings, ClearGui, HideModeButton, OnDestroy, SetEVAUI, SetIcon, ShowModeButton, Start, Update
- auto: C / FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/TemperatureGauge.cs
- L=179 kind=MonoBehaviour type=TemperatureGauge bases=MonoBehaviour,IComparable<TemperatureGauge> anc=MonoBehaviour ns=KSP.UI.Screens.Flight
- unity(63): UnityEngine.UI=4, GameObject=4, SetActive=3, Lifecycle=2, Transform=1, MonoBehaviour=1 | ksp: Part=5, GameSettings=2, FlightCamera=2 | fan_in=1
- methods(5): Awake, CompareTo, GaugeUpdate, OnDestroy, Setup
- auto: C / FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/TemperatureGaugeSystem.cs
- L=248 kind=MonoBehaviour type=TemperatureGaugeSystem bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.Flight
- unity(49): Instantiate/Destroy=5, Transform=4, Lifecycle=3, GameObject=3, MonoBehaviour=1 | ksp: FlightGlobals=8, Part=6, PhysicsGlobals=4, Vessel=1, HighLogic=1 | fan_in=1
- methods(8): Awake, CreateGauges, DestroyGauges, OnDestroy, RebuildGaugeList, TestRebuild, TestRecreate, Update
- auto: B / FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/ThrottleGauge.cs
- L=30 kind=MonoBehaviour type=ThrottleGauge bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.Flight
- unity(64): Time.=1, MonoBehaviour=1, Lifecycle=1, GetComponent=1 | ksp: FlightGlobals=4, KerbalEVA=1 | fan_in=0
- methods(2): LateUpdate, Reset
- auto: B / FlightUI

## Assembly-CSharp/KSP/UI/Screens/Flight/VerticalSpeedGauge.cs
- L=46 kind=MonoBehaviour type=VerticalSpeedGauge bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.Flight
- unity(64): Lifecycle=2, Shader=1, MonoBehaviour=1, GetComponent=1 | ksp: FlightGlobals=4 | fan_in=0
- methods(3): LateUpdate, Reset, Start
- auto: B / FlightUI

## Assembly-CSharp/KSP/UI/Screens/HelixGauge.cs
- L=174 kind=MonoBehaviour type=HelixGauge bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(76): UnityEngine.UI=15, Transform=5, Lifecycle=3, MonoBehaviour=1 | ksp: GameSettings=3 | fan_in=0
- methods(10): Awake, GetAngle, GetAngle180, GetAngle360, GetAngle90, GetReadoutWorldPos, LateUpdate, Start, UpdateGauge, UpdateReadoutField
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/KbApp.cs
- L=146 kind=MonoBehaviour type=KbApp bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(57): Lifecycle=3, GameObject=3, SetActive=2, Instantiate/Destroy=2, Texture=1, MonoBehaviour=1 | ksp: none | fan_in=7
- methods(16): ActivateApp, Awake, DisablePanel, DisplayApp, EnablePanel, Hide, HideApp, Hover, HoverOut, OnDestroy, Restore, Setup, Show, Start, displayApp, hideApp
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/KbApp_Null.cs
- L=17 kind=MonoBehaviour type=KbApp_Null bases=KbApp anc=KbApp/MonoBehaviour ns=KSP.UI.Screens
- unity(15): none | ksp: none | fan_in=0
- methods(3): ActivateApp, DisplayApp, HideApp
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/KbApp_PlanetInfo.cs
- L=35 kind=MonoBehaviour type=KbApp_PlanetInfo bases=KbApp anc=KbApp/MonoBehaviour ns=KSP.UI.Screens
- unity(87): GameObject=5, TMPro=2, GetComponent=2, Instantiate/Destroy=1 | ksp: Localizer=1, CelestialBody=1 | fan_in=0
- methods(3): ActivateApp, DisplayApp, HideApp
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/KbApp_PlanetParameters.cs
- L=89 kind=MonoBehaviour type=KbApp_PlanetParameters bases=KbApp anc=KbApp/MonoBehaviour ns=KSP.UI.Screens
- unity(37): Transform=2, Instantiate/Destroy=1 | ksp: Localizer=26, PhysicsGlobals=1, CelestialBody=1 | fan_in=0
- methods(6): ActivateApp, CreateAtmosphericCharacteristics, CreatePhysicalCharacteristics, DisplayApp, HideApp, boolDelegate_KbApp_PlanetParameters
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/KbApp_PlanetResources.cs
- L=524 kind=MonoBehaviour type=KbApp_PlanetResources bases=KbApp anc=KbApp/MonoBehaviour ns=KSP.UI.Screens
- unity(64): GameObject=13, EventSystems=11, Instantiate/Destroy=8, Transform=6, GetComponent=6, SetActive=4 | ksp: Localizer=3, PartResourceDefinition=1, CelestialBody=1 | fan_in=0
- methods(26): ActivateApp, AddResourceDataItem, CreateResourceList, DisplayApp, DisposeResourceList, HideApp, InitHeaderAndFooter, MouseInputResCtrlMinus, MouseInputResCtrlPlus, OnResourcesCtrlColorChange, OnResourcesCtrlStyleChange, OnSort, OnToggleAtmos, OnToggleOcean, OnToggleSurface, ResourceListAndPiechart_select, SetKBDisplay, SetResourceListAppearance, SetSliceOver, SetSliceOverOff, onResourcesListItem_Click, onResourcesListItem_Enter, onResourcesListItem_Exit, onSliceExit ...
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/KbApp_UnownedInfo.cs
- L=139 kind=MonoBehaviour type=KbApp_UnownedInfo bases=KbAppUnowned anc=KbAppUnowned/KbApp/MonoBehaviour ns=KSP.UI.Screens
- unity(82): TMPro=10, GameObject=7, Instantiate/Destroy=3, GetComponent=3, Transform=2, Time.=2 | ksp: Localizer=6, Vessel=2, Planetarium=2, MapView=1 | fan_in=1
- methods(6): ActivateApp, CacheLocalStrings, CreateUnownedTrackingInfoList, FixedUpdate, UpdateNowQuestionmark, UpdateTrackingInfoList
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/KbApp_VesselCrew.cs
- L=144 kind=MonoBehaviour type=KbApp_VesselCrew bases=KbAppVessel anc=KbAppVessel/KbApp/MonoBehaviour ns=KSP.UI.Screens
- unity(48): Instantiate/Destroy=3, Transform=2, GameObject=2, UnityEngine.UI=1 | ksp: ProtoCrewMember=12, AvailablePart=5, Vessel=2, PartLoader=1, Localizer=1 | fan_in=0
- methods(6): ActivateApp, CompareKVP, CompareSeatIdx, CreateVesselCrewList, SetupKerbalItem, boolDelegate_KbApp_VesselCrew
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/KbApp_VesselInfo.cs
- L=331 kind=MonoBehaviour type=KbApp_VesselInfo bases=KbAppVessel anc=KbAppVessel/KbApp/MonoBehaviour ns=KSP.UI.Screens
- unity(62): TMPro=16, Instantiate/Destroy=3, UnityEngine.UI=2, Transform=2, Time.=2, GameObject=2 | ksp: Localizer=31, Vessel=9, HighLogic=2, ProtoPartSnapshot=1, MapView=1 | fan_in=1
- methods(9): ActivateApp, CacheLocalStrings, CreateVesselInfoBox, CreateVesselInfoList, FixedUpdate, UpdateCommNet, UpdateNowQuestionmark, UpdateVesselInfoList, boolDelegate_KbApp_VesselInfo
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/KbAppFrame.cs
- L=78 kind=MonoBehaviour type=KbAppFrame bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(91): UnityEngine.UI=9, Transform=5, TMPro=3, Lifecycle=2, GetComponent=2, SetActive=1 | ksp: GameSettings=2, Localizer=1 | fan_in=1
- methods(4): Awake, OnDestroy, Reposition, Setup
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/KbAppUnowned.cs
- L=72 kind=MonoBehaviour type=KbAppUnowned bases=KbApp anc=KbApp/MonoBehaviour ns=KSP.UI.Screens
- unity(43): Lifecycle=2, EventSystems=1 | ksp: GameEvents=9, Vessel=7, InputLockManager=2 | fan_in=1
- methods(9): Awake, DisplayApp, HideApp, OnDestroy, OnInputLocksChange, OnKnowledgeChange, OnPanelHeaderTap, Setup, onVesselRename
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/KbAppVessel.cs
- L=62 kind=MonoBehaviour type=KbAppVessel bases=KbApp anc=KbApp/MonoBehaviour ns=KSP.UI.Screens
- unity(46): Lifecycle=2, EventSystems=1 | ksp: GameEvents=6, Vessel=2, InputLockManager=2 | fan_in=2
- methods(8): Awake, DisplayApp, HideApp, OnDestroy, OnInputLocksChange, OnPanelHeaderTap, OnVesselRename, Setup
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/KbItem.cs
- L=9 kind=MonoBehaviour type=KbItem bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(36): MonoBehaviour=1 | ksp: none | fan_in=7
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/KbItem_flagImage.cs
- L=9 kind=MonoBehaviour type=KbItem_flagImage bases=KbItem anc=KbItem/MonoBehaviour ns=KSP.UI.Screens
- unity(44): UnityEngine.UI=1 | ksp: none | fan_in=1
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/KbItem_kerbalInfo.cs
- L=18 kind=MonoBehaviour type=KbItem_kerbalInfo bases=KbItem anc=KbItem/MonoBehaviour ns=KSP.UI.Screens
- unity(77): TMPro=3, UnityEngine.UI=1 | ksp: none | fan_in=1
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/KbItem_resourceFooter.cs
- L=23 kind=MonoBehaviour type=KbItem_resourceFooter bases=KbItem anc=KbItem/MonoBehaviour ns=KSP.UI.Screens
- unity(82): UnityEngine.UI=3, TMPro=2 | ksp: none | fan_in=1
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/KbItem_resourceHeader.cs
- L=15 kind=MonoBehaviour type=KbItem_resourceHeader bases=KbItem anc=KbItem/MonoBehaviour ns=KSP.UI.Screens
- unity(70): UnityEngine.UI=3 | ksp: none | fan_in=1
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/KbItem_resourceItem.cs
- L=15 kind=MonoBehaviour type=KbItem_resourceItem bases=KbItem anc=KbItem/MonoBehaviour ns=KSP.UI.Screens
- unity(59): TMPro=2 | ksp: none | fan_in=1
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/KbItem_unownedInfo.cs
- L=24 kind=MonoBehaviour type=KbItem_unownedInfo bases=KbItem anc=KbItem/MonoBehaviour ns=KSP.UI.Screens
- unity(89): TMPro=6, UnityEngine.UI=1 | ksp: none | fan_in=1
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/KbItem_vesselInfo.cs
- L=19 kind=MonoBehaviour type=KbItem_vesselInfo bases=KbItem anc=KbItem/MonoBehaviour ns=KSP.UI.Screens
- unity(82): TMPro=5 | ksp: none | fan_in=1
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/KerbalListItem.cs
- L=66 kind=MonoBehaviour type=KerbalListItem bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(94): Instantiate/Destroy=4, Transform=3, Texture=3, Light=3, UnityEngine.UI=2, GetComponent=2 | ksp: none | fan_in=1
- methods(2): Initialize, OnDestroy
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/KnowledgeBase.cs
- L=253 kind=MonoBehaviour type=KnowledgeBase bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(62): Transform=7, GameObject=7, SetActive=6, Instantiate/Destroy=3, Lifecycle=2, Time.=1 | ksp: MapView=9, GameEvents=7, Vessel=5, ProtoCrewMember=5, AvailablePart=5 | fan_in=4
- methods(10): ActivateApps, Awake, Cleanup, GetUnloadedVesselMass, GetVesselCrewByAvailablePart, Hide, OnAppLauncherReady, OnDestroy, OnMapFocusChange, Show
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/KSCFacilityContextMenu.cs
- L=408 kind=MonoBehaviour type=KSCFacilityContextMenu bases=AnchoredDialog anc=AnchoredDialog/MonoBehaviour ns=KSP.UI.Screens
- unity(71): GameObject=13, SetActive=12, TMPro=8, UnityEngine.UI=6, Transform=4, GetComponent=2 | ksp: Localizer=14, GameEvents=12, Funding=5, HighLogic=2, Game=2 | fan_in=2
- methods(19): Create, CreateWindowContent, Dismiss, GetWindowTitle, OnClickOut, OnDemolishButtonInput, OnDestroyThis, OnDowngradeButtonInput, OnEnterBtnInput, OnFacilityValuesModified, OnGameSceneLoadRequested, OnKSCStructureEvent, OnRepairButtonInput, OnUpgradeButtonHoverIn, OnUpgradeButtonHoverOut, OnUpgradeButtonInput, PreviewNextLevel, StartThis, getFundsTextColorTag
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/KSCVesselMarker.cs
- L=331 kind=MonoBehaviour type=KSCVesselMarker bases=AnchoredDialog anc=AnchoredDialog/MonoBehaviour ns=KSP.UI.Screens
- unity(71): SetActive=9, GameObject=9, UnityEngine.UI=6, TMPro=6, GetComponent=4, Coroutine=3 | ksp: Vessel=5, Localizer=5, InputLockManager=4, GameEvents=3, ProtoCrewMember=2 | fan_in=1
- methods(17): ClearCtrls, Collapse, Create, CreateWindowContent, Dismiss, Expand, GetWindowTitle, OnClickOut, OnDestroyThis, OnFlyButtonInput, OnLateUpdate, OnMarkerButtonInput, OnPanelSetupComplete, OnRecoverButtonInput, StartThis, makeShipname, onInputLocksModified
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/KSCVesselMarkers.cs
- L=143 kind=MonoBehaviour type=KSCVesselMarkers bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(46): Coroutine=4, Lifecycle=2, MonoBehaviour=1, Instantiate/Destroy=1 | ksp: GameEvents=21, Vessel=5, HighLogic=5, FlightGlobals=3, Planetarium=1 | fan_in=5
- methods(14): Awake, ClearVesselMarkers, FlyVessel, OnDestroy, OnMarkerDismiss, RecoverVessel, RefreshMarkers, SpawnVesselMarkers, onGUIACScreenDespawn, onGUIScreenDespawn, onGUIScreenSpawn, onGUIVesselScreenSpawn, onSceneLoadRequested, onVesselDestroy
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/KSPediaApp.cs
- L=41 kind=MonoBehaviour type=KSPediaApp bases=UIApp anc=UIApp/MonoBehaviour ns=KSP.UI.Screens
- unity(36): Transform=1 | ksp: none | fan_in=0
- methods(7): DisplayApp, GetAppScenes, GetAppScreenPos, HideApp, OnAppAboutToStart, OnAppDestroy, OnAppInitialized
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/LED.cs
- L=122 kind=MonoBehaviour type=LED bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(60): Coroutine=3, UnityEngine.UI=2, Time.=1, Texture=1, Shader=1, MonoBehaviour=1 | ksp: none | fan_in=0
- methods(7): Awake, Blink, BlinkCoroutine, Reset, SetColor, SetOn, setOff
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/LinearGauge.cs
- L=95 kind=MonoBehaviour type=LinearGauge bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(61): UnityEngine.UI=2, Lifecycle=2, Time.=1, MonoBehaviour=1, GetComponent=1 | ksp: none | fan_in=0
- methods(4): Awake, LateUpdate, Reset, SetValue
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Mapview/MapContextMenu.cs
- L=151 kind=class type=MapContextMenu bases=- anc=- ns=KSP.UI.Screens.Mapview
- unity(45): Coroutine=6, UnityEngine.UI=3, Input.=1, GetComponent=1, EventSystems=1 | ksp: MapView=4, Localizer=2, InputLockManager=2, GameSettings=1 | fan_in=2
- methods(7): Create, DialogUpdate, Dismiss, SetupTransform, UpdatePosition, dialogUpdateCoroutine, onDialogDismiss
- auto: C / MapView

## Assembly-CSharp/KSP/UI/Screens/Mapview/MapContextMenuOptions/AddManeuver.cs
- L=30 kind=class type=AddManeuver bases=MapContextMenuOption anc=MapContextMenuOption/DialogGUIButton/DialogGUIBase ns=KSP.UI.Screens.Mapview.MapContextMenuOptions
- unity(0): none | ksp: Localizer=1, InputLockManager=1 | fan_in=1
- methods(1): OnSelect
- auto: A / MapView

## Assembly-CSharp/KSP/UI/Screens/Mapview/MapContextMenuOptions/AutoWarpToUT.cs
- L=62 kind=class type=AutoWarpToUT bases=MapContextMenuOption anc=MapContextMenuOption/DialogGUIButton/DialogGUIBase ns=KSP.UI.Screens.Mapview.MapContextMenuOptions
- unity(0): none | ksp: Localizer=3, GameSettings=2, TimeWarp=1, Planetarium=1 | fan_in=1
- methods(3): CheckAvailable, OnCheckEnabled, OnSelect
- auto: A / MapView

## Assembly-CSharp/KSP/UI/Screens/Mapview/MapContextMenuOptions/DeleteCustomWaypoint.cs
- L=48 kind=class type=DeleteCustomWaypoint bases=MapContextMenuOption anc=MapContextMenuOption/DialogGUIButton/DialogGUIBase ns=KSP.UI.Screens.Mapview.MapContextMenuOptions
- unity(0): none | ksp: Localizer=2, HighLogic=1 | fan_in=1
- methods(3): CheckAvailable, OnCheckEnabled, OnSelect
- auto: A / MapView

## Assembly-CSharp/KSP/UI/Screens/Mapview/MapContextMenuOptions/FocusObject.cs
- L=108 kind=class type=FocusObject bases=MapContextMenuOption anc=MapContextMenuOption/DialogGUIButton/DialogGUIBase ns=KSP.UI.Screens.Mapview.MapContextMenuOptions
- unity(0): none | ksp: CelestialBody=5, Localizer=4, MapView=2, InputLockManager=2, Vessel=1 | fan_in=1
- methods(7): CheckAvailable, GetMode, OnCheckEnabled, OnSelect, TryViewInTrackingStation, onTrackingStationDismiss, onTrackingStationProceed
- auto: A / MapView

## Assembly-CSharp/KSP/UI/Screens/Mapview/MapContextMenuOptions/MapContextMenuOption.cs
- L=41 kind=class type=MapContextMenuOption bases=DialogGUIButton anc=DialogGUIButton/DialogGUIBase ns=KSP.UI.Screens.Mapview.MapContextMenuOptions
- unity(0): none | ksp: none | fan_in=8
- methods(4): CheckAvailable, CheckEnabled, OnCheckEnabled, OnSelect
- auto: A / MapView

## Assembly-CSharp/KSP/UI/Screens/Mapview/MapContextMenuOptions/SetAsTarget.cs
- L=60 kind=class type=SetAsTarget bases=MapContextMenuOption anc=MapContextMenuOption/DialogGUIButton/DialogGUIBase ns=KSP.UI.Screens.Mapview.MapContextMenuOptions
- unity(0): none | ksp: Localizer=2, FlightGlobals=2 | fan_in=1
- methods(3): CheckAvailable, OnCheckEnabled, OnSelect
- auto: A / MapView

## Assembly-CSharp/KSP/UI/Screens/Mapview/MapContextMenuOptions/WaypointNavigation.cs
- L=81 kind=class type=WaypointNavigation bases=MapContextMenuOption anc=MapContextMenuOption/DialogGUIButton/DialogGUIBase ns=KSP.UI.Screens.Mapview.MapContextMenuOptions
- unity(0): none | ksp: Localizer=6, HighLogic=1 | fan_in=1
- methods(3): CheckAvailable, OnCheckEnabled, OnSelect
- auto: A / MapView

## Assembly-CSharp/KSP/UI/Screens/Mapview/MapViewCanvasUtil.cs
- L=137 kind=MonoBehaviour type=MapViewCanvasUtil bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.Mapview
- unity(84): Transform=10, UnityEngine.UI=9, Camera=4, GetComponent=3, LayerMask=2, GameObject=2 | ksp: Vector3d=2 | fan_in=6
- methods(5): GetNodeCanvasContainer, OnUIScaleChange, ResetNodeCanvasContainer, ResetNodeCanvasContainerScale, ScaledToUISpacePos
- auto: C / MapView

## Assembly-CSharp/KSP/UI/Screens/MCListItem.cs
- L=45 kind=MonoBehaviour type=MCListItem bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(62): UnityEngine.UI=1, TMPro=1, MonoBehaviour=1, Lifecycle=1 | ksp: Contract=4 | fan_in=1
- methods(2): Awake, Setup
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/MEPartCategoryButton.cs
- L=101 kind=MonoBehaviour type=MEPartCategoryButton bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler anc=MonoBehaviour ns=KSP.UI.Screens
- unity(84): UnityEngine.UI=5, EventSystems=4, Texture=2, Lifecycle=2, Input.=2, GetComponent=2 | ksp: none | fan_in=2
- methods(10): Awake, Lock, OnPointerEnter, OnPointerExit, SetCount, SetState, ShowCount, Unlock, Update, UpdateState
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/MessageSystemButton.cs
- L=158 kind=MonoBehaviour type=MessageSystemButton bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(76): Texture=8, Shader=4, EventSystems=4, UnityEngine.UI=2, SetActive=1, MonoBehaviour=1 | ksp: none | fan_in=10
- methods(9): AddInputDelegate, InstantiateFromPrefab, MouseInput, OnClick, OnRightClick, SetAsRead, SetButtonColor, SetMessage, SetTexture
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/MessageSystemPopup.cs
- L=139 kind=MonoBehaviour type=MessageSystemPopup bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(69): UnityEngine.UI=4, GameObject=3, TMPro=2, SetActive=2, Lifecycle=2, Instantiate/Destroy=2 | ksp: GameEvents=2 | fan_in=1
- methods(13): Hide, InstantiateFromPrefab, MouseinputClose, MouseinputDiscard, OnDestroy, OnDiscard, OnSceneLoadRequest, OnShowNext, OnShowPrevious, Show, ShowNext, ShowPrevious, Start
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/MissionControl.cs
- L=600 kind=MonoBehaviour type=MissionControl bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(78): GameObject=41, SetActive=38, UnityEngine.UI=17, TMPro=5, Time.=3, Lifecycle=3 | ksp: Contract=33, Localizer=16, Planetarium=2, GameSettings=2, GameEvents=2 | fan_in=6
- methods(31): AddItem, Awake, ClearInfoPanel, DestroyInstructor, OnClickAccept, OnClickActive, OnClickAgentInfo, OnClickArchive, OnClickArchiveAll, OnClickArchiveCancelled, OnClickArchiveCompleted, OnClickArchiveFailed, OnClickAvailable, OnClickCancel, OnClickDecline, OnDeselectContract, OnDestroy, OnSelectContract, RebuildContractList, RefreshContracts, RefreshUIControls, SetArchiveMode, SetDisplayModeActive, SetDisplayModeArchive ...
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/MissionRecoveryDialog.cs
- L=509 kind=MonoBehaviour type=MissionRecoveryDialog bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(74): GameObject=17, SetActive=15, UnityEngine.UI=10, Texture=6, Transform=4, Lifecycle=4 | ksp: Localizer=14, GameEvents=4, ProtoVessel=2, InputLockManager=2, HighLogic=2 | fan_in=20
- methods(22): AddCrewWidget, AddDataWidget, AddPartWidget, AddResourceWidget, Awake, CreateFullDialog, CreateScienceDialog, GetProceedButtonCaption, MissionIsUninteresting, OnCrewTabPress, OnDestroy, OnGameSceneLoadRequested, OnPartsTabPress, OnProceedButtonClick, OnSciTabPress, SetMode, Start, Update, dismissDialog, updateCrewWindowContent, updatePartsWindowContent, updateScienceWindowContent
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/OnOpen.cs
- L=4 kind=delegate type=OnOpen bases=- anc=- ns=KSP.UI.Screens
- unity(0): none | ksp: none | fan_in=1
- methods(1): OnOpen
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/OnSelectItem.cs
- L=4 kind=delegate type=OnSelectItem bases=- anc=- ns=KSP.UI.Screens
- unity(0): none | ksp: none | fan_in=2
- methods(1): OnSelectItem
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/PartCategorizerButton.cs
- L=257 kind=MonoBehaviour type=PartCategorizerButton bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(84): SetActive=18, GameObject=18, UnityEngine.UI=15, EventSystems=4, Transform=2, GetComponent=2 | ksp: Localizer=1 | fan_in=4
- methods(14): DisableDividers, EnableDividerOverlay, EnableDividerSpaceUnder, ForceAspect, InitializeBtn, InitializeToggleBtn, MouseInput_click, MouseInput_pointerEnter, MouseInput_pointerExit, OnFalse, OnTrue, SetIcon, SetRadioGroup, UpdateIconState
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/PartCategorizerPopup.cs
- L=200 kind=MonoBehaviour type=PartCategorizerPopup bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(66): UnityEngine.UI=5, GameObject=5, SetActive=4, GetComponent=2, Transform=1, TMPro=1 | ksp: Localizer=5, HighLogic=4, InputLockManager=2 | fan_in=1
- methods(13): AcceptDelete, Awake, CreateIcon, CreateIconList, CriteriaAccept, CriteriaDelete, Hide, MouseinputAccept, MouseinputClose, MouseinputDelete, OnIconSelect, Show, UpdateIconList
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/PartCategorizerPopupAddPart.cs
- L=191 kind=MonoBehaviour type=PartCategorizerPopupAddPart bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(84): GameObject=14, Transform=12, GetComponent=7, UnityEngine.UI=6, SetActive=2, Lifecycle=2 | ksp: Localizer=3, HighLogic=2 | fan_in=1
- methods(10): Awake, CreateIconList, CriteriaAccept, Hide, MouseinputAccept, MouseinputClose, OnDestroy, OnIconSelect, Show, UpdateIconList
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/PartDropZone.cs
- L=68 kind=MonoBehaviour type=PartDropZone bases=TooltipController anc=TooltipController/MonoBehaviour ns=KSP.UI.Screens
- unity(52): Lifecycle=2, GetComponent=1, EventSystems=1 | ksp: EditorLogic=3, Localizer=2, AvailablePart=1 | fan_in=1
- methods(4): Awake, ButtonInputDelegate, OnDestroy, OnTooltipSpawned
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/PartListCategories/BulkheadProfile.cs
- L=74 kind=class type=BulkheadProfile bases=PartCategory anc=PartCategory ns=KSP.UI.Screens.PartListCategories
- unity(5): none | ksp: AvailablePart=2 | fan_in=1
- methods(3): ExclusionCriteria, appendValue, defaultTag
- auto: A / Editor

## Assembly-CSharp/KSP/UI/Screens/PartListCategories/PartCategory.cs
- L=33 kind=class type=PartCategory bases=- anc=- ns=KSP.UI.Screens.PartListCategories
- unity(35): UnityEngine.UI=1 | ksp: AvailablePart=3, Localizer=1 | fan_in=2
- methods(2): ExclusionCriteria, GetCategory
- auto: A / Editor

## Assembly-CSharp/KSP/UI/Screens/PartListCategories/TechTier.cs
- L=27 kind=class type=TechTier bases=PartCategory anc=PartCategory ns=KSP.UI.Screens.PartListCategories
- unity(12): none | ksp: AvailablePart=1 | fan_in=1
- methods(1): ExclusionCriteria
- auto: A / Editor

## Assembly-CSharp/KSP/UI/Screens/PrereleaseWarningDialog.cs
- L=32 kind=MonoBehaviour type=PrereleaseWarningDialog bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(87): Instantiate/Destroy=2, GameObject=2, UnityEngine.UI=1, Transform=1, TMPro=1, MonoBehaviour=1 | ksp: Localizer=1 | fan_in=0
- methods(2): Awake, OnBtnContinue
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/ProtoStageIcon.cs
- L=242 kind=class type=ProtoStageIcon bases=- anc=- ns=KSP.UI.Screens
- unity(8): none | ksp: Part=4, HighLogic=1, FlightGlobals=1 | fan_in=7
- methods(14): BlinkBorder, ClearInfoBoxes, CreateIcon, DisableIcon, DisplayInfo, Freeze, Highlight, RemoveIcon, RemoveInfo, SetBackgroundColor, SetBorderColor, SetIcon, SetIconColor, Unfreeze
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/ProtoStageIconInfo.cs
- L=129 kind=class type=ProtoStageIconInfo bases=- anc=- ns=KSP.UI.Screens
- unity(9): none | ksp: none | fan_in=4
- methods(9): SetCaption, SetLength, SetMessage, SetMsgBgColor, SetMsgTextColor, SetProgressBarBgColor, SetProgressBarColor, SetValue, StartInfoBox
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/RDArchivesController.cs
- L=771 kind=MonoBehaviour type=RDArchivesController bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(45): Instantiate/Destroy=10, GameObject=10, GetComponent=6, UnityEngine.UI=3, TMPro=2, Lifecycle=2 | ksp: Localizer=14, HighLogic=3, ScienceSubject=1 | fan_in=1
- methods(34): AddPlanetRecursively, AddPlanets, CacheReportLists, Cascade, ClearALLSubFilterLists, ClearAllSubFilterSortingFlags, ClearSubFilterList, DestroyInstructor, DestroyReportLists, DropdownListCallback, DropdownListItemCallback, FilterBiomes, FilterExperiments, FilterList, FilterReports, FilterReportsFunc, FilterSituations, GetPlanetFilterFunction, GetPlanetFilterlist, OnDestroy, OnReportListSort, RandomFloat, RefreshALLSubFilterLists, RefreshSubFilterList ...
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/RDController.cs
- L=291 kind=MonoBehaviour type=RDController bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(60): GameObject=7, SetActive=5, TMPro=4, Lifecycle=4, Transform=2, UnityEngine.UI=1 | ksp: Localizer=6, AvailablePart=6, ResearchAndDevelopment=1, PartLoader=1, HighLogic=1 | fan_in=8
- methods(11): ActionButtonClick, Awake, OnDestroy, OnEnable, RegisterNode, ShowNodePanel, ShowNothingPanel, SnapToStartNode, Start, UpdatePanel, UpdatePurchaseButton
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/RDDropDownList.cs
- L=178 kind=MonoBehaviour type=RDDropDownList bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(58): GetComponent=7, Instantiate/Destroy=2, EventSystems=2, TMPro=1, SetActive=1, MonoBehaviour=1 | ksp: none | fan_in=4
- methods(15): AddItem, AddSelectedItem, ClearList, Close, CloseInternal, CreateInstanceFromPrefab, GetBiomeFilter, GetFilter, OnFalse, OnTrue, Open, OpenInternal, SetStandardToggleButtonInputDelegate, UpdateItem, UpdateSelectedItem
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/RDDropDownListContainer.cs
- L=28 kind=MonoBehaviour type=RDDropDownListContainer bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(50): MonoBehaviour=1, GetComponent=1 | ksp: none | fan_in=1
- methods(1): Initialize
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/RDDropDownListItem.cs
- L=21 kind=MonoBehaviour type=RDDropDownListItem bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(67): TMPro=2, MonoBehaviour=1 | ksp: none | fan_in=1
- methods(1): Setup
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/RDEnvironmentAdapter.cs
- L=79 kind=static class type=RDEnvironmentAdapter bases=- anc=- ns=KSP.UI.Screens
- unity(0): none | ksp: ResearchAndDevelopment=16, CelestialBody=2, ScienceSubject=1 | fan_in=2
- methods(8): GetBiomeTags, GetBiomeTagsLocalized, GetExperiment, GetExperimentIDs, GetResults, GetSituationTags, GetSituationTagsDescriptions, GetSubjects
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/RDGridArea.cs
- L=6 kind=MonoBehaviour type=RDGridArea bases=UIGridArea anc=UIGridArea/MonoBehaviour ns=KSP.UI.Screens
- unity(15): none | ksp: none | fan_in=1
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/RDNode.cs
- L=771 kind=MonoBehaviour type=RDNode bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(59): Transform=38, GameObject=6, Instantiate/Destroy=3, SetActive=2, Material=2, Lifecycle=2 | ksp: ConfigNode=7, Localizer=4, AvailablePart=2 | fan_in=9
- methods(26): Awake, DrawArrow, FakeGizmos, FindNodeByID, GetPosition, GetTooltipCaption, GetVectorArray, GetVisiblePartsCount, InitializeArrows, Load, LoadLinks, NodeInput, OnDrawGizmos, PartsInTotal, PartsNotUnlocked, Save, SelectNode, SetButtonState, SetIconState, Setup, ShowArrows, Start, UnselectNode, UpdateGraphics ...
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/RDNodeList.cs
- L=60 kind=MonoBehaviour type=RDNodeList bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(89): SetActive=8, GameObject=8, GetComponent=2, MonoBehaviour=1, Lifecycle=1, Instantiate/Destroy=1 | ksp: none | fan_in=1
- methods(3): AddNodes, Awake, ClearList
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/RDNodeListItem.cs
- L=13 kind=MonoBehaviour type=RDNodeListItem bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(55): TMPro=1, MonoBehaviour=1 | ksp: none | fan_in=1
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/RDNodePrefab.cs
- L=162 kind=MonoBehaviour type=RDNodePrefab bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(85): UnityEngine.UI=11, Transform=8, Texture=6, GameObject=3, SetActive=2, Material=2 | ksp: none | fan_in=2
- methods(14): AddInputDelegate, GetArrowHeadPrefab, GetIconColor, HideAvailablePartsCircle, InstantiateArrowHeadAtPos, SelectNode, SetArrowHeadState, SetAvailablePartsCircle, SetIcon, SetIconColor, SetScale, SetViewable, Setup, UnselectNode
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/RDPartList.cs
- L=193 kind=MonoBehaviour type=RDPartList bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(62): GetComponent=6, UnityEngine.UI=3, Texture=2, Lifecycle=2, Instantiate/Destroy=2, MonoBehaviour=1 | ksp: AvailablePart=12, Localizer=2, Funding=2, PartLoader=1 | fan_in=1
- methods(10): AddPartListItem, AddParts, AddUpgradeListItem, AddUpgrades, Awake, OnDestroy, OnPartListUpdate, Refresh, SetPart, SetupParts
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/RDPartListItem.cs
- L=29 kind=MonoBehaviour type=RDPartListItem bases=EditorPartIcon anc=EditorPartIcon/MonoBehaviour ns=KSP.UI.Screens
- unity(59): UnityEngine.UI=1, TMPro=1 | ksp: AvailablePart=2 | fan_in=2
- methods(1): Setup
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/RDPlanetListItemContainer.cs
- L=286 kind=MonoBehaviour type=RDPlanetListItemContainer bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(85): Transform=16, GameObject=10, GetComponent=7, Texture=4, EventSystems=4, Camera=4 | ksp: Localizer=1 | fan_in=1
- methods(22): AddChild, CreateThumbnailCamera, GetParentInHierarchy, Hide, HideChildren, OnDestroy, OnFalse, OnPointerEnter, OnPointerExit, OnTrue, PopInPlanet, PopOutPlanet, RotatePlanet, SelectionCallback, SetLightDirection, SetSelectionCallback, Setup, Show, ShowChildren, StartRotation, StopRotation, Update
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/RDReportListItemContainer.cs
- L=97 kind=MonoBehaviour type=RDReportListItemContainer bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(65): TMPro=4, Lifecycle=2, UnityEngine.UI=1, MonoBehaviour=1, IMGUI=1 | ksp: Localizer=4 | fan_in=1
- methods(9): ClosePopup, OnButtonInput, OnDestroy, PushDataToGUI, SetDataLabel, SetDescriptionLabel, SetScienceLabel, SetValueSlider, Start
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/RDSceneDespawner.cs
- L=20 kind=MonoBehaviour type=RDSceneDespawner bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(63): UnityEngine.UI=1, MonoBehaviour=1, Lifecycle=1 | ksp: GameEvents=1 | fan_in=0
- methods(2): BtnExit, Start
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/RDSceneSpawner.cs
- L=47 kind=MonoBehaviour type=RDSceneSpawner bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(87): Light=6, Texture=2, Lifecycle=2, MonoBehaviour=1 | ksp: GameEvents=4, ResearchAndDevelopment=1, HighLogic=1 | fan_in=0
- methods(4): Awake, OnDestroy, onRDDespawn, onRDSpawn
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/RDTechTree.cs
- L=399 kind=MonoBehaviour type=RDTechTree bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(40): GameObject=6, GetComponent=2, MonoBehaviour=1, Lifecycle=1, LayerMask=1, Instantiate/Destroy=1 | ksp: ConfigNode=27, HighLogic=4, ResearchAndDevelopment=3, GameDatabase=1 | fan_in=5
- methods(18): Awake, FindTech, GetCheapestUnavailableNodes, GetNextUnavailableNodes, GetRDScreenPrefab, GetTechNodesFromDB, GetTreeConfigNode, GetTreeNodes, GetTreeTechs, LoadTechTitles, LoadTechTree, PreLoad, ReLoad, RefreshUI, SaveTechTree, SpawnTechTreeNodes, WipeTechTree, recurseForNextTechs
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/RDTechTreeSearchBar.cs
- L=305 kind=MonoBehaviour type=RDTechTreeSearchBar bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(48): UnityEngine.UI=3, Coroutine=3, Time.=2, Lifecycle=2, GetComponent=2, GameObject=2 | ksp: AvailablePart=2 | fan_in=4
- methods(15): Awake, ClearSearchBox, DeselectSearchResults, PartMatchesSearch, SearchField_OnClick, SearchField_OnValueChange, SearchForNode, SearchRoutine, SearchStart, SearchStop, SearchTagSplit, SelectPartIcons, Start, TagMatchType, TermMatchesTag
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/RDTestSceneLoader.cs
- L=360 kind=MonoBehaviour type=RDTestSceneLoader bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(48): GameObject=9, Transform=2, Lifecycle=2, SetActive=1, MonoBehaviour=1, Instantiate/Destroy=1 | ksp: ConfigNode=17, AvailablePart=9, CelestialBody=4, ScienceSubject=3, PartResourceDefinition=1 | fan_in=5
- methods(12): Awake, GetBiomeTags, GetBiomeTagsLocalized, GetExperiment, GetExperimentIDs, GetMiniBiomeTags, GetMiniBiomeTagsLocalized, GetResults, GetSituationTags, GetSituationTagsDescriptions, LoadPartDefs, OnDestroy
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/ResourceDisplay.cs
- L=478 kind=MonoBehaviour type=ResourceDisplay bases=UIApp anc=UIApp/MonoBehaviour ns=KSP.UI.Screens
- unity(58): GameObject=13, SetActive=11, Transform=4, Resources.=4, Instantiate/Destroy=4, Coroutine=3 | ksp: FlightGlobals=12, Part=8, GameEvents=8, Vessel=7, PartResourceDefinition=5 | fan_in=4
- methods(32): AddOnHideResource, AddOnShowResource, ClearResourceList, CreateResourceList, DisablePanel, DisplayApp, EnablePanel, ForceHideResourceList, GetAppScenes, GetAppScreenPos, HideApp, HideResourceList, Hover, HoverOut, OnAppAboutToStart, OnAppDestroy, OnAppInitialized, OnHideResource, OnShowResource, OnVesselChanged, OnVesselDestroy, OnVesselGoOffRails, OnVesselWasModified, Refresh ...
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/ResourceDisplayOptions.cs
- L=24 kind=MonoBehaviour type=ResourceDisplayOptions bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(55): UnityEngine.UI=1, MonoBehaviour=1 | ksp: none | fan_in=1
- methods(2): OnStageInput, Setup
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/ResourceItem.cs
- L=201 kind=MonoBehaviour type=ResourceItem bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(60): TMPro=4, Lifecycle=3, UnityEngine.UI=2, Time.=2, EventSystems=2, Resources.=1 | ksp: PartResource=2, Vessel=1, PartResourceDefinition=1, FlightGlobals=1 | fan_in=2
- methods(8): Awake, FixedUpdate, MouseInput_click, MouseInput_pointerEnter, MouseInput_pointerExit, Setup, Update, UpdateStaged
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/RotationalGauge.cs
- L=116 kind=MonoBehaviour type=RotationalGauge bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(53): Lifecycle=2, UnityEngine.UI=1, Time.=1, MonoBehaviour=1 | ksp: none | fan_in=4
- methods(3): Awake, LateUpdate, SetValue
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/RotationalGaugeOffsetMarker.cs
- L=91 kind=MonoBehaviour type=RotationalGaugeOffsetMarker bases=RotationalGauge anc=RotationalGauge/MonoBehaviour ns=KSP.UI.Screens
- unity(85): Transform=4, Lifecycle=4, GameObject=4, UnityEngine.UI=2, TMPro=2, GetComponent=2 | ksp: none | fan_in=1
- methods(7): Awake, LateUpdate, OnDestroy, SetNextToValue, SetTextField, Start, ToggleOffsetMarker
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/AccessorBase.cs
- L=181 kind=class type=AccessorBase bases=- anc=- ns=KSP.UI.Screens.Settings.Controls
- unity(17): none | ksp: none | fan_in=4
- methods(3): Create, IsDigit, IsIdentifier
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/FieldAccessor.cs
- L=26 kind=class type=FieldAccessor bases=AccessorBase anc=AccessorBase ns=KSP.UI.Screens.Settings.Controls
- unity(0): none | ksp: none | fan_in=1
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/PropertyAccessor.cs
- L=27 kind=class type=PropertyAccessor bases=AccessorBase anc=AccessorBase ns=KSP.UI.Screens.Settings.Controls
- unity(0): none | ksp: none | fan_in=1
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/ReflectedSettingsWindowTab.cs
- L=43 kind=MonoBehaviour type=ReflectedSettingsWindowTab bases=SettingsControlBase anc=SettingsControlBase/MonoBehaviour ns=KSP.UI.Screens.Settings.Controls
- unity(73): GameObject=4, UnityEngine.UI=1, SetActive=1, Lifecycle=1 | ksp: none | fan_in=1
- methods(3): OnStart, ShowTab, Start
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsAxisScale.cs
- L=83 kind=MonoBehaviour type=SettingsAxisScale bases=SettingsControlReflection anc=SettingsControlReflection/SettingsControlBase/MonoBehaviour ns=KSP.UI.Screens.Settings.Controls
- unity(31): UnityEngine.UI=1 | ksp: none | fan_in=0
- methods(5): OnStart, PreApply, SliderValueChange, ValueInitialized, ValueUpdated
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsBoolToggle.cs
- L=47 kind=MonoBehaviour type=SettingsBoolToggle bases=SettingsControlReflection anc=SettingsControlReflection/SettingsControlBase/MonoBehaviour ns=KSP.UI.Screens.Settings.Controls
- unity(15): none | ksp: none | fan_in=0
- methods(4): OnStart, OnToggled, ValueInitialized, ValueUpdated
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsBoolToggleBitMask.cs
- L=45 kind=MonoBehaviour type=SettingsBoolToggleBitMask bases=SettingsControlReflection anc=SettingsControlReflection/SettingsControlBase/MonoBehaviour ns=KSP.UI.Screens.Settings.Controls
- unity(15): none | ksp: none | fan_in=0
- methods(4): OnApply, OnStart, OnToggled, ValueInitialized
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsBoolTogglePPFX.cs
- L=112 kind=MonoBehaviour type=SettingsBoolTogglePPFX bases=SettingsControlReflection anc=SettingsControlReflection/SettingsControlBase/MonoBehaviour ns=KSP.UI.Screens.Settings.Controls
- unity(28): Lifecycle=1, GetComponent=1 | ksp: GameSettings=1 | fan_in=0
- methods(6): OnStart, OnToggled, SetupAdditionalFields, Update, ValueInitialized, ValueUpdated
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsControlBase.cs
- L=45 kind=MonoBehaviour type=SettingsControlBase bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.Settings.Controls
- unity(46): TMPro=1, MonoBehaviour=1 | ksp: none | fan_in=11
- methods(4): IsValid, OnApply, OnRevert, SetTitleText
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsControlReflection.cs
- L=110 kind=MonoBehaviour type=SettingsControlReflection bases=SettingsControlBase anc=SettingsControlBase/MonoBehaviour ns=KSP.UI.Screens.Settings.Controls
- unity(44): TMPro=1, SendMessage/Invoke=1, Lifecycle=1, GameObject=1 | ksp: GameSettings=1 | fan_in=17
- methods(10): GetAccessor, GetValue, OnApply, OnRevert, OnStart, PreApply, SetValue, Start, ValueInitialized, ValueUpdated
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsFloatRange.cs
- L=76 kind=MonoBehaviour type=SettingsFloatRange bases=SettingsControlReflection anc=SettingsControlReflection/SettingsControlBase/MonoBehaviour ns=KSP.UI.Screens.Settings.Controls
- unity(36): UnityEngine.UI=1 | ksp: none | fan_in=0
- methods(5): OnSliderValueChange, OnStart, SetColorForRecommendedUIScale, ValueInitialized, ValueUpdated
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsGraphicsResolution.cs
- L=117 kind=MonoBehaviour type=SettingsGraphicsResolution bases=SettingsControlBase anc=SettingsControlBase/MonoBehaviour ns=KSP.UI.Screens.Settings.Controls
- unity(50): Screen/Cursor=4, UnityEngine.UI=2, TMPro=1, Lifecycle=1, Application.=1 | ksp: GameSettings=2, Localizer=1 | fan_in=0
- methods(7): GetAvailableResolutions, OnApply, OnButtonDown, OnButtonUp, OnRevert, SetValue, Start
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsInput.cs
- L=208 kind=MonoBehaviour type=SettingsInput bases=SettingsWindow anc=SettingsWindow/MonoBehaviour ns=KSP.UI.Screens.Settings.Controls
- unity(73): Transform=14, GameObject=8, Instantiate/Destroy=6, UnityEngine.UI=2, SendMessage/Invoke=2, Lifecycle=1 | ksp: none | fan_in=0
- methods(6): LoadFile, OnBindingRequest, OnBindingRequestFinished, OnBindingRequestModes, SaveFile, Start
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsInputAxis.cs
- L=140 kind=MonoBehaviour type=SettingsInputAxis bases=SettingsControlReflection anc=SettingsControlReflection/SettingsControlBase/MonoBehaviour ns=KSP.UI.Screens.Settings.Controls
- unity(66): UnityEngine.UI=6, TMPro=6 | ksp: GameSettings=8, Localizer=2 | fan_in=1
- methods(12): OnChangePrimaryDeadzone, OnChangePrimarySensitivity, OnChangeSecondaryDeadzone, OnChangeSecondarySensitivity, OnClickPrimary, OnClickSecondary, OnStart, OnTogglePrimaryInvert, OnToggleSecondaryInvert, UpdateInputSettings, ValueInitialized, ValueUpdated
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsInputBinding.cs
- L=375 kind=MonoBehaviour type=SettingsInputBinding bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.Settings.Controls
- unity(66): KeyCode=11, Transform=6, Instantiate/Destroy=5, UnityEngine.UI=4, TMPro=3, Input.=3 | ksp: Localizer=17, ConfigNode=8 | fan_in=3
- methods(14): OnAccept, OnCancel, OnClear, OnModeDockingRotation, OnModeDockingTranslation, OnModeStaging, ResetAxisValues, SetAxis, SetKey, SetupAxis, SetupKey, SetupModes, Start, Update
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsInputBindingMode.cs
- L=12 kind=MonoBehaviour type=SettingsInputBindingMode bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.Settings.Controls
- unity(55): TMPro=1, MonoBehaviour=1 | ksp: none | fan_in=1
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsInputGroupTitle.cs
- L=6 kind=MonoBehaviour type=SettingsInputGroupTitle bases=SettingsControlBase anc=SettingsControlBase/MonoBehaviour ns=KSP.UI.Screens.Settings.Controls
- unity(15): none | ksp: none | fan_in=2
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsInputKey.cs
- L=84 kind=MonoBehaviour type=SettingsInputKey bases=SettingsControlReflection anc=SettingsControlReflection/SettingsControlBase/MonoBehaviour ns=KSP.UI.Screens.Settings.Controls
- unity(51): UnityEngine.UI=2, TMPro=2 | ksp: Localizer=4, ConfigNode=1 | fan_in=2
- methods(7): GetValue, OnClickPrimary, OnClickSecondary, OnStart, UpdateInputSettings, ValueInitialized, ValueUpdated
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsIntArrayButtons.cs
- L=92 kind=MonoBehaviour type=SettingsIntArrayButtons bases=SettingsControlReflection anc=SettingsControlReflection/SettingsControlBase/MonoBehaviour ns=KSP.UI.Screens.Settings.Controls
- unity(36): UnityEngine.UI=2 | ksp: Localizer=1 | fan_in=1
- methods(5): OnButtonDown, OnButtonUp, OnStart, ValueInitialized, ValueUpdated
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsIntArrayRange.cs
- L=82 kind=MonoBehaviour type=SettingsIntArrayRange bases=SettingsControlReflection anc=SettingsControlReflection/SettingsControlBase/MonoBehaviour ns=KSP.UI.Screens.Settings.Controls
- unity(29): UnityEngine.UI=1 | ksp: Localizer=1 | fan_in=0
- methods(4): OnSliderValueChange, OnStart, ValueInitialized, ValueUpdated
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsIntButtons.cs
- L=41 kind=MonoBehaviour type=SettingsIntButtons bases=SettingsControlReflection anc=SettingsControlReflection/SettingsControlBase/MonoBehaviour ns=KSP.UI.Screens.Settings.Controls
- unity(52): UnityEngine.UI=2 | ksp: none | fan_in=0
- methods(3): OnButtonDown, OnButtonUp, OnStart
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsIntRange.cs
- L=39 kind=MonoBehaviour type=SettingsIntRange bases=SettingsControlReflection anc=SettingsControlReflection/SettingsControlBase/MonoBehaviour ns=KSP.UI.Screens.Settings.Controls
- unity(40): UnityEngine.UI=1 | ksp: none | fan_in=0
- methods(4): OnSliderValueChange, OnStart, ValueInitialized, ValueUpdated
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsIntToggle.cs
- L=53 kind=MonoBehaviour type=SettingsIntToggle bases=SettingsControlReflection anc=SettingsControlReflection/SettingsControlBase/MonoBehaviour ns=KSP.UI.Screens.Settings.Controls
- unity(15): none | ksp: none | fan_in=0
- methods(4): OnStart, OnToggled, ValueInitialized, ValueUpdated
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsKeyboardLayoutApply.cs
- L=17 kind=MonoBehaviour type=SettingsKeyboardLayoutApply bases=SettingsControlBase anc=SettingsControlBase/MonoBehaviour ns=KSP.UI.Screens.Settings.Controls
- unity(55): UnityEngine.UI=1, Lifecycle=1 | ksp: GameSettings=1 | fan_in=1
- methods(1): Start
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsKeyboardLayoutInput.cs
- L=158 kind=MonoBehaviour type=SettingsKeyboardLayoutInput bases=SettingsControlBase anc=SettingsControlBase/MonoBehaviour ns=KSP.UI.Screens.Settings.Controls
- unity(37): UnityEngine.UI=2, TMPro=1, Lifecycle=1 | ksp: GameSettings=9, ConfigNode=5, Localizer=1 | fan_in=1
- methods(10): ChangeLayout, CleanLayout, Init, LoadCurrentLayout, OnApply, OnNext, OnPrev, OnRevert, SetKeysLayout, Start
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsKeyboardLayoutOs.cs
- L=18 kind=MonoBehaviour type=SettingsKeyboardLayoutOs bases=SettingsControlBase anc=SettingsControlBase/MonoBehaviour ns=KSP.UI.Screens.Settings.Controls
- unity(59): TMPro=1, Lifecycle=1 | ksp: Localizer=1 | fan_in=1
- methods(1): Start
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsLabel.cs
- L=20 kind=MonoBehaviour type=SettingsLabel bases=SettingsControlReflection anc=SettingsControlReflection/SettingsControlBase/MonoBehaviour ns=KSP.UI.Screens.Settings.Controls
- unity(18): none | ksp: none | fan_in=0
- methods(1): OnStart
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsLayoutConfig.cs
- L=138 kind=MonoBehaviour type=SettingsLayoutConfig bases=SettingsWindow anc=SettingsWindow/MonoBehaviour ns=KSP.UI.Screens.Settings.Controls
- unity(80): Transform=14, SetActive=4, Instantiate/Destroy=4, Lifecycle=3, TMPro=2, GetComponent=2 | ksp: Localizer=3, ConfigNode=2 | fan_in=3
- methods(10): Awake, OnApply, OnDestroy, OnLayoutChange, OnLayoutResult, OnRebindResult, OnRevert, SaveChanges, ShowLayoutChangePopUp, Start
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsScreenTab.cs
- L=80 kind=MonoBehaviour type=SettingsScreenTab bases=SettingsControlBase anc=SettingsControlBase/MonoBehaviour ns=KSP.UI.Screens.Settings.Controls
- unity(81): UnityEngine.UI=5, SetActive=3, GameObject=3, Lifecycle=2, EventSystems=2, GetComponent=1 | ksp: none | fan_in=1
- methods(5): Activate, Awake, DeselectTab, SelectTab, Start
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsTerrainDetail.cs
- L=110 kind=MonoBehaviour type=SettingsTerrainDetail bases=SettingsControlBase anc=SettingsControlBase/MonoBehaviour ns=KSP.UI.Screens.Settings.Controls
- unity(38): UnityEngine.UI=1, TMPro=1, Lifecycle=1 | ksp: Localizer=1, GameEvents=1 | fan_in=0
- methods(4): OnApply, OnRevert, OnSliderValueChange, Start
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/SettingsTrackIRBool.cs
- L=112 kind=MonoBehaviour type=SettingsTrackIRBool bases=SettingsControlReflection anc=SettingsControlReflection/SettingsControlBase/MonoBehaviour ns=KSP.UI.Screens.Settings.Controls
- unity(29): GameObject=1 | ksp: GameSettings=1 | fan_in=0
- methods(9): EnableInteraction, GetAccessor, GetValue, OnStart, OnToggled, SetValue, SettingChanged, ValueInitialized, ValueUpdated
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/Values/SettingsValueBoolTitleChange.cs
- L=28 kind=MonoBehaviour type=SettingsValueBoolTitleChange bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.Settings.Controls.Values
- unity(59): MonoBehaviour=1, Lifecycle=1, GetComponent=1 | ksp: none | fan_in=0
- methods(2): Awake, UpdateValue
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/Controls/Values/SettingsValueIntToString.cs
- L=25 kind=MonoBehaviour type=SettingsValueIntToString bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.Settings.Controls.Values
- unity(59): MonoBehaviour=1, Lifecycle=1, GetComponent=1 | ksp: none | fan_in=0
- methods(2): Awake, UpdateValue
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/ReflectedSettingsWindow.cs
- L=370 kind=MonoBehaviour type=ReflectedSettingsWindow bases=SettingsWindow anc=SettingsWindow/MonoBehaviour ns=KSP.UI.Screens.Settings
- unity(42): GameObject=6, Transform=3, Instantiate/Destroy=3, UnityEngine.UI=1, Lifecycle=1 | ksp: none | fan_in=3
- methods(9): GetField, GetTabParent, IsValid, IsValidSecondary, OnCloseWindow, OnOpenWindow, Setup, SetupReflectionValues, Start
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/ReflectedSettingsWindow_6DOF.cs
- L=10 kind=MonoBehaviour type=ReflectedSettingsWindow_6DOF bases=ReflectedSettingsWindow anc=ReflectedSettingsWindow/SettingsWindow/MonoBehaviour ns=KSP.UI.Screens.Settings
- unity(15): none | ksp: none | fan_in=0
- methods(1): IsValidSecondary
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/ReflectedSettingsWindow_Mouse.cs
- L=10 kind=MonoBehaviour type=ReflectedSettingsWindow_Mouse bases=ReflectedSettingsWindow anc=ReflectedSettingsWindow/SettingsWindow/MonoBehaviour ns=KSP.UI.Screens.Settings
- unity(15): none | ksp: none | fan_in=0
- methods(1): IsValidSecondary
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/ReflectedSettingsWindow_TrackIR.cs
- L=10 kind=MonoBehaviour type=ReflectedSettingsWindow_TrackIR bases=ReflectedSettingsWindow anc=ReflectedSettingsWindow/SettingsWindow/MonoBehaviour ns=KSP.UI.Screens.Settings
- unity(15): none | ksp: none | fan_in=0
- methods(1): IsValidSecondary
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/SettingsExpansion.cs
- L=31 kind=static class type=SettingsExpansion bases=- anc=- ns=KSP.UI.Screens.Settings
- unity(0): none | ksp: none | fan_in=2
- methods(1): IsExpansion
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/SettingsFieldWrapper.cs
- L=17 kind=class type=SettingsFieldWrapper bases=- anc=- ns=KSP.UI.Screens.Settings
- unity(0): none | ksp: none | fan_in=0
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/SettingsPlatform.cs
- L=57 kind=static class type=SettingsPlatform bases=- anc=- ns=KSP.UI.Screens.Settings
- unity(33): Application.=7 | ksp: none | fan_in=2
- methods(1): IsPlatform
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/SettingsScreen.cs
- L=638 kind=MonoBehaviour type=SettingsScreen bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.Settings
- unity(85): UnityEngine.UI=43, Transform=42, GetComponent=14, GameObject=13, Instantiate/Destroy=9, SetActive=6 | ksp: GameSettings=8, HighLogic=2, ConfigNode=2, GameEvents=1 | fan_in=7
- methods(26): Awake, CacheCurrentSelectedItemColumn, Clear, CloseResetPopUp, FocusSelectableWithinRange, GetKeyLayoutMap, LoadResetPopupSelectablesCo, LoadSelectables, LoadSelectablesCo, NavigateBetweenColumns, OnAccept, OnApply, OnCancel, OnCloseWindow, OnDestroy, OnOpenWindow, OnReset, OnRevert, Recreate, SetExplicitUINavigation, Setup, SetupTab, SetupWindow, Start ...
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/SettingsScreenSetup.cs
- L=11 kind=MonoBehaviour type=SettingsScreenSetup bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.Settings
- unity(36): MonoBehaviour=1 | ksp: none | fan_in=1
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/SettingsSetup.cs
- L=335 kind=class type=SettingsSetup bases=- anc=- ns=KSP.UI.Screens.Settings
- unity(6): none | ksp: none | fan_in=3
- methods(10): FloatArrayToString, GetControlTypeFields, GetField, IntArrayToString, IsValid, SetupReflectionValues, StringArrayToString, StringToFloatArray, StringToIntArray, StringToStringArray
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/SettingsTemplate.cs
- L=9 kind=MonoBehaviour type=SettingsTemplate bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.Settings
- unity(55): UnityEngine.UI=1, MonoBehaviour=1 | ksp: none | fan_in=4
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/SettingsTemplateDualLayouts.cs
- L=22 kind=MonoBehaviour type=SettingsTemplateDualLayouts bases=SettingsTemplate anc=SettingsTemplate/MonoBehaviour ns=KSP.UI.Screens.Settings
- unity(59): UnityEngine.UI=2 | ksp: none | fan_in=2
- methods(1): GetLayoutFlipFlop
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/SettingsValue.cs
- L=15 kind=attribute type=SettingsValue bases=Attribute anc=Attribute ns=KSP.UI.Screens.Settings
- unity(0): none | ksp: none | fan_in=19
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Settings/SettingsWindow.cs
- L=16 kind=MonoBehaviour type=SettingsWindow bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.Settings
- unity(36): MonoBehaviour=1 | ksp: none | fan_in=6
- methods(1): IsValid
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/SimpleLayout.cs
- L=83 kind=MonoBehaviour type=SimpleLayout bases=MonoBehaviour,ApplauncherLayout anc=MonoBehaviour ns=KSP.UI.Screens
- unity(80): UnityEngine.UI=10, GameObject=2, Transform=1, MonoBehaviour=1 | ksp: none | fan_in=1
- methods(11): GetBottomLeftSpacer, GetGameObject, GetKnowledgeBaseList, GetModList, GetModListBtnDown, GetModListBtnUp, GetModListDivider, GetModScrollRect, GetRectTransform, GetStockList, GetTopRightSpacer
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/SpaceCenter/BuildingPicker.cs
- L=136 kind=MonoBehaviour type=BuildingPicker bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.SpaceCenter
- unity(70): GameObject=5, SetActive=4, UnityEngine.UI=2, Transform=2, Coroutine=2, MonoBehaviour=1 | ksp: InputLockManager=1 | fan_in=0
- methods(6): ConstructBuildingList, ConstructUI, HideUI, ShowUI, Start, Update
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/SpaceCenter/BuildingPickerItem.cs
- L=136 kind=MonoBehaviour type=BuildingPickerItem bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler anc=MonoBehaviour ns=KSP.UI.Screens.SpaceCenter
- unity(82): EventSystems=8, UnityEngine.UI=4, Transform=4, Lifecycle=2, Instantiate/Destroy=2, GameObject=2 | ksp: GameEvents=2, FlightCamera=1 | fan_in=1
- methods(11): Awake, CreatePointer, DestroyPointer, OnBuildingClick, OnBuildingInView, OnDestroy, OnGameUnpause, OnPointerClick, OnPointerEnter, OnPointerExit, Setup
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/SpaceCenter/MissionSummaryDialog/CrewWidget.cs
- L=130 kind=MonoBehaviour type=CrewWidget bases=MissionSummaryWidget anc=MissionSummaryWidget/MonoBehaviour ns=KSP.UI.Screens.SpaceCenter.MissionSummaryDialog
- unity(81): Texture=11, SetActive=4, GameObject=4, TMPro=2, UnityEngine.UI=1, Instantiate/Destroy=1 | ksp: Localizer=5, ProtoCrewMember=4, KerbalRoster=4, HighLogic=3, GameDatabase=1 | fan_in=2
- methods(3): Create, UpdateFields, selectSprite
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/SpaceCenter/MissionSummaryDialog/MissionSummaryWidget.cs
- L=23 kind=MonoBehaviour type=MissionSummaryWidget bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens.SpaceCenter.MissionSummaryDialog
- unity(84): UnityEngine.UI=3, TMPro=1, MonoBehaviour=1, GetComponent=1 | ksp: none | fan_in=4
- methods(1): Init
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/SpaceCenter/MissionSummaryDialog/PartWidget.cs
- L=78 kind=MonoBehaviour type=PartWidget bases=MissionSummaryWidget anc=MissionSummaryWidget/MonoBehaviour ns=KSP.UI.Screens.SpaceCenter.MissionSummaryDialog
- unity(32): Instantiate/Destroy=1, GetComponent=1 | ksp: Localizer=3, AvailablePart=3 | fan_in=2
- methods(5): AddDuplicate, Create, Equals, GetHashCode, UpdateFields
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/SpaceCenter/MissionSummaryDialog/ResourceWidget.cs
- L=67 kind=MonoBehaviour type=ResourceWidget bases=MissionSummaryWidget anc=MissionSummaryWidget/MonoBehaviour ns=KSP.UI.Screens.SpaceCenter.MissionSummaryDialog
- unity(34): Instantiate/Destroy=1, GetComponent=1 | ksp: Localizer=3, PartResourceDefinition=2 | fan_in=2
- methods(5): AddAmount, Create, Equals, GetHashCode, UpdateFields
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/SpaceCenter/MissionSummaryDialog/ScienceSubjectWidget.cs
- L=39 kind=MonoBehaviour type=ScienceSubjectWidget bases=MissionSummaryWidget anc=MissionSummaryWidget/MonoBehaviour ns=KSP.UI.Screens.SpaceCenter.MissionSummaryDialog
- unity(44): Instantiate/Destroy=1, GetComponent=1 | ksp: Localizer=3, ScienceSubject=2 | fan_in=3
- methods(2): Create, UpdateFields
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/SpriteTumblerObject.cs
- L=118 kind=MonoBehaviour type=SpriteTumblerObject bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(55): UnityEngine.UI=2, Texture=1, MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=1
- methods(5): Awake, TumbleTo, UpdateDelta, afterAltModeTumbleTo, afterAltModeUpdateDelta
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/StageIconInfoBox.cs
- L=97 kind=MonoBehaviour type=StageIconInfoBox bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(78): UnityEngine.UI=4, SetActive=3, GameObject=3, TMPro=2, MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=2
- methods(10): Awake, Collapse, Expand, SetCaption, SetMessage, SetMsgBgColor, SetMsgTextColor, SetProgressBarBgColor, SetProgressBarColor, SetValue
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/SteamCraftInfo.cs
- L=110 kind=class type=SteamCraftInfo bases=- anc=- ns=KSP.UI.Screens
- unity(35): Steam=5 | ksp: ConfigNode=6 | fan_in=4
- methods(3): CreateMetaData, ProcessMetaData, UpdateSteamState
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/StrategyListItem.cs
- L=74 kind=MonoBehaviour type=StrategyListItem bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(66): EventSystems=2, UnityEngine.UI=1, Texture=1, TMPro=1, MonoBehaviour=1 | ksp: none | fan_in=1
- methods(4): Initialize, SetupButton, UpdateButton, updateTitle
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/SubassemblyDropZone.cs
- L=203 kind=MonoBehaviour type=SubassemblyDropZone bases=TooltipController anc=TooltipController/MonoBehaviour ns=KSP.UI.Screens
- unity(51): Coroutine=5, Lifecycle=3, EventSystems=2, GetComponent=1, GameObject=1 | ksp: Localizer=15, EditorLogic=6, ShipConstruct=2, Part=2, InputLockManager=2 | fan_in=1
- methods(12): Awake, ButtonInputDelegate, EndDialogCoroutine, EscapeString, OnAppFocus, OnDestroy, OnDisable, OnPointerExit, OnSave, OnTooltipSpawned, SaveSubassembly, SpawnSaveDialog
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/TrackingStationObjectButton.cs
- L=122 kind=MonoBehaviour type=TrackingStationObjectButton bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(84): UnityEngine.UI=8, Input.=4, GetComponent=3, Texture=2, Lifecycle=2, GameObject=2 | ksp: none | fan_in=2
- methods(10): Awake, Lock, OnMouseEnter, OnMouseExit, SetCount, SetState, ShowCount, Unlock, Update, UpdateState
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/TrackingStationWidget.cs
- L=204 kind=MonoBehaviour type=TrackingStationWidget bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(58): TMPro=3, UnityEngine.UI=2, Lifecycle=2, GetComponent=2, GameObject=2, EventSystems=2 | ksp: Vessel=14, Localizer=9, Planetarium=2 | fan_in=2
- methods(12): Awake, CacheLocalStrings, CheckManeuverNode, GetMETString, GetManeuverString, GetMissionInfo, OnMouseEnter, OnMouseExit, SetVessel, Terminate, Update, onToggle
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/Tumbler.cs
- L=169 kind=MonoBehaviour type=Tumbler bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(69): Transform=8, Shader=5, Time.=3, Lifecycle=2, Renderer=1, MonoBehaviour=1 | ksp: none | fan_in=0
- methods(6): Awake, LateUpdate, SetColor, SetValue, TumbleTo, Update
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/TypeButton.cs
- L=35 kind=class type=TypeButton bases=- anc=- ns=KSP.UI.Screens
- unity(65): GameObject=3, UnityEngine.UI=2, TMPro=1 | ksp: none | fan_in=2
- methods(2): Deselect, Select
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/uGUITumblerObject.cs
- L=68 kind=MonoBehaviour type=uGUITumblerObject bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(57): UnityEngine.UI=1, TMPro=1, MonoBehaviour=1, GetComponent=1 | ksp: none | fan_in=0
- methods(3): GetValue, TumbleTo, UpdateDelta
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/UIApp.cs
- L=244 kind=MonoBehaviour type=UIApp bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(63): Coroutine=8, Lifecycle=2, Instantiate/Destroy=2, GameObject=2, EventSystems=2, Animation=2 | ksp: GameEvents=2 | fan_in=13
- methods(27): AddToAppLauncher, Awake, DisablePanel, DisplayApp, EnablePanel, ForceAddToAppLauncher, GetAppScenes, GetAppScreenPos, Hide, HideApp, Hover, HoverOut, HoverOutCoroutine, MouseInput_PointerEnter, MouseInput_PointerExit, OnAppAboutToStart, OnAppDestroy, OnAppInitialized, OnAppLauncherHide, OnAppLauncherReady, OnAppLauncherShow, OnAppStarted, OnDestroy, Reposition ...
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/UIAppSpawner.cs
- L=51 kind=MonoBehaviour type=UIAppSpawner bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(65): Lifecycle=2, GameObject=2, MonoBehaviour=1, Instantiate/Destroy=1 | ksp: GameEvents=2, HighLogic=1 | fan_in=0
- methods(3): Awake, OnDestroy, OnLevelLoaded
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/UIGridArea.cs
- L=202 kind=MonoBehaviour type=UIGridArea bases=MonoBehaviour,IScrollHandler,IEventSystemHandler anc=MonoBehaviour ns=KSP.UI.Screens
- unity(71): Transform=8, UnityEngine.UI=6, Material=2, Lifecycle=2, EventSystems=2, TMPro=1 | ksp: GameSettings=1 | fan_in=1
- methods(14): Awake, CalculateSnapBounds, CenterOfGrid, InputZoomDown, InputZoomUp, MousePointOnGrid, OnScroll, SetZoomButtonState, SetZoomLevelText, Snap, SnapToNode, Start, Zoom, ZoomTo
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/UIScrollRectState.cs
- L=72 kind=MonoBehaviour type=UIScrollRectState bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(66): UnityEngine.UI=2, SetActive=2, GameObject=2, MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=1
- methods(4): Awake, Disable, Enable, SetState
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/UISpaceCenter.cs
- L=261 kind=MonoBehaviour type=UISpaceCenter bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(42): Lifecycle=3, UnityEngine.UI=1, TMPro=1, SetActive=1, MonoBehaviour=1, KeyCode=1 | ksp: GameEvents=29, InputLockManager=7, TimeWarp=2, GameSettings=2 | fan_in=2
- methods(22): Awake, OnApplicationFocus, OnDestroy, OnFacilityDespawn_AC, OnFacilityDespawn_ADM, OnFacilityDespawn_MC, OnFacilityDespawn_RD, OnFacilityDespawn_VSD, OnFacilitySpawn_AC, OnFacilitySpawn_ADM, OnFacilitySpawn_MC, OnFacilitySpawn_RD, OnFacilitySpawn_VSD, OnPauseMenuDismiss, Pause, QuitToMenu, SetBuildingText, UnPause, Update, lockUI, onInputLockModified, unlockUI
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/VesselIconSprite.cs
- L=34 kind=MonoBehaviour type=VesselIconSprite bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(69): UnityEngine.UI=1, Texture=1, MonoBehaviour=1 | ksp: none | fan_in=1
- methods(1): SetType
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/VesselRenameDialog.cs
- L=333 kind=MonoBehaviour type=VesselRenameDialog bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(76): SetActive=11, GameObject=11, UnityEngine.UI=5, Transform=4, TMPro=3, Instantiate/Destroy=3 | ksp: Vessel=3, Localizer=2, InputLockManager=2, GameSettings=2, Part=1 | fan_in=2
- methods(14): OnButtonAccept, OnButtonDismiss, OnButtonRemove, OnNameFieldEndEdit, OnNameFieldModified, OnNameFieldSelected, OnPriorityChanged, OnToggle, Spawn, SpawnNameFromPart, Start, Terminate, ToggleSetup, Update
- auto: C / UI

## Assembly-CSharp/KSP/UI/Screens/VSDSceneSpawner.cs
- L=34 kind=MonoBehaviour type=VSDSceneSpawner bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Screens
- unity(63): Lifecycle=2, MonoBehaviour=1, Coroutine=1 | ksp: GameEvents=5 | fan_in=0
- methods(4): Awake, OnDestroy, onVSDDespawn, onVSDSpawn
- auto: C / UI

## Assembly-CSharp/KSP/UI/SimpleAppFrame.cs
- L=55 kind=MonoBehaviour type=SimpleAppFrame bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(82): Transform=3, UnityEngine.UI=2, Lifecycle=2, GameObject=2, EventSystems=2, MonoBehaviour=1 | ksp: none | fan_in=1
- methods(5): AddGlobalInputDelegate, Awake, OnDestroy, Reposition, Setup
- auto: C / UI

## Assembly-CSharp/KSP/UI/SliderValueToText.cs
- L=40 kind=MonoBehaviour type=SliderValueToText bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(77): UnityEngine.UI=2, TMPro=1, MonoBehaviour=1, Lifecycle=1, GetComponent=1 | ksp: Localizer=1 | fan_in=0
- methods(2): Awake, SliderValueChangeListener
- auto: C / UI

## Assembly-CSharp/KSP/UI/StageManagerParentResizer.cs
- L=40 kind=MonoBehaviour type=StageManagerParentResizer bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(82): UnityEngine.UI=3, Lifecycle=2, Transform=1, MonoBehaviour=1 | ksp: none | fan_in=0
- methods(3): Awake, OnDestroy, Resize
- auto: C / UI

## Assembly-CSharp/KSP/UI/Tooltip.cs
- L=16 kind=MonoBehaviour type=Tooltip bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(86): UnityEngine.UI=4, Transform=1, MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=0
- methods(1): Awake
- auto: C / UI

## Assembly-CSharp/KSP/UI/TooltipController.cs
- L=115 kind=MonoBehaviour type=TooltipController bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler anc=MonoBehaviour ns=KSP.UI
- unity(68): EventSystems=4, UnityEngine.UI=3, Lifecycle=3, MonoBehaviour=1, GetComponent=1, GameObject=1 | ksp: GameEvents=2 | fan_in=9
- methods(11): Awake, OnAppFocus, OnDestroy, OnDisable, OnPointerEnter, OnPointerExit, OnTooltipAboutToDespawn, OnTooltipAboutToSpawn, OnTooltipDespawned, OnTooltipSpawned, OnTooltipUpdate
- auto: C / UI

## Assembly-CSharp/KSP/UI/TooltipTypes/Tooltip_Text.cs
- L=9 kind=MonoBehaviour type=Tooltip_Text bases=Tooltip anc=Tooltip/MonoBehaviour ns=KSP.UI.TooltipTypes
- unity(44): TMPro=1 | ksp: none | fan_in=4
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/TooltipTypes/Tooltip_TitleAndText.cs
- L=11 kind=MonoBehaviour type=Tooltip_TitleAndText bases=Tooltip anc=Tooltip/MonoBehaviour ns=KSP.UI.TooltipTypes
- unity(59): TMPro=2 | ksp: none | fan_in=6
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/TooltipTypes/TooltipController_CrewAC.cs
- L=86 kind=MonoBehaviour type=TooltipController_CrewAC bases=TooltipController anc=TooltipController/MonoBehaviour ns=KSP.UI.TooltipTypes
- unity(15): none | ksp: Localizer=5, HighLogic=5, ProtoCrewMember=3, KerbalRoster=1 | fan_in=3
- methods(4): CacheLocalStrings, OnTooltipAboutToSpawn, OnTooltipSpawned, SetTooltip
- auto: C / UI

## Assembly-CSharp/KSP/UI/TooltipTypes/TooltipController_Text.cs
- L=60 kind=MonoBehaviour type=TooltipController_Text bases=TooltipController anc=TooltipController/MonoBehaviour ns=KSP.UI.TooltipTypes
- unity(27): Lifecycle=1 | ksp: Localizer=1 | fan_in=34
- methods(6): Awake, OnTooltipAboutToSpawn, OnTooltipDespawned, OnTooltipSpawned, OnTooltipUpdate, SetText
- auto: C / UI

## Assembly-CSharp/KSP/UI/TooltipTypes/TooltipController_TitleAndText.cs
- L=38 kind=MonoBehaviour type=TooltipController_TitleAndText bases=TooltipController anc=TooltipController/MonoBehaviour ns=KSP.UI.TooltipTypes
- unity(15): none | ksp: none | fan_in=4
- methods(3): OnTooltipAboutToSpawn, OnTooltipSpawned, OnTooltipUpdate
- auto: C / UI

## Assembly-CSharp/KSP/UI/TooltipTypes/UIRadioButtonTooltip.cs
- L=74 kind=MonoBehaviour type=UIRadioButtonTooltip bases=TooltipController anc=TooltipController/MonoBehaviour ns=KSP.UI.TooltipTypes
- unity(50): EventSystems=2, Lifecycle=1, GetComponent=1 | ksp: none | fan_in=0
- methods(6): Awake, OnFalse, OnTooltipAboutToSpawn, OnTooltipDespawned, OnTooltipSpawned, OnTrue
- auto: C / UI

## Assembly-CSharp/KSP/UI/TooltipTypes/UIStateButtonTooltip.cs
- L=73 kind=MonoBehaviour type=UIStateButtonTooltip bases=TooltipController anc=TooltipController/MonoBehaviour ns=KSP.UI.TooltipTypes
- unity(33): Lifecycle=1, GetComponent=1 | ksp: none | fan_in=0
- methods(6): Awake, FindButtonStateTooltip, OnTooltipAboutToSpawn, OnTooltipDespawned, OnTooltipSpawned, OnValueChanged
- auto: C / UI

## Assembly-CSharp/KSP/UI/TweeningController.cs
- L=101 kind=MonoBehaviour type=TweeningController bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(85): UnityEngine.UI=6, GameObject=6, Transform=5, GetComponent=4, Lifecycle=2, SetActive=1 | ksp: GameEvents=2 | fan_in=4
- methods(7): Awake, OnDestroy, OnGameSceneLoadRequested, OnTweeningComplete, SpawnResizingLayoutElement, Tween, TweenIntoList
- auto: C / UI

## Assembly-CSharp/KSP/UI/TweeningElement.cs
- L=103 kind=MonoBehaviour type=TweeningElement bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(86): Transform=14, UnityEngine.UI=5, Coroutine=3, Time.=2, MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=1
- methods(4): Awake, EaseInBack, StartTweening, Tween
- auto: C / UI

## Assembly-CSharp/KSP/UI/U5Util.cs
- L=57 kind=static class type=U5Util bases=- anc=- ns=KSP.UI
- unity(70): Transform=6, UnityEngine.UI=5, GameObject=2, LayerMask=1 | ksp: none | fan_in=6
- methods(4): ColorToHex, GetSimulatedScrollMultiplier, HexToColor, SetLayerRecursive
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIAvailability.cs
- L=9 kind=enum type=UIAvailability bases=- anc=- ns=KSP.UI
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIButtonToggle.cs
- L=102 kind=MonoBehaviour type=UIButtonToggle bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(79): UnityEngine.UI=11, Texture=2, MonoBehaviour=1, Lifecycle=1, GetComponent=1 | ksp: none | fan_in=21
- methods(3): Awake, OnClick, SetState
- auto: C / UI

## Assembly-CSharp/KSP/UI/UICameraBase.cs
- L=15 kind=MonoBehaviour type=UICameraBase bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(74): Camera=2, MonoBehaviour=1, GetComponent=1 | ksp: none | fan_in=3
- methods(1): Reset
- auto: C / UI

## Assembly-CSharp/KSP/UI/UICanvasPrefab.cs
- L=11 kind=MonoBehaviour type=UICanvasPrefab bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(55): UnityEngine.UI=1, MonoBehaviour=1 | ksp: none | fan_in=9
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/UICanvasPrefabSpawner.cs
- L=37 kind=MonoBehaviour type=UICanvasPrefabSpawner bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(64): MonoBehaviour=1, Lifecycle=1, GameObject=1 | ksp: none | fan_in=0
- methods(1): Awake
- auto: C / UI

## Assembly-CSharp/KSP/UI/UICascadingList.cs
- L=205 kind=MonoBehaviour type=UICascadingList bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(58): UnityEngine.UI=6, Transform=4, GetComponent=4, MonoBehaviour=1, EventSystems=1 | ksp: none | fan_in=4
- methods(11): Add, AddCascadingItem, AddSegment, AddSegmentBody, AddSegmentFooter, AddSegmentHeader, ClearList, RemoveCascadingItem, SegmentHeaderButtonInput, UpdateBodiesCallback, UpdateCascadingItem
- auto: C / UI

## Assembly-CSharp/KSP/UI/UICraftSaveFlyoutController.cs
- L=308 kind=MonoBehaviour type=UICraftSaveFlyoutController bases=UIHoverSlidePanel anc=UIHoverSlidePanel/MonoBehaviour ns=KSP.UI
- unity(70): GameObject=8, Coroutine=6, Lifecycle=5, UnityEngine.UI=4, Transform=4, SetActive=4 | ksp: MissionSystem=2, Localizer=2, HighLogic=2, GameEvents=2, Game=2 | fan_in=2
- methods(19): CloseFlyout, CreateFolderSaveItem, GetSubdirectoryList, OnDestroy, OnDisable, OnEnable, OnPointerEnter, OnPointerExit, OpenCraftFolderBrowser, ResetItems, SaveFolderSelectable_onPointerEnter, SaveFolderSelectable_onPointerExit, SetupSaveFolderChoices, SetupToggleGroup, Start, Update, UpdateFolderDisplay, UpdateSaveLocationForMissions, UpdateSelectionRadioButtons
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIDialogCamera.cs
- L=24 kind=MonoBehaviour type=UIDialogCamera bases=UICameraBase anc=UICameraBase/MonoBehaviour ns=KSP.UI
- unity(72): Lifecycle=2, Camera=2 | ksp: none | fan_in=0
- methods(2): Awake, OnDestroy
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIDragAndDropController.cs
- L=61 kind=MonoBehaviour type=UIDragAndDropController bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(86): UnityEngine.UI=5, Transform=3, Lifecycle=2, GetComponent=2, GameObject=2, SetActive=1 | ksp: none | fan_in=2
- methods(4): Awake, OnDestroy, Register, Unregister
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIDragPanel.cs
- L=139 kind=MonoBehaviour type=UIDragPanel bases=DragHandler anc=DragHandler/MonoBehaviour ns=KSP.UI
- unity(78): EventSystems=7, Transform=6, UnityEngine.UI=4, Input.=2, GameObject=2, Instantiate/Destroy=1 | ksp: InputLockManager=2 | fan_in=4
- methods(3): OnBeginDrag, OnDrag, OnEndDrag
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIExtensions.cs
- L=70 kind=static class type=UIExtensions bases=- anc=- ns=KSP.UI
- unity(72): Transform=9, UnityEngine.UI=5, GameObject=3, EventSystems=2 | ksp: none | fan_in=0
- methods(9): AddEvent, Clear, ClearChildren, ClearChildrenImmediate, CopyValues, Lock, OnEventDelegate, SetLocalPositionZ, Unlock
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIHoverPanel.cs
- L=79 kind=MonoBehaviour type=UIHoverPanel bases=PointerEnterExitHandler anc=PointerEnterExitHandler/MonoBehaviour ns=KSP.UI
- unity(74): GameObject=5, Texture=2, EventSystems=2, UnityEngine.UI=1, SetActive=1, Lifecycle=1 | ksp: none | fan_in=3
- methods(8): Awake, DisableHover, EnableHover, OnPointerEnter, OnPointerExit, PointerEnter, PointerExit, SetStateObjects
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIHoverSlidePanel.cs
- L=115 kind=MonoBehaviour type=UIHoverSlidePanel bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler anc=MonoBehaviour ns=KSP.UI
- unity(80): Coroutine=7, SetActive=4, EventSystems=4, UnityEngine.UI=2, GameObject=2, Time.=1 | ksp: none | fan_in=5
- methods(5): MoveToState, OnPointerEnter, OnPointerExit, Reset, Start
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIHoverText.cs
- L=71 kind=UIBehaviour type=UIHoverText bases=XHoverable anc=XHoverable/UIBehaviour ns=KSP.UI
- unity(79): UnityEngine.UI=3, TMPro=2, Lifecycle=2, EventSystems=2, GetComponent=1 | ksp: none | fan_in=5
- methods(6): OnDisable, OnHover, OnUnhover, Start, UpdateTargetControl, UpdateText
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIHoverToggler.cs
- L=71 kind=UIBehaviour type=UIHoverToggler bases=XSelectable anc=XSelectable/UIBehaviour ns=KSP.UI
- unity(75): EventSystems=3, SetActive=2, Lifecycle=2, UnityEngine.UI=1, GetComponent=1, GameObject=1 | ksp: none | fan_in=1
- methods(6): OnDeselect, OnDisable, OnHover, OnUnhover, SetText, Start
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIInputExtraMouseButtons.cs
- L=60 kind=MonoBehaviour type=UIInputExtraMouseButtons bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(83): UnityEngine.UI=3, Input.=3, Lifecycle=2, EventSystems=2, MonoBehaviour=1, GetComponent=1 | ksp: none | fan_in=0
- methods(4): Awake, OnMouseEnter, OnMouseExit, Update
- auto: C / UI

## Assembly-CSharp/KSP/UI/UILaunchsiteController.cs
- L=180 kind=MonoBehaviour type=UILaunchsiteController bases=UIHoverSlidePanel anc=UIHoverSlidePanel/MonoBehaviour ns=KSP.UI
- unity(69): Coroutine=6, Lifecycle=4, EventSystems=4, UnityEngine.UI=2, Transform=2, GameObject=2 | ksp: GameEvents=2 | fan_in=1
- methods(13): OnDestroy, OnDisable, OnEnable, OnPointerEnter, OnPointerExit, Start, addlaunchPadItem, launchSitesSelectable_onPointerEnter, launchSitesSelectable_onPointerExit, resetItems, setSelectedItem, setupItems, setupToggleGroup
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIList.cs
- L=398 kind=MonoBehaviour type=UIList bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(67): Transform=26, SetActive=7, GameObject=7, UnityEngine.UI=5, MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=31
- methods(18): AddItem, Awake, Clear, ClearUI, Contains, GetDropItemIndex, GetEnumerator, GetIndex, GetUiListItems, GetUilistItemAt, InsertItem, Refresh, RemoveItem, RemoveItemAndMove, SetActive, SetAnchor, Sort, SwapItems
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIListData.cs
- L=15 kind=class type=UIListData bases=- anc=- ns=KSP.UI
- unity(0): none | ksp: none | fan_in=3
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIListDropArea.cs
- L=61 kind=MonoBehaviour type=UIListDropArea bases=DropHandler anc=DropHandler/MonoBehaviour ns=KSP.UI
- unity(57): GetComponent=3, Lifecycle=1, EventSystems=1 | ksp: none | fan_in=0
- methods(2): Awake, OnDrop
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIListItem.cs
- L=74 kind=MonoBehaviour type=UIListItem bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(73): UnityEngine.UI=3, TMPro=3, GetComponent=3, MonoBehaviour=1 | ksp: none | fan_in=43
- methods(3): GetButtonElement, GetStateButtonElement, GetTextElement
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIListItem_spacer.cs
- L=9 kind=MonoBehaviour type=UIListItem_spacer bases=UIListItem anc=UIListItem/MonoBehaviour ns=KSP.UI
- unity(44): UnityEngine.UI=1 | ksp: none | fan_in=3
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIListMover.cs
- L=71 kind=MonoBehaviour type=UIListMover bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(40): UnityEngine.UI=1, MonoBehaviour=1 | ksp: none | fan_in=0
- methods(1): SetAnchor
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIListSorter.cs
- L=117 kind=MonoBehaviour type=UIListSorter bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(27): MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=5
- methods(6): AddOnSortCallback, ClickButton, OnSort, RemoveOnSortCallback, SetSortingMode, Start
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIListToggleController.cs
- L=34 kind=MonoBehaviour type=UIListToggleController bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(56): SetActive=1, MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=1
- methods(3): ActivateList, RefreshCurrent, Start
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIMainCamera.cs
- L=34 kind=MonoBehaviour type=UIMainCamera bases=UICameraBase anc=UICameraBase/MonoBehaviour ns=KSP.UI
- unity(75): Camera=3, Lifecycle=2 | ksp: none | fan_in=7
- methods(2): Awake, OnDestroy
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIMasterController.cs
- L=792 kind=MonoBehaviour type=UIMasterController bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(75): UnityEngine.UI=53, GameObject=21, Transform=8, Instantiate/Destroy=7, Screen/Cursor=5, Lifecycle=5 | ksp: GameEvents=14, GameSettings=5, InputLockManager=3 | fan_in=74
- methods(45): AddCanvas, AnyCornerOffScreen, Awake, CanSpawnCanvasPrefab, CheckScreenResolution, ClampToScreen, ClampToWindow, CutToScreen, CutToWindow, DespawnTooltip, DestroyCurrentTooltip, DragTooltip, FocusModalDialog, GetCanvasWrapper, GetMaxSuggestedUIScale, HideUI, LateUpdate, OnDestroy, OnHideUI, OnSceneChange, OnShowUI, PinTooltip, RegisterModalDialog, RegisterNonModalDialog ...
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIOnClick.cs
- L=48 kind=MonoBehaviour type=UIOnClick bases=MonoBehaviour,IPointerClickHandler,IEventSystemHandler anc=MonoBehaviour ns=KSP.UI
- unity(82): EventSystems=4, UnityEngine.UI=2, MonoBehaviour=1, Lifecycle=1, GetComponent=1 | ksp: none | fan_in=2
- methods(2): Awake, OnPointerClick
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIOnHover.cs
- L=23 kind=MonoBehaviour type=UIOnHover bases=MonoBehaviour,IPointerEnterHandler,IEventSystemHandler anc=MonoBehaviour ns=KSP.UI
- unity(93): UnityEngine.UI=4, EventSystems=4, MonoBehaviour=1 | ksp: none | fan_in=1
- methods(2): OnPointerEnter, OnPointerExit
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIPanelTransition.cs
- L=238 kind=MonoBehaviour type=UIPanelTransition bases=UITransitionBase anc=UITransitionBase/MonoBehaviour ns=KSP.UI
- unity(52): Coroutine=5, SetActive=4, Lifecycle=2, GameObject=2, UnityEngine.UI=1, Time.=1 | ksp: none | fan_in=13
- methods(7): Awake, GetState, Reset, Start, Transition, TransitionImmediate, TransitionState
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIPanelTransitionManager.cs
- L=116 kind=MonoBehaviour type=UIPanelTransitionManager bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(41): Coroutine=4, MonoBehaviour=1 | ksp: none | fan_in=1
- methods(4): AnyPanelsTransitioning, BringIn, Dismiss, PanelTransitionIn
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIPanelTransitionRTScaler.cs
- L=93 kind=MonoBehaviour type=UIPanelTransitionRTScaler bases=UITransitionBase anc=UITransitionBase/MonoBehaviour ns=KSP.UI
- unity(57): Coroutine=4, Time.=1, Lifecycle=1 | ksp: none | fan_in=0
- methods(4): Awake, GetState, Transition, TransitionState
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIPanelTransitionToggle.cs
- L=45 kind=MonoBehaviour type=UIPanelTransitionToggle bases=UIPanelTransition anc=UIPanelTransition/UITransitionBase/MonoBehaviour ns=KSP.UI
- unity(53): Lifecycle=1, GameObject=1 | ksp: none | fan_in=4
- methods(5): Collapse, CollapseImmediate, Expand, ExpandImmediate, OnValidate
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIPanelTweener.cs
- L=90 kind=MonoBehaviour type=UIPanelTweener bases=UITransitionBase anc=UITransitionBase/MonoBehaviour ns=KSP.UI
- unity(68): Coroutine=4, UnityEngine.UI=3, GetComponent=2, Time.=1, Lifecycle=1 | ksp: none | fan_in=1
- methods(6): Awake, OnTransitionComplete, Reset, SetInteractable, Transition, TransitionState
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIPlanetariumDateTime.cs
- L=54 kind=MonoBehaviour type=UIPlanetariumDateTime bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(61): Lifecycle=3, TMPro=1, MonoBehaviour=1 | ksp: GameEvents=4, Planetarium=2 | fan_in=0
- methods(6): OnDestroy, SetTime, Start, Update, onGamePause, onGameUnPause
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIPrefabSpawner.cs
- L=70 kind=MonoBehaviour type=UIPrefabSpawner bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(67): UnityEngine.UI=3, MonoBehaviour=1, Lifecycle=1, Instantiate/Destroy=1, Coroutine=1 | ksp: none | fan_in=0
- methods(3): Awake, Spawn, Start
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIRadioButton.cs
- L=295 kind=MonoBehaviour type=UIRadioButton bases=MonoBehaviour,IPointerClickHandler,IEventSystemHandler anc=MonoBehaviour ns=KSP.UI
- unity(83): EventSystems=24, UnityEngine.UI=21, Lifecycle=2, GetComponent=2, GameObject=2, TMPro=1 | ksp: GameEvents=2 | fan_in=32
- methods(7): Awake, MenuNavigationInputListener, OnDestroy, PopOtherButtonsInGroup, SetGroup, SetState, ToggleState
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIRadioButtonColorChanger.cs
- L=161 kind=MonoBehaviour type=UIRadioButtonColorChanger bases=MonoBehaviour,IPointerDownHandler,IEventSystemHandler anc=MonoBehaviour ns=KSP.UI
- unity(77): UnityEngine.UI=10, EventSystems=8, Lifecycle=3, MonoBehaviour=1, GetComponent=1 | ksp: none | fan_in=0
- methods(10): Awake, OnDestroy, OnFalse, OnPointerDown, OnPointerEnter, OnPointerExit, OnTrue, PopOtherButtonsInGroup, SetGroup, Start
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIRadioButtonStateChanger.cs
- L=60 kind=MonoBehaviour type=UIRadioButtonStateChanger bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(47): MonoBehaviour=1, Lifecycle=1, GetComponent=1 | ksp: none | fan_in=1
- methods(2): Awake, SetState
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIRectScaler.cs
- L=114 kind=MonoBehaviour type=UIRectScaler bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(65): Lifecycle=4, UnityEngine.UI=3, Screen/Cursor=2, GetComponent=2, MonoBehaviour=1 | ksp: GameSettings=4, GameEvents=2 | fan_in=1
- methods(8): Awake, OnDestroy, OverFlowProtector, Set, SetGameSettings, SetGameSettingsReciprocal, Start, Update
- auto: C / UI

## Assembly-CSharp/KSP/UI/UISelectableGridLayoutGroup.cs
- L=224 kind=MonoBehaviour type=UISelectableGridLayoutGroup bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(63): Transform=8, Instantiate/Destroy=5, GameObject=4, UnityEngine.UI=3, GetComponent=2, MonoBehaviour=1 | ksp: none | fan_in=2
- methods(10): AddItem, Awake, Clear, ClickItem, CreateItem, InsertItem, OnGridSelectItem, RemoveItem, Select, SelectSet
- auto: C / UI

## Assembly-CSharp/KSP/UI/UISelectableGridLayoutGroupItem.cs
- L=51 kind=MonoBehaviour type=UISelectableGridLayoutGroupItem bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(81): UnityEngine.UI=6, Transform=1, MonoBehaviour=1, GetComponent=1 | ksp: none | fan_in=11
- methods(3): Deselect, Select, Setup
- auto: C / UI

## Assembly-CSharp/KSP/UI/UISelectableList.cs
- L=165 kind=MonoBehaviour type=UISelectableList bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(68): UnityEngine.UI=8, Instantiate/Destroy=4, GameObject=2, Transform=1, MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=3
- methods(10): AddItem, Clear, ClickItem, CreateAndInsertItem, CreateItem, InsertItem, OnDestroy, RemoveItem, Select, UpdateObjects
- auto: C / UI

## Assembly-CSharp/KSP/UI/UISelectableListItem.cs
- L=24 kind=MonoBehaviour type=UISelectableListItem bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(36): MonoBehaviour=1 | ksp: none | fan_in=3
- methods(3): Deselect, Select, SetupListItem
- auto: C / UI

## Assembly-CSharp/KSP/UI/UISelectableListItem_ColorButton.cs
- L=81 kind=MonoBehaviour type=UISelectableListItem_ColorButton bases=UISelectableListItem anc=UISelectableListItem/MonoBehaviour ns=KSP.UI
- unity(75): UnityEngine.UI=7, GetComponent=2, Lifecycle=1 | ksp: none | fan_in=0
- methods(5): Awake, Deselect, Select, SetState, SetupListItem
- auto: C / UI

## Assembly-CSharp/KSP/UI/UISelectableListItem_StateButton.cs
- L=90 kind=MonoBehaviour type=UISelectableListItem_StateButton bases=UISelectableListItem anc=UISelectableListItem/MonoBehaviour ns=KSP.UI
- unity(83): UnityEngine.UI=9, Texture=4, GetComponent=3, Lifecycle=1 | ksp: none | fan_in=0
- methods(5): Awake, Deselect, Select, SetState, SetupListItem
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIShipDescriptionHoverSlidePanel.cs
- L=92 kind=MonoBehaviour type=UIShipDescriptionHoverSlidePanel bases=UIHoverSlidePanel anc=UIHoverSlidePanel/MonoBehaviour ns=KSP.UI
- unity(66): EventSystems=4, TMPro=2, Coroutine=2, Lifecycle=1 | ksp: EditorLogic=3 | fan_in=0
- methods(7): InputFieldSelectable_onPointerEnter, InputFieldSelectable_onPointerExit, OnEndEdit, OnPointerEnter, OnPointerExit, Start, UpdateHoverState
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIStateButton.cs
- L=138 kind=MonoBehaviour type=UIStateButton bases=MonoBehaviour,IPointerClickHandler,IEventSystemHandler anc=MonoBehaviour ns=KSP.UI
- unity(88): UnityEngine.UI=19, EventSystems=7, GetComponent=3, MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=17
- methods(5): Awake, Enable, Reset, SetState, ToggleState
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIStateImage.cs
- L=76 kind=MonoBehaviour type=UIStateImage bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(68): UnityEngine.UI=2, Texture=2, Lifecycle=2, MonoBehaviour=1, GetComponent=1 | ksp: GameEvents=4, TimeWarp=1 | fan_in=20
- methods(6): Awake, OnDestroy, Reset, SetState, onGamePause, onGameUnPause
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIStatePanel.cs
- L=113 kind=MonoBehaviour type=UIStatePanel bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(57): SetActive=2, GameObject=2, UnityEngine.UI=1, TMPro=1, MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=5
- methods(6): Awake, Disable, Enable, SetState, SetText, SetTextOverflowMode
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIStateRawImage.cs
- L=53 kind=MonoBehaviour type=UIStateRawImage bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(70): UnityEngine.UI=2, Texture=1, MonoBehaviour=1, Lifecycle=1, GetComponent=1 | ksp: none | fan_in=4
- methods(3): Awake, Reset, SetState
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIStateText.cs
- L=56 kind=MonoBehaviour type=UIStateText bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(64): TMPro=2, MonoBehaviour=1, Lifecycle=1, GetComponent=1 | ksp: none | fan_in=2
- methods(3): Awake, Reset, SetState
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIStateToggleButton.cs
- L=191 kind=MonoBehaviour type=UIStateToggleButton bases=MonoBehaviour,IPointerClickHandler,IEventSystemHandler anc=MonoBehaviour ns=KSP.UI
- unity(86): UnityEngine.UI=27, Texture=4, EventSystems=4, Lifecycle=2, GetComponent=2, MonoBehaviour=1 | ksp: none | fan_in=2
- methods(6): Awake, OnPointerClick, SetState, Setup, Start, ToggleState
- auto: C / UI

## Assembly-CSharp/KSP/UI/UITextButton.cs
- L=135 kind=MonoBehaviour type=UITextButton bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(65): EventSystems=5, UnityEngine.UI=2, Lifecycle=2, GetComponent=2, MonoBehaviour=1 | ksp: none | fan_in=0
- methods(8): Awake, OnMouseClick, OnMouseDown, OnMouseEnter, OnMouseExit, OnMouseUp, SetState, Start
- auto: C / UI

## Assembly-CSharp/KSP/UI/UITimeWarpController.cs
- L=169 kind=MonoBehaviour type=UITimeWarpController bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(57): SetActive=4, GameObject=4, UnityEngine.UI=2, Lifecycle=2, MonoBehaviour=1, GetComponent=1 | ksp: TimeWarp=20, Localizer=4, GameEvents=2, InputLockManager=1, HighLogic=1 | fan_in=0
- methods(12): CanSetHighRate, ClickTimewarp0, ClickTimewarp1, ClickTimewarp2, ClickTimewarp3, ClickTimewarp4, ClickTimewarp5, ClickTimewarp6, ClickTimewarp7, OnDestroy, OnTimeWarpRateChanged, Start
- auto: C / UI

## Assembly-CSharp/KSP/UI/UITimeWarpHoverController.cs
- L=48 kind=MonoBehaviour type=UITimeWarpHoverController bases=UIHoverSlidePanel anc=UIHoverSlidePanel/MonoBehaviour ns=KSP.UI
- unity(62): Lifecycle=2, Coroutine=2, Transform=1 | ksp: TimeWarp=3 | fan_in=0
- methods(2): LateUpdate, Update
- auto: C / UI

## Assembly-CSharp/KSP/UI/UITimeWarpScrubber.cs
- L=50 kind=MonoBehaviour type=UITimeWarpScrubber bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(75): EventSystems=2, UnityEngine.UI=1, MonoBehaviour=1, Lifecycle=1, GetComponent=1, GameObject=1 | ksp: Planetarium=4, TimeWarp=3 | fan_in=0
- methods(3): OnPointerDown, OnPointerUp, Start
- auto: C / UI

## Assembly-CSharp/KSP/UI/UITogglePanel.cs
- L=44 kind=MonoBehaviour type=UITogglePanel bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(82): SetActive=4, GameObject=2, EventSystems=2, MonoBehaviour=1, Lifecycle=1 | ksp: none | fan_in=0
- methods(3): Awake, Hover, HoverOut
- auto: C / UI

## Assembly-CSharp/KSP/UI/UITransitionBase.cs
- L=77 kind=MonoBehaviour type=UITransitionBase bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(69): UnityEngine.UI=4, Time.=1, MonoBehaviour=1, GetComponent=1 | ksp: none | fan_in=3
- methods(6): Lerp_Pre, Lerp_Transition, Linear_Pre, Linear_Transition, SetInteractable, SetMethod
- auto: C / UI

## Assembly-CSharp/KSP/UI/UITransparencyController.cs
- L=153 kind=MonoBehaviour type=UITransparencyController bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(72): Shader=8, SceneManager=3, Lifecycle=2, UnityEngine.UI=1, MonoBehaviour=1, Material=1 | ksp: HighLogic=2, GameSettings=2, GameEvents=2 | fan_in=0
- methods(8): OnDestroy, OnLevelLoaded, OnSceneLoaded, SetOpacity, Setup, Start, UpdateOpacity, onSettingsApplied
- auto: C / UI

## Assembly-CSharp/KSP/UI/UITreeView.cs
- L=344 kind=MonoBehaviour type=UITreeView bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(55): UnityEngine.UI=9, Transform=4, Coroutine=3, Instantiate/Destroy=2, GetComponent=2, Time.=1 | ksp: Localizer=4 | fan_in=3
- methods(14): AddItem, ClearItems, ClearItemsRecursive, FindItemByURL, GetItem, OnSetExpand, OnToggleExpand, Refresh, ScrollRectToLevelSelection, SelectFirstItem, SelectItem, SetupItemList, UnselectAllRecursive, Update
- auto: C / UI

## Assembly-CSharp/KSP/UI/UITreeViewItem.cs
- L=64 kind=MonoBehaviour type=UITreeViewItem bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(81): UnityEngine.UI=5, Texture=2, TMPro=1, MonoBehaviour=1, Instantiate/Destroy=1, GameObject=1 | ksp: none | fan_in=1
- methods(1): Setup
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIVectorCamera.cs
- L=24 kind=MonoBehaviour type=UIVectorCamera bases=UICameraBase anc=UICameraBase/MonoBehaviour ns=KSP.UI
- unity(72): Lifecycle=2, Camera=2 | ksp: none | fan_in=0
- methods(2): Awake, OnDestroy
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIWarpToNextMorning.cs
- L=166 kind=MonoBehaviour type=UIWarpToNextMorning bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(34): Lifecycle=2, UnityEngine.UI=1, MonoBehaviour=1 | ksp: Vessel=14, FlightGlobals=12, HighLogic=5, TimeWarp=4, CelestialBody=3 | fan_in=0
- methods(5): CalculateTimeToDaylightFromVessel, Setup, Start, Update, WarpToMorning
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIWindow.cs
- L=223 kind=MonoBehaviour type=UIWindow bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(58): UnityEngine.UI=6, EventSystems=4, Lifecycle=3, Transform=1, MonoBehaviour=1 | ksp: GameEvents=2 | fan_in=2
- methods(10): Awake, ClampToScreen, CutToScreen, MoveRectTransform, MoveWindow, OnDestroy, OnScreenResolutionModified, ResizeRectTransform, ResizeWindow, Start
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIWindowArea.cs
- L=97 kind=MonoBehaviour type=UIWindowArea bases=MonoBehaviour,IBeginDragHandler,IEventSystemHandler anc=MonoBehaviour ns=KSP.UI
- unity(72): EventSystems=6, Screen/Cursor=2, UnityEngine.UI=1, MonoBehaviour=1, Lifecycle=1, GetComponent=1 | ksp: none | fan_in=0
- methods(5): OnBeginDrag, OnDrag, OnMove, OnResize, Start
- auto: C / UI

## Assembly-CSharp/KSP/UI/UIWorldPointer.cs
- L=322 kind=MonoBehaviour type=UIWorldPointer bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(74): UnityEngine.UI=6, GameObject=5, Transform=4, Texture=4, Material=4, Lifecycle=4 | ksp: none | fan_in=2
- methods(16): ChamferCorner, Create, CreateLine, GetAnchoredPoint, GetClosestCornerOrMidpoint, GetTexture, GetUIPoint, LateUpdate, OnDestroy, OnDisable, OnEnable, Setup, Terminate, UpdateDirect, UpdateKinked, UpdateLine
- auto: C / UI

## Assembly-CSharp/KSP/UI/Util/CanvasTools.cs
- L=8 kind=MonoBehaviour type=CanvasTools bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Util
- unity(36): MonoBehaviour=1 | ksp: none | fan_in=0
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/Util/ImgText.cs
- L=39 kind=MonoBehaviour type=ImgText bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Util
- unity(72): UnityEngine.UI=1, Texture=1, TMPro=1, MonoBehaviour=1 | ksp: none | fan_in=5
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/Util/RectContainmentDetector.cs
- L=51 kind=MonoBehaviour type=RectContainmentDetector bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI.Util
- unity(80): UnityEngine.UI=3, Lifecycle=2, Camera=2, MonoBehaviour=1, GetComponent=1 | ksp: none | fan_in=1
- methods(2): Start, Update
- auto: C / UI

## Assembly-CSharp/KSP/UI/Util/RectUtil.cs
- L=74 kind=static class type=RectUtil bases=- anc=- ns=KSP.UI.Util
- unity(73): UnityEngine.UI=12, Camera=3, Transform=1 | ksp: none | fan_in=3
- methods(2): GetRectContainment, WorldToUISpacePos
- auto: C / UI

## Assembly-CSharp/KSP/UI/Util/UISkinDefSO.cs
- L=12 kind=ScriptableObject type=UISkinDefSO bases=ScriptableObject anc=ScriptableObject ns=KSP.UI.Util
- unity(29): ScriptableObject=1 | ksp: none | fan_in=10
- methods(0): 
- auto: C / UI

## Assembly-CSharp/KSP/UI/VesselListItem.cs
- L=41 kind=MonoBehaviour type=VesselListItem bases=MonoBehaviour anc=MonoBehaviour ns=KSP.UI
- unity(81): TMPro=6, MonoBehaviour=1 | ksp: Localizer=3 | fan_in=2
- methods(1): Setup
- auto: C / UI

## Assembly-CSharp/CommNet/CommLink.cs
- L=107 kind=class type=CommLink bases=Link<CommNetwork,CommNode,CommLink,CommPath> anc=Link ns=CommNet
- unity(0): none | ksp: none | fan_in=5
- methods(7): GetBestSignal, GetSignalStrength, Set, SetSignalStrength, ToString, Update, ZeroSignalStrength
- auto: A / CommNet

## Assembly-CSharp/CommNet/CommNetBody.cs
- L=44 kind=MonoBehaviour type=CommNetBody bases=MonoBehaviour anc=MonoBehaviour ns=CommNet
- unity(63): Lifecycle=2, Transform=1, MonoBehaviour=1, GetComponent=1 | ksp: HighLogic=3, GameEvents=2, CelestialBody=2 | fan_in=0
- methods(5): CreateOccluder, OnDestroy, OnNetworkInitialized, OnNetworkPreUpdate, Start
- auto: B / CommNet

## Assembly-CSharp/CommNet/CommNetHome.cs
- L=106 kind=MonoBehaviour type=CommNetHome bases=MonoBehaviour anc=MonoBehaviour ns=CommNet
- unity(55): Transform=4, Lifecycle=3, MonoBehaviour=1, GetComponent=1 | ksp: HighLogic=3, GameEvents=2, CelestialBody=2 | fan_in=1
- methods(6): CreateNode, OnDestroy, OnNetworkInitialized, OnNetworkPreUpdate, Start, Update
- auto: B / CommNet

## Assembly-CSharp/CommNet/CommNetNetwork.cs
- L=167 kind=MonoBehaviour type=CommNetNetwork bases=MonoBehaviour anc=MonoBehaviour ns=CommNet
- unity(41): Lifecycle=3, Time.=1, MonoBehaviour=1, Instantiate/Destroy=1 | ksp: GameEvents=5, FlightGlobals=2, Vessel=1, HighLogic=1 | fan_in=8
- methods(10): Add, Awake, DebugInfo, OnDestroy, OnMapFocusChange, QueueRebuild, Remove, Reset, ResetNetwork, Update
- auto: B / CommNet

## Assembly-CSharp/CommNet/CommNetNode.cs
- L=64 kind=MonoBehaviour type=CommNetNode bases=MonoBehaviour anc=MonoBehaviour ns=CommNet
- unity(48): Lifecycle=2, Transform=1, MonoBehaviour=1 | ksp: GameEvents=2 | fan_in=0
- methods(5): OnDestroy, OnNetworkInitialized, OnNetworkPostUpdate, OnNetworkPreUpdate, Start
- auto: B / CommNet

## Assembly-CSharp/CommNet/CommNetParams.cs
- L=74 kind=class type=CommNetParams bases=GameParameters.CustomParameterNode anc=CustomParameterNode/ParameterNode/IConfigNode ns=CommNet
- unity(0): none | ksp: Localizer=2 | fan_in=5
- methods(2): Interactible, SetDifficultyPreset
- auto: A / CommNet

## Assembly-CSharp/CommNet/CommNetScenario.cs
- L=103 kind=ScenarioModule type=CommNetScenario bases=ScenarioModule anc=ScenarioModule/MonoBehaviour ns=CommNet
- unity(62): Instantiate/Destroy=4, GameObject=3, Lifecycle=2, GetComponent=2 | ksp: HighLogic=2, ConfigNode=2, ScenarioModule=1 | fan_in=18
- methods(5): OnAwake, OnDestroy, OnLoad, OnSave, Start
- auto: B / CommNet

## Assembly-CSharp/CommNet/CommNetUI.cs
- L=479 kind=MonoBehaviour type=CommNetUI bases=MonoBehaviour anc=MonoBehaviour ns=CommNet
- unity(56): Shader=8, Material=5, Texture=4, Instantiate/Destroy=4, Lifecycle=3, Resources.=2 | ksp: Localizer=6, GameEvents=6, FlightGlobals=4, Vessel=3, MapView=2 | fan_in=5
- methods(15): Awake, ClampAndSetMode, CreateLine, Hide, NextMode, OnDestroy, OnMapFocusChange, PreviousMode, ResetMode, SetDisplayVessel, Show, Start, SwitchMode, UpdateDisplay, UpdateLine
- auto: C / CommNet

## Assembly-CSharp/CommNet/CommNetUIModeButton.cs
- L=79 kind=MonoBehaviour type=CommNetUIModeButton bases=MonoBehaviour,IPointerClickHandler,IEventSystemHandler anc=MonoBehaviour ns=CommNet
- unity(81): Lifecycle=5, EventSystems=5, SetActive=2, GameObject=2, UnityEngine.UI=1, MonoBehaviour=1 | ksp: GameEvents=2, Localizer=1 | fan_in=1
- methods(8): Awake, OnDestroy, OnEnable, OnNetworkInitialized, OnPointerClick, Start, Update, UpdateUI
- auto: C / CommNet

## Assembly-CSharp/CommNet/CommNetVessel.cs
- L=599 kind=VesselModule type=CommNetVessel bases=VesselModule anc=VesselModule/MonoBehaviour ns=CommNet
- unity(24): GameObject=3, Lifecycle=2, Transform=1, Instantiate/Destroy=1 | ksp: PhysicsGlobals=12, GameEvents=6, Vessel=5, HighLogic=4, Part=3 | fan_in=7
- methods(18): CalculatePlasmaMult, CreateControlConnection, FindCommandSources, GetBestTransmitter, GetControlLevel, GetSignalStrengthModifier, OnAwake, OnDestroy, OnMapFocusChange, OnNetworkInitialized, OnNetworkPostUpdate, OnNetworkPreUpdate, OnStart, RegisterCommandSource, UnregisterCommandSource, Update, UpdateComm, UpdateControlState
- auto: B / CommNet

## Assembly-CSharp/CommNet/CommNetwork.cs
- L=459 kind=class type=CommNetwork bases=Net<CommNetwork,CommNode,CommLink,CommPath> anc=Net ns=CommNet
- unity(3): Raycast/Query=1 | ksp: Vector3d=4 | fan_in=6
- methods(16): Add, CreateDebug, CreateShortestPathTree, FindClosestControlSource, FindClosestWhere, FindHome, FindPath, IsControlSource, IsHome, Rebuild, Remove, SetNodeConnection, TestOcclusion, TryConnect, UpdateShortestPath, UpdateShortestWhere
- auto: B / CommNet

## Assembly-CSharp/CommNet/CommNode.cs
- L=147 kind=class type=CommNode bases=Node<CommNetwork,CommNode,CommLink,CommPath> anc=Node/Dictionary ns=CommNet
- unity(35): Transform=9 | ksp: Vector3d=3 | fan_in=9
- methods(5): GetSignalStrengthMultiplier, NetworkPostUpdate, NetworkPreUpdate, ToString, Update
- auto: B / CommNet

## Assembly-CSharp/CommNet/CommPath.cs
- L=71 kind=class type=CommPath bases=Path<CommNetwork,CommNode,CommLink,CommPath> anc=Path/List ns=CommNet
- unity(0): none | ksp: none | fan_in=8
- methods(4): Clear, IsLastHopHome, ToString, UpdateFromPath
- auto: A / CommNet

## Assembly-CSharp/CommNet/CommRangeModel.cs
- L=27 kind=class type=CommRangeModel bases=IRangeModel anc=IRangeModel ns=CommNet
- unity(0): none | ksp: none | fan_in=1
- methods(3): GetMaximumRange, GetNormalizedRange, InRange
- auto: A / CommNet

## Assembly-CSharp/CommNet/HopType.cs
- L=9 kind=enum type=HopType bases=- anc=- ns=CommNet
- unity(0): none | ksp: none | fan_in=4
- methods(0): 
- auto: A / CommNet

## Assembly-CSharp/CommNet/ICommNetControlSource.cs
- L=13 kind=interface type=ICommNetControlSource bases=- anc=- ns=CommNet
- unity(0): none | ksp: none | fan_in=4
- methods(0): 
- auto: A / CommNet

## Assembly-CSharp/CommNet/IRangeModel.cs
- L=11 kind=interface type=IRangeModel bases=- anc=- ns=CommNet
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / CommNet

## Assembly-CSharp/CommNet/IRelayEnabler.cs
- L=9 kind=interface type=IRelayEnabler bases=- anc=- ns=CommNet
- unity(0): none | ksp: ProtoPartModuleSnapshot=1 | fan_in=2
- methods(0): 
- auto: A / CommNet

## Assembly-CSharp/CommNet/ModuleProbeControlPoint.cs
- L=93 kind=PartModule type=ModuleProbeControlPoint bases=PartModule anc=PartModule/MonoBehaviour ns=CommNet
- unity(15): none | ksp: Localizer=4, KSPField=2, ProtoPartModuleSnapshot=1, PartModule=1 | fan_in=3
- methods(8): CacheLocalStrings, CanControl, CanControlUnloaded, GetInfo, GetModuleDisplayName, IsProbeControlPointBroken, OnModuleAdjusterAdded, OnModuleAdjusterRemoved
- auto: A / CommNet

## Assembly-CSharp/CommNet/ModuleRelayEnabler.cs
- L=99 kind=PartModule type=ModuleRelayEnabler bases=PartModule,IResourceConsumer,IRelayEnabler anc=PartModule/MonoBehaviour ns=CommNet
- unity(22): Lifecycle=1 | ksp: PartResourceDefinition=3, KSPField=2, ProtoPartModuleSnapshot=1, PartModule=1, Localizer=1 | fan_in=0
- methods(8): CanRelay, CanRelayUnloaded, FixedUpdate, GetConsumedResources, GetInfo, OnAwake, OnSave, OnStart
- auto: A / CommNet

## Assembly-CSharp/CommNet/Network/Link.cs
- L=83 kind=class type=Link bases=- anc=- ns=CommNet.Network
- unity(10): Lifecycle=1 | ksp: Localizer=2 | fan_in=0
- methods(7): Contains, GetPoints, OnDestroy, OtherEnd, Set, ToString, Update
- auto: A / CommNet

## Assembly-CSharp/CommNet/Network/Net.cs
- L=419 kind=class type=Net bases=- anc=- ns=CommNet.Network
- unity(0): none | ksp: none | fan_in=4
- methods(19): Add, Connect, Contains, CreateShortestPathTree, Disconnect, FindClosestWhere, FindPath, GetLinkPoints, IncrementPathingID, PostUpdateNodes, PreUpdateNodes, Rebuild, Remove, SetNodeConnection, ToString, UpdateNetwork, UpdateOccluders, UpdateShortestPath, UpdateShortestWhere
- auto: A / CommNet

## Assembly-CSharp/CommNet/Network/Node.cs
- L=132 kind=class type=Node bases=Dictionary<_Data,_Link> anc=Dictionary ns=CommNet.Network
- unity(1): none | ksp: Vector3d=1, Localizer=1 | fan_in=0
- methods(6): GetLinkPoints, NetworkPostUpdate, NetworkPreUpdate, SetNet, SetOccluder, ToString
- auto: A / CommNet

## Assembly-CSharp/CommNet/Network/Path.cs
- L=185 kind=class type=Path bases=List<_Link>,IConfigNode anc=List ns=CommNet.Network
- unity(1): none | ksp: ConfigNode=2 | fan_in=0
- methods(8): CopyTo, Equals, GetHashCode, GetPoints, IsEmpty, Load, Save, UpdateFromPath
- auto: A / CommNet

## Assembly-CSharp/CommNet/NodeUtilities.cs
- L=26 kind=static class type=NodeUtilities bases=- anc=- ns=CommNet
- unity(0): none | ksp: none | fan_in=2
- methods(1): ConvertSignalStrength
- auto: A / CommNet

## Assembly-CSharp/CommNet/Occluder.cs
- L=22 kind=class type=Occluder bases=- anc=- ns=CommNet
- unity(40): Raycast/Query=1, Lifecycle=1 | ksp: Vector3d=4 | fan_in=6
- methods(3): InRange, Raycast, Update
- auto: B / CommNet

## Assembly-CSharp/CommNet/Occluders/OccluderHorizonCulling.cs
- L=107 kind=class type=OccluderHorizonCulling bases=Occluder anc=Occluder ns=CommNet.Occluders
- unity(48): Transform=9, Raycast/Query=1, Lifecycle=1, GetComponent=1 | ksp: Vector3d=6, QuaternionD=2, CelestialBody=2 | fan_in=1
- methods(3): InRange, Raycast, Update
- auto: B / CommNet

## Assembly-CSharp/CommNet/SignalStrength.cs
- L=11 kind=enum type=SignalStrength bases=- anc=- ns=CommNet
- unity(0): none | ksp: none | fan_in=12
- methods(0): 
- auto: A / CommNet

## Assembly-CSharp/CommNet/SignalStrengthColor.cs
- L=11 kind=enum type=SignalStrengthColor bases=- anc=- ns=CommNet
- unity(0): none | ksp: none | fan_in=1
- methods(0): 
- auto: A / CommNet

## Assembly-CSharp/CommNet/Tooltip_SignalStrength.cs
- L=13 kind=MonoBehaviour type=Tooltip_SignalStrength bases=Tooltip anc=Tooltip/MonoBehaviour ns=CommNet
- unity(59): UnityEngine.UI=1, TMPro=1 | ksp: none | fan_in=1
- methods(0): 
- auto: B / CommNet

## Assembly-CSharp/CommNet/Tooltip_SignalStrengthItem.cs
- L=22 kind=MonoBehaviour type=Tooltip_SignalStrengthItem bases=MonoBehaviour anc=MonoBehaviour ns=CommNet
- unity(55): TMPro=1, MonoBehaviour=1 | ksp: none | fan_in=1
- methods(1): Setup
- auto: B / CommNet

## Assembly-CSharp/CommNet/TooltipController_SignalStrength.cs
- L=172 kind=MonoBehaviour type=TooltipController_SignalStrength bases=TooltipController anc=TooltipController/MonoBehaviour ns=CommNet
- unity(40): Instantiate/Destroy=3, Transform=2, GameObject=2 | ksp: Localizer=6, FlightGlobals=4, Vessel=1, CommNetVessel=1 | fan_in=2
- methods(9): CacheLocalStrings, ClearList, OnNetworkUpdate, OnTooltipAboutToSpawn, OnTooltipDespawned, OnTooltipSpawned, OnTooltipUpdate, UpdateList, UpdateTooltip
- auto: B / CommNet

## Assembly-CSharp/CommNet/VesselControlState.cs
- L=17 kind=enum type=VesselControlState bases=- anc=- ns=CommNet
- unity(0): none | ksp: none | fan_in=8
- methods(0): 
- auto: A / CommNet

## Assembly-CSharp/Contracts/Agents/Agent.cs
- L=290 kind=class type=Agent bases=- anc=- ns=Contracts.Agents
- unity(34): Texture=6 | ksp: Contract=4, Localizer=3, GameDatabase=3, ConfigNode=1 | fan_in=10
- methods(6): CanProcessContract, GetMindsetString, HasExclusiveKeywords, LoadAgent, ProcessContract, ScoreAgentSuitability
- auto: B / Contracts

## Assembly-CSharp/Contracts/Agents/AgentList.cs
- L=164 kind=MonoBehaviour type=AgentList bases=MonoBehaviour anc=MonoBehaviour ns=Contracts.Agents
- unity(38): Lifecycle=3, MonoBehaviour=1 | ksp: Localizer=2, ConfigNode=2, GameDatabase=1, Contract=1, AvailablePart=1 | fan_in=8
- methods(9): Awake, GetAgent, GetAgentRandom, GetAgentbyTitle, GetPartManufacturer, GetSuitableAgentForContract, LoadAgents, OnDestroy, Start
- auto: B / Contracts

## Assembly-CSharp/Contracts/Agents/AgentMentality.cs
- L=202 kind=class type=AgentMentality bases=- anc=- ns=Contracts.Agents
- unity(9): none | ksp: Contract=10 | fan_in=20
- methods(17): AddMentality, CanProcessContract, FactorDeadline, FactorExpiry, FactorFundsAdvance, FactorFundsCompletion, FactorFundsFailure, FactorReputationCompletion, FactorReputationFailure, FactorScienceCompletion, GenerateMentalityTypes, GetDescription, GetDisplayName, GetMentalityType, OnAdded, ProcessContract, ScoreKeyword
- auto: A / Contracts

## Assembly-CSharp/Contracts/Agents/AgentStanding.cs
- L=44 kind=class type=AgentStanding bases=- anc=- ns=Contracts.Agents
- unity(27): none | ksp: none | fan_in=1
- methods(1): Link
- auto: A / Contracts

## Assembly-CSharp/Contracts/Agents/KeywordScore.cs
- L=12 kind=enum type=KeywordScore bases=- anc=- ns=Contracts.Agents
- unity(0): none | ksp: none | fan_in=20
- methods(0): 
- auto: A / Contracts

## Assembly-CSharp/Contracts/Agents/Mentalities/Commercial.cs
- L=40 kind=class type=Commercial bases=AgentMentality anc=AgentMentality ns=Contracts.Agents.Mentalities
- unity(9): none | ksp: Localizer=2, Contract=2 | fan_in=0
- methods(5): CanProcessContract, GetDescription, GetDisplayName, ProcessContract, ScoreKeyword
- auto: A / Contracts

## Assembly-CSharp/Contracts/Agents/Mentalities/Competitive.cs
- L=37 kind=class type=Competitive bases=AgentMentality anc=AgentMentality ns=Contracts.Agents.Mentalities
- unity(10): none | ksp: Contract=2, Localizer=1 | fan_in=0
- methods(5): CanProcessContract, GetDescription, GetDisplayName, ProcessContract, ScoreKeyword
- auto: A / Contracts

## Assembly-CSharp/Contracts/Agents/Mentalities/Conglomerate.cs
- L=41 kind=class type=Conglomerate bases=AgentMentality anc=AgentMentality ns=Contracts.Agents.Mentalities
- unity(9): none | ksp: Localizer=2, Contract=2 | fan_in=0
- methods(5): CanProcessContract, GetDescription, GetDisplayName, ProcessContract, ScoreKeyword
- auto: A / Contracts

## Assembly-CSharp/Contracts/Agents/Mentalities/Cooperative.cs
- L=37 kind=class type=Cooperative bases=AgentMentality anc=AgentMentality ns=Contracts.Agents.Mentalities
- unity(10): none | ksp: Contract=2, Localizer=1 | fan_in=0
- methods(5): CanProcessContract, GetDescription, GetDisplayName, ProcessContract, ScoreKeyword
- auto: A / Contracts

## Assembly-CSharp/Contracts/Agents/Mentalities/EasyGoing.cs
- L=39 kind=class type=EasyGoing bases=AgentMentality anc=AgentMentality ns=Contracts.Agents.Mentalities
- unity(9): none | ksp: Localizer=2, Contract=2 | fan_in=0
- methods(5): CanProcessContract, GetDescription, GetDisplayName, ProcessContract, ScoreKeyword
- auto: A / Contracts

## Assembly-CSharp/Contracts/Agents/Mentalities/Ecomental.cs
- L=37 kind=class type=Ecomental bases=AgentMentality anc=AgentMentality ns=Contracts.Agents.Mentalities
- unity(10): none | ksp: Localizer=2, Contract=2 | fan_in=0
- methods(5): CanProcessContract, GetDescription, GetDisplayName, ProcessContract, ScoreKeyword
- auto: A / Contracts

## Assembly-CSharp/Contracts/Agents/Mentalities/Economic.cs
- L=41 kind=class type=Economic bases=AgentMentality anc=AgentMentality ns=Contracts.Agents.Mentalities
- unity(9): none | ksp: Localizer=2, Contract=2 | fan_in=0
- methods(5): CanProcessContract, GetDescription, GetDisplayName, ProcessContract, ScoreKeyword
- auto: A / Contracts

## Assembly-CSharp/Contracts/Agents/Mentalities/Hasty.cs
- L=40 kind=class type=Hasty bases=AgentMentality anc=AgentMentality ns=Contracts.Agents.Mentalities
- unity(9): none | ksp: Localizer=2, Contract=2 | fan_in=0
- methods(5): CanProcessContract, GetDescription, GetDisplayName, ProcessContract, ScoreKeyword
- auto: A / Contracts

## Assembly-CSharp/Contracts/Agents/Mentalities/Industrial.cs
- L=37 kind=class type=Industrial bases=AgentMentality anc=AgentMentality ns=Contracts.Agents.Mentalities
- unity(10): none | ksp: Localizer=2, Contract=2 | fan_in=0
- methods(5): CanProcessContract, GetDescription, GetDisplayName, ProcessContract, ScoreKeyword
- auto: A / Contracts

## Assembly-CSharp/Contracts/Agents/Mentalities/Kerbalminded.cs
- L=37 kind=class type=Kerbalminded bases=AgentMentality anc=AgentMentality ns=Contracts.Agents.Mentalities
- unity(10): none | ksp: Localizer=2, Contract=2 | fan_in=0
- methods(5): CanProcessContract, GetDescription, GetDisplayName, ProcessContract, ScoreKeyword
- auto: A / Contracts

## Assembly-CSharp/Contracts/Agents/Mentalities/Moral.cs
- L=31 kind=class type=Moral bases=AgentMentality anc=AgentMentality ns=Contracts.Agents.Mentalities
- unity(11): none | ksp: Localizer=2, Contract=2 | fan_in=0
- methods(4): CanProcessContract, GetDescription, GetDisplayName, ProcessContract
- auto: A / Contracts

## Assembly-CSharp/Contracts/Agents/Mentalities/NoRandomContractMentality.cs
- L=37 kind=class type=NoRandomContractMentality bases=AgentMentality anc=AgentMentality ns=Contracts.Agents.Mentalities
- unity(10): none | ksp: Localizer=1, Contract=1 | fan_in=0
- methods(4): CanProcessContract, GetDisplayName, OnAdded, ScoreKeyword
- auto: A / Contracts

## Assembly-CSharp/Contracts/Agents/Mentalities/Patient.cs
- L=41 kind=class type=Patient bases=AgentMentality anc=AgentMentality ns=Contracts.Agents.Mentalities
- unity(9): none | ksp: Localizer=2, Contract=2 | fan_in=0
- methods(5): CanProcessContract, GetDescription, GetDisplayName, ProcessContract, ScoreKeyword
- auto: A / Contracts

## Assembly-CSharp/Contracts/Agents/Mentalities/Perfectionist.cs
- L=37 kind=class type=Perfectionist bases=AgentMentality anc=AgentMentality ns=Contracts.Agents.Mentalities
- unity(10): none | ksp: Localizer=2, Contract=2 | fan_in=0
- methods(5): CanProcessContract, GetDescription, GetDisplayName, ProcessContract, ScoreKeyword
- auto: A / Contracts

## Assembly-CSharp/Contracts/Agents/Mentalities/Pioneer.cs
- L=37 kind=class type=Pioneer bases=AgentMentality anc=AgentMentality ns=Contracts.Agents.Mentalities
- unity(10): none | ksp: Localizer=2, Contract=2 | fan_in=0
- methods(5): CanProcessContract, GetDescription, GetDisplayName, ProcessContract, ScoreKeyword
- auto: A / Contracts

## Assembly-CSharp/Contracts/Agents/Mentalities/Record.cs
- L=46 kind=class type=Record bases=AgentMentality anc=AgentMentality ns=Contracts.Agents.Mentalities
- unity(8): none | ksp: Localizer=2, Contract=2 | fan_in=0
- methods(5): CanProcessContract, GetDescription, GetDisplayName, ProcessContract, ScoreKeyword
- auto: A / Contracts

## Assembly-CSharp/Contracts/Agents/Mentalities/Scientific.cs
- L=37 kind=class type=Scientific bases=AgentMentality anc=AgentMentality ns=Contracts.Agents.Mentalities
- unity(10): none | ksp: Localizer=2, Contract=2 | fan_in=0
- methods(5): CanProcessContract, GetDescription, GetDisplayName, ProcessContract, ScoreKeyword
- auto: A / Contracts

## Assembly-CSharp/Contracts/Agents/Mentalities/Startup.cs
- L=41 kind=class type=Startup bases=AgentMentality anc=AgentMentality ns=Contracts.Agents.Mentalities
- unity(9): none | ksp: Localizer=2, Contract=2 | fan_in=0
- methods(5): CanProcessContract, GetDescription, GetDisplayName, ProcessContract, ScoreKeyword
- auto: A / Contracts

## Assembly-CSharp/Contracts/Agents/Mentalities/Stern.cs
- L=39 kind=class type=Stern bases=AgentMentality anc=AgentMentality ns=Contracts.Agents.Mentalities
- unity(9): none | ksp: Localizer=2, Contract=2 | fan_in=0
- methods(5): CanProcessContract, GetDescription, GetDisplayName, ProcessContract, ScoreKeyword
- auto: A / Contracts

## Assembly-CSharp/Contracts/BodyLocation.cs
- L=8 kind=enum type=BodyLocation bases=- anc=- ns=Contracts
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / Contracts

## Assembly-CSharp/Contracts/ContractPredicate.cs
- L=73 kind=class type=ContractPredicate bases=- anc=- ns=Contracts
- unity(14): Lifecycle=1 | ksp: ConfigNode=4, Vessel=1, ProtoVessel=1, Localizer=1, Contract=1 | fan_in=6
- methods(9): GetAllowMultiple, GetDescription, Load, OnLoad, OnSave, OnUpdate, Save, Test, Update
- auto: A / Contracts

## Assembly-CSharp/Contracts/IContractParameterHost.cs
- L=41 kind=interface type=IContractParameterHost bases=- anc=- ns=Contracts
- unity(0): none | ksp: ContractParameter=11, Contract=1 | fan_in=9
- methods(0): 
- auto: A / Contracts

## Assembly-CSharp/Contracts/Parameters/AcquireCrew.cs
- L=197 kind=ContractParameter type=AcquireCrew bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=Contracts.Parameters
- unity(8): none | ksp: GameEvents=13, Part=7, ConfigNode=2, Vessel=1, ProtoCrewMember=1 | fan_in=1
- methods(17): AddKerbal, CheckNewKerbal, GetTitle, KillKerbal, OnCouple, OnCrewBoardVessel, OnCrewKilled, OnCrewTransferred, OnLoad, OnPartDied, OnRegister, OnReset, OnSave, OnUnregister, RemoveKerbal, ScanVessel, SetTitle
- auto: A / Contracts

## Assembly-CSharp/Contracts/Parameters/AcquirePart.cs
- L=167 kind=ContractParameter type=AcquirePart bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=Contracts.Parameters
- unity(9): none | ksp: GameEvents=5, Part=3, ConfigNode=2, Vessel=1, FlightGlobals=1 | fan_in=1
- methods(14): AddPart, CheckNewPart, DestroyPart, GetTitle, OnCouple, OnLoad, OnPartDied, OnRegister, OnReset, OnSave, OnUnregister, RemovePart, ScanVessel, SetTitle
- auto: A / Contracts

## Assembly-CSharp/Contracts/Parameters/CollectScience.cs
- L=109 kind=ContractParameter type=CollectScience bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=Contracts.Parameters
- unity(4): none | ksp: Vessel=5, GameEvents=4, CelestialBody=3, Localizer=2, ConfigNode=2 | fan_in=0
- methods(8): GetHashString, GetTitle, OnLoad, OnRegister, OnSave, OnScience, OnTriggeredScience, OnUnregister
- auto: A / Contracts

## Assembly-CSharp/Contracts/Parameters/EnterOrbit.cs
- L=63 kind=ContractParameter type=EnterOrbit bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=Contracts.Parameters
- unity(6): none | ksp: CelestialBody=4, GameEvents=2, ConfigNode=2, Vessel=1, Localizer=1 | fan_in=1
- methods(7): GetHashString, GetTitle, OnEnterOrbit, OnLoad, OnRegister, OnSave, OnUnregister
- auto: A / Contracts

## Assembly-CSharp/Contracts/Parameters/EnterSOI.cs
- L=63 kind=ContractParameter type=EnterSOI bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=Contracts.Parameters
- unity(6): none | ksp: CelestialBody=4, GameEvents=2, ConfigNode=2, Vessel=1, Localizer=1 | fan_in=0
- methods(7): GetHashString, GetTitle, OnEnterSOI, OnLoad, OnRegister, OnSave, OnUnregister
- auto: A / Contracts

## Assembly-CSharp/Contracts/Parameters/KerbalDeaths.cs
- L=82 kind=ContractParameter type=KerbalDeaths bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=Contracts.Parameters
- unity(5): none | ksp: Localizer=2, GameEvents=2, ConfigNode=2, ContractParameter=1 | fan_in=0
- methods(7): GetHashString, GetTitle, OnCrewKilled, OnLoad, OnRegister, OnSave, OnUnregister
- auto: A / Contracts

## Assembly-CSharp/Contracts/Parameters/LandOnBody.cs
- L=63 kind=ContractParameter type=LandOnBody bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=Contracts.Parameters
- unity(6): none | ksp: CelestialBody=4, GameEvents=2, ConfigNode=2, Vessel=1, Localizer=1 | fan_in=0
- methods(7): GetHashString, GetTitle, OnLoad, OnRegister, OnSave, OnUnregister, OnVesselLand
- auto: A / Contracts

## Assembly-CSharp/Contracts/Parameters/OR.cs
- L=32 kind=ContractParameter type=OR bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=Contracts.Parameters
- unity(11): none | ksp: ContractParameter=2 | fan_in=0
- methods(2): GetHashString, OnParameterStateChange
- auto: A / Contracts

## Assembly-CSharp/Contracts/Parameters/PartTest.cs
- L=336 kind=ContractParameter type=PartTest bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=Contracts.Parameters
- unity(2): none | ksp: GameEvents=7, FlightGlobals=7, Vessel=4, Contract=4, Part=3 | fan_in=0
- methods(16): GetHashString, GetNotes, GetTitle, OnDock, OnLoad, OnPartRunTest, OnRegister, OnReset, OnSave, OnTestPartDestroyed, OnUnregister, OnUpdate, OnVesselChanged, PartModified, VesselModified, hasTestedPart
- auto: A / Contracts

## Assembly-CSharp/Contracts/Parameters/PlantFlag.cs
- L=63 kind=ContractParameter type=PlantFlag bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=Contracts.Parameters
- unity(6): none | ksp: CelestialBody=3, GameEvents=2, ConfigNode=2, Vessel=1, Localizer=1 | fan_in=0
- methods(7): GetHashString, GetTitle, OnLoad, OnPlantFlag, OnRegister, OnSave, OnUnregister
- auto: A / Contracts

## Assembly-CSharp/Contracts/Parameters/ReachAltitudeEnvelope.cs
- L=72 kind=ContractParameter type=ReachAltitudeEnvelope bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=Contracts.Parameters
- unity(5): none | ksp: FlightGlobals=3, ConfigNode=2, Localizer=1, HighLogic=1, ContractParameter=1 | fan_in=1
- methods(5): GetTitle, OnLoad, OnReset, OnSave, OnUpdate
- auto: A / Contracts

## Assembly-CSharp/Contracts/Parameters/ReachBiome.cs
- L=110 kind=ContractParameter type=ReachBiome bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=Contracts.Parameters
- unity(4): none | ksp: FlightGlobals=3, ConfigNode=2, Vessel=1, HighLogic=1, ContractParameter=1 | fan_in=0
- methods(8): GetTitle, GetTitleStringShort, OnLoad, OnReset, OnSave, OnUpdate, bodyHasBiome, checkVesselWithinBiome
- auto: A / Contracts

## Assembly-CSharp/Contracts/Parameters/ReachDestination.cs
- L=99 kind=ContractParameter type=ReachDestination bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=Contracts.Parameters
- unity(4): none | ksp: FlightGlobals=4, CelestialBody=4, Vessel=3, GameEvents=3, ConfigNode=2 | fan_in=2
- methods(10): GetTitle, GetTitleStringShort, OnLoad, OnRegister, OnReset, OnSave, OnUnregister, OnVesselSOIChange, TrackVessel, checkVesselDestination
- auto: A / Contracts

## Assembly-CSharp/Contracts/Parameters/ReachFlightEnvelope.cs
- L=287 kind=ContractParameter type=ReachFlightEnvelope bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=Contracts.Parameters
- unity(12): Coroutine=4 | ksp: Vessel=33, Localizer=19, GameEvents=7, FlightGlobals=3, Contract=3 | fan_in=0
- methods(12): GetSituationStringShort, GetTitle, OnFlightReady, OnLoad, OnRegister, OnSave, OnUnregister, OnVesselChange, OnVesselSituationChange, TrackVessel, checkVesselWithinBiome, checkVesselWithinFlightEnvelope
- auto: A / Contracts

## Assembly-CSharp/Contracts/Parameters/ReachSituation.cs
- L=141 kind=ContractParameter type=ReachSituation bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=Contracts.Parameters
- unity(3): none | ksp: Vessel=22, Localizer=8, GameEvents=3, FlightGlobals=2, ConfigNode=2 | fan_in=2
- methods(10): GetTitle, GetTitleStringShort, OnLoad, OnRegister, OnReset, OnSave, OnUnregister, OnVesselSituationChange, TrackVessel, checkVesselSituation
- auto: A / Contracts

## Assembly-CSharp/Contracts/Parameters/ReachSpeedEnvelope.cs
- L=90 kind=ContractParameter type=ReachSpeedEnvelope bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=Contracts.Parameters
- unity(4): none | ksp: Vessel=8, FlightGlobals=2, ConfigNode=2, Localizer=1, HighLogic=1 | fan_in=1
- methods(6): GetTitle, OnLoad, OnReset, OnSave, OnUpdate, checkVesselWithinEnvelope
- auto: A / Contracts

## Assembly-CSharp/Contracts/Parameters/RecoverKerbal.cs
- L=262 kind=ContractParameter type=RecoverKerbal bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=Contracts.Parameters
- unity(7): none | ksp: GameEvents=10, ProtoCrewMember=2, FlightGlobals=2, ConfigNode=2, Vessel=1 | fan_in=1
- methods(13): AddKerbal, GetTitle, OnCrewKilled, OnFlightReady, OnLoad, OnPartDied, OnRegister, OnSave, OnUnregister, OnVesselLoad, OnVesselRecovered, RemoveKerbal, SetTitle
- auto: A / Contracts

## Assembly-CSharp/Contracts/Parameters/RecoverPart.cs
- L=189 kind=ContractParameter type=RecoverPart bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=Contracts.Parameters
- unity(2): none | ksp: GameEvents=8, FlightGlobals=2, ConfigNode=2, Vessel=1, ProtoVessel=1 | fan_in=1
- methods(11): AddPartToRecover, GetTitle, OnFlightReady, OnLoad, OnPartDie, OnRegister, OnSave, OnUnregister, OnVesselLoad, OnVesselRecovered, RemovePartToRecover
- auto: A / Contracts

## Assembly-CSharp/Contracts/Parameters/XOR.cs
- L=41 kind=ContractParameter type=XOR bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=Contracts.Parameters
- unity(9): none | ksp: ContractParameter=2 | fan_in=0
- methods(2): GetHashString, OnParameterStateChange
- auto: A / Contracts

## Assembly-CSharp/Contracts/ParameterState.cs
- L=14 kind=enum type=ParameterState bases=- anc=- ns=Contracts
- unity(0): none | ksp: none | fan_in=33
- methods(0): 
- auto: A / Contracts

## Assembly-CSharp/Contracts/Predicates/AgentFlag.cs
- L=88 kind=class type=AgentFlag bases=ContractPredicate anc=ContractPredicate ns=Contracts.Predicates
- unity(12): none | ksp: ConfigNode=2, Vessel=1, ProtoVessel=1, Localizer=1 | fan_in=0
- methods(4): GetDescription, OnLoad, OnSave, Test
- auto: A / Contracts

## Assembly-CSharp/Contracts/Predicates/AgentParts.cs
- L=112 kind=class type=AgentParts bases=ContractPredicate anc=ContractPredicate ns=Contracts.Predicates
- unity(12): none | ksp: Localizer=3, ConfigNode=2, Vessel=1, ProtoVessel=1 | fan_in=0
- methods(4): GetDescription, OnLoad, OnSave, Test
- auto: A / Contracts

## Assembly-CSharp/Contracts/Predicates/IsActiveVessel.cs
- L=23 kind=class type=IsActiveVessel bases=ContractPredicate anc=ContractPredicate ns=Contracts.Predicates
- unity(12): none | ksp: Vessel=1, ProtoVessel=1, FlightGlobals=1 | fan_in=0
- methods(1): Test
- auto: A / Contracts

## Assembly-CSharp/Contracts/Predicates/IsControllable.cs
- L=28 kind=class type=IsControllable bases=ContractPredicate anc=ContractPredicate ns=Contracts.Predicates
- unity(12): none | ksp: Vessel=1, ProtoVessel=1 | fan_in=0
- methods(2): GetDescription, Test
- auto: A / Contracts

## Assembly-CSharp/Contracts/Predicates/LaunchID.cs
- L=111 kind=class type=LaunchID bases=ContractPredicate anc=ContractPredicate ns=Contracts.Predicates
- unity(3): none | ksp: Vessel=2, ProtoVessel=2, HighLogic=2, Contract=2, ConfigNode=2 | fan_in=0
- methods(6): ContainsLaunchID_GreaterThanOrEqualTo, GetDescription, OnContractStateChange, OnLoad, OnSave, Test
- auto: A / Contracts

## Assembly-CSharp/Contracts/Templates/CollectScience.cs
- L=177 kind=ContractParameter type=CollectScience bases=Contract anc=ContractParameter/IContractParameterHost ns=Contracts.Templates
- unity(5): none | ksp: CelestialBody=12, Localizer=7, Planetarium=6, Reputation=4, FlightGlobals=2 | fan_in=0
- methods(11): Generate, GenerateContract_Orbit, GenerateContract_Surface, GetDescription, GetSynopsys, GetTitle, GetWeightBodies, MeetRequirements, MessageCompleted, OnLoad, OnSave
- auto: A / Contracts

## Assembly-CSharp/Contracts/Templates/GrandTour.cs
- L=552 kind=Contract type=GrandTour bases=Contract anc=Contract/IContractParameterHost ns=Contracts.Templates
- unity(1): none | ksp: CelestialBody=29, Localizer=27, Vessel=19, Planetarium=14, FlightGlobals=4 | fan_in=0
- methods(16): AttemptFinalDestination, AttemptFocusedTour, AttemptRandomTour, Generate, GetDescription, GetFocusedChallenges, GetHashString, GetNotes, GetSynopsys, GetTitle, GetWeightBodies, MeetRequirements, MessageCompleted, OnAccepted, OnLoad, OnSave
- auto: A / Contracts

## Assembly-CSharp/Contracts/Templates/OrbitalConstructionContract.cs
- L=414 kind=Contract type=OrbitalConstructionContract bases=Contract,IUpdateWaypoints anc=Contract/IContractParameterHost ns=Contracts.Templates
- unity(4): none | ksp: Reputation=6, GameEvents=6, FlightGlobals=6, Localizer=5, CelestialBody=4 | fan_in=0
- methods(20): CanBeCancelled, CanBeDeclined, CleanupRenderer, CreateVesselName, Generate, GetDescription, GetHashString, GetSynopsys, GetTitle, MeetRequirements, MessageCompleted, OnAccepted, OnContractPreBuiltVesselSpawned, OnLoad, OnRegister, OnSave, OnUnregister, OnVesselDestroy, SetupRenderer, VesselIdChanged
- auto: A / Contracts

## Assembly-CSharp/Contracts/Templates/PlantFlag.cs
- L=137 kind=ContractParameter type=PlantFlag bases=Contract anc=ContractParameter/IContractParameterHost ns=Contracts.Templates
- unity(6): none | ksp: CelestialBody=10, Vessel=5, Planetarium=4, Localizer=3, Reputation=2 | fan_in=0
- methods(10): Generate, GetDescription, GetHashString, GetSynopsys, GetTitle, GetWeightBodies, MeetRequirements, MessageCompleted, OnLoad, OnSave
- auto: A / Contracts

## Assembly-CSharp/Contracts/Templates/RoverConstructionContract.cs
- L=337 kind=Contract type=RoverConstructionContract bases=Contract,IUpdateWaypoints anc=Contract/IContractParameterHost ns=Contracts.Templates
- unity(5): none | ksp: Reputation=6, GameEvents=6, FlightGlobals=6, CelestialBody=5, Localizer=4 | fan_in=0
- methods(19): CalculateWaypoint, CanBeCancelled, CanBeDeclined, CreateVesselName, Generate, GetDescription, GetHashString, GetSynopsys, GetTitle, MeetRequirements, MessageCompleted, OnAccepted, OnContractPreBuiltVesselSpawned, OnLoad, OnRegister, OnSave, OnUnregister, OnVesselDestroy, VesselIdChanged
- auto: A / Contracts

## Assembly-CSharp/Contracts/Templates/VesselRepairContract.cs
- L=602 kind=Contract type=VesselRepairContract bases=Contract anc=Contract/IContractParameterHost ns=Contracts.Templates
- unity(3): none | ksp: FlightGlobals=9, Reputation=6, GameEvents=6, Localizer=5, PartLoader=4 | fan_in=0
- methods(20): CanBeCancelled, CanBeDeclined, CleanupRenderer, CreateVesselName, Generate, GetDescription, GetHashString, GetSynopsys, GetTitle, MeetRequirements, MessageCompleted, OnAccepted, OnContractPreBuiltVesselSpawned, OnLoad, OnRegister, OnSave, OnUnregister, OnVesselDestroy, SetupRenderer, VesselIdChanged
- auto: A / Contracts

## Assembly-CSharp/Contracts/TextGen.cs
- L=180 kind=static class type=TextGen bases=- anc=- ns=Contracts
- unity(11): none | ksp: ConfigNode=2, Localizer=1, GameDatabase=1 | fan_in=22
- methods(5): GenerateBackStories, GetSentence, ProcessTextReplacements, ReplaceCharAt, Setup
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Contracts/ARMContract.cs
- L=235 kind=Contract type=ARMContract bases=Contract anc=Contract/IContractParameterHost ns=FinePrint.Contracts
- unity(2): none | ksp: Planetarium=18, Localizer=18, CelestialBody=8, Vessel=3, Reputation=3 | fan_in=0
- methods(12): CanBeCancelled, CanBeDeclined, Generate, GetDescription, GetHashString, GetSynopsys, GetTitle, GetWeightBodies, MeetRequirements, MessageCompleted, OnLoad, OnSave
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Contracts/BaseContract.cs
- L=264 kind=Contract type=BaseContract bases=Contract,IUpdateWaypoints,ICheckSpecificVessels anc=Contract/IContractParameterHost ns=FinePrint.Contracts
- unity(3): none | ksp: Localizer=13, Planetarium=9, CelestialBody=9, Vessel=3, Reputation=3 | fan_in=1
- methods(15): AreFacilitiesUnlocked, CanBeCancelled, CanBeDeclined, Generate, GetDescription, GetHashString, GetSynopsys, GetTitle, GetWeightBodies, MeetRequirements, MessageCompleted, OnAccepted, OnFinished, OnLoad, OnSave
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Contracts/CometSampleContract.cs
- L=278 kind=Contract type=CometSampleContract bases=Contract anc=Contract/IContractParameterHost ns=FinePrint.Contracts
- unity(9): none | ksp: Localizer=13, FlightGlobals=8, GameEvents=7, Reputation=6, Vessel=5 | fan_in=0
- methods(17): CanBeCancelled, CanBeDeclined, Generate, GetDescription, GetHashString, GetSynopsys, GetTitle, MeetRequirements, MessageCompleted, OnDiscoverableObjectExpired, OnLoad, OnRegister, OnSave, OnUnregister, OnVesselRename, ValidCometAvailable, VesselIdChanged
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Contracts/ISRUContract.cs
- L=298 kind=Contract type=ISRUContract bases=Contract anc=Contract/IContractParameterHost ns=FinePrint.Contracts
- unity(4): none | ksp: ConfigNode=14, CelestialBody=11, Localizer=8, Planetarium=7, Vessel=3 | fan_in=0
- methods(14): CanBeCancelled, CanBeDeclined, CelestialIsForbidden, Generate, GetDescription, GetHashString, GetSynopsys, GetTitle, GetWeightBodies, MeetRequirements, MessageCompleted, OnLoad, OnSave, PossibleResources
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/AsteroidParameter.cs
- L=231 kind=ContractParameter type=AsteroidParameter bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=FinePrint.Contracts.Parameters
- unity(0): none | ksp: FlightGlobals=8, Localizer=7, GameEvents=7, Part=3, Contract=3 | fan_in=2
- methods(13): GetHashString, GetNotes, GetTitle, OnDock, OnLoad, OnRegister, OnReset, OnSave, OnUnregister, OnUpdate, PartModified, VesselModified, VesselTowingAsteroid
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/CometDetectionParameter.cs
- L=92 kind=ContractParameter type=CometDetectionParameter bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=FinePrint.Contracts.Parameters
- unity(0): none | ksp: Localizer=4, GameEvents=2, ConfigNode=2, Vessel=1, HighLogic=1 | fan_in=1
- methods(9): CometDetected, GetHashString, GetNotes, GetTitle, OnLoad, OnRegister, OnReset, OnSave, OnUnregister
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/CometParameter.cs
- L=231 kind=ContractParameter type=CometParameter bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=FinePrint.Contracts.Parameters
- unity(0): none | ksp: FlightGlobals=8, Localizer=7, GameEvents=7, Part=3, Contract=3 | fan_in=0
- methods(13): GetHashString, GetNotes, GetTitle, OnDock, OnLoad, OnRegister, OnReset, OnSave, OnUnregister, OnUpdate, PartModified, VesselModified, VesselTowingComet
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/CometScienceParameter.cs
- L=125 kind=ContractParameter type=CometScienceParameter bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=FinePrint.Contracts.Parameters
- unity(3): none | ksp: GameEvents=11, Vessel=2, ConfigNode=2, ScienceSubject=1, ProtoVessel=1 | fan_in=1
- methods(11): GetHashString, GetTitle, OnExperimentDeployed, OnLoad, OnRegister, OnSave, OnScience, OnTriggeredScience, OnUnregister, OnVesselRename, VesselIdChanged
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/ConstructionParameter.cs
- L=128 kind=ContractParameter type=ConstructionParameter bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=FinePrint.Contracts.Parameters
- unity(3): none | ksp: GameEvents=4, FlightGlobals=4, CelestialBody=4, Vessel=2, PartLoader=2 | fan_in=1
- methods(10): GetHashString, GetMessageComplete, GetTitle, OnConstructionPartAttached, OnLoad, OnRegister, OnSave, OnUnregister, UpdatePartInfo, VesselIdChanged
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/CrewCapacityParameter.cs
- L=167 kind=ContractParameter type=CrewCapacityParameter bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=FinePrint.Contracts.Parameters
- unity(0): none | ksp: FlightGlobals=11, GameEvents=7, Part=3, Localizer=3, Contract=2 | fan_in=2
- methods(12): GetHashString, GetNotes, GetTitle, OnDock, OnLoad, OnRegister, OnReset, OnSave, OnUnregister, OnUpdate, PartModified, VesselModified
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/CrewTraitParameter.cs
- L=202 kind=ContractParameter type=CrewTraitParameter bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=FinePrint.Contracts.Parameters
- unity(0): none | ksp: FlightGlobals=12, GameEvents=9, Localizer=4, Part=3, Vessel=2 | fan_in=4
- methods(13): CrewModified, GetHashString, GetNotes, GetTitle, OnDock, OnLoad, OnRegister, OnReset, OnSave, OnUnregister, OnUpdate, PartModified, VesselModified
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/KerbalDestinationParameter.cs
- L=143 kind=ContractParameter type=KerbalDestinationParameter bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=FinePrint.Contracts.Parameters
- unity(0): none | ksp: Localizer=6, ProtoCrewMember=3, HighLogic=3, Planetarium=2, GameEvents=2 | fan_in=2
- methods(8): CheckFlightLog, CheckFlightLogs, GetHashString, GetTitle, OnLoad, OnRegister, OnSave, OnUnregister
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/KerbalGeeAdventureParameter.cs
- L=93 kind=ContractParameter type=KerbalGeeAdventureParameter bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=FinePrint.Contracts.Parameters
- unity(0): none | ksp: GameEvents=4, ProtoCrewMember=2, ConfigNode=2, Localizer=1, ContractParameter=1 | fan_in=1
- methods(8): GetHashString, GetTitle, OnCrewKilled, OnKerbalPassedOut, OnLoad, OnRegister, OnSave, OnUnregister
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/KerbalTourParameter.cs
- L=134 kind=ContractParameter type=KerbalTourParameter bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=FinePrint.Contracts.Parameters
- unity(0): none | ksp: ProtoCrewMember=8, GameEvents=6, Localizer=2, HighLogic=2, ContractParameter=2 | fan_in=2
- methods(11): GetHashString, GetMessageFailed, GetTitle, OnCrewKilled, OnKerbalInactiveChange, OnLoad, OnParameterStateChange, OnRegister, OnSave, OnUnregister, OnVesselRecovered
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/LocationAndSituationParameter.cs
- L=169 kind=ContractParameter type=LocationAndSituationParameter bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=FinePrint.Contracts.Parameters
- unity(0): none | ksp: Vessel=15, Localizer=10, FlightGlobals=9, GameEvents=6, CelestialBody=3 | fan_in=5
- methods(10): ChangeBody, ChangeSituation, GetHashString, GetTitle, OnLoad, OnRegister, OnReset, OnSave, OnUnregister, OnUpdate
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/MobileBaseParameter.cs
- L=135 kind=ContractParameter type=MobileBaseParameter bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=FinePrint.Contracts.Parameters
- unity(0): none | ksp: FlightGlobals=12, GameEvents=10, Vessel=3, Part=3, Contract=2 | fan_in=1
- methods(10): ChangeSituation, GetHashString, GetTitle, OnDock, OnRegister, OnReset, OnUnregister, OnUpdate, PartModified, VesselModified
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/PartRequestParameter.cs
- L=340 kind=ContractParameter type=PartRequestParameter bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=FinePrint.Contracts.Parameters
- unity(0): none | ksp: GameEvents=7, FlightGlobals=7, Part=5, Vessel=4, ConfigNode=3 | fan_in=3
- methods(14): CheckLists, GetHashString, GetTitle, OnDock, OnLoad, OnRegister, OnReset, OnSave, OnUnregister, OnUpdate, PartModified, ScanVesselForExistingParts, VesselHasPartRequest, VesselModified
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/ProgressTrackingParameter.cs
- L=351 kind=ContractParameter type=ProgressTrackingParameter bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=FinePrint.Contracts.Parameters
- unity(0): none | ksp: Localizer=95, Planetarium=25, GameEvents=2, ConfigNode=2, HighLogic=1 | fan_in=1
- methods(9): GetHashString, GetMessageComplete, GetNotes, GetTitle, OnLoad, OnRegister, OnSave, OnUnregister, ProgressCheck
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/RepairPartParameter.cs
- L=148 kind=ContractParameter type=RepairPartParameter bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=FinePrint.Contracts.Parameters
- unity(3): none | ksp: GameEvents=6, FlightGlobals=4, CelestialBody=4, PartLoader=2, Localizer=2 | fan_in=1
- methods(11): GetHashString, GetMessageComplete, GetTitle, OnLoad, OnPartRepaired, OnRegister, OnSave, OnUnregister, PartIdChanged, UpdatePartInfo, VesselIdChanged
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/ResourceExtractionParameter.cs
- L=155 kind=ContractParameter type=ResourceExtractionParameter bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=FinePrint.Contracts.Parameters
- unity(0): none | ksp: Localizer=3, Planetarium=2, GameEvents=2, Contract=2, ConfigNode=2 | fan_in=1
- methods(9): GetHashString, GetTitle, OnLoad, OnRegister, OnReset, OnSave, OnUnregister, OnUpdate, ResourcesHarvested
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/ResourcePossessionParameter.cs
- L=130 kind=ContractParameter type=ResourcePossessionParameter bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=FinePrint.Contracts.Parameters
- unity(0): none | ksp: Localizer=3, ConfigNode=2, ContractParameter=1, Contract=1 | fan_in=2
- methods(8): GetHashString, GetNotes, GetTitle, OnLoad, OnRegister, OnReset, OnSave, OnUpdate
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/RoverWayPointParameter.cs
- L=310 kind=ContractParameter type=RoverWayPointParameter bases=WaypointParameter anc=WaypointParameter/ContractParameter/IContractParameterHost ns=FinePrint.Contracts.Parameters
- unity(0): none | ksp: Localizer=6, Contract=4, Vessel=3, HighLogic=2, GameEvents=2 | fan_in=1
- methods(14): CleanupWaypoints, GetHashString, GetMessageComplete, GetNotes, GetTitle, OnLoad, OnRegister, OnReset, OnSave, OnUnregister, OnUpdate, UpdateRoverInfo, UpdateWaypoints, VesselIdChanged
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/SpecificOrbitParameter.cs
- L=303 kind=ContractParameter type=SpecificOrbitParameter bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=FinePrint.Contracts.Parameters
- unity(0): none | ksp: Localizer=21, Contract=8, Planetarium=6, Orbit=6, HighLogic=3 | fan_in=3
- methods(12): CleanupRenderer, GetHashString, GetNotes, GetTitle, OnLoad, OnRegister, OnReset, OnSave, OnUnregister, OnUpdate, SetupRenderer, ValidateOrbit
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/SpecificVesselParameter.cs
- L=361 kind=ContractParameter type=SpecificVesselParameter bases=WaypointParameter anc=WaypointParameter/ContractParameter/IContractParameterHost ns=FinePrint.Contracts.Parameters
- unity(6): Coroutine=3 | ksp: Vessel=16, Localizer=11, FlightGlobals=7, Contract=6, Planetarium=3 | fan_in=8
- methods(16): CleanupWaypoints, GetHashString, GetMessageComplete, GetNotes, GetTitle, KillContract, OnLoad, OnRegister, OnReset, OnSave, OnUnregister, OnUpdate, SpecificVesselRename, SpecificVesselScan, TargetVesselSearch, UpdateWaypoints
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/StabilityParameter.cs
- L=98 kind=ContractParameter type=StabilityParameter bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=FinePrint.Contracts.Parameters
- unity(4): Time.=1 | ksp: Vessel=2, ConfigNode=2, Localizer=1, FlightGlobals=1, ContractParameter=1 | fan_in=5
- methods(7): GetHashString, GetTitle, OnLoad, OnRegister, OnReset, OnSave, OnUpdate
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/StationaryPointParameter.cs
- L=159 kind=ContractParameter type=StationaryPointParameter bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=FinePrint.Contracts.Parameters
- unity(0): none | ksp: Contract=3, Planetarium=2, Localizer=2, HighLogic=2, ConfigNode=2 | fan_in=1
- methods(9): GetHashString, GetMessageComplete, GetTitle, OnLoad, OnRegister, OnReset, OnSave, OnUnregister, OnUpdate
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/SurveyWaypointParameter.cs
- L=557 kind=ContractParameter type=SurveyWaypointParameter bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=FinePrint.Contracts.Parameters
- unity(1): none | ksp: Localizer=24, Contract=5, HighLogic=3, CelestialBody=3, ResearchAndDevelopment=2 | fan_in=1
- methods(17): Bounce, CalculateAltitudes, CheckExperimentResults, GetHashString, GetMessageComplete, GetTitle, NextWaypointInCluster, OnLoad, OnRegister, OnSave, OnUnregister, OnUpdate, ProcessWaypoint, SiteString, TriggerRange, UpdateNavWaypoint, VerifyWaypoint
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/VesselDestinationParameter.cs
- L=116 kind=ContractParameter type=VesselDestinationParameter bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=FinePrint.Contracts.Parameters
- unity(0): none | ksp: Localizer=6, GameEvents=4, Planetarium=2, ConfigNode=2, CelestialBody=2 | fan_in=1
- methods(8): CheckFlightLog, GetHashString, GetTitle, OnLoad, OnRegister, OnReset, OnSave, OnUnregister
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Contracts/Parameters/VesselSystemsParameter.cs
- L=248 kind=ContractParameter type=VesselSystemsParameter bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=FinePrint.Contracts.Parameters
- unity(0): none | ksp: FlightGlobals=11, Localizer=10, GameEvents=7, Part=3, Vessel=2 | fan_in=4
- methods(13): GetHashString, GetNotes, GetTitle, OnDock, OnLoad, OnRegister, OnReset, OnSave, OnUnregister, OnUpdate, PartModified, VesselModified, isVesselValid
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Contracts/StationContract.cs
- L=305 kind=Contract type=StationContract bases=Contract,IUpdateWaypoints,ICheckSpecificVessels anc=Contract/IContractParameterHost ns=FinePrint.Contracts
- unity(2): none | ksp: Localizer=15, Planetarium=10, CelestialBody=4, Vessel=3, Reputation=3 | fan_in=1
- methods(16): AreAsteroidsUnlocked, AreFacilitiesUnlocked, CanBeCancelled, CanBeDeclined, Generate, GetDescription, GetHashString, GetSynopsys, GetTitle, GetWeightBodies, MeetRequirements, MessageCompleted, OnAccepted, OnFinished, OnLoad, OnSave
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Contracts/TourismContract.cs
- L=633 kind=Contract type=TourismContract bases=Contract anc=Contract/IContractParameterHost ns=FinePrint.Contracts
- unity(0): none | ksp: CelestialBody=42, ProtoCrewMember=16, Localizer=13, HighLogic=9, Planetarium=7 | fan_in=2
- methods(35): BuildPossibleItineraries, CanBeCancelled, CanBeDeclined, ClearKerbalsHard, ClearKerbalsSoft, Generate, GetContractSubject, GetDescription, GetHashString, GetKerbal, GetNotes, GetSynopsys, GetTitle, GetWeightBodies, MeetRequirements, MessageCancelled, MessageCompleted, MessageDeadlineExpired, MessageFailed, OnAccepted, OnCancelled, OnDeadlineExpired, OnDeclined, OnFailed ...
- auto: A / Contracts

## Assembly-CSharp/FinePrint/ScenarioContractEvents.cs
- L=185 kind=ScenarioModule type=ScenarioContractEvents bases=ScenarioModule anc=ScenarioModule/MonoBehaviour ns=FinePrint
- unity(38): SendMessage/Invoke=3, Coroutine=3, Lifecycle=1 | ksp: GameEvents=14, Vessel=3, ProtoVessel=3, ProtoCrewMember=3, HighLogic=3 | fan_in=1
- methods(14): CheckSpecificVessels, InvokeCheckSpecificVessels, OnAwake, OnDestroy, OnLoad, OnSave, PartCoupleEvent, ProcessGameSeed, TouristDepartures, VesselRecoveredEvent, VesselRenameEvent, VesselTerminatedEvent, VesselWasModifiedEvent, VesselWillDestroyEvent
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Utilities/CelestialUtilities.cs
- L=467 kind=static class type=CelestialUtilities bases=- anc=- ns=FinePrint.Utilities
- unity(4): Transform=1, GetComponent=1 | ksp: CelestialBody=49, Vector3d=8, Vessel=7, Planetarium=6, FlightGlobals=4 | fan_in=30
- methods(26): AddChildren, ApplicableSituation, CanBodyBeKolniya, CanBodyBeSynchronous, CanBodyBeTundra, ChildrenOf, GetAltitudeForDensity, GetHighestPeak, GetHostPlanet, GetMinimumOrbitalDistance, GetNeighbors, GetNextTimeWarp, GetPlanetarySystem, GetSolarExtents, GreatCircleDistance, HighestBody, IsFlyablePlanet, IsGasGiant, KolniyaSMA, LLAtoECEF, LowestBody, MapFocusBody, PlanetScienceRanking, RandomBody ...
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Utilities/CompatibilityUtilities.cs
- L=619 kind=class type=CompatibilityUtilities bases=- anc=- ns=FinePrint.Utilities
- unity(3): none | ksp: Vessel=21, ConfigNode=21, Contract=4, Planetarium=3, Game=3 | fan_in=2
- methods(6): CleanUpUnsanitaryEVAKerbals, FindSurveyDefinition, OldCareerSave, SavePriorTo, UpdateCareerSave, UpgradeVesselSituation
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Utilities/ConstructionExtensions.cs
- L=26 kind=static class type=ConstructionExtensions bases=- anc=- ns=FinePrint.Utilities
- unity(0): none | ksp: none | fan_in=0
- methods(1): ContainsPart
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Utilities/ConstructionPart.cs
- L=20 kind=class type=ConstructionPart bases=- anc=- ns=FinePrint.Utilities
- unity(0): none | ksp: AvailablePart=2 | fan_in=4
- methods(0): 
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Utilities/ConstructionParts.cs
- L=39 kind=class type=ConstructionParts bases=- anc=- ns=FinePrint.Utilities
- unity(17): none | ksp: none | fan_in=1
- methods(1): PartsForContractType
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Utilities/FlightBand.cs
- L=13 kind=enum type=FlightBand bases=- anc=- ns=FinePrint.Utilities
- unity(0): none | ksp: none | fan_in=4
- methods(0): 
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Utilities/GrammaticalGender.cs
- L=9 kind=enum type=GrammaticalGender bases=- anc=- ns=FinePrint.Utilities
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Utilities/ICheckSpecificVessels.cs
- L=6 kind=interface type=ICheckSpecificVessels bases=- anc=- ns=FinePrint.Utilities
- unity(0): none | ksp: none | fan_in=4
- methods(0): 
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Utilities/IUpdateWaypoints.cs
- L=6 kind=interface type=IUpdateWaypoints bases=- anc=- ns=FinePrint.Utilities
- unity(0): none | ksp: none | fan_in=8
- methods(0): 
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Utilities/LoadResult.cs
- L=10 kind=enum type=LoadResult bases=- anc=- ns=FinePrint.Utilities
- unity(0): none | ksp: none | fan_in=1
- methods(0): 
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Utilities/MannedStatus.cs
- L=9 kind=enum type=MannedStatus bases=- anc=- ns=FinePrint.Utilities
- unity(0): none | ksp: none | fan_in=16
- methods(0): 
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Utilities/OrbitalProperties.cs
- L=13 kind=enum type=OrbitalProperties bases=- anc=- ns=FinePrint.Utilities
- unity(0): none | ksp: none | fan_in=1
- methods(0): 
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Utilities/OrbitType.cs
- L=13 kind=enum type=OrbitType bases=- anc=- ns=FinePrint.Utilities
- unity(0): none | ksp: none | fan_in=7
- methods(0): 
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Utilities/OrbitUtilities.cs
- L=360 kind=static class type=OrbitUtilities bases=- anc=- ns=FinePrint.Utilities
- unity(5): none | ksp: Orbit=15, Vector3d=10, Planetarium=3, CelestialBody=3, FlightGlobals=2 | fan_in=6
- methods(15): AngleOfAscendingNode, AngleOfDescendingNode, GenerateApsides, GenerateArgumentOfPeriapsis, GenerateEpoch, GenerateFinalization, GenerateInclination, GenerateLongitudeOfAscendingNode, GenerateMeanAnomalyAtEpoch, GenerateOrbit, GetRelativeInclination, IdentifyOrbit, PositionOfApoapsis, PositionOfPeriapsis, ValidateOrbit
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Utilities/PreBuiltCraft.cs
- L=61 kind=class type=PreBuiltCraft bases=- anc=- ns=FinePrint.Utilities
- unity(12): none | ksp: none | fan_in=1
- methods(2): CraftByURL, CraftForContractType
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Utilities/PreBuiltCraftDefinition.cs
- L=38 kind=class type=PreBuiltCraftDefinition bases=- anc=- ns=FinePrint.Utilities
- unity(10): none | ksp: ConfigNode=2 | fan_in=7
- methods(0): 
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Utilities/PreBuiltCraftPosition.cs
- L=11 kind=class type=PreBuiltCraftPosition bases=- anc=- ns=FinePrint.Utilities
- unity(0): none | ksp: none | fan_in=3
- methods(0): 
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Utilities/ProgressMilestone.cs
- L=282 kind=class type=ProgressMilestone bases=- anc=- ns=FinePrint.Utilities
- unity(0): none | ksp: CelestialBody=2, Planetarium=1 | fan_in=4
- methods(1): CheckCompletion
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Utilities/ProgressRewardType.cs
- L=8 kind=enum type=ProgressRewardType bases=- anc=- ns=FinePrint.Utilities
- unity(0): none | ksp: none | fan_in=3
- methods(0): 
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Utilities/ProgressType.cs
- L=35 kind=enum type=ProgressType bases=- anc=- ns=FinePrint.Utilities
- unity(0): none | ksp: none | fan_in=44
- methods(0): 
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Utilities/ProgressUtilities.cs
- L=676 kind=class type=ProgressUtilities bases=- anc=- ns=FinePrint.Utilities
- unity(1): none | ksp: CelestialBody=57, Planetarium=10, Contract=8, FlightGlobals=5, ResearchAndDevelopment=4 | fan_in=29
- methods(21): AverageFacilityLevel, CelestialCrewCounts, CurrentTrackRecord, ExperimentPossibleAt, FindNextRecord, GetAnyBodyProgress, GetBodiesProgress, GetBodyProgress, GetNextUnreached, GetProgressLevel, HaveAnyTech, HaveModuleTech, HavePartTech, IntroWorldFirstContract, OutlierWorldFirstContract, ProgressTypePrestige, ReachedHomeBodies, ScoreProgressType, VisitedSurfaceOf, WorldFirstIntervalReward, WorldFirstStandardReward
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Utilities/PronounCasing.cs
- L=9 kind=enum type=PronounCasing bases=- anc=- ns=FinePrint.Utilities
- unity(0): none | ksp: none | fan_in=1
- methods(0): 
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Utilities/RecordTrackType.cs
- L=11 kind=enum type=RecordTrackType bases=- anc=- ns=FinePrint.Utilities
- unity(0): none | ksp: none | fan_in=6
- methods(0): 
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Utilities/SpriteMap.cs
- L=24 kind=class type=SpriteMap bases=- anc=- ns=FinePrint.Utilities
- unity(67): Texture=5 | ksp: none | fan_in=1
- methods(0): 
- auto: B / Contracts

## Assembly-CSharp/FinePrint/Utilities/StringUtilities.cs
- L=569 kind=class type=StringUtilities bases=- anc=- ns=FinePrint.Utilities
- unity(0): none | ksp: Localizer=25, ProtoCrewMember=6, ConfigNode=5, Reputation=3, Planetarium=2 | fan_in=33
- methods(26): AlphaNumericDesignation, AppropriatePronoun, CacheLocalStrings, CalculateReadDuration, CardinalDirectionBetween, FillListWithElements, FormattedCurrencies, GenerateSiteName, GetSiteGeneratorInfoFileName, IntegerToGreek, IsVowel, KerbalGrammaticalGender, LoadSiteGenerationInfo, NamedSitePrefix, NamedSiteSuffix, PackDelimitedString, PossessiveString, ShortKerbalName, ShortName, SpecificVesselName, ThisThisAndThat, TitleCase, TryParseGeneric, UnpackDelimitedPair ...
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Utilities/SurveyDefinition.cs
- L=79 kind=class type=SurveyDefinition bases=- anc=- ns=FinePrint.Utilities
- unity(0): none | ksp: Reputation=4, ConfigNode=4 | fan_in=4
- methods(0): 
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Utilities/SurveyDefinitionParameter.cs
- L=102 kind=class type=SurveyDefinitionParameter bases=- anc=- ns=FinePrint.Utilities
- unity(32): Texture=4 | ksp: ConfigNode=3 | fan_in=2
- methods(0): 
- auto: B / Contracts

## Assembly-CSharp/FinePrint/Utilities/SystemUtilities.cs
- L=652 kind=class type=SystemUtilities bases=- anc=- ns=FinePrint.Utilities
- unity(11): Texture=3, GameObject=1 | ksp: ConfigNode=14, FlightGlobals=10, HighLogic=6, Contract=6, CelestialBody=5 | fan_in=66
- methods(26): CheckTouristRecoveryContractKerbals, CoinFlip, CondenseNodeList, ExpungeKerbal, ExpungeVessel, FlightIsReady, GetDeviationAsFraction, HashNumber, LoadNode, LoadNodeList, LoadNodePair, LoadTexture, MeasureDeviation, MeasureDeviationByValue, ProcessSideRequests, RandomColor, RandomSplitChoice, SaveNodeList, ShuffleList, SuperSeed, TryConvert, ValidateLoadCraftNode, WithinDeviation, WithinDeviationAccuracy ...
- auto: B / Contracts

## Assembly-CSharp/FinePrint/Utilities/WaypointClusterState.cs
- L=10 kind=enum type=WaypointClusterState bases=- anc=- ns=FinePrint.Utilities
- unity(0): none | ksp: none | fan_in=2
- methods(0): 
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Utilities/WaypointParameter.cs
- L=21 kind=ContractParameter type=WaypointParameter bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=FinePrint.Utilities
- unity(0): none | ksp: ContractParameter=1, CelestialBody=1 | fan_in=3
- methods(3): CleanupWaypoints, SetupWaypoints, UpdateWaypoints
- auto: A / Contracts

## Assembly-CSharp/FinePrint/Waypoint.cs
- L=479 kind=class type=Waypoint bases=- anc=- ns=FinePrint
- unity(8): Time.=3, Transform=1, Texture=1 | ksp: Vector3d=15, ScaledSpace=6, CelestialBody=5, FlightGlobals=4, MapView=3 | fan_in=33
- methods(16): CachePositions, CheckAlpha, CheckOcclusion, CheckZoom, CleanupMapNode, OnMenuDismissed, OnUpdateNodeCaption, OnUpdateNodeIcon, OnUpdateNodePosition, ProcessClick, RandomizeAwayFrom, RandomizeNear, SetFadeRange, SetupMapNode, UpdateWaypoint, setName
- auto: A / Contracts

## Assembly-CSharp/FinePrint/WaypointCastHit.cs
- L=19 kind=struct type=WaypointCastHit bases=IScreenCaster anc=IScreenCaster ns=FinePrint
- unity(30): Camera=1 | ksp: none | fan_in=1
- methods(1): GetScreenSpacePoint
- auto: A / Contracts

## Assembly-CSharp/FinePrint/WaypointManager.cs
- L=346 kind=MonoBehaviour type=WaypointManager bases=MonoBehaviour anc=MonoBehaviour ns=FinePrint
- unity(47): Material=3, GameObject=3, Shader=2, Lifecycle=2, GetComponent=2, Camera=2 | ksp: CelestialBody=8, Vector3d=6, HighLogic=6, FlightGlobals=6, MapView=5 | fan_in=13
- methods(14): AddWaypoint, ChooseRandomPosition, ChooseRandomPositionNear, Distance, DistanceToVessel, FindWaypoint, Instance, LateUpdate, LateralDistanceToVessel, OnDestroy, RemoveWaypoint, UpdateContractWaypoints, UpdateWaypointNodes, WaypointsNearVessel
- auto: C / Contracts

## Assembly-CSharp/KerbNet/KerbNetMode.cs
- L=192 kind=class type=KerbNetMode bases=- anc=- ns=KerbNet
- unity(24): Texture=4 | ksp: Vessel=7, GameSettings=1 | fan_in=4
- methods(19): AutoGenerateMode, CustomPass, GetBackgroundColor, GetCoordinateColor, GetErrorState, GetModeCaption, GetModeColorTint, GetModeColorTintBlock, GetTerrainContourColors, Init, InterpolateContourTexture, InterpolateMainTexture, LocalCoordinateInfo, OnActivated, OnDeactivated, OnInit, OnPrecache, Precache, isModeActive
- auto: B / CommNet

## Assembly-CSharp/KerbNet/KerbNetModeBiome.cs
- L=81 kind=class type=KerbNetModeBiome bases=KerbNetMode anc=KerbNetMode ns=KerbNet
- unity(35): Texture=1, Resources.=1, Renderer=1 | ksp: Localizer=5, Vessel=3, CelestialBody=2, Planetarium=1, OrbitDriver=1 | fan_in=1
- methods(5): CacheLocalStrings, GetCoordinateColor, LocalCoordinateInfo, OnInit, OnPrecache
- auto: A / CommNet

## Assembly-CSharp/KerbNet/KerbNetModeResource.cs
- L=239 kind=class type=KerbNetModeResource bases=KerbNetMode anc=KerbNetMode ns=KerbNet
- unity(15): Texture=1, Resources.=1 | ksp: Vessel=5, Localizer=5, GameEvents=5, CelestialBody=4, PartResourceDefinition=1 | fan_in=2
- methods(16): AutoGenerateMode, CacheLocalStrings, CheckScannedBody, GetAbundance, GetCoordinateColor, GetErrorState, GetModeCaption, GetModeColorTint, LocalCoordinateInfo, OnActivated, OnColorClick, OnDeactivated, OnInit, OnPrecache, OrbitalSurveyCompleted, VesselSOIChange
- auto: A / CommNet

## Assembly-CSharp/KerbNet/KerbNetModeTerrain.cs
- L=63 kind=class type=KerbNetModeTerrain bases=KerbNetMode anc=KerbNetMode ns=KerbNet
- unity(45): Renderer=2, Texture=1, Resources.=1 | ksp: Localizer=4, Vessel=2 | fan_in=1
- methods(4): CacheLocalStrings, GetTerrainContourColors, LocalCoordinateInfo, OnInit
- auto: A / CommNet

## Assembly-CSharp/SentinelMission/CometDetectionContract.cs
- L=140 kind=Contract type=CometDetectionContract bases=Contract,IUpdateWaypoints anc=Contract/IContractParameterHost ns=SentinelMission
- unity(8): none | ksp: Localizer=5, Reputation=4, ConfigNode=4, Planetarium=3, Contract=1 | fan_in=0
- methods(11): CanBeCancelled, CanBeDeclined, Generate, GetDescription, GetHashString, GetSynopsys, GetTitle, MeetRequirements, MessageCompleted, OnLoad, OnSave
- auto: A / Contracts

## Assembly-CSharp/SentinelMission/SentinelContract.cs
- L=259 kind=Contract type=SentinelContract bases=Contract,IUpdateWaypoints anc=Contract/IContractParameterHost ns=SentinelMission
- unity(7): none | ksp: Planetarium=15, Localizer=11, Reputation=6, ConfigNode=4, CelestialBody=4 | fan_in=1
- methods(12): CanBeCancelled, CanBeDeclined, Generate, GetDescription, GetHashString, GetSynopsys, GetTitle, MeetRequirements, MessageCompleted, OnLoad, OnSave, SetFocusBody
- auto: A / Contracts

## Assembly-CSharp/SentinelMission/SentinelModule.cs
- L=114 kind=PartModule type=SentinelModule bases=PartModule anc=PartModule/MonoBehaviour ns=SentinelMission
- unity(22): Lifecycle=1 | ksp: Localizer=10, KSPField=2, KSPEvent=2, Planetarium=1, PartModule=1 | fan_in=2
- methods(7): CacheLocalStrings, FixedUpdate, OnStart, ShowMessage, StartTracking, StopTracking, TelescopeCanActivate
- auto: A / Contracts

## Assembly-CSharp/SentinelMission/SentinelParameter.cs
- L=138 kind=ContractParameter type=SentinelParameter bases=ContractParameter anc=ContractParameter/IContractParameterHost ns=SentinelMission
- unity(0): none | ksp: Localizer=9, Planetarium=3, Orbit=3, CelestialBody=3, ConfigNode=2 | fan_in=2
- methods(7): DiscoverAsteroid, GetHashString, GetNotes, GetTitle, OnLoad, OnRegister, OnSave
- auto: A / Contracts

## Assembly-CSharp/SentinelMission/SentinelScanType.cs
- L=10 kind=enum type=SentinelScanType bases=- anc=- ns=SentinelMission
- unity(0): none | ksp: none | fan_in=3
- methods(0): 
- auto: A / Contracts

## Assembly-CSharp/SentinelMission/SentinelScenario.cs
- L=340 kind=ScenarioModule type=SentinelScenario bases=ScenarioModule anc=ScenarioModule/MonoBehaviour ns=SentinelMission
- unity(36): Coroutine=5, Lifecycle=2, Instantiate/Destroy=1, GameObject=1 | ksp: Vessel=16, Planetarium=10, Orbit=7, FlightGlobals=4, ConfigNode=4 | fan_in=3
- methods(12): ActiveSentinelMatchingOrbit, GetDeployedSentinels, LoadGameDBSettings, OnAwake, OnDestroy, OnLoad, ProcessSentinelScan, ResetSentinels, SentinelAsteroidOrbit, SentinelRoutine, SpawnComet, Start
- auto: A / Contracts

## Assembly-CSharp/SentinelMission/SentinelUtilities.cs
- L=412 kind=static class type=SentinelUtilities bases=- anc=- ns=SentinelMission
- unity(0): none | ksp: CelestialBody=17, Planetarium=14, Vessel=5, Orbit=5, FlightGlobals=4 | fan_in=6
- methods(14): AdjustedSentinelViewAngle, CalculateReadDuration, FindInnerAndOuterBodies, GetEscapeVelocity, GetMinimumOrbitalSpeed, GetProgradeBurnAllowance, GetRetrogradeBurnAllowance, GetVesselClass, IsOnSolarOrbit, RandomRange, RandomScanType, SentinelCanScan, WeightedAsteroidClass, WeightedRandom
- auto: A / Contracts
