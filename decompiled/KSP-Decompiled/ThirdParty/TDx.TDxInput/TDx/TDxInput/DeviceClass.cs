using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace TDx.TDxInput
{
	[ComImport]
	[ComSourceInterfaces("TDx.TDxInput._ISimpleDeviceEvents\0\0")]
	[Guid("82C5AB54-C92C-4D52-AAC5-27E25E22604C")]
	[TypeLibType(2)]
	[ClassInterface(0)]
	public class DeviceClass : ISimpleDevice, Device, _ISimpleDeviceEvents_Event
	{
		[DispId(3)]
		public virtual extern Sensor Sensor
		{
			[MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
			[DispId(3)]
			[return: MarshalAs(UnmanagedType.Interface)]
			get;
		}

		[DispId(4)]
		public virtual extern Keyboard Keyboard
		{
			[MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
			[DispId(4)]
			[return: MarshalAs(UnmanagedType.Interface)]
			get;
		}

		[DispId(5)]
		public virtual extern int Type
		{
			[MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
			[DispId(5)]
			get;
		}

		[DispId(6)]
		public virtual extern bool IsConnected
		{
			[MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
			[DispId(6)]
			get;
		}

		public virtual extern event _ISimpleDeviceEvents_DeviceChangeEventHandler DeviceChange;

		[MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
		[DispId(1)]
		public virtual extern void Connect();

		[MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
		[DispId(2)]
		public virtual extern void Disconnect();

		[MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
		[DispId(7)]
		public virtual extern void LoadPreferences([In][MarshalAs(UnmanagedType.BStr)] string PreferencesName);
	}
}
