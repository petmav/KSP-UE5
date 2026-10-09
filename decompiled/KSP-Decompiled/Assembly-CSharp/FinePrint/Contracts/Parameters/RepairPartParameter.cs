using System;
using Contracts;
using KSP.Localization;

namespace FinePrint.Contracts.Parameters
{
	[Serializable]
	public class RepairPartParameter : ContractParameter
	{
		protected CelestialBody targetBody;

		protected uint vesselPersistentId;

		protected uint partPersistentId;

		protected string partName;

		protected string vesselName = "";

		protected AvailablePart ap;

		protected Vessel vsl;

		public CelestialBody TargetBody => targetBody;

		public uint VesselPersistentId => vesselPersistentId;

		public uint PartPersistentId => partPersistentId;

		public string PartName => partName;

		public string VesselName => vesselName;

		public RepairPartParameter()
		{
		}

		public RepairPartParameter(uint partId, string partName, uint vesselId, string vesselName, CelestialBody targetBody)
		{
			UpdatePartInfo(partId, partName, vesselId, vesselName, targetBody);
		}

		public void UpdatePartInfo(uint partId, string partName, uint vesselId, string vesselName, CelestialBody targetBody)
		{
			partPersistentId = partId;
			if (!string.IsNullOrEmpty(partName))
			{
				this.partName = partName;
				ap = PartLoader.getPartInfoByName(partName);
			}
			vesselPersistentId = vesselId;
			if (!string.IsNullOrEmpty(vesselName))
			{
				this.vesselName = vesselName;
			}
			if (targetBody != null)
			{
				this.targetBody = targetBody;
			}
			FlightGlobals.FindVessel(vesselId, out vsl);
		}

		protected override string GetHashString()
		{
			return partName + partPersistentId;
		}

		protected override string GetTitle()
		{
			if (vsl == null)
			{
				FlightGlobals.FindVessel(vesselPersistentId, out vsl);
			}
			return Localizer.Format("#autoLOC_6002578", (vsl != null) ? vsl.vesselName : vesselName, (ap != null) ? ap.title : partName, (targetBody != null) ? targetBody.bodyDisplayName : "");
		}

		protected override void OnLoad(ConfigNode node)
		{
			string value = "";
			if (node.TryGetValue("bodyName", ref value))
			{
				targetBody = FlightGlobals.GetBodyByName(value);
			}
			node.TryGetValue("partName", ref partName);
			node.TryGetValue("partPersistentId", ref partPersistentId);
			node.TryGetValue("vesselPersistentId", ref vesselPersistentId);
			node.TryGetValue("vesselName", ref vesselName);
			ap = PartLoader.getPartInfoByName(partName);
			FlightGlobals.FindVessel(vesselPersistentId, out vsl);
		}

		protected override void OnSave(ConfigNode node)
		{
			node.AddValue("bodyName", targetBody.bodyName);
			node.AddValue("partName", partName);
			node.AddValue("partPersistentId", partPersistentId);
			node.AddValue("vesselPersistentId", vesselPersistentId);
			if (!string.IsNullOrEmpty(vesselName))
			{
				node.AddValue("vesselName", vesselName);
			}
		}

		protected override void OnRegister()
		{
			GameEvents.onVesselPersistentIdChanged.Add(VesselIdChanged);
			GameEvents.onPartPersistentIdChanged.Add(PartIdChanged);
			GameEvents.onPartRepaired.Add(OnPartRepaired);
		}

		protected override void OnUnregister()
		{
			GameEvents.onVesselPersistentIdChanged.Remove(VesselIdChanged);
			GameEvents.onPartPersistentIdChanged.Remove(PartIdChanged);
			GameEvents.onPartRepaired.Remove(OnPartRepaired);
		}

		protected override string GetMessageComplete()
		{
			return Localizer.Format("#autoLOC_6002580");
		}

		private void PartIdChanged(uint vslId, uint oldId, uint newId)
		{
			if (vslId == vesselPersistentId && oldId == partPersistentId)
			{
				partPersistentId = newId;
			}
		}

		private void VesselIdChanged(uint oldId, uint newId)
		{
			if (vesselPersistentId == oldId)
			{
				vesselPersistentId = newId;
			}
		}

		private void OnPartRepaired(Part p)
		{
			if ((partPersistentId == 0 || p.persistentId == partPersistentId) && p.vessel.persistentId == vesselPersistentId)
			{
				SetComplete();
				base.Root.Complete();
			}
		}
	}
}
