using System.Xml.XPath;

namespace MS.Internal.Xml.XPath;

internal class Axis : AstNode
{
	public enum AxisType
	{
		Ancestor,
		AncestorOrSelf,
		Attribute,
		Child,
		Descendant,
		DescendantOrSelf,
		Following,
		FollowingSibling,
		Namespace,
		Parent,
		Preceding,
		PrecedingSibling,
		Self,
		None
	}

	private AxisType axisType;

	private AstNode input;

	private string prefix;

	private string name;

	private XPathNodeType nodeType;

	protected bool abbrAxis;

	private string urn = string.Empty;

	public override AstType Type => AstType.Axis;

	public override XPathResultType ReturnType => XPathResultType.NodeSet;

	public AstNode Input
	{
		get
		{
			return input;
		}
		set
		{
			input = value;
		}
	}

	public string Prefix => prefix;

	public string Name => name;

	public XPathNodeType NodeType => nodeType;

	public AxisType TypeOfAxis => axisType;

	public bool AbbrAxis => abbrAxis;

	public string Urn
	{
		get
		{
			return urn;
		}
		set
		{
			urn = value;
		}
	}

	public Axis(AxisType axisType, AstNode input, string prefix, string name, XPathNodeType nodetype)
	{
		this.axisType = axisType;
		this.input = input;
		this.prefix = prefix;
		this.name = name;
		nodeType = nodetype;
	}

	public Axis(AxisType axisType, AstNode input)
		: this(axisType, input, string.Empty, string.Empty, XPathNodeType.All)
	{
		abbrAxis = true;
	}
}
