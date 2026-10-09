using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace TDx.TDxInput
{
	[ComImport]
	[TypeLibType(1026)]
	[Guid("85004B00-1AA7-4777-B1CE-8427301B942D")]
	[ComSourceInterfaces("TDx.TDxInput._ISensorEvents\0\0")]
	[ClassInterface(0)]
	public class SensorClass : ISensor, Sensor, _ISensorEvents_Event
	{
		[DispId(1)]
		public virtual extern Vector3D Translation
		{
			[MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
			[DispId(1)]
			[return: MarshalAs(UnmanagedType.Interface)]
			get;
		}

		[DispId(2)]
		public virtual extern AngleAxis Rotation
		{
			[MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
			[DispId(2)]
			[return: MarshalAs(UnmanagedType.Interface)]
			get;
		}

		[DispId(3)]
		public virtual extern object Device
		{
			[MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
			[DispId(3)]
			[return: MarshalAs(UnmanagedType.IDispatch)]
			get;
		}

		[DispId(4)]
		public virtual extern double Period
		{
			[MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
			[DispId(4)]
			get;
		}

		public virtual extern event _ISensorEvents_SensorInputEventHandler SensorInput;
	}
}
