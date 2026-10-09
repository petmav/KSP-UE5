using System.Xml.XPath;

namespace MS.Internal.Xml.XPath;

internal sealed class SortKey
{
	private int numKeys;

	private object[] keys;

	private int originalPosition;

	private XPathNavigator node;

	public object this[int index]
	{
		get
		{
			return keys[index];
		}
		set
		{
			keys[index] = value;
		}
	}

	public int NumKeys => numKeys;

	public int OriginalPosition => originalPosition;

	public XPathNavigator Node => node;

	public SortKey(int numKeys, int originalPosition, XPathNavigator node)
	{
		this.numKeys = numKeys;
		keys = new object[numKeys];
		this.originalPosition = originalPosition;
		this.node = node;
	}
}
