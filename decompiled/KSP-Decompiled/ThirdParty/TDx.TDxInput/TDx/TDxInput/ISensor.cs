using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace TDx.TDxInput
{
	[ComImport]
	[TypeLibType(4160)]
	[Guid("F3A6775E-6FA1-4829-BF32-5B045C29078F")]
	public interface ISensor
	{
		[DispId(1)]
		Vector3D Translation
		{
			[MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
			[DispId(1)]
			[return: MarshalAs(UnmanagedType.Interface)]
			get;
		}

		[DispId(2)]
		AngleAxis Rotation
		{
			[MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
			[DispId(2)]
			[return: MarshalAs(UnmanagedType.Interface)]
			get;
		}

		[DispId(3)]
		object Device
		{
			[MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
			[DispId(3)]
			[return: MarshalAs(UnmanagedType.IDispatch)]
			get;
		}

		[DispId(4)]
		double Period
		{
			[MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
			[DispId(4)]
			get;
		}
	}
}
