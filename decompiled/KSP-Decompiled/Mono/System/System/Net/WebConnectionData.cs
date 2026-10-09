using System.IO;

namespace System.Net;

internal class WebConnectionData
{
	private HttpWebRequest _request;

	public int StatusCode;

	public string StatusDescription;

	public WebHeaderCollection Headers;

	public Version Version;

	public Version ProxyVersion;

	public Stream stream;

	public string[] Challenge;

	private ReadState _readState;

	public HttpWebRequest request
	{
		get
		{
			return _request;
		}
		set
		{
			_request = value;
		}
	}

	public ReadState ReadState
	{
		get
		{
			return _readState;
		}
		set
		{
			lock (this)
			{
				if (_readState == ReadState.Aborted && value != ReadState.Aborted)
				{
					throw new WebException("Aborted", WebExceptionStatus.RequestCanceled);
				}
				_readState = value;
			}
		}
	}

	public WebConnectionData()
	{
		_readState = ReadState.None;
	}

	public WebConnectionData(HttpWebRequest request)
	{
		_request = request;
	}
}
