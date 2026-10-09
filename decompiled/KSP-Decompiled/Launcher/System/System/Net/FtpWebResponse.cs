using System.IO;
using Unity;

namespace System.Net;

public class FtpWebResponse : WebResponse
{
	private Stream stream;

	private Uri uri;

	private FtpStatusCode statusCode;

	private DateTime lastModified;

	private string bannerMessage;

	private string welcomeMessage;

	private string exitMessage;

	private string statusDescription;

	private string method;

	private bool disposed;

	private FtpWebRequest request;

	internal long contentLength;

	public override long ContentLength => contentLength;

	public override WebHeaderCollection Headers => new WebHeaderCollection();

	public override Uri ResponseUri => uri;

	public DateTime LastModified
	{
		get
		{
			return lastModified;
		}
		internal set
		{
			lastModified = value;
		}
	}

	public string BannerMessage
	{
		get
		{
			return bannerMessage;
		}
		internal set
		{
			bannerMessage = value;
		}
	}

	public string WelcomeMessage
	{
		get
		{
			return welcomeMessage;
		}
		internal set
		{
			welcomeMessage = value;
		}
	}

	public string ExitMessage
	{
		get
		{
			return exitMessage;
		}
		internal set
		{
			exitMessage = value;
		}
	}

	public FtpStatusCode StatusCode
	{
		get
		{
			return statusCode;
		}
		internal set
		{
			statusCode = value;
		}
	}

	public override bool SupportsHeaders => true;

	public string StatusDescription
	{
		get
		{
			return statusDescription;
		}
		internal set
		{
			statusDescription = value;
		}
	}

	internal Stream Stream
	{
		get
		{
			return stream;
		}
		set
		{
			stream = value;
		}
	}

	internal FtpWebResponse(FtpWebRequest request, Uri uri, string method, bool keepAlive)
	{
		lastModified = DateTime.MinValue;
		bannerMessage = string.Empty;
		welcomeMessage = string.Empty;
		exitMessage = string.Empty;
		contentLength = -1L;
		base._002Ector();
		this.request = request;
		this.uri = uri;
		this.method = method;
	}

	internal FtpWebResponse(FtpWebRequest request, Uri uri, string method, FtpStatusCode statusCode, string statusDescription)
	{
		lastModified = DateTime.MinValue;
		bannerMessage = string.Empty;
		welcomeMessage = string.Empty;
		exitMessage = string.Empty;
		contentLength = -1L;
		base._002Ector();
		this.request = request;
		this.uri = uri;
		this.method = method;
		this.statusCode = statusCode;
		this.statusDescription = statusDescription;
	}

	internal FtpWebResponse(FtpWebRequest request, Uri uri, string method, FtpStatus status)
		: this(request, uri, method, status.StatusCode, status.StatusDescription)
	{
	}

	public override void Close()
	{
		if (disposed)
		{
			return;
		}
		disposed = true;
		if (stream != null)
		{
			stream.Close();
			if (stream == Stream.Null)
			{
				request.OperationCompleted();
			}
		}
		stream = null;
	}

	public override Stream GetResponseStream()
	{
		if (stream == null)
		{
			return Stream.Null;
		}
		if (method != "RETR" && method != "NLST")
		{
			CheckDisposed();
		}
		return stream;
	}

	internal void UpdateStatus(FtpStatus status)
	{
		statusCode = status.StatusCode;
		statusDescription = status.StatusDescription;
	}

	private void CheckDisposed()
	{
		if (disposed)
		{
			throw new ObjectDisposedException(GetType().FullName);
		}
	}

	internal bool IsFinal()
	{
		return statusCode >= FtpStatusCode.CommandOK;
	}

	internal FtpWebResponse()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
