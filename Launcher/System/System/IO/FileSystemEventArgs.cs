namespace System.IO;

public class FileSystemEventArgs : EventArgs
{
	private WatcherChangeTypes changeType;

	private string directory;

	private string name;

	public WatcherChangeTypes ChangeType => changeType;

	public string FullPath => Path.Combine(directory, name);

	public string Name => name;

	public FileSystemEventArgs(WatcherChangeTypes changeType, string directory, string name)
	{
		this.changeType = changeType;
		this.directory = directory;
		this.name = name;
	}

	internal void SetName(string name)
	{
		this.name = name;
	}
}
