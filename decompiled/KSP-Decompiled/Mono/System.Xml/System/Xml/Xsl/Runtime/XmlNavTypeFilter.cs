using System.Xml.XPath;

namespace System.Xml.Xsl.Runtime;

internal class XmlNavTypeFilter : XmlNavigatorFilter
{
	private static XmlNavigatorFilter[] TypeFilters;

	private XPathNodeType nodeType;

	private int mask;

	static XmlNavTypeFilter()
	{
		TypeFilters = new XmlNavigatorFilter[9];
		TypeFilters[1] = new XmlNavTypeFilter(XPathNodeType.Element);
		TypeFilters[4] = new XmlNavTypeFilter(XPathNodeType.Text);
		TypeFilters[7] = new XmlNavTypeFilter(XPathNodeType.ProcessingInstruction);
		TypeFilters[8] = new XmlNavTypeFilter(XPathNodeType.Comment);
	}

	public static XmlNavigatorFilter Create(XPathNodeType nodeType)
	{
		return TypeFilters[(int)nodeType];
	}

	private XmlNavTypeFilter(XPathNodeType nodeType)
	{
		this.nodeType = nodeType;
		mask = XPathNavigator.GetContentKindMask(nodeType);
	}

	public override bool MoveToContent(XPathNavigator navigator)
	{
		return navigator.MoveToChild(nodeType);
	}

	public override bool MoveToNextContent(XPathNavigator navigator)
	{
		return navigator.MoveToNext(nodeType);
	}

	public override bool MoveToFollowingSibling(XPathNavigator navigator)
	{
		return navigator.MoveToNext(nodeType);
	}

	public override bool MoveToPreviousSibling(XPathNavigator navigator)
	{
		return navigator.MoveToPrevious(nodeType);
	}

	public override bool MoveToFollowing(XPathNavigator navigator, XPathNavigator navEnd)
	{
		return navigator.MoveToFollowing(nodeType, navEnd);
	}

	public override bool IsFiltered(XPathNavigator navigator)
	{
		return ((1 << (int)navigator.NodeType) & mask) == 0;
	}
}
