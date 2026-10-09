using System.IO;

namespace System.Net;

internal class WebAsyncResult : SimpleAsyncResult
{
	private int nbytes;

	private IAsyncResult innerAsyncResult;

	private HttpWebResponse response;

	private Stream writeStream;

	private byte[] buffer;

	private int offset;

	private int size;

	public bool EndCalled;

	public bool AsyncWriteAll;

	public HttpWebRequest AsyncObject;

	internal int NBytes
	{
		get
		{
			return nbytes;
		}
		set
		{
			nbytes = value;
		}
	}

	internal IAsyncResult InnerAsyncResult
	{
		get
		{
			return innerAsyncResult;
		}
		set
		{
			innerAsyncResult = value;
		}
	}

	internal Stream WriteStream => writeStream;

	internal HttpWebResponse Response => response;

	internal byte[] Buffer => buffer;

	internal int Offset => offset;

	internal int Size => size;

	public WebAsyncResult(AsyncCallback cb, object state)
		: base(cb, state)
	{
	}

	public WebAsyncResult(HttpWebRequest request, AsyncCallback cb, object state)
		: base(cb, state)
	{
		AsyncObject = request;
	}

	public WebAsyncResult(AsyncCallback cb, object state, byte[] buffer, int offset, int size)
		: base(cb, state)
	{
		this.buffer = buffer;
		this.offset = offset;
		this.size = size;
	}

	internal void Reset()
	{
		nbytes = 0;
		response = null;
		buffer = null;
		offset = 0;
		size = 0;
		Reset_internal();
	}

	internal void SetCompleted(bool synch, int nbytes)
	{
		this.nbytes = nbytes;
		SetCompleted_internal(synch);
	}

	internal void SetCompleted(bool synch, Stream writeStream)
	{
		this.writeStream = writeStream;
		SetCompleted_internal(synch);
	}

	internal void SetCompleted(bool synch, HttpWebResponse response)
	{
		this.response = response;
		SetCompleted_internal(synch);
	}

	internal void DoCallback()
	{
		DoCallback_internal();
	}
}
