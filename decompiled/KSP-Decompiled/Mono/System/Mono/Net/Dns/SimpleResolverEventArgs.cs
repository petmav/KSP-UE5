using System;
using System.Net;
using System.Threading;

namespace Mono.Net.Dns;

internal class SimpleResolverEventArgs : EventArgs
{
	public ResolverAsyncOperation LastOperation;

	internal ushort QueryID;

	internal ushort Retries;

	internal Timer Timer;

	internal IPAddress PTRAddress;

	public ResolverError ResolverError { get; set; }

	public string ErrorMessage { get; set; }

	public string HostName { get; set; }

	public IPHostEntry HostEntry { get; internal set; }

	public object UserToken { get; set; }

	public event EventHandler<SimpleResolverEventArgs> Completed;

	internal void Reset(ResolverAsyncOperation op)
	{
		ResolverError = ResolverError.NoError;
		ErrorMessage = null;
		HostEntry = null;
		LastOperation = op;
		QueryID = 0;
		Retries = 0;
		PTRAddress = null;
	}

	protected internal void OnCompleted(object sender)
	{
		this.Completed?.Invoke(sender, this);
	}
}
