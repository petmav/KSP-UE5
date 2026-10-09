using System;
using System.Collections.Generic;
using FinePrint.Utilities;
using UnityEngine;

namespace Contracts.Parameters
{
	[Serializable]
	public class AcquirePart : ContractParameter
	{
		public enum CompleteCondition
		{
			All,
			Any
		}

		[SerializeField]
		private List<uint> partsToRecover = new List<uint>();

		[SerializeField]
		private string title = "";

		[SerializeField]
		private CompleteCondition winCondition;

		private bool eventsAdded;

		public AcquirePart()
		{
		}

		public AcquirePart(string title, CompleteCondition winCondition = CompleteCondition.All)
		{
			this.title = title;
			partsToRecover = new List<uint>();
			this.winCondition = winCondition;
		}

		public void SetTitle(string title)
		{
			this.title = title;
		}

		protected override string GetTitle()
		{
			return title;
		}

		protected override void OnLoad(ConfigNode node)
		{
			SystemUtilities.LoadNode(node, "Parameter.AcquirePart", "title", ref title, "");
			SystemUtilities.LoadNode(node, "Parameter.AcquirePart", "winOn", ref winCondition, CompleteCondition.Any);
			string[] values = node.GetValues("part");
			int num = values.Length;
			for (int i = 0; i < num; i++)
			{
				partsToRecover.Add(uint.Parse(values[i]));
			}
		}

		protected override void OnSave(ConfigNode node)
		{
			node.AddValue("title", title);
			node.AddValue("winOn", (int)winCondition);
			int count = partsToRecover.Count;
			for (int i = 0; i < count; i++)
			{
				node.AddValue("part", partsToRecover[i].ToString());
			}
		}

		protected override void OnRegister()
		{
			eventsAdded = false;
			if (base.Root.ContractState == Contract.State.Active)
			{
				GameEvents.onPartCouple.Add(OnCouple);
				GameEvents.onPartDie.Add(OnPartDied);
				eventsAdded = true;
			}
		}

		protected override void OnUnregister()
		{
			if (eventsAdded)
			{
				GameEvents.onPartCouple.Remove(OnCouple);
				GameEvents.onPartDie.Remove(OnPartDied);
			}
		}

		protected override void OnReset()
		{
			if (SystemUtilities.FlightIsReady(checkVessel: true))
			{
				ScanVessel(FlightGlobals.ActiveVessel);
			}
		}

		private void OnCouple(GameEvents.FromToAction<Part, Part> action)
		{
			ScanVessel(action.from.vessel);
		}

		private void OnPartDied(Part p)
		{
			DestroyPart(p.flightID);
		}

		public void AddPart(uint id)
		{
			if (!partsToRecover.Contains(id))
			{
				partsToRecover.Add(id);
			}
		}

		public void RemovePart(uint id)
		{
			if (partsToRecover.Contains(id))
			{
				partsToRecover.Remove(id);
			}
		}

		private void ScanVessel(Vessel v)
		{
			int count = v.parts.Count;
			while (count-- > 0)
			{
				CheckNewPart(v.parts[count].flightID);
			}
		}

		private void CheckNewPart(uint id)
		{
			if (partsToRecover.Contains(id))
			{
				partsToRecover.RemoveAll((uint x) => x == id);
				if (winCondition == CompleteCondition.Any)
				{
					SetComplete();
				}
				else if (winCondition == CompleteCondition.All && partsToRecover.Count <= 0)
				{
					SetComplete();
				}
			}
		}

		private void DestroyPart(uint id)
		{
			if (partsToRecover.Contains(id))
			{
				partsToRecover.RemoveAll((uint x) => x == id);
				if (winCondition == CompleteCondition.All)
				{
					SetFailed();
				}
				else if (winCondition == CompleteCondition.Any && partsToRecover.Count <= 0)
				{
					SetFailed();
				}
			}
		}
	}
}
