using KSP.Localization;

namespace Experience.Effects
{
	public class ScienceResetSkill : ExperienceEffect
	{
		public ScienceResetSkill(ExperienceTrait parent)
			: base(parent)
		{
		}

		public ScienceResetSkill(ExperienceTrait parent, float[] modifiers)
			: base(parent, modifiers)
		{
		}

		protected override float GetDefaultValue()
		{
			return 0f;
		}

		protected override string GetDescription()
		{
			return Localizer.Format("#autoLOC_18818");
		}
	}
}
