namespace System.Runtime.InteropServices;

[Serializable]
[ComVisible(true)]
public enum GCHandleType
{
	Weak,
	WeakTrackResurrection,
	Normal,
	Pinned
}
