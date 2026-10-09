namespace System.IO;

internal struct InotifyEvent
{
	public static readonly InotifyEvent Default;

	public int WatchDescriptor;

	public InotifyMask Mask;

	public string Name;

	public override string ToString()
	{
		return $"[Descriptor: {WatchDescriptor} Mask: {Mask} Name: {Name}]";
	}
}
