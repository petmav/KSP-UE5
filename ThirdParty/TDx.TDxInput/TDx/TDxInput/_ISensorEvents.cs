using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace TDx.TDxInput
{
	[ComImport]
	[InterfaceType(2)]
	[Guid("E6929A4A-6F41-46C6-9252-A8CC53472CB1")]
	[TypeLibType(4096)]
	public interface _ISensorEvents
	{
		[MethodImpl(MethodImplOptions.InternalCall, MethodCodeType = MethodCodeType.Runtime)]
		[DispId(1)]
		void SensorInput();
	}
}
