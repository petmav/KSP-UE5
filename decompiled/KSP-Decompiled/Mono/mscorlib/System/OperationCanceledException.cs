using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using System.Threading;

namespace System;

[Serializable]
[ComVisible(true)]
public class OperationCanceledException : SystemException
{
	[NonSerialized]
	private CancellationToken _cancellationToken;

	public CancellationToken CancellationToken
	{
		get
		{
			return _cancellationToken;
		}
		private set
		{
			_cancellationToken = value;
		}
	}

	public OperationCanceledException()
		: base(Environment.GetResourceString("The operation was canceled."))
	{
		SetErrorCode(-2146233029);
	}

	public OperationCanceledException(string message)
		: base(message)
	{
		SetErrorCode(-2146233029);
	}

	public OperationCanceledException(string message, Exception innerException)
		: base(message, innerException)
	{
		SetErrorCode(-2146233029);
	}

	public OperationCanceledException(CancellationToken token)
		: this()
	{
		CancellationToken = token;
	}

	public OperationCanceledException(string message, CancellationToken token)
		: this(message)
	{
		CancellationToken = token;
	}

	public OperationCanceledException(string message, Exception innerException, CancellationToken token)
		: this(message, innerException)
	{
		CancellationToken = token;
	}

	protected OperationCanceledException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
	}
}
