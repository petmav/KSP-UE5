using System.Collections.Generic;
using System.ComponentModel;
using System.Xml.XPath;

namespace System.Xml.Xsl.Runtime;

[EditorBrowsable(EditorBrowsableState.Never)]
public struct DodSequenceMerge
{
	private IList<XPathNavigator> firstSequence;

	private List<IEnumerator<XPathNavigator>> sequencesToMerge;

	private int nodeCount;

	private XmlQueryRuntime runtime;

	public void Create(XmlQueryRuntime runtime)
	{
		firstSequence = null;
		sequencesToMerge = null;
		nodeCount = 0;
		this.runtime = runtime;
	}

	public void AddSequence(IList<XPathNavigator> sequence)
	{
		if (sequence.Count == 0)
		{
			return;
		}
		if (firstSequence == null)
		{
			firstSequence = sequence;
			return;
		}
		if (sequencesToMerge == null)
		{
			sequencesToMerge = new List<IEnumerator<XPathNavigator>>();
			MoveAndInsertSequence(firstSequence.GetEnumerator());
			nodeCount = firstSequence.Count;
		}
		MoveAndInsertSequence(sequence.GetEnumerator());
		nodeCount += sequence.Count;
	}

	public IList<XPathNavigator> MergeSequences()
	{
		if (firstSequence == null)
		{
			return XmlQueryNodeSequence.Empty;
		}
		if (sequencesToMerge == null || sequencesToMerge.Count <= 1)
		{
			return firstSequence;
		}
		XmlQueryNodeSequence xmlQueryNodeSequence = new XmlQueryNodeSequence(nodeCount);
		while (sequencesToMerge.Count != 1)
		{
			IEnumerator<XPathNavigator> enumerator = sequencesToMerge[sequencesToMerge.Count - 1];
			sequencesToMerge.RemoveAt(sequencesToMerge.Count - 1);
			xmlQueryNodeSequence.Add(enumerator.Current);
			MoveAndInsertSequence(enumerator);
		}
		do
		{
			xmlQueryNodeSequence.Add(sequencesToMerge[0].Current);
		}
		while (sequencesToMerge[0].MoveNext());
		return xmlQueryNodeSequence;
	}

	private void MoveAndInsertSequence(IEnumerator<XPathNavigator> sequence)
	{
		if (sequence.MoveNext())
		{
			InsertSequence(sequence);
		}
	}

	private void InsertSequence(IEnumerator<XPathNavigator> sequence)
	{
		for (int num = sequencesToMerge.Count - 1; num >= 0; num--)
		{
			switch (runtime.ComparePosition(sequence.Current, sequencesToMerge[num].Current))
			{
			case -1:
				sequencesToMerge.Insert(num + 1, sequence);
				return;
			case 0:
				if (!sequence.MoveNext())
				{
					return;
				}
				break;
			}
		}
		sequencesToMerge.Insert(0, sequence);
	}
}
