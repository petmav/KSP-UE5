using System.Runtime.InteropServices;
using UnityEngine.Scripting;

namespace UnityEngine;

[StructLayout(LayoutKind.Sequential)]
[NativeClass(null)]
[RequiredByNativeCode]
[ExcludeFromObjectFactory]
internal class FailedToLoadScriptObject : Object
{
	private FailedToLoadScriptObject()
	{
	}
}
