namespace System.Xml.Xsl.Qil;

internal class QilName : QilLiteral
{
	private string local;

	private string uri;

	private string prefix;

	public string LocalName
	{
		get
		{
			return local;
		}
		set
		{
			local = value;
		}
	}

	public string NamespaceUri
	{
		get
		{
			return uri;
		}
		set
		{
			uri = value;
		}
	}

	public string Prefix
	{
		get
		{
			return prefix;
		}
		set
		{
			prefix = value;
		}
	}

	public string QualifiedName
	{
		get
		{
			if (prefix.Length == 0)
			{
				return local;
			}
			return prefix + ":" + local;
		}
	}

	public QilName(QilNodeType nodeType, string local, string uri, string prefix)
		: base(nodeType, null)
	{
		LocalName = local;
		NamespaceUri = uri;
		Prefix = prefix;
		base.Value = this;
	}

	public override int GetHashCode()
	{
		return local.GetHashCode();
	}

	public override bool Equals(object other)
	{
		QilName qilName = other as QilName;
		if (qilName == null)
		{
			return false;
		}
		if (local == qilName.local)
		{
			return uri == qilName.uri;
		}
		return false;
	}

	public static bool operator ==(QilName a, QilName b)
	{
		if ((object)a == b)
		{
			return true;
		}
		if ((object)a == null || (object)b == null)
		{
			return false;
		}
		if (a.local == b.local)
		{
			return a.uri == b.uri;
		}
		return false;
	}

	public static bool operator !=(QilName a, QilName b)
	{
		return !(a == b);
	}

	public override string ToString()
	{
		if (prefix.Length == 0)
		{
			if (uri.Length == 0)
			{
				return local;
			}
			return "{" + uri + "}" + local;
		}
		return "{" + uri + "}" + prefix + ":" + local;
	}
}
