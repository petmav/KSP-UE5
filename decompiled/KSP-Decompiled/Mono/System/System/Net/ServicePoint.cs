using System.Collections.Generic;
using System.Net.Sockets;
using System.Security.Cryptography.X509Certificates;
using System.Threading;
using Unity;

namespace System.Net;

public class ServicePoint
{
	private readonly Uri uri;

	private int connectionLimit;

	private int maxIdleTime;

	private int currentConnections;

	private DateTime idleSince;

	private DateTime lastDnsResolve;

	private Version protocolVersion;

	private IPHostEntry host;

	private bool usesProxy;

	private Dictionary<string, WebConnectionGroup> groups;

	private bool sendContinue;

	private bool useConnect;

	private object hostE;

	private bool useNagle;

	private BindIPEndPoint endPointCallback;

	private bool tcp_keepalive;

	private int tcp_keepalive_time;

	private int tcp_keepalive_interval;

	private Timer idleTimer;

	private object m_ServerCertificateOrBytes;

	private object m_ClientCertificateOrBytes;

	public Uri Address => uri;

	public BindIPEndPoint BindIPEndPointDelegate
	{
		get
		{
			return endPointCallback;
		}
		set
		{
			endPointCallback = value;
		}
	}

	[System.MonoTODO]
	public int ConnectionLeaseTimeout
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

	public int ConnectionLimit
	{
		get
		{
			return connectionLimit;
		}
		set
		{
			if (value <= 0)
			{
				throw new ArgumentOutOfRangeException();
			}
			connectionLimit = value;
		}
	}

	public string ConnectionName => uri.Scheme;

	public int CurrentConnections => currentConnections;

	public DateTime IdleSince => idleSince.ToLocalTime();

	public int MaxIdleTime
	{
		get
		{
			return maxIdleTime;
		}
		set
		{
			if (value < -1 || value > int.MaxValue)
			{
				throw new ArgumentOutOfRangeException();
			}
			lock (this)
			{
				maxIdleTime = value;
				if (idleTimer != null)
				{
					idleTimer.Change(maxIdleTime, maxIdleTime);
				}
			}
		}
	}

	public virtual Version ProtocolVersion => protocolVersion;

	[System.MonoTODO]
	public int ReceiveBufferSize
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

	public bool SupportsPipelining => HttpVersion.Version11.Equals(protocolVersion);

	public bool Expect100Continue
	{
		get
		{
			return SendContinue;
		}
		set
		{
			SendContinue = value;
		}
	}

	public bool UseNagleAlgorithm
	{
		get
		{
			return useNagle;
		}
		set
		{
			useNagle = value;
		}
	}

	internal bool SendContinue
	{
		get
		{
			if (sendContinue)
			{
				if (!(protocolVersion == null))
				{
					return protocolVersion == HttpVersion.Version11;
				}
				return true;
			}
			return false;
		}
		set
		{
			sendContinue = value;
		}
	}

	internal bool UsesProxy
	{
		get
		{
			return usesProxy;
		}
		set
		{
			usesProxy = value;
		}
	}

	internal bool UseConnect
	{
		get
		{
			return useConnect;
		}
		set
		{
			useConnect = value;
		}
	}

	private bool HasTimedOut
	{
		get
		{
			int dnsRefreshTimeout = ServicePointManager.DnsRefreshTimeout;
			if (dnsRefreshTimeout != -1)
			{
				return lastDnsResolve + TimeSpan.FromMilliseconds(dnsRefreshTimeout) < DateTime.UtcNow;
			}
			return false;
		}
	}

	internal IPHostEntry HostEntry
	{
		get
		{
			lock (hostE)
			{
				string text = uri.Host;
				if (uri.HostNameType == UriHostNameType.IPv6 || uri.HostNameType == UriHostNameType.IPv4)
				{
					if (host != null)
					{
						return host;
					}
					if (uri.HostNameType == UriHostNameType.IPv6)
					{
						text = text.Substring(1, text.Length - 2);
					}
					host = new IPHostEntry();
					host.AddressList = new IPAddress[1] { IPAddress.Parse(text) };
					return host;
				}
				if (!HasTimedOut && host != null)
				{
					return host;
				}
				lastDnsResolve = DateTime.UtcNow;
				try
				{
					host = Dns.GetHostEntry(text);
				}
				catch
				{
					return null;
				}
			}
			return host;
		}
	}

	public X509Certificate Certificate
	{
		get
		{
			object serverCertificateOrBytes = m_ServerCertificateOrBytes;
			if (serverCertificateOrBytes != null && serverCertificateOrBytes.GetType() == typeof(byte[]))
			{
				return (X509Certificate)(m_ServerCertificateOrBytes = new X509Certificate((byte[])serverCertificateOrBytes));
			}
			return serverCertificateOrBytes as X509Certificate;
		}
	}

	public X509Certificate ClientCertificate
	{
		get
		{
			object clientCertificateOrBytes = m_ClientCertificateOrBytes;
			if (clientCertificateOrBytes != null && clientCertificateOrBytes.GetType() == typeof(byte[]))
			{
				return (X509Certificate)(m_ClientCertificateOrBytes = new X509Certificate((byte[])clientCertificateOrBytes));
			}
			return clientCertificateOrBytes as X509Certificate;
		}
	}

	internal ServicePoint(Uri uri, int connectionLimit, int maxIdleTime)
	{
		sendContinue = true;
		hostE = new object();
		base._002Ector();
		this.uri = uri;
		this.connectionLimit = connectionLimit;
		this.maxIdleTime = maxIdleTime;
		currentConnections = 0;
		idleSince = DateTime.UtcNow;
	}

	private static Exception GetMustImplement()
	{
		return new NotImplementedException();
	}

	public void SetTcpKeepAlive(bool enabled, int keepAliveTime, int keepAliveInterval)
	{
		if (enabled)
		{
			if (keepAliveTime <= 0)
			{
				throw new ArgumentOutOfRangeException("keepAliveTime", "Must be greater than 0");
			}
			if (keepAliveInterval <= 0)
			{
				throw new ArgumentOutOfRangeException("keepAliveInterval", "Must be greater than 0");
			}
		}
		tcp_keepalive = enabled;
		tcp_keepalive_time = keepAliveTime;
		tcp_keepalive_interval = keepAliveInterval;
	}

	internal void KeepAliveSetup(Socket socket)
	{
		if (tcp_keepalive)
		{
			byte[] array = new byte[12];
			PutBytes(array, tcp_keepalive ? 1u : 0u, 0);
			PutBytes(array, (uint)tcp_keepalive_time, 4);
			PutBytes(array, (uint)tcp_keepalive_interval, 8);
			socket.IOControl(IOControlCode.KeepAliveValues, array, null);
		}
	}

	private static void PutBytes(byte[] bytes, uint v, int offset)
	{
		if (BitConverter.IsLittleEndian)
		{
			bytes[offset] = (byte)(v & 0xFFu);
			bytes[offset + 1] = (byte)((v & 0xFF00) >> 8);
			bytes[offset + 2] = (byte)((v & 0xFF0000) >> 16);
			bytes[offset + 3] = (byte)((v & 0xFF000000u) >> 24);
		}
		else
		{
			bytes[offset + 3] = (byte)(v & 0xFFu);
			bytes[offset + 2] = (byte)((v & 0xFF00) >> 8);
			bytes[offset + 1] = (byte)((v & 0xFF0000) >> 16);
			bytes[offset] = (byte)((v & 0xFF000000u) >> 24);
		}
	}

	private WebConnectionGroup GetConnectionGroup(string name)
	{
		if (name == null)
		{
			name = "";
		}
		if (groups != null && groups.TryGetValue(name, out var value))
		{
			return value;
		}
		value = new WebConnectionGroup(this, name);
		value.ConnectionClosed += delegate
		{
			currentConnections--;
		};
		if (groups == null)
		{
			groups = new Dictionary<string, WebConnectionGroup>();
		}
		groups.Add(name, value);
		return value;
	}

	private void RemoveConnectionGroup(WebConnectionGroup group)
	{
		if (groups == null || groups.Count == 0)
		{
			throw new InvalidOperationException();
		}
		groups.Remove(group.Name);
	}

	private bool CheckAvailableForRecycling(out DateTime outIdleSince)
	{
		outIdleSince = DateTime.MinValue;
		List<WebConnectionGroup> list = null;
		List<WebConnectionGroup> list2 = null;
		TimeSpan timeSpan;
		lock (this)
		{
			if (groups == null || groups.Count == 0)
			{
				idleSince = DateTime.MinValue;
				return true;
			}
			timeSpan = TimeSpan.FromMilliseconds(maxIdleTime);
			list = new List<WebConnectionGroup>(groups.Values);
		}
		foreach (WebConnectionGroup item in list)
		{
			if (item.TryRecycle(timeSpan, ref outIdleSince))
			{
				if (list2 == null)
				{
					list2 = new List<WebConnectionGroup>();
				}
				list2.Add(item);
			}
		}
		lock (this)
		{
			idleSince = outIdleSince;
			if (list2 != null && groups != null)
			{
				foreach (WebConnectionGroup item2 in list2)
				{
					if (groups.ContainsKey(item2.Name))
					{
						RemoveConnectionGroup(item2);
					}
				}
			}
			if (groups != null && groups.Count == 0)
			{
				groups = null;
			}
			if (groups == null)
			{
				if (idleTimer != null)
				{
					idleTimer.Dispose();
					idleTimer = null;
				}
				return true;
			}
			return false;
		}
	}

	private void IdleTimerCallback(object obj)
	{
		CheckAvailableForRecycling(out var _);
	}

	internal void SetVersion(Version version)
	{
		protocolVersion = version;
	}

	internal EventHandler SendRequest(HttpWebRequest request, string groupName)
	{
		WebConnection connection;
		lock (this)
		{
			connection = GetConnectionGroup(groupName).GetConnection(request, out var created);
			if (created)
			{
				currentConnections++;
				if (idleTimer == null)
				{
					idleTimer = new Timer(IdleTimerCallback, null, maxIdleTime, maxIdleTime);
				}
			}
		}
		return connection.SendRequest(request);
	}

	public bool CloseConnectionGroup(string connectionGroupName)
	{
		WebConnectionGroup webConnectionGroup = null;
		lock (this)
		{
			webConnectionGroup = GetConnectionGroup(connectionGroupName);
			if (webConnectionGroup != null)
			{
				RemoveConnectionGroup(webConnectionGroup);
			}
		}
		if (webConnectionGroup != null)
		{
			webConnectionGroup.Close();
			return true;
		}
		return false;
	}

	internal void UpdateServerCertificate(X509Certificate certificate)
	{
		if (certificate != null)
		{
			m_ServerCertificateOrBytes = certificate.GetRawCertData();
		}
		else
		{
			m_ServerCertificateOrBytes = null;
		}
	}

	internal void UpdateClientCertificate(X509Certificate certificate)
	{
		if (certificate != null)
		{
			m_ClientCertificateOrBytes = certificate.GetRawCertData();
		}
		else
		{
			m_ClientCertificateOrBytes = null;
		}
	}

	internal bool CallEndPointDelegate(Socket sock, IPEndPoint remote)
	{
		if (endPointCallback == null)
		{
			return true;
		}
		int num = 0;
		while (true)
		{
			IPEndPoint iPEndPoint = null;
			try
			{
				iPEndPoint = endPointCallback(this, remote, num);
			}
			catch
			{
				return false;
			}
			if (iPEndPoint == null)
			{
				return true;
			}
			try
			{
				sock.Bind(iPEndPoint);
			}
			catch (SocketException)
			{
				num = checked(num + 1);
				continue;
			}
			break;
		}
		return true;
	}

	internal Socket GetConnection(PooledStream PooledStream, object owner, bool async, out IPAddress address, ref Socket abortSocket, ref Socket abortSocket6)
	{
		throw new NotImplementedException();
	}

	internal ServicePoint()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
