using KSP.Localization;
using UnityEngine;

public class TutorialFlightBasics : TutorialScenario
{
	private Texture2D YPRDiagram;

	private Texture2D chuteDiagram;

	private TutorialPage welcome;

	private TutorialPage basicControls;

	private TutorialPage pitch;

	private TutorialPage yaw;

	private TutorialPage roll;

	private TutorialPage PYRsummary;

	private TutorialPage throttle;

	private TutorialPage basicControlsSummary;

	private TutorialPage SAS;

	private TutorialPage SAS2;

	private TutorialPage RCS;

	private TutorialPage staging;

	private TutorialPage navball;

	private TutorialPage altimeter;

	private TutorialPage chuteColors;

	private TutorialPage readyToLaunch;

	private TutorialPage ivaBonusPage;

	private TutorialPage deathBonusPage;

	private TutorialPage dockingCtrlBonusPage;

	private TutorialPage shipLaunched;

	private TutorialPage readyChute;

	private TutorialPage chuteBurnBonusPage;

	private TutorialPage coastDown;

	private TutorialPage waitToSlow;

	private TutorialPage landed;

	private KFSMEvent fsmEvent;

	private bool deadPage;

	private ModuleEngines srb;

	private ModuleParachute chute;

	protected override void OnAssetSetup()
	{
		instructorPrefabName = "Instructor_Gene";
		YPRDiagram = GameDatabase.Instance.GetTexture("Squad/Tutorials/YPRDiagram", asNormalMap: false);
		chuteDiagram = GameDatabase.Instance.GetTexture("Squad/Tutorials/ChuteColors", asNormalMap: false);
	}

	protected override void OnTutorialSetup()
	{
		welcome = new TutorialPage("welcome");
		welcome.windowTitle = Localizer.Format("#autoLOC_314461");
		welcome.OnEnter = delegate
		{
			InputLockManager.SetControlLock(~(ControlTypes.UI | ControlTypes.PAUSE | ControlTypes.CAMERAMODES | ControlTypes.CAMERACONTROLS), "flightTutorial");
			instructor.StopRepeatingEmote();
		};
		welcome.dialog = new MultiOptionDialog(welcome.name, "", "", null, dRect, new DialogGUIVerticalLayout(sw: true), new DialogGUILabel(Localizer.Format("#autoLOC_314470", Localizer.Format(instructor.CharacterName), tutorialHighlightColorString), expandW: false, expandH: true), new DialogGUIButton(Localizer.Format("#autoLOC_314471"), delegate
		{
			Tutorial.GoToNextPage();
		}, dismissOnSelect: true));
		Tutorial.AddPage(welcome);
		basicControls = new TutorialPage("basic controls");
		basicControls.windowTitle = Localizer.Format("#autoLOC_314482");
		basicControls.OnEnter = delegate
		{
			instructor.PlayEmote(instructor.anim_idle_sigh);
		};
		basicControls.dialog = new MultiOptionDialog(basicControls.name, "", "", null, dRect, new DialogGUIVerticalLayout(sw: true), new DialogGUILabel(Localizer.Format("#autoLOC_314489"), expandW: false, expandH: true), new DialogGUIButton(Localizer.Format("#autoLOC_314490"), delegate
		{
			Tutorial.GoToNextPage();
		}, dismissOnSelect: true));
		Tutorial.AddPage(basicControls);
		pitch = new TutorialPage("yprdiag");
		pitch.windowTitle = Localizer.Format("#autoLOC_314502");
		pitch.OnEnter = delegate
		{
			InputLockManager.SetControlLock(~(ControlTypes.UI | ControlTypes.PAUSE | ControlTypes.CAMERAMODES | ControlTypes.CAMERACONTROLS), "flightTutorial");
		};
		pitch.dialog = new MultiOptionDialog(pitch.name, "", "", null, dRect, new DialogGUIVerticalLayout(sw: true), new DialogGUILabel(Localizer.Format("#autoLOC_314509"), expandW: false, expandH: true), new DialogGUIImage(new Vector2(256f, 256f), Vector2.zero, Color.white, YPRDiagram), new DialogGUILabel(Localizer.Format("#autoLOC_314511"), expandW: false, expandH: true), new DialogGUIButton(Localizer.Format("#autoLOC_314512"), delegate
		{
			Tutorial.GoToNextPage();
		}, dismissOnSelect: true));
		Tutorial.AddPage(pitch);
		PYRsummary = new TutorialPage("PYRsummary");
		PYRsummary.windowTitle = Localizer.Format("#autoLOC_314523");
		PYRsummary.OnEnter = delegate
		{
			InputLockManager.SetControlLock(~(ControlTypes.UI | ControlTypes.PITCH | ControlTypes.ROLL | ControlTypes.YAW | ControlTypes.PAUSE | ControlTypes.CAMERAMODES | ControlTypes.CAMERACONTROLS), "flightTutorial");
			instructor.PlayEmote(instructor.anim_true_nodA);
		};
		PYRsummary.dialog = new MultiOptionDialog(PYRsummary.name, "", "", null, dRect, new DialogGUIVerticalLayout(sw: true), new DialogGUILabel(Localizer.Format("#autoLOC_314532", tutorialControlColorString, GameSettings.PITCH_UP.name, tutorialControlColorString, GameSettings.PITCH_DOWN.name, tutorialControlColorString, GameSettings.YAW_LEFT.name, tutorialControlColorString, GameSettings.YAW_RIGHT.name, tutorialControlColorString, GameSettings.ROLL_LEFT.name, tutorialControlColorString, GameSettings.ROLL_RIGHT.name), expandW: false, expandH: true), new DialogGUIButton(Localizer.Format("#autoLOC_314533"), delegate
		{
			Tutorial.GoToNextPage();
		}, dismissOnSelect: true));
		Tutorial.AddPage(PYRsummary);
		basicControlsSummary = new TutorialPage("basicControlsSummary");
		basicControlsSummary.windowTitle = Localizer.Format("#autoLOC_314544");
		basicControlsSummary.OnEnter = delegate
		{
			InputLockManager.SetControlLock(~(ControlTypes.UI | ControlTypes.PITCH | ControlTypes.ROLL | ControlTypes.YAW | ControlTypes.THROTTLE | ControlTypes.PAUSE | ControlTypes.CAMERAMODES | ControlTypes.CAMERACONTROLS | ControlTypes.THROTTLE_CUT_MAX), "flightTutorial");
		};
		basicControlsSummary.dialog = new MultiOptionDialog(basicControlsSummary.name, "", "", null, dRect, new DialogGUIVerticalLayout(sw: true), new DialogGUILabel(Localizer.Format("#autoLOC_314551"), expandW: false, expandH: true), new DialogGUIButton(Localizer.Format("#autoLOC_314552"), delegate
		{
			Tutorial.GoToNextPage();
		}, dismissOnSelect: true));
		Tutorial.AddPage(basicControlsSummary);
		staging = new TutorialPage("staging");
		staging.windowTitle = Localizer.Format("#autoLOC_314563");
		staging.OnEnter = delegate
		{
			instructor.PlayEmote(instructor.anim_true_nodB);
		};
		staging.dialog = new MultiOptionDialog(staging.name, "", "", null, dRect, new DialogGUIVerticalLayout(sw: true), new DialogGUILabel(Localizer.Format("#autoLOC_314570", tutorialControlColorString, GameSettings.LAUNCH_STAGES.name), expandW: false, expandH: true), new DialogGUIButton(Localizer.Format("#autoLOC_314571"), delegate
		{
			Tutorial.GoToNextPage();
		}, dismissOnSelect: true));
		Tutorial.AddPage(staging);
		navball = new TutorialPage("navball");
		navball.windowTitle = Localizer.Format("#autoLOC_314582");
		navball.OnEnter = delegate
		{
		};
		navball.dialog = new MultiOptionDialog(navball.name, "", "", null, dRect, new DialogGUIVerticalLayout(sw: true), new DialogGUILabel(Localizer.Format("#autoLOC_314589"), expandW: false, expandH: true), new DialogGUIButton(Localizer.Format("#autoLOC_314590"), delegate
		{
			Tutorial.GoToNextPage();
		}, dismissOnSelect: true));
		Tutorial.AddPage(navball);
		altimeter = new TutorialPage("altimeter");
		altimeter.windowTitle = Localizer.Format("#autoLOC_314601");
		altimeter.OnEnter = delegate
		{
			instructor.PlayEmote(instructor.anim_idle_lookAround);
		};
		altimeter.dialog = new MultiOptionDialog(altimeter.name, "", "", null, dRect, new DialogGUIVerticalLayout(sw: true), new DialogGUILabel(Localizer.Format("#autoLOC_314608"), expandW: false, expandH: true), new DialogGUIButton(Localizer.Format("#autoLOC_314609"), delegate
		{
			Tutorial.GoToNextPage();
		}, dismissOnSelect: true));
		Tutorial.AddPage(altimeter);
		chuteColors = new TutorialPage("chuteColors");
		chuteColors.windowTitle = Localizer.Format("#autoLOC_314620");
		chuteColors.OnEnter = delegate
		{
		};
		chuteColors.dialog = new MultiOptionDialog(chuteColors.name, "", "", null, dRect, new DialogGUIVerticalLayout(sw: true), new DialogGUILabel(Localizer.Format("#autoLOC_314627"), expandW: false, expandH: true), new DialogGUIImage(new Vector2(192f, 32f), Vector2.zero, Color.white, chuteDiagram), new DialogGUILabel(Localizer.Format("#autoLOC_314629"), expandW: false, expandH: true), new DialogGUIButton(Localizer.Format("#autoLOC_314630"), delegate
		{
			Tutorial.GoToNextPage();
		}, dismissOnSelect: true));
		Tutorial.AddPage(chuteColors);
		readyToLaunch = new TutorialPage("readyToLaunch");
		readyToLaunch.windowTitle = Localizer.Format("#autoLOC_314642");
		readyToLaunch.OnEnter = delegate
		{
			InputLockManager.SetControlLock(ControlTypes.STAGING, "flightTutorial");
			HighLogic.CurrentGame.Parameters.Flight.CanIVA = true;
			instructor.PlayEmote(instructor.anim_true_thumbsUp);
			if (FlightGlobals.ActiveVessel.ctrlState.killRot)
			{
				FlightGlobals.ActiveVessel.ctrlState.killRot = false;
				FlightGlobals.ActiveVessel.ActionGroups.ToggleGroup(KSPActionGroup.SAS);
			}
			FlightGlobals.ActiveVessel.ActionGroups[KSPActionGroup.SAS] = false;
			InputLockManager.RemoveControlLock("flightTutorial");
			InputLockManager.SetControlLock(ControlTypes.SAS, "onFlightTutorial");
		};
		readyToLaunch.dialog = new MultiOptionDialog(readyToLaunch.name, "", "", null, dRect, new DialogGUIVerticalLayout(sw: true), new DialogGUILabel(Localizer.Format("#autoLOC_314660", tutorialControlColorString, GameSettings.PAUSE.name, tutorialHighlightColorString, tutorialControlColorString, GameSettings.LAUNCH_STAGES.name), expandW: false, expandH: true));
		readyToLaunch.SetAdvanceCondition((KFSMState KFSMEvent) => FlightGlobals.ActiveVessel.situation == Vessel.Situations.FLYING);
		Tutorial.AddPage(readyToLaunch);
		shipLaunched = new TutorialPage("shipLaunched");
		shipLaunched.windowTitle = Localizer.Format("#autoLOC_314671");
		shipLaunched.OnEnter = delegate
		{
			Vessel activeVessel3 = FlightGlobals.ActiveVessel;
			int count2 = activeVessel3.Parts.Count;
			while (count2-- > 0)
			{
				Part part2 = activeVessel3.Parts[count2];
				if (part2.partInfo.name.Contains("solidBooster"))
				{
					srb = part2.Modules.GetModule<ModuleEngines>();
				}
				if (part2.partInfo.name == "parachuteSingle")
				{
					chute = part2.Modules.GetModule<ModuleParachute>();
				}
			}
			instructor.PlayEmote(instructor.anim_true_thumbsUp);
		};
		shipLaunched.dialog = new MultiOptionDialog(shipLaunched.name, "", "", null, dRect, new DialogGUIVerticalLayout(sw: true), new DialogGUILabel(Localizer.Format("#autoLOC_314692", tutorialControlColorString, GameSettings.PAUSE.name, tutorialHighlightColorString), expandW: false, expandH: true));
		shipLaunched.SetAdvanceCondition(delegate
		{
			Vessel activeVessel2 = FlightGlobals.ActiveVessel;
			return srb == null || chute == null || (activeVessel2.verticalSpeed < 10.0 && srb.flameout && srb.EngineIgnited && srb.propellantReqMet < 1f && activeVessel2.missionTime > 5.0) || chute.deploymentState == ModuleParachute.deploymentStates.DEPLOYED || chute.deploymentState == ModuleParachute.deploymentStates.SEMIDEPLOYED;
		});
		Tutorial.AddPage(shipLaunched);
		readyChute = new TutorialPage("readyChute");
		readyChute.windowTitle = Localizer.Format("#autoLOC_314708");
		readyChute.OnEnter = delegate
		{
			instructor.PlayEmote(instructor.anim_idle_lookAround);
		};
		readyChute.dialog = new MultiOptionDialog(readyChute.name, "", "", null, dRect, new DialogGUIVerticalLayout(sw: true), new DialogGUILabel(Localizer.Format("#autoLOC_314715"), expandW: false, expandH: true));
		readyChute.SetAdvanceCondition((KFSMState KFSMEvent) => chute == null || chute.deploymentState != ModuleParachute.deploymentStates.STOWED);
		Tutorial.AddPage(readyChute);
		waitToSlow = new TutorialPage("waitToSlow");
		waitToSlow.windowTitle = Localizer.Format("#autoLOC_314727");
		waitToSlow.OnEnter = delegate
		{
			instructor.PlayEmote(instructor.anim_idle_lookAround);
		};
		waitToSlow.dialog = new MultiOptionDialog(waitToSlow.name, "", "", null, dRect, new DialogGUIVerticalLayout(sw: true), new DialogGUILabel(Localizer.Format("#autoLOC_314734"), expandW: false, expandH: true));
		waitToSlow.SetAdvanceCondition((KFSMState KFSMEvent) => chute == null || (chute.deploymentState != 0 && chute.deploymentState != ModuleParachute.deploymentStates.ACTIVE));
		Tutorial.AddPage(waitToSlow);
		coastDown = new TutorialPage("coastDown");
		coastDown.windowTitle = Localizer.Format("#autoLOC_314746");
		coastDown.OnEnter = delegate
		{
			instructor.PlayEmote(instructor.anim_true_thumbsUp);
		};
		coastDown.dialog = new MultiOptionDialog(coastDown.name, "", "", null, dRect, new DialogGUIVerticalLayout(sw: true), new DialogGUILabel(Localizer.Format("#autoLOC_314754"), expandW: false, expandH: true));
		coastDown.SetAdvanceCondition((KFSMState KFSMEvent) => FlightGlobals.ActiveVessel.situation == Vessel.Situations.LANDED || FlightGlobals.ActiveVessel.situation == Vessel.Situations.SPLASHED);
		Tutorial.AddPage(coastDown);
		landed = new TutorialPage("landed");
		landed.windowTitle = Localizer.Format("#autoLOC_314766");
		landed.OnEnter = delegate
		{
			instructor.PlayEmote(instructor.anim_true_thumbUp);
			HighLogic.CurrentGame.Parameters.Flight.CanEVA = true;
		};
		landed.dialog = new MultiOptionDialog(landed.name, "", "", null, dRect, new DialogGUIVerticalLayout(sw: true), new DialogGUILabel(Localizer.Format("#autoLOC_314775", tutorialControlColorString, GameSettings.PAUSE.name, tutorialHighlightColorString), expandW: false, expandH: true), new DialogGUIButton(Localizer.Format("#autoLOC_314776"), delegate
		{
			CompleteTutorial();
		}, dismissOnSelect: true));
		Tutorial.AddPage(landed);
		ivaBonusPage = new TutorialPage("ivaBonusPage");
		ivaBonusPage.windowTitle = Localizer.Format("#autoLOC_314788");
		ivaBonusPage.OnEnter = delegate
		{
			ivaBonusPage.onAdvanceConditionMet.GoToStateOnEvent = Tutorial.lastPage;
			instructor.PlayEmote(instructor.anim_idle_lookAround);
		};
		ivaBonusPage.dialog = new MultiOptionDialog(ivaBonusPage.name, "", "", null, dRect, new DialogGUIVerticalLayout(sw: true), new DialogGUILabel(Localizer.Format("#autoLOC_314800", tutorialControlColorString, GameSettings.CAMERA_NEXT.name, tutorialControlColorString, GameSettings.CAMERA_MODE.name), expandW: false, expandH: true));
		ivaBonusPage.SetAdvanceCondition(delegate
		{
			if (CameraManager.Instance.currentCameraMode != CameraManager.CameraMode.Internal && CameraManager.Instance.currentCameraMode != CameraManager.CameraMode.IVA)
			{
				Object.DestroyImmediate(dialogDisplay.gameObject);
				dialogDisplay = null;
				return true;
			}
			return false;
		});
		Tutorial.AddState(ivaBonusPage);
		deathBonusPage = new TutorialPage("deathBonusPage");
		deathBonusPage.windowTitle = Localizer.Format("#autoLOC_314820");
		deathBonusPage.OnEnter = delegate
		{
			InputLockManager.SetControlLock(ControlTypes.STAGING, "flightTutorial");
			deathBonusPage.onAdvanceConditionMet.GoToStateOnEvent = Tutorial.lastPage;
			instructor.PlayEmote(instructor.anim_false_disappointed);
			deadPage = true;
		};
		deathBonusPage.dialog = new MultiOptionDialog(deathBonusPage.name, "", "", null, dRect, new DialogGUIVerticalLayout(sw: true), new DialogGUILabel(Localizer.Format("#autoLOC_314830"), expandW: false, expandH: true), new DialogGUIButton(Localizer.Format("#autoLOC_314831"), delegate
		{
			CloseTutorialWindow();
		}, dismissOnSelect: true));
		deathBonusPage.SetAdvanceCondition((KFSMState st) => FlightGlobals.ActiveVessel.state != Vessel.State.DEAD);
		Tutorial.AddState(deathBonusPage);
		chuteBurnBonusPage = new TutorialPage("chuteBurnBonusPage");
		chuteBurnBonusPage.windowTitle = Localizer.Format("#autoLOC_314847");
		chuteBurnBonusPage.onAdvanceConditionMet.GoToStateOnEvent = deathBonusPage;
		chuteBurnBonusPage.OnEnter = delegate
		{
			InputLockManager.SetControlLock(ControlTypes.STAGING, "flightTutorial");
			chuteBurnBonusPage.onAdvanceConditionMet.GoToStateOnEvent = deathBonusPage;
			instructor.PlayEmote(instructor.anim_false_disappointed);
		};
		chuteBurnBonusPage.dialog = new MultiOptionDialog(chuteBurnBonusPage.name, "", "", null, dRect, new DialogGUIVerticalLayout(sw: true), new DialogGUILabel(Localizer.Format("#autoLOC_314857"), expandW: false, expandH: true));
		chuteBurnBonusPage.SetAdvanceCondition(delegate
		{
			if (FlightGlobals.ActiveVessel.state == Vessel.State.DEAD)
			{
				Object.DestroyImmediate(dialogDisplay.gameObject);
				dialogDisplay = null;
				return true;
			}
			return false;
		});
		Tutorial.AddState(chuteBurnBonusPage);
		dockingCtrlBonusPage = new TutorialPage("dockingCtrlBonusPage");
		dockingCtrlBonusPage.windowTitle = Localizer.Format("#autoLOC_314876");
		dockingCtrlBonusPage.OnEnter = delegate
		{
			InputLockManager.SetControlLock(ControlTypes.STAGING, "flightTutorial");
			dockingCtrlBonusPage.onAdvanceConditionMet.GoToStateOnEvent = Tutorial.lastPage;
			instructor.PlayEmote(instructor.anim_idle_lookAround);
		};
		dockingCtrlBonusPage.dialog = new MultiOptionDialog(dockingCtrlBonusPage.name, "", "", null, dRect, new DialogGUIVerticalLayout(sw: true), new DialogGUILabel(Localizer.Format("#autoLOC_6005086"), expandW: false, expandH: true));
		dockingCtrlBonusPage.SetAdvanceCondition(delegate
		{
			if (FlightUIModeController.Instance.Mode != FlightUIMode.DOCKING)
			{
				Object.DestroyImmediate(dialogDisplay.gameObject);
				dialogDisplay = null;
				return true;
			}
			return false;
		});
		Tutorial.AddState(dockingCtrlBonusPage);
		fsmEvent = new KFSMEvent("Found IVA Mode");
		fsmEvent.GoToStateOnEvent = ivaBonusPage;
		fsmEvent.OnCheckCondition = (KFSMState st) => CameraManager.Instance.currentCameraMode == CameraManager.CameraMode.IVA;
		Tutorial.AddEventExcluding(fsmEvent, ivaBonusPage);
		fsmEvent = new KFSMEvent("Died");
		fsmEvent.GoToStateOnEvent = deathBonusPage;
		fsmEvent.OnCheckCondition = (KFSMState st) => FlightGlobals.ActiveVessel.state == Vessel.State.DEAD;
		Tutorial.AddEventExcluding(fsmEvent, deathBonusPage, chuteBurnBonusPage);
		fsmEvent = new KFSMEvent("ChuteBurn");
		fsmEvent.GoToStateOnEvent = chuteBurnBonusPage;
		fsmEvent.OnCheckCondition = delegate
		{
			Vessel activeVessel = FlightGlobals.ActiveVessel;
			if (!deadPage && activeVessel.state != Vessel.State.DEAD)
			{
				if (activeVessel.situation == Vessel.Situations.FLYING && activeVessel.srfSpeed >= 100.0)
				{
					ModuleParachute moduleParachute = null;
					int count = activeVessel.Parts.Count;
					while (count-- > 0)
					{
						Part part = activeVessel.Parts[count];
						if (part.partInfo.name == "parachuteSingle")
						{
							moduleParachute = part.Modules.GetModule<ModuleParachute>();
							break;
						}
					}
					if (moduleParachute == null)
					{
						return false;
					}
					return moduleParachute.deploymentState == ModuleParachute.deploymentStates.CUT;
				}
				return false;
			}
			return false;
		};
		Tutorial.AddEventExcluding(fsmEvent, chuteBurnBonusPage, deathBonusPage);
		fsmEvent = new KFSMEvent("Found Docking Mode");
		fsmEvent.GoToStateOnEvent = dockingCtrlBonusPage;
		fsmEvent.OnCheckCondition = (KFSMState st) => FlightUIModeController.Instance.Mode == FlightUIMode.DOCKING;
		Tutorial.AddEventExcluding(fsmEvent, dockingCtrlBonusPage, ivaBonusPage);
		Tutorial.StartTutorial(welcome);
	}

	protected override void OnOnDestroy()
	{
		HighLogic.CurrentGame.Parameters.Flight.CanEVA = true;
	}
}
