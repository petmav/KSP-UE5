using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

namespace CameraFXModules
{
	public class FlightCameraFX : MonoBehaviour
	{
		[Serializable]
		public class EngineVibrations
		{
			public float Generic;

			public float SolidBooster;

			public float LiquidFuel;

			public float Piston;

			public float Turbine;

			public float ScramJet;

			public float Electric;

			public float Nuclear;

			public float MonoProp;

			public float GetVibration(EngineType t)
			{
				switch (t)
				{
				default:
					return Generic;
				case EngineType.SolidBooster:
					return SolidBooster;
				case EngineType.LiquidFuel:
					return LiquidFuel;
				case EngineType.Piston:
					return Piston;
				case EngineType.Turbine:
					return Turbine;
				case EngineType.ScramJet:
					return ScramJet;
				case EngineType.Electric:
					return Electric;
				case EngineType.Nuclear:
					return Nuclear;
				case EngineType.MonoProp:
					return MonoProp;
				}
			}
		}

		private CameraFX host;

		private Wobble GForce;

		private float gForceScalar;

		private Wobble GroundRoll;

		private float groundRollScalar;

		private Wobble AtmoFlight;

		private float atmoFlightScalar;

		private Wobble EngineVibration;

		private List<IThrustProvider> vesselThrusters;

		private IThrustProvider iThr;

		private float totalThrust;

		private float thrustScalar;

		public WobbleFXParams fxGforce;

		public WobbleFXParams fxGroundRoll;

		public WobbleFXParams fxEngineVibration;

		public WobbleFXParams fxAtmoFlight;

		public FadeWobbleFXParams fxCollision;

		public FadeWobbleFXParams fxSplashdown;

		public FadeWobbleFXParams fxExplosion;

		public FadeWobbleFXParams fxOverHeat;

		public FadeWobbleFXParams fxJointBreak;

		public FadeWobbleFXParams fxSeparate;

		public MinMaxFloat rangeGroundRollSpeed;

		public MinMaxFloat rangeGForceMagnitude;

		public MinMaxFloat rangeExplosionDistance;

		public MinMaxFloat rangeSplashdownSpeed;

		public MinMaxFloat rangeCollisionRspd;

		public MinMaxFloat rangeEngineVibrationThrust;

		public MinMaxFloat rangeAtmosFlightIAS;

		public EngineVibrations engineVibrations;

		private IEnumerator Start()
		{
			while (!FlightGlobals.ready)
			{
				yield return null;
			}
			host = CameraFX.Instance;
			if (host != null)
			{
				GForce = new Wobble("GForce", 0f, 5f, WobbleModes.All, Views.FlightExternal | Views.FlightInternal, (float)GetInstanceID() * UnityEngine.Random.value, fxGforce.fxPars.rotFactor, fxGforce.fxPars.linFactor);
				host.FX.AddFX(GForce);
				GroundRoll = new Wobble("GroundRoll", 0f, 2f, WobbleModes.Y | WobbleModes.Pitch | WobbleModes.Roll, Views.FlightExternal | Views.FlightInternal, (float)GetInstanceID() * UnityEngine.Random.value, fxGroundRoll.fxPars.rotFactor, fxGroundRoll.fxPars.linFactor);
				host.FX.AddFX(GroundRoll);
				AtmoFlight = new Wobble("AtmoFlight", 0f, 0f, WobbleModes.X | WobbleModes.Y | WobbleModes.Roll, Views.FlightExternal | Views.FlightInternal, (float)GetInstanceID() * UnityEngine.Random.value, fxAtmoFlight.fxPars.rotFactor, fxAtmoFlight.fxPars.linFactor);
				host.FX.AddFX(AtmoFlight);
				EngineVibration = new Wobble("EngineVibration", 0.1f, 15f, WobbleModes.All, Views.FlightExternal | Views.FlightInternal, (float)GetInstanceID() * UnityEngine.Random.value, fxEngineVibration.fxPars.rotFactor, fxEngineVibration.fxPars.linFactor);
				host.FX.AddFX(EngineVibration);
				vesselThrusters = FlightGlobals.ActiveVessel.FindPartModulesImplementing<IThrustProvider>();
				GameEvents.onCollision.Add(OnVesselEvent);
				GameEvents.onCrash.Add(OnVesselEvent);
				GameEvents.onCrashSplashdown.Add(OnVesselEvent);
				GameEvents.onJointBreak.Add(OnVesselEvent);
				GameEvents.onStageSeparation.Add(OnVesselEvent);
				GameEvents.onOverheat.Add(OnVesselEvent);
				GameEvents.onOverPressure.Add(OnVesselEvent);
				GameEvents.onOverG.Add(OnVesselEvent);
				GameEvents.onUndock.Add(OnVesselEvent);
				GameEvents.onVesselSituationChange.Add(OnVesselSitChange);
				GameEvents.onPartExplode.Add(OnExplosion);
				GameEvents.onVesselWasModified.Add(OnVesselUpdate);
				GameEvents.onVesselChange.Add(OnVesselUpdate);
			}
		}

		private void OnDestroy()
		{
			if (host != null)
			{
				host.FX.RemoveFX(GForce);
				host.FX.RemoveFX(GroundRoll);
				host.FX.RemoveFX(AtmoFlight);
				host.FX.RemoveFX(EngineVibration);
				GameEvents.onCollision.Remove(OnVesselEvent);
				GameEvents.onCrash.Remove(OnVesselEvent);
				GameEvents.onCrashSplashdown.Remove(OnVesselEvent);
				GameEvents.onJointBreak.Remove(OnVesselEvent);
				GameEvents.onStageSeparation.Remove(OnVesselEvent);
				GameEvents.onOverheat.Remove(OnVesselEvent);
				GameEvents.onOverPressure.Remove(OnVesselEvent);
				GameEvents.onOverG.Remove(OnVesselEvent);
				GameEvents.onUndock.Remove(OnVesselEvent);
				GameEvents.onVesselSituationChange.Remove(OnVesselSitChange);
				GameEvents.onPartExplode.Remove(OnExplosion);
				GameEvents.onVesselWasModified.Remove(OnVesselUpdate);
				GameEvents.onVesselChange.Remove(OnVesselUpdate);
			}
		}

		private void LateUpdate()
		{
			if (!(host != null) || !FlightGlobals.ready)
			{
				return;
			}
			if (!MapView.MapIsEnabled && FlightGlobals.ActiveVessel.state != Vessel.State.DEAD)
			{
				gForceScalar = Mathf.Clamp01(Mathf.Pow(rangeGForceMagnitude.GetInverseLerp((float)FlightGlobals.ActiveVessel.geeForce), fxGforce.slope) + FlightGlobals.ActiveVessel.gThresh);
				GForce.amplitude = fxGforce.amplitude.GetLerp(gForceScalar);
				GForce.frequency = fxGforce.frequency.GetLerp(gForceScalar);
				groundRollScalar = (FlightGlobals.ActiveVessel.LandedOrSplashed ? Mathf.Pow(rangeGroundRollSpeed.GetInverseLerp((float)FlightGlobals.ActiveVessel.horizontalSrfSpeed), fxGroundRoll.slope) : 0f);
				GroundRoll.amplitude = fxGroundRoll.amplitude.GetLerp(groundRollScalar);
				GroundRoll.frequency = fxGroundRoll.frequency.GetLerp(groundRollScalar);
				atmoFlightScalar = Mathf.Clamp01((FlightGlobals.ActiveVessel.situation == Vessel.Situations.FLYING) ? Mathf.Pow(rangeAtmosFlightIAS.GetInverseLerp((float)FlightGlobals.ActiveVessel.indicatedAirSpeed), fxAtmoFlight.slope) : (0f + FlightGlobals.ActiveVessel.presThresh));
				AtmoFlight.amplitude = fxAtmoFlight.amplitude.GetLerp(atmoFlightScalar);
				AtmoFlight.frequency = fxAtmoFlight.frequency.GetLerp(atmoFlightScalar);
				totalThrust = 0f;
				int num = vesselThrusters.Count - 1;
				if (num >= 0)
				{
					for (int num2 = num; num2 >= 0; num2--)
					{
						iThr = vesselThrusters[num2];
						totalThrust += iThr.GetCurrentThrust() * engineVibrations.GetVibration(iThr.GetEngineType());
					}
					thrustScalar = Mathf.Pow(rangeEngineVibrationThrust.GetInverseLerp(totalThrust), fxEngineVibration.slope);
					EngineVibration.amplitude = fxEngineVibration.amplitude.GetLerp(thrustScalar);
					EngineVibration.frequency = fxEngineVibration.frequency.GetLerp(thrustScalar);
				}
				else
				{
					EngineVibration.amplitude = 0f;
				}
			}
			else
			{
				GForce.amplitude = 0f;
				GroundRoll.amplitude = 0f;
				AtmoFlight.amplitude = 0f;
				EngineVibration.amplitude = 0f;
			}
		}

		private void OnVesselEvent(EventReport evt)
		{
			if (evt.origin.vessel == FlightGlobals.ActiveVessel)
			{
				switch (evt.eventType)
				{
				case FlightEvents.COLLISION:
				{
					float num = Mathf.Clamp01(rangeCollisionRspd.GetInverseLerp(evt.param));
					host.FX.AddFX(new Fade(new Wobble("Thump", fxCollision.wobblePars.amplitude.max * num, fxCollision.wobblePars.frequency.max, WobbleModes.All, Views.All, Time.realtimeSinceStartup * UnityEngine.Random.value, fxCollision.wobblePars.fxPars.rotFactor, fxCollision.wobblePars.fxPars.linFactor), fxCollision.duration * num, fxCollision.falloff));
					break;
				}
				case FlightEvents.OVERHEAT:
				case FlightEvents.OVERPRESSURE:
				case FlightEvents.OVERG:
					host.FX.AddFX(new Fade(new Wobble("Overheat", fxOverHeat.wobblePars.amplitude.max, fxOverHeat.wobblePars.frequency.max, WobbleModes.All, Views.All, Time.realtimeSinceStartup * UnityEngine.Random.value, fxOverHeat.wobblePars.fxPars.rotFactor, fxOverHeat.wobblePars.fxPars.linFactor), fxOverHeat.duration, fxOverHeat.falloff));
					break;
				case FlightEvents.STAGESEPARATION:
				case FlightEvents.UNDOCK:
					host.FX.AddFX(new Fade(new Wobble("StageThump", fxSeparate.wobblePars.amplitude.max, fxSeparate.wobblePars.frequency.max, WobbleModes.All, Views.All, Time.realtimeSinceStartup * UnityEngine.Random.value, fxSeparate.wobblePars.fxPars.rotFactor, fxSeparate.wobblePars.fxPars.linFactor), fxSeparate.duration, fxSeparate.falloff));
					break;
				case FlightEvents.JOINTBREAK:
					host.FX.AddFX(new Fade(new Wobble("JointBreakThump", fxJointBreak.wobblePars.amplitude.max, fxJointBreak.wobblePars.frequency.max, WobbleModes.All, Views.All, Time.realtimeSinceStartup * UnityEngine.Random.value, fxJointBreak.wobblePars.fxPars.rotFactor, fxJointBreak.wobblePars.fxPars.linFactor), fxJointBreak.duration, fxJointBreak.falloff));
					break;
				}
			}
		}

		private void OnVesselSitChange(GameEvents.HostedFromToAction<Vessel, Vessel.Situations> vSt)
		{
			if (vSt.host == FlightGlobals.ActiveVessel && vSt.to != vSt.from && vSt.from == Vessel.Situations.FLYING && vSt.to == Vessel.Situations.SPLASHED)
			{
				float num = Mathf.Clamp01(rangeSplashdownSpeed.GetInverseLerp((float)Math.Abs(vSt.host.verticalSpeed)));
				host.FX.AddFX(new Fade(new Wobble("Splashdown", fxSplashdown.wobblePars.amplitude.max * num, fxSplashdown.wobblePars.frequency.max, WobbleModes.All, Views.All, Time.realtimeSinceStartup * UnityEngine.Random.value, fxSplashdown.wobblePars.fxPars.rotFactor, fxSplashdown.wobblePars.fxPars.linFactor), fxSplashdown.duration, fxSplashdown.falloff));
			}
		}

		private void OnExplosion(GameEvents.ExplosionReaction exp)
		{
			if (exp.distance < rangeExplosionDistance.max)
			{
				float num = Mathf.Clamp01(rangeExplosionDistance.GetInverseLerp(exp.distance));
				host.FX.AddFX(new Fade(new Wobble("Explosion", fxExplosion.wobblePars.amplitude.max * exp.magnitude * num, fxExplosion.wobblePars.frequency.max, WobbleModes.All, Views.All, Time.realtimeSinceStartup * UnityEngine.Random.value, fxExplosion.wobblePars.fxPars.rotFactor, fxExplosion.wobblePars.fxPars.linFactor), fxExplosion.duration, fxExplosion.falloff));
			}
		}

		private void OnVesselUpdate(Vessel v)
		{
			if (v == FlightGlobals.ActiveVessel)
			{
				vesselThrusters = v.FindPartModulesImplementing<IThrustProvider>();
			}
		}
	}
}
