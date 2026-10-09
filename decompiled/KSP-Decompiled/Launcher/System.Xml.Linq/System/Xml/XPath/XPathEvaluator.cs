using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Xml.Linq;

namespace System.Xml.XPath;

[StructLayout(LayoutKind.Sequential, Size = 1)]
internal struct XPathEvaluator
{
	public object Evaluate<T>(XNode node, string expression, IXmlNamespaceResolver resolver) where T : class
	{
		object obj = node.CreateNavigator().Evaluate(expression, resolver);
		if (obj is XPathNodeIterator)
		{
			return EvaluateIterator<T>((XPathNodeIterator)obj);
		}
		if (!(obj is T))
		{
			throw new InvalidOperationException(System.Xml.Linq.Res.GetString("InvalidOperation_UnexpectedEvaluation", obj.GetType()));
		}
		return (T)obj;
	}

	private IEnumerable<T> EvaluateIterator<T>(XPathNodeIterator result)
	{
		foreach (XPathNavigator item in result)
		{
			object r = item.UnderlyingObject;
			if (!(r is T))
			{
				throw new InvalidOperationException(System.Xml.Linq.Res.GetString("InvalidOperation_UnexpectedEvaluation", r.GetType()));
			}
			yield return (T)r;
			XText t = r as XText;
			if (t == null || t.parent == null)
			{
				continue;
			}
			while (t != t.parent.content)
			{
				t = t.next as XText;
				if (t == null)
				{
					break;
				}
				yield return (T)(object)t;
			}
		}
	}
}
