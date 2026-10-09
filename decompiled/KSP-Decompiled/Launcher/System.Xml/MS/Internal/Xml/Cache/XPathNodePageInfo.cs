namespace MS.Internal.Xml.Cache;

internal sealed class XPathNodePageInfo
{
	private int pageNum;

	private int nodeCount;

	private XPathNode[] pagePrev;

	private XPathNode[] pageNext;

	public int PageNumber => pageNum;

	public int NodeCount
	{
		get
		{
			return nodeCount;
		}
		set
		{
			nodeCount = value;
		}
	}

	public XPathNode[] PreviousPage => pagePrev;

	public XPathNode[] NextPage
	{
		get
		{
			return pageNext;
		}
		set
		{
			pageNext = value;
		}
	}

	public XPathNodePageInfo(XPathNode[] pagePrev, int pageNum)
	{
		this.pagePrev = pagePrev;
		this.pageNum = pageNum;
		nodeCount = 1;
	}
}
