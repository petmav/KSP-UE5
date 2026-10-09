namespace System.Xml.Xsl.Xslt;

internal class OutputScopeManager
{
	public struct ScopeReord
	{
		public int scopeCount;

		public string prefix;

		public string nsUri;
	}

	private ScopeReord[] records = new ScopeReord[32];

	private int lastRecord;

	private int lastScopes;

	public OutputScopeManager()
	{
		Reset();
	}

	public void Reset()
	{
		records[0].prefix = null;
		records[0].nsUri = null;
		PushScope();
	}

	public void PushScope()
	{
		lastScopes++;
	}

	public void PopScope()
	{
		if (0 < lastScopes)
		{
			lastScopes--;
			return;
		}
		while (records[--lastRecord].scopeCount == 0)
		{
		}
		lastScopes = records[lastRecord].scopeCount;
		lastScopes--;
	}

	public void AddNamespace(string prefix, string uri)
	{
		AddRecord(prefix, uri);
	}

	private void AddRecord(string prefix, string uri)
	{
		records[lastRecord].scopeCount = lastScopes;
		lastRecord++;
		if (lastRecord == records.Length)
		{
			ScopeReord[] destinationArray = new ScopeReord[lastRecord * 2];
			Array.Copy(records, 0, destinationArray, 0, lastRecord);
			records = destinationArray;
		}
		lastScopes = 0;
		records[lastRecord].prefix = prefix;
		records[lastRecord].nsUri = uri;
	}

	public void InvalidateAllPrefixes()
	{
		if (records[lastRecord].prefix != null)
		{
			AddRecord(null, null);
		}
	}

	public void InvalidateNonDefaultPrefixes()
	{
		string text = LookupNamespace(string.Empty);
		if (text == null)
		{
			InvalidateAllPrefixes();
		}
		else if (records[lastRecord].prefix.Length != 0 || records[lastRecord - 1].prefix != null)
		{
			AddRecord(null, null);
			AddRecord(string.Empty, text);
		}
	}

	public string LookupNamespace(string prefix)
	{
		int num = lastRecord;
		while (records[num].prefix != null)
		{
			if (records[num].prefix == prefix)
			{
				return records[num].nsUri;
			}
			num--;
		}
		return null;
	}
}
