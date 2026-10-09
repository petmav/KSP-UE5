using System;
using System.Collections.Generic;
using System.IO;
using System.Net;
using System.Runtime.InteropServices;
using System.Security.Authentication;
using System.Security.Cryptography.X509Certificates;
using System.Text;
using System.Threading;
using Mono.Net;
using Mono.Net.Security;
using Mono.Security.Interface;
using Mono.Util;

namespace Mono.AppleTls;

internal class AppleTlsContext : MobileTlsContext
{
	public const string SecurityLibrary = "/System/Library/Frameworks/Security.framework/Security";

	private GCHandle handle;

	private IntPtr context;

	private SslReadFunc readFunc;

	private SslWriteFunc writeFunc;

	private SecIdentity serverIdentity;

	private SecIdentity clientIdentity;

	private X509Certificate remoteCertificate;

	private X509Certificate localClientCertificate;

	private MonoTlsConnectionInfo connectionInfo;

	private bool havePeerTrust;

	private bool isAuthenticated;

	private bool handshakeFinished;

	private int handshakeStarted;

	private bool closed;

	private bool disposed;

	private bool closedGraceful;

	private int pendingIO;

	private Exception lastException;

	public IntPtr Handle
	{
		get
		{
			if (!HasContext)
			{
				throw new ObjectDisposedException("AppleTlsContext");
			}
			return context;
		}
	}

	public override bool HasContext
	{
		get
		{
			if (!disposed)
			{
				return context != IntPtr.Zero;
			}
			return false;
		}
	}

	public override bool IsAuthenticated => isAuthenticated;

	public override MonoTlsConnectionInfo ConnectionInfo => connectionInfo;

	internal override bool IsRemoteCertificateAvailable => remoteCertificate != null;

	internal override X509Certificate LocalClientCertificate => localClientCertificate;

	public override X509Certificate RemoteCertificate => remoteCertificate;

	public override TlsProtocols NegotiatedProtocol => connectionInfo.ProtocolVersion;

	public SslProtocol MaxProtocol
	{
		get
		{
			SslProtocol maxVersion;
			SslStatus status = SSLGetProtocolVersionMax(Handle, out maxVersion);
			CheckStatusAndThrow(status);
			return maxVersion;
		}
		set
		{
			SslStatus status = SSLSetProtocolVersionMax(Handle, value);
			CheckStatusAndThrow(status);
		}
	}

	public SslProtocol MinProtocol
	{
		get
		{
			SslProtocol minVersion;
			SslStatus status = SSLGetProtocolVersionMin(Handle, out minVersion);
			CheckStatusAndThrow(status);
			return minVersion;
		}
		set
		{
			SslStatus status = SSLSetProtocolVersionMin(Handle, value);
			CheckStatusAndThrow(status);
		}
	}

	public SslSessionState SessionState
	{
		get
		{
			SslSessionState state = SslSessionState.Invalid;
			SslStatus status = SSLGetSessionState(Handle, ref state);
			CheckStatusAndThrow(status);
			return state;
		}
	}

	public unsafe byte[] PeerId
	{
		get
		{
			IntPtr peerID;
			IntPtr peerIDLen;
			SslStatus sslStatus = SSLGetPeerID(Handle, out peerID, out peerIDLen);
			CheckStatusAndThrow(sslStatus);
			if (sslStatus != 0 || (int)peerIDLen == 0)
			{
				return null;
			}
			byte[] array = new byte[(int)peerIDLen];
			Marshal.Copy(peerID, array, 0, (int)peerIDLen);
			return array;
		}
		set
		{
			IntPtr peerIDLen = ((value == null) ? IntPtr.Zero : ((IntPtr)value.Length));
			SslStatus status;
			fixed (byte* peerID = value)
			{
				status = SSLSetPeerID(Handle, peerID, peerIDLen);
			}
			CheckStatusAndThrow(status);
		}
	}

	public IntPtr BufferedReadSize
	{
		get
		{
			IntPtr bufSize;
			SslStatus status = SSLGetBufferedReadSize(Handle, out bufSize);
			CheckStatusAndThrow(status);
			return bufSize;
		}
	}

	public SslCipherSuite NegotiatedCipher
	{
		get
		{
			SslCipherSuite cipherSuite;
			SslStatus status = SSLGetNegotiatedCipher(Handle, out cipherSuite);
			CheckStatusAndThrow(status);
			return cipherSuite;
		}
	}

	public string PeerDomainName
	{
		get
		{
			SslStatus sslStatus = SSLGetPeerDomainNameLength(Handle, out var peerNameLen);
			CheckStatusAndThrow(sslStatus);
			if (sslStatus != 0 || (int)peerNameLen == 0)
			{
				return string.Empty;
			}
			byte[] array = new byte[(int)peerNameLen];
			sslStatus = SSLGetPeerDomainName(Handle, array, ref peerNameLen);
			CheckStatusAndThrow(sslStatus);
			int num = (int)peerNameLen;
			if (sslStatus != 0)
			{
				return string.Empty;
			}
			if (num > 0 && array[num - 1] == 0)
			{
				num--;
			}
			return Encoding.UTF8.GetString(array, 0, num);
		}
		set
		{
			SslStatus status;
			if (value == null)
			{
				status = SSLSetPeerDomainName(Handle, null, (IntPtr)0);
			}
			else
			{
				byte[] bytes = Encoding.UTF8.GetBytes(value);
				status = SSLSetPeerDomainName(Handle, bytes, (IntPtr)bytes.Length);
			}
			CheckStatusAndThrow(status);
		}
	}

	public SslClientCertificateState ClientCertificateState
	{
		get
		{
			SslClientCertificateState clientState;
			SslStatus status = SSLGetClientCertificateState(Handle, out clientState);
			CheckStatusAndThrow(status);
			return clientState;
		}
	}

	public AppleTlsContext(MobileAuthenticatedStream parent, bool serverMode, string targetHost, SslProtocols enabledProtocols, X509Certificate serverCertificate, X509CertificateCollection clientCertificates, bool askForClientCert)
		: base(parent, serverMode, targetHost, enabledProtocols, serverCertificate, clientCertificates, askForClientCert)
	{
		handle = GCHandle.Alloc(this, GCHandleType.Weak);
		readFunc = NativeReadCallback;
		writeFunc = NativeWriteCallback;
		if (base.IsServer && serverCertificate == null)
		{
			throw new ArgumentNullException("serverCertificate");
		}
	}

	private void CheckStatusAndThrow(SslStatus status, params SslStatus[] acceptable)
	{
		Exception ex = Interlocked.Exchange(ref lastException, null);
		if (ex != null)
		{
			throw ex;
		}
		if (status == SslStatus.Success || Array.IndexOf(acceptable, status) > -1)
		{
			return;
		}
		switch (status)
		{
		case SslStatus.ClosedAbort:
			throw new IOException("Connection closed.");
		case SslStatus.BadCert:
			throw new TlsException(AlertDescription.BadCertificate);
		case SslStatus.NoRootCert:
		case SslStatus.UnknownRootCert:
		case SslStatus.XCertChainInvalid:
			throw new TlsException(AlertDescription.CertificateUnknown, status.ToString());
		case SslStatus.CertNotYetValid:
		case SslStatus.CertExpired:
			throw new TlsException(AlertDescription.CertificateExpired);
		case SslStatus.Protocol:
			throw new TlsException(AlertDescription.ProtocolVersion);
		default:
			throw new TlsException(AlertDescription.InternalError, "Unknown Secure Transport error `{0}'.", status);
		}
	}

	public override void StartHandshake()
	{
		if (Interlocked.CompareExchange(ref handshakeStarted, 1, 1) != 0)
		{
			throw new InvalidOperationException();
		}
		InitializeConnection();
		SetSessionOption(SslSessionOption.BreakOnCertRequested, value: true);
		SetSessionOption(SslSessionOption.BreakOnClientAuth, value: true);
		SetSessionOption(SslSessionOption.BreakOnServerAuth, value: true);
		if (base.IsServer)
		{
			serverIdentity = AppleCertificateHelper.GetIdentity(base.LocalServerCertificate, out var intermediateCerts);
			if (serverIdentity == null)
			{
				throw new AuthenticationException("Unable to get server certificate from keychain.");
			}
			SetCertificate(serverIdentity, intermediateCerts);
			for (int i = 0; i < intermediateCerts.Length; i++)
			{
				intermediateCerts[i].Dispose();
			}
		}
	}

	public override void FinishHandshake()
	{
		InitializeSession();
		isAuthenticated = true;
	}

	public override void Flush()
	{
	}

	public override bool ProcessHandshake()
	{
		if (handshakeFinished)
		{
			throw new NotSupportedException("Handshake already finished.");
		}
		while (true)
		{
			lastException = null;
			SslStatus sslStatus = SSLHandshake(Handle);
			CheckStatusAndThrow(sslStatus, SslStatus.WouldBlock, SslStatus.PeerAuthCompleted, SslStatus.PeerClientCertRequested);
			switch (sslStatus)
			{
			case SslStatus.PeerAuthCompleted:
				RequirePeerTrust();
				break;
			case SslStatus.PeerClientCertRequested:
				RequirePeerTrust();
				if (remoteCertificate == null)
				{
					throw new TlsException(AlertDescription.InternalError, "Cannot request client certificate before receiving one from the server.");
				}
				localClientCertificate = SelectClientCertificate(remoteCertificate, null);
				if (localClientCertificate != null)
				{
					clientIdentity = AppleCertificateHelper.GetIdentity(localClientCertificate);
					if (clientIdentity == null)
					{
						throw new TlsException(AlertDescription.CertificateUnknown);
					}
					SetCertificate(clientIdentity, new SecCertificate[0]);
				}
				break;
			case SslStatus.WouldBlock:
				return false;
			case SslStatus.Success:
				handshakeFinished = true;
				return true;
			}
		}
	}

	private void RequirePeerTrust()
	{
		if (!havePeerTrust)
		{
			EvaluateTrust();
			havePeerTrust = true;
		}
	}

	private void EvaluateTrust()
	{
		InitializeSession();
		SecTrust secTrust = null;
		X509CertificateCollection x509CertificateCollection = null;
		bool flag;
		try
		{
			secTrust = GetPeerTrust(!base.IsServer);
			if (secTrust == null || secTrust.Count == 0)
			{
				remoteCertificate = null;
				if (!base.IsServer)
				{
					throw new TlsException(AlertDescription.CertificateUnknown);
				}
				x509CertificateCollection = null;
			}
			else
			{
				_ = secTrust.Count;
				_ = 1;
				x509CertificateCollection = new X509CertificateCollection();
				for (int i = 0; i < secTrust.Count; i++)
				{
					x509CertificateCollection.Add(secTrust.GetCertificate(i));
				}
				remoteCertificate = new X509Certificate(x509CertificateCollection[0]);
			}
			flag = ValidateCertificate(x509CertificateCollection);
		}
		catch (Exception)
		{
			throw new TlsException(AlertDescription.CertificateUnknown, "Certificate validation threw exception.");
		}
		finally
		{
			secTrust?.Dispose();
			if (x509CertificateCollection != null)
			{
				for (int j = 0; j < x509CertificateCollection.Count; j++)
				{
					x509CertificateCollection[j].Dispose();
				}
			}
		}
		if (!flag)
		{
			throw new TlsException(AlertDescription.CertificateUnknown);
		}
	}

	private void InitializeConnection()
	{
		context = SSLCreateContext(IntPtr.Zero, (!base.IsServer) ? SslProtocolSide.Client : SslProtocolSide.Server, SslConnectionType.Stream);
		SslStatus status = SSLSetIOFuncs(Handle, readFunc, writeFunc);
		CheckStatusAndThrow(status);
		status = SSLSetConnection(Handle, GCHandle.ToIntPtr(handle));
		CheckStatusAndThrow(status);
		if ((base.EnabledProtocols & SslProtocols.Tls) != 0)
		{
			MinProtocol = SslProtocol.Tls_1_0;
		}
		else if ((base.EnabledProtocols & SslProtocols.Tls11) != 0)
		{
			MinProtocol = SslProtocol.Tls_1_1;
		}
		else
		{
			MinProtocol = SslProtocol.Tls_1_2;
		}
		if ((base.EnabledProtocols & SslProtocols.Tls12) != 0)
		{
			MaxProtocol = SslProtocol.Tls_1_2;
		}
		else if ((base.EnabledProtocols & SslProtocols.Tls11) != 0)
		{
			MaxProtocol = SslProtocol.Tls_1_1;
		}
		else
		{
			MaxProtocol = SslProtocol.Tls_1_0;
		}
		if (base.Settings != null && base.Settings.EnabledCiphers != null)
		{
			SslCipherSuite[] array = new SslCipherSuite[base.Settings.EnabledCiphers.Length];
			for (int i = 0; i < base.Settings.EnabledCiphers.Length; i++)
			{
				array[i] = (SslCipherSuite)base.Settings.EnabledCiphers[i];
			}
			SetEnabledCiphers(array);
		}
		if (base.AskForClientCertificate)
		{
			SetClientSideAuthenticate(SslAuthenticate.Try);
		}
		if (!base.IsServer && !string.IsNullOrEmpty(base.TargetHost) && !IPAddress.TryParse(base.TargetHost, out var _))
		{
			PeerDomainName = base.ServerName;
		}
	}

	private void InitializeSession()
	{
		if (connectionInfo == null)
		{
			SslCipherSuite negotiatedCipher = NegotiatedCipher;
			SslProtocol negotiatedProtocolVersion = GetNegotiatedProtocolVersion();
			connectionInfo = new MonoTlsConnectionInfo
			{
				CipherSuiteCode = (CipherSuiteCode)negotiatedCipher,
				ProtocolVersion = GetProtocol(negotiatedProtocolVersion),
				PeerDomainName = PeerDomainName
			};
		}
	}

	private static TlsProtocols GetProtocol(SslProtocol protocol)
	{
		return protocol switch
		{
			SslProtocol.Tls_1_0 => TlsProtocols.Tls10, 
			SslProtocol.Tls_1_1 => TlsProtocols.Tls11, 
			SslProtocol.Tls_1_2 => TlsProtocols.Tls12, 
			_ => throw new NotSupportedException(), 
		};
	}

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern SslStatus SSLGetProtocolVersionMax(IntPtr context, out SslProtocol maxVersion);

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern SslStatus SSLSetProtocolVersionMax(IntPtr context, SslProtocol maxVersion);

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern SslStatus SSLGetProtocolVersionMin(IntPtr context, out SslProtocol minVersion);

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern SslStatus SSLSetProtocolVersionMin(IntPtr context, SslProtocol minVersion);

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern SslStatus SSLGetNegotiatedProtocolVersion(IntPtr context, out SslProtocol protocol);

	public SslProtocol GetNegotiatedProtocolVersion()
	{
		SslProtocol protocol;
		SslStatus status = SSLGetNegotiatedProtocolVersion(Handle, out protocol);
		CheckStatusAndThrow(status);
		return protocol;
	}

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern SslStatus SSLGetSessionOption(IntPtr context, SslSessionOption option, out bool value);

	public bool GetSessionOption(SslSessionOption option)
	{
		bool value;
		SslStatus status = SSLGetSessionOption(Handle, option, out value);
		CheckStatusAndThrow(status);
		return value;
	}

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern SslStatus SSLSetSessionOption(IntPtr context, SslSessionOption option, bool value);

	public void SetSessionOption(SslSessionOption option, bool value)
	{
		SslStatus status = SSLSetSessionOption(Handle, option, value);
		CheckStatusAndThrow(status);
	}

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern SslStatus SSLSetClientSideAuthenticate(IntPtr context, SslAuthenticate auth);

	public void SetClientSideAuthenticate(SslAuthenticate auth)
	{
		SslStatus status = SSLSetClientSideAuthenticate(Handle, auth);
		CheckStatusAndThrow(status);
	}

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern SslStatus SSLHandshake(IntPtr context);

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern SslStatus SSLGetSessionState(IntPtr context, ref SslSessionState state);

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern SslStatus SSLGetPeerID(IntPtr context, out IntPtr peerID, out IntPtr peerIDLen);

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private unsafe static extern SslStatus SSLSetPeerID(IntPtr context, byte* peerID, IntPtr peerIDLen);

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern SslStatus SSLGetBufferedReadSize(IntPtr context, out IntPtr bufSize);

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern SslStatus SSLGetNumberSupportedCiphers(IntPtr context, out IntPtr numCiphers);

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private unsafe static extern SslStatus SSLGetSupportedCiphers(IntPtr context, SslCipherSuite* ciphers, ref IntPtr numCiphers);

	public unsafe IList<SslCipherSuite> GetSupportedCiphers()
	{
		SslStatus sslStatus = SSLGetNumberSupportedCiphers(Handle, out var numCiphers);
		CheckStatusAndThrow(sslStatus);
		if (sslStatus != 0 || (int)numCiphers <= 0)
		{
			return null;
		}
		SslCipherSuite[] array = new SslCipherSuite[(int)numCiphers];
		fixed (SslCipherSuite* ciphers = array)
		{
			sslStatus = SSLGetSupportedCiphers(Handle, ciphers, ref numCiphers);
		}
		CheckStatusAndThrow(sslStatus);
		return array;
	}

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern SslStatus SSLGetNumberEnabledCiphers(IntPtr context, out IntPtr numCiphers);

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private unsafe static extern SslStatus SSLGetEnabledCiphers(IntPtr context, SslCipherSuite* ciphers, ref IntPtr numCiphers);

	public unsafe IList<SslCipherSuite> GetEnabledCiphers()
	{
		SslStatus sslStatus = SSLGetNumberEnabledCiphers(Handle, out var numCiphers);
		CheckStatusAndThrow(sslStatus);
		if (sslStatus != 0 || (int)numCiphers <= 0)
		{
			return null;
		}
		SslCipherSuite[] array = new SslCipherSuite[(int)numCiphers];
		fixed (SslCipherSuite* ciphers = array)
		{
			sslStatus = SSLGetEnabledCiphers(Handle, ciphers, ref numCiphers);
		}
		CheckStatusAndThrow(sslStatus);
		return array;
	}

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private unsafe static extern SslStatus SSLSetEnabledCiphers(IntPtr context, SslCipherSuite* ciphers, IntPtr numCiphers);

	public unsafe void SetEnabledCiphers(SslCipherSuite[] ciphers)
	{
		if (ciphers == null)
		{
			throw new ArgumentNullException("ciphers");
		}
		SslStatus status;
		fixed (SslCipherSuite* ciphers2 = ciphers)
		{
			status = SSLSetEnabledCiphers(Handle, ciphers2, (IntPtr)ciphers.Length);
		}
		CheckStatusAndThrow(status);
	}

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern SslStatus SSLGetNegotiatedCipher(IntPtr context, out SslCipherSuite cipherSuite);

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern SslStatus SSLGetPeerDomainNameLength(IntPtr context, out IntPtr peerNameLen);

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern SslStatus SSLGetPeerDomainName(IntPtr context, byte[] peerName, ref IntPtr peerNameLen);

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern SslStatus SSLSetPeerDomainName(IntPtr context, byte[] peerName, IntPtr peerNameLen);

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern SslStatus SSLSetCertificate(IntPtr context, IntPtr certRefs);

	private CFArray Bundle(SecIdentity identity, IEnumerable<SecCertificate> certificates)
	{
		if (identity == null)
		{
			throw new ArgumentNullException("identity");
		}
		int num = 0;
		int num2 = 0;
		if (certificates != null)
		{
			foreach (SecCertificate certificate in certificates)
			{
				_ = certificate;
				num2++;
			}
		}
		IntPtr[] array = new IntPtr[num2 + 1];
		array[0] = identity.Handle;
		foreach (SecCertificate certificate2 in certificates)
		{
			array[++num] = certificate2.Handle;
		}
		return CFArray.CreateArray(array);
	}

	public void SetCertificate(SecIdentity identify, IEnumerable<SecCertificate> certificates)
	{
		using CFArray cFArray = Bundle(identify, certificates);
		SslStatus status = SSLSetCertificate(Handle, cFArray.Handle);
		CheckStatusAndThrow(status);
	}

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern SslStatus SSLGetClientCertificateState(IntPtr context, out SslClientCertificateState clientState);

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern SslStatus SSLCopyPeerTrust(IntPtr context, out IntPtr trust);

	public SecTrust GetPeerTrust(bool requireTrust)
	{
		IntPtr trust;
		SslStatus status = SSLCopyPeerTrust(Handle, out trust);
		if (requireTrust)
		{
			CheckStatusAndThrow(status);
			if (trust == IntPtr.Zero)
			{
				throw new TlsException(AlertDescription.CertificateUnknown);
			}
		}
		if (!(trust == IntPtr.Zero))
		{
			return new SecTrust(trust, owns: true);
		}
		return null;
	}

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern IntPtr SSLCreateContext(IntPtr alloc, SslProtocolSide protocolSide, SslConnectionType connectionType);

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern SslStatus SSLSetConnection(IntPtr context, IntPtr connection);

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern SslStatus SSLSetIOFuncs(IntPtr context, SslReadFunc readFunc, SslWriteFunc writeFunc);

	[MonoPInvokeCallback(typeof(SslReadFunc))]
	private static SslStatus NativeReadCallback(IntPtr ptr, IntPtr data, ref IntPtr dataLength)
	{
		AppleTlsContext appleTlsContext = null;
		try
		{
			GCHandle gCHandle = GCHandle.FromIntPtr(ptr);
			if (!gCHandle.IsAllocated)
			{
				return SslStatus.Internal;
			}
			appleTlsContext = (AppleTlsContext)gCHandle.Target;
			if (appleTlsContext == null || appleTlsContext.disposed)
			{
				return SslStatus.ClosedAbort;
			}
			return appleTlsContext.NativeReadCallback(data, ref dataLength);
		}
		catch (Exception ex)
		{
			if (appleTlsContext != null && appleTlsContext.lastException == null)
			{
				appleTlsContext.lastException = ex;
			}
			return SslStatus.Internal;
		}
	}

	[MonoPInvokeCallback(typeof(SslWriteFunc))]
	private static SslStatus NativeWriteCallback(IntPtr ptr, IntPtr data, ref IntPtr dataLength)
	{
		AppleTlsContext appleTlsContext = null;
		try
		{
			GCHandle gCHandle = GCHandle.FromIntPtr(ptr);
			if (!gCHandle.IsAllocated)
			{
				return SslStatus.Internal;
			}
			appleTlsContext = (AppleTlsContext)gCHandle.Target;
			if (appleTlsContext == null || appleTlsContext.disposed)
			{
				return SslStatus.ClosedAbort;
			}
			return appleTlsContext.NativeWriteCallback(data, ref dataLength);
		}
		catch (Exception ex)
		{
			if (appleTlsContext != null && appleTlsContext.lastException == null)
			{
				appleTlsContext.lastException = ex;
			}
			return SslStatus.Internal;
		}
	}

	private SslStatus NativeReadCallback(IntPtr data, ref IntPtr dataLength)
	{
		if (closed || disposed || base.Parent == null)
		{
			return SslStatus.ClosedAbort;
		}
		int num = (int)dataLength;
		byte[] array = new byte[num];
		bool outWantMore;
		int num2 = base.Parent.InternalRead(array, 0, num, out outWantMore);
		dataLength = (IntPtr)num2;
		if (num2 < 0)
		{
			return SslStatus.ClosedAbort;
		}
		Marshal.Copy(array, 0, data, num2);
		if (num2 > 0)
		{
			return SslStatus.Success;
		}
		if (outWantMore)
		{
			return SslStatus.WouldBlock;
		}
		if (num2 == 0)
		{
			closedGraceful = true;
			return SslStatus.ClosedGraceful;
		}
		return SslStatus.Success;
	}

	private SslStatus NativeWriteCallback(IntPtr data, ref IntPtr dataLength)
	{
		if (closed || disposed || base.Parent == null)
		{
			return SslStatus.ClosedAbort;
		}
		int num = (int)dataLength;
		byte[] array = new byte[num];
		Marshal.Copy(data, array, 0, num);
		if (!base.Parent.InternalWrite(array, 0, num))
		{
			return SslStatus.ClosedAbort;
		}
		return SslStatus.Success;
	}

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private unsafe static extern SslStatus SSLRead(IntPtr context, byte* data, IntPtr dataLength, out IntPtr processed);

	public unsafe override (int ret, bool wantMore) Read(byte[] buffer, int offset, int count)
	{
		if (Interlocked.Exchange(ref pendingIO, 1) == 1)
		{
			throw new InvalidOperationException();
		}
		lastException = null;
		try
		{
			SslStatus sslStatus;
			IntPtr processed;
			fixed (byte* data = &buffer[offset])
			{
				sslStatus = SSLRead(Handle, data, (IntPtr)count, out processed);
			}
			if (closedGraceful && (sslStatus == SslStatus.ClosedAbort || sslStatus == SslStatus.ClosedGraceful))
			{
				return (ret: 0, wantMore: false);
			}
			CheckStatusAndThrow(sslStatus, SslStatus.WouldBlock, SslStatus.ClosedGraceful);
			bool item = sslStatus == SslStatus.WouldBlock;
			return (ret: (int)processed, wantMore: item);
		}
		catch (Exception)
		{
			throw;
		}
		finally
		{
			pendingIO = 0;
		}
	}

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private unsafe static extern SslStatus SSLWrite(IntPtr context, byte* data, IntPtr dataLength, out IntPtr processed);

	public unsafe override (int ret, bool wantMore) Write(byte[] buffer, int offset, int count)
	{
		if (Interlocked.Exchange(ref pendingIO, 1) == 1)
		{
			throw new InvalidOperationException();
		}
		lastException = null;
		try
		{
			SslStatus sslStatus = SslStatus.ClosedAbort;
			IntPtr processed = (IntPtr)(-1);
			fixed (byte* data = &buffer[offset])
			{
				sslStatus = SSLWrite(Handle, data, (IntPtr)count, out processed);
			}
			CheckStatusAndThrow(sslStatus, SslStatus.WouldBlock);
			bool item = sslStatus == SslStatus.WouldBlock;
			return (ret: (int)processed, wantMore: item);
		}
		finally
		{
			pendingIO = 0;
		}
	}

	[DllImport("/System/Library/Frameworks/Security.framework/Security")]
	private static extern SslStatus SSLClose(IntPtr context);

	public override void Shutdown()
	{
		closed = true;
	}

	protected override void Dispose(bool disposing)
	{
		try
		{
			if (!disposed && disposing)
			{
				disposed = true;
				if (serverIdentity != null)
				{
					serverIdentity.Dispose();
					serverIdentity = null;
				}
				if (clientIdentity != null)
				{
					clientIdentity.Dispose();
					clientIdentity = null;
				}
				if (remoteCertificate != null)
				{
					remoteCertificate.Dispose();
					remoteCertificate = null;
				}
			}
		}
		finally
		{
			disposed = true;
			if (context != IntPtr.Zero)
			{
				CFObject.CFRelease(context);
				context = IntPtr.Zero;
			}
			base.Dispose(disposing);
		}
	}
}
