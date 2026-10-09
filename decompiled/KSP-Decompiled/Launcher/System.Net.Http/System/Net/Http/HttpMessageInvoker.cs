using System.Threading;
using System.Threading.Tasks;

namespace System.Net.Http;

public class HttpMessageInvoker : IDisposable
{
	private HttpMessageHandler handler;

	private readonly bool disposeHandler;

	public HttpMessageInvoker(HttpMessageHandler handler)
		: this(handler, disposeHandler: true)
	{
	}

	public HttpMessageInvoker(HttpMessageHandler handler, bool disposeHandler)
	{
		if (handler == null)
		{
			throw new ArgumentNullException("handler");
		}
		this.handler = handler;
		this.disposeHandler = disposeHandler;
	}

	public void Dispose()
	{
		Dispose(disposing: true);
	}

	protected virtual void Dispose(bool disposing)
	{
		if (disposing && disposeHandler && handler != null)
		{
			handler.Dispose();
			handler = null;
		}
	}

	public virtual Task<HttpResponseMessage> SendAsync(HttpRequestMessage request, CancellationToken cancellationToken)
	{
		return handler.SendAsync(request, cancellationToken);
	}
}
