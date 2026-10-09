namespace Experience
{
	public class ExperienceEffectConfig
	{
		private string name;

		private ConfigNode config;

		public string Name => name;

		public ConfigNode Config => config;

		public static ExperienceEffectConfig Create(ConfigNode node)
		{
			ExperienceEffectConfig experienceEffectConfig = new ExperienceEffectConfig();
			string value = node.GetValue("name");
			if (string.IsNullOrEmpty(value))
			{
				return null;
			}
			experienceEffectConfig.name = value;
			experienceEffectConfig.config = node.CreateCopy();
			return experienceEffectConfig;
		}
	}
}
