using System.Runtime.InteropServices;

namespace System.IO.Pipes;

internal struct SecurityAttributes
{
	public readonly int Length;

	public readonly IntPtr SecurityDescriptor;

	public readonly bool Inheritable;

	public SecurityAttributes(HandleInheritability inheritability, IntPtr securityDescriptor)
	{
		Length = Marshal.SizeOf(typeof(SecurityAttributes));
		SecurityDescriptor = securityDescriptor;
		Inheritable = inheritability == HandleInheritability.Inheritable;
	}
}
