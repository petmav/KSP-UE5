using System;
using System.Collections.Generic;
using UnityEngine;

namespace Expansions.Serenity
{
	[Serializable]
	public abstract class ControlledBase : IConfigNode
	{
		[SerializeField]
		internal uint partId;

		[SerializeField]
		internal List<uint> SymmetryPartIDs;

		[SerializeField]
		private string partNickName = "";

		[SerializeField]
		internal int rowIndex = -1;

		[SerializeField]
		internal uint moduleId;

		[SerializeField]
		internal uint moduleActionsId;

		internal uint persistentActionId;

		public Part Part { get; protected set; }

		public ModuleRoboticController Controller { get; internal set; }

		public uint PartPersistentId
		{
			get
			{
				if (!(Part == null))
				{
					return Part.persistentId;
				}
				return 0u;
			}
		}

		public List<Part> SymmetryParts
		{
			get
			{
				if (!(Part != null))
				{
					return null;
				}
				return Part.symmetryCounterparts;
			}
		}

		public string PartNickName => partNickName;

		[SerializeField]
		public PartModule Module { get; protected set; }

		public uint PersistentActionId => persistentActionId;

		internal abstract string BaseName { get; }

		internal ControlledBase(Part part, PartModule module, ModuleRoboticController controller)
		{
			partId = part.persistentId;
			Part = part;
			Module = module;
			if (module != null)
			{
				moduleId = module.GetPersistentId();
				persistentActionId = module.GetPersistenActiontId();
			}
			Controller = controller;
		}

		internal ControlledBase()
		{
			partId = 0u;
			Part = null;
			moduleId = 0u;
			Module = null;
			Controller = null;
		}

		public void SetPartNickName(string newNickName)
		{
			partNickName = newNickName;
		}

		protected abstract bool OnAssignReferenceVars();

		protected abstract void ClearSymmetryLists();

		protected abstract void AddSymmetryPart(Part part);

		protected abstract bool OnChangeSymmetryMaster(Part part, out uint oldPartId);

		internal bool AssignReferenceVars()
		{
			if (!FlightGlobals.FindLoadedPart(partId, out var partout))
			{
				Debug.LogWarningFormat("[ModuleRoboticController]: Unable to find Controlled Part in vessel: {0}", partId);
				return false;
			}
			Part = partout;
			if (moduleId != 0)
			{
				Module = partout.Modules[moduleId];
			}
			if (partNickName == "")
			{
				partNickName = Part.partInfo.title;
			}
			bool result = OnAssignReferenceVars();
			RebuildSymmetryList();
			return result;
		}

		internal void RebuildSymmetryList(params uint[] excludedPartIds)
		{
			ClearSymmetryLists();
			if (SymmetryParts == null)
			{
				return;
			}
			for (int i = 0; i < SymmetryParts.Count; i++)
			{
				if (excludedPartIds.IndexOf(SymmetryParts[i].persistentId) <= -1)
				{
					AddSymmetryPart(SymmetryParts[i]);
				}
			}
		}

		public void ChangeSymmetryMaster(Part newPart)
		{
			uint oldPartId;
			if (!SymmetryParts.Contains(newPart))
			{
				Debug.LogErrorFormat("[ControlledAxis]: Cannot change Symmetry Master to {0} as its not listed in the SymmetryParts for {1}", newPart.persistentId, PartPersistentId);
			}
			else if (OnChangeSymmetryMaster(newPart, out oldPartId))
			{
				Part = newPart;
				partId = newPart.persistentId;
				RebuildSymmetryList(oldPartId);
			}
		}

		protected abstract void OnLoad(ConfigNode node);

		protected abstract void OnSave(ConfigNode node);

		public void Load(ConfigNode node)
		{
			node.TryGetValue("persistentId", ref partId);
			node.TryGetValue("moduleId", ref moduleId);
			node.TryGetValue("partNickName", ref partNickName);
			node.TryGetValue("rowIndex", ref rowIndex);
			OnLoad(node);
		}

		public void Save(ConfigNode node)
		{
			node.AddValue("persistentId", partId);
			node.AddValue("moduleId", moduleId);
			node.AddValue("partNickName", partNickName);
			node.AddValue("rowIndex", rowIndex);
			if (SymmetryParts != null && SymmetryParts.Count > 0)
			{
				ConfigNode configNode = node.AddNode("SYMPARTS");
				for (int i = 0; i < SymmetryParts.Count; i++)
				{
					configNode.AddValue("symPersistentId", SymmetryParts[i].persistentId);
				}
			}
			OnSave(node);
		}
	}
}
