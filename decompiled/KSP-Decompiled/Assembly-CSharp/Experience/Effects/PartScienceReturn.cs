using KSP.Localization;

namespace Experience.Effects
{
	public class PartScienceReturn : ExperienceEffect
	{
		public PartScienceReturn(ExperienceTrait parent)
			: base(parent)
		{
		}

		public PartScienceReturn(ExperienceTrait parent, float[] modifiers)
			: base(parent, modifiers)
		{
		}

		protected override float GetDefaultValue()
		{
			return 1f;
		}

		protected override string GetDescription()
		{
			string text = (GetValue() * 100f).ToString("F2");
			return Localizer.Format("#autoLOC_18706", text);
		}

		protected override void OnRegister(Part part)
		{
			part.PartValues.ScienceReturnSum.Add(GetValue);
		}

		protected override void OnUnregister(Part part)
		{
			part.PartValues.ScienceReturnSum.Remove(GetValue);
		}

		private float GetValue()
		{
			return base.LevelModifiers[base.Parent.CrewMemberExperienceLevel(base.LevelModifiers.Length)];
		}
	}
}
