namespace Experience.Effects
{
	public class ScienceSkill : ExperienceEffect
	{
		public ScienceSkill(ExperienceTrait parent)
			: base(parent)
		{
		}

		public ScienceSkill(ExperienceTrait parent, float[] modifiers)
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

		protected override void OnRegister(Part part)
		{
			part.PartValues.ScienceSkill.Add(GetValue);
		}

		protected override void OnUnregister(Part part)
		{
			part.PartValues.ScienceSkill.Remove(GetValue);
		}

		private int GetValue()
		{
			return base.Parent.CrewMemberExperienceLevel();
		}
	}
}
