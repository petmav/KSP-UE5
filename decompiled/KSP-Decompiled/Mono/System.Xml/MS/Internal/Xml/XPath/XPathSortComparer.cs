using System.Collections;
using System.Collections.Generic;
using System.Xml.XPath;

namespace MS.Internal.Xml.XPath;

internal sealed class XPathSortComparer : IComparer<SortKey>
{
	private const int minSize = 3;

	private Query[] expressions;

	private IComparer[] comparers;

	private int numSorts;

	public int NumSorts => numSorts;

	public XPathSortComparer(int size)
	{
		if (size <= 0)
		{
			size = 3;
		}
		expressions = new Query[size];
		comparers = new IComparer[size];
	}

	public XPathSortComparer()
		: this(3)
	{
	}

	public void AddSort(Query evalQuery, IComparer comparer)
	{
		if (numSorts == expressions.Length)
		{
			Query[] array = new Query[numSorts * 2];
			IComparer[] array2 = new IComparer[numSorts * 2];
			for (int i = 0; i < numSorts; i++)
			{
				array[i] = expressions[i];
				array2[i] = comparers[i];
			}
			expressions = array;
			comparers = array2;
		}
		if (evalQuery.StaticType == XPathResultType.NodeSet || evalQuery.StaticType == XPathResultType.Any)
		{
			evalQuery = new StringFunctions(Function.FunctionType.FuncString, new Query[1] { evalQuery });
		}
		expressions[numSorts] = evalQuery;
		comparers[numSorts] = comparer;
		numSorts++;
	}

	public Query Expression(int i)
	{
		return expressions[i];
	}

	int IComparer<SortKey>.Compare(SortKey x, SortKey y)
	{
		int num = 0;
		for (int i = 0; i < x.NumKeys; i++)
		{
			num = comparers[i].Compare(x[i], y[i]);
			if (num != 0)
			{
				return num;
			}
		}
		return x.OriginalPosition - y.OriginalPosition;
	}

	internal XPathSortComparer Clone()
	{
		XPathSortComparer xPathSortComparer = new XPathSortComparer(numSorts);
		for (int i = 0; i < numSorts; i++)
		{
			xPathSortComparer.comparers[i] = comparers[i];
			xPathSortComparer.expressions[i] = (Query)expressions[i].Clone();
		}
		xPathSortComparer.numSorts = numSorts;
		return xPathSortComparer;
	}
}
