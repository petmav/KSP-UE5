namespace KSP.UI.Screens.DebugToolbar
{
	public class ScreenDatabaseAudio : ScreenDatabaseList
	{
		protected override void LoadDatabaseListItems()
		{
			int i = 0;
			for (int count = GameDatabase.Instance.databaseAudio.Count; i < count; i++)
			{
				textMeshQueue.AddLine(GameDatabase.Instance.databaseAudio[i].name);
			}
		}
	}
}
