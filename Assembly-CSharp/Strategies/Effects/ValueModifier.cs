using System;
using KSP.Localization;

namespace Strategies.Effects
{
	[Serializable]
	public class ValueModifier : StrategyEffect
	{
		private float minValue;

		private float maxValue;

		private string valueId;

		private string effectDescription;

		public ValueModifier(Strategy parent)
			: base(parent)
		{
		}

		public ValueModifier(Strategy parent, string valueId, string description, float minValue, float maxValue)
			: base(parent)
		{
			this.valueId = valueId;
			effectDescription = description;
			this.minValue = minValue;
			this.maxValue = maxValue;
		}

		protected override string GetDescription()
		{
			return Localizer.Format("#autoLOC_6002357", ToPercentage(ParentLerp(minValue, maxValue), "0.##"), effectDescription);
		}

		protected override void OnLoadFromConfig(ConfigNode node)
		{
			string value = node.GetValue("minValue");
			if (value != null)
			{
				minValue = float.Parse(value);
			}
			value = node.GetValue("maxValue");
			if (value != null)
			{
				maxValue = float.Parse(value);
			}
			if (node.HasValue("valueId"))
			{
				valueId = node.GetValue("valueId");
			}
			if (node.HasValue("effectDescription"))
			{
				effectDescription = node.GetValue("effectDescription");
			}
		}

		protected override void OnRegister()
		{
			GameEvents.Modifiers.onValueModifierQuery.Add(OnValueModifierQuery);
		}

		protected override void OnUnregister()
		{
			GameEvents.Modifiers.onValueModifierQuery.Remove(OnValueModifierQuery);
		}

		private void OnValueModifierQuery(ValueModifierQuery q)
		{
			if (q.ValueId == valueId)
			{
				q.AddDelta(q.GetInput() * ParentLerp(minValue, maxValue) - q.GetInput());
			}
		}
	}
}
