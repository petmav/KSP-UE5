using KSP.Localization;

namespace Experience.Effects
{
	public class FuelUsage : ExperienceEffect
	{
		public FuelUsage(ExperienceTrait parent)
			: base(parent)
		{
		}

		public FuelUsage(ExperienceTrait parent, float[] modifiers)
			: base(parent, modifiers)
		{
		}

		protected override float GetDefaultValue()
		{
			return 1f;
		}

		protected override string GetDescription()
		{
			string text = (GetValue() * 100f).ToString("F2") + "%";
			return Localizer.Format("#autoLOC_18516", text);
		}

		protected override void OnRegister(Part part)
		{
			part.PartValues.FuelUsage.Add(GetValue);
		}

		protected override void OnUnregister(Part part)
		{
			part.PartValues.FuelUsage.Remove(GetValue);
		}

		private float GetValue()
		{
			return base.LevelModifiers[base.Parent.CrewMemberExperienceLevel(base.LevelModifiers.Length)];
		}
	}
}
