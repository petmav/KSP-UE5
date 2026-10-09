using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace TDx.TDxInput
{
	[ComImport]
	[Guid("00612962-8FB6-47B2-BF98-4E8C0FF5F559")]
	[TypeLibType(4160)]
	public interface ITDxInfo
	{
		[MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
		[DispId(1)]
		[return: MarshalAs(UnmanagedType.BStr)]
		string RevisionNumber();
	}
}
