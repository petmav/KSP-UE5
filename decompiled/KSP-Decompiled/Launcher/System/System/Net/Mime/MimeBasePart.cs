using System.Collections.Specialized;
using System.Net.Mail;
using System.Text;

namespace System.Net.Mime;

internal class MimeBasePart
{
	internal class MimePartAsyncResult : LazyAsyncResult
	{
		internal MimePartAsyncResult(MimeBasePart part, object state, AsyncCallback callback)
			: base(part, state, callback)
		{
		}
	}

	protected ContentType contentType;

	protected ContentDisposition contentDisposition;

	private HeaderCollection headers;

	internal const string defaultCharSet = "utf-8";

	internal string ContentID
	{
		get
		{
			return Headers[MailHeaderInfo.GetString(MailHeaderID.ContentID)];
		}
		set
		{
			if (string.IsNullOrEmpty(value))
			{
				Headers.Remove(MailHeaderInfo.GetString(MailHeaderID.ContentID));
			}
			else
			{
				Headers[MailHeaderInfo.GetString(MailHeaderID.ContentID)] = value;
			}
		}
	}

	internal string ContentLocation
	{
		get
		{
			return Headers[MailHeaderInfo.GetString(MailHeaderID.ContentLocation)];
		}
		set
		{
			if (string.IsNullOrEmpty(value))
			{
				Headers.Remove(MailHeaderInfo.GetString(MailHeaderID.ContentLocation));
			}
			else
			{
				Headers[MailHeaderInfo.GetString(MailHeaderID.ContentLocation)] = value;
			}
		}
	}

	internal NameValueCollection Headers
	{
		get
		{
			if (headers == null)
			{
				headers = new HeaderCollection();
			}
			if (contentType == null)
			{
				contentType = new ContentType();
			}
			contentType.PersistIfNeeded(headers, forcePersist: false);
			if (contentDisposition != null)
			{
				contentDisposition.PersistIfNeeded(headers, forcePersist: false);
			}
			return headers;
		}
	}

	internal ContentType ContentType
	{
		get
		{
			if (contentType == null)
			{
				contentType = new ContentType();
			}
			return contentType;
		}
		set
		{
			if (value == null)
			{
				throw new ArgumentNullException("value");
			}
			contentType = value;
			contentType.PersistIfNeeded((HeaderCollection)Headers, forcePersist: true);
		}
	}

	internal MimeBasePart()
	{
	}

	internal static bool ShouldUseBase64Encoding(Encoding encoding)
	{
		if (encoding == Encoding.Unicode || encoding == Encoding.UTF8 || encoding == Encoding.UTF32 || encoding == Encoding.BigEndianUnicode)
		{
			return true;
		}
		return false;
	}

	internal static string EncodeHeaderValue(string value, Encoding encoding, bool base64Encoding)
	{
		return EncodeHeaderValue(value, encoding, base64Encoding, 0);
	}

	internal static string EncodeHeaderValue(string value, Encoding encoding, bool base64Encoding, int headerLength)
	{
		if (IsAscii(value, permitCROrLF: false))
		{
			return value;
		}
		if (encoding == null)
		{
			encoding = Encoding.GetEncoding("utf-8");
		}
		IEncodableStream encoderForHeader = new EncodedStreamFactory().GetEncoderForHeader(encoding, base64Encoding, headerLength);
		byte[] bytes = encoding.GetBytes(value);
		encoderForHeader.EncodeBytes(bytes, 0, bytes.Length);
		return encoderForHeader.GetEncodedString();
	}

	internal static string DecodeHeaderValue(string value)
	{
		if (value == null || value.Length == 0)
		{
			return string.Empty;
		}
		string text = string.Empty;
		string[] array = value.Split(new char[3] { '\r', '\n', ' ' }, StringSplitOptions.RemoveEmptyEntries);
		for (int i = 0; i < array.Length; i++)
		{
			string[] array2 = array[i].Split('?');
			if (array2.Length != 5 || array2[0] != "=" || array2[4] != "=")
			{
				return value;
			}
			string name = array2[1];
			bool useBase64Encoding = array2[2] == "B";
			byte[] bytes = Encoding.ASCII.GetBytes(array2[3]);
			int count = new EncodedStreamFactory().GetEncoderForHeader(Encoding.GetEncoding(name), useBase64Encoding, 0).DecodeBytes(bytes, 0, bytes.Length);
			Encoding encoding = Encoding.GetEncoding(name);
			text += encoding.GetString(bytes, 0, count);
		}
		return text;
	}

	internal static Encoding DecodeEncoding(string value)
	{
		if (value == null || value.Length == 0)
		{
			return null;
		}
		string[] array = value.Split('?', '\r', '\n');
		if (array.Length < 5 || array[0] != "=" || array[4] != "=")
		{
			return null;
		}
		return Encoding.GetEncoding(array[1]);
	}

	internal static bool IsAscii(string value, bool permitCROrLF)
	{
		if (value == null)
		{
			throw new ArgumentNullException("value");
		}
		foreach (char c in value)
		{
			if (c > '\u007f')
			{
				return false;
			}
			if (!permitCROrLF && (c == '\r' || c == '\n'))
			{
				return false;
			}
		}
		return true;
	}

	internal static bool IsAnsi(string value, bool permitCROrLF)
	{
		if (value == null)
		{
			throw new ArgumentNullException("value");
		}
		foreach (char c in value)
		{
			if (c > 'ÿ')
			{
				return false;
			}
			if (!permitCROrLF && (c == '\r' || c == '\n'))
			{
				return false;
			}
		}
		return true;
	}

	internal void PrepareHeaders(bool allowUnicode)
	{
		contentType.PersistIfNeeded((HeaderCollection)Headers, forcePersist: false);
		headers.InternalSet(MailHeaderInfo.GetString(MailHeaderID.ContentType), contentType.Encode(allowUnicode));
		if (contentDisposition != null)
		{
			contentDisposition.PersistIfNeeded((HeaderCollection)Headers, forcePersist: false);
			headers.InternalSet(MailHeaderInfo.GetString(MailHeaderID.ContentDisposition), contentDisposition.Encode(allowUnicode));
		}
	}

	internal virtual void Send(BaseWriter writer, bool allowUnicode)
	{
		throw new NotImplementedException();
	}

	internal virtual IAsyncResult BeginSend(BaseWriter writer, AsyncCallback callback, bool allowUnicode, object state)
	{
		throw new NotImplementedException();
	}

	internal void EndSend(IAsyncResult asyncResult)
	{
		if (asyncResult == null)
		{
			throw new ArgumentNullException("asyncResult");
		}
		LazyAsyncResult lazyAsyncResult = asyncResult as MimePartAsyncResult;
		if (lazyAsyncResult == null || lazyAsyncResult.AsyncObject != this)
		{
			throw new ArgumentException(global::SR.GetString("The IAsyncResult object was not returned from the corresponding asynchronous method on this class."), "asyncResult");
		}
		if (lazyAsyncResult.EndCalled)
		{
			throw new InvalidOperationException(global::SR.GetString("{0} can only be called once for each asynchronous operation.", "EndSend"));
		}
		lazyAsyncResult.InternalWaitForCompletion();
		lazyAsyncResult.EndCalled = true;
		if (lazyAsyncResult.Result is Exception)
		{
			throw (Exception)lazyAsyncResult.Result;
		}
	}
}
