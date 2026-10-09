using System;
using System.Collections.Generic;
using UnityEngine;

namespace Strategies
{
	[Serializable]
	public class StrategyConfig
	{
		private string name;

		private string departmentName;

		private DepartmentConfig department;

		private string title;

		private string description;

		private string iconString;

		private Texture2D iconImage;

		private string[] groupTags;

		private List<StrategyEffectConfig> effects;

		private float initialCostFundsMin;

		private float initialCostScienceMin;

		private float initialCostReputationMin;

		private float initialCostFundsMax;

		private float initialCostScienceMax;

		private float initialCostReputationMax;

		private float requiredReputationMin;

		private float requiredReputationMax;

		private double minLeastDuration;

		private double maxLeastDuration;

		private double maxLongestDuration;

		private double minLongestDuration;

		private bool hasFactorSlider;

		private float factorSliderDefault;

		private int factorSliderSteps;

		public string Name => name;

		public string DepartmentName => departmentName;

		public DepartmentConfig Department => department;

		public string Title => title;

		public string Description => description;

		public string IconString => iconString;

		public Texture2D IconImage => iconImage;

		public string[] GroupTags => groupTags;

		public List<StrategyEffectConfig> Effects => effects;

		public float InitialCostFundsMin => initialCostFundsMin;

		public float InitialCostScienceMin => initialCostScienceMin;

		public float InitialCostReputationMin => initialCostReputationMin;

		public float InitialCostFundsMax => initialCostFundsMax;

		public float InitialCostScienceMax => initialCostScienceMax;

		public float InitialCostReputationMax => initialCostReputationMax;

		public float RequiredReputationMin => requiredReputationMin;

		public float RequiredReputationMax => requiredReputationMax;

		public double MinLeastDuration => minLeastDuration;

		public double MaxLeastDuration => maxLeastDuration;

		public double MaxLongestDuration => maxLongestDuration;

		public double MinLongestDuration => minLongestDuration;

		public bool HasFactorSlider => hasFactorSlider;

		public float FactorSliderDefault => factorSliderDefault;

		public int FactorSliderSteps => factorSliderSteps;

		public static StrategyConfig Create(ConfigNode node, List<DepartmentConfig> departments)
		{
			StrategyConfig strategyConfig = new StrategyConfig();
			strategyConfig.name = node.GetValue("name");
			if (strategyConfig.Name == null)
			{
				Debug.LogError("StrategyConfig: name cannot be null");
				return null;
			}
			string value = node.GetValue("department");
			if (string.IsNullOrEmpty(value))
			{
				return null;
			}
			strategyConfig.departmentName = value;
			strategyConfig.department = GetDepartmentConfig(strategyConfig.DepartmentName, departments);
			if (strategyConfig.Department == null)
			{
				return null;
			}
			strategyConfig.Department.Strategies.Add(strategyConfig);
			value = node.GetValue("title");
			if (!string.IsNullOrEmpty(value))
			{
				strategyConfig.title = value;
			}
			value = node.GetValue("desc");
			if (!string.IsNullOrEmpty(value))
			{
				strategyConfig.description = value;
			}
			value = node.GetValue("description");
			if (!string.IsNullOrEmpty(value))
			{
				strategyConfig.description = value;
			}
			value = node.GetValue("icon");
			if (!string.IsNullOrEmpty(value))
			{
				strategyConfig.iconString = value;
				strategyConfig.iconImage = GameDatabase.Instance.GetTexture(strategyConfig.iconString, asNormalMap: false);
			}
			value = node.GetValue("groupTag");
			if (!string.IsNullOrEmpty(value))
			{
				strategyConfig.groupTags = value.Split(',');
				int num = strategyConfig.groupTags.Length;
				while (num-- > 0)
				{
					strategyConfig.groupTags[num] = strategyConfig.groupTags[num].Trim();
				}
			}
			strategyConfig.effects = new List<StrategyEffectConfig>();
			ConfigNode[] nodes = node.GetNodes("EFFECT");
			int i = 0;
			for (int num2 = nodes.Length; i < num2; i++)
			{
				StrategyEffectConfig strategyEffectConfig = StrategyEffectConfig.Create(nodes[i]);
				if (strategyEffectConfig != null)
				{
					strategyConfig.Effects.Add(strategyEffectConfig);
				}
			}
			value = node.GetValue("initialCostFunds");
			if (!string.IsNullOrEmpty(value))
			{
				strategyConfig.initialCostFundsMin = float.Parse(value);
				strategyConfig.initialCostFundsMax = float.Parse(value);
			}
			value = node.GetValue("initialCostReputation");
			if (!string.IsNullOrEmpty(value))
			{
				strategyConfig.initialCostReputationMin = float.Parse(value);
				strategyConfig.initialCostReputationMax = float.Parse(value);
			}
			value = node.GetValue("initialCostScience");
			if (!string.IsNullOrEmpty(value))
			{
				strategyConfig.initialCostScienceMin = float.Parse(value);
				strategyConfig.initialCostScienceMax = float.Parse(value);
			}
			if (node.HasValue("initialCostFundsMin"))
			{
				strategyConfig.initialCostFundsMin = float.Parse(node.GetValue("initialCostFundsMin"));
			}
			if (node.HasValue("initialCostFundsMax"))
			{
				strategyConfig.initialCostFundsMax = float.Parse(node.GetValue("initialCostFundsMax"));
			}
			if (node.HasValue("initialCostScienceMin"))
			{
				strategyConfig.initialCostScienceMin = float.Parse(node.GetValue("initialCostScienceMin"));
			}
			if (node.HasValue("initialCostScienceMax"))
			{
				strategyConfig.initialCostScienceMax = float.Parse(node.GetValue("initialCostScienceMax"));
			}
			if (node.HasValue("initialCostReputationMin"))
			{
				strategyConfig.initialCostReputationMin = float.Parse(node.GetValue("initialCostReputationMin"));
			}
			if (node.HasValue("initialCostReputationMax"))
			{
				strategyConfig.initialCostReputationMax = float.Parse(node.GetValue("initialCostReputationMax"));
			}
			if (node.HasValue("requiredReputationMin"))
			{
				strategyConfig.requiredReputationMin = float.Parse(node.GetValue("requiredReputationMin"));
			}
			if (node.HasValue("requiredReputationMax"))
			{
				strategyConfig.requiredReputationMax = float.Parse(node.GetValue("requiredReputationMax"));
			}
			value = node.GetValue("minDuration");
			if (!string.IsNullOrEmpty(value))
			{
				strategyConfig.minLeastDuration = double.Parse(value);
				strategyConfig.maxLeastDuration = double.Parse(value);
			}
			value = node.GetValue("maxDuration");
			if (!string.IsNullOrEmpty(value))
			{
				strategyConfig.minLongestDuration = double.Parse(value);
				strategyConfig.maxLongestDuration = double.Parse(value);
			}
			if (node.HasValue("minLeastDuration"))
			{
				strategyConfig.minLeastDuration = double.Parse(node.GetValue("minLeastDuration"));
			}
			if (node.HasValue("maxLeastDuration"))
			{
				strategyConfig.maxLeastDuration = double.Parse(node.GetValue("maxLeastDuration"));
			}
			if (node.HasValue("minLongestDuration"))
			{
				strategyConfig.minLongestDuration = double.Parse(node.GetValue("minLongestDuration"));
			}
			if (node.HasValue("maxLongestDuration"))
			{
				strategyConfig.maxLongestDuration = double.Parse(node.GetValue("maxLongestDuration"));
			}
			value = node.GetValue("hasFactorSlider");
			if (!string.IsNullOrEmpty(value))
			{
				strategyConfig.hasFactorSlider = bool.Parse(value);
			}
			value = node.GetValue("factorSliderDefault");
			if (!string.IsNullOrEmpty(value))
			{
				strategyConfig.factorSliderDefault = float.Parse(value);
			}
			value = node.GetValue("factorSliderSteps");
			if (!string.IsNullOrEmpty(value))
			{
				strategyConfig.factorSliderSteps = int.Parse(value);
			}
			return strategyConfig;
		}

		private static DepartmentConfig GetDepartmentConfig(string name, List<DepartmentConfig> departments)
		{
			int num = 0;
			int count = departments.Count;
			while (true)
			{
				if (num < count)
				{
					if (departments[num].Name == name)
					{
						break;
					}
					num++;
					continue;
				}
				return null;
			}
			return departments[num];
		}
	}
}
