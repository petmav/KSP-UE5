namespace KSP.UI.Screens.DebugToolbar
{
	public class ScreenDatabaseAssemblies : ScreenDatabaseList
	{
		protected override void LoadDatabaseListItems()
		{
			int i = 0;
			for (int count = AssemblyLoader.loadedAssemblies.Count; i < count; i++)
			{
				textMeshQueue.AddLine(AssemblyLoader.loadedAssemblies[i].assembly.FullName);
			}
		}
	}
}
