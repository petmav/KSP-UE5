using KSP.Localization;

namespace Experience.Effects
{
	public class MaxThrottle : ExperienceEffect
	{
		public MaxThrottle(ExperienceTrait parent)
			: base(parent)
		{
		}

		public MaxThrottle(ExperienceTrait parent, float[] modifiers)
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
			return Localizer.Format("#autoLOC_7001012", text);
		}

		protected override void OnRegister(Part part)
		{
			part.PartValues.MaxThrottle.Add(GetValue);
		}

		protected override void OnUnregister(Part part)
		{
			part.PartValues.MaxThrottle.Remove(GetValue);
		}

		private float GetValue()
		{
			return base.LevelModifiers[base.Parent.CrewMemberExperienceLevel(base.LevelModifiers.Length)];
		}
	}
}
