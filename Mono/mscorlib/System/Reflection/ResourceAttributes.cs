using System.Runtime.InteropServices;

namespace System.Reflection;

[Serializable]
[ComVisible(true)]
[Flags]
public enum ResourceAttributes
{
	Public = 1,
	Private = 2
}
