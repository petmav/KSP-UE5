namespace Experience.Effects
{
	public class SpecialExperimentSkill : ExperienceEffect
	{
		public SpecialExperimentSkill(ExperienceTrait parent)
			: base(parent)
		{
		}

		public SpecialExperimentSkill(ExperienceTrait parent, float[] modifiers)
			: base(parent, modifiers)
		{
		}

		protected override float GetDefaultValue()
		{
			return 0f;
		}

		protected override string GetDescription()
		{
			return "";
		}
	}
}
