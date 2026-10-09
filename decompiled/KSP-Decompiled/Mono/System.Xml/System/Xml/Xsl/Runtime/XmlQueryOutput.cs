using System.Collections.Generic;
using System.ComponentModel;
using System.Globalization;
using System.Xml.XPath;

namespace System.Xml.Xsl.Runtime;

[EditorBrowsable(EditorBrowsableState.Never)]
public sealed class XmlQueryOutput : XmlWriter
{
	private XmlRawWriter xwrt;

	private XmlQueryRuntime runtime;

	private XmlAttributeCache attrCache;

	private int depth;

	private XmlState xstate;

	private XmlSequenceWriter seqwrt;

	private XmlNamespaceManager nsmgr;

	private int cntNmsp;

	private Dictionary<string, string> conflictPrefixes;

	private int prefixIndex;

	private string piTarget;

	private StringConcat nodeText;

	private Stack<string> stkNames;

	private XPathNodeType rootType;

	private Dictionary<string, string> usedPrefixes = new Dictionary<string, string>();

	internal XmlSequenceWriter SequenceWriter => seqwrt;

	internal XmlRawWriter Writer
	{
		get
		{
			return xwrt;
		}
		set
		{
			if (value is IRemovableWriter removableWriter)
			{
				removableWriter.OnRemoveWriterEvent = SetWrappedWriter;
			}
			xwrt = value;
		}
	}

	public override WriteState WriteState
	{
		get
		{
			throw new NotSupportedException();
		}
	}

	public override XmlSpace XmlSpace
	{
		get
		{
			throw new NotSupportedException();
		}
	}

	public override string XmlLang
	{
		get
		{
			throw new NotSupportedException();
		}
	}

	internal XmlQueryOutput(XmlQueryRuntime runtime, XmlSequenceWriter seqwrt)
	{
		this.runtime = runtime;
		this.seqwrt = seqwrt;
		xstate = XmlState.WithinSequence;
	}

	internal XmlQueryOutput(XmlQueryRuntime runtime, XmlEventCache xwrt)
	{
		this.runtime = runtime;
		this.xwrt = xwrt;
		xstate = XmlState.WithinContent;
		depth = 1;
		rootType = XPathNodeType.Root;
	}

	private void SetWrappedWriter(XmlRawWriter writer)
	{
		if (Writer is XmlAttributeCache)
		{
			attrCache = (XmlAttributeCache)Writer;
		}
		Writer = writer;
	}

	public override void WriteStartDocument()
	{
		throw new NotSupportedException();
	}

	public override void WriteStartDocument(bool standalone)
	{
		throw new NotSupportedException();
	}

	public override void WriteEndDocument()
	{
		throw new NotSupportedException();
	}

	public override void WriteDocType(string name, string pubid, string sysid, string subset)
	{
		throw new NotSupportedException();
	}

	public override void WriteStartElement(string prefix, string localName, string ns)
	{
		ConstructWithinContent(XPathNodeType.Element);
		WriteStartElementUnchecked(prefix, localName, ns);
		WriteNamespaceDeclarationUnchecked(prefix, ns);
		if (attrCache == null)
		{
			attrCache = new XmlAttributeCache();
		}
		attrCache.Init(Writer);
		Writer = attrCache;
		attrCache = null;
		PushElementNames(prefix, localName, ns);
	}

	public override void WriteEndElement()
	{
		if (xstate == XmlState.EnumAttrs)
		{
			StartElementContentUnchecked();
		}
		PopElementNames(out var prefix, out var localName, out var ns);
		WriteEndElementUnchecked(prefix, localName, ns);
		if (depth == 0)
		{
			EndTree();
		}
	}

	public override void WriteFullEndElement()
	{
		WriteEndElement();
	}

	public override void WriteStartAttribute(string prefix, string localName, string ns)
	{
		if (prefix.Length == 5 && prefix == "xmlns")
		{
			WriteStartNamespace(localName);
			return;
		}
		ConstructInEnumAttrs(XPathNodeType.Attribute);
		if (ns.Length != 0 && depth != 0)
		{
			prefix = CheckAttributePrefix(prefix, ns);
		}
		WriteStartAttributeUnchecked(prefix, localName, ns);
	}

	public override void WriteEndAttribute()
	{
		if (xstate == XmlState.WithinNmsp)
		{
			WriteEndNamespace();
			return;
		}
		WriteEndAttributeUnchecked();
		if (depth == 0)
		{
			EndTree();
		}
	}

	public override void WriteComment(string text)
	{
		WriteStartComment();
		WriteCommentString(text);
		WriteEndComment();
	}

	public override void WriteProcessingInstruction(string target, string text)
	{
		WriteStartProcessingInstruction(target);
		WriteProcessingInstructionString(text);
		WriteEndProcessingInstruction();
	}

	public override void WriteEntityRef(string name)
	{
		throw new NotSupportedException();
	}

	public override void WriteCharEntity(char ch)
	{
		throw new NotSupportedException();
	}

	public override void WriteSurrogateCharEntity(char lowChar, char highChar)
	{
		throw new NotSupportedException();
	}

	public override void WriteWhitespace(string ws)
	{
		throw new NotSupportedException();
	}

	public override void WriteString(string text)
	{
		WriteString(text, disableOutputEscaping: false);
	}

	public override void WriteChars(char[] buffer, int index, int count)
	{
		throw new NotSupportedException();
	}

	public override void WriteRaw(char[] buffer, int index, int count)
	{
		throw new NotSupportedException();
	}

	public override void WriteRaw(string data)
	{
		WriteString(data, disableOutputEscaping: true);
	}

	public override void WriteCData(string text)
	{
		WriteString(text, disableOutputEscaping: false);
	}

	public override void WriteBase64(byte[] buffer, int index, int count)
	{
		throw new NotSupportedException();
	}

	public override void Close()
	{
	}

	public override void Flush()
	{
	}

	public override string LookupPrefix(string ns)
	{
		throw new NotSupportedException();
	}

	public void StartTree(XPathNodeType rootType)
	{
		Writer = seqwrt.StartTree(rootType, nsmgr, runtime.NameTable);
		this.rootType = rootType;
		xstate = ((rootType == XPathNodeType.Attribute || rootType == XPathNodeType.Namespace) ? XmlState.EnumAttrs : XmlState.WithinContent);
	}

	public void EndTree()
	{
		seqwrt.EndTree();
		xstate = XmlState.WithinSequence;
		Writer = null;
	}

	public void WriteStartElementUnchecked(string prefix, string localName, string ns)
	{
		if (nsmgr != null)
		{
			nsmgr.PushScope();
		}
		Writer.WriteStartElement(prefix, localName, ns);
		usedPrefixes.Clear();
		usedPrefixes[prefix] = ns;
		xstate = XmlState.EnumAttrs;
		depth++;
	}

	public void WriteStartElementUnchecked(string localName)
	{
		WriteStartElementUnchecked(string.Empty, localName, string.Empty);
	}

	public void StartElementContentUnchecked()
	{
		if (cntNmsp != 0)
		{
			WriteCachedNamespaces();
		}
		Writer.StartElementContent();
		xstate = XmlState.WithinContent;
	}

	public void WriteEndElementUnchecked(string prefix, string localName, string ns)
	{
		Writer.WriteEndElement(prefix, localName, ns);
		xstate = XmlState.WithinContent;
		depth--;
		if (nsmgr != null)
		{
			nsmgr.PopScope();
		}
	}

	public void WriteEndElementUnchecked(string localName)
	{
		WriteEndElementUnchecked(string.Empty, localName, string.Empty);
	}

	public void WriteStartAttributeUnchecked(string prefix, string localName, string ns)
	{
		Writer.WriteStartAttribute(prefix, localName, ns);
		xstate = XmlState.WithinAttr;
		depth++;
	}

	public void WriteStartAttributeUnchecked(string localName)
	{
		WriteStartAttributeUnchecked(string.Empty, localName, string.Empty);
	}

	public void WriteEndAttributeUnchecked()
	{
		Writer.WriteEndAttribute();
		xstate = XmlState.EnumAttrs;
		depth--;
	}

	public void WriteNamespaceDeclarationUnchecked(string prefix, string ns)
	{
		if (depth == 0)
		{
			Writer.WriteNamespaceDeclaration(prefix, ns);
			return;
		}
		if (nsmgr == null)
		{
			if (ns.Length == 0 && prefix.Length == 0)
			{
				return;
			}
			nsmgr = new XmlNamespaceManager(runtime.NameTable);
			nsmgr.PushScope();
		}
		if (nsmgr.LookupNamespace(prefix) != ns)
		{
			AddNamespace(prefix, ns);
		}
		usedPrefixes[prefix] = ns;
	}

	public void WriteStringUnchecked(string text)
	{
		Writer.WriteString(text);
	}

	public void WriteRawUnchecked(string text)
	{
		Writer.WriteRaw(text);
	}

	public void WriteStartRoot()
	{
		if (xstate != 0)
		{
			ThrowInvalidStateError(XPathNodeType.Root);
		}
		StartTree(XPathNodeType.Root);
		depth++;
	}

	public void WriteEndRoot()
	{
		depth--;
		EndTree();
	}

	public void WriteStartElementLocalName(string localName)
	{
		WriteStartElement(string.Empty, localName, string.Empty);
	}

	public void WriteStartAttributeLocalName(string localName)
	{
		WriteStartAttribute(string.Empty, localName, string.Empty);
	}

	public void WriteStartElementComputed(string tagName, int prefixMappingsIndex)
	{
		WriteStartComputed(XPathNodeType.Element, tagName, prefixMappingsIndex);
	}

	public void WriteStartElementComputed(string tagName, string ns)
	{
		WriteStartComputed(XPathNodeType.Element, tagName, ns);
	}

	public void WriteStartElementComputed(XPathNavigator navigator)
	{
		WriteStartComputed(XPathNodeType.Element, navigator);
	}

	public void WriteStartElementComputed(XmlQualifiedName name)
	{
		WriteStartComputed(XPathNodeType.Element, name);
	}

	public void WriteStartAttributeComputed(string tagName, int prefixMappingsIndex)
	{
		WriteStartComputed(XPathNodeType.Attribute, tagName, prefixMappingsIndex);
	}

	public void WriteStartAttributeComputed(string tagName, string ns)
	{
		WriteStartComputed(XPathNodeType.Attribute, tagName, ns);
	}

	public void WriteStartAttributeComputed(XPathNavigator navigator)
	{
		WriteStartComputed(XPathNodeType.Attribute, navigator);
	}

	public void WriteStartAttributeComputed(XmlQualifiedName name)
	{
		WriteStartComputed(XPathNodeType.Attribute, name);
	}

	public void WriteNamespaceDeclaration(string prefix, string ns)
	{
		ConstructInEnumAttrs(XPathNodeType.Namespace);
		if (nsmgr == null)
		{
			WriteNamespaceDeclarationUnchecked(prefix, ns);
		}
		else
		{
			string text = nsmgr.LookupNamespace(prefix);
			if (ns != text)
			{
				if (text != null && usedPrefixes.ContainsKey(prefix))
				{
					throw new XslTransformException("Cannot construct namespace declaration xmlns{0}{1}='{2}'. Prefix '{1}' is already mapped to namespace '{3}'.", (prefix.Length == 0) ? "" : ":", prefix, ns, text);
				}
				AddNamespace(prefix, ns);
			}
		}
		if (depth == 0)
		{
			EndTree();
		}
		usedPrefixes[prefix] = ns;
	}

	public void WriteStartNamespace(string prefix)
	{
		ConstructInEnumAttrs(XPathNodeType.Namespace);
		piTarget = prefix;
		nodeText.Clear();
		xstate = XmlState.WithinNmsp;
		depth++;
	}

	public void WriteNamespaceString(string text)
	{
		nodeText.ConcatNoDelimiter(text);
	}

	public void WriteEndNamespace()
	{
		xstate = XmlState.EnumAttrs;
		depth--;
		WriteNamespaceDeclaration(piTarget, nodeText.GetResult());
		if (depth == 0)
		{
			EndTree();
		}
	}

	public void WriteStartComment()
	{
		ConstructWithinContent(XPathNodeType.Comment);
		nodeText.Clear();
		xstate = XmlState.WithinComment;
		depth++;
	}

	public void WriteCommentString(string text)
	{
		nodeText.ConcatNoDelimiter(text);
	}

	public void WriteEndComment()
	{
		Writer.WriteComment(nodeText.GetResult());
		xstate = XmlState.WithinContent;
		depth--;
		if (depth == 0)
		{
			EndTree();
		}
	}

	public void WriteStartProcessingInstruction(string target)
	{
		ConstructWithinContent(XPathNodeType.ProcessingInstruction);
		ValidateNames.ValidateNameThrow("", target, "", XPathNodeType.ProcessingInstruction, ValidateNames.Flags.AllExceptPrefixMapping);
		piTarget = target;
		nodeText.Clear();
		xstate = XmlState.WithinPI;
		depth++;
	}

	public void WriteProcessingInstructionString(string text)
	{
		nodeText.ConcatNoDelimiter(text);
	}

	public void WriteEndProcessingInstruction()
	{
		Writer.WriteProcessingInstruction(piTarget, nodeText.GetResult());
		xstate = XmlState.WithinContent;
		depth--;
		if (depth == 0)
		{
			EndTree();
		}
	}

	public void WriteItem(XPathItem item)
	{
		if (item.IsNode)
		{
			XPathNavigator xPathNavigator = (XPathNavigator)item;
			if (xstate == XmlState.WithinSequence)
			{
				seqwrt.WriteItem(xPathNavigator);
			}
			else
			{
				CopyNode(xPathNavigator);
			}
		}
		else
		{
			seqwrt.WriteItem(item);
		}
	}

	public void XsltCopyOf(XPathNavigator navigator)
	{
		if (navigator is RtfNavigator rtfNavigator)
		{
			rtfNavigator.CopyToWriter(this);
		}
		else if (navigator.NodeType == XPathNodeType.Root)
		{
			if (navigator.MoveToFirstChild())
			{
				do
				{
					CopyNode(navigator);
				}
				while (navigator.MoveToNext());
				navigator.MoveToParent();
			}
		}
		else
		{
			CopyNode(navigator);
		}
	}

	public bool StartCopy(XPathNavigator navigator)
	{
		if (navigator.NodeType == XPathNodeType.Root)
		{
			return true;
		}
		if (StartCopy(navigator, callChk: true))
		{
			CopyNamespaces(navigator, XPathNamespaceScope.ExcludeXml);
			return true;
		}
		return false;
	}

	public void EndCopy(XPathNavigator navigator)
	{
		if (navigator.NodeType == XPathNodeType.Element)
		{
			WriteEndElement();
		}
	}

	private void AddNamespace(string prefix, string ns)
	{
		nsmgr.AddNamespace(prefix, ns);
		cntNmsp++;
		usedPrefixes[prefix] = ns;
	}

	private void WriteString(string text, bool disableOutputEscaping)
	{
		switch (xstate)
		{
		case XmlState.WithinSequence:
			StartTree(XPathNodeType.Text);
			goto case XmlState.WithinContent;
		case XmlState.WithinContent:
			if (disableOutputEscaping)
			{
				WriteRawUnchecked(text);
			}
			else
			{
				WriteStringUnchecked(text);
			}
			break;
		case XmlState.EnumAttrs:
			StartElementContentUnchecked();
			goto case XmlState.WithinContent;
		case XmlState.WithinAttr:
			WriteStringUnchecked(text);
			break;
		case XmlState.WithinNmsp:
			WriteNamespaceString(text);
			break;
		case XmlState.WithinComment:
			WriteCommentString(text);
			break;
		case XmlState.WithinPI:
			WriteProcessingInstructionString(text);
			break;
		}
		if (depth == 0)
		{
			EndTree();
		}
	}

	private void CopyNode(XPathNavigator navigator)
	{
		int num = depth;
		while (true)
		{
			if (StartCopy(navigator, depth == num))
			{
				_ = navigator.NodeType;
				if (navigator.MoveToFirstAttribute())
				{
					do
					{
						StartCopy(navigator, callChk: false);
					}
					while (navigator.MoveToNextAttribute());
					navigator.MoveToParent();
				}
				CopyNamespaces(navigator, (depth - 1 == num) ? XPathNamespaceScope.ExcludeXml : XPathNamespaceScope.Local);
				StartElementContentUnchecked();
				if (navigator.MoveToFirstChild())
				{
					continue;
				}
				EndCopy(navigator, depth - 1 == num);
			}
			while (true)
			{
				if (depth == num)
				{
					return;
				}
				if (navigator.MoveToNext())
				{
					break;
				}
				navigator.MoveToParent();
				EndCopy(navigator, depth - 1 == num);
			}
		}
	}

	private bool StartCopy(XPathNavigator navigator, bool callChk)
	{
		bool result = false;
		switch (navigator.NodeType)
		{
		case XPathNodeType.Element:
			if (callChk)
			{
				WriteStartElement(navigator.Prefix, navigator.LocalName, navigator.NamespaceURI);
			}
			else
			{
				WriteStartElementUnchecked(navigator.Prefix, navigator.LocalName, navigator.NamespaceURI);
			}
			result = true;
			break;
		case XPathNodeType.Attribute:
			if (callChk)
			{
				WriteStartAttribute(navigator.Prefix, navigator.LocalName, navigator.NamespaceURI);
			}
			else
			{
				WriteStartAttributeUnchecked(navigator.Prefix, navigator.LocalName, navigator.NamespaceURI);
			}
			WriteString(navigator.Value);
			if (callChk)
			{
				WriteEndAttribute();
			}
			else
			{
				WriteEndAttributeUnchecked();
			}
			break;
		case XPathNodeType.Namespace:
			if (callChk)
			{
				if (Writer is XmlAttributeCache { Count: not 0 })
				{
					throw new XslTransformException("Namespace nodes cannot be added to the parent element after an attribute node has already been added.", string.Empty);
				}
				WriteNamespaceDeclaration(navigator.LocalName, navigator.Value);
			}
			else
			{
				WriteNamespaceDeclarationUnchecked(navigator.LocalName, navigator.Value);
			}
			break;
		case XPathNodeType.Text:
		case XPathNodeType.SignificantWhitespace:
		case XPathNodeType.Whitespace:
			if (callChk)
			{
				WriteString(navigator.Value, disableOutputEscaping: false);
			}
			else
			{
				WriteStringUnchecked(navigator.Value);
			}
			break;
		case XPathNodeType.Root:
			ThrowInvalidStateError(XPathNodeType.Root);
			break;
		case XPathNodeType.Comment:
			WriteStartComment();
			WriteCommentString(navigator.Value);
			WriteEndComment();
			break;
		case XPathNodeType.ProcessingInstruction:
			WriteStartProcessingInstruction(navigator.LocalName);
			WriteProcessingInstructionString(navigator.Value);
			WriteEndProcessingInstruction();
			break;
		}
		return result;
	}

	private void EndCopy(XPathNavigator navigator, bool callChk)
	{
		if (callChk)
		{
			WriteEndElement();
		}
		else
		{
			WriteEndElementUnchecked(navigator.Prefix, navigator.LocalName, navigator.NamespaceURI);
		}
	}

	private void CopyNamespaces(XPathNavigator navigator, XPathNamespaceScope nsScope)
	{
		if (navigator.NamespaceURI.Length == 0)
		{
			WriteNamespaceDeclarationUnchecked(string.Empty, string.Empty);
		}
		if (navigator.MoveToFirstNamespace(nsScope))
		{
			CopyNamespacesHelper(navigator, nsScope);
			navigator.MoveToParent();
		}
	}

	private void CopyNamespacesHelper(XPathNavigator navigator, XPathNamespaceScope nsScope)
	{
		string localName = navigator.LocalName;
		string value = navigator.Value;
		if (navigator.MoveToNextNamespace(nsScope))
		{
			CopyNamespacesHelper(navigator, nsScope);
		}
		WriteNamespaceDeclarationUnchecked(localName, value);
	}

	private void ConstructWithinContent(XPathNodeType rootType)
	{
		switch (xstate)
		{
		case XmlState.WithinSequence:
			StartTree(rootType);
			xstate = XmlState.WithinContent;
			break;
		case XmlState.EnumAttrs:
			StartElementContentUnchecked();
			break;
		default:
			ThrowInvalidStateError(rootType);
			break;
		case XmlState.WithinContent:
			break;
		}
	}

	private void ConstructInEnumAttrs(XPathNodeType rootType)
	{
		switch (xstate)
		{
		case XmlState.WithinSequence:
			StartTree(rootType);
			xstate = XmlState.EnumAttrs;
			break;
		default:
			ThrowInvalidStateError(rootType);
			break;
		case XmlState.EnumAttrs:
			break;
		}
	}

	private void WriteCachedNamespaces()
	{
		while (cntNmsp != 0)
		{
			cntNmsp--;
			nsmgr.GetNamespaceDeclaration(cntNmsp, out var prefix, out var uri);
			Writer.WriteNamespaceDeclaration(prefix, uri);
		}
	}

	private XPathNodeType XmlStateToNodeType(XmlState xstate)
	{
		return xstate switch
		{
			XmlState.EnumAttrs => XPathNodeType.Element, 
			XmlState.WithinContent => XPathNodeType.Element, 
			XmlState.WithinAttr => XPathNodeType.Attribute, 
			XmlState.WithinComment => XPathNodeType.Comment, 
			XmlState.WithinPI => XPathNodeType.ProcessingInstruction, 
			_ => XPathNodeType.Element, 
		};
	}

	private string CheckAttributePrefix(string prefix, string ns)
	{
		if (nsmgr == null)
		{
			WriteNamespaceDeclarationUnchecked(prefix, ns);
		}
		else
		{
			while (true)
			{
				string text = nsmgr.LookupNamespace(prefix);
				if (!(text != ns))
				{
					break;
				}
				if (text != null)
				{
					prefix = RemapPrefix(prefix, ns, isElemPrefix: false);
					continue;
				}
				AddNamespace(prefix, ns);
				break;
			}
		}
		return prefix;
	}

	private string RemapPrefix(string prefix, string ns, bool isElemPrefix)
	{
		if (conflictPrefixes == null)
		{
			conflictPrefixes = new Dictionary<string, string>(16);
		}
		if (nsmgr == null)
		{
			nsmgr = new XmlNamespaceManager(runtime.NameTable);
			nsmgr.PushScope();
		}
		string value = nsmgr.LookupPrefix(ns);
		if ((value == null || (!isElemPrefix && value.Length == 0)) && (!conflictPrefixes.TryGetValue(ns, out value) || !(value != prefix) || (!isElemPrefix && value.Length == 0)))
		{
			value = "xp_" + prefixIndex++.ToString(CultureInfo.InvariantCulture);
		}
		conflictPrefixes[ns] = value;
		return value;
	}

	private void WriteStartComputed(XPathNodeType nodeType, string tagName, int prefixMappingsIndex)
	{
		runtime.ParseTagName(tagName, prefixMappingsIndex, out var prefix, out var localName, out var ns);
		prefix = EnsureValidName(prefix, localName, ns, nodeType);
		if (nodeType == XPathNodeType.Element)
		{
			WriteStartElement(prefix, localName, ns);
		}
		else
		{
			WriteStartAttribute(prefix, localName, ns);
		}
	}

	private void WriteStartComputed(XPathNodeType nodeType, string tagName, string ns)
	{
		ValidateNames.ParseQNameThrow(tagName, out var prefix, out var localName);
		prefix = EnsureValidName(prefix, localName, ns, nodeType);
		if (nodeType == XPathNodeType.Element)
		{
			WriteStartElement(prefix, localName, ns);
		}
		else
		{
			WriteStartAttribute(prefix, localName, ns);
		}
	}

	private void WriteStartComputed(XPathNodeType nodeType, XPathNavigator navigator)
	{
		string prefix = navigator.Prefix;
		string localName = navigator.LocalName;
		string namespaceURI = navigator.NamespaceURI;
		if (navigator.NodeType != nodeType)
		{
			prefix = EnsureValidName(prefix, localName, namespaceURI, nodeType);
		}
		if (nodeType == XPathNodeType.Element)
		{
			WriteStartElement(prefix, localName, namespaceURI);
		}
		else
		{
			WriteStartAttribute(prefix, localName, namespaceURI);
		}
	}

	private void WriteStartComputed(XPathNodeType nodeType, XmlQualifiedName name)
	{
		string prefix = ((name.Namespace.Length != 0) ? RemapPrefix(string.Empty, name.Namespace, nodeType == XPathNodeType.Element) : string.Empty);
		prefix = EnsureValidName(prefix, name.Name, name.Namespace, nodeType);
		if (nodeType == XPathNodeType.Element)
		{
			WriteStartElement(prefix, name.Name, name.Namespace);
		}
		else
		{
			WriteStartAttribute(prefix, name.Name, name.Namespace);
		}
	}

	private string EnsureValidName(string prefix, string localName, string ns, XPathNodeType nodeType)
	{
		if (!ValidateNames.ValidateName(prefix, localName, ns, nodeType, ValidateNames.Flags.AllExceptNCNames))
		{
			prefix = ((ns.Length != 0) ? RemapPrefix(string.Empty, ns, nodeType == XPathNodeType.Element) : string.Empty);
			ValidateNames.ValidateNameThrow(prefix, localName, ns, nodeType, ValidateNames.Flags.AllExceptNCNames);
		}
		return prefix;
	}

	private void PushElementNames(string prefix, string localName, string ns)
	{
		if (stkNames == null)
		{
			stkNames = new Stack<string>(15);
		}
		stkNames.Push(prefix);
		stkNames.Push(localName);
		stkNames.Push(ns);
	}

	private void PopElementNames(out string prefix, out string localName, out string ns)
	{
		ns = stkNames.Pop();
		localName = stkNames.Pop();
		prefix = stkNames.Pop();
	}

	private void ThrowInvalidStateError(XPathNodeType constructorType)
	{
		switch (constructorType)
		{
		case XPathNodeType.Root:
		case XPathNodeType.Element:
		case XPathNodeType.Text:
		case XPathNodeType.ProcessingInstruction:
		case XPathNodeType.Comment:
			throw new XslTransformException("An item of type '{0}' cannot be constructed within a node of type '{1}'.", constructorType.ToString(), XmlStateToNodeType(xstate).ToString());
		case XPathNodeType.Attribute:
		case XPathNodeType.Namespace:
			if (depth == 1)
			{
				throw new XslTransformException("An item of type '{0}' cannot be constructed within a node of type '{1}'.", constructorType.ToString(), rootType.ToString());
			}
			if (xstate == XmlState.WithinContent)
			{
				throw new XslTransformException("Attribute and namespace nodes cannot be added to the parent element after a text, comment, pi, or sub-element node has already been added.", string.Empty);
			}
			goto case XPathNodeType.Root;
		default:
			throw new XslTransformException("An item of type '{0}' cannot be constructed within a node of type '{1}'.", "Unknown", XmlStateToNodeType(xstate).ToString());
		}
	}
}
