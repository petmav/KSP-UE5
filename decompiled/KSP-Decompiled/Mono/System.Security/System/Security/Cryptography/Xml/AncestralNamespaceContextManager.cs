using System.Collections;
using System.Xml;

namespace System.Security.Cryptography.Xml;

internal abstract class AncestralNamespaceContextManager
{
	internal ArrayList _ancestorStack = new ArrayList();

	internal NamespaceFrame GetScopeAt(int i)
	{
		return (NamespaceFrame)_ancestorStack[i];
	}

	internal NamespaceFrame GetCurrentScope()
	{
		return GetScopeAt(_ancestorStack.Count - 1);
	}

	protected XmlAttribute GetNearestRenderedNamespaceWithMatchingPrefix(string nsPrefix, out int depth)
	{
		XmlAttribute xmlAttribute = null;
		depth = -1;
		for (int num = _ancestorStack.Count - 1; num >= 0; num--)
		{
			if ((xmlAttribute = GetScopeAt(num).GetRendered(nsPrefix)) != null)
			{
				depth = num;
				return xmlAttribute;
			}
		}
		return null;
	}

	protected XmlAttribute GetNearestUnrenderedNamespaceWithMatchingPrefix(string nsPrefix, out int depth)
	{
		XmlAttribute xmlAttribute = null;
		depth = -1;
		for (int num = _ancestorStack.Count - 1; num >= 0; num--)
		{
			if ((xmlAttribute = GetScopeAt(num).GetUnrendered(nsPrefix)) != null)
			{
				depth = num;
				return xmlAttribute;
			}
		}
		return null;
	}

	internal void EnterElementContext()
	{
		_ancestorStack.Add(new NamespaceFrame());
	}

	internal void ExitElementContext()
	{
		_ancestorStack.RemoveAt(_ancestorStack.Count - 1);
	}

	internal abstract void TrackNamespaceNode(XmlAttribute attr, SortedList nsListToRender, Hashtable nsLocallyDeclared);

	internal abstract void TrackXmlNamespaceNode(XmlAttribute attr, SortedList nsListToRender, SortedList attrListToRender, Hashtable nsLocallyDeclared);

	internal abstract void GetNamespacesToRender(XmlElement element, SortedList attrListToRender, SortedList nsListToRender, Hashtable nsLocallyDeclared);

	internal void LoadUnrenderedNamespaces(Hashtable nsLocallyDeclared)
	{
		object[] array = new object[nsLocallyDeclared.Count];
		nsLocallyDeclared.Values.CopyTo(array, 0);
		object[] array2 = array;
		foreach (object obj in array2)
		{
			AddUnrendered((XmlAttribute)obj);
		}
	}

	internal void LoadRenderedNamespaces(SortedList nsRenderedList)
	{
		foreach (object key in nsRenderedList.GetKeyList())
		{
			AddRendered((XmlAttribute)key);
		}
	}

	internal void AddRendered(XmlAttribute attr)
	{
		GetCurrentScope().AddRendered(attr);
	}

	internal void AddUnrendered(XmlAttribute attr)
	{
		GetCurrentScope().AddUnrendered(attr);
	}
}
