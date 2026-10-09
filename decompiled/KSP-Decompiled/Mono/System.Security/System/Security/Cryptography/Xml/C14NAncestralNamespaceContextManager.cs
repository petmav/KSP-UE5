using System.Collections;
using System.Xml;

namespace System.Security.Cryptography.Xml;

internal class C14NAncestralNamespaceContextManager : AncestralNamespaceContextManager
{
	internal C14NAncestralNamespaceContextManager()
	{
	}

	private void GetNamespaceToRender(string nsPrefix, SortedList attrListToRender, SortedList nsListToRender, Hashtable nsLocallyDeclared)
	{
		foreach (XmlAttribute key in nsListToRender.GetKeyList())
		{
			if (Utils.HasNamespacePrefix(key, nsPrefix))
			{
				return;
			}
		}
		foreach (XmlAttribute key2 in attrListToRender.GetKeyList())
		{
			if (key2.LocalName.Equals(nsPrefix))
			{
				return;
			}
		}
		XmlAttribute xmlAttribute = (XmlAttribute)nsLocallyDeclared[nsPrefix];
		int depth;
		XmlAttribute nearestRenderedNamespaceWithMatchingPrefix = GetNearestRenderedNamespaceWithMatchingPrefix(nsPrefix, out depth);
		if (xmlAttribute != null)
		{
			if (Utils.IsNonRedundantNamespaceDecl(xmlAttribute, nearestRenderedNamespaceWithMatchingPrefix))
			{
				nsLocallyDeclared.Remove(nsPrefix);
				if (Utils.IsXmlNamespaceNode(xmlAttribute))
				{
					attrListToRender.Add(xmlAttribute, null);
				}
				else
				{
					nsListToRender.Add(xmlAttribute, null);
				}
			}
			return;
		}
		int depth2;
		XmlAttribute nearestUnrenderedNamespaceWithMatchingPrefix = GetNearestUnrenderedNamespaceWithMatchingPrefix(nsPrefix, out depth2);
		if (nearestUnrenderedNamespaceWithMatchingPrefix != null && depth2 > depth && Utils.IsNonRedundantNamespaceDecl(nearestUnrenderedNamespaceWithMatchingPrefix, nearestRenderedNamespaceWithMatchingPrefix))
		{
			if (Utils.IsXmlNamespaceNode(nearestUnrenderedNamespaceWithMatchingPrefix))
			{
				attrListToRender.Add(nearestUnrenderedNamespaceWithMatchingPrefix, null);
			}
			else
			{
				nsListToRender.Add(nearestUnrenderedNamespaceWithMatchingPrefix, null);
			}
		}
	}

	internal override void GetNamespacesToRender(XmlElement element, SortedList attrListToRender, SortedList nsListToRender, Hashtable nsLocallyDeclared)
	{
		XmlAttribute xmlAttribute = null;
		object[] array = new object[nsLocallyDeclared.Count];
		nsLocallyDeclared.Values.CopyTo(array, 0);
		object[] array2 = array;
		for (int i = 0; i < array2.Length; i++)
		{
			xmlAttribute = (XmlAttribute)array2[i];
			int depth;
			XmlAttribute nearestRenderedNamespaceWithMatchingPrefix = GetNearestRenderedNamespaceWithMatchingPrefix(Utils.GetNamespacePrefix(xmlAttribute), out depth);
			if (Utils.IsNonRedundantNamespaceDecl(xmlAttribute, nearestRenderedNamespaceWithMatchingPrefix))
			{
				nsLocallyDeclared.Remove(Utils.GetNamespacePrefix(xmlAttribute));
				if (Utils.IsXmlNamespaceNode(xmlAttribute))
				{
					attrListToRender.Add(xmlAttribute, null);
				}
				else
				{
					nsListToRender.Add(xmlAttribute, null);
				}
			}
		}
		for (int num = _ancestorStack.Count - 1; num >= 0; num--)
		{
			foreach (XmlAttribute value in GetScopeAt(num).GetUnrendered().Values)
			{
				if (value != null)
				{
					GetNamespaceToRender(Utils.GetNamespacePrefix(value), attrListToRender, nsListToRender, nsLocallyDeclared);
				}
			}
		}
	}

	internal override void TrackNamespaceNode(XmlAttribute attr, SortedList nsListToRender, Hashtable nsLocallyDeclared)
	{
		nsLocallyDeclared.Add(Utils.GetNamespacePrefix(attr), attr);
	}

	internal override void TrackXmlNamespaceNode(XmlAttribute attr, SortedList nsListToRender, SortedList attrListToRender, Hashtable nsLocallyDeclared)
	{
		nsLocallyDeclared.Add(Utils.GetNamespacePrefix(attr), attr);
	}
}
