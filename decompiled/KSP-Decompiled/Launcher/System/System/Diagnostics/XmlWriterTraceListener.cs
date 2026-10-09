using System.IO;
using System.Security.Permissions;
using Unity;

namespace System.Diagnostics;

[HostProtection(SecurityAction.LinkDemand, Synchronization = true)]
public class XmlWriterTraceListener : TextWriterTraceListener
{
	public XmlWriterTraceListener(Stream stream)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public XmlWriterTraceListener(Stream stream, string name)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public XmlWriterTraceListener(TextWriter writer)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public XmlWriterTraceListener(TextWriter writer, string name)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public XmlWriterTraceListener(string filename)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public XmlWriterTraceListener(string filename, string name)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
