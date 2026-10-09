using KSP.Localization;

namespace Experience.Effects
{
	public class HeatProduction : ExperienceEffect
	{
		public HeatProduction(ExperienceTrait parent)
			: base(parent)
		{
		}

		public HeatProduction(ExperienceTrait parent, float[] modifiers)
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
			return Localizer.Format("#autoLOC_18624", text);
		}

		protected override void OnRegister(Part part)
		{
			part.PartValues.HeatProduction.Add(GetValue);
		}

		protected override void OnUnregister(Part part)
		{
			part.PartValues.HeatProduction.Remove(GetValue);
		}

		private float GetValue()
		{
			return base.LevelModifiers[base.Parent.CrewMemberExperienceLevel(base.LevelModifiers.Length)];
		}
	}
}
