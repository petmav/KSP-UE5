using System;
using KSP.Localization;

namespace Strategies.Effects
{
	[Serializable]
	public class CurrencyOperation : StrategyEffect
	{
		public enum Operator
		{
			Add,
			Multiply,
			Set
		}

		private float minValue;

		private float maxValue;

		private Currency currency;

		private TransactionReasons AffectReasons;

		private string effectDescription;

		private Operator op;

		public CurrencyOperation(Strategy parent)
			: base(parent)
		{
		}

		public CurrencyOperation(Strategy parent, float minValue, float maxValue, Currency currency, Operator op, TransactionReasons AffectReasons, string description)
			: base(parent)
		{
			this.minValue = minValue;
			this.maxValue = maxValue;
			this.currency = currency;
			this.op = op;
			this.AffectReasons = AffectReasons;
			effectDescription = description;
		}

		protected override string GetDescription()
		{
			switch (op)
			{
			default:
				return Localizer.Format("#autoLOC_303954", effectDescription);
			case Operator.Add:
				return Localizer.Format("#autoLOC_6002355", (ParentLerp(minValue, maxValue) > 0f) ? "+" : "", ParentLerp(minValue, maxValue).ToString("N1"), currency.displayDescription(), effectDescription);
			case Operator.Multiply:
				return Localizer.Format("#autoLOC_6002356", ToPercentage(ParentLerp(minValue, maxValue), "0.##"), currency.displayDescription(), effectDescription);
			case Operator.Set:
				return Localizer.Format("#autoLOC_303952", currency.displayDescription(), ParentLerp(minValue, maxValue).ToString("N1"), effectDescription);
			}
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
			if (node.HasValue("currency"))
			{
				currency = (Currency)Enum.Parse(typeof(Currency), node.GetValue("currency"));
			}
			if (node.HasValue("operation"))
			{
				op = (Operator)Enum.Parse(typeof(Operator), node.GetValue("operation"));
			}
			if (node.HasValue("AffectReasons"))
			{
				string[] array = node.GetValue("AffectReasons").Split(',');
				int num = array.Length;
				for (int i = 0; i < num; i++)
				{
					AffectReasons |= (TransactionReasons)Enum.Parse(typeof(TransactionReasons), array[i].Trim());
				}
			}
			if (node.HasValue("effectDescription"))
			{
				effectDescription = node.GetValue("effectDescription");
			}
		}

		protected override void OnRegister()
		{
			GameEvents.Modifiers.OnCurrencyModifierQuery.Add(OnEffectQuery);
		}

		protected override void OnUnregister()
		{
			GameEvents.Modifiers.OnCurrencyModifierQuery.Remove(OnEffectQuery);
		}

		protected virtual void OnEffectQuery(CurrencyModifierQuery qry)
		{
			if ((qry.reason & AffectReasons) != 0)
			{
				switch (op)
				{
				case Operator.Add:
					qry.AddDelta(currency, ParentLerp(minValue, maxValue));
					break;
				case Operator.Multiply:
					qry.AddDelta(currency, ParentLerp(minValue, maxValue) * qry.GetInput(currency) - qry.GetInput(currency));
					break;
				case Operator.Set:
					qry.AddDelta(currency, ParentLerp(minValue, maxValue) - qry.GetInput(currency));
					break;
				}
			}
		}
	}
}
