using UnityEngine;

namespace Strategies
{
	public class StrategyEffect
	{
		private Strategy parent;

		public Strategy Parent => parent;

		public string Description => GetDescription();

		public StrategyEffect(Strategy parent)
		{
			this.parent = parent;
		}

		public void Register()
		{
			OnRegister();
		}

		public void Unregister()
		{
			OnUnregister();
		}

		public void Load(ConfigNode node)
		{
			OnLoad(node);
		}

		public void Save(ConfigNode node)
		{
			node.AddValue("name", GetType().Name);
			OnSave(node);
		}

		public void LoadFromConfig(ConfigNode node)
		{
			OnLoadFromConfig(node);
		}

		public void Update()
		{
			OnUpdate();
		}

		protected virtual string GetDescription()
		{
			return "";
		}

		protected virtual void OnRegister()
		{
		}

		protected virtual void OnUnregister()
		{
		}

		protected virtual void OnLoad(ConfigNode node)
		{
		}

		protected virtual void OnSave(ConfigNode node)
		{
		}

		protected virtual void OnLoadFromConfig(ConfigNode node)
		{
		}

		protected virtual void OnUpdate()
		{
		}

		public virtual bool CanActivate(ref string reason)
		{
			return true;
		}

		protected string ToPercentage(float percentage, string format = "F0")
		{
			float num = percentage * 100f - 100f;
			return ((num > 0f) ? "+" : "") + num.ToString(format) + "%";
		}

		protected float ParentLerp(float minValue, float maxValue)
		{
			return Mathf.Lerp(minValue, maxValue, Parent.Factor);
		}
	}
}
