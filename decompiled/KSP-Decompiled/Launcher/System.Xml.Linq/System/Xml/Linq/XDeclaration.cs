using System.Text;

namespace System.Xml.Linq;

public class XDeclaration
{
	private string version;

	private string encoding;

	private string standalone;

	public string Encoding
	{
		get
		{
			return encoding;
		}
		set
		{
			encoding = value;
		}
	}

	public string Standalone
	{
		get
		{
			return standalone;
		}
		set
		{
			standalone = value;
		}
	}

	public string Version
	{
		get
		{
			return version;
		}
		set
		{
			version = value;
		}
	}

	public XDeclaration(string version, string encoding, string standalone)
	{
		this.version = version;
		this.encoding = encoding;
		this.standalone = standalone;
	}

	public XDeclaration(XDeclaration other)
	{
		if (other == null)
		{
			throw new ArgumentNullException("other");
		}
		version = other.version;
		encoding = other.encoding;
		standalone = other.standalone;
	}

	internal XDeclaration(XmlReader r)
	{
		version = r.GetAttribute("version");
		encoding = r.GetAttribute("encoding");
		standalone = r.GetAttribute("standalone");
		r.Read();
	}

	public override string ToString()
	{
		StringBuilder stringBuilder = new StringBuilder("<?xml");
		if (version != null)
		{
			stringBuilder.Append(" version=\"");
			stringBuilder.Append(version);
			stringBuilder.Append("\"");
		}
		if (encoding != null)
		{
			stringBuilder.Append(" encoding=\"");
			stringBuilder.Append(encoding);
			stringBuilder.Append("\"");
		}
		if (standalone != null)
		{
			stringBuilder.Append(" standalone=\"");
			stringBuilder.Append(standalone);
			stringBuilder.Append("\"");
		}
		stringBuilder.Append("?>");
		return stringBuilder.ToString();
	}
}
