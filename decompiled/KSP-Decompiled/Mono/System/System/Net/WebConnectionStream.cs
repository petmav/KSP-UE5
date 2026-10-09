using System.IO;
using System.Net.Sockets;
using System.Text;
using System.Threading;

namespace System.Net;

internal class WebConnectionStream : Stream
{
	private static byte[] crlf = new byte[2] { 13, 10 };

	private bool isRead;

	private WebConnection cnc;

	private HttpWebRequest request;

	private byte[] readBuffer;

	private int readBufferOffset;

	private int readBufferSize;

	private int stream_length;

	private long contentLength;

	private long totalRead;

	internal long totalWritten;

	private bool nextReadCalled;

	private int pendingReads;

	private int pendingWrites;

	private ManualResetEvent pending;

	private bool allowBuffering;

	private bool sendChunked;

	private MemoryStream writeBuffer;

	private bool requestWritten;

	private byte[] headers;

	private bool disposed;

	private bool headersSent;

	private object locker = new object();

	private bool initRead;

	private bool read_eof;

	private bool complete_request_written;

	private int read_timeout;

	private int write_timeout;

	private AsyncCallback cb_wrapper;

	internal bool IgnoreIOErrors;

	internal HttpWebRequest Request => request;

	internal WebConnection Connection => cnc;

	public override bool CanTimeout => true;

	public override int ReadTimeout
	{
		get
		{
			return read_timeout;
		}
		set
		{
			if (value < -1)
			{
				throw new ArgumentOutOfRangeException("value");
			}
			read_timeout = value;
		}
	}

	public override int WriteTimeout
	{
		get
		{
			return write_timeout;
		}
		set
		{
			if (value < -1)
			{
				throw new ArgumentOutOfRangeException("value");
			}
			write_timeout = value;
		}
	}

	internal bool CompleteRequestWritten => complete_request_written;

	internal bool SendChunked
	{
		set
		{
			sendChunked = value;
		}
	}

	internal byte[] ReadBuffer
	{
		set
		{
			readBuffer = value;
		}
	}

	internal int ReadBufferOffset
	{
		set
		{
			readBufferOffset = value;
		}
	}

	internal int ReadBufferSize
	{
		set
		{
			readBufferSize = value;
		}
	}

	internal byte[] WriteBuffer => writeBuffer.GetBuffer();

	internal int WriteBufferLength
	{
		get
		{
			if (writeBuffer == null)
			{
				return -1;
			}
			return (int)writeBuffer.Length;
		}
	}

	internal bool RequestWritten => requestWritten;

	internal bool GetResponseOnClose { get; set; }

	public override bool CanSeek => false;

	public override bool CanRead
	{
		get
		{
			if (!disposed)
			{
				return isRead;
			}
			return false;
		}
	}

	public override bool CanWrite
	{
		get
		{
			if (!disposed)
			{
				return !isRead;
			}
			return false;
		}
	}

	public override long Length
	{
		get
		{
			if (!isRead)
			{
				throw new NotSupportedException();
			}
			return stream_length;
		}
	}

	public override long Position
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

	public WebConnectionStream(WebConnection cnc, WebConnectionData data)
	{
		if (data == null)
		{
			throw new InvalidOperationException("data was not initialized");
		}
		if (data.Headers == null)
		{
			throw new InvalidOperationException("data.Headers was not initialized");
		}
		if (data.request == null)
		{
			throw new InvalidOperationException("data.request was not initialized");
		}
		isRead = true;
		cb_wrapper = ReadCallbackWrapper;
		pending = new ManualResetEvent(initialState: true);
		request = data.request;
		read_timeout = request.ReadWriteTimeout;
		write_timeout = read_timeout;
		this.cnc = cnc;
		string text = data.Headers["Transfer-Encoding"];
		bool num = text != null && text.IndexOf("chunked", StringComparison.OrdinalIgnoreCase) != -1;
		string text2 = data.Headers["Content-Length"];
		if (!num && text2 != null && text2 != "")
		{
			try
			{
				contentLength = int.Parse(text2);
				if (contentLength == 0L && !IsNtlmAuth())
				{
					ReadAll();
				}
			}
			catch
			{
				contentLength = long.MaxValue;
			}
		}
		else
		{
			contentLength = long.MaxValue;
		}
		if (!int.TryParse(text2, out stream_length))
		{
			stream_length = -1;
		}
	}

	public WebConnectionStream(WebConnection cnc, HttpWebRequest request)
	{
		read_timeout = request.ReadWriteTimeout;
		write_timeout = read_timeout;
		isRead = false;
		cb_wrapper = WriteCallbackWrapper;
		this.cnc = cnc;
		this.request = request;
		allowBuffering = request.InternalAllowBuffering;
		sendChunked = request.SendChunked;
		if (sendChunked)
		{
			pending = new ManualResetEvent(initialState: true);
		}
		else if (allowBuffering)
		{
			writeBuffer = new MemoryStream();
		}
	}

	private bool CheckAuthHeader(string headerName)
	{
		string text = cnc.Data.Headers[headerName];
		if (text != null)
		{
			return text.IndexOf("NTLM", StringComparison.Ordinal) != -1;
		}
		return false;
	}

	private bool IsNtlmAuth()
	{
		if (request.Proxy != null && !request.Proxy.IsBypassed(request.Address) && CheckAuthHeader("Proxy-Authenticate"))
		{
			return true;
		}
		return CheckAuthHeader("WWW-Authenticate");
	}

	internal void CheckResponseInBuffer()
	{
		if (contentLength > 0 && readBufferSize - readBufferOffset >= contentLength && !IsNtlmAuth())
		{
			ReadAll();
		}
	}

	internal void ForceCompletion()
	{
		if (!nextReadCalled)
		{
			if (contentLength == long.MaxValue)
			{
				contentLength = 0L;
			}
			nextReadCalled = true;
			cnc.NextRead();
		}
	}

	internal void CheckComplete()
	{
		if (!nextReadCalled && readBufferSize - readBufferOffset == contentLength)
		{
			nextReadCalled = true;
			cnc.NextRead();
		}
	}

	internal void ReadAll()
	{
		if (!isRead || read_eof || totalRead >= contentLength || nextReadCalled)
		{
			if (isRead && !nextReadCalled)
			{
				nextReadCalled = true;
				cnc.NextRead();
			}
			return;
		}
		if (!pending.WaitOne(ReadTimeout))
		{
			throw new WebException("The operation has timed out.", WebExceptionStatus.Timeout);
		}
		lock (locker)
		{
			if (totalRead >= contentLength)
			{
				return;
			}
			byte[] array = null;
			int num = readBufferSize - readBufferOffset;
			int num2;
			if (contentLength == long.MaxValue)
			{
				MemoryStream memoryStream = new MemoryStream();
				byte[] array2 = null;
				if (readBuffer != null && num > 0)
				{
					memoryStream.Write(readBuffer, readBufferOffset, num);
					if (readBufferSize >= 8192)
					{
						array2 = readBuffer;
					}
				}
				if (array2 == null)
				{
					array2 = new byte[8192];
				}
				int count;
				while ((count = cnc.Read(request, array2, 0, array2.Length)) != 0)
				{
					memoryStream.Write(array2, 0, count);
				}
				array = memoryStream.GetBuffer();
				num2 = (int)memoryStream.Length;
				contentLength = num2;
			}
			else
			{
				num2 = (int)(contentLength - totalRead);
				array = new byte[num2];
				if (readBuffer != null && num > 0)
				{
					if (num > num2)
					{
						num = num2;
					}
					Buffer.BlockCopy(readBuffer, readBufferOffset, array, 0, num);
				}
				int num3 = num2 - num;
				int num4 = -1;
				while (num3 > 0 && num4 != 0)
				{
					num4 = cnc.Read(request, array, num, num3);
					num3 -= num4;
					num += num4;
				}
			}
			readBuffer = array;
			readBufferOffset = 0;
			readBufferSize = num2;
			totalRead = 0L;
			nextReadCalled = true;
		}
		cnc.NextRead();
	}

	private void WriteCallbackWrapper(IAsyncResult r)
	{
		if (r is WebAsyncResult { AsyncWriteAll: not false })
		{
			return;
		}
		if (r.AsyncState != null)
		{
			WebAsyncResult webAsyncResult2 = (WebAsyncResult)r.AsyncState;
			webAsyncResult2.InnerAsyncResult = r;
			webAsyncResult2.DoCallback();
			return;
		}
		try
		{
			EndWrite(r);
		}
		catch
		{
		}
	}

	private void ReadCallbackWrapper(IAsyncResult r)
	{
		if (r.AsyncState != null)
		{
			WebAsyncResult obj = (WebAsyncResult)r.AsyncState;
			obj.InnerAsyncResult = r;
			obj.DoCallback();
			return;
		}
		try
		{
			EndRead(r);
		}
		catch
		{
		}
	}

	public override int Read(byte[] buffer, int offset, int size)
	{
		AsyncCallback callback = cb_wrapper;
		WebAsyncResult webAsyncResult = (WebAsyncResult)BeginRead(buffer, offset, size, callback, null);
		if (!webAsyncResult.IsCompleted && !webAsyncResult.WaitUntilComplete(ReadTimeout, exitContext: false))
		{
			nextReadCalled = true;
			cnc.Close(sendNext: true);
			throw new WebException("The operation has timed out.", WebExceptionStatus.Timeout);
		}
		return EndRead(webAsyncResult);
	}

	public override IAsyncResult BeginRead(byte[] buffer, int offset, int size, AsyncCallback cb, object state)
	{
		if (!isRead)
		{
			throw new NotSupportedException("this stream does not allow reading");
		}
		if (buffer == null)
		{
			throw new ArgumentNullException("buffer");
		}
		int num = buffer.Length;
		if (offset < 0 || num < offset)
		{
			throw new ArgumentOutOfRangeException("offset");
		}
		if (size < 0 || num - offset < size)
		{
			throw new ArgumentOutOfRangeException("size");
		}
		lock (locker)
		{
			pendingReads++;
			pending.Reset();
		}
		WebAsyncResult webAsyncResult = new WebAsyncResult(cb, state, buffer, offset, size);
		if (totalRead >= contentLength)
		{
			webAsyncResult.SetCompleted(synch: true, -1);
			webAsyncResult.DoCallback();
			return webAsyncResult;
		}
		int num2 = readBufferSize - readBufferOffset;
		if (num2 > 0)
		{
			int num3 = ((num2 > size) ? size : num2);
			Buffer.BlockCopy(readBuffer, readBufferOffset, buffer, offset, num3);
			readBufferOffset += num3;
			offset += num3;
			size -= num3;
			totalRead += num3;
			if (size == 0 || totalRead >= contentLength)
			{
				webAsyncResult.SetCompleted(synch: true, num3);
				webAsyncResult.DoCallback();
				return webAsyncResult;
			}
			webAsyncResult.NBytes = num3;
		}
		if (cb != null)
		{
			cb = cb_wrapper;
		}
		if (contentLength != long.MaxValue && contentLength - totalRead < size)
		{
			size = (int)(contentLength - totalRead);
		}
		if (!read_eof)
		{
			webAsyncResult.InnerAsyncResult = cnc.BeginRead(request, buffer, offset, size, cb, webAsyncResult);
		}
		else
		{
			webAsyncResult.SetCompleted(synch: true, webAsyncResult.NBytes);
			webAsyncResult.DoCallback();
		}
		return webAsyncResult;
	}

	public override int EndRead(IAsyncResult r)
	{
		WebAsyncResult webAsyncResult = (WebAsyncResult)r;
		if (webAsyncResult.EndCalled)
		{
			int nBytes = webAsyncResult.NBytes;
			if (nBytes < 0)
			{
				return 0;
			}
			return nBytes;
		}
		webAsyncResult.EndCalled = true;
		if (!webAsyncResult.IsCompleted)
		{
			int num = -1;
			try
			{
				num = cnc.EndRead(request, webAsyncResult);
			}
			catch (Exception e)
			{
				lock (locker)
				{
					pendingReads--;
					if (pendingReads == 0)
					{
						pending.Set();
					}
				}
				nextReadCalled = true;
				cnc.Close(sendNext: true);
				webAsyncResult.SetCompleted(synch: false, e);
				webAsyncResult.DoCallback();
				throw;
			}
			if (num < 0)
			{
				num = 0;
				read_eof = true;
			}
			totalRead += num;
			webAsyncResult.SetCompleted(synch: false, num + webAsyncResult.NBytes);
			webAsyncResult.DoCallback();
			if (num == 0)
			{
				contentLength = totalRead;
			}
		}
		lock (locker)
		{
			pendingReads--;
			if (pendingReads == 0)
			{
				pending.Set();
			}
		}
		if (totalRead >= contentLength && !nextReadCalled)
		{
			ReadAll();
		}
		int nBytes2 = webAsyncResult.NBytes;
		if (nBytes2 < 0)
		{
			return 0;
		}
		return nBytes2;
	}

	private void WriteAsyncCB(IAsyncResult r)
	{
		WebAsyncResult webAsyncResult = (WebAsyncResult)r.AsyncState;
		webAsyncResult.InnerAsyncResult = null;
		try
		{
			cnc.EndWrite(request, throwOnError: true, r);
			webAsyncResult.SetCompleted(synch: false, 0);
			if (!initRead)
			{
				initRead = true;
				cnc.InitRead();
			}
		}
		catch (Exception ex)
		{
			KillBuffer();
			nextReadCalled = true;
			cnc.Close(sendNext: true);
			if (ex is SocketException)
			{
				ex = new IOException("Error writing request", ex);
			}
			webAsyncResult.SetCompleted(synch: false, ex);
		}
		if (allowBuffering && !sendChunked && request.ContentLength > 0 && totalWritten == request.ContentLength)
		{
			complete_request_written = true;
		}
		webAsyncResult.DoCallback();
	}

	public override IAsyncResult BeginWrite(byte[] buffer, int offset, int size, AsyncCallback cb, object state)
	{
		if (request.Aborted)
		{
			throw new WebException("The request was canceled.", WebExceptionStatus.RequestCanceled);
		}
		if (isRead)
		{
			throw new NotSupportedException("this stream does not allow writing");
		}
		if (buffer == null)
		{
			throw new ArgumentNullException("buffer");
		}
		int num = buffer.Length;
		if (offset < 0 || num < offset)
		{
			throw new ArgumentOutOfRangeException("offset");
		}
		if (size < 0 || num - offset < size)
		{
			throw new ArgumentOutOfRangeException("size");
		}
		if (sendChunked)
		{
			lock (locker)
			{
				pendingWrites++;
				pending.Reset();
			}
		}
		WebAsyncResult webAsyncResult = new WebAsyncResult(cb, state);
		AsyncCallback cb2 = WriteAsyncCB;
		if (sendChunked)
		{
			requestWritten = true;
			string s = $"{size:X}\r\n";
			byte[] bytes = Encoding.ASCII.GetBytes(s);
			int num2 = 2 + size + bytes.Length;
			byte[] array = new byte[num2];
			Buffer.BlockCopy(bytes, 0, array, 0, bytes.Length);
			Buffer.BlockCopy(buffer, offset, array, bytes.Length, size);
			Buffer.BlockCopy(crlf, 0, array, bytes.Length + size, crlf.Length);
			if (allowBuffering)
			{
				if (writeBuffer == null)
				{
					writeBuffer = new MemoryStream();
				}
				writeBuffer.Write(buffer, offset, size);
				totalWritten += size;
			}
			buffer = array;
			offset = 0;
			size = num2;
		}
		else
		{
			CheckWriteOverflow(request.ContentLength, totalWritten, size);
			if (allowBuffering)
			{
				if (writeBuffer == null)
				{
					writeBuffer = new MemoryStream();
				}
				writeBuffer.Write(buffer, offset, size);
				totalWritten += size;
				if (request.ContentLength <= 0 || totalWritten < request.ContentLength)
				{
					webAsyncResult.SetCompleted(synch: true, 0);
					webAsyncResult.DoCallback();
					return webAsyncResult;
				}
				webAsyncResult.AsyncWriteAll = true;
				requestWritten = true;
				buffer = writeBuffer.GetBuffer();
				offset = 0;
				size = (int)totalWritten;
			}
		}
		try
		{
			webAsyncResult.InnerAsyncResult = cnc.BeginWrite(request, buffer, offset, size, cb2, webAsyncResult);
			if (webAsyncResult.InnerAsyncResult == null)
			{
				if (!webAsyncResult.IsCompleted)
				{
					webAsyncResult.SetCompleted(synch: true, 0);
				}
				webAsyncResult.DoCallback();
			}
		}
		catch (Exception)
		{
			if (!IgnoreIOErrors)
			{
				throw;
			}
			webAsyncResult.SetCompleted(synch: true, 0);
			webAsyncResult.DoCallback();
		}
		totalWritten += size;
		return webAsyncResult;
	}

	private void CheckWriteOverflow(long contentLength, long totalWritten, long size)
	{
		if (contentLength != -1)
		{
			long num = contentLength - totalWritten;
			if (size > num)
			{
				KillBuffer();
				nextReadCalled = true;
				cnc.Close(sendNext: true);
				throw new ProtocolViolationException("The number of bytes to be written is greater than the specified ContentLength.");
			}
		}
	}

	public override void EndWrite(IAsyncResult r)
	{
		if (r == null)
		{
			throw new ArgumentNullException("r");
		}
		if (!(r is WebAsyncResult webAsyncResult))
		{
			throw new ArgumentException("Invalid IAsyncResult");
		}
		if (webAsyncResult.EndCalled)
		{
			return;
		}
		if (sendChunked)
		{
			lock (locker)
			{
				pendingWrites--;
				if (pendingWrites <= 0)
				{
					pending.Set();
				}
			}
		}
		webAsyncResult.EndCalled = true;
		if (webAsyncResult.AsyncWriteAll)
		{
			webAsyncResult.WaitUntilComplete();
			if (webAsyncResult.GotException)
			{
				throw webAsyncResult.Exception;
			}
		}
		else if ((!allowBuffering || sendChunked) && webAsyncResult.GotException)
		{
			throw webAsyncResult.Exception;
		}
	}

	public override void Write(byte[] buffer, int offset, int size)
	{
		AsyncCallback callback = cb_wrapper;
		WebAsyncResult webAsyncResult = (WebAsyncResult)BeginWrite(buffer, offset, size, callback, null);
		if (!webAsyncResult.IsCompleted && !webAsyncResult.WaitUntilComplete(WriteTimeout, exitContext: false))
		{
			KillBuffer();
			nextReadCalled = true;
			cnc.Close(sendNext: true);
			throw new IOException("Write timed out.");
		}
		EndWrite(webAsyncResult);
	}

	public override void Flush()
	{
	}

	internal void SetHeadersAsync(bool setInternalLength, SimpleAsyncCallback callback)
	{
		SimpleAsyncResult.Run((SimpleAsyncResult r) => SetHeadersAsync(r, setInternalLength), callback);
	}

	private bool SetHeadersAsync(SimpleAsyncResult result, bool setInternalLength)
	{
		if (headersSent)
		{
			return false;
		}
		string method = request.Method;
		int num;
		switch (method)
		{
		default:
			num = ((method == "TRACE") ? 1 : 0);
			break;
		case "GET":
		case "CONNECT":
		case "HEAD":
			num = 1;
			break;
		}
		bool flag = (byte)num != 0;
		int num2;
		switch (method)
		{
		default:
			num2 = ((method == "UNLOCK") ? 1 : 0);
			break;
		case "PROPFIND":
		case "PROPPATCH":
		case "MKCOL":
		case "COPY":
		case "MOVE":
		case "LOCK":
			num2 = 1;
			break;
		}
		bool flag2 = (byte)num2 != 0;
		if (setInternalLength && !flag && writeBuffer != null)
		{
			request.InternalContentLength = writeBuffer.Length;
		}
		bool flag3 = !flag && (writeBuffer == null || request.ContentLength > -1);
		if (!(sendChunked || flag3 || flag || flag2))
		{
			return false;
		}
		headersSent = true;
		headers = request.GetRequestHeaders();
		return cnc.BeginWrite(request, headers, 0, headers.Length, delegate(IAsyncResult r)
		{
			try
			{
				cnc.EndWrite(request, throwOnError: true, r);
				if (!initRead)
				{
					initRead = true;
					cnc.InitRead();
				}
				long num3 = request.ContentLength;
				if (!sendChunked && num3 == 0L)
				{
					requestWritten = true;
				}
				result.SetCompleted(synch: false);
			}
			catch (WebException e)
			{
				result.SetCompleted(synch: false, e);
			}
			catch (Exception innerException)
			{
				result.SetCompleted(synch: false, new WebException("Error writing headers", WebExceptionStatus.SendFailure, WebExceptionInternalStatus.RequestFatal, innerException));
			}
		}, null) != null;
	}

	internal SimpleAsyncResult WriteRequestAsync(SimpleAsyncCallback callback)
	{
		SimpleAsyncResult simpleAsyncResult = WriteRequestAsync(callback);
		try
		{
			if (!WriteRequestAsync(simpleAsyncResult))
			{
				simpleAsyncResult.SetCompleted(synch: true);
			}
		}
		catch (Exception e)
		{
			simpleAsyncResult.SetCompleted(synch: true, e);
		}
		return simpleAsyncResult;
	}

	internal bool WriteRequestAsync(SimpleAsyncResult result)
	{
		if (requestWritten)
		{
			return false;
		}
		requestWritten = true;
		if (sendChunked || !allowBuffering || writeBuffer == null)
		{
			return false;
		}
		byte[] bytes = writeBuffer.GetBuffer();
		int length = (int)writeBuffer.Length;
		if (request.ContentLength != -1 && request.ContentLength < length)
		{
			nextReadCalled = true;
			cnc.Close(sendNext: true);
			throw new WebException("Specified Content-Length is less than the number of bytes to write", null, WebExceptionStatus.ServerProtocolViolation, null);
		}
		SetHeadersAsync(setInternalLength: true, delegate(SimpleAsyncResult inner)
		{
			if (inner.GotException)
			{
				result.SetCompleted(inner.CompletedSynchronouslyPeek, inner.Exception);
			}
			else if (cnc.Data.StatusCode != 0 && cnc.Data.StatusCode != 100)
			{
				result.SetCompleted(inner.CompletedSynchronouslyPeek);
			}
			else
			{
				if (!initRead)
				{
					initRead = true;
					cnc.InitRead();
				}
				if (length == 0)
				{
					complete_request_written = true;
					result.SetCompleted(inner.CompletedSynchronouslyPeek);
				}
				else
				{
					cnc.BeginWrite(request, bytes, 0, length, delegate(IAsyncResult r)
					{
						try
						{
							complete_request_written = cnc.EndWrite(request, throwOnError: false, r);
							result.SetCompleted(synch: false);
						}
						catch (Exception e)
						{
							result.SetCompleted(synch: false, e);
						}
					}, null);
				}
			}
		});
		return true;
	}

	internal void InternalClose()
	{
		disposed = true;
	}

	public override void Close()
	{
		if (GetResponseOnClose)
		{
			if (!disposed)
			{
				disposed = true;
				HttpWebResponse obj = (HttpWebResponse)request.GetResponse();
				obj.ReadAll();
				obj.Close();
			}
		}
		else if (sendChunked)
		{
			if (!disposed)
			{
				disposed = true;
				if (!pending.WaitOne(WriteTimeout))
				{
					throw new WebException("The operation has timed out.", WebExceptionStatus.Timeout);
				}
				byte[] bytes = Encoding.ASCII.GetBytes("0\r\n\r\n");
				string err_msg = null;
				cnc.Write(request, bytes, 0, bytes.Length, ref err_msg);
			}
		}
		else if (isRead)
		{
			if (!nextReadCalled)
			{
				CheckComplete();
				if (!nextReadCalled)
				{
					nextReadCalled = true;
					cnc.Close(sendNext: true);
				}
			}
		}
		else if (!allowBuffering)
		{
			complete_request_written = true;
			if (!initRead)
			{
				initRead = true;
				cnc.InitRead();
			}
		}
		else if (!disposed && !requestWritten)
		{
			long num = request.ContentLength;
			if (!sendChunked && num != -1 && totalWritten != num)
			{
				IOException innerException = new IOException("Cannot close the stream until all bytes are written");
				nextReadCalled = true;
				cnc.Close(sendNext: true);
				throw new WebException("Request was cancelled.", WebExceptionStatus.RequestCanceled, WebExceptionInternalStatus.RequestFatal, innerException);
			}
			disposed = true;
		}
	}

	internal void KillBuffer()
	{
		writeBuffer = null;
	}

	public override long Seek(long a, SeekOrigin b)
	{
		throw new NotSupportedException();
	}

	public override void SetLength(long a)
	{
		throw new NotSupportedException();
	}
}
