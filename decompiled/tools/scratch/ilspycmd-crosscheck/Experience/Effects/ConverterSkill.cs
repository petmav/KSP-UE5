using KSP.Localization;

namespace Experience.Effects
{
	public class ConverterSkill : ExperienceEffect
	{
		public ConverterSkill(ExperienceTrait parent)
			: base(parent)
		{
		}

		public ConverterSkill(ExperienceTrait parent, float[] modifiers)
			: base(parent, modifiers)
		{
		}

		protected override float GetDefaultValue()
		{
			return 0f;
		}

		protected override string GetDescription()
		{
			return Localizer.Format("#autoLOC_18401");
		}
	}
}
