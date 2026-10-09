using System.Xml;
using System.Xml.XPath;
using System.Xml.Xsl;

namespace MS.Internal.Xml.XPath;

internal abstract class BaseAxisQuery : Query
{
	internal Query qyInput;

	private bool nameTest;

	private string name;

	private string prefix;

	private string nsUri;

	private XPathNodeType typeTest;

	protected XPathNavigator currentNode;

	protected int position;

	protected string Name => name;

	protected string Prefix => prefix;

	protected string Namespace => nsUri;

	protected bool NameTest => nameTest;

	protected XPathNodeType TypeTest => typeTest;

	public override int CurrentPosition => position;

	public override XPathNavigator Current => currentNode;

	public override double XsltDefaultPriority
	{
		get
		{
			if (qyInput.GetType() != typeof(ContextQuery))
			{
				return 0.5;
			}
			if (name.Length != 0)
			{
				return 0.0;
			}
			if (prefix.Length != 0)
			{
				return -0.25;
			}
			return -0.5;
		}
	}

	public override XPathResultType StaticType => XPathResultType.NodeSet;

	protected BaseAxisQuery(Query qyInput)
	{
		name = string.Empty;
		prefix = string.Empty;
		nsUri = string.Empty;
		this.qyInput = qyInput;
	}

	protected BaseAxisQuery(Query qyInput, string name, string prefix, XPathNodeType typeTest)
	{
		this.qyInput = qyInput;
		this.name = name;
		this.prefix = prefix;
		this.typeTest = typeTest;
		nameTest = prefix.Length != 0 || name.Length != 0;
		nsUri = string.Empty;
	}

	protected BaseAxisQuery(BaseAxisQuery other)
		: base(other)
	{
		qyInput = Query.Clone(other.qyInput);
		name = other.name;
		prefix = other.prefix;
		nsUri = other.nsUri;
		typeTest = other.typeTest;
		nameTest = other.nameTest;
		position = other.position;
		currentNode = other.currentNode;
	}

	public override void Reset()
	{
		position = 0;
		currentNode = null;
		qyInput.Reset();
	}

	public override void SetXsltContext(XsltContext context)
	{
		nsUri = context.LookupNamespace(prefix);
		qyInput.SetXsltContext(context);
	}

	public virtual bool matches(XPathNavigator e)
	{
		if (TypeTest == e.NodeType || TypeTest == XPathNodeType.All || (TypeTest == XPathNodeType.Text && (e.NodeType == XPathNodeType.Whitespace || e.NodeType == XPathNodeType.SignificantWhitespace)))
		{
			if (!NameTest)
			{
				return true;
			}
			if ((name.Equals(e.LocalName) || name.Length == 0) && nsUri.Equals(e.NamespaceURI))
			{
				return true;
			}
		}
		return false;
	}

	public override object Evaluate(XPathNodeIterator nodeIterator)
	{
		ResetCount();
		Reset();
		qyInput.Evaluate(nodeIterator);
		return this;
	}

	public override void PrintQuery(XmlWriter w)
	{
		w.WriteStartElement(GetType().Name);
		if (NameTest)
		{
			w.WriteAttributeString("name", (Prefix.Length != 0) ? (Prefix + ":" + Name) : Name);
		}
		if (TypeTest != XPathNodeType.Element)
		{
			w.WriteAttributeString("nodeType", TypeTest.ToString());
		}
		qyInput.PrintQuery(w);
		w.WriteEndElement();
	}
}
