using System.Reflection;
using System.Runtime.InteropServices;

namespace System;

[ComVisible(true)]
public class ResolveEventArgs : EventArgs
{
	private string m_Name;

	private Assembly m_Requesting;

	public string Name => m_Name;

	public Assembly RequestingAssembly => m_Requesting;

	public ResolveEventArgs(string name)
	{
		m_Name = name;
	}

	public ResolveEventArgs(string name, Assembly requestingAssembly)
	{
		m_Name = name;
		m_Requesting = requestingAssembly;
	}
}
