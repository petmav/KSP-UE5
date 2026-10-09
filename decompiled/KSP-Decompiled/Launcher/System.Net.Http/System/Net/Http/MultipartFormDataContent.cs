using System.Net.Http.Headers;

namespace System.Net.Http;

public class MultipartFormDataContent : MultipartContent
{
	public MultipartFormDataContent()
		: base("form-data")
	{
	}

	public MultipartFormDataContent(string boundary)
		: base("form-data", boundary)
	{
	}

	public override void Add(HttpContent content)
	{
		base.Add(content);
		AddContentDisposition(content, null, null);
	}

	public void Add(HttpContent content, string name)
	{
		base.Add(content);
		if (string.IsNullOrWhiteSpace(name))
		{
			throw new ArgumentException("name");
		}
		AddContentDisposition(content, name, null);
	}

	public void Add(HttpContent content, string name, string fileName)
	{
		base.Add(content);
		if (string.IsNullOrWhiteSpace(name))
		{
			throw new ArgumentException("name");
		}
		if (string.IsNullOrWhiteSpace(fileName))
		{
			throw new ArgumentException("fileName");
		}
		AddContentDisposition(content, name, fileName);
	}

	private void AddContentDisposition(HttpContent content, string name, string fileName)
	{
		HttpContentHeaders httpContentHeaders = content.Headers;
		if (httpContentHeaders.ContentDisposition == null)
		{
			httpContentHeaders.ContentDisposition = new ContentDispositionHeaderValue("form-data")
			{
				Name = name,
				FileName = fileName,
				FileNameStar = fileName
			};
		}
	}
}
