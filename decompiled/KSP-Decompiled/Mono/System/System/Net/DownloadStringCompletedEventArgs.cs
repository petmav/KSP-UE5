using System.ComponentModel;
using Unity;

namespace System.Net;

public class DownloadStringCompletedEventArgs : AsyncCompletedEventArgs
{
	private string m_Result;

	public string Result
	{
		get
		{
			RaiseExceptionIfNecessary();
			return m_Result;
		}
	}

	internal DownloadStringCompletedEventArgs(string result, Exception exception, bool cancelled, object userToken)
		: base(exception, cancelled, userToken)
	{
		m_Result = result;
	}

	internal DownloadStringCompletedEventArgs()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
