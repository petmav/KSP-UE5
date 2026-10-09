using System.Threading;
using System.Threading.Tasks;

namespace System.Net.Http;

public abstract class DelegatingHandler : HttpMessageHandler
{
	private bool disposed;

	private HttpMessageHandler handler;

	public HttpMessageHandler InnerHandler
	{
		get
		{
			return handler;
		}
		set
		{
			if (value == null)
			{
				throw new ArgumentNullException("InnerHandler");
			}
			handler = value;
		}
	}

	protected DelegatingHandler()
	{
	}

	protected DelegatingHandler(HttpMessageHandler innerHandler)
	{
		if (innerHandler == null)
		{
			throw new ArgumentNullException("innerHandler");
		}
		InnerHandler = innerHandler;
	}

	protected override void Dispose(bool disposing)
	{
		if (disposing && !disposed)
		{
			disposed = true;
			if (InnerHandler != null)
			{
				InnerHandler.Dispose();
			}
		}
		base.Dispose(disposing);
	}

	protected internal override Task<HttpResponseMessage> SendAsync(HttpRequestMessage request, CancellationToken cancellationToken)
	{
		return InnerHandler.SendAsync(request, cancellationToken);
	}
}
