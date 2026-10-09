using System.Security.Permissions;
using Unity;

namespace System.Diagnostics;

[HostProtection(SecurityAction.LinkDemand, Synchronization = true)]
[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
public class EventSchemaTraceListener : TextWriterTraceListener
{
	public int BufferSize
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public long MaximumFileSize
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(long);
		}
	}

	public int MaximumNumberOfFiles
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public TraceLogRetentionOption TraceLogRetentionOption
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(TraceLogRetentionOption);
		}
	}

	public EventSchemaTraceListener(string fileName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EventSchemaTraceListener(string fileName, string name)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EventSchemaTraceListener(string fileName, string name, int bufferSize)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EventSchemaTraceListener(string fileName, string name, int bufferSize, TraceLogRetentionOption logRetentionOption)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EventSchemaTraceListener(string fileName, string name, int bufferSize, TraceLogRetentionOption logRetentionOption, long maximumFileSize)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public EventSchemaTraceListener(string fileName, string name, int bufferSize, TraceLogRetentionOption logRetentionOption, long maximumFileSize, int maximumNumberOfFiles)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
