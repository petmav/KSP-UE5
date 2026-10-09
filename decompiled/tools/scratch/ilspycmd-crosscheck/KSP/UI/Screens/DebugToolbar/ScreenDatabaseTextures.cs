namespace KSP.UI.Screens.DebugToolbar
{
	public class ScreenDatabaseTextures : ScreenDatabaseList
	{
		protected override void LoadDatabaseListItems()
		{
			int i = 0;
			for (int count = GameDatabase.Instance.databaseTexture.Count; i < count; i++)
			{
				textMeshQueue.AddLine(GameDatabase.Instance.databaseTexture[i].name);
			}
		}
	}
}
