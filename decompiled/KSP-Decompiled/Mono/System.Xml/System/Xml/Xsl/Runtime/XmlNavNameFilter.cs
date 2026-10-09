using System.Xml.XPath;

namespace System.Xml.Xsl.Runtime;

internal class XmlNavNameFilter : XmlNavigatorFilter
{
	private string localName;

	private string namespaceUri;

	public static XmlNavigatorFilter Create(string localName, string namespaceUri)
	{
		return new XmlNavNameFilter(localName, namespaceUri);
	}

	private XmlNavNameFilter(string localName, string namespaceUri)
	{
		this.localName = localName;
		this.namespaceUri = namespaceUri;
	}

	public override bool MoveToContent(XPathNavigator navigator)
	{
		return navigator.MoveToChild(localName, namespaceUri);
	}

	public override bool MoveToNextContent(XPathNavigator navigator)
	{
		return navigator.MoveToNext(localName, namespaceUri);
	}

	public override bool MoveToFollowingSibling(XPathNavigator navigator)
	{
		return navigator.MoveToNext(localName, namespaceUri);
	}

	public override bool MoveToPreviousSibling(XPathNavigator navigator)
	{
		return navigator.MoveToPrevious(localName, namespaceUri);
	}

	public override bool MoveToFollowing(XPathNavigator navigator, XPathNavigator navEnd)
	{
		return navigator.MoveToFollowing(localName, namespaceUri, navEnd);
	}

	public override bool IsFiltered(XPathNavigator navigator)
	{
		if (!(navigator.LocalName != localName))
		{
			return navigator.NamespaceURI != namespaceUri;
		}
		return true;
	}
}
