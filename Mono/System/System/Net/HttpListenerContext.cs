using System.Net.WebSockets;
using System.Security.Principal;
using System.Text;
using System.Threading.Tasks;
using Unity;

namespace System.Net;

public sealed class HttpListenerContext
{
	private HttpListenerRequest request;

	private HttpListenerResponse response;

	private IPrincipal user;

	private HttpConnection cnc;

	private string error;

	private int err_status;

	internal HttpListener Listener;

	internal int ErrorStatus
	{
		get
		{
			return err_status;
		}
		set
		{
			err_status = value;
		}
	}

	internal string ErrorMessage
	{
		get
		{
			return error;
		}
		set
		{
			error = value;
		}
	}

	internal bool HaveError => error != null;

	internal HttpConnection Connection => cnc;

	public HttpListenerRequest Request => request;

	public HttpListenerResponse Response => response;

	public IPrincipal User => user;

	internal HttpListenerContext(HttpConnection cnc)
	{
		err_status = 400;
		base._002Ector();
		this.cnc = cnc;
		request = new HttpListenerRequest(this);
		response = new HttpListenerResponse(this);
	}

	internal void ParseAuthentication(AuthenticationSchemes expectedSchemes)
	{
		if (expectedSchemes == AuthenticationSchemes.Anonymous)
		{
			return;
		}
		string text = request.Headers["Authorization"];
		if (text != null && text.Length >= 2)
		{
			string[] array = text.Split(new char[1] { ' ' }, 2);
			if (string.Compare(array[0], "basic", ignoreCase: true) == 0)
			{
				user = ParseBasicAuthentication(array[1]);
			}
		}
	}

	internal IPrincipal ParseBasicAuthentication(string authData)
	{
		try
		{
			string text = null;
			string text2 = null;
			int num = -1;
			string @string = Encoding.Default.GetString(Convert.FromBase64String(authData));
			num = @string.IndexOf(':');
			text2 = @string.Substring(num + 1);
			@string = @string.Substring(0, num);
			num = @string.IndexOf('\\');
			text = ((num <= 0) ? @string : @string.Substring(num));
			return new GenericPrincipal(new HttpListenerBasicIdentity(text, text2), new string[0]);
		}
		catch (Exception)
		{
			return null;
		}
	}

	[System.MonoTODO]
	public Task<HttpListenerWebSocketContext> AcceptWebSocketAsync(string subProtocol)
	{
		throw new NotImplementedException();
	}

	[System.MonoTODO]
	public Task<HttpListenerWebSocketContext> AcceptWebSocketAsync(string subProtocol, TimeSpan keepAliveInterval)
	{
		throw new NotImplementedException();
	}

	[System.MonoTODO]
	public Task<HttpListenerWebSocketContext> AcceptWebSocketAsync(string subProtocol, int receiveBufferSize, TimeSpan keepAliveInterval)
	{
		throw new NotImplementedException();
	}

	[System.MonoTODO]
	public Task<HttpListenerWebSocketContext> AcceptWebSocketAsync(string subProtocol, int receiveBufferSize, TimeSpan keepAliveInterval, ArraySegment<byte> internalBuffer)
	{
		throw new NotImplementedException();
	}

	internal HttpListenerContext()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
