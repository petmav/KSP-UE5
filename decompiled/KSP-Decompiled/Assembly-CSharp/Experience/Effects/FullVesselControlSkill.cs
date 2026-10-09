using KSP.Localization;

namespace Experience.Effects
{
	public class FullVesselControlSkill : ExperienceEffect
	{
		public FullVesselControlSkill(ExperienceTrait parent)
			: base(parent)
		{
		}

		public FullVesselControlSkill(ExperienceTrait parent, float[] modifiers)
			: base(parent, modifiers)
		{
		}

		protected override float GetDefaultValue()
		{
			return 0f;
		}

		protected override string GetDescription()
		{
			string result = Localizer.Format("#autoLOC_18556");
			if (!HighLogic.CurrentGame.Parameters.Difficulty.EnableCommNet)
			{
				return "";
			}
			return result;
		}
	}
}
