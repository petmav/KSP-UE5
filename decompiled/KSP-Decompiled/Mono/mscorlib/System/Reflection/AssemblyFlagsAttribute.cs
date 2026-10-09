using System.Runtime.InteropServices;

namespace System.Reflection;

[AttributeUsage(AttributeTargets.Assembly, Inherited = false)]
[ComVisible(true)]
public sealed class AssemblyFlagsAttribute : Attribute
{
	private AssemblyNameFlags m_flags;

	[CLSCompliant(false)]
	[Obsolete("This property has been deprecated. Please use AssemblyFlags instead. http://go.microsoft.com/fwlink/?linkid=14202")]
	public uint Flags => (uint)m_flags;

	public int AssemblyFlags => (int)m_flags;

	[Obsolete("This constructor has been deprecated. Please use AssemblyFlagsAttribute(AssemblyNameFlags) instead. http://go.microsoft.com/fwlink/?linkid=14202")]
	[CLSCompliant(false)]
	public AssemblyFlagsAttribute(uint flags)
	{
		m_flags = (AssemblyNameFlags)flags;
	}

	[Obsolete("This constructor has been deprecated. Please use AssemblyFlagsAttribute(AssemblyNameFlags) instead. http://go.microsoft.com/fwlink/?linkid=14202")]
	public AssemblyFlagsAttribute(int assemblyFlags)
	{
		m_flags = (AssemblyNameFlags)assemblyFlags;
	}

	public AssemblyFlagsAttribute(AssemblyNameFlags assemblyFlags)
	{
		m_flags = assemblyFlags;
	}
}
