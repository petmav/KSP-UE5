namespace System.Xml.Xsl.Qil;

internal class QilList : QilNode
{
	private int count;

	private QilNode[] members;

	public override XmlQueryType XmlType
	{
		get
		{
			if (xmlType == null)
			{
				XmlQueryType left = XmlQueryTypeFactory.Empty;
				if (count > 0)
				{
					if (nodeType == QilNodeType.Sequence)
					{
						for (int i = 0; i < count; i++)
						{
							left = XmlQueryTypeFactory.Sequence(left, members[i].XmlType);
						}
					}
					else if (nodeType == QilNodeType.BranchList)
					{
						left = members[0].XmlType;
						for (int j = 1; j < count; j++)
						{
							left = XmlQueryTypeFactory.Choice(left, members[j].XmlType);
						}
					}
				}
				xmlType = left;
			}
			return xmlType;
		}
	}

	public override int Count => count;

	public override QilNode this[int index]
	{
		get
		{
			if (index >= 0 && index < count)
			{
				return members[index];
			}
			throw new IndexOutOfRangeException();
		}
		set
		{
			if (index >= 0 && index < count)
			{
				members[index] = value;
				xmlType = null;
				return;
			}
			throw new IndexOutOfRangeException();
		}
	}

	public QilList(QilNodeType nodeType)
		: base(nodeType)
	{
		members = new QilNode[4];
		xmlType = null;
	}

	public override QilNode ShallowClone(QilFactory f)
	{
		QilList obj = (QilList)MemberwiseClone();
		obj.members = (QilNode[])members.Clone();
		return obj;
	}

	public override void Insert(int index, QilNode node)
	{
		if (index < 0 || index > count)
		{
			throw new IndexOutOfRangeException();
		}
		if (count == members.Length)
		{
			QilNode[] destinationArray = new QilNode[count * 2];
			Array.Copy(members, destinationArray, count);
			members = destinationArray;
		}
		if (index < count)
		{
			Array.Copy(members, index, members, index + 1, count - index);
		}
		count++;
		members[index] = node;
		xmlType = null;
	}

	public override void RemoveAt(int index)
	{
		if (index < 0 || index >= count)
		{
			throw new IndexOutOfRangeException();
		}
		count--;
		if (index < count)
		{
			Array.Copy(members, index + 1, members, index, count - index);
		}
		members[count] = null;
		xmlType = null;
	}
}
