namespace MS.Internal.Xml.Cache;

internal struct XPathNodeRef
{
	private XPathNode[] page;

	private int idx;

	public static XPathNodeRef Null => default(XPathNodeRef);

	public bool IsNull => page == null;

	public XPathNode[] Page => page;

	public int Index => idx;

	public XPathNodeRef(XPathNode[] page, int idx)
	{
		this.page = page;
		this.idx = idx;
	}

	public override int GetHashCode()
	{
		return XPathNodeHelper.GetLocation(page, idx);
	}
}
