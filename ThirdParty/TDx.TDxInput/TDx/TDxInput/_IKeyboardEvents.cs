using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace TDx.TDxInput
{
	[ComImport]
	[TypeLibType(4096)]
	[Guid("6B6BB0A8-4491-40CF-B1A9-C15A801FE151")]
	[InterfaceType(2)]
	public interface _IKeyboardEvents
	{
		[MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
		[DispId(1)]
		void KeyDown([In] int keyCode);

		[MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
		[DispId(2)]
		void KeyUp([In] int keyCode);
	}
}
