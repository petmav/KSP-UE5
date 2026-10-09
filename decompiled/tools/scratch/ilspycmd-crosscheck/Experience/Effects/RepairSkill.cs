using System;
using KSP.Localization;

namespace Experience.Effects
{
	public class RepairSkill : ExperienceEffect
	{
		public enum Skills
		{
			Parachutes = 1,
			Wheels = 3,
			LandingLegs = 2
		}

		public static string[] SkillsReadable = new string[3]
		{
			Localizer.Format("#autoLOC_18738"),
			Localizer.Format("#autoLOC_18739"),
			Localizer.Format("#autoLOC_18740")
		};

		public RepairSkill(ExperienceTrait parent)
			: base(parent)
		{
		}

		public RepairSkill(ExperienceTrait parent, float[] modifiers)
			: base(parent, modifiers)
		{
		}

		protected override float GetDefaultValue()
		{
			return 0f;
		}

		protected override string GetDescription()
		{
			int num = base.Parent.CrewMemberExperienceLevel();
			if (num == 0)
			{
				return "";
			}
			string text = Localizer.Format("#autoLOC_18766");
			string[] names = Enum.GetNames(typeof(Skills));
			int[] array = (int[])Enum.GetValues(typeof(Skills));
			int i = 0;
			for (int num2 = names.Length; i < num2; i++)
			{
				if (num >= array[i])
				{
					text = text + "\n" + Localizer.Format(SkillsReadable[i]);
				}
			}
			return text;
		}

		protected override void OnRegister(Part part)
		{
			part.PartValues.RepairSkill.Add(GetValue);
		}

		protected override void OnUnregister(Part part)
		{
			part.PartValues.RepairSkill.Remove(GetValue);
		}

		private int GetValue()
		{
			return base.Parent.CrewMemberExperienceLevel();
		}
	}
}
