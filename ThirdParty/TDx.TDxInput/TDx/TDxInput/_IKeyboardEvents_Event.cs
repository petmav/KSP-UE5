using System.Runtime.InteropServices;

namespace TDx.TDxInput
{
	[ComEventInterface(typeof(_IKeyboardEvents_0000), typeof(_IKeyboardEvents_EventProvider_0000))]
	[TypeLibType(16)]
	[ComVisible(false)]
	public interface _IKeyboardEvents_Event
	{
		event _IKeyboardEvents_KeyDownEventHandler KeyDown;

		event _IKeyboardEvents_KeyUpEventHandler KeyUp;
	}
}
