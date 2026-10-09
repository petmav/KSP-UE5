using KSP.Localization;

namespace Experience.Effects
{
	public class ExternalExperimentSkill : ExperienceEffect
	{
		public ExternalExperimentSkill(ExperienceTrait parent)
			: base(parent)
		{
		}

		public ExternalExperimentSkill(ExperienceTrait parent, float[] modifiers)
			: base(parent, modifiers)
		{
		}

		protected override float GetDefaultValue()
		{
			return 0f;
		}

		protected override string GetDescription()
		{
			return Localizer.Format("#autoLOC_18491");
		}
	}
}
