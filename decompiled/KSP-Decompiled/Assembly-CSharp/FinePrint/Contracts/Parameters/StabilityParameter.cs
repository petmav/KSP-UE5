using System.Globalization;
using Contracts;
using FinePrint.Utilities;
using KSP.Localization;
using UnityEngine;

namespace FinePrint.Contracts.Parameters
{
	public class StabilityParameter : ContractParameter
	{
		private float holdSeconds;

		private float holdTimer;

		public StabilityParameter()
		{
			holdSeconds = 10f;
			holdTimer = 10f;
		}

		public StabilityParameter(float holdSeconds)
		{
			this.holdSeconds = holdSeconds;
			holdTimer = holdSeconds;
		}

		protected override string GetHashString()
		{
			return SystemUtilities.SuperSeed(base.Root).ToString(CultureInfo.InvariantCulture) + base.ID;
		}

		protected override string GetTitle()
		{
			return Localizer.Format("#autoLOC_285413", (int)((double)holdSeconds + 0.5));
		}

		protected override void OnRegister()
		{
			base.DisableOnStateChange = false;
		}

		protected override void OnReset()
		{
			SetIncomplete();
		}

		protected override void OnSave(ConfigNode node)
		{
			node.AddValue("holdSeconds", holdSeconds);
		}

		protected override void OnLoad(ConfigNode node)
		{
			SystemUtilities.LoadNode(node, "StabilityParameter", "holdSeconds", ref holdSeconds, 10f);
		}

		protected override void OnUpdate()
		{
			if (!SystemUtilities.FlightIsReady(base.Root.ContractState, Contract.State.Active, checkVessel: true))
			{
				return;
			}
			Vessel activeVessel = FlightGlobals.ActiveVessel;
			bool flag = true;
			if (activeVessel.geeForce > 0.1 && !activeVessel.LandedOrSplashed)
			{
				flag = false;
			}
			if (flag && activeVessel.LandedOrSplashed && activeVessel.srf_velocity.sqrMagnitude > 0.09000000357627869)
			{
				flag = false;
			}
			if (flag && activeVessel.situation == Vessel.Situations.SUB_ORBITAL && activeVessel.heightFromTerrain < 500f && activeVessel.heightFromTerrain != -1f)
			{
				flag = false;
			}
			if (base.State == ParameterState.Incomplete)
			{
				if (!flag)
				{
					holdTimer = holdSeconds;
				}
				else
				{
					holdTimer -= Time.deltaTime;
				}
				if (holdTimer <= 0f)
				{
					SetComplete();
				}
			}
			if (base.State == ParameterState.Complete && !flag)
			{
				SetIncomplete();
			}
		}
	}
}
