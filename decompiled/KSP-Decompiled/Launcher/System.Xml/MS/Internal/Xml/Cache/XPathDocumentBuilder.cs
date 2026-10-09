using System;
using System.Collections;
using System.Collections.Generic;
using System.Xml;
using System.Xml.XPath;

namespace MS.Internal.Xml.Cache;

internal sealed class XPathDocumentBuilder : XmlRawWriter
{
	private struct NodePageFactory
	{
		private XPathNode[] page;

		private XPathNodePageInfo pageInfo;

		private int pageSize;

		public XPathNode[] NextNodePage => page;

		public int NextNodeIndex => pageInfo.NodeCount;

		public void Init(int initialPageSize)
		{
			pageSize = initialPageSize;
			page = new XPathNode[pageSize];
			pageInfo = new XPathNodePageInfo(null, 1);
			page[0].Create(pageInfo);
		}

		public void AllocateSlot(out XPathNode[] page, out int idx)
		{
			page = this.page;
			idx = pageInfo.NodeCount;
			if (++pageInfo.NodeCount >= this.page.Length)
			{
				if (pageSize < 65536)
				{
					pageSize *= 2;
				}
				this.page = new XPathNode[pageSize];
				pageInfo.NextPage = this.page;
				pageInfo = new XPathNodePageInfo(page, pageInfo.PageNumber + 1);
				this.page[0].Create(pageInfo);
			}
		}
	}

	private struct TextBlockBuilder
	{
		private IXmlLineInfo lineInfo;

		private TextBlockType textType;

		private string text;

		private int lineNum;

		private int linePos;

		public TextBlockType TextType => textType;

		public bool HasText => textType != TextBlockType.None;

		public int LineNumber => lineNum;

		public int LinePosition => linePos;

		public void Initialize(IXmlLineInfo lineInfo)
		{
			this.lineInfo = lineInfo;
			textType = TextBlockType.None;
		}

		public void WriteTextBlock(string text, TextBlockType textType)
		{
			if (text.Length == 0)
			{
				return;
			}
			if (this.textType == TextBlockType.None)
			{
				this.text = text;
				this.textType = textType;
				if (lineInfo != null)
				{
					lineNum = lineInfo.LineNumber;
					linePos = lineInfo.LinePosition;
				}
			}
			else
			{
				this.text += text;
				if (textType < this.textType)
				{
					this.textType = textType;
				}
			}
		}

		public string ReadText()
		{
			if (textType == TextBlockType.None)
			{
				return string.Empty;
			}
			textType = TextBlockType.None;
			return text;
		}
	}

	private NodePageFactory nodePageFact;

	private NodePageFactory nmspPageFact;

	private TextBlockBuilder textBldr;

	private Stack<XPathNodeRef> stkNmsp;

	private XPathNodeInfoTable infoTable;

	private XPathDocument doc;

	private IXmlLineInfo lineInfo;

	private XmlNameTable nameTable;

	private bool atomizeNames;

	private XPathNode[] pageNmsp;

	private int idxNmsp;

	private XPathNode[] pageParent;

	private int idxParent;

	private XPathNode[] pageSibling;

	private int idxSibling;

	private int lineNumBase;

	private int linePosBase;

	private XmlQualifiedName idAttrName;

	private Hashtable elemIdMap;

	private XPathNodeRef[] elemNameIndex;

	private const int ElementIndexSize = 64;

	public XPathDocumentBuilder(XPathDocument doc, IXmlLineInfo lineInfo, string baseUri, XPathDocument.LoadFlags flags)
	{
		nodePageFact.Init(256);
		nmspPageFact.Init(16);
		stkNmsp = new Stack<XPathNodeRef>();
		Initialize(doc, lineInfo, baseUri, flags);
	}

	public void Initialize(XPathDocument doc, IXmlLineInfo lineInfo, string baseUri, XPathDocument.LoadFlags flags)
	{
		this.doc = doc;
		nameTable = doc.NameTable;
		atomizeNames = (flags & XPathDocument.LoadFlags.AtomizeNames) != 0;
		idxParent = (idxSibling = 0);
		elemNameIndex = new XPathNodeRef[64];
		textBldr.Initialize(lineInfo);
		this.lineInfo = lineInfo;
		lineNumBase = 0;
		linePosBase = 0;
		infoTable = new XPathNodeInfoTable();
		XPathNode[] page;
		int idxText = NewNode(out page, XPathNodeType.Text, string.Empty, string.Empty, string.Empty, string.Empty);
		this.doc.SetCollapsedTextNode(page, idxText);
		idxNmsp = NewNamespaceNode(out pageNmsp, nameTable.Add("xml"), nameTable.Add("http://www.w3.org/XML/1998/namespace"), null, 0);
		this.doc.SetXmlNamespaceNode(pageNmsp, idxNmsp);
		if ((flags & XPathDocument.LoadFlags.Fragment) == 0)
		{
			idxParent = NewNode(out pageParent, XPathNodeType.Root, string.Empty, string.Empty, string.Empty, baseUri);
			this.doc.SetRootNode(pageParent, idxParent);
		}
		else
		{
			this.doc.SetRootNode(nodePageFact.NextNodePage, nodePageFact.NextNodeIndex);
		}
	}

	public override void WriteDocType(string name, string pubid, string sysid, string subset)
	{
	}

	public override void WriteStartElement(string prefix, string localName, string ns)
	{
		WriteStartElement(prefix, localName, ns, string.Empty);
	}

	public void WriteStartElement(string prefix, string localName, string ns, string baseUri)
	{
		if (atomizeNames)
		{
			prefix = nameTable.Add(prefix);
			localName = nameTable.Add(localName);
			ns = nameTable.Add(ns);
		}
		AddSibling(XPathNodeType.Element, localName, ns, prefix, baseUri);
		pageParent = pageSibling;
		idxParent = idxSibling;
		idxSibling = 0;
		int num = pageParent[idxParent].LocalNameHashCode & 0x3F;
		elemNameIndex[num] = LinkSimilarElements(elemNameIndex[num].Page, elemNameIndex[num].Index, pageParent, idxParent);
		if (elemIdMap != null)
		{
			idAttrName = (XmlQualifiedName)elemIdMap[new XmlQualifiedName(localName, prefix)];
		}
	}

	public override void WriteEndElement()
	{
		WriteEndElement(allowShortcutTag: true);
	}

	public override void WriteFullEndElement()
	{
		WriteEndElement(allowShortcutTag: false);
	}

	internal override void WriteEndElement(string prefix, string localName, string namespaceName)
	{
		WriteEndElement(allowShortcutTag: true);
	}

	internal override void WriteFullEndElement(string prefix, string localName, string namespaceName)
	{
		WriteEndElement(allowShortcutTag: false);
	}

	public void WriteEndElement(bool allowShortcutTag)
	{
		if (!pageParent[idxParent].HasContentChild)
		{
			TextBlockType textType = textBldr.TextType;
			if (textType != TextBlockType.Text)
			{
				if ((uint)(textType - 5) > 1u)
				{
					pageParent[idxParent].SetEmptyValue(allowShortcutTag);
					goto IL_012d;
				}
			}
			else
			{
				if (lineInfo == null)
				{
					goto IL_00aa;
				}
				if (textBldr.LineNumber == pageParent[idxParent].LineNumber)
				{
					int num = textBldr.LinePosition - pageParent[idxParent].LinePosition;
					if (num >= 0 && num <= 255)
					{
						pageParent[idxParent].SetCollapsedLineInfoOffset(num);
						goto IL_00aa;
					}
				}
			}
			CachedTextNode();
			pageParent[idxParent].SetValue(pageSibling[idxSibling].Value);
		}
		else if (textBldr.HasText)
		{
			CachedTextNode();
		}
		goto IL_012d;
		IL_00aa:
		pageParent[idxParent].SetCollapsedValue(textBldr.ReadText());
		goto IL_012d;
		IL_012d:
		if (pageParent[idxParent].HasNamespaceDecls)
		{
			doc.AddNamespace(pageParent, idxParent, pageNmsp, idxNmsp);
			XPathNodeRef xPathNodeRef = stkNmsp.Pop();
			pageNmsp = xPathNodeRef.Page;
			idxNmsp = xPathNodeRef.Index;
		}
		pageSibling = pageParent;
		idxSibling = idxParent;
		idxParent = pageParent[idxParent].GetParent(out pageParent);
	}

	public override void WriteStartAttribute(string prefix, string localName, string namespaceName)
	{
		if (atomizeNames)
		{
			prefix = nameTable.Add(prefix);
			localName = nameTable.Add(localName);
			namespaceName = nameTable.Add(namespaceName);
		}
		AddSibling(XPathNodeType.Attribute, localName, namespaceName, prefix, string.Empty);
	}

	public override void WriteEndAttribute()
	{
		pageSibling[idxSibling].SetValue(textBldr.ReadText());
		if (idAttrName != null && pageSibling[idxSibling].LocalName == idAttrName.Name && pageSibling[idxSibling].Prefix == idAttrName.Namespace)
		{
			doc.AddIdElement(pageSibling[idxSibling].Value, pageParent, idxParent);
		}
	}

	public override void WriteCData(string text)
	{
		WriteString(text, TextBlockType.Text);
	}

	public override void WriteComment(string text)
	{
		AddSibling(XPathNodeType.Comment, string.Empty, string.Empty, string.Empty, string.Empty);
		pageSibling[idxSibling].SetValue(text);
	}

	public override void WriteProcessingInstruction(string name, string text)
	{
		WriteProcessingInstruction(name, text, string.Empty);
	}

	public void WriteProcessingInstruction(string name, string text, string baseUri)
	{
		if (atomizeNames)
		{
			name = nameTable.Add(name);
		}
		AddSibling(XPathNodeType.ProcessingInstruction, name, string.Empty, string.Empty, baseUri);
		pageSibling[idxSibling].SetValue(text);
	}

	public override void WriteWhitespace(string ws)
	{
		WriteString(ws, TextBlockType.Whitespace);
	}

	public override void WriteString(string text)
	{
		WriteString(text, TextBlockType.Text);
	}

	public override void WriteChars(char[] buffer, int index, int count)
	{
		WriteString(new string(buffer, index, count), TextBlockType.Text);
	}

	public override void WriteRaw(string data)
	{
		WriteString(data, TextBlockType.Text);
	}

	public override void WriteRaw(char[] buffer, int index, int count)
	{
		WriteString(new string(buffer, index, count), TextBlockType.Text);
	}

	public void WriteString(string text, TextBlockType textType)
	{
		textBldr.WriteTextBlock(text, textType);
	}

	public override void WriteEntityRef(string name)
	{
		throw new NotImplementedException();
	}

	public override void WriteCharEntity(char ch)
	{
		char[] value = new char[1] { ch };
		WriteString(new string(value), TextBlockType.Text);
	}

	public override void WriteSurrogateCharEntity(char lowChar, char highChar)
	{
		char[] value = new char[2] { highChar, lowChar };
		WriteString(new string(value), TextBlockType.Text);
	}

	public override void Close()
	{
		if (textBldr.HasText)
		{
			CachedTextNode();
		}
		if (doc.GetRootNode(out var pageRoot) == nodePageFact.NextNodeIndex && pageRoot == nodePageFact.NextNodePage)
		{
			AddSibling(XPathNodeType.Text, string.Empty, string.Empty, string.Empty, string.Empty);
			pageSibling[idxSibling].SetValue(string.Empty);
		}
	}

	public override void Flush()
	{
	}

	internal override void WriteXmlDeclaration(XmlStandalone standalone)
	{
	}

	internal override void WriteXmlDeclaration(string xmldecl)
	{
	}

	internal override void StartElementContent()
	{
	}

	internal override void WriteNamespaceDeclaration(string prefix, string namespaceName)
	{
		if (atomizeNames)
		{
			prefix = nameTable.Add(prefix);
		}
		namespaceName = nameTable.Add(namespaceName);
		XPathNode[] pageNode = pageNmsp;
		int sibling = idxNmsp;
		while (sibling != 0 && (object)pageNode[sibling].LocalName != prefix)
		{
			sibling = pageNode[sibling].GetSibling(out pageNode);
		}
		XPathNode[] page;
		int num = NewNamespaceNode(out page, prefix, namespaceName, pageParent, idxParent);
		if (sibling != 0)
		{
			XPathNode[] pageNode2 = pageNmsp;
			int sibling2 = idxNmsp;
			XPathNode[] array = page;
			int num2 = num;
			while (sibling2 != sibling || pageNode2 != pageNode)
			{
				int parent = pageNode2[sibling2].GetParent(out var pageNode3);
				parent = NewNamespaceNode(out pageNode3, pageNode2[sibling2].LocalName, pageNode2[sibling2].Value, pageNode3, parent);
				array[num2].SetSibling(infoTable, pageNode3, parent);
				array = pageNode3;
				num2 = parent;
				sibling2 = pageNode2[sibling2].GetSibling(out pageNode2);
			}
			sibling = pageNode[sibling].GetSibling(out pageNode);
			if (sibling != 0)
			{
				array[num2].SetSibling(infoTable, pageNode, sibling);
			}
		}
		else if (idxParent != 0)
		{
			page[num].SetSibling(infoTable, pageNmsp, idxNmsp);
		}
		else
		{
			doc.SetRootNode(page, num);
		}
		if (idxParent != 0)
		{
			if (!pageParent[idxParent].HasNamespaceDecls)
			{
				stkNmsp.Push(new XPathNodeRef(pageNmsp, idxNmsp));
				pageParent[idxParent].HasNamespaceDecls = true;
			}
			pageNmsp = page;
			idxNmsp = num;
		}
	}

	public void CreateIdTables(IDtdInfo dtdInfo)
	{
		foreach (IDtdAttributeListInfo attributeList in dtdInfo.GetAttributeLists())
		{
			IDtdAttributeInfo dtdAttributeInfo = attributeList.LookupIdAttribute();
			if (dtdAttributeInfo != null)
			{
				if (elemIdMap == null)
				{
					elemIdMap = new Hashtable();
				}
				elemIdMap.Add(new XmlQualifiedName(attributeList.LocalName, attributeList.Prefix), new XmlQualifiedName(dtdAttributeInfo.LocalName, dtdAttributeInfo.Prefix));
			}
		}
	}

	private XPathNodeRef LinkSimilarElements(XPathNode[] pagePrev, int idxPrev, XPathNode[] pageNext, int idxNext)
	{
		pagePrev?[idxPrev].SetSimilarElement(infoTable, pageNext, idxNext);
		return new XPathNodeRef(pageNext, idxNext);
	}

	private int NewNamespaceNode(out XPathNode[] page, string prefix, string namespaceUri, XPathNode[] pageElem, int idxElem)
	{
		nmspPageFact.AllocateSlot(out var page2, out var idx);
		ComputeLineInfo(isTextNode: false, out var lineNumOffset, out var linePosOffset);
		XPathNodeInfoAtom info = infoTable.Create(prefix, string.Empty, string.Empty, string.Empty, pageElem, page2, null, doc, lineNumBase, linePosBase);
		page2[idx].Create(info, XPathNodeType.Namespace, idxElem);
		page2[idx].SetValue(namespaceUri);
		page2[idx].SetLineInfoOffsets(lineNumOffset, linePosOffset);
		page = page2;
		return idx;
	}

	private int NewNode(out XPathNode[] page, XPathNodeType xptyp, string localName, string namespaceUri, string prefix, string baseUri)
	{
		nodePageFact.AllocateSlot(out var page2, out var idx);
		ComputeLineInfo(XPathNavigator.IsText(xptyp), out var lineNumOffset, out var linePosOffset);
		XPathNodeInfoAtom info = infoTable.Create(localName, namespaceUri, prefix, baseUri, pageParent, page2, page2, doc, lineNumBase, linePosBase);
		page2[idx].Create(info, xptyp, idxParent);
		page2[idx].SetLineInfoOffsets(lineNumOffset, linePosOffset);
		page = page2;
		return idx;
	}

	private void ComputeLineInfo(bool isTextNode, out int lineNumOffset, out int linePosOffset)
	{
		if (lineInfo == null)
		{
			lineNumOffset = 0;
			linePosOffset = 0;
			return;
		}
		int lineNumber;
		int linePosition;
		if (isTextNode)
		{
			lineNumber = textBldr.LineNumber;
			linePosition = textBldr.LinePosition;
		}
		else
		{
			lineNumber = lineInfo.LineNumber;
			linePosition = lineInfo.LinePosition;
		}
		lineNumOffset = lineNumber - lineNumBase;
		if (lineNumOffset < 0 || lineNumOffset > 16383)
		{
			lineNumBase = lineNumber;
			lineNumOffset = 0;
		}
		linePosOffset = linePosition - linePosBase;
		if (linePosOffset < 0 || linePosOffset > 65535)
		{
			linePosBase = linePosition;
			linePosOffset = 0;
		}
	}

	private void AddSibling(XPathNodeType xptyp, string localName, string namespaceUri, string prefix, string baseUri)
	{
		if (textBldr.HasText)
		{
			CachedTextNode();
		}
		XPathNode[] page;
		int num = NewNode(out page, xptyp, localName, namespaceUri, prefix, baseUri);
		if (idxParent != 0)
		{
			pageParent[idxParent].SetParentProperties(xptyp);
			if (idxSibling != 0)
			{
				pageSibling[idxSibling].SetSibling(infoTable, page, num);
			}
		}
		pageSibling = page;
		idxSibling = num;
	}

	private void CachedTextNode()
	{
		TextBlockType textType = textBldr.TextType;
		string value = textBldr.ReadText();
		AddSibling((XPathNodeType)textType, string.Empty, string.Empty, string.Empty, string.Empty);
		pageSibling[idxSibling].SetValue(value);
	}
}
