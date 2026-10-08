using System.Runtime.InteropServices;

namespace TDx.TDxInput
{
	[ComImport]
	[CoClass(typeof(KeyboardClass))]
	[Guid("D6F968E7-2993-48D7-AF24-8B602D925B2C")]
	public interface Keyboard : IKeyboard, _IKeyboardEvents_Event
	{
	}
}
