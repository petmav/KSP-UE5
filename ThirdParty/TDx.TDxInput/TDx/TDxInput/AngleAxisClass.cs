using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace TDx.TDxInput
{
	[ComImport]
	[TypeLibType(1026)]
	[Guid("512A6C3E-3010-401B-8623-E413E2ACC138")]
	[ClassInterface(0)]
	public class AngleAxisClass : IAngleAxis, AngleAxis
	{
		[DispId(1)]
		public virtual extern double X
		{
			[MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
			[DispId(1)]
			get;
			[MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
			[DispId(1)]
			[param: In]
			set;
		}

		[DispId(2)]
		public virtual extern double Y
		{
			[MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
			[DispId(2)]
			get;
			[MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
			[DispId(2)]
			[param: In]
			set;
		}

		[DispId(3)]
		public virtual extern double Z
		{
			[MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
			[DispId(3)]
			get;
			[MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
			[DispId(3)]
			[param: In]
			set;
		}

		[DispId(4)]
		public virtual extern double Angle
		{
			[MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
			[DispId(4)]
			get;
			[MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
			[DispId(4)]
			[param: In]
			set;
		}
	}
}
