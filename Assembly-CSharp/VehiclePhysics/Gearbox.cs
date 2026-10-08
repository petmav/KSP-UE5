using System;
using EdyCommonTools;
using UnityEngine;
using UnityEngine.Serialization;

namespace VehiclePhysics
{
	public class Gearbox : Block
	{
		public enum Type
		{
			Manual,
			Automatic
		}

		public enum AutomaticGear
		{
			M,
			P,
			R,
			N,
			D,
			L,
			_Count
		}

		[Serializable]
		public class Settings
		{
			public Type type;

			public float[] forwardGearRatios = new float[5] { 3.84f, 2.35f, 1.53f, 1.1f, 0.84f };

			public float[] reverseGearRatios = new float[1] { -3.44f };

			public float auto2ndGearMinSpeed = 8f;

			public bool parkRequiresStopAndBrakes;

			public bool allowParkInManual;

			[Range(0f, 2f)]
			public float manualShiftTime = 0.25f;

			public bool autoShift = true;

			public float autoShiftNeutralRpm = 800f;

			public float autoShiftFirstGearRpm = 1000f;

			public float autoShiftDownRevs = 1900f;

			public float autoShiftUpRevs = 3500f;

			[Range(0f, 2f)]
			public float autoShiftUpInterval = 1f;

			[Range(0f, 2f)]
			public float autoShiftDownInterval = 0.3f;

			[Range(0f, 2f)]
			public float automaticTransitionTime = 0.5f;

			[Range(0f, 2f)]
			public float automaticShiftInterval = 1f;

			public float automaticGearDownRevs = 1900f;

			public float automaticGearUpRevs = 3500f;

			public bool automaticShiftReverseGears;

			[FormerlySerializedAs("automaticStartGear")]
			public int automaticInitialGearForward = 1;

			public int automaticInitialGearReverse = -1;
		}

		public enum AutoShiftOverride
		{
			None,
			ForceAutoShift,
			ForceManualShift
		}

		[Serializable]
		public struct StateVars
		{
			public float L;

			public int manualGear;

			public float manualRatio;

			public bool engaged;

			public float lastEngagedTime;

			public float lastAutoShiftTime;

			public bool vehicleMoving;

			public AutomaticGear gearMode;

			public int automaticGear;

			public float automaticRatio;

			public bool transition;

			public float transitionStartedTime;

			public int fromGear;

			public int toGear;

			public float fromRatio;

			public float toRatio;

			public float transitionRatio;
		}

		public int manualGearInput;

		public int automaticGearInput;

		public int gearShiftInput;

		public bool bypassAutoShift;

		public float stateVehicleSpeed = 999f;

		public float stateVehicleBrakes = 1f;

		public float stateVehicleThrottle = 1f;

		public Action signalSwitchingGears;

		public Settings settings = new Settings();

		public float inputToRpmRatio = 1f;

		public float damping = 0.95f;

		public float pedalSoftPressThreshold = 0.1f;

		public float pedalHardPressThreshold = 0.6f;

		public float inverseSpeedThreshold = 1f;

		public float vehicleMovingThresholdMin = 0.1f;

		public float vehicleMovingThresholdMax = 1f;

		private Connection m_input;

		private Connection m_output;

		private float I = 0.001f;

		private float T;

		private StateVars m_stateVars = new StateVars
		{
			L = 0f,
			manualGear = 0,
			manualRatio = 0f,
			engaged = false,
			lastEngagedTime = 0f,
			lastAutoShiftTime = 0f,
			vehicleMoving = false,
			gearMode = AutomaticGear.M,
			automaticGear = 0,
			automaticRatio = 0f,
			transition = false,
			transitionStartedTime = 0f,
			fromGear = 0,
			toGear = 0,
			fromRatio = 0f,
			toRatio = 0f,
			transitionRatio = 0f
		};

		private bool m_isSwitchingGears;

		public int sensorGearMode => (int)m_stateVars.gearMode;

		public int sensorEngagedGear => EngagedGear();

		public bool sensorSwitchingGears => m_isSwitchingGears;

		public float sensorOutputRpm
		{
			get
			{
				if (m_output == null)
				{
					return 0f;
				}
				return m_output.L / m_output.I * Block.WToRpm;
			}
		}

		public AutoShiftOverride autoShiftOverride { get; set; }

		public void Reset()
		{
			m_stateVars.gearMode = AutomaticGear.M;
			m_stateVars.manualGear = 0;
			m_stateVars.engaged = false;
			m_stateVars.vehicleMoving = false;
			m_stateVars.automaticGear = 0;
			m_stateVars.transition = false;
			m_isSwitchingGears = false;
			m_stateVars.lastEngagedTime = 0f;
			m_stateVars.lastAutoShiftTime = 0f;
			m_stateVars.transitionStartedTime = 0f;
		}

		public float GetCurrentGearRatio()
		{
			if (settings.type == Type.Manual)
			{
				if (m_stateVars.engaged)
				{
					return m_stateVars.manualRatio;
				}
			}
			else if (m_stateVars.transition)
			{
				if (m_stateVars.fromGear == 0)
				{
					return m_stateVars.toRatio;
				}
				if (m_stateVars.toGear != 0 && Mathf.Sign(m_stateVars.fromRatio) == Mathf.Sign(m_stateVars.toRatio))
				{
					return m_stateVars.transitionRatio;
				}
			}
			else if (m_stateVars.automaticGear != 0)
			{
				return m_stateVars.automaticRatio;
			}
			return float.NaN;
		}

		public float GetGearRatio(int gear)
		{
			if (gear == 0)
			{
				return float.NaN;
			}
			return GearRatio(ClampGear(gear));
		}

		public StateVars GetStateVars()
		{
			return m_stateVars;
		}

		public void SetStateVars(StateVars stateVars)
		{
			m_stateVars = stateVars;
		}

		protected override void Initialize()
		{
			SetInputs(1);
			SetOutputs(1);
		}

		public override bool CheckConnections()
		{
			m_input = base.inputs[0];
			m_output = base.outputs[0];
			if (m_input != null)
			{
				return m_output != null;
			}
			return false;
		}

		public override void PreStep()
		{
			if (settings.type == Type.Manual)
			{
				StandardProcessInput();
			}
			else
			{
				PlanetaryProcessInput();
			}
			bypassAutoShift = false;
			m_isSwitchingGears = IsSwitchingGears();
			if (m_isSwitchingGears && signalSwitchingGears != null)
			{
				signalSwitchingGears();
			}
		}

		public override void GetState(ref State S)
		{
			S.L = m_stateVars.L;
		}

		public override void SetSubstepState(State S)
		{
			m_stateVars.L = S.L;
		}

		public override void ComputeStateUpstream()
		{
			if (m_stateVars.gearMode == AutomaticGear.P)
			{
				m_input.L = m_stateVars.L;
				m_input.I = I;
				m_input.Tr = 0f;
			}
			else if (settings.type == Type.Manual)
			{
				StandardComputeState();
			}
			else
			{
				PlanetaryComputeState();
			}
		}

		public override void EvaluateTorqueDownstream()
		{
			if (m_stateVars.gearMode == AutomaticGear.P)
			{
				m_output.outTd = (0f - m_output.L) / Solver.deltaTime - m_output.Tr * damping;
				T = m_input.outTd;
			}
			else if (settings.type == Type.Manual)
			{
				StandardEvaluateTorque();
			}
			else
			{
				PlanetaryEvaluateTorque();
			}
		}

		public override void GetSubstepDerivative(ref Derivative D)
		{
			D.T = T;
		}

		public override void SetState(State S)
		{
			m_stateVars.L = S.L;
		}

		private bool CanEngageParkMode()
		{
			if (m_stateVars.gearMode == AutomaticGear.P)
			{
				return true;
			}
			if (settings.parkRequiresStopAndBrakes)
			{
				if (Mathf.Abs(stateVehicleSpeed) < 0.01f && stateVehicleBrakes > pedalHardPressThreshold)
				{
					return true;
				}
				automaticGearInput = (int)m_stateVars.gearMode;
				return false;
			}
			return true;
		}

		private bool CanDisengageParkMode()
		{
			if (m_stateVars.gearMode != AutomaticGear.P)
			{
				return true;
			}
			if (settings.parkRequiresStopAndBrakes)
			{
				if (stateVehicleBrakes > pedalHardPressThreshold)
				{
					return true;
				}
				automaticGearInput = 1;
				return false;
			}
			return true;
		}

		private float GearRatio(int gear)
		{
			float result = 0f;
			if (gear > 0)
			{
				result = settings.forwardGearRatios[gear - 1];
			}
			else if (gear < 0)
			{
				result = settings.reverseGearRatios[-gear - 1];
			}
			return result;
		}

		private int ClampGear(int gear)
		{
			if (gear > 0)
			{
				int num = settings.forwardGearRatios.Length;
				if (gear > num)
				{
					gear = num;
				}
			}
			else if (gear < 0)
			{
				int num2 = settings.reverseGearRatios.Length;
				if (gear < -num2)
				{
					gear = -num2;
				}
			}
			return gear;
		}

		private int ClampReverseGear(int gear)
		{
			if (gear >= 0)
			{
				gear = -1;
			}
			int num = settings.reverseGearRatios.Length;
			if (gear < -num)
			{
				gear = -num;
			}
			return gear;
		}

		private int ClampForwardGear(int gear)
		{
			if (gear <= 0)
			{
				gear = 1;
			}
			int num = settings.forwardGearRatios.Length;
			if (gear > num)
			{
				gear = num;
			}
			return gear;
		}

		private int EngagedGear()
		{
			if (settings.type == Type.Manual)
			{
				if (!m_stateVars.engaged)
				{
					return 0;
				}
				return m_stateVars.manualGear;
			}
			return m_stateVars.automaticGear;
		}

		private bool IsSwitchingGears()
		{
			if (settings.type == Type.Manual)
			{
				if (m_stateVars.manualGear != 0)
				{
					return !m_stateVars.engaged;
				}
				return false;
			}
			return m_stateVars.transition;
		}

		private void ShiftGearUp(float inRpm, float outRpm)
		{
			int num = settings.forwardGearRatios.Length;
			while (manualGearInput < num)
			{
				manualGearInput++;
				if (outRpm * settings.forwardGearRatios[manualGearInput - 1] < settings.autoShiftUpRevs)
				{
					break;
				}
			}
		}

		private void ShiftGearDown(float inRpm, float outRpm)
		{
			while (manualGearInput > 1)
			{
				manualGearInput--;
				if (outRpm * settings.forwardGearRatios[manualGearInput - 1] > settings.autoShiftDownRevs)
				{
					break;
				}
			}
		}

		private void DoAutoShiftLogic()
		{
			bool flag = stateVehicleThrottle > pedalSoftPressThreshold;
			bool flag2 = stateVehicleBrakes > pedalSoftPressThreshold;
			float time = Solver.time;
			float num = m_stateVars.L / I * Block.WToRpm * inputToRpmRatio;
			if (manualGearInput == 0)
			{
				if (num > settings.autoShiftFirstGearRpm && flag && !flag2)
				{
					ShiftGearUp(num, m_output.L / m_output.I * Block.WToRpm);
					m_stateVars.lastAutoShiftTime = time;
				}
			}
			else
			{
				if (!m_stateVars.engaged)
				{
					return;
				}
				if (num > settings.autoShiftUpRevs)
				{
					if (flag && manualGearInput > 0 && manualGearInput < settings.forwardGearRatios.Length && time - m_stateVars.lastAutoShiftTime >= settings.autoShiftUpInterval + settings.manualShiftTime && (manualGearInput != 1 || stateVehicleSpeed > settings.auto2ndGearMinSpeed))
					{
						ShiftGearUp(num, m_output.L / m_output.I * Block.WToRpm);
						m_stateVars.lastAutoShiftTime = time;
					}
				}
				else
				{
					if (!(num < settings.autoShiftDownRevs))
					{
						return;
					}
					if (manualGearInput > 1 && time - m_stateVars.lastAutoShiftTime >= settings.autoShiftDownInterval + settings.manualShiftTime)
					{
						if (stateVehicleSpeed < settings.auto2ndGearMinSpeed)
						{
							manualGearInput = 1;
						}
						else
						{
							ShiftGearDown(num, m_output.L / m_output.I * Block.WToRpm);
						}
						m_stateVars.lastAutoShiftTime = time;
					}
					if (m_stateVars.vehicleMoving && num < settings.autoShiftNeutralRpm && flag2 && (manualGearInput == 1 || manualGearInput < 0))
					{
						manualGearInput = 0;
					}
				}
			}
		}

		private void StandardProcessInput()
		{
			float num = MathUtility.FastAbs(stateVehicleSpeed);
			if (!m_stateVars.vehicleMoving && num > vehicleMovingThresholdMax)
			{
				m_stateVars.vehicleMoving = true;
			}
			else if (m_stateVars.vehicleMoving && num < vehicleMovingThresholdMin)
			{
				m_stateVars.vehicleMoving = false;
			}
			if (settings.allowParkInManual)
			{
				if (automaticGearInput == 1 && CanEngageParkMode())
				{
					m_stateVars.gearMode = AutomaticGear.P;
				}
				else if (automaticGearInput != 1 && CanDisengageParkMode())
				{
					m_stateVars.gearMode = AutomaticGear.M;
				}
			}
			else
			{
				m_stateVars.gearMode = AutomaticGear.M;
			}
			automaticGearInput = (int)m_stateVars.gearMode;
			if (((settings.autoShift && autoShiftOverride != AutoShiftOverride.ForceManualShift) || autoShiftOverride == AutoShiftOverride.ForceAutoShift) && !bypassAutoShift)
			{
				DoAutoShiftLogic();
			}
			manualGearInput += gearShiftInput;
			gearShiftInput = 0;
			manualGearInput = ClampGear(manualGearInput);
			if (manualGearInput != m_stateVars.manualGear)
			{
				if (m_stateVars.engaged)
				{
					m_stateVars.engaged = false;
					m_stateVars.lastEngagedTime = Solver.time;
					if (manualGearInput == 0)
					{
						m_stateVars.manualGear = 0;
					}
				}
				else if (manualGearInput == 0)
				{
					m_stateVars.manualGear = 0;
				}
				else if (Solver.time - m_stateVars.lastEngagedTime >= settings.manualShiftTime)
				{
					m_stateVars.manualGear = manualGearInput;
					m_stateVars.engaged = true;
				}
			}
			else if (m_stateVars.manualGear != 0)
			{
				m_stateVars.engaged = true;
			}
			if (m_stateVars.engaged)
			{
				m_stateVars.manualRatio = GearRatio(m_stateVars.manualGear);
			}
		}

		private void StandardComputeState()
		{
			if (m_stateVars.engaged)
			{
				float num = 1f / m_stateVars.manualRatio;
				m_input.L = m_output.L * num;
				m_input.I = m_output.I * num * num;
				m_input.Tr = m_output.Tr * num;
			}
			else
			{
				m_input.L = m_stateVars.L;
				m_input.I = I;
				m_input.Tr = 0f;
			}
		}

		private void StandardEvaluateTorque()
		{
			if (m_stateVars.engaged)
			{
				m_output.outTd = m_input.outTd * m_stateVars.manualRatio;
				float num = I * m_input.L / m_input.I;
				T = (num - m_stateVars.L) / Solver.deltaTime;
			}
			else
			{
				m_output.outTd = 0f;
				T = m_input.outTd;
			}
		}

		private int DoAutomaticForwardGearSelection(int currentGear)
		{
			bool flag = stateVehicleThrottle > pedalSoftPressThreshold;
			float num = m_stateVars.L / I * Block.WToRpm * inputToRpmRatio;
			float num2 = m_output.L / m_output.I * Block.WToRpm * inputToRpmRatio;
			int num3 = currentGear;
			if (stateVehicleSpeed < settings.auto2ndGearMinSpeed)
			{
				num3 = settings.automaticInitialGearForward;
			}
			else if (num > settings.automaticGearUpRevs)
			{
				if (flag)
				{
					int num4 = settings.forwardGearRatios.Length;
					while (num3 < num4)
					{
						num3++;
						if (num2 * settings.forwardGearRatios[num3 - 1] < settings.automaticGearUpRevs)
						{
							break;
						}
					}
				}
			}
			else if (num < settings.automaticGearDownRevs)
			{
				while (num3 > 1)
				{
					num3--;
					if (num2 * settings.forwardGearRatios[num3 - 1] > settings.automaticGearDownRevs)
					{
						break;
					}
				}
			}
			return num3;
		}

		private int DoAutomaticReverseGearSelection(int currentGear)
		{
			bool flag = stateVehicleThrottle > pedalSoftPressThreshold;
			float num = m_stateVars.L / I * Block.WToRpm * inputToRpmRatio;
			float num2 = m_output.L / m_output.I * Block.WToRpm * inputToRpmRatio;
			int num3 = currentGear;
			if (0f - stateVehicleSpeed < settings.auto2ndGearMinSpeed)
			{
				num3 = settings.automaticInitialGearReverse;
			}
			else if (num > settings.automaticGearUpRevs)
			{
				if (flag)
				{
					int num4 = settings.reverseGearRatios.Length;
					while (-num3 < num4)
					{
						num3--;
						if (num2 * settings.reverseGearRatios[-num3 - 1] < settings.automaticGearUpRevs)
						{
							break;
						}
					}
				}
			}
			else if (num < settings.automaticGearDownRevs)
			{
				while (-num3 > 1)
				{
					num3++;
					if (num2 * settings.reverseGearRatios[-num3 - 1] > settings.automaticGearDownRevs)
					{
						break;
					}
				}
			}
			return num3;
		}

		private void PlanetaryProcessInput()
		{
			float num = Solver.time - m_stateVars.transitionStartedTime;
			if (m_stateVars.transition && num >= settings.automaticTransitionTime)
			{
				m_stateVars.transition = false;
				m_stateVars.automaticGear = m_stateVars.toGear;
				m_stateVars.automaticRatio = GearRatio(m_stateVars.automaticGear);
				manualGearInput = m_stateVars.automaticGear;
			}
			if (automaticGearInput == 1)
			{
				if (CanEngageParkMode() && m_stateVars.gearMode != AutomaticGear.P && !m_stateVars.transition)
				{
					m_stateVars.gearMode = AutomaticGear.P;
					m_stateVars.automaticGear = 0;
					m_stateVars.transitionStartedTime = Solver.time;
					manualGearInput = 0;
				}
			}
			else if (CanDisengageParkMode())
			{
				if (automaticGearInput < 0)
				{
					automaticGearInput = 0;
				}
				else if (automaticGearInput >= 6)
				{
					automaticGearInput = 5;
				}
				switch ((AutomaticGear)automaticGearInput)
				{
				case AutomaticGear.R:
					if (stateVehicleSpeed < inverseSpeedThreshold)
					{
						m_stateVars.gearMode = AutomaticGear.R;
					}
					break;
				default:
					m_stateVars.gearMode = (AutomaticGear)automaticGearInput;
					break;
				case AutomaticGear.D:
					if (stateVehicleSpeed > 0f - inverseSpeedThreshold)
					{
						m_stateVars.gearMode = AutomaticGear.D;
					}
					break;
				case AutomaticGear.L:
					if (stateVehicleSpeed > 0f - inverseSpeedThreshold && stateVehicleSpeed < settings.auto2ndGearMinSpeed)
					{
						m_stateVars.gearMode = AutomaticGear.L;
					}
					break;
				}
				if (autoShiftOverride == AutoShiftOverride.ForceAutoShift && m_stateVars.gearMode == AutomaticGear.M)
				{
					if (stateVehicleSpeed > 0f - inverseSpeedThreshold)
					{
						m_stateVars.gearMode = AutomaticGear.D;
					}
				}
				else if (autoShiftOverride == AutoShiftOverride.ForceManualShift && m_stateVars.gearMode == AutomaticGear.D)
				{
					m_stateVars.gearMode = AutomaticGear.M;
				}
				automaticGearInput = (int)m_stateVars.gearMode;
			}
			if (m_stateVars.transition || !(num >= settings.automaticTransitionTime + settings.automaticShiftInterval))
			{
				return;
			}
			int num2 = 0;
			switch (m_stateVars.gearMode)
			{
			case AutomaticGear.M:
				num2 = ClampGear(manualGearInput + gearShiftInput);
				break;
			case AutomaticGear.R:
				num2 = ((!settings.automaticShiftReverseGears) ? ((m_stateVars.automaticGear >= 0) ? settings.automaticInitialGearReverse : m_stateVars.automaticGear) : (bypassAutoShift ? m_stateVars.automaticGear : DoAutomaticReverseGearSelection(m_stateVars.automaticGear)));
				num2 = ClampReverseGear(num2 + gearShiftInput);
				break;
			case AutomaticGear.D:
				num2 = (bypassAutoShift ? m_stateVars.automaticGear : DoAutomaticForwardGearSelection(m_stateVars.automaticGear));
				num2 = ClampForwardGear(num2 + gearShiftInput);
				break;
			case AutomaticGear.L:
				num2 = 1;
				break;
			}
			gearShiftInput = 0;
			if (num2 != m_stateVars.automaticGear)
			{
				m_stateVars.fromGear = m_stateVars.automaticGear;
				m_stateVars.toGear = num2;
				if (m_stateVars.fromGear != 0)
				{
					m_stateVars.fromRatio = GearRatio(m_stateVars.fromGear);
				}
				if (m_stateVars.toGear != 0)
				{
					m_stateVars.toRatio = GearRatio(m_stateVars.toGear);
				}
				m_stateVars.transitionRatio = m_stateVars.fromRatio;
				m_stateVars.transitionStartedTime = Solver.time;
				m_stateVars.transition = true;
			}
		}

		private void PlanetaryComputeState()
		{
			if (m_stateVars.transition)
			{
				float num = ((settings.automaticTransitionTime > 0f) ? Mathf.Clamp01((Solver.time - m_stateVars.transitionStartedTime) / settings.automaticTransitionTime) : 1f);
				if (m_stateVars.fromGear == 0)
				{
					StateTransitionFromZeroToGear(num);
					return;
				}
				if (m_stateVars.toGear == 0)
				{
					StateTransitionFromGearToZero(num);
					return;
				}
				if (Mathf.Sign(m_stateVars.fromRatio) == Mathf.Sign(m_stateVars.toRatio))
				{
					StateTransitionFromGearToGear(num);
					return;
				}
				float num2 = m_stateVars.fromRatio / (m_stateVars.fromRatio - m_stateVars.toRatio);
				if (num <= num2)
				{
					StateTransitionFromGearToZero(num / num2);
				}
				else
				{
					StateTransitionFromZeroToGear((num - num2) / (1f - num2));
				}
			}
			else if (m_stateVars.automaticGear == 0)
			{
				m_input.L = m_stateVars.L;
				m_input.I = I;
				m_input.Tr = 0f;
			}
			else
			{
				float num3 = 1f / m_stateVars.automaticRatio;
				m_input.L = m_output.L * num3;
				m_input.I = m_output.I * num3 * num3;
				m_input.Tr = m_output.Tr * num3;
			}
		}

		private void PlanetaryEvaluateTorque()
		{
			if (m_stateVars.transition)
			{
				float num = ((settings.automaticTransitionTime > 0f) ? Mathf.Clamp01((Solver.time - m_stateVars.transitionStartedTime) / settings.automaticTransitionTime) : 1f);
				if (m_stateVars.fromGear == 0)
				{
					TorqueTransitionFromZeroToGear(num);
					return;
				}
				if (m_stateVars.toGear == 0)
				{
					TorqueTransitionFromGearToZero(num);
					return;
				}
				if (Mathf.Sign(m_stateVars.fromRatio) == Mathf.Sign(m_stateVars.toRatio))
				{
					TorqueTransitionFromGearToGear(num);
					return;
				}
				float num2 = m_stateVars.fromRatio / (m_stateVars.fromRatio - m_stateVars.toRatio);
				if (num <= num2)
				{
					TorqueTransitionFromGearToZero(num / num2);
				}
				else
				{
					TorqueTransitionFromZeroToGear((num - num2) / (1f - num2));
				}
			}
			else if (m_stateVars.automaticGear == 0)
			{
				m_output.outTd = 0f;
				T = m_input.outTd;
			}
			else
			{
				m_output.outTd = m_input.outTd * m_stateVars.automaticRatio;
				float num3 = I * m_input.L / m_input.I;
				T = (num3 - m_stateVars.L) / Solver.deltaTime;
			}
		}

		private void StateTransitionFromZeroToGear(float t)
		{
			float num = 1f / m_stateVars.toRatio;
			m_input.L = MathUtility.UnclampedLerp(m_stateVars.L, m_output.L * num, t);
			m_input.I = MathUtility.UnclampedLerp(I, m_output.I * num * num, t);
			m_input.Tr = m_output.Tr * num * t;
		}

		private void StateTransitionFromGearToZero(float t)
		{
			float num = 1f / m_stateVars.fromRatio;
			m_input.L = MathUtility.UnclampedLerp(m_output.L * num, m_stateVars.L, t);
			m_input.I = MathUtility.UnclampedLerp(m_output.I * num * num, I, t);
			m_input.Tr = m_output.Tr * num * (1f - t);
		}

		private void StateTransitionFromGearToGear(float t)
		{
			m_stateVars.transitionRatio = MathUtility.UnclampedLerp(m_stateVars.fromRatio, m_stateVars.toRatio, t);
			float num = 1f / m_stateVars.transitionRatio;
			m_input.L = m_output.L * num;
			m_input.I = m_output.I * num * num;
			m_input.Tr = m_output.Tr * num;
		}

		private void TorqueTransitionFromZeroToGear(float t)
		{
			float to = (I * m_input.L / m_input.I - m_stateVars.L) / Solver.deltaTime;
			m_output.outTd = m_input.outTd * m_stateVars.toRatio * t;
			T = MathUtility.UnclampedLerp(m_input.outTd, to, t);
		}

		private void TorqueTransitionFromGearToZero(float t)
		{
			float from = (I * m_input.L / m_input.I - m_stateVars.L) / Solver.deltaTime;
			m_output.outTd = m_input.outTd * m_stateVars.fromRatio * (1f - t);
			T = MathUtility.UnclampedLerp(from, m_input.outTd, t);
		}

		private void TorqueTransitionFromGearToGear(float t)
		{
			m_output.outTd = m_input.outTd * MathUtility.UnclampedLerp(m_stateVars.fromRatio, m_stateVars.toRatio, t);
			float num = I * m_input.L / m_input.I;
			T = (num - m_stateVars.L) / Solver.deltaTime;
		}
	}
}
