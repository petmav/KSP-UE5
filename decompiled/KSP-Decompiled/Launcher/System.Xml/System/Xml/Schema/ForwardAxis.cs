namespace System.Xml.Schema;

internal class ForwardAxis
{
	private DoubleLinkAxis topNode;

	private DoubleLinkAxis rootNode;

	private bool isAttribute;

	private bool isDss;

	private bool isSelfAxis;

	internal DoubleLinkAxis RootNode => rootNode;

	internal DoubleLinkAxis TopNode => topNode;

	internal bool IsAttribute => isAttribute;

	internal bool IsDss => isDss;

	internal bool IsSelfAxis => isSelfAxis;

	public ForwardAxis(DoubleLinkAxis axis, bool isdesorself)
	{
		isDss = isdesorself;
		isAttribute = Asttree.IsAttribute(axis);
		topNode = axis;
		rootNode = axis;
		while (rootNode.Input != null)
		{
			rootNode = (DoubleLinkAxis)rootNode.Input;
		}
		isSelfAxis = Asttree.IsSelf(topNode);
	}
}
