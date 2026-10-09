using System;
using KSP.Localization;

namespace Experience.Effects
{
	public class FailureRepairSkill : ExperienceEffect
	{
		public enum Skills
		{
			Failures = 1
		}

		public static string[] SkillsReadable = new string[1] { "Repair Failures" };

		public FailureRepairSkill(ExperienceTrait parent)
			: base(parent)
		{
		}

		public FailureRepairSkill(ExperienceTrait parent, float[] modifiers)
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
			string text = Localizer.Format("#autoLOC_8003452");
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
			part.PartValues.FailureRepairSkill.Add(GetValue);
		}

		protected override void OnUnregister(Part part)
		{
			part.PartValues.FailureRepairSkill.Remove(GetValue);
		}

		private int GetValue()
		{
			return base.Parent.CrewMemberExperienceLevel();
		}
	}
}
