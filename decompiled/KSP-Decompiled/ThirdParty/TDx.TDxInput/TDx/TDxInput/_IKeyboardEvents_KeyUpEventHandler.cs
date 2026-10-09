using System.Runtime.InteropServices;

namespace TDx.TDxInput
{
	[TypeLibType(16)]
	[ComVisible(false)]
	public delegate void _IKeyboardEvents_KeyUpEventHandler([In] int keyCode);
}
