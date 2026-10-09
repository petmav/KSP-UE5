using System;
using UnityEngine;

namespace ModuleWheels
{
	public abstract class ModuleWheelSubmodule : PartModule
	{
		[KSPField]
		public int baseModuleIndex = -1;

		protected ModuleWheelBase wheelBase;

		internal KSPWheelController wheel;

		protected bool baseSetup;

		public override void OnAwake()
		{
			baseSetup = false;
			SetWheelBase();
			wheelBase.RegisterSubmodule(this);
		}

		protected void SetWheelBase()
		{
			if (baseModuleIndex == -1)
			{
				wheelBase = GetComponent<ModuleWheelBase>();
				return;
			}
			ModuleWheelBase moduleWheelBase = base.part.Modules[baseModuleIndex] as ModuleWheelBase;
			if (moduleWheelBase == null)
			{
				Debug.LogError("[ModuleWheelSubmodule]: Module at index " + baseModuleIndex + " is not a ModuleWheelBase type", base.gameObject);
			}
			else
			{
				wheelBase = moduleWheelBase;
			}
		}

		public virtual void OnDestroy()
		{
			if (!(wheelBase == null))
			{
				wheelBase.UnregisterSubmodule(this);
				if (wheelBase.InopSystems != null && wheelBase.InopSystems.OnModified != null)
				{
					WheelSubsystems inopSystems = wheelBase.InopSystems;
					inopSystems.OnModified = (Callback<WheelSubsystems>)Delegate.Remove(inopSystems.OnModified, new Callback<WheelSubsystems>(OnSubsystemsModified));
				}
			}
		}

		public void OnWheelInit(KSPWheelController w)
		{
			wheel = w;
			baseSetup = true;
			OnWheelSetup();
			WheelSubsystems inopSystems = wheelBase.InopSystems;
			inopSystems.OnModified = (Callback<WheelSubsystems>)Delegate.Combine(inopSystems.OnModified, new Callback<WheelSubsystems>(OnSubsystemsModified));
		}

		protected abstract void OnWheelSetup();

		public abstract string OnGatherInfo();

		protected virtual void OnSubsystemsModified(WheelSubsystems s)
		{
		}
	}
}
