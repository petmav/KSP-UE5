namespace KSP.UI.Screens.DebugToolbar
{
	public class ScreenDatabaseConfig : ScreenDatabaseList
	{
		public string category = "PART";

		protected override void LoadDatabaseListItems()
		{
			UrlDir.UrlConfig[] configs = GameDatabase.Instance.GetConfigs(category);
			int i = 0;
			for (int num = configs.Length; i < num; i++)
			{
				textMeshQueue.AddLine(configs[i].url);
			}
		}
	}
}
