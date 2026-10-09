using System.Xml.Xsl.Qil;

namespace System.Xml.Xsl.Xslt;

internal class Text : XslNode
{
	public readonly SerializationHints Hints;

	public Text(string data, SerializationHints hints, XslVersion xslVer)
		: base(XslNodeType.Text, null, data, xslVer)
	{
		Hints = hints;
	}
}
