using System.ComponentModel;
using System.Xml.XPath;

namespace System.Xml.Xsl.Runtime;

[EditorBrowsable(EditorBrowsableState.Never)]
public struct FollowingSiblingMergeIterator
{
	private ContentMergeIterator wrapped;

	public XPathNavigator Current => wrapped.Current;

	public void Create(XmlNavigatorFilter filter)
	{
		wrapped.Create(filter);
	}

	public IteratorResult MoveNext(XPathNavigator navigator)
	{
		return wrapped.MoveNext(navigator, isContent: false);
	}
}
