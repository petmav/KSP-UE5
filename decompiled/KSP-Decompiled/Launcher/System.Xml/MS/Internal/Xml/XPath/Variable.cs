using System.Xml.XPath;

namespace MS.Internal.Xml.XPath;

internal class Variable : AstNode
{
	private string localname;

	private string prefix;

	public override AstType Type => AstType.Variable;

	public override XPathResultType ReturnType => XPathResultType.Any;

	public string Localname => localname;

	public string Prefix => prefix;

	public Variable(string name, string prefix)
	{
		localname = name;
		this.prefix = prefix;
	}
}
