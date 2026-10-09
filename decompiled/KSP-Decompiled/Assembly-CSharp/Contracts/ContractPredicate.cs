using System;
using KSP.Localization;
using UnityEngine;

namespace Contracts
{
	[Serializable]
	public class ContractPredicate
	{
		[HideInInspector]
		public IContractParameterHost Parent { get; private set; }

		[HideInInspector]
		public Contract Root { get; private set; }

		public string Description => GetDescription();

		public bool AllowMultiple => GetAllowMultiple();

		public ContractPredicate(IContractParameterHost parent)
		{
			Parent = parent;
			Root = parent.Root;
		}

		public void Load(ConfigNode node)
		{
			OnLoad(node);
		}

		public void Save(ConfigNode node)
		{
			OnSave(node);
		}

		public virtual bool Test(Vessel vessel)
		{
			return false;
		}

		public virtual bool Test(ProtoVessel vessel)
		{
			return false;
		}

		protected virtual string GetDescription()
		{
			return Localizer.Format("#autoLOC_268399");
		}

		protected virtual bool GetAllowMultiple()
		{
			return false;
		}

		protected virtual void OnLoad(ConfigNode node)
		{
		}

		protected virtual void OnSave(ConfigNode node)
		{
		}

		public void Update()
		{
			OnUpdate();
		}

		protected virtual void OnUpdate()
		{
		}
	}
}
