using System.Runtime.InteropServices;

namespace System.EnterpriseServices;

[StructLayout(LayoutKind.Sequential, Pack = 1)]
[ComVisible(false)]
public struct BOID
{
	[MarshalAs(UnmanagedType.ByValArray, SizeConst = 0)]
	public byte[] rgb;
}
