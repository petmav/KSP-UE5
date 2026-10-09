using System.Collections;
using System.Diagnostics;
using System.IO;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using Mono.Net.Security;

namespace System.Net;

internal class WebConnection
{
	private enum NtlmAuthState
	{
		None,
		Challenge,
		Response
	}

	private class AbortHelper
	{
		public WebConnection Connection;

		public void Abort(object sender, EventArgs args)
		{
			WebConnection webConnection = ((HttpWebRequest)sender).WebConnection;
			if (webConnection == null)
			{
				webConnection = Connection;
			}
			webConnection.Abort(sender, args);
		}
	}

	private ServicePoint sPoint;

	private Stream nstream;

	internal Socket socket;

	private object socketLock = new object();

	private IWebConnectionState state;

	private WebExceptionStatus status;

	private bool keepAlive;

	private byte[] buffer;

	private EventHandler abortHandler;

	private AbortHelper abortHelper;

	internal WebConnectionData Data;

	private bool chunkedRead;

	private MonoChunkStream chunkStream;

	private Queue queue;

	private bool reused;

	private int position;

	private HttpWebRequest priority_request;

	private NetworkCredential ntlm_credentials;

	private bool ntlm_authenticated;

	private bool unsafe_sharing;

	private NtlmAuthState connect_ntlm_auth_state;

	private HttpWebRequest connect_request;

	private Exception connect_exception;

	private MonoTlsStream tlsStream;

	internal MonoChunkStream MonoChunkStream => chunkStream;

	internal bool Connected
	{
		get
		{
			lock (this)
			{
				return socket != null && socket.Connected;
			}
		}
	}

	internal HttpWebRequest PriorityRequest
	{
		set
		{
			priority_request = value;
		}
	}

	internal bool NtlmAuthenticated
	{
		get
		{
			return ntlm_authenticated;
		}
		set
		{
			ntlm_authenticated = value;
		}
	}

	internal NetworkCredential NtlmCredential
	{
		get
		{
			return ntlm_credentials;
		}
		set
		{
			ntlm_credentials = value;
		}
	}

	internal bool UnsafeAuthenticatedConnectionSharing
	{
		get
		{
			return unsafe_sharing;
		}
		set
		{
			unsafe_sharing = value;
		}
	}

	public WebConnection(IWebConnectionState wcs, ServicePoint sPoint)
	{
		state = wcs;
		this.sPoint = sPoint;
		buffer = new byte[4096];
		Data = new WebConnectionData();
		queue = wcs.Group.Queue;
		abortHelper = new AbortHelper();
		abortHelper.Connection = this;
		abortHandler = abortHelper.Abort;
	}

	private bool CanReuse()
	{
		return !socket.Poll(0, SelectMode.SelectRead);
	}

	private void Connect(HttpWebRequest request)
	{
		lock (socketLock)
		{
			if (this.socket != null && this.socket.Connected && status == WebExceptionStatus.Success && CanReuse() && CompleteChunkedRead())
			{
				reused = true;
				return;
			}
			reused = false;
			if (this.socket != null)
			{
				this.socket.Close();
				this.socket = null;
			}
			chunkStream = null;
			IPHostEntry hostEntry = sPoint.HostEntry;
			if (hostEntry == null)
			{
				status = ((!sPoint.UsesProxy) ? WebExceptionStatus.NameResolutionFailure : WebExceptionStatus.ProxyNameResolutionFailure);
				return;
			}
			IPAddress[] addressList = hostEntry.AddressList;
			foreach (IPAddress iPAddress in addressList)
			{
				try
				{
					this.socket = new Socket(iPAddress.AddressFamily, SocketType.Stream, ProtocolType.Tcp);
				}
				catch (Exception ex)
				{
					if (!request.Aborted)
					{
						status = WebExceptionStatus.ConnectFailure;
					}
					connect_exception = ex;
					break;
				}
				IPEndPoint iPEndPoint = new IPEndPoint(iPAddress, sPoint.Address.Port);
				this.socket.NoDelay = !sPoint.UseNagleAlgorithm;
				try
				{
					sPoint.KeepAliveSetup(this.socket);
				}
				catch
				{
				}
				if (!sPoint.CallEndPointDelegate(this.socket, iPEndPoint))
				{
					this.socket.Close();
					this.socket = null;
					status = WebExceptionStatus.ConnectFailure;
					continue;
				}
				try
				{
					if (!request.Aborted)
					{
						this.socket.Connect(iPEndPoint);
						status = WebExceptionStatus.Success;
					}
					break;
				}
				catch (ThreadAbortException)
				{
					Socket socket = this.socket;
					this.socket = null;
					socket?.Close();
					break;
				}
				catch (ObjectDisposedException)
				{
					break;
				}
				catch (Exception ex4)
				{
					Socket socket2 = this.socket;
					this.socket = null;
					socket2?.Close();
					if (!request.Aborted)
					{
						status = WebExceptionStatus.ConnectFailure;
					}
					connect_exception = ex4;
				}
			}
		}
	}

	private bool CreateTunnel(HttpWebRequest request, Uri connectUri, Stream stream, out byte[] buffer)
	{
		StringBuilder stringBuilder = new StringBuilder();
		stringBuilder.Append("CONNECT ");
		stringBuilder.Append(request.Address.Host);
		stringBuilder.Append(':');
		stringBuilder.Append(request.Address.Port);
		stringBuilder.Append(" HTTP/");
		if (request.ServicePoint.ProtocolVersion == HttpVersion.Version11)
		{
			stringBuilder.Append("1.1");
		}
		else
		{
			stringBuilder.Append("1.0");
		}
		stringBuilder.Append("\r\nHost: ");
		stringBuilder.Append(request.Address.Authority);
		bool flag = false;
		string[] challenge = Data.Challenge;
		Data.Challenge = null;
		string text = request.Headers["Proxy-Authorization"];
		bool flag2 = text != null;
		if (flag2)
		{
			stringBuilder.Append("\r\nProxy-Authorization: ");
			stringBuilder.Append(text);
			flag = text.ToUpper().Contains("NTLM");
		}
		else if (challenge != null && Data.StatusCode == 407)
		{
			ICredentials credentials = request.Proxy.Credentials;
			flag2 = true;
			if (connect_request == null)
			{
				connect_request = (HttpWebRequest)WebRequest.Create(connectUri.Scheme + "://" + connectUri.Host + ":" + connectUri.Port + "/");
				connect_request.Method = "CONNECT";
				connect_request.Credentials = credentials;
			}
			if (credentials != null)
			{
				for (int i = 0; i < challenge.Length; i++)
				{
					Authorization authorization = AuthenticationManager.Authenticate(challenge[i], connect_request, credentials);
					if (authorization != null)
					{
						flag = authorization.ModuleAuthenticationType == "NTLM";
						stringBuilder.Append("\r\nProxy-Authorization: ");
						stringBuilder.Append(authorization.Message);
						break;
					}
				}
			}
		}
		if (flag)
		{
			stringBuilder.Append("\r\nProxy-Connection: keep-alive");
			connect_ntlm_auth_state++;
		}
		stringBuilder.Append("\r\n\r\n");
		Data.StatusCode = 0;
		byte[] bytes = Encoding.Default.GetBytes(stringBuilder.ToString());
		stream.Write(bytes, 0, bytes.Length);
		int num;
		WebHeaderCollection webHeaderCollection = ReadHeaders(stream, out buffer, out num);
		if ((!flag2 || connect_ntlm_auth_state == NtlmAuthState.Challenge) && webHeaderCollection != null && num == 407)
		{
			string text2 = webHeaderCollection["Connection"];
			if (socket != null && !string.IsNullOrEmpty(text2) && text2.ToLower() == "close")
			{
				socket.Close();
				socket = null;
			}
			Data.StatusCode = num;
			Data.Challenge = webHeaderCollection.GetValues("Proxy-Authenticate");
			Data.Headers = webHeaderCollection;
			return false;
		}
		if (num != 200)
		{
			Data.StatusCode = num;
			Data.Headers = webHeaderCollection;
			return false;
		}
		return webHeaderCollection != null;
	}

	private WebHeaderCollection ReadHeaders(Stream stream, out byte[] retBuffer, out int status)
	{
		retBuffer = null;
		status = 200;
		byte[] array = new byte[1024];
		MemoryStream memoryStream = new MemoryStream();
		while (true)
		{
			int num = stream.Read(array, 0, 1024);
			if (num == 0)
			{
				break;
			}
			memoryStream.Write(array, 0, num);
			int start = 0;
			string output = null;
			bool flag = false;
			WebHeaderCollection webHeaderCollection = new WebHeaderCollection();
			while (ReadLine(memoryStream.GetBuffer(), ref start, (int)memoryStream.Length, ref output))
			{
				if (output == null)
				{
					int num2 = 0;
					try
					{
						num2 = int.Parse(webHeaderCollection["Content-Length"]);
					}
					catch
					{
						num2 = 0;
					}
					if (memoryStream.Length - start - num2 > 0)
					{
						retBuffer = new byte[memoryStream.Length - start - num2];
						Buffer.BlockCopy(memoryStream.GetBuffer(), start + num2, retBuffer, 0, retBuffer.Length);
					}
					else
					{
						FlushContents(stream, num2 - (int)(memoryStream.Length - start));
					}
					return webHeaderCollection;
				}
				if (flag)
				{
					webHeaderCollection.Add(output);
					continue;
				}
				string[] array2 = output.Split(' ');
				if (array2.Length < 2)
				{
					HandleError(WebExceptionStatus.ServerProtocolViolation, null, "ReadHeaders2");
					return null;
				}
				if (string.Compare(array2[0], "HTTP/1.1", ignoreCase: true) == 0)
				{
					Data.ProxyVersion = HttpVersion.Version11;
				}
				else
				{
					if (string.Compare(array2[0], "HTTP/1.0", ignoreCase: true) != 0)
					{
						HandleError(WebExceptionStatus.ServerProtocolViolation, null, "ReadHeaders2");
						return null;
					}
					Data.ProxyVersion = HttpVersion.Version10;
				}
				status = (int)uint.Parse(array2[1]);
				if (array2.Length >= 3)
				{
					Data.StatusDescription = string.Join(" ", array2, 2, array2.Length - 2);
				}
				flag = true;
			}
		}
		HandleError(WebExceptionStatus.ServerProtocolViolation, null, "ReadHeaders");
		return null;
	}

	private void FlushContents(Stream stream, int contentLength)
	{
		while (contentLength > 0)
		{
			byte[] array = new byte[contentLength];
			int num = stream.Read(array, 0, contentLength);
			if (num > 0)
			{
				contentLength -= num;
				continue;
			}
			break;
		}
	}

	private bool CreateStream(HttpWebRequest request)
	{
		try
		{
			NetworkStream networkStream = new NetworkStream(socket, ownsSocket: false);
			if (request.Address.Scheme == Uri.UriSchemeHttps)
			{
				if (!reused || nstream == null || tlsStream == null)
				{
					byte[] array = null;
					if (sPoint.UseConnect && !CreateTunnel(request, sPoint.Address, networkStream, out array))
					{
						return false;
					}
					tlsStream = new MonoTlsStream(request, networkStream);
					nstream = tlsStream.CreateStream(array);
				}
			}
			else
			{
				nstream = networkStream;
			}
		}
		catch (Exception ex)
		{
			if (tlsStream != null)
			{
				status = tlsStream.ExceptionStatus;
			}
			else if (!request.Aborted)
			{
				status = WebExceptionStatus.ConnectFailure;
			}
			connect_exception = ex;
			return false;
		}
		return true;
	}

	private void HandleError(WebExceptionStatus st, Exception e, string where)
	{
		status = st;
		lock (this)
		{
			if (st == WebExceptionStatus.RequestCanceled)
			{
				Data = new WebConnectionData();
			}
		}
		if (e == null)
		{
			try
			{
				throw new Exception(new StackTrace().ToString());
			}
			catch (Exception ex)
			{
				e = ex;
			}
		}
		HttpWebRequest httpWebRequest = null;
		if (Data != null && Data.request != null)
		{
			httpWebRequest = Data.request;
		}
		Close(sendNext: true);
		if (httpWebRequest != null)
		{
			httpWebRequest.FinishedReading = true;
			httpWebRequest.SetResponseError(st, e, where);
		}
	}

	private void ReadDone(IAsyncResult result)
	{
		WebConnectionData data = Data;
		Stream stream = nstream;
		if (stream == null)
		{
			Close(sendNext: true);
			return;
		}
		int num = -1;
		try
		{
			num = stream.EndRead(result);
		}
		catch (ObjectDisposedException)
		{
			return;
		}
		catch (Exception ex2)
		{
			if (!(ex2.InnerException is ObjectDisposedException))
			{
				HandleError(WebExceptionStatus.ReceiveFailure, ex2, "ReadDone1");
			}
			return;
		}
		if (num == 0)
		{
			HandleError(WebExceptionStatus.ReceiveFailure, null, "ReadDone2");
			return;
		}
		if (num < 0)
		{
			HandleError(WebExceptionStatus.ServerProtocolViolation, null, "ReadDone3");
			return;
		}
		int num2 = -1;
		num += position;
		if (data.ReadState == ReadState.None)
		{
			Exception ex3 = null;
			try
			{
				num2 = GetResponse(data, sPoint, buffer, num);
			}
			catch (Exception ex4)
			{
				ex3 = ex4;
			}
			if (ex3 != null || num2 == -1)
			{
				HandleError(WebExceptionStatus.ServerProtocolViolation, ex3, "ReadDone4");
				return;
			}
		}
		if (data.ReadState == ReadState.Aborted)
		{
			HandleError(WebExceptionStatus.RequestCanceled, null, "ReadDone");
			return;
		}
		if (data.ReadState != ReadState.Content)
		{
			int num3 = num * 2;
			byte[] dst = new byte[(num3 < buffer.Length) ? buffer.Length : num3];
			Buffer.BlockCopy(buffer, 0, dst, 0, num);
			buffer = dst;
			position = num;
			data.ReadState = ReadState.None;
			InitRead();
			return;
		}
		position = 0;
		WebConnectionStream webConnectionStream = new WebConnectionStream(this, data);
		bool flag = ExpectContent(data.StatusCode, data.request.Method);
		string text = null;
		if (flag)
		{
			text = data.Headers["Transfer-Encoding"];
		}
		chunkedRead = text != null && text.IndexOf("chunked", StringComparison.OrdinalIgnoreCase) != -1;
		if (!chunkedRead)
		{
			webConnectionStream.ReadBuffer = buffer;
			webConnectionStream.ReadBufferOffset = num2;
			webConnectionStream.ReadBufferSize = num;
			try
			{
				webConnectionStream.CheckResponseInBuffer();
			}
			catch (Exception e)
			{
				HandleError(WebExceptionStatus.ReceiveFailure, e, "ReadDone7");
			}
		}
		else if (chunkStream == null)
		{
			try
			{
				chunkStream = new MonoChunkStream(buffer, num2, num, data.Headers);
			}
			catch (Exception e2)
			{
				HandleError(WebExceptionStatus.ServerProtocolViolation, e2, "ReadDone5");
				return;
			}
		}
		else
		{
			chunkStream.ResetBuffer();
			try
			{
				chunkStream.Write(buffer, num2, num);
			}
			catch (Exception e3)
			{
				HandleError(WebExceptionStatus.ServerProtocolViolation, e3, "ReadDone6");
				return;
			}
		}
		data.stream = webConnectionStream;
		if (!flag)
		{
			webConnectionStream.ForceCompletion();
		}
		data.request.SetResponseData(data);
	}

	private static bool ExpectContent(int statusCode, string method)
	{
		if (method == "HEAD")
		{
			return false;
		}
		if (statusCode >= 200 && statusCode != 204)
		{
			return statusCode != 304;
		}
		return false;
	}

	internal void InitRead()
	{
		Stream stream = nstream;
		try
		{
			int count = buffer.Length - position;
			stream.BeginRead(buffer, position, count, ReadDone, null);
		}
		catch (Exception e)
		{
			HandleError(WebExceptionStatus.ReceiveFailure, e, "InitRead");
		}
	}

	private static int GetResponse(WebConnectionData data, ServicePoint sPoint, byte[] buffer, int max)
	{
		int start = 0;
		string output = null;
		bool flag = false;
		bool flag2 = false;
		do
		{
			if (data.ReadState == ReadState.Aborted)
			{
				return -1;
			}
			if (data.ReadState == ReadState.None)
			{
				if (!ReadLine(buffer, ref start, max, ref output))
				{
					return 0;
				}
				if (output == null)
				{
					flag2 = true;
					continue;
				}
				flag2 = false;
				data.ReadState = ReadState.Status;
				string[] array = output.Split(' ');
				if (array.Length < 2)
				{
					return -1;
				}
				if (string.Compare(array[0], "HTTP/1.1", ignoreCase: true) == 0)
				{
					data.Version = HttpVersion.Version11;
					sPoint.SetVersion(HttpVersion.Version11);
				}
				else
				{
					data.Version = HttpVersion.Version10;
					sPoint.SetVersion(HttpVersion.Version10);
				}
				data.StatusCode = (int)uint.Parse(array[1]);
				if (array.Length >= 3)
				{
					data.StatusDescription = string.Join(" ", array, 2, array.Length - 2);
				}
				else
				{
					data.StatusDescription = "";
				}
				if (start >= max)
				{
					return start;
				}
			}
			flag2 = false;
			if (data.ReadState != ReadState.Status)
			{
				continue;
			}
			data.ReadState = ReadState.Headers;
			data.Headers = new WebHeaderCollection();
			ArrayList arrayList = new ArrayList();
			bool flag3 = false;
			while (!flag3 && ReadLine(buffer, ref start, max, ref output))
			{
				if (output == null)
				{
					flag3 = true;
				}
				else if (output.Length > 0 && (output[0] == ' ' || output[0] == '\t'))
				{
					int num = arrayList.Count - 1;
					if (num < 0)
					{
						break;
					}
					string value = (string)arrayList[num] + output;
					arrayList[num] = value;
				}
				else
				{
					arrayList.Add(output);
				}
			}
			if (!flag3)
			{
				return 0;
			}
			foreach (string item in arrayList)
			{
				int num2 = item.IndexOf(':');
				if (num2 == -1)
				{
					throw new ArgumentException("no colon found", "header");
				}
				string name = item.Substring(0, num2);
				string value2 = item.Substring(num2 + 1).Trim();
				WebHeaderCollection headers = data.Headers;
				if (WebHeaderCollection.AllowMultiValues(name))
				{
					headers.AddInternal(name, value2);
				}
				else
				{
					headers.SetInternal(name, value2);
				}
			}
			if (data.StatusCode == 100)
			{
				sPoint.SendContinue = true;
				if (start >= max)
				{
					return start;
				}
				if (data.request.ExpectContinue)
				{
					data.request.DoContinueDelegate(data.StatusCode, data.Headers);
					data.request.ExpectContinue = false;
				}
				data.ReadState = ReadState.None;
				flag = true;
				continue;
			}
			data.ReadState = ReadState.Content;
			return start;
		}
		while (flag2 || flag);
		return -1;
	}

	private void InitConnection(HttpWebRequest request)
	{
		request.WebConnection = this;
		if (request.ReuseConnection)
		{
			request.StoredConnection = this;
		}
		if (request.Aborted)
		{
			return;
		}
		keepAlive = request.KeepAlive;
		Data = new WebConnectionData(request);
		while (true)
		{
			Connect(request);
			if (request.Aborted)
			{
				return;
			}
			if (status != 0)
			{
				if (!request.Aborted)
				{
					request.SetWriteStreamError(status, connect_exception);
					Close(sendNext: true);
				}
				return;
			}
			if (CreateStream(request))
			{
				break;
			}
			if (request.Aborted)
			{
				return;
			}
			WebExceptionStatus webExceptionStatus = status;
			if (Data.Challenge != null)
			{
				continue;
			}
			Exception ex = connect_exception;
			if (ex == null && (Data.StatusCode == 401 || Data.StatusCode == 407))
			{
				webExceptionStatus = WebExceptionStatus.ProtocolError;
				if (Data.Headers == null)
				{
					Data.Headers = new WebHeaderCollection();
				}
				HttpWebResponse response = new HttpWebResponse(sPoint.Address, "CONNECT", Data, null);
				ex = new WebException((Data.StatusCode == 407) ? "(407) Proxy Authentication Required" : "(401) Unauthorized", null, webExceptionStatus, response);
			}
			connect_exception = null;
			request.SetWriteStreamError(webExceptionStatus, ex);
			Close(sendNext: true);
			return;
		}
		request.SetWriteStream(new WebConnectionStream(this, request));
	}

	internal EventHandler SendRequest(HttpWebRequest request)
	{
		if (request.Aborted)
		{
			return null;
		}
		lock (this)
		{
			if (state.TrySetBusy())
			{
				status = WebExceptionStatus.Success;
				ThreadPool.QueueUserWorkItem(delegate(object o)
				{
					try
					{
						InitConnection((HttpWebRequest)o);
					}
					catch
					{
					}
				}, request);
			}
			else
			{
				lock (queue)
				{
					queue.Enqueue(request);
				}
			}
		}
		return abortHandler;
	}

	private void SendNext()
	{
		lock (queue)
		{
			if (queue.Count > 0)
			{
				SendRequest((HttpWebRequest)queue.Dequeue());
			}
		}
	}

	internal void NextRead()
	{
		lock (this)
		{
			if (Data.request != null)
			{
				Data.request.FinishedReading = true;
			}
			string name = (sPoint.UsesProxy ? "Proxy-Connection" : "Connection");
			string text = ((Data.Headers != null) ? Data.Headers[name] : null);
			bool flag = Data.Version == HttpVersion.Version11 && keepAlive;
			if (Data.ProxyVersion != null && Data.ProxyVersion != HttpVersion.Version11)
			{
				flag = false;
			}
			if (text != null)
			{
				text = text.ToLower();
				flag = keepAlive && text.IndexOf("keep-alive", StringComparison.Ordinal) != -1;
			}
			if ((socket != null && !socket.Connected) || !flag || (text != null && text.IndexOf("close", StringComparison.Ordinal) != -1))
			{
				Close(sendNext: false);
			}
			state.SetIdle();
			if (priority_request != null)
			{
				SendRequest(priority_request);
				priority_request = null;
			}
			else
			{
				SendNext();
			}
		}
	}

	private static bool ReadLine(byte[] buffer, ref int start, int max, ref string output)
	{
		bool flag = false;
		StringBuilder stringBuilder = new StringBuilder();
		int num = 0;
		while (start < max)
		{
			num = buffer[start++];
			if (num == 10)
			{
				if (stringBuilder.Length > 0 && stringBuilder[stringBuilder.Length - 1] == '\r')
				{
					stringBuilder.Length--;
				}
				flag = false;
				break;
			}
			if (flag)
			{
				stringBuilder.Length--;
				break;
			}
			if (num == 13)
			{
				flag = true;
			}
			stringBuilder.Append((char)num);
		}
		if (num != 10 && num != 13)
		{
			return false;
		}
		if (stringBuilder.Length == 0)
		{
			output = null;
			if (num != 10)
			{
				return num == 13;
			}
			return true;
		}
		if (flag)
		{
			stringBuilder.Length--;
		}
		output = stringBuilder.ToString();
		return true;
	}

	internal IAsyncResult BeginRead(HttpWebRequest request, byte[] buffer, int offset, int size, AsyncCallback cb, object state)
	{
		Stream stream = null;
		lock (this)
		{
			if (Data.request != request)
			{
				throw new ObjectDisposedException(typeof(NetworkStream).FullName);
			}
			if (nstream == null)
			{
				return null;
			}
			stream = nstream;
		}
		IAsyncResult asyncResult = null;
		if (!chunkedRead || (!chunkStream.DataAvailable && chunkStream.WantMore))
		{
			try
			{
				asyncResult = stream.BeginRead(buffer, offset, size, cb, state);
				cb = null;
			}
			catch (Exception)
			{
				HandleError(WebExceptionStatus.ReceiveFailure, null, "chunked BeginRead");
				throw;
			}
		}
		if (chunkedRead)
		{
			WebAsyncResult webAsyncResult = new WebAsyncResult(cb, state, buffer, offset, size);
			webAsyncResult.InnerAsyncResult = asyncResult;
			if (asyncResult == null)
			{
				webAsyncResult.SetCompleted(synch: true, (Exception)null);
				webAsyncResult.DoCallback();
			}
			return webAsyncResult;
		}
		return asyncResult;
	}

	internal int EndRead(HttpWebRequest request, IAsyncResult result)
	{
		Stream stream = null;
		lock (this)
		{
			if (request.Aborted)
			{
				throw new WebException("Request aborted", WebExceptionStatus.RequestCanceled);
			}
			if (Data.request != request)
			{
				throw new ObjectDisposedException(typeof(NetworkStream).FullName);
			}
			if (nstream == null)
			{
				throw new ObjectDisposedException(typeof(NetworkStream).FullName);
			}
			stream = nstream;
		}
		int read = 0;
		bool flag = false;
		WebAsyncResult webAsyncResult = null;
		IAsyncResult innerAsyncResult = ((WebAsyncResult)result).InnerAsyncResult;
		if (chunkedRead && innerAsyncResult is WebAsyncResult)
		{
			webAsyncResult = (WebAsyncResult)innerAsyncResult;
			IAsyncResult innerAsyncResult2 = webAsyncResult.InnerAsyncResult;
			if (innerAsyncResult2 != null && !(innerAsyncResult2 is WebAsyncResult))
			{
				read = stream.EndRead(innerAsyncResult2);
				flag = read == 0;
			}
		}
		else if (!(innerAsyncResult is WebAsyncResult))
		{
			read = stream.EndRead(innerAsyncResult);
			webAsyncResult = (WebAsyncResult)result;
			flag = read == 0;
		}
		if (chunkedRead)
		{
			try
			{
				chunkStream.WriteAndReadBack(webAsyncResult.Buffer, webAsyncResult.Offset, webAsyncResult.Size, ref read);
				if (!flag && read == 0 && chunkStream.WantMore)
				{
					read = EnsureRead(webAsyncResult.Buffer, webAsyncResult.Offset, webAsyncResult.Size);
				}
			}
			catch (Exception ex)
			{
				if (ex is WebException)
				{
					throw ex;
				}
				throw new WebException("Invalid chunked data.", ex, WebExceptionStatus.ServerProtocolViolation, null);
			}
			if ((flag || read == 0) && chunkStream.ChunkLeft != 0)
			{
				HandleError(WebExceptionStatus.ReceiveFailure, null, "chunked EndRead");
				throw new WebException("Read error", null, WebExceptionStatus.ReceiveFailure, null);
			}
		}
		if (read == 0)
		{
			return -1;
		}
		return read;
	}

	private int EnsureRead(byte[] buffer, int offset, int size)
	{
		byte[] array = null;
		int i;
		for (i = 0; i == 0; i += chunkStream.Read(buffer, offset + i, size - i))
		{
			if (!chunkStream.WantMore)
			{
				break;
			}
			int num = chunkStream.ChunkLeft;
			if (num <= 0)
			{
				num = 1024;
			}
			else if (num > 16384)
			{
				num = 16384;
			}
			if (array == null || array.Length < num)
			{
				array = new byte[num];
			}
			int num2 = nstream.Read(array, 0, num);
			if (num2 <= 0)
			{
				return 0;
			}
			chunkStream.Write(array, 0, num2);
		}
		return i;
	}

	private bool CompleteChunkedRead()
	{
		if (!chunkedRead || chunkStream == null)
		{
			return true;
		}
		while (chunkStream.WantMore)
		{
			int num = nstream.Read(buffer, 0, buffer.Length);
			if (num <= 0)
			{
				return false;
			}
			chunkStream.Write(buffer, 0, num);
		}
		return true;
	}

	internal IAsyncResult BeginWrite(HttpWebRequest request, byte[] buffer, int offset, int size, AsyncCallback cb, object state)
	{
		Stream stream = null;
		lock (this)
		{
			if (Data.request != request)
			{
				throw new ObjectDisposedException(typeof(NetworkStream).FullName);
			}
			if (nstream == null)
			{
				return null;
			}
			stream = nstream;
		}
		IAsyncResult asyncResult = null;
		try
		{
			return stream.BeginWrite(buffer, offset, size, cb, state);
		}
		catch (ObjectDisposedException)
		{
			lock (this)
			{
				if (Data.request != request)
				{
					return null;
				}
			}
			throw;
		}
		catch (IOException ex2)
		{
			if (ex2.InnerException is SocketException { SocketErrorCode: SocketError.NotConnected })
			{
				return null;
			}
			throw;
		}
		catch (Exception)
		{
			status = WebExceptionStatus.SendFailure;
			throw;
		}
	}

	internal bool EndWrite(HttpWebRequest request, bool throwOnError, IAsyncResult result)
	{
		Stream stream = null;
		lock (this)
		{
			if (status == WebExceptionStatus.RequestCanceled)
			{
				return true;
			}
			if (Data.request != request)
			{
				throw new ObjectDisposedException(typeof(NetworkStream).FullName);
			}
			if (nstream == null)
			{
				throw new ObjectDisposedException(typeof(NetworkStream).FullName);
			}
			stream = nstream;
		}
		try
		{
			stream.EndWrite(result);
			return true;
		}
		catch (Exception ex)
		{
			status = WebExceptionStatus.SendFailure;
			if (throwOnError && ex.InnerException != null)
			{
				throw ex.InnerException;
			}
			return false;
		}
	}

	internal int Read(HttpWebRequest request, byte[] buffer, int offset, int size)
	{
		Stream stream = null;
		lock (this)
		{
			if (Data.request != request)
			{
				throw new ObjectDisposedException(typeof(NetworkStream).FullName);
			}
			if (nstream == null)
			{
				return 0;
			}
			stream = nstream;
		}
		int read = 0;
		try
		{
			bool flag = false;
			if (!chunkedRead)
			{
				read = stream.Read(buffer, offset, size);
				flag = read == 0;
			}
			if (chunkedRead)
			{
				try
				{
					chunkStream.WriteAndReadBack(buffer, offset, size, ref read);
					if (!flag && read == 0 && chunkStream.WantMore)
					{
						read = EnsureRead(buffer, offset, size);
					}
				}
				catch (Exception e)
				{
					HandleError(WebExceptionStatus.ReceiveFailure, e, "chunked Read1");
					throw;
				}
				if ((flag || read == 0) && chunkStream.WantMore)
				{
					HandleError(WebExceptionStatus.ReceiveFailure, null, "chunked Read2");
					throw new WebException("Read error", null, WebExceptionStatus.ReceiveFailure, null);
				}
			}
		}
		catch (Exception e2)
		{
			HandleError(WebExceptionStatus.ReceiveFailure, e2, "Read");
		}
		return read;
	}

	internal bool Write(HttpWebRequest request, byte[] buffer, int offset, int size, ref string err_msg)
	{
		err_msg = null;
		Stream stream = null;
		lock (this)
		{
			if (Data.request != request)
			{
				throw new ObjectDisposedException(typeof(NetworkStream).FullName);
			}
			stream = nstream;
			if (stream == null)
			{
				return false;
			}
		}
		try
		{
			stream.Write(buffer, offset, size);
		}
		catch (Exception ex)
		{
			err_msg = ex.Message;
			WebExceptionStatus st = WebExceptionStatus.SendFailure;
			string where = "Write: " + err_msg;
			_ = ex is WebException;
			HandleError(st, ex, where);
			return false;
		}
		return true;
	}

	internal void Close(bool sendNext)
	{
		lock (this)
		{
			if (Data != null && Data.request != null && Data.request.ReuseConnection)
			{
				Data.request.ReuseConnection = false;
				return;
			}
			if (nstream != null)
			{
				try
				{
					nstream.Close();
				}
				catch
				{
				}
				nstream = null;
			}
			if (socket != null)
			{
				try
				{
					socket.Close();
				}
				catch
				{
				}
				socket = null;
			}
			if (ntlm_authenticated)
			{
				ResetNtlm();
			}
			if (Data != null)
			{
				lock (Data)
				{
					Data.ReadState = ReadState.Aborted;
				}
			}
			state.SetIdle();
			Data = new WebConnectionData();
			if (sendNext)
			{
				SendNext();
			}
			connect_request = null;
			connect_ntlm_auth_state = NtlmAuthState.None;
		}
	}

	private void Abort(object sender, EventArgs args)
	{
		lock (this)
		{
			lock (queue)
			{
				HttpWebRequest httpWebRequest = (HttpWebRequest)sender;
				if (Data.request == httpWebRequest || Data.request == null)
				{
					if (!httpWebRequest.FinishedReading)
					{
						status = WebExceptionStatus.RequestCanceled;
						Close(sendNext: false);
						if (queue.Count > 0)
						{
							Data.request = (HttpWebRequest)queue.Dequeue();
							SendRequest(Data.request);
						}
					}
					return;
				}
				httpWebRequest.FinishedReading = true;
				httpWebRequest.SetResponseError(WebExceptionStatus.RequestCanceled, null, "User aborted");
				if (queue.Count > 0 && queue.Peek() == sender)
				{
					queue.Dequeue();
				}
				else
				{
					if (queue.Count <= 0)
					{
						return;
					}
					object[] array = queue.ToArray();
					queue.Clear();
					for (int num = array.Length - 1; num >= 0; num--)
					{
						if (array[num] != sender)
						{
							queue.Enqueue(array[num]);
						}
					}
				}
			}
		}
	}

	internal void ResetNtlm()
	{
		ntlm_authenticated = false;
		ntlm_credentials = null;
		unsafe_sharing = false;
	}
}
