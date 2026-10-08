using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace TDx.TDxInput
{
	[ComImport]
	[TypeLibType(2)]
	[ClassInterface(0)]
	[Guid("1A960ECE-0E57-4A68-B694-8373114F1FF4")]
	public class TDxInfoClass : ITDxInfo, TDxInfo
	{
		[MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
		[DispId(1)]
		[return: MarshalAs(UnmanagedType.BStr)]
		public virtual extern string RevisionNumber();
	}
}
