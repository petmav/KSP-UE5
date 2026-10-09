using System.Runtime.InteropServices;
using System.Runtime.Serialization;

namespace System.Threading;

[Serializable]
[ComVisible(false)]
public class AbandonedMutexException : SystemException
{
	private int m_MutexIndex = -1;

	private Mutex m_Mutex;

	public Mutex Mutex => m_Mutex;

	public int MutexIndex => m_MutexIndex;

	public AbandonedMutexException()
		: base(Environment.GetResourceString("The wait completed due to an abandoned mutex."))
	{
		SetErrorCode(-2146233043);
	}

	public AbandonedMutexException(string message)
		: base(message)
	{
		SetErrorCode(-2146233043);
	}

	public AbandonedMutexException(string message, Exception inner)
		: base(message, inner)
	{
		SetErrorCode(-2146233043);
	}

	public AbandonedMutexException(int location, WaitHandle handle)
		: base(Environment.GetResourceString("The wait completed due to an abandoned mutex."))
	{
		SetErrorCode(-2146233043);
		SetupException(location, handle);
	}

	public AbandonedMutexException(string message, int location, WaitHandle handle)
		: base(message)
	{
		SetErrorCode(-2146233043);
		SetupException(location, handle);
	}

	public AbandonedMutexException(string message, Exception inner, int location, WaitHandle handle)
		: base(message, inner)
	{
		SetErrorCode(-2146233043);
		SetupException(location, handle);
	}

	private void SetupException(int location, WaitHandle handle)
	{
		m_MutexIndex = location;
		if (handle != null)
		{
			m_Mutex = handle as Mutex;
		}
	}

	protected AbandonedMutexException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
	}
}
