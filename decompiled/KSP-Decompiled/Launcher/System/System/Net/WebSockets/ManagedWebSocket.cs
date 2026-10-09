using System.Buffers;
using System.IO;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace System.Net.WebSockets;

internal sealed class ManagedWebSocket : WebSocket
{
	private sealed class Utf8MessageState
	{
		internal bool SequenceInProgress;

		internal int AdditionalBytesExpected;

		internal int ExpectedValueMin;

		internal int CurrentDecodeBits;
	}

	private enum MessageOpcode : byte
	{
		Continuation = 0,
		Text = 1,
		Binary = 2,
		Close = 8,
		Ping = 9,
		Pong = 10
	}

	[StructLayout(LayoutKind.Auto)]
	private struct MessageHeader
	{
		internal MessageOpcode Opcode;

		internal bool Fin;

		internal long PayloadLength;

		internal int Mask;
	}

	private static readonly RandomNumberGenerator s_random = RandomNumberGenerator.Create();

	private static readonly UTF8Encoding s_textEncoding = new UTF8Encoding(encoderShouldEmitUTF8Identifier: false, throwOnInvalidBytes: true);

	private static readonly WebSocketState[] s_validSendStates = new WebSocketState[2]
	{
		WebSocketState.Open,
		WebSocketState.CloseReceived
	};

	private static readonly WebSocketState[] s_validReceiveStates = new WebSocketState[2]
	{
		WebSocketState.Open,
		WebSocketState.CloseSent
	};

	private static readonly WebSocketState[] s_validCloseOutputStates = new WebSocketState[2]
	{
		WebSocketState.Open,
		WebSocketState.CloseReceived
	};

	private static readonly WebSocketState[] s_validCloseStates = new WebSocketState[3]
	{
		WebSocketState.Open,
		WebSocketState.CloseReceived,
		WebSocketState.CloseSent
	};

	private const int MaxMessageHeaderLength = 14;

	private const int MaxControlPayloadLength = 125;

	private const int MaskLength = 4;

	private readonly Stream _stream;

	private readonly bool _isServer;

	private readonly string _subprotocol;

	private readonly Timer _keepAliveTimer;

	private readonly CancellationTokenSource _abortSource = new CancellationTokenSource();

	private byte[] _receiveBuffer;

	private readonly bool _receiveBufferFromPool;

	private readonly Utf8MessageState _utf8TextState = new Utf8MessageState();

	private readonly SemaphoreSlim _sendFrameAsyncLock = new SemaphoreSlim(1, 1);

	private WebSocketState _state = WebSocketState.Open;

	private bool _disposed;

	private bool _sentCloseFrame;

	private bool _receivedCloseFrame;

	private WebSocketCloseStatus? _closeStatus;

	private string _closeStatusDescription;

	private MessageHeader _lastReceiveHeader = new MessageHeader
	{
		Opcode = MessageOpcode.Text,
		Fin = true
	};

	private int _receiveBufferOffset;

	private int _receiveBufferCount;

	private int _receivedMaskOffsetOffset;

	private byte[] _sendBuffer;

	private bool _lastSendWasFragment;

	private Task _lastSendAsync;

	private Task<WebSocketReceiveResult> _lastReceiveAsync;

	private object StateUpdateLock => _abortSource;

	private object ReceiveAsyncLock => _utf8TextState;

	public override WebSocketCloseStatus? CloseStatus => _closeStatus;

	public override string CloseStatusDescription => _closeStatusDescription;

	public override WebSocketState State => _state;

	public override string SubProtocol => _subprotocol;

	public static ManagedWebSocket CreateFromConnectedStream(Stream stream, bool isServer, string subprotocol, TimeSpan keepAliveInterval, int receiveBufferSize, ArraySegment<byte>? receiveBuffer = null)
	{
		return new ManagedWebSocket(stream, isServer, subprotocol, keepAliveInterval, receiveBufferSize, receiveBuffer);
	}

	private ManagedWebSocket(Stream stream, bool isServer, string subprotocol, TimeSpan keepAliveInterval, int receiveBufferSize, ArraySegment<byte>? receiveBuffer)
	{
		_stream = stream;
		_isServer = isServer;
		_subprotocol = subprotocol;
		if (receiveBuffer.HasValue && receiveBuffer.GetValueOrDefault().Array != null && receiveBuffer.GetValueOrDefault().Offset == 0 && receiveBuffer.GetValueOrDefault().Count == receiveBuffer.GetValueOrDefault().Array.Length && receiveBuffer.GetValueOrDefault().Count >= 14)
		{
			_receiveBuffer = receiveBuffer.Value.Array;
		}
		else
		{
			_receiveBufferFromPool = true;
			_receiveBuffer = ArrayPool<byte>.Shared.Rent(Math.Max(receiveBufferSize, 14));
		}
		_abortSource.Token.Register(delegate(object s)
		{
			ManagedWebSocket managedWebSocket = (ManagedWebSocket)s;
			lock (managedWebSocket.StateUpdateLock)
			{
				WebSocketState state = managedWebSocket._state;
				if (state != WebSocketState.Closed && state != WebSocketState.Aborted)
				{
					managedWebSocket._state = ((state != 0 && state != WebSocketState.Connecting) ? WebSocketState.Aborted : WebSocketState.Closed);
				}
			}
		}, this);
		if (keepAliveInterval > TimeSpan.Zero)
		{
			_keepAliveTimer = new Timer(delegate(object s)
			{
				((ManagedWebSocket)s).SendKeepAliveFrameAsync();
			}, this, keepAliveInterval, keepAliveInterval);
		}
	}

	public override void Dispose()
	{
		lock (StateUpdateLock)
		{
			DisposeCore();
		}
	}

	private void DisposeCore()
	{
		if (!_disposed)
		{
			_disposed = true;
			_keepAliveTimer?.Dispose();
			_stream?.Dispose();
			if (_receiveBufferFromPool)
			{
				byte[] receiveBuffer = _receiveBuffer;
				_receiveBuffer = null;
				ArrayPool<byte>.Shared.Return(receiveBuffer);
			}
			if (_state < WebSocketState.Aborted)
			{
				_state = WebSocketState.Closed;
			}
		}
	}

	public override Task SendAsync(ArraySegment<byte> buffer, WebSocketMessageType messageType, bool endOfMessage, CancellationToken cancellationToken)
	{
		if (messageType != 0 && messageType != WebSocketMessageType.Binary)
		{
			throw new ArgumentException(global::SR.Format("The message type '{0}' is not allowed for the '{1}' operation. Valid message types are: '{2}, {3}'. To close the WebSocket, use the '{4}' operation instead. ", "Close", "SendAsync", "Binary", "Text", "CloseOutputAsync"), "messageType");
		}
		WebSocketValidate.ValidateArraySegment(buffer, "buffer");
		try
		{
			WebSocketValidate.ThrowIfInvalidState(_state, _disposed, s_validSendStates);
			ThrowIfOperationInProgress(_lastSendAsync, "SendAsync");
		}
		catch (Exception exception)
		{
			return Task.FromException(exception);
		}
		MessageOpcode opcode = ((!_lastSendWasFragment) ? ((messageType != WebSocketMessageType.Binary) ? MessageOpcode.Text : MessageOpcode.Binary) : MessageOpcode.Continuation);
		Task task = SendFrameAsync(opcode, endOfMessage, buffer, cancellationToken);
		_lastSendWasFragment = !endOfMessage;
		_lastSendAsync = task;
		return task;
	}

	public override Task<WebSocketReceiveResult> ReceiveAsync(ArraySegment<byte> buffer, CancellationToken cancellationToken)
	{
		WebSocketValidate.ValidateArraySegment(buffer, "buffer");
		try
		{
			WebSocketValidate.ThrowIfInvalidState(_state, _disposed, s_validReceiveStates);
			lock (ReceiveAsyncLock)
			{
				ThrowIfOperationInProgress(_lastReceiveAsync, "ReceiveAsync");
				return _lastReceiveAsync = ReceiveAsyncPrivate(buffer, cancellationToken);
			}
		}
		catch (Exception exception)
		{
			return Task.FromException<WebSocketReceiveResult>(exception);
		}
	}

	public override Task CloseAsync(WebSocketCloseStatus closeStatus, string statusDescription, CancellationToken cancellationToken)
	{
		WebSocketValidate.ValidateCloseStatus(closeStatus, statusDescription);
		try
		{
			WebSocketValidate.ThrowIfInvalidState(_state, _disposed, s_validCloseStates);
		}
		catch (Exception exception)
		{
			return Task.FromException(exception);
		}
		return CloseAsyncPrivate(closeStatus, statusDescription, cancellationToken);
	}

	public override Task CloseOutputAsync(WebSocketCloseStatus closeStatus, string statusDescription, CancellationToken cancellationToken)
	{
		WebSocketValidate.ValidateCloseStatus(closeStatus, statusDescription);
		try
		{
			WebSocketValidate.ThrowIfInvalidState(_state, _disposed, s_validCloseOutputStates);
		}
		catch (Exception exception)
		{
			return Task.FromException(exception);
		}
		return SendCloseFrameAsync(closeStatus, statusDescription, cancellationToken);
	}

	public override void Abort()
	{
		_abortSource.Cancel();
		Dispose();
	}

	private Task SendFrameAsync(MessageOpcode opcode, bool endOfMessage, ArraySegment<byte> payloadBuffer, CancellationToken cancellationToken)
	{
		if (!cancellationToken.CanBeCanceled && _sendFrameAsyncLock.Wait(0))
		{
			return SendFrameLockAcquiredNonCancelableAsync(opcode, endOfMessage, payloadBuffer);
		}
		return SendFrameFallbackAsync(opcode, endOfMessage, payloadBuffer, cancellationToken);
	}

	private Task SendFrameLockAcquiredNonCancelableAsync(MessageOpcode opcode, bool endOfMessage, ArraySegment<byte> payloadBuffer)
	{
		Task task = null;
		bool flag = true;
		try
		{
			int count = WriteFrameToSendBuffer(opcode, endOfMessage, payloadBuffer);
			task = _stream.WriteAsync(_sendBuffer, 0, count, CancellationToken.None);
			if (task.IsCompleted)
			{
				return task;
			}
			flag = false;
		}
		catch (Exception innerException)
		{
			return Task.FromException((_state == WebSocketState.Aborted) ? CreateOperationCanceledException(innerException) : new WebSocketException(WebSocketError.ConnectionClosedPrematurely, innerException));
		}
		finally
		{
			if (flag)
			{
				_sendFrameAsyncLock.Release();
				ReleaseSendBuffer();
			}
		}
		return task.ContinueWith(delegate(Task t, object s)
		{
			ManagedWebSocket managedWebSocket = (ManagedWebSocket)s;
			managedWebSocket._sendFrameAsyncLock.Release();
			managedWebSocket.ReleaseSendBuffer();
			try
			{
				t.GetAwaiter().GetResult();
			}
			catch (Exception innerException2)
			{
				throw (managedWebSocket._state == WebSocketState.Aborted) ? CreateOperationCanceledException(innerException2) : new WebSocketException(WebSocketError.ConnectionClosedPrematurely, innerException2);
			}
		}, this, CancellationToken.None, TaskContinuationOptions.ExecuteSynchronously, TaskScheduler.Default);
	}

	private async Task SendFrameFallbackAsync(MessageOpcode opcode, bool endOfMessage, ArraySegment<byte> payloadBuffer, CancellationToken cancellationToken)
	{
		await _sendFrameAsyncLock.WaitAsync().ConfigureAwait(continueOnCapturedContext: false);
		try
		{
			int count = WriteFrameToSendBuffer(opcode, endOfMessage, payloadBuffer);
			using (cancellationToken.Register(delegate(object s)
			{
				((ManagedWebSocket)s).Abort();
			}, this))
			{
				await _stream.WriteAsync(_sendBuffer, 0, count, cancellationToken).ConfigureAwait(continueOnCapturedContext: false);
			}
		}
		catch (Exception innerException)
		{
			throw (_state == WebSocketState.Aborted) ? CreateOperationCanceledException(innerException, cancellationToken) : new WebSocketException(WebSocketError.ConnectionClosedPrematurely, innerException);
		}
		finally
		{
			_sendFrameAsyncLock.Release();
			ReleaseSendBuffer();
		}
	}

	private int WriteFrameToSendBuffer(MessageOpcode opcode, bool endOfMessage, ArraySegment<byte> payloadBuffer)
	{
		AllocateSendBuffer(payloadBuffer.Count + 14);
		int? num = null;
		int num2;
		if (_isServer)
		{
			num2 = WriteHeader(opcode, _sendBuffer, payloadBuffer, endOfMessage, useMask: false);
		}
		else
		{
			num = WriteHeader(opcode, _sendBuffer, payloadBuffer, endOfMessage, useMask: true);
			num2 = num.GetValueOrDefault() + 4;
		}
		if (payloadBuffer.Count > 0)
		{
			Buffer.BlockCopy(payloadBuffer.Array, payloadBuffer.Offset, _sendBuffer, num2, payloadBuffer.Count);
			if (num.HasValue)
			{
				ApplyMask(_sendBuffer, num2, _sendBuffer, num.Value, 0, payloadBuffer.Count);
			}
		}
		return num2 + payloadBuffer.Count;
	}

	private void SendKeepAliveFrameAsync()
	{
		if (!_sendFrameAsyncLock.Wait(0))
		{
			return;
		}
		Task task = SendFrameLockAcquiredNonCancelableAsync(MessageOpcode.Ping, endOfMessage: true, new ArraySegment<byte>(Array.Empty<byte>()));
		if (!task.IsCompletedSuccessfully)
		{
			task.ContinueWith(delegate(Task p)
			{
				_ = p.Exception;
			}, CancellationToken.None, TaskContinuationOptions.OnlyOnFaulted | TaskContinuationOptions.ExecuteSynchronously, TaskScheduler.Default);
		}
	}

	private static int WriteHeader(MessageOpcode opcode, byte[] sendBuffer, ArraySegment<byte> payload, bool endOfMessage, bool useMask)
	{
		sendBuffer[0] = (byte)opcode;
		if (endOfMessage)
		{
			sendBuffer[0] |= 128;
		}
		int num;
		if (payload.Count <= 125)
		{
			sendBuffer[1] = (byte)payload.Count;
			num = 2;
		}
		else if (payload.Count <= 65535)
		{
			sendBuffer[1] = 126;
			sendBuffer[2] = (byte)(payload.Count / 256);
			sendBuffer[3] = (byte)payload.Count;
			num = 4;
		}
		else
		{
			sendBuffer[1] = 127;
			int num2 = payload.Count;
			for (int num3 = 9; num3 >= 2; num3--)
			{
				sendBuffer[num3] = (byte)num2;
				num2 /= 256;
			}
			num = 10;
		}
		if (useMask)
		{
			sendBuffer[1] |= 128;
			WriteRandomMask(sendBuffer, num);
		}
		return num;
	}

	private static void WriteRandomMask(byte[] buffer, int offset)
	{
		s_random.GetBytes(buffer, offset, 4);
	}

	private async Task<WebSocketReceiveResult> ReceiveAsyncPrivate(ArraySegment<byte> payloadBuffer, CancellationToken cancellationToken)
	{
		CancellationTokenRegistration registration = cancellationToken.Register(delegate(object s)
		{
			((ManagedWebSocket)s).Abort();
		}, this);
		try
		{
			MessageHeader header;
			while (true)
			{
				header = _lastReceiveHeader;
				if (header.PayloadLength == 0L)
				{
					if (_receiveBufferCount < (_isServer ? 10 : 14))
					{
						if (_receiveBufferCount < 2)
						{
							await EnsureBufferContainsAsync(2, cancellationToken).ConfigureAwait(continueOnCapturedContext: false);
						}
						long num = _receiveBuffer[_receiveBufferOffset + 1] & 0x7F;
						if (_isServer || num > 125)
						{
							int minimumRequiredBytes = 2 + (_isServer ? 4 : 0) + ((num > 125) ? ((num == 126) ? 2 : 8) : 0);
							await EnsureBufferContainsAsync(minimumRequiredBytes, cancellationToken).ConfigureAwait(continueOnCapturedContext: false);
						}
					}
					if (!TryParseMessageHeaderFromReceiveBuffer(out header))
					{
						await CloseWithReceiveErrorAndThrowAsync(WebSocketCloseStatus.ProtocolError, WebSocketError.Faulted, cancellationToken).ConfigureAwait(continueOnCapturedContext: false);
					}
					_receivedMaskOffsetOffset = 0;
				}
				if (header.Opcode != MessageOpcode.Ping && header.Opcode != MessageOpcode.Pong)
				{
					break;
				}
				await HandleReceivedPingPongAsync(header, cancellationToken).ConfigureAwait(continueOnCapturedContext: false);
			}
			if (header.Opcode == MessageOpcode.Close)
			{
				return await HandleReceivedCloseAsync(header, cancellationToken).ConfigureAwait(continueOnCapturedContext: false);
			}
			if (header.Opcode == MessageOpcode.Continuation)
			{
				header.Opcode = _lastReceiveHeader.Opcode;
			}
			int bytesToRead = (int)Math.Min(payloadBuffer.Count, header.PayloadLength);
			if (bytesToRead == 0)
			{
				_lastReceiveHeader = header;
				return new WebSocketReceiveResult(0, (header.Opcode != MessageOpcode.Text) ? WebSocketMessageType.Binary : WebSocketMessageType.Text, header.PayloadLength == 0L && header.Fin);
			}
			if (_receiveBufferCount == 0)
			{
				await EnsureBufferContainsAsync(1, cancellationToken, throwOnPrematureClosure: false).ConfigureAwait(continueOnCapturedContext: false);
			}
			int bytesToCopy = Math.Min(bytesToRead, _receiveBufferCount);
			if (_isServer)
			{
				_receivedMaskOffsetOffset = ApplyMask(_receiveBuffer, _receiveBufferOffset, header.Mask, _receivedMaskOffsetOffset, bytesToCopy);
			}
			Buffer.BlockCopy(_receiveBuffer, _receiveBufferOffset, payloadBuffer.Array, payloadBuffer.Offset, bytesToCopy);
			ConsumeFromBuffer(bytesToCopy);
			header.PayloadLength -= bytesToCopy;
			if (header.Opcode == MessageOpcode.Text && !TryValidateUtf8(new ArraySegment<byte>(payloadBuffer.Array, payloadBuffer.Offset, bytesToCopy), header.Fin, _utf8TextState))
			{
				await CloseWithReceiveErrorAndThrowAsync(WebSocketCloseStatus.InvalidPayloadData, WebSocketError.Faulted, cancellationToken).ConfigureAwait(continueOnCapturedContext: false);
			}
			_lastReceiveHeader = header;
			return new WebSocketReceiveResult(bytesToCopy, (header.Opcode != MessageOpcode.Text) ? WebSocketMessageType.Binary : WebSocketMessageType.Text, bytesToCopy == 0 || (header.Fin && header.PayloadLength == 0));
		}
		catch (Exception innerException)
		{
			if (_state == WebSocketState.Aborted)
			{
				throw new OperationCanceledException("Aborted", innerException);
			}
			throw new WebSocketException(WebSocketError.ConnectionClosedPrematurely, innerException);
		}
		finally
		{
			registration.Dispose();
		}
	}

	private async Task<WebSocketReceiveResult> HandleReceivedCloseAsync(MessageHeader header, CancellationToken cancellationToken)
	{
		lock (StateUpdateLock)
		{
			_receivedCloseFrame = true;
			if (_state < WebSocketState.CloseReceived)
			{
				_state = WebSocketState.CloseReceived;
			}
		}
		WebSocketCloseStatus closeStatus = WebSocketCloseStatus.NormalClosure;
		string closeStatusDescription = string.Empty;
		if (header.PayloadLength == 1)
		{
			await CloseWithReceiveErrorAndThrowAsync(WebSocketCloseStatus.ProtocolError, WebSocketError.Faulted, cancellationToken).ConfigureAwait(continueOnCapturedContext: false);
		}
		else if (header.PayloadLength >= 2)
		{
			if (_receiveBufferCount < header.PayloadLength)
			{
				await EnsureBufferContainsAsync((int)header.PayloadLength, cancellationToken).ConfigureAwait(continueOnCapturedContext: false);
			}
			if (_isServer)
			{
				ApplyMask(_receiveBuffer, _receiveBufferOffset, header.Mask, 0, header.PayloadLength);
			}
			closeStatus = (WebSocketCloseStatus)((_receiveBuffer[_receiveBufferOffset] << 8) | _receiveBuffer[_receiveBufferOffset + 1]);
			if (!IsValidCloseStatus(closeStatus))
			{
				await CloseWithReceiveErrorAndThrowAsync(WebSocketCloseStatus.ProtocolError, WebSocketError.Faulted, cancellationToken).ConfigureAwait(continueOnCapturedContext: false);
			}
			if (header.PayloadLength > 2)
			{
				try
				{
					closeStatusDescription = s_textEncoding.GetString(_receiveBuffer, _receiveBufferOffset + 2, (int)header.PayloadLength - 2);
				}
				catch (DecoderFallbackException innerException)
				{
					await CloseWithReceiveErrorAndThrowAsync(WebSocketCloseStatus.ProtocolError, WebSocketError.Faulted, cancellationToken, innerException).ConfigureAwait(continueOnCapturedContext: false);
				}
			}
			ConsumeFromBuffer((int)header.PayloadLength);
		}
		_closeStatus = closeStatus;
		_closeStatusDescription = closeStatusDescription;
		return new WebSocketReceiveResult(0, WebSocketMessageType.Close, endOfMessage: true, closeStatus, closeStatusDescription);
	}

	private async Task HandleReceivedPingPongAsync(MessageHeader header, CancellationToken cancellationToken)
	{
		if (header.PayloadLength > 0 && _receiveBufferCount < header.PayloadLength)
		{
			await EnsureBufferContainsAsync((int)header.PayloadLength, cancellationToken).ConfigureAwait(continueOnCapturedContext: false);
		}
		if (header.Opcode == MessageOpcode.Ping)
		{
			if (_isServer)
			{
				ApplyMask(_receiveBuffer, _receiveBufferOffset, header.Mask, 0, header.PayloadLength);
			}
			await SendFrameAsync(MessageOpcode.Pong, endOfMessage: true, new ArraySegment<byte>(_receiveBuffer, _receiveBufferOffset, (int)header.PayloadLength), cancellationToken).ConfigureAwait(continueOnCapturedContext: false);
		}
		if (header.PayloadLength > 0)
		{
			ConsumeFromBuffer((int)header.PayloadLength);
		}
	}

	private static bool IsValidCloseStatus(WebSocketCloseStatus closeStatus)
	{
		if (closeStatus < WebSocketCloseStatus.NormalClosure || closeStatus >= (WebSocketCloseStatus)5000)
		{
			return false;
		}
		if (closeStatus >= (WebSocketCloseStatus)3000)
		{
			return true;
		}
		if ((uint)(closeStatus - 1000) <= 3u || (uint)(closeStatus - 1007) <= 4u)
		{
			return true;
		}
		return false;
	}

	private async Task CloseWithReceiveErrorAndThrowAsync(WebSocketCloseStatus closeStatus, WebSocketError error, CancellationToken cancellationToken, Exception innerException = null)
	{
		if (!_sentCloseFrame)
		{
			await CloseOutputAsync(closeStatus, string.Empty, cancellationToken).ConfigureAwait(continueOnCapturedContext: false);
		}
		_receiveBufferCount = 0;
		throw new WebSocketException(error, innerException);
	}

	private bool TryParseMessageHeaderFromReceiveBuffer(out MessageHeader resultHeader)
	{
		MessageHeader messageHeader = default(MessageHeader);
		messageHeader.Fin = (_receiveBuffer[_receiveBufferOffset] & 0x80) != 0;
		bool flag = (_receiveBuffer[_receiveBufferOffset] & 0x70) != 0;
		messageHeader.Opcode = (MessageOpcode)(_receiveBuffer[_receiveBufferOffset] & 0xFu);
		bool flag2 = (_receiveBuffer[_receiveBufferOffset + 1] & 0x80) != 0;
		messageHeader.PayloadLength = _receiveBuffer[_receiveBufferOffset + 1] & 0x7F;
		ConsumeFromBuffer(2);
		if (messageHeader.PayloadLength == 126)
		{
			messageHeader.PayloadLength = (_receiveBuffer[_receiveBufferOffset] << 8) | _receiveBuffer[_receiveBufferOffset + 1];
			ConsumeFromBuffer(2);
		}
		else if (messageHeader.PayloadLength == 127)
		{
			messageHeader.PayloadLength = 0L;
			for (int i = 0; i < 8; i++)
			{
				messageHeader.PayloadLength = (messageHeader.PayloadLength << 8) | _receiveBuffer[_receiveBufferOffset + i];
			}
			ConsumeFromBuffer(8);
		}
		bool flag3 = flag;
		if (flag2)
		{
			if (!_isServer)
			{
				flag3 = true;
			}
			messageHeader.Mask = CombineMaskBytes(_receiveBuffer, _receiveBufferOffset);
			ConsumeFromBuffer(4);
		}
		switch (messageHeader.Opcode)
		{
		case MessageOpcode.Continuation:
			if (_lastReceiveHeader.Fin)
			{
				flag3 = true;
			}
			break;
		case MessageOpcode.Text:
		case MessageOpcode.Binary:
			if (!_lastReceiveHeader.Fin)
			{
				flag3 = true;
			}
			break;
		case MessageOpcode.Close:
		case MessageOpcode.Ping:
		case MessageOpcode.Pong:
			if (messageHeader.PayloadLength > 125 || !messageHeader.Fin)
			{
				flag3 = true;
			}
			break;
		default:
			flag3 = true;
			break;
		}
		resultHeader = messageHeader;
		return !flag3;
	}

	private async Task CloseAsyncPrivate(WebSocketCloseStatus closeStatus, string statusDescription, CancellationToken cancellationToken)
	{
		if (!_sentCloseFrame)
		{
			await SendCloseFrameAsync(closeStatus, statusDescription, cancellationToken).ConfigureAwait(continueOnCapturedContext: false);
		}
		byte[] closeBuffer = ArrayPool<byte>.Shared.Rent(139);
		try
		{
			while (!_receivedCloseFrame)
			{
				Task<WebSocketReceiveResult> task;
				lock (ReceiveAsyncLock)
				{
					if (_receivedCloseFrame)
					{
						break;
					}
					task = _lastReceiveAsync;
					if (task == null || (task.Status == TaskStatus.RanToCompletion && task.Result.MessageType != WebSocketMessageType.Close))
					{
						task = (_lastReceiveAsync = ReceiveAsyncPrivate(new ArraySegment<byte>(closeBuffer), cancellationToken));
					}
					goto IL_0130;
				}
				IL_0130:
				await task.ConfigureAwait(continueOnCapturedContext: false);
			}
		}
		finally
		{
			ArrayPool<byte>.Shared.Return(closeBuffer);
		}
		lock (StateUpdateLock)
		{
			DisposeCore();
			if (_state < WebSocketState.Closed)
			{
				_state = WebSocketState.Closed;
			}
		}
	}

	private async Task SendCloseFrameAsync(WebSocketCloseStatus closeStatus, string closeStatusDescription, CancellationToken cancellationToken)
	{
		byte[] buffer = null;
		try
		{
			int num = 2;
			if (string.IsNullOrEmpty(closeStatusDescription))
			{
				buffer = ArrayPool<byte>.Shared.Rent(num);
			}
			else
			{
				num += s_textEncoding.GetByteCount(closeStatusDescription);
				buffer = ArrayPool<byte>.Shared.Rent(num);
				s_textEncoding.GetBytes(closeStatusDescription, 0, closeStatusDescription.Length, buffer, 2);
			}
			ushort num2 = (ushort)closeStatus;
			buffer[0] = (byte)(num2 >> 8);
			buffer[1] = (byte)(num2 & 0xFFu);
			await SendFrameAsync(MessageOpcode.Close, endOfMessage: true, new ArraySegment<byte>(buffer, 0, num), cancellationToken).ConfigureAwait(continueOnCapturedContext: false);
		}
		finally
		{
			if (buffer != null)
			{
				ArrayPool<byte>.Shared.Return(buffer);
			}
		}
		lock (StateUpdateLock)
		{
			_sentCloseFrame = true;
			if (_state <= WebSocketState.CloseReceived)
			{
				_state = WebSocketState.CloseSent;
			}
		}
	}

	private void ConsumeFromBuffer(int count)
	{
		_receiveBufferCount -= count;
		_receiveBufferOffset += count;
	}

	private async Task EnsureBufferContainsAsync(int minimumRequiredBytes, CancellationToken cancellationToken, bool throwOnPrematureClosure = true)
	{
		if (_receiveBufferCount >= minimumRequiredBytes)
		{
			return;
		}
		if (_receiveBufferCount > 0)
		{
			Buffer.BlockCopy(_receiveBuffer, _receiveBufferOffset, _receiveBuffer, 0, _receiveBufferCount);
		}
		_receiveBufferOffset = 0;
		while (_receiveBufferCount < minimumRequiredBytes)
		{
			int num = await _stream.ReadAsync(_receiveBuffer, _receiveBufferCount, _receiveBuffer.Length - _receiveBufferCount, cancellationToken).ConfigureAwait(continueOnCapturedContext: false);
			_receiveBufferCount += num;
			if (num == 0)
			{
				if (_disposed)
				{
					throw new ObjectDisposedException("WebSocket");
				}
				if (throwOnPrematureClosure)
				{
					throw new WebSocketException(WebSocketError.ConnectionClosedPrematurely);
				}
				break;
			}
		}
	}

	private void AllocateSendBuffer(int minLength)
	{
		_sendBuffer = ArrayPool<byte>.Shared.Rent(minLength);
	}

	private void ReleaseSendBuffer()
	{
		byte[] sendBuffer = _sendBuffer;
		if (sendBuffer != null)
		{
			_sendBuffer = null;
			ArrayPool<byte>.Shared.Return(sendBuffer);
		}
	}

	private static int CombineMaskBytes(byte[] buffer, int maskOffset)
	{
		return BitConverter.ToInt32(buffer, maskOffset);
	}

	private static int ApplyMask(byte[] toMask, int toMaskOffset, byte[] mask, int maskOffset, int maskOffsetIndex, long count)
	{
		return ApplyMask(toMask, toMaskOffset, CombineMaskBytes(mask, maskOffset), maskOffsetIndex, count);
	}

	private unsafe static int ApplyMask(byte[] toMask, int toMaskOffset, int mask, int maskIndex, long count)
	{
		int num = maskIndex * 8;
		int num2 = (mask >>> num) | (mask << 32 - num);
		if (count > 0)
		{
			fixed (byte* ptr = toMask)
			{
				byte* ptr2 = ptr + toMaskOffset;
				if ((long)ptr2 % 4L == 0L)
				{
					while (count >= 4)
					{
						count -= 4;
						*(int*)ptr2 ^= num2;
						ptr2 += 4;
					}
				}
				if (count > 0)
				{
					byte* ptr3 = (byte*)(&mask);
					byte* ptr4 = ptr2 + count;
					while (ptr2 < ptr4)
					{
						byte* intPtr = ptr2++;
						*intPtr ^= ptr3[maskIndex];
						maskIndex = (maskIndex + 1) & 3;
					}
				}
			}
		}
		return maskIndex;
	}

	private void ThrowIfOperationInProgress(Task operationTask, [CallerMemberName] string methodName = null)
	{
		if (operationTask != null && !operationTask.IsCompleted)
		{
			Abort();
			throw new InvalidOperationException(global::SR.Format("There is already one outstanding '{0}' call for this WebSocket instance. ReceiveAsync and SendAsync can be called simultaneously, but at most one outstanding operation for each of them is allowed at the same time.", methodName));
		}
	}

	private static Exception CreateOperationCanceledException(Exception innerException, CancellationToken cancellationToken = default(CancellationToken))
	{
		return new OperationCanceledException(new OperationCanceledException().Message, innerException, cancellationToken);
	}

	private static bool TryValidateUtf8(ArraySegment<byte> arraySegment, bool endOfMessage, Utf8MessageState state)
	{
		int num = arraySegment.Offset;
		while (num < arraySegment.Offset + arraySegment.Count)
		{
			if (!state.SequenceInProgress)
			{
				state.SequenceInProgress = true;
				byte b = arraySegment.Array[num];
				num++;
				if ((b & 0x80) == 0)
				{
					state.AdditionalBytesExpected = 0;
					state.CurrentDecodeBits = b & 0x7F;
					state.ExpectedValueMin = 0;
				}
				else
				{
					if ((b & 0xC0) == 128)
					{
						return false;
					}
					if ((b & 0xE0) == 192)
					{
						state.AdditionalBytesExpected = 1;
						state.CurrentDecodeBits = b & 0x1F;
						state.ExpectedValueMin = 128;
					}
					else if ((b & 0xF0) == 224)
					{
						state.AdditionalBytesExpected = 2;
						state.CurrentDecodeBits = b & 0xF;
						state.ExpectedValueMin = 2048;
					}
					else
					{
						if ((b & 0xF8) != 240)
						{
							return false;
						}
						state.AdditionalBytesExpected = 3;
						state.CurrentDecodeBits = b & 7;
						state.ExpectedValueMin = 65536;
					}
				}
			}
			while (state.AdditionalBytesExpected > 0 && num < arraySegment.Offset + arraySegment.Count)
			{
				byte b2 = arraySegment.Array[num];
				if ((b2 & 0xC0) != 128)
				{
					return false;
				}
				num++;
				state.AdditionalBytesExpected--;
				state.CurrentDecodeBits = (state.CurrentDecodeBits << 6) | (b2 & 0x3F);
				if (state.AdditionalBytesExpected == 1 && state.CurrentDecodeBits >= 864 && state.CurrentDecodeBits <= 895)
				{
					return false;
				}
				if (state.AdditionalBytesExpected == 2 && state.CurrentDecodeBits >= 272)
				{
					return false;
				}
			}
			if (state.AdditionalBytesExpected == 0)
			{
				state.SequenceInProgress = false;
				if (state.CurrentDecodeBits < state.ExpectedValueMin)
				{
					return false;
				}
			}
		}
		if (endOfMessage && state.SequenceInProgress)
		{
			return false;
		}
		return true;
	}
}
