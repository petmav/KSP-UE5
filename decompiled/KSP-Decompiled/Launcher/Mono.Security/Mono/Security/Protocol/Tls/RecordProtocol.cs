using System;
using System.IO;
using System.Threading;
using Mono.Security.Protocol.Tls.Handshake;

namespace Mono.Security.Protocol.Tls;

internal abstract class RecordProtocol
{
	private class ReceiveRecordAsyncResult : IAsyncResult
	{
		private object locker = new object();

		private AsyncCallback _userCallback;

		private object _userState;

		private Exception _asyncException;

		private ManualResetEvent handle;

		private byte[] _resultingBuffer;

		private Stream _record;

		private bool completed;

		private byte[] _initialBuffer;

		public Stream Record => _record;

		public byte[] ResultingBuffer => _resultingBuffer;

		public byte[] InitialBuffer => _initialBuffer;

		public object AsyncState => _userState;

		public Exception AsyncException => _asyncException;

		public bool CompletedWithError
		{
			get
			{
				if (!IsCompleted)
				{
					return false;
				}
				return _asyncException != null;
			}
		}

		public WaitHandle AsyncWaitHandle
		{
			get
			{
				lock (locker)
				{
					if (handle == null)
					{
						handle = new ManualResetEvent(completed);
					}
				}
				return handle;
			}
		}

		public bool CompletedSynchronously => false;

		public bool IsCompleted
		{
			get
			{
				lock (locker)
				{
					return completed;
				}
			}
		}

		public ReceiveRecordAsyncResult(AsyncCallback userCallback, object userState, byte[] initialBuffer, Stream record)
		{
			_userCallback = userCallback;
			_userState = userState;
			_initialBuffer = initialBuffer;
			_record = record;
		}

		private void SetComplete(Exception ex, byte[] resultingBuffer)
		{
			lock (locker)
			{
				if (!completed)
				{
					completed = true;
					_asyncException = ex;
					_resultingBuffer = resultingBuffer;
					if (handle != null)
					{
						handle.Set();
					}
					if (_userCallback != null)
					{
						_userCallback.BeginInvoke(this, null, null);
					}
				}
			}
		}

		public void SetComplete(Exception ex)
		{
			SetComplete(ex, null);
		}

		public void SetComplete(byte[] resultingBuffer)
		{
			SetComplete(null, resultingBuffer);
		}

		public void SetComplete()
		{
			SetComplete(null, null);
		}
	}

	private class SendRecordAsyncResult : IAsyncResult
	{
		private object locker = new object();

		private AsyncCallback _userCallback;

		private object _userState;

		private Exception _asyncException;

		private ManualResetEvent handle;

		private HandshakeMessage _message;

		private bool completed;

		public HandshakeMessage Message => _message;

		public object AsyncState => _userState;

		public Exception AsyncException => _asyncException;

		public bool CompletedWithError
		{
			get
			{
				if (!IsCompleted)
				{
					return false;
				}
				return _asyncException != null;
			}
		}

		public WaitHandle AsyncWaitHandle
		{
			get
			{
				lock (locker)
				{
					if (handle == null)
					{
						handle = new ManualResetEvent(completed);
					}
				}
				return handle;
			}
		}

		public bool CompletedSynchronously => false;

		public bool IsCompleted
		{
			get
			{
				lock (locker)
				{
					return completed;
				}
			}
		}

		public SendRecordAsyncResult(AsyncCallback userCallback, object userState, HandshakeMessage message)
		{
			_userCallback = userCallback;
			_userState = userState;
			_message = message;
		}

		public void SetComplete(Exception ex)
		{
			lock (locker)
			{
				if (!completed)
				{
					completed = true;
					if (handle != null)
					{
						handle.Set();
					}
					if (_userCallback != null)
					{
						_userCallback.BeginInvoke(this, null, null);
					}
					_asyncException = ex;
				}
			}
		}

		public void SetComplete()
		{
			SetComplete(null);
		}
	}

	private static ManualResetEvent record_processing = new ManualResetEvent(initialState: true);

	protected Stream innerStream;

	protected Context context;

	public Context Context
	{
		get
		{
			return context;
		}
		set
		{
			context = value;
		}
	}

	public RecordProtocol(Stream innerStream, Context context)
	{
		this.innerStream = innerStream;
		this.context = context;
		this.context.RecordProtocol = this;
	}

	public virtual void SendRecord(HandshakeType type)
	{
		IAsyncResult asyncResult = BeginSendRecord(type, null, null);
		EndSendRecord(asyncResult);
	}

	protected abstract void ProcessHandshakeMessage(TlsStream handMsg);

	protected virtual void ProcessChangeCipherSpec()
	{
		Context context = Context;
		context.ReadSequenceNumber = 0uL;
		if (context is ClientContext)
		{
			context.EndSwitchingSecurityParameters(client: true);
		}
		else
		{
			context.StartSwitchingSecurityParameters(client: false);
		}
		context.ChangeCipherSpecDone = true;
	}

	public virtual HandshakeMessage GetMessage(HandshakeType type)
	{
		throw new NotSupportedException();
	}

	public IAsyncResult BeginReceiveRecord(Stream record, AsyncCallback callback, object state)
	{
		if (context.ReceivedConnectionEnd)
		{
			throw new TlsException(AlertDescription.InternalError, "The session is finished and it's no longer valid.");
		}
		record_processing.Reset();
		byte[] initialBuffer = new byte[1];
		ReceiveRecordAsyncResult receiveRecordAsyncResult = new ReceiveRecordAsyncResult(callback, state, initialBuffer, record);
		record.BeginRead(receiveRecordAsyncResult.InitialBuffer, 0, receiveRecordAsyncResult.InitialBuffer.Length, InternalReceiveRecordCallback, receiveRecordAsyncResult);
		return receiveRecordAsyncResult;
	}

	private void InternalReceiveRecordCallback(IAsyncResult asyncResult)
	{
		ReceiveRecordAsyncResult receiveRecordAsyncResult = asyncResult.AsyncState as ReceiveRecordAsyncResult;
		Stream record = receiveRecordAsyncResult.Record;
		try
		{
			if (receiveRecordAsyncResult.Record.EndRead(asyncResult) == 0)
			{
				receiveRecordAsyncResult.SetComplete((byte[])null);
				return;
			}
			int num = receiveRecordAsyncResult.InitialBuffer[0];
			ContentType contentType = (ContentType)num;
			byte[] array = ReadRecordBuffer(num, record);
			if (array == null)
			{
				receiveRecordAsyncResult.SetComplete((byte[])null);
				return;
			}
			if ((contentType != ContentType.Alert || array.Length != 2) && Context.Read != null && Context.Read.Cipher != null)
			{
				array = decryptRecordFragment(contentType, array);
			}
			switch (contentType)
			{
			case ContentType.Alert:
				ProcessAlert((AlertLevel)array[0], (AlertDescription)array[1]);
				if (record.CanSeek)
				{
					record.SetLength(0L);
				}
				array = null;
				break;
			case ContentType.ChangeCipherSpec:
				ProcessChangeCipherSpec();
				break;
			case ContentType.Handshake:
			{
				TlsStream tlsStream = new TlsStream(array);
				while (!tlsStream.EOF)
				{
					ProcessHandshakeMessage(tlsStream);
				}
				break;
			}
			case (ContentType)128:
				context.HandshakeMessages.Write(array);
				break;
			default:
				throw new TlsException(AlertDescription.UnexpectedMessage, "Unknown record received from server.");
			case ContentType.ApplicationData:
				break;
			}
			receiveRecordAsyncResult.SetComplete(array);
		}
		catch (Exception complete)
		{
			receiveRecordAsyncResult.SetComplete(complete);
		}
	}

	public byte[] EndReceiveRecord(IAsyncResult asyncResult)
	{
		if (!(asyncResult is ReceiveRecordAsyncResult receiveRecordAsyncResult))
		{
			throw new ArgumentException("Either the provided async result is null or was not created by this RecordProtocol.");
		}
		if (!receiveRecordAsyncResult.IsCompleted)
		{
			receiveRecordAsyncResult.AsyncWaitHandle.WaitOne();
		}
		if (receiveRecordAsyncResult.CompletedWithError)
		{
			throw receiveRecordAsyncResult.AsyncException;
		}
		byte[] resultingBuffer = receiveRecordAsyncResult.ResultingBuffer;
		record_processing.Set();
		return resultingBuffer;
	}

	public byte[] ReceiveRecord(Stream record)
	{
		if (context.ReceivedConnectionEnd)
		{
			throw new TlsException(AlertDescription.InternalError, "The session is finished and it's no longer valid.");
		}
		record_processing.Reset();
		byte[] array = new byte[1];
		if (record.Read(array, 0, array.Length) == 0)
		{
			return null;
		}
		int num = array[0];
		ContentType contentType = (ContentType)num;
		byte[] array2 = ReadRecordBuffer(num, record);
		if (array2 == null)
		{
			return null;
		}
		if ((contentType != ContentType.Alert || array2.Length != 2) && Context.Read != null && Context.Read.Cipher != null)
		{
			array2 = decryptRecordFragment(contentType, array2);
		}
		switch (contentType)
		{
		case ContentType.Alert:
			ProcessAlert((AlertLevel)array2[0], (AlertDescription)array2[1]);
			if (record.CanSeek)
			{
				record.SetLength(0L);
			}
			array2 = null;
			break;
		case ContentType.ChangeCipherSpec:
			ProcessChangeCipherSpec();
			break;
		case ContentType.Handshake:
		{
			TlsStream tlsStream = new TlsStream(array2);
			while (!tlsStream.EOF)
			{
				ProcessHandshakeMessage(tlsStream);
			}
			break;
		}
		case (ContentType)128:
			context.HandshakeMessages.Write(array2);
			break;
		default:
			throw new TlsException(AlertDescription.UnexpectedMessage, "Unknown record received from server.");
		case ContentType.ApplicationData:
			break;
		}
		record_processing.Set();
		return array2;
	}

	private byte[] ReadRecordBuffer(int contentType, Stream record)
	{
		if (!Enum.IsDefined(typeof(ContentType), (ContentType)contentType))
		{
			throw new TlsException(AlertDescription.DecodeError);
		}
		byte[] array = new byte[4];
		if (record.Read(array, 0, 4) != 4)
		{
			throw new TlsException("buffer underrun");
		}
		short num = (short)((array[0] << 8) | array[1]);
		short num2 = (short)((array[2] << 8) | array[3]);
		if (record.CanSeek && num2 + 5 > record.Length)
		{
			return null;
		}
		int i = 0;
		byte[] array2 = new byte[num2];
		int num3;
		for (; i != num2; i += num3)
		{
			num3 = record.Read(array2, i, array2.Length - i);
			if (num3 == 0)
			{
				throw new TlsException(AlertDescription.CloseNotify, "Received 0 bytes from stream. It must be closed.");
			}
		}
		if (num != context.Protocol && context.ProtocolNegotiated)
		{
			throw new TlsException(AlertDescription.ProtocolVersion, "Invalid protocol version on message received");
		}
		return array2;
	}

	private void ProcessAlert(AlertLevel alertLevel, AlertDescription alertDesc)
	{
		if (alertLevel != AlertLevel.Warning && alertLevel == AlertLevel.Fatal)
		{
			throw new TlsException(alertLevel, alertDesc);
		}
		if (alertDesc == AlertDescription.CloseNotify)
		{
			context.ReceivedConnectionEnd = true;
		}
	}

	internal void SendAlert(ref Exception ex)
	{
		Alert alert = ((ex is TlsException ex2) ? ex2.Alert : new Alert(AlertDescription.InternalError));
		try
		{
			SendAlert(alert);
		}
		catch (Exception innerException)
		{
			ex = new IOException($"Error while sending TLS Alert ({alert.Level}:{alert.Description}): {ex}", innerException);
		}
	}

	public void SendAlert(AlertDescription description)
	{
		SendAlert(new Alert(description));
	}

	public void SendAlert(AlertLevel level, AlertDescription description)
	{
		SendAlert(new Alert(level, description));
	}

	public void SendAlert(Alert alert)
	{
		AlertLevel alertLevel;
		AlertDescription alertDescription;
		bool flag;
		if (alert == null)
		{
			alertLevel = AlertLevel.Fatal;
			alertDescription = AlertDescription.InternalError;
			flag = true;
		}
		else
		{
			alertLevel = alert.Level;
			alertDescription = alert.Description;
			flag = alert.IsCloseNotify;
		}
		SendRecord(ContentType.Alert, new byte[2]
		{
			(byte)alertLevel,
			(byte)alertDescription
		});
		if (flag)
		{
			context.SentConnectionEnd = true;
		}
	}

	public void SendChangeCipherSpec()
	{
		SendRecord(ContentType.ChangeCipherSpec, new byte[1] { 1 });
		Context context = this.context;
		context.WriteSequenceNumber = 0uL;
		if (context is ClientContext)
		{
			context.StartSwitchingSecurityParameters(client: true);
		}
		else
		{
			context.EndSwitchingSecurityParameters(client: false);
		}
	}

	public void SendChangeCipherSpec(Stream recordStream)
	{
		byte[] array = EncodeRecord(ContentType.ChangeCipherSpec, new byte[1] { 1 });
		recordStream.Write(array, 0, array.Length);
		Context context = this.context;
		context.WriteSequenceNumber = 0uL;
		if (context is ClientContext)
		{
			context.StartSwitchingSecurityParameters(client: true);
		}
		else
		{
			context.EndSwitchingSecurityParameters(client: false);
		}
	}

	public IAsyncResult BeginSendChangeCipherSpec(AsyncCallback callback, object state)
	{
		return BeginSendRecord(ContentType.ChangeCipherSpec, new byte[1] { 1 }, callback, state);
	}

	public void EndSendChangeCipherSpec(IAsyncResult asyncResult)
	{
		EndSendRecord(asyncResult);
		Context context = this.context;
		context.WriteSequenceNumber = 0uL;
		if (context is ClientContext)
		{
			context.StartSwitchingSecurityParameters(client: true);
		}
		else
		{
			context.EndSwitchingSecurityParameters(client: false);
		}
	}

	public IAsyncResult BeginSendRecord(HandshakeType handshakeType, AsyncCallback callback, object state)
	{
		HandshakeMessage message = GetMessage(handshakeType);
		message.Process();
		SendRecordAsyncResult sendRecordAsyncResult = new SendRecordAsyncResult(callback, state, message);
		BeginSendRecord(message.ContentType, message.EncodeMessage(), InternalSendRecordCallback, sendRecordAsyncResult);
		return sendRecordAsyncResult;
	}

	private void InternalSendRecordCallback(IAsyncResult ar)
	{
		SendRecordAsyncResult sendRecordAsyncResult = ar.AsyncState as SendRecordAsyncResult;
		try
		{
			EndSendRecord(ar);
			sendRecordAsyncResult.Message.Update();
			sendRecordAsyncResult.Message.Reset();
			sendRecordAsyncResult.SetComplete();
		}
		catch (Exception complete)
		{
			sendRecordAsyncResult.SetComplete(complete);
		}
	}

	public IAsyncResult BeginSendRecord(ContentType contentType, byte[] recordData, AsyncCallback callback, object state)
	{
		if (context.SentConnectionEnd)
		{
			throw new TlsException(AlertDescription.InternalError, "The session is finished and it's no longer valid.");
		}
		byte[] array = EncodeRecord(contentType, recordData);
		return innerStream.BeginWrite(array, 0, array.Length, callback, state);
	}

	public void EndSendRecord(IAsyncResult asyncResult)
	{
		if (asyncResult is SendRecordAsyncResult)
		{
			SendRecordAsyncResult sendRecordAsyncResult = asyncResult as SendRecordAsyncResult;
			if (!sendRecordAsyncResult.IsCompleted)
			{
				sendRecordAsyncResult.AsyncWaitHandle.WaitOne();
			}
			if (sendRecordAsyncResult.CompletedWithError)
			{
				throw sendRecordAsyncResult.AsyncException;
			}
		}
		else
		{
			innerStream.EndWrite(asyncResult);
		}
	}

	public void SendRecord(ContentType contentType, byte[] recordData)
	{
		IAsyncResult asyncResult = BeginSendRecord(contentType, recordData, null, null);
		EndSendRecord(asyncResult);
	}

	public byte[] EncodeRecord(ContentType contentType, byte[] recordData)
	{
		return EncodeRecord(contentType, recordData, 0, recordData.Length);
	}

	public byte[] EncodeRecord(ContentType contentType, byte[] recordData, int offset, int count)
	{
		if (context.SentConnectionEnd)
		{
			throw new TlsException(AlertDescription.InternalError, "The session is finished and it's no longer valid.");
		}
		TlsStream tlsStream = new TlsStream();
		short num;
		for (int i = offset; i < offset + count; i += num)
		{
			num = 0;
			num = (short)((count + offset - i <= 16384) ? ((short)(count + offset - i)) : 16384);
			byte[] array = new byte[num];
			Buffer.BlockCopy(recordData, i, array, 0, num);
			if (Context.Write != null && Context.Write.Cipher != null)
			{
				array = encryptRecordFragment(contentType, array);
			}
			tlsStream.Write((byte)contentType);
			tlsStream.Write(context.Protocol);
			tlsStream.Write((short)array.Length);
			tlsStream.Write(array);
		}
		return tlsStream.ToArray();
	}

	public byte[] EncodeHandshakeRecord(HandshakeType handshakeType)
	{
		HandshakeMessage message = GetMessage(handshakeType);
		message.Process();
		byte[] result = EncodeRecord(message.ContentType, message.EncodeMessage());
		message.Update();
		message.Reset();
		return result;
	}

	private byte[] encryptRecordFragment(ContentType contentType, byte[] fragment)
	{
		byte[] array = null;
		array = ((!(Context is ClientContext)) ? context.Write.Cipher.ComputeServerRecordMAC(contentType, fragment) : context.Write.Cipher.ComputeClientRecordMAC(contentType, fragment));
		byte[] result = context.Write.Cipher.EncryptRecord(fragment, array);
		context.WriteSequenceNumber++;
		return result;
	}

	private byte[] decryptRecordFragment(ContentType contentType, byte[] fragment)
	{
		byte[] dcrFragment = null;
		byte[] dcrMAC = null;
		try
		{
			context.Read.Cipher.DecryptRecord(fragment, out dcrFragment, out dcrMAC);
		}
		catch
		{
			if (context is ServerContext)
			{
				Context.RecordProtocol.SendAlert(AlertDescription.DecryptionFailed);
			}
			throw;
		}
		byte[] array = null;
		array = ((!(Context is ClientContext)) ? context.Read.Cipher.ComputeClientRecordMAC(contentType, dcrFragment) : context.Read.Cipher.ComputeServerRecordMAC(contentType, dcrFragment));
		if (!Compare(array, dcrMAC))
		{
			throw new TlsException(AlertDescription.BadRecordMAC, "Bad record MAC");
		}
		context.ReadSequenceNumber++;
		return dcrFragment;
	}

	private bool Compare(byte[] array1, byte[] array2)
	{
		if (array1 == null)
		{
			return array2 == null;
		}
		if (array2 == null)
		{
			return false;
		}
		if (array1.Length != array2.Length)
		{
			return false;
		}
		for (int i = 0; i < array1.Length; i++)
		{
			if (array1[i] != array2[i])
			{
				return false;
			}
		}
		return true;
	}
}
