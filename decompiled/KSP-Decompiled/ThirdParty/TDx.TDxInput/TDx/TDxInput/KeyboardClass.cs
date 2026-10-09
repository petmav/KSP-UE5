using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace TDx.TDxInput
{
	[ComImport]
	[ClassInterface(0)]
	[ComSourceInterfaces("TDx.TDxInput._IKeyboardEvents\0\0")]
	[Guid("25BBE090-583A-4903-A61B-D0EC629AC4EC")]
	[TypeLibType(1026)]
	public class KeyboardClass : IKeyboard, Keyboard, _IKeyboardEvents_Event
	{
		[DispId(1)]
		public virtual extern int Keys
		{
			[MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
			[DispId(1)]
			get;
		}

		[DispId(2)]
		public virtual extern int ProgrammableKeys
		{
			[MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
			[DispId(2)]
			get;
		}

		[DispId(5)]
		public virtual extern object Device
		{
			[MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
			[DispId(5)]
			[return: MarshalAs(UnmanagedType.IDispatch)]
			get;
		}

		public virtual extern event _IKeyboardEvents_KeyDownEventHandler KeyDown;

		public virtual extern event _IKeyboardEvents_KeyUpEventHandler KeyUp;

		[MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
		[DispId(3)]
		[return: MarshalAs(UnmanagedType.BStr)]
		public virtual extern string GetKeyLabel([In] int key);

		[MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
		[DispId(4)]
		[return: MarshalAs(UnmanagedType.BStr)]
		public virtual extern string GetKeyName([In] int key);

		[MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
		[DispId(6)]
		public virtual extern bool IsKeyDown([In] int key);

		[MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
		[DispId(7)]
		public virtual extern bool IsKeyUp([In] int key);
	}
}
