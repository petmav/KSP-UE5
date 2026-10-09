using KSP.Localization;

namespace Experience.Effects
{
	public class VesselScienceReturn : ExperienceEffect
	{
		public VesselScienceReturn(ExperienceTrait parent)
			: base(parent)
		{
		}

		public VesselScienceReturn(ExperienceTrait parent, float[] modifiers)
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
			return Localizer.Format("#autoLOC_18908", text);
		}

		protected override void OnRegister(Part part)
		{
			part.PartValues.ScienceReturnMax.Add(GetValue);
		}

		protected override void OnUnregister(Part part)
		{
			part.PartValues.ScienceReturnMax.Remove(GetValue);
		}

		private float GetValue()
		{
			return base.LevelModifiers[base.Parent.CrewMemberExperienceLevel(base.LevelModifiers.Length)];
		}
	}
}
