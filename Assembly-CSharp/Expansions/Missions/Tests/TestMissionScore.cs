using System;
using Expansions.Missions.Editor;
using KSP.Localization;
using UnityEngine;

namespace Expansions.Missions.Tests
{
	[MEScoreModule(new Type[] { })]
	public class TestMissionScore : TestModule, IScoreableObjective
	{
		[MEGUI_InputField(canBePinned = false, resetValue = "0", guiName = "#autoLOC_8000050", Tooltip = "#autoLOC_8000051")]
		public float score;

		[MEGUI_Dropdown(canBePinned = false, resetValue = "LessThan", guiName = "#autoLOC_8000052", Tooltip = "#autoLOC_8000053")]
		public TestComparisonOperator comparisonOperator;

		private static double epsilon = 1E-15;

		private string bodyScore;

		public override void Awake()
		{
			base.Awake();
			title = Localizer.Format("#autoLOC_8000049");
		}

		public override bool Test()
		{
			if (base.node != null)
			{
				switch (comparisonOperator)
				{
				default:
					return false;
				case TestComparisonOperator.LessThan:
					return base.node.mission.currentScore < score;
				case TestComparisonOperator.LessThanorEqual:
					return base.node.mission.currentScore <= score;
				case TestComparisonOperator.Equal:
					return (double)Math.Abs(score - base.node.mission.currentScore) < epsilon;
				case TestComparisonOperator.GreaterThanorEqual:
					return base.node.mission.currentScore >= score;
				case TestComparisonOperator.GreaterThan:
					return base.node.mission.currentScore > score;
				}
			}
			return false;
		}

		public override string GetNodeBodyParameterString(BaseAPField field)
		{
			if (field.name == "score")
			{
				bodyScore = "";
				switch (comparisonOperator)
				{
				case TestComparisonOperator.LessThan:
					bodyScore = "<";
					break;
				case TestComparisonOperator.LessThanorEqual:
					bodyScore = "<=";
					break;
				case TestComparisonOperator.Equal:
					bodyScore = "=";
					break;
				case TestComparisonOperator.GreaterThanorEqual:
					bodyScore = ">=";
					break;
				case TestComparisonOperator.GreaterThan:
					bodyScore = ">";
					break;
				}
				bodyScore = Localizer.Format("#autoLOC_8006046", bodyScore, score.ToString("0"));
				return bodyScore;
			}
			return base.GetNodeBodyParameterString(field);
		}

		public override string GetInfo()
		{
			return Localizer.Format("#autoLOC_8004022");
		}

		public override void Save(ConfigNode node)
		{
			base.Save(node);
			node.AddValue("score", score);
			node.AddValue("comparisonOperator", comparisonOperator);
		}

		public override void Load(ConfigNode node)
		{
			base.Load(node);
			node.TryGetValue("score", ref score);
			if (!node.TryGetEnum("comparisonOperator", ref comparisonOperator, TestComparisonOperator.Equal))
			{
				Debug.LogError("Failed to parse comparisonOperator from " + title);
			}
		}

		public object GetScoreModifier(Type scoreModule)
		{
			return null;
		}
	}
}
