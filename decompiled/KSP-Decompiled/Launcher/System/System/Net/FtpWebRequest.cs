using System.Globalization;
using System.IO;
using System.Net.Sockets;
using System.Security.Authentication;
using System.Security.Cryptography.X509Certificates;
using System.Text;
using System.Threading;
using Mono.Net.Security;
using Mono.Security.Interface;
using Unity;

namespace System.Net;

public sealed class FtpWebRequest : WebRequest
{
	private enum RequestState
	{
		Before,
		Scheduled,
		Connecting,
		Authenticating,
		OpeningData,
		TransferInProgress,
		Finished,
		Aborted,
		Error
	}

	private Uri requestUri;

	private string file_name;

	private ServicePoint servicePoint;

	private Stream origDataStream;

	private Stream dataStream;

	private Stream controlStream;

	private StreamReader controlReader;

	private NetworkCredential credentials;

	private IPHostEntry hostEntry;

	private IPEndPoint localEndPoint;

	private IPEndPoint remoteEndPoint;

	private IWebProxy proxy;

	private int timeout;

	private int rwTimeout;

	private long offset;

	private bool binary;

	private bool enableSsl;

	private bool usePassive;

	private bool keepAlive;

	private string method;

	private string renameTo;

	private object locker;

	private RequestState requestState;

	private FtpAsyncResult asyncResult;

	private FtpWebResponse ftpResponse;

	private Stream requestStream;

	private string initial_path;

	private const string ChangeDir = "CWD";

	private const string UserCommand = "USER";

	private const string PasswordCommand = "PASS";

	private const string TypeCommand = "TYPE";

	private const string PassiveCommand = "PASV";

	private const string ExtendedPassiveCommand = "EPSV";

	private const string PortCommand = "PORT";

	private const string ExtendedPortCommand = "EPRT";

	private const string AbortCommand = "ABOR";

	private const string AuthCommand = "AUTH";

	private const string RestCommand = "REST";

	private const string RenameFromCommand = "RNFR";

	private const string RenameToCommand = "RNTO";

	private const string QuitCommand = "QUIT";

	private const string EOL = "\r\n";

	private static readonly string[] supportedCommands = new string[13]
	{
		"APPE", "DELE", "LIST", "MDTM", "MKD", "NLST", "PWD", "RENAME", "RETR", "RMD",
		"SIZE", "STOR", "STOU"
	};

	private Encoding dataEncoding;

	[System.MonoTODO]
	public X509CertificateCollection ClientCertificates
	{
		get
		{
			throw GetMustImplement();
		}
		set
		{
			throw GetMustImplement();
		}
	}

	[System.MonoTODO]
	public override string ConnectionGroupName
	{
		get
		{
			throw GetMustImplement();
		}
		set
		{
			throw GetMustImplement();
		}
	}

	public override string ContentType
	{
		get
		{
			throw new NotSupportedException();
		}
		set
		{
			throw new NotSupportedException();
		}
	}

	public override long ContentLength
	{
		get
		{
			return 0L;
		}
		set
		{
		}
	}

	public long ContentOffset
	{
		get
		{
			return offset;
		}
		set
		{
			CheckRequestStarted();
			if (value < 0)
			{
				throw new ArgumentOutOfRangeException();
			}
			offset = value;
		}
	}

	public override ICredentials Credentials
	{
		get
		{
			return credentials;
		}
		set
		{
			CheckRequestStarted();
			if (value == null)
			{
				throw new ArgumentNullException();
			}
			if (!(value is NetworkCredential))
			{
				throw new ArgumentException();
			}
			credentials = value as NetworkCredential;
		}
	}

	public bool EnableSsl
	{
		get
		{
			return enableSsl;
		}
		set
		{
			CheckRequestStarted();
			enableSsl = value;
		}
	}

	[System.MonoTODO]
	public override WebHeaderCollection Headers
	{
		get
		{
			throw GetMustImplement();
		}
		set
		{
			throw GetMustImplement();
		}
	}

	[System.MonoTODO("We don't support KeepAlive = true")]
	public bool KeepAlive
	{
		get
		{
			return keepAlive;
		}
		set
		{
			CheckRequestStarted();
		}
	}

	public override string Method
	{
		get
		{
			return method;
		}
		set
		{
			CheckRequestStarted();
			if (value == null)
			{
				throw new ArgumentNullException("Method string cannot be null");
			}
			if (value.Length == 0 || Array.BinarySearch(supportedCommands, value) < 0)
			{
				throw new ArgumentException("Method not supported", "value");
			}
			method = value;
		}
	}

	public override bool PreAuthenticate
	{
		get
		{
			throw new NotSupportedException();
		}
		set
		{
			throw new NotSupportedException();
		}
	}

	public override IWebProxy Proxy
	{
		get
		{
			return proxy;
		}
		set
		{
			CheckRequestStarted();
			proxy = value;
		}
	}

	public int ReadWriteTimeout
	{
		get
		{
			return rwTimeout;
		}
		set
		{
			CheckRequestStarted();
			if (value < -1)
			{
				throw new ArgumentOutOfRangeException();
			}
			rwTimeout = value;
		}
	}

	public string RenameTo
	{
		get
		{
			return renameTo;
		}
		set
		{
			CheckRequestStarted();
			if (value == null || value.Length == 0)
			{
				throw new ArgumentException("RenameTo value can't be null or empty", "RenameTo");
			}
			renameTo = value;
		}
	}

	public override Uri RequestUri => requestUri;

	public ServicePoint ServicePoint => GetServicePoint();

	public bool UsePassive
	{
		get
		{
			return usePassive;
		}
		set
		{
			CheckRequestStarted();
			usePassive = value;
		}
	}

	[System.MonoTODO]
	public override bool UseDefaultCredentials
	{
		get
		{
			throw GetMustImplement();
		}
		set
		{
			throw GetMustImplement();
		}
	}

	public bool UseBinary
	{
		get
		{
			return binary;
		}
		set
		{
			CheckRequestStarted();
			binary = value;
		}
	}

	public override int Timeout
	{
		get
		{
			return timeout;
		}
		set
		{
			CheckRequestStarted();
			if (value < -1)
			{
				throw new ArgumentOutOfRangeException();
			}
			timeout = value;
		}
	}

	private string DataType
	{
		get
		{
			if (!binary)
			{
				return "A";
			}
			return "I";
		}
	}

	private RequestState State
	{
		get
		{
			lock (locker)
			{
				return requestState;
			}
		}
		set
		{
			lock (locker)
			{
				CheckIfAborted();
				CheckFinalState();
				requestState = value;
			}
		}
	}

	internal FtpWebRequest(Uri uri)
	{
		timeout = 100000;
		rwTimeout = 300000;
		binary = true;
		usePassive = true;
		method = "RETR";
		locker = new object();
		dataEncoding = Encoding.UTF8;
		base._002Ector();
		requestUri = uri;
		proxy = GlobalProxySelection.Select;
	}

	private static Exception GetMustImplement()
	{
		return new NotImplementedException();
	}

	public override void Abort()
	{
		lock (locker)
		{
			if (State == RequestState.TransferInProgress)
			{
				SendCommand(false, "ABOR");
			}
			if (!InFinalState())
			{
				State = RequestState.Aborted;
				ftpResponse = new FtpWebResponse(this, requestUri, method, FtpStatusCode.FileActionAborted, "Aborted by request");
			}
		}
	}

	public override IAsyncResult BeginGetResponse(AsyncCallback callback, object state)
	{
		if (asyncResult != null && !asyncResult.IsCompleted)
		{
			throw new InvalidOperationException("Cannot re-call BeginGetRequestStream/BeginGetResponse while a previous call is still in progress");
		}
		CheckIfAborted();
		asyncResult = new FtpAsyncResult(callback, state);
		lock (locker)
		{
			if (InFinalState())
			{
				asyncResult.SetCompleted(synch: true, ftpResponse);
			}
			else
			{
				if (State == RequestState.Before)
				{
					State = RequestState.Scheduled;
				}
				Thread thread = new Thread(ProcessRequest);
				thread.IsBackground = true;
				thread.Start();
			}
		}
		return asyncResult;
	}

	public override WebResponse EndGetResponse(IAsyncResult asyncResult)
	{
		if (asyncResult == null)
		{
			throw new ArgumentNullException("AsyncResult cannot be null!");
		}
		if (!(asyncResult is FtpAsyncResult) || asyncResult != this.asyncResult)
		{
			throw new ArgumentException("AsyncResult is from another request!");
		}
		FtpAsyncResult ftpAsyncResult = (FtpAsyncResult)asyncResult;
		if (!ftpAsyncResult.WaitUntilComplete(timeout, exitContext: false))
		{
			Abort();
			throw new WebException("Transfer timed out.", WebExceptionStatus.Timeout);
		}
		CheckIfAborted();
		asyncResult = null;
		if (ftpAsyncResult.GotException)
		{
			throw ftpAsyncResult.Exception;
		}
		return ftpAsyncResult.Response;
	}

	public override WebResponse GetResponse()
	{
		IAsyncResult asyncResult = BeginGetResponse(null, null);
		return EndGetResponse(asyncResult);
	}

	public override IAsyncResult BeginGetRequestStream(AsyncCallback callback, object state)
	{
		if (method != "STOR" && method != "STOU" && method != "APPE")
		{
			throw new ProtocolViolationException();
		}
		lock (locker)
		{
			CheckIfAborted();
			if (State != 0)
			{
				throw new InvalidOperationException("Cannot re-call BeginGetRequestStream/BeginGetResponse while a previous call is still in progress");
			}
			State = RequestState.Scheduled;
		}
		asyncResult = new FtpAsyncResult(callback, state);
		Thread thread = new Thread(ProcessRequest);
		thread.IsBackground = true;
		thread.Start();
		return asyncResult;
	}

	public override Stream EndGetRequestStream(IAsyncResult asyncResult)
	{
		if (asyncResult == null)
		{
			throw new ArgumentNullException("asyncResult");
		}
		if (!(asyncResult is FtpAsyncResult))
		{
			throw new ArgumentException("asyncResult");
		}
		if (State == RequestState.Aborted)
		{
			throw new WebException("Request aborted", WebExceptionStatus.RequestCanceled);
		}
		if (asyncResult != this.asyncResult)
		{
			throw new ArgumentException("AsyncResult is from another request!");
		}
		FtpAsyncResult ftpAsyncResult = (FtpAsyncResult)asyncResult;
		if (!ftpAsyncResult.WaitUntilComplete(timeout, exitContext: false))
		{
			Abort();
			throw new WebException("Request timed out");
		}
		if (ftpAsyncResult.GotException)
		{
			throw ftpAsyncResult.Exception;
		}
		return ftpAsyncResult.Stream;
	}

	public override Stream GetRequestStream()
	{
		IAsyncResult asyncResult = BeginGetRequestStream(null, null);
		return EndGetRequestStream(asyncResult);
	}

	private ServicePoint GetServicePoint()
	{
		if (servicePoint == null)
		{
			servicePoint = ServicePointManager.FindServicePoint(requestUri, proxy);
		}
		return servicePoint;
	}

	private void ResolveHost()
	{
		CheckIfAborted();
		hostEntry = GetServicePoint().HostEntry;
		if (hostEntry == null)
		{
			ftpResponse.UpdateStatus(new FtpStatus(FtpStatusCode.ActionAbortedLocalProcessingError, "Cannot resolve server name"));
			throw new WebException("The remote server name could not be resolved: " + requestUri, null, WebExceptionStatus.NameResolutionFailure, ftpResponse);
		}
	}

	private void ProcessRequest()
	{
		if (State == RequestState.Scheduled)
		{
			ftpResponse = new FtpWebResponse(this, requestUri, method, keepAlive);
			try
			{
				ProcessMethod();
				asyncResult.SetCompleted(synch: false, ftpResponse);
				return;
			}
			catch (Exception completeWithError)
			{
				if (!GetServicePoint().UsesProxy)
				{
					State = RequestState.Error;
				}
				SetCompleteWithError(completeWithError);
				return;
			}
		}
		if (InProgress())
		{
			FtpStatus responseStatus = GetResponseStatus();
			ftpResponse.UpdateStatus(responseStatus);
			if (ftpResponse.IsFinal())
			{
				State = RequestState.Finished;
			}
		}
		asyncResult.SetCompleted(synch: false, ftpResponse);
	}

	private void SetType()
	{
		if (binary)
		{
			FtpStatus ftpStatus = SendCommand("TYPE", DataType);
			if (ftpStatus.StatusCode < FtpStatusCode.CommandOK || ftpStatus.StatusCode >= (FtpStatusCode)300)
			{
				throw CreateExceptionFromResponse(ftpStatus);
			}
		}
	}

	private string GetRemoteFolderPath(Uri uri)
	{
		string text = Uri.UnescapeDataString(uri.LocalPath);
		string text2;
		if (initial_path == null || initial_path == "/")
		{
			text2 = text;
		}
		else
		{
			if (text[0] == '/')
			{
				text = text.Substring(1);
			}
			text2 = new Uri(new UriBuilder
			{
				Scheme = "ftp",
				Host = "dummy-host",
				Path = initial_path
			}.Uri, text).LocalPath;
		}
		int num = text2.LastIndexOf('/');
		if (num == -1)
		{
			return null;
		}
		return text2.Substring(0, num + 1);
	}

	private void CWDAndSetFileName(Uri uri)
	{
		string remoteFolderPath = GetRemoteFolderPath(uri);
		if (remoteFolderPath != null)
		{
			FtpStatus ftpStatus = SendCommand("CWD", remoteFolderPath);
			if (ftpStatus.StatusCode < FtpStatusCode.CommandOK || ftpStatus.StatusCode >= (FtpStatusCode)300)
			{
				throw CreateExceptionFromResponse(ftpStatus);
			}
			int num = uri.LocalPath.LastIndexOf('/');
			if (num >= 0)
			{
				file_name = Uri.UnescapeDataString(uri.LocalPath.Substring(num + 1));
			}
		}
	}

	private void ProcessMethod()
	{
		if (GetServicePoint().UsesProxy)
		{
			if (method != "RETR")
			{
				throw new NotSupportedException("FTP+proxy only supports RETR");
			}
			HttpWebRequest obj = (HttpWebRequest)WebRequest.Create(proxy.GetProxy(requestUri));
			obj.Address = requestUri;
			requestState = RequestState.Finished;
			WebResponse response = obj.GetResponse();
			ftpResponse.Stream = new FtpDataStream(this, response.GetResponseStream(), isRead: true);
			ftpResponse.StatusCode = FtpStatusCode.CommandOK;
			return;
		}
		State = RequestState.Connecting;
		ResolveHost();
		OpenControlConnection();
		CWDAndSetFileName(requestUri);
		SetType();
		switch (method)
		{
		case "RETR":
		case "NLST":
		case "LIST":
			DownloadData();
			break;
		case "APPE":
		case "STOR":
		case "STOU":
			UploadData();
			break;
		case "SIZE":
		case "MDTM":
		case "PWD":
		case "MKD":
		case "RENAME":
		case "DELE":
			ProcessSimpleMethod();
			break;
		default:
			throw new Exception($"Support for command {method} not implemented yet");
		}
		CheckIfAborted();
	}

	private void CloseControlConnection()
	{
		if (controlStream != null)
		{
			SendCommand("QUIT");
			controlStream.Close();
			controlStream = null;
		}
	}

	internal void CloseDataConnection()
	{
		if (origDataStream != null)
		{
			origDataStream.Close();
			origDataStream = null;
		}
	}

	private void CloseConnection()
	{
		CloseControlConnection();
		CloseDataConnection();
	}

	private void ProcessSimpleMethod()
	{
		State = RequestState.TransferInProgress;
		if (method == "PWD")
		{
			method = "PWD";
		}
		if (method == "RENAME")
		{
			method = "RNFR";
		}
		FtpStatus ftpStatus = SendCommand(method, file_name);
		ftpResponse.Stream = Stream.Null;
		string statusDescription = ftpStatus.StatusDescription;
		switch (method)
		{
		case "SIZE":
		{
			if (ftpStatus.StatusCode != FtpStatusCode.FileStatus)
			{
				throw CreateExceptionFromResponse(ftpStatus);
			}
			int num = 4;
			int num2 = 0;
			while (num < statusDescription.Length && char.IsDigit(statusDescription[num]))
			{
				num++;
				num2++;
			}
			if (num2 == 0)
			{
				throw new WebException("Bad format for server response in " + method);
			}
			if (!long.TryParse(statusDescription.Substring(4, num2), out var result))
			{
				throw new WebException("Bad format for server response in " + method);
			}
			ftpResponse.contentLength = result;
			break;
		}
		case "MDTM":
			if (ftpStatus.StatusCode != FtpStatusCode.FileStatus)
			{
				throw CreateExceptionFromResponse(ftpStatus);
			}
			ftpResponse.LastModified = DateTime.ParseExact(statusDescription.Substring(4), "yyyyMMddHHmmss", null);
			break;
		case "MKD":
			if (ftpStatus.StatusCode != FtpStatusCode.PathnameCreated)
			{
				throw CreateExceptionFromResponse(ftpStatus);
			}
			break;
		case "CWD":
			method = "PWD";
			if (ftpStatus.StatusCode != FtpStatusCode.FileActionOK)
			{
				throw CreateExceptionFromResponse(ftpStatus);
			}
			ftpStatus = SendCommand(method);
			if (ftpStatus.StatusCode != FtpStatusCode.PathnameCreated)
			{
				throw CreateExceptionFromResponse(ftpStatus);
			}
			break;
		case "RNFR":
			method = "RENAME";
			if (ftpStatus.StatusCode != FtpStatusCode.FileCommandPending)
			{
				throw CreateExceptionFromResponse(ftpStatus);
			}
			ftpStatus = SendCommand("RNTO", (renameTo != null) ? renameTo : string.Empty);
			if (ftpStatus.StatusCode != FtpStatusCode.FileActionOK)
			{
				throw CreateExceptionFromResponse(ftpStatus);
			}
			break;
		case "DELE":
			if (ftpStatus.StatusCode != FtpStatusCode.FileActionOK)
			{
				throw CreateExceptionFromResponse(ftpStatus);
			}
			break;
		}
		State = RequestState.Finished;
	}

	private void UploadData()
	{
		State = RequestState.OpeningData;
		OpenDataConnection();
		State = RequestState.TransferInProgress;
		requestStream = new FtpDataStream(this, dataStream, isRead: false);
		asyncResult.Stream = requestStream;
	}

	private void DownloadData()
	{
		State = RequestState.OpeningData;
		OpenDataConnection();
		State = RequestState.TransferInProgress;
		ftpResponse.Stream = new FtpDataStream(this, dataStream, isRead: true);
	}

	private void CheckRequestStarted()
	{
		if (State != 0)
		{
			throw new InvalidOperationException("There is a request currently in progress");
		}
	}

	private void OpenControlConnection()
	{
		Exception innerException = null;
		Socket socket = null;
		IPAddress[] addressList = hostEntry.AddressList;
		foreach (IPAddress iPAddress in addressList)
		{
			socket = new Socket(iPAddress.AddressFamily, SocketType.Stream, ProtocolType.Tcp);
			remoteEndPoint = new IPEndPoint(iPAddress, requestUri.Port);
			if (!ServicePoint.CallEndPointDelegate(socket, remoteEndPoint))
			{
				socket.Close();
				socket = null;
				continue;
			}
			try
			{
				socket.Connect(remoteEndPoint);
				localEndPoint = (IPEndPoint)socket.LocalEndPoint;
			}
			catch (SocketException ex)
			{
				innerException = ex;
				socket.Close();
				socket = null;
				continue;
			}
			break;
		}
		if (socket == null)
		{
			throw new WebException("Unable to connect to remote server", innerException, WebExceptionStatus.UnknownError, ftpResponse);
		}
		controlStream = new NetworkStream(socket);
		controlReader = new StreamReader(controlStream, Encoding.ASCII);
		State = RequestState.Authenticating;
		Authenticate();
		FtpStatus ftpStatus = SendCommand("OPTS", "utf8", "on");
		if (ftpStatus.StatusCode < FtpStatusCode.CommandOK || ftpStatus.StatusCode > (FtpStatusCode)300)
		{
			dataEncoding = Encoding.Default;
		}
		else
		{
			dataEncoding = Encoding.UTF8;
		}
		ftpStatus = SendCommand("PWD");
		initial_path = GetInitialPath(ftpStatus);
	}

	private static string GetInitialPath(FtpStatus status)
	{
		int statusCode = (int)status.StatusCode;
		if (statusCode < 200 || statusCode > 300 || status.StatusDescription.Length <= 4)
		{
			throw new WebException("Error getting current directory: " + status.StatusDescription, null, WebExceptionStatus.UnknownError, null);
		}
		string text = status.StatusDescription.Substring(4);
		if (text[0] == '"')
		{
			int num = text.IndexOf('"', 1);
			if (num == -1)
			{
				throw new WebException("Error getting current directory: PWD -> " + status.StatusDescription, null, WebExceptionStatus.UnknownError, null);
			}
			text = text.Substring(1, num - 1);
		}
		if (!text.EndsWith("/"))
		{
			text += "/";
		}
		return text;
	}

	private Socket SetupPassiveConnection(string statusDescription, bool ipv6)
	{
		if (statusDescription.Length < 4)
		{
			throw new WebException("Cannot open passive data connection");
		}
		int num = (ipv6 ? GetPortV6(statusDescription) : GetPortV4(statusDescription));
		if (num < 0 || num > 65535)
		{
			throw new WebException("Cannot open passive data connection");
		}
		IPEndPoint iPEndPoint = new IPEndPoint(remoteEndPoint.Address, num);
		Socket socket = new Socket(iPEndPoint.AddressFamily, SocketType.Stream, ProtocolType.Tcp);
		try
		{
			socket.Connect(iPEndPoint);
			return socket;
		}
		catch (SocketException)
		{
			socket.Close();
			throw new WebException("Cannot open passive data connection");
		}
	}

	private int GetPortV4(string responseString)
	{
		string[] array = responseString.Split(' ', '(', ',', ')');
		if (array.Length <= 7)
		{
			throw new FormatException(global::SR.GetString("The response string '{0}' has invalid format.", responseString));
		}
		int num = array.Length - 1;
		if (array[num] == "" || !char.IsNumber(array[num], 0))
		{
			num--;
		}
		return Convert.ToByte(array[num--], NumberFormatInfo.InvariantInfo) | (Convert.ToByte(array[num--], NumberFormatInfo.InvariantInfo) << 8);
	}

	private int GetPortV6(string responseString)
	{
		int num = responseString.LastIndexOf("(");
		int num2 = responseString.LastIndexOf(")");
		if (num == -1 || num2 <= num)
		{
			throw new FormatException(global::SR.GetString("The response string '{0}' has invalid format.", responseString));
		}
		string[] array = responseString.Substring(num + 1, num2 - num - 1).Split('|');
		if (array.Length < 4)
		{
			throw new FormatException(global::SR.GetString("The response string '{0}' has invalid format.", responseString));
		}
		return Convert.ToInt32(array[3], NumberFormatInfo.InvariantInfo);
	}

	private string FormatAddress(IPAddress address, int Port)
	{
		byte[] addressBytes = address.GetAddressBytes();
		StringBuilder stringBuilder = new StringBuilder(32);
		byte[] array = addressBytes;
		foreach (byte value in array)
		{
			stringBuilder.Append(value);
			stringBuilder.Append(',');
		}
		stringBuilder.Append(Port / 256);
		stringBuilder.Append(',');
		stringBuilder.Append(Port % 256);
		return stringBuilder.ToString();
	}

	private string FormatAddressV6(IPAddress address, int port)
	{
		StringBuilder stringBuilder = new StringBuilder(43);
		string value = address.ToString();
		stringBuilder.Append("|2|");
		stringBuilder.Append(value);
		stringBuilder.Append('|');
		stringBuilder.Append(port.ToString(NumberFormatInfo.InvariantInfo));
		stringBuilder.Append('|');
		return stringBuilder.ToString();
	}

	private Exception CreateExceptionFromResponse(FtpStatus status)
	{
		FtpWebResponse response = new FtpWebResponse(this, requestUri, method, status);
		return new WebException("Server returned an error: " + status.StatusDescription, null, WebExceptionStatus.ProtocolError, response);
	}

	internal void SetTransferCompleted()
	{
		if (!InFinalState())
		{
			State = RequestState.Finished;
			FtpStatus responseStatus = GetResponseStatus();
			ftpResponse.UpdateStatus(responseStatus);
			if (!keepAlive)
			{
				CloseConnection();
			}
		}
	}

	internal void OperationCompleted()
	{
		if (!keepAlive)
		{
			CloseConnection();
		}
	}

	private void SetCompleteWithError(Exception exc)
	{
		if (asyncResult != null)
		{
			asyncResult.SetCompleted(synch: false, exc);
		}
	}

	private Socket InitDataConnection()
	{
		bool flag = remoteEndPoint.AddressFamily == AddressFamily.InterNetworkV6;
		FtpStatus ftpStatus;
		if (usePassive)
		{
			ftpStatus = SendCommand(flag ? "EPSV" : "PASV");
			if (ftpStatus.StatusCode != (FtpStatusCode)(flag ? 229 : 227))
			{
				throw CreateExceptionFromResponse(ftpStatus);
			}
			return SetupPassiveConnection(ftpStatus.StatusDescription, flag);
		}
		Socket socket = new Socket(remoteEndPoint.AddressFamily, SocketType.Stream, ProtocolType.Tcp);
		try
		{
			socket.Bind(new IPEndPoint(localEndPoint.Address, 0));
			socket.Listen(1);
		}
		catch (SocketException innerException)
		{
			socket.Close();
			throw new WebException("Couldn't open listening socket on client", innerException);
		}
		IPEndPoint iPEndPoint = (IPEndPoint)socket.LocalEndPoint;
		string text = (flag ? FormatAddressV6(iPEndPoint.Address, iPEndPoint.Port) : FormatAddress(iPEndPoint.Address, iPEndPoint.Port));
		ftpStatus = SendCommand(flag ? "EPRT" : "PORT", text);
		if (ftpStatus.StatusCode != FtpStatusCode.CommandOK)
		{
			socket.Close();
			throw CreateExceptionFromResponse(ftpStatus);
		}
		return socket;
	}

	private void OpenDataConnection()
	{
		Socket socket = InitDataConnection();
		FtpStatus ftpStatus;
		if (offset > 0)
		{
			ftpStatus = SendCommand("REST", offset.ToString());
			if (ftpStatus.StatusCode != FtpStatusCode.FileCommandPending)
			{
				throw CreateExceptionFromResponse(ftpStatus);
			}
		}
		ftpStatus = ((!(method != "NLST") || !(method != "LIST") || !(method != "STOU")) ? SendCommand(method) : SendCommand(method, file_name));
		if (ftpStatus.StatusCode != FtpStatusCode.OpeningData && ftpStatus.StatusCode != FtpStatusCode.DataAlreadyOpen)
		{
			throw CreateExceptionFromResponse(ftpStatus);
		}
		if (usePassive)
		{
			origDataStream = new NetworkStream(socket, ownsSocket: true);
			dataStream = origDataStream;
			if (EnableSsl)
			{
				ChangeToSSLSocket(ref dataStream);
			}
		}
		else
		{
			Socket socket2 = null;
			try
			{
				socket2 = socket.Accept();
			}
			catch (SocketException)
			{
				socket.Close();
				socket2?.Close();
				throw new ProtocolViolationException("Server commited a protocol violation.");
			}
			socket.Close();
			origDataStream = new NetworkStream(socket2, ownsSocket: true);
			dataStream = origDataStream;
			if (EnableSsl)
			{
				ChangeToSSLSocket(ref dataStream);
			}
		}
		ftpResponse.UpdateStatus(ftpStatus);
	}

	private void Authenticate()
	{
		string text = null;
		string text2 = null;
		string text3 = null;
		if (credentials != null)
		{
			text = credentials.UserName;
			text2 = credentials.Password;
			text3 = credentials.Domain;
		}
		if (text == null)
		{
			text = "anonymous";
		}
		if (text2 == null)
		{
			text2 = "@anonymous";
		}
		if (!string.IsNullOrEmpty(text3))
		{
			text = text3 + "\\" + text;
		}
		FtpStatus ftpStatus = GetResponseStatus();
		ftpResponse.BannerMessage = ftpStatus.StatusDescription;
		if (EnableSsl)
		{
			InitiateSecureConnection(ref controlStream);
			controlReader = new StreamReader(controlStream, Encoding.ASCII);
			ftpStatus = SendCommand("PBSZ", "0");
			int statusCode = (int)ftpStatus.StatusCode;
			if (statusCode < 200 || statusCode >= 300)
			{
				throw CreateExceptionFromResponse(ftpStatus);
			}
			ftpStatus = SendCommand("PROT", "P");
			statusCode = (int)ftpStatus.StatusCode;
			if (statusCode < 200 || statusCode >= 300)
			{
				throw CreateExceptionFromResponse(ftpStatus);
			}
			ftpStatus = new FtpStatus(FtpStatusCode.SendUserCommand, "");
		}
		if (ftpStatus.StatusCode != FtpStatusCode.SendUserCommand)
		{
			throw CreateExceptionFromResponse(ftpStatus);
		}
		ftpStatus = SendCommand("USER", text);
		switch (ftpStatus.StatusCode)
		{
		case FtpStatusCode.SendPasswordCommand:
			ftpStatus = SendCommand("PASS", text2);
			if (ftpStatus.StatusCode != FtpStatusCode.LoggedInProceed)
			{
				throw CreateExceptionFromResponse(ftpStatus);
			}
			break;
		default:
			throw CreateExceptionFromResponse(ftpStatus);
		case FtpStatusCode.LoggedInProceed:
			break;
		}
		ftpResponse.WelcomeMessage = ftpStatus.StatusDescription;
		ftpResponse.UpdateStatus(ftpStatus);
	}

	private FtpStatus SendCommand(string command, params string[] parameters)
	{
		return SendCommand(waitResponse: true, command, parameters);
	}

	private FtpStatus SendCommand(bool waitResponse, string command, params string[] parameters)
	{
		string text = command;
		if (parameters.Length != 0)
		{
			text = text + " " + string.Join(" ", parameters);
		}
		text += "\r\n";
		byte[] bytes = dataEncoding.GetBytes(text);
		try
		{
			controlStream.Write(bytes, 0, bytes.Length);
		}
		catch (IOException)
		{
			return new FtpStatus(FtpStatusCode.ServiceNotAvailable, "Write failed");
		}
		if (!waitResponse)
		{
			return null;
		}
		FtpStatus responseStatus = GetResponseStatus();
		if (ftpResponse != null)
		{
			ftpResponse.UpdateStatus(responseStatus);
		}
		return responseStatus;
	}

	internal static FtpStatus ServiceNotAvailable()
	{
		return new FtpStatus(FtpStatusCode.ServiceNotAvailable, Locale.GetText("Invalid response from server"));
	}

	internal FtpStatus GetResponseStatus()
	{
		string text = null;
		try
		{
			text = controlReader.ReadLine();
		}
		catch (IOException)
		{
		}
		if (text == null || text.Length < 3)
		{
			return ServiceNotAvailable();
		}
		if (!int.TryParse(text.Substring(0, 3), out var result))
		{
			return ServiceNotAvailable();
		}
		if (text.Length > 3 && text[3] == '-')
		{
			string text2 = null;
			string value = result + " ";
			do
			{
				text2 = null;
				try
				{
					text2 = controlReader.ReadLine();
				}
				catch (IOException)
				{
				}
				if (text2 == null)
				{
					return ServiceNotAvailable();
				}
				text = text + Environment.NewLine + text2;
			}
			while (!text2.StartsWith(value, StringComparison.Ordinal));
		}
		return new FtpStatus((FtpStatusCode)result, text);
	}

	private void InitiateSecureConnection(ref Stream stream)
	{
		FtpStatus ftpStatus = SendCommand("AUTH", "TLS");
		if (ftpStatus.StatusCode != FtpStatusCode.ServerWantsSecureSession)
		{
			throw CreateExceptionFromResponse(ftpStatus);
		}
		ChangeToSSLSocket(ref stream);
	}

	internal bool ChangeToSSLSocket(ref Stream stream)
	{
		MonoTlsProvider providerInternal = Mono.Net.Security.MonoTlsProviderFactory.GetProviderInternal();
		MonoTlsSettings monoTlsSettings = MonoTlsSettings.CopyDefaultSettings();
		monoTlsSettings.UseServicePointManagerCallback = true;
		IMonoSslStream monoSslStream = providerInternal.CreateSslStream(stream, leaveInnerStreamOpen: true, monoTlsSettings);
		monoSslStream.AuthenticateAsClient(requestUri.Host, null, SslProtocols.Default, checkCertificateRevocation: false);
		stream = monoSslStream.AuthenticatedStream;
		return true;
	}

	private bool InFinalState()
	{
		if (State != RequestState.Aborted && State != RequestState.Error)
		{
			return State == RequestState.Finished;
		}
		return true;
	}

	private bool InProgress()
	{
		if (State != 0)
		{
			return !InFinalState();
		}
		return false;
	}

	internal void CheckIfAborted()
	{
		if (State == RequestState.Aborted)
		{
			throw new WebException("Request aborted", WebExceptionStatus.RequestCanceled);
		}
	}

	private void CheckFinalState()
	{
		if (InFinalState())
		{
			throw new InvalidOperationException("Cannot change final state");
		}
	}

	internal FtpWebRequest()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
