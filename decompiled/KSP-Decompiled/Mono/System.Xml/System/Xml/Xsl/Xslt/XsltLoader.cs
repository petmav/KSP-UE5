using System.Collections.Generic;
using System.Collections.Specialized;
using System.IO;
using System.Reflection;
using System.Text;
using System.Xml.XmlConfiguration;
using System.Xml.XPath;
using System.Xml.Xsl.Qil;

namespace System.Xml.Xsl.Xslt;

internal class XsltLoader : IErrorHelper
{
	private enum InstructionFlags
	{
		None = 0,
		AllowParam = 1,
		AllowSort = 2,
		AllowFallback = 4
	}

	private Compiler compiler;

	private XmlResolver xmlResolver;

	private QueryReaderSettings readerSettings;

	private KeywordsTable atoms;

	private XsltInput input;

	private Stylesheet curStylesheet;

	private Template curTemplate;

	private object curFunction;

	internal static QilName nullMode = AstFactory.QName(string.Empty);

	public static int V1Opt = 1;

	public static int V1Req = 2;

	public static int V2Opt = 4;

	public static int V2Req = 8;

	private HybridDictionary documentUriInUse = new HybridDictionary();

	private XsltInput.XsltAttribute[] stylesheetAttributes = new XsltInput.XsltAttribute[4]
	{
		new XsltInput.XsltAttribute("version", V1Req | V2Req),
		new XsltInput.XsltAttribute("id", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("default-validation", V2Opt),
		new XsltInput.XsltAttribute("input-type-annotations", V2Opt)
	};

	private XsltInput.XsltAttribute[] importIncludeAttributes = new XsltInput.XsltAttribute[1]
	{
		new XsltInput.XsltAttribute("href", V1Req | V2Req)
	};

	private XsltInput.XsltAttribute[] loadStripSpaceAttributes = new XsltInput.XsltAttribute[1]
	{
		new XsltInput.XsltAttribute("elements", V1Req | V2Req)
	};

	private XsltInput.XsltAttribute[] outputAttributes = new XsltInput.XsltAttribute[17]
	{
		new XsltInput.XsltAttribute("name", V2Opt),
		new XsltInput.XsltAttribute("method", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("byte-order-mark", V2Opt),
		new XsltInput.XsltAttribute("cdata-section-elements", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("doctype-public", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("doctype-system", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("encoding", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("escape-uri-attributes", V2Opt),
		new XsltInput.XsltAttribute("include-content-type", V2Opt),
		new XsltInput.XsltAttribute("indent", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("media-type", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("normalization-form", V2Opt),
		new XsltInput.XsltAttribute("omit-xml-declaration", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("standalone", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("undeclare-prefixes", V2Opt),
		new XsltInput.XsltAttribute("use-character-maps", V2Opt),
		new XsltInput.XsltAttribute("version", V1Opt | V2Opt)
	};

	private XsltInput.XsltAttribute[] keyAttributes = new XsltInput.XsltAttribute[4]
	{
		new XsltInput.XsltAttribute("name", V1Req | V2Req),
		new XsltInput.XsltAttribute("match", V1Req | V2Req),
		new XsltInput.XsltAttribute("use", V1Req | V2Opt),
		new XsltInput.XsltAttribute("collation", V2Opt)
	};

	private XsltInput.XsltAttribute[] decimalFormatAttributes = new XsltInput.XsltAttribute[11]
	{
		new XsltInput.XsltAttribute("name", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("infinity", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("NaN", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("decimal-separator", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("grouping-separator", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("percent", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("per-mille", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("zero-digit", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("digit", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("pattern-separator", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("minus-sign", V1Opt | V2Opt)
	};

	private XsltInput.XsltAttribute[] namespaceAliasAttributes = new XsltInput.XsltAttribute[2]
	{
		new XsltInput.XsltAttribute("stylesheet-prefix", V1Req | V2Req),
		new XsltInput.XsltAttribute("result-prefix", V1Req | V2Req)
	};

	private XsltInput.XsltAttribute[] attributeSetAttributes = new XsltInput.XsltAttribute[2]
	{
		new XsltInput.XsltAttribute("name", V1Req | V2Req),
		new XsltInput.XsltAttribute("use-attribute-sets", V1Opt | V2Opt)
	};

	private XsltInput.XsltAttribute[] templateAttributes = new XsltInput.XsltAttribute[5]
	{
		new XsltInput.XsltAttribute("match", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("name", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("priority", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("mode", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("as", V2Opt)
	};

	private XsltInput.XsltAttribute[] scriptAttributes = new XsltInput.XsltAttribute[2]
	{
		new XsltInput.XsltAttribute("implements-prefix", V1Req | V2Req),
		new XsltInput.XsltAttribute("language", V1Opt | V2Opt)
	};

	private XsltInput.XsltAttribute[] assemblyAttributes = new XsltInput.XsltAttribute[2]
	{
		new XsltInput.XsltAttribute("name", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("href", V1Opt | V2Opt)
	};

	private XsltInput.XsltAttribute[] usingAttributes = new XsltInput.XsltAttribute[1]
	{
		new XsltInput.XsltAttribute("namespace", V1Req | V2Req)
	};

	private const int MAX_LOADINSTRUCTIONS_DEPTH = 1024;

	private int loadInstructionsDepth;

	private XsltInput.XsltAttribute[] applyTemplatesAttributes = new XsltInput.XsltAttribute[2]
	{
		new XsltInput.XsltAttribute("select", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("mode", V1Opt | V2Opt)
	};

	private XsltInput.XsltAttribute[] callTemplateAttributes = new XsltInput.XsltAttribute[1]
	{
		new XsltInput.XsltAttribute("name", V1Req | V2Req)
	};

	private XsltInput.XsltAttribute[] copyAttributes = new XsltInput.XsltAttribute[5]
	{
		new XsltInput.XsltAttribute("copy-namespaces", V2Opt),
		new XsltInput.XsltAttribute("inherit-namespaces", V2Opt),
		new XsltInput.XsltAttribute("use-attribute-sets", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("type", V2Opt),
		new XsltInput.XsltAttribute("validation", V2Opt)
	};

	private XsltInput.XsltAttribute[] copyOfAttributes = new XsltInput.XsltAttribute[4]
	{
		new XsltInput.XsltAttribute("select", V1Req | V2Req),
		new XsltInput.XsltAttribute("copy-namespaces", V2Opt),
		new XsltInput.XsltAttribute("type", V2Opt),
		new XsltInput.XsltAttribute("validation", V2Opt)
	};

	private XsltInput.XsltAttribute[] ifAttributes = new XsltInput.XsltAttribute[1]
	{
		new XsltInput.XsltAttribute("test", V1Req | V2Req)
	};

	private XsltInput.XsltAttribute[] forEachAttributes = new XsltInput.XsltAttribute[1]
	{
		new XsltInput.XsltAttribute("select", V1Req | V2Req)
	};

	private XsltInput.XsltAttribute[] messageAttributes = new XsltInput.XsltAttribute[2]
	{
		new XsltInput.XsltAttribute("select", V2Opt),
		new XsltInput.XsltAttribute("terminate", V1Opt | V2Opt)
	};

	private XsltInput.XsltAttribute[] numberAttributes = new XsltInput.XsltAttribute[11]
	{
		new XsltInput.XsltAttribute("value", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("select", V2Opt),
		new XsltInput.XsltAttribute("level", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("count", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("from", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("format", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("lang", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("letter-value", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("ordinal", V2Opt),
		new XsltInput.XsltAttribute("grouping-separator", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("grouping-size", V1Opt | V2Opt)
	};

	private XsltInput.XsltAttribute[] valueOfAttributes = new XsltInput.XsltAttribute[3]
	{
		new XsltInput.XsltAttribute("select", V1Req | V2Opt),
		new XsltInput.XsltAttribute("separator", V2Opt),
		new XsltInput.XsltAttribute("disable-output-escaping", V1Opt | V2Opt)
	};

	private XsltInput.XsltAttribute[] variableAttributes = new XsltInput.XsltAttribute[5]
	{
		new XsltInput.XsltAttribute("name", V1Req | V2Req),
		new XsltInput.XsltAttribute("select", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("as", V2Opt),
		new XsltInput.XsltAttribute("required", 0),
		new XsltInput.XsltAttribute("tunnel", 0)
	};

	private XsltInput.XsltAttribute[] paramAttributes = new XsltInput.XsltAttribute[5]
	{
		new XsltInput.XsltAttribute("name", V1Req | V2Req),
		new XsltInput.XsltAttribute("select", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("as", V2Opt),
		new XsltInput.XsltAttribute("required", V2Opt),
		new XsltInput.XsltAttribute("tunnel", V2Opt)
	};

	private XsltInput.XsltAttribute[] withParamAttributes = new XsltInput.XsltAttribute[5]
	{
		new XsltInput.XsltAttribute("name", V1Req | V2Req),
		new XsltInput.XsltAttribute("select", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("as", V2Opt),
		new XsltInput.XsltAttribute("required", 0),
		new XsltInput.XsltAttribute("tunnel", V2Opt)
	};

	private XsltInput.XsltAttribute[] commentAttributes = new XsltInput.XsltAttribute[1]
	{
		new XsltInput.XsltAttribute("select", V2Opt)
	};

	private XsltInput.XsltAttribute[] processingInstructionAttributes = new XsltInput.XsltAttribute[2]
	{
		new XsltInput.XsltAttribute("name", V1Req | V2Req),
		new XsltInput.XsltAttribute("select", V2Opt)
	};

	private XsltInput.XsltAttribute[] textAttributes = new XsltInput.XsltAttribute[1]
	{
		new XsltInput.XsltAttribute("disable-output-escaping", V1Opt | V2Opt)
	};

	private XsltInput.XsltAttribute[] elementAttributes = new XsltInput.XsltAttribute[6]
	{
		new XsltInput.XsltAttribute("name", V1Req | V2Req),
		new XsltInput.XsltAttribute("namespace", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("inherit-namespaces", V2Opt),
		new XsltInput.XsltAttribute("use-attribute-sets", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("type", V2Opt),
		new XsltInput.XsltAttribute("validation", V2Opt)
	};

	private XsltInput.XsltAttribute[] attributeAttributes = new XsltInput.XsltAttribute[6]
	{
		new XsltInput.XsltAttribute("name", V1Req | V2Req),
		new XsltInput.XsltAttribute("namespace", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("select", V2Opt),
		new XsltInput.XsltAttribute("separator", V2Opt),
		new XsltInput.XsltAttribute("type", V2Opt),
		new XsltInput.XsltAttribute("validation", V2Opt)
	};

	private XsltInput.XsltAttribute[] sortAttributes = new XsltInput.XsltAttribute[7]
	{
		new XsltInput.XsltAttribute("select", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("lang", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("order", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("collation", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("stable", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("case-order", V1Opt | V2Opt),
		new XsltInput.XsltAttribute("data-type", V1Opt | V2Opt)
	};

	private bool V1 => compiler.Version == 1;

	public void Load(Compiler compiler, object stylesheet, XmlResolver xmlResolver)
	{
		this.compiler = compiler;
		this.xmlResolver = xmlResolver ?? XmlNullResolver.Singleton;
		if (stylesheet is XmlReader reader)
		{
			readerSettings = new QueryReaderSettings(reader);
			Load(reader);
		}
		else if (stylesheet is string text)
		{
			XmlResolver xmlResolver2 = xmlResolver;
			if (xmlResolver == null || xmlResolver == XmlNullResolver.Singleton)
			{
				xmlResolver2 = new XmlUrlResolver();
			}
			Uri uri = xmlResolver2.ResolveUri(null, text);
			if (uri == null)
			{
				throw new XslLoadException("Cannot resolve the referenced document '{0}'.", text);
			}
			readerSettings = new QueryReaderSettings(new NameTable());
			XmlReader reader2;
			using (reader2 = CreateReader(uri, xmlResolver2))
			{
				Load(reader2);
			}
		}
		else if (stylesheet is IXPathNavigable iXPathNavigable)
		{
			XmlReader reader2 = XPathNavigatorReader.Create(iXPathNavigable.CreateNavigator());
			readerSettings = new QueryReaderSettings(reader2.NameTable);
			Load(reader2);
		}
		compiler.StartApplyTemplates = AstFactory.ApplyTemplates(nullMode);
		ProcessOutputSettings();
		foreach (AttributeSet value in compiler.AttributeSets.Values)
		{
			CheckAttributeSetsDfs(value);
		}
	}

	private void Load(XmlReader reader)
	{
		atoms = new KeywordsTable(reader.NameTable);
		AtomizeAttributes();
		LoadStylesheet(reader, include: false);
	}

	private void AtomizeAttributes(XsltInput.XsltAttribute[] attributes)
	{
		for (int i = 0; i < attributes.Length; i++)
		{
			attributes[i].name = atoms.NameTable.Add(attributes[i].name);
		}
	}

	private void AtomizeAttributes()
	{
		AtomizeAttributes(stylesheetAttributes);
		AtomizeAttributes(importIncludeAttributes);
		AtomizeAttributes(loadStripSpaceAttributes);
		AtomizeAttributes(outputAttributes);
		AtomizeAttributes(keyAttributes);
		AtomizeAttributes(decimalFormatAttributes);
		AtomizeAttributes(namespaceAliasAttributes);
		AtomizeAttributes(attributeSetAttributes);
		AtomizeAttributes(templateAttributes);
		AtomizeAttributes(scriptAttributes);
		AtomizeAttributes(assemblyAttributes);
		AtomizeAttributes(usingAttributes);
		AtomizeAttributes(applyTemplatesAttributes);
		AtomizeAttributes(callTemplateAttributes);
		AtomizeAttributes(copyAttributes);
		AtomizeAttributes(copyOfAttributes);
		AtomizeAttributes(ifAttributes);
		AtomizeAttributes(forEachAttributes);
		AtomizeAttributes(messageAttributes);
		AtomizeAttributes(numberAttributes);
		AtomizeAttributes(valueOfAttributes);
		AtomizeAttributes(variableAttributes);
		AtomizeAttributes(paramAttributes);
		AtomizeAttributes(withParamAttributes);
		AtomizeAttributes(commentAttributes);
		AtomizeAttributes(processingInstructionAttributes);
		AtomizeAttributes(textAttributes);
		AtomizeAttributes(elementAttributes);
		AtomizeAttributes(attributeAttributes);
		AtomizeAttributes(sortAttributes);
	}

	private Uri ResolveUri(string relativeUri, string baseUri)
	{
		Uri baseUri2 = ((baseUri.Length != 0) ? xmlResolver.ResolveUri(null, baseUri) : null);
		Uri uri = xmlResolver.ResolveUri(baseUri2, relativeUri);
		if (uri == null)
		{
			throw new XslLoadException("Cannot resolve the referenced document '{0}'.", relativeUri);
		}
		return uri;
	}

	private XmlReader CreateReader(Uri uri, XmlResolver xmlResolver)
	{
		object entity = xmlResolver.GetEntity(uri, null, null);
		if (entity is Stream stream)
		{
			return readerSettings.CreateReader(stream, uri.ToString());
		}
		if (entity is XmlReader result)
		{
			return result;
		}
		if (entity is IXPathNavigable iXPathNavigable)
		{
			return XPathNavigatorReader.Create(iXPathNavigable.CreateNavigator());
		}
		throw new XslLoadException("Cannot load the stylesheet object referenced by URI '{0}', because the provided XmlResolver returned an object of type '{1}'. One of Stream, XmlReader, and IXPathNavigable types was expected.", uri.ToString(), (entity == null) ? "null" : entity.GetType().ToString());
	}

	private Stylesheet LoadStylesheet(Uri uri, bool include)
	{
		using XmlReader reader = CreateReader(uri, xmlResolver);
		return LoadStylesheet(reader, include);
	}

	private Stylesheet LoadStylesheet(XmlReader reader, bool include)
	{
		string baseURI = reader.BaseURI;
		documentUriInUse.Add(baseURI, null);
		compiler.AddModule(baseURI);
		Stylesheet stylesheet = curStylesheet;
		XsltInput xsltInput = input;
		Stylesheet result = (include ? curStylesheet : compiler.CreateStylesheet());
		input = new XsltInput(reader, compiler, atoms);
		curStylesheet = result;
		try
		{
			LoadDocument();
			if (!include)
			{
				compiler.MergeWithStylesheet(curStylesheet);
				List<Uri> importHrefs = curStylesheet.ImportHrefs;
				curStylesheet.Imports = new Stylesheet[importHrefs.Count];
				int num = importHrefs.Count;
				while (0 <= --num)
				{
					curStylesheet.Imports[num] = LoadStylesheet(importHrefs[num], include: false);
				}
			}
		}
		catch (XslLoadException)
		{
			throw;
		}
		catch (Exception ex2)
		{
			if (!XmlException.IsCatchableException(ex2))
			{
				throw;
			}
			ISourceLineInfo sourceLineInfo;
			if (!(ex2 is XmlException { SourceUri: not null } ex3))
			{
				sourceLineInfo = input.BuildReaderLineInfo();
			}
			else
			{
				ISourceLineInfo sourceLineInfo2 = new SourceLineInfo(ex3.SourceUri, ex3.LineNumber, ex3.LinePosition, ex3.LineNumber, ex3.LinePosition);
				sourceLineInfo = sourceLineInfo2;
			}
			ISourceLineInfo lineInfo = sourceLineInfo;
			throw new XslLoadException(ex2, lineInfo);
		}
		finally
		{
			documentUriInUse.Remove(baseURI);
			input = xsltInput;
			curStylesheet = stylesheet;
		}
		return result;
	}

	private void LoadDocument()
	{
		if (!input.FindStylesheetElement())
		{
			ReportError("Stylesheet must start either with an 'xsl:stylesheet' or an 'xsl:transform' element, or with a literal result element that has an 'xsl:version' attribute, where prefix 'xsl' denotes the 'http://www.w3.org/1999/XSL/Transform' namespace.");
			return;
		}
		if (input.IsXsltNamespace())
		{
			if (input.IsKeyword(atoms.Stylesheet) || input.IsKeyword(atoms.Transform))
			{
				LoadRealStylesheet();
			}
			else
			{
				ReportError("Stylesheet must start either with an 'xsl:stylesheet' or an 'xsl:transform' element, or with a literal result element that has an 'xsl:version' attribute, where prefix 'xsl' denotes the 'http://www.w3.org/1999/XSL/Transform' namespace.");
				input.SkipNode();
			}
		}
		else
		{
			LoadSimplifiedStylesheet();
		}
		input.Finish();
	}

	private void LoadSimplifiedStylesheet()
	{
		curTemplate = AstFactory.Template(null, "/", nullMode, double.NaN, input.XslVersion);
		input.CanHaveApplyImports = true;
		XslNode xslNode = LoadLiteralResultElement(asStylesheet: true);
		if (xslNode != null)
		{
			SetLineInfo(curTemplate, xslNode.SourceLine);
			List<XslNode> list = new List<XslNode>();
			list.Add(xslNode);
			SetContent(curTemplate, list);
			curStylesheet.AddTemplate(curTemplate);
		}
		curTemplate = null;
	}

	private void LoadRealStylesheet()
	{
		XsltInput.ContextInfo attributes = input.GetAttributes(stylesheetAttributes);
		ParseValidationAttribute(2, defVal: true);
		ParseInputTypeAnnotationsAttribute(3);
		XsltInput.DelayedQName elementName = input.ElementName;
		if (!input.MoveToFirstChild())
		{
			return;
		}
		bool flag = true;
		do
		{
			bool flag2 = false;
			switch (input.NodeType)
			{
			case XmlNodeType.Element:
				if (input.IsXsltNamespace())
				{
					if (input.IsKeyword(atoms.Import))
					{
						if (!flag)
						{
							ReportError("'{0}' element children must precede all other children of the '{1}' element.", input.QualifiedName, elementName);
							input.SkipNode();
						}
						else
						{
							flag2 = true;
							LoadImport();
						}
					}
					else if (input.IsKeyword(atoms.Include))
					{
						LoadInclude();
					}
					else if (input.IsKeyword(atoms.StripSpace))
					{
						LoadStripSpace(attributes.nsList);
					}
					else if (input.IsKeyword(atoms.PreserveSpace))
					{
						LoadPreserveSpace(attributes.nsList);
					}
					else if (input.IsKeyword(atoms.Output))
					{
						LoadOutput();
					}
					else if (input.IsKeyword(atoms.Key))
					{
						LoadKey(attributes.nsList);
					}
					else if (input.IsKeyword(atoms.DecimalFormat))
					{
						LoadDecimalFormat(attributes.nsList);
					}
					else if (input.IsKeyword(atoms.NamespaceAlias))
					{
						LoadNamespaceAlias(attributes.nsList);
					}
					else if (input.IsKeyword(atoms.AttributeSet))
					{
						LoadAttributeSet(attributes.nsList);
					}
					else if (input.IsKeyword(atoms.Variable))
					{
						LoadGlobalVariableOrParameter(attributes.nsList, XslNodeType.Variable);
					}
					else if (input.IsKeyword(atoms.Param))
					{
						LoadGlobalVariableOrParameter(attributes.nsList, XslNodeType.Param);
					}
					else if (input.IsKeyword(atoms.Template))
					{
						LoadTemplate(attributes.nsList);
					}
					else
					{
						input.GetVersionAttribute();
						if (!input.ForwardCompatibility)
						{
							ReportError("'{0}' cannot be a child of the '{1}' element.", input.QualifiedName, elementName);
						}
						input.SkipNode();
					}
				}
				else if (input.IsNs(atoms.UrnMsxsl) && input.IsKeyword(atoms.Script))
				{
					LoadMsScript(attributes.nsList);
				}
				else
				{
					if (input.IsNullNamespace())
					{
						ReportError("Top-level element '{0}' may not have a null namespace URI.", input.LocalName);
					}
					input.SkipNode();
				}
				flag = flag2;
				break;
			default:
				ReportError("'{0}' element cannot have text node children.", elementName);
				break;
			case XmlNodeType.Whitespace:
			case XmlNodeType.SignificantWhitespace:
				break;
			}
		}
		while (input.MoveToNextSibling());
	}

	private void LoadImport()
	{
		input.GetAttributes(importIncludeAttributes);
		if (input.MoveToXsltAttribute(0, "href"))
		{
			Uri uri = ResolveUri(input.Value, input.BaseUri);
			if (documentUriInUse.Contains(uri.ToString()))
			{
				ReportError("Stylesheet '{0}' cannot directly or indirectly include or import itself.", input.Value);
			}
			else
			{
				curStylesheet.ImportHrefs.Add(uri);
			}
		}
		CheckNoContent();
	}

	private void LoadInclude()
	{
		input.GetAttributes(importIncludeAttributes);
		if (input.MoveToXsltAttribute(0, "href"))
		{
			Uri uri = ResolveUri(input.Value, input.BaseUri);
			if (documentUriInUse.Contains(uri.ToString()))
			{
				ReportError("Stylesheet '{0}' cannot directly or indirectly include or import itself.", input.Value);
			}
			else
			{
				LoadStylesheet(uri, include: true);
			}
		}
		CheckNoContent();
	}

	private void LoadStripSpace(NsDecl stylesheetNsList)
	{
		XsltInput.ContextInfo attributes = input.GetAttributes(loadStripSpaceAttributes);
		attributes.nsList = MergeNamespaces(attributes.nsList, stylesheetNsList);
		if (input.MoveToXsltAttribute(0, atoms.Elements))
		{
			ParseWhitespaceRules(input.Value, preserveSpace: false);
		}
		CheckNoContent();
	}

	private void LoadPreserveSpace(NsDecl stylesheetNsList)
	{
		XsltInput.ContextInfo attributes = input.GetAttributes(loadStripSpaceAttributes);
		attributes.nsList = MergeNamespaces(attributes.nsList, stylesheetNsList);
		if (input.MoveToXsltAttribute(0, atoms.Elements))
		{
			ParseWhitespaceRules(input.Value, preserveSpace: true);
		}
		CheckNoContent();
	}

	private void LoadOutput()
	{
		input.GetAttributes(outputAttributes);
		Output output = compiler.Output;
		XmlWriterSettings settings = output.Settings;
		int currentPrecedence = compiler.CurrentPrecedence;
		if (ParseQNameAttribute(0) != null)
		{
			ReportNYI("xsl:output/@name");
		}
		if (input.MoveToXsltAttribute(1, "method") && output.MethodPrec <= currentPrecedence)
		{
			compiler.EnterForwardsCompatible();
			XmlOutputMethod method;
			XmlQualifiedName xmlQualifiedName = ParseOutputMethod(input.Value, out method);
			if (compiler.ExitForwardsCompatible(input.ForwardCompatibility) && xmlQualifiedName != null)
			{
				if (currentPrecedence == output.MethodPrec && !output.Method.Equals(xmlQualifiedName))
				{
					ReportWarning("Attribute '{0}' of 'xsl:output' cannot be defined more than once with the same import precedence.", "method");
				}
				settings.OutputMethod = method;
				output.Method = xmlQualifiedName;
				output.MethodPrec = currentPrecedence;
			}
		}
		if (ParseYesNoAttribute(2, "byte-order-mark") != TriState.Unknown)
		{
			ReportNYI("xsl:output/@byte-order-mark");
		}
		if (input.MoveToXsltAttribute(3, "cdata-section-elements"))
		{
			compiler.EnterForwardsCompatible();
			string[] array = XmlConvert.SplitString(input.Value);
			List<XmlQualifiedName> list = new List<XmlQualifiedName>();
			for (int i = 0; i < array.Length; i++)
			{
				list.Add(ResolveQName(ignoreDefaultNs: false, array[i]));
			}
			if (compiler.ExitForwardsCompatible(input.ForwardCompatibility))
			{
				settings.CDataSectionElements.AddRange(list);
			}
		}
		if (input.MoveToXsltAttribute(4, "doctype-public") && output.DocTypePublicPrec <= currentPrecedence)
		{
			if (currentPrecedence == output.DocTypePublicPrec && settings.DocTypePublic != input.Value)
			{
				ReportWarning("Attribute '{0}' of 'xsl:output' cannot be defined more than once with the same import precedence.", "doctype-public");
			}
			settings.DocTypePublic = input.Value;
			output.DocTypePublicPrec = currentPrecedence;
		}
		if (input.MoveToXsltAttribute(5, "doctype-system") && output.DocTypeSystemPrec <= currentPrecedence)
		{
			if (currentPrecedence == output.DocTypeSystemPrec && settings.DocTypeSystem != input.Value)
			{
				ReportWarning("Attribute '{0}' of 'xsl:output' cannot be defined more than once with the same import precedence.", "doctype-system");
			}
			settings.DocTypeSystem = input.Value;
			output.DocTypeSystemPrec = currentPrecedence;
		}
		if (input.MoveToXsltAttribute(6, "encoding") && output.EncodingPrec <= currentPrecedence)
		{
			try
			{
				Encoding encoding = Encoding.GetEncoding(input.Value);
				if (currentPrecedence == output.EncodingPrec && output.Encoding != input.Value)
				{
					ReportWarning("Attribute '{0}' of 'xsl:output' cannot be defined more than once with the same import precedence.", "encoding");
				}
				settings.Encoding = encoding;
				output.Encoding = input.Value;
				output.EncodingPrec = currentPrecedence;
			}
			catch (ArgumentException)
			{
				if (!input.ForwardCompatibility)
				{
					ReportWarning("'{0}' is not a supported encoding name.", input.Value);
				}
			}
		}
		if (ParseYesNoAttribute(7, "escape-uri-attributes") == TriState.False)
		{
			ReportNYI("xsl:output/@escape-uri-attributes == flase()");
		}
		if (ParseYesNoAttribute(8, "include-content-type") == TriState.False)
		{
			ReportNYI("xsl:output/@include-content-type == flase()");
		}
		TriState triState = ParseYesNoAttribute(9, "indent");
		if (triState != TriState.Unknown && output.IndentPrec <= currentPrecedence)
		{
			bool flag = triState == TriState.True;
			if (currentPrecedence == output.IndentPrec && settings.Indent != flag)
			{
				ReportWarning("Attribute '{0}' of 'xsl:output' cannot be defined more than once with the same import precedence.", "indent");
			}
			settings.Indent = flag;
			output.IndentPrec = currentPrecedence;
		}
		if (input.MoveToXsltAttribute(10, "media-type") && output.MediaTypePrec <= currentPrecedence)
		{
			if (currentPrecedence == output.MediaTypePrec && settings.MediaType != input.Value)
			{
				ReportWarning("Attribute '{0}' of 'xsl:output' cannot be defined more than once with the same import precedence.", "media-type");
			}
			settings.MediaType = input.Value;
			output.MediaTypePrec = currentPrecedence;
		}
		if (input.MoveToXsltAttribute(11, "normalization-form"))
		{
			ReportNYI("xsl:output/@normalization-form");
		}
		triState = ParseYesNoAttribute(12, "omit-xml-declaration");
		if (triState != TriState.Unknown && output.OmitXmlDeclarationPrec <= currentPrecedence)
		{
			bool flag2 = triState == TriState.True;
			if (currentPrecedence == output.OmitXmlDeclarationPrec && settings.OmitXmlDeclaration != flag2)
			{
				ReportWarning("Attribute '{0}' of 'xsl:output' cannot be defined more than once with the same import precedence.", "omit-xml-declaration");
			}
			settings.OmitXmlDeclaration = flag2;
			output.OmitXmlDeclarationPrec = currentPrecedence;
		}
		triState = ParseYesNoAttribute(13, "standalone");
		if (triState != TriState.Unknown && output.StandalonePrec <= currentPrecedence)
		{
			XmlStandalone xmlStandalone = ((triState == TriState.True) ? XmlStandalone.Yes : XmlStandalone.No);
			if (currentPrecedence == output.StandalonePrec && settings.Standalone != xmlStandalone)
			{
				ReportWarning("Attribute '{0}' of 'xsl:output' cannot be defined more than once with the same import precedence.", "standalone");
			}
			settings.Standalone = xmlStandalone;
			output.StandalonePrec = currentPrecedence;
		}
		if (ParseYesNoAttribute(14, "undeclare-prefixes") == TriState.True)
		{
			ReportNYI("xsl:output/@undeclare-prefixes == true()");
		}
		if (ParseUseCharacterMaps(15).Count != 0)
		{
			ReportNYI("xsl:output/@use-character-maps");
		}
		if (input.MoveToXsltAttribute(16, "version") && output.VersionPrec <= currentPrecedence)
		{
			if (currentPrecedence == output.VersionPrec && output.Version != input.Value)
			{
				ReportWarning("Attribute '{0}' of 'xsl:output' cannot be defined more than once with the same import precedence.", "version");
			}
			output.Version = input.Value;
			output.VersionPrec = currentPrecedence;
		}
		CheckNoContent();
	}

	private void ProcessOutputSettings()
	{
		Output output = compiler.Output;
		XmlWriterSettings settings = output.Settings;
		if (settings.OutputMethod == XmlOutputMethod.Html && output.IndentPrec == int.MinValue)
		{
			settings.Indent = true;
		}
		if (output.MediaTypePrec == int.MinValue)
		{
			settings.MediaType = ((settings.OutputMethod == XmlOutputMethod.Xml) ? "text/xml" : ((settings.OutputMethod == XmlOutputMethod.Html) ? "text/html" : ((settings.OutputMethod == XmlOutputMethod.Text) ? "text/plain" : null)));
		}
	}

	private void CheckUseAttrubuteSetInList(IList<XslNode> list)
	{
		foreach (XslNode item in list)
		{
			switch (item.NodeType)
			{
			case XslNodeType.UseAttributeSet:
			{
				if (compiler.AttributeSets.TryGetValue(item.Name, out var value))
				{
					CheckAttributeSetsDfs(value);
				}
				break;
			}
			case XslNodeType.List:
				CheckUseAttrubuteSetInList(item.Content);
				break;
			}
		}
	}

	private void CheckAttributeSetsDfs(AttributeSet attSet)
	{
		switch (attSet.CycleCheck)
		{
		case CycleCheck.NotStarted:
			attSet.CycleCheck = CycleCheck.Processing;
			CheckUseAttrubuteSetInList(attSet.Content);
			attSet.CycleCheck = CycleCheck.Completed;
			break;
		default:
			compiler.ReportError(attSet.Content[0].SourceLine, "Circular reference in the definition of attribute set '{0}'.", attSet.Name.QualifiedName);
			break;
		case CycleCheck.Completed:
			break;
		}
	}

	private void LoadKey(NsDecl stylesheetNsList)
	{
		XsltInput.ContextInfo attributes = input.GetAttributes(keyAttributes);
		attributes.nsList = MergeNamespaces(attributes.nsList, stylesheetNsList);
		QilName qilName = ParseQNameAttribute(0);
		string match = ParseStringAttribute(1, "match");
		string text = ParseStringAttribute(2, "use");
		ParseCollationAttribute(3);
		input.MoveToElement();
		List<XslNode> list = null;
		if (V1)
		{
			if (text == null)
			{
				input.SkipNode();
			}
			else
			{
				CheckNoContent();
			}
		}
		else
		{
			list = LoadInstructions();
			if (list.Count != 0)
			{
				list = LoadEndTag(list);
			}
			if (text == null == (list.Count == 0))
			{
				ReportError("'xsl:key' has a 'use' attribute and has non-empty content, or it has empty content and no 'use' attribute.");
			}
			else if (text == null)
			{
				ReportNYI("xsl:key[count(@use) = 0]");
			}
		}
		Key item = (Key)SetInfo(AstFactory.Key(qilName, match, text, input.XslVersion), null, attributes);
		if (compiler.Keys.Contains(qilName))
		{
			compiler.Keys[qilName].Add(item);
			return;
		}
		List<Key> list2 = new List<Key>();
		list2.Add(item);
		compiler.Keys.Add(list2);
	}

	private void LoadDecimalFormat(NsDecl stylesheetNsList)
	{
		XsltInput.ContextInfo attributes = input.GetAttributes(decimalFormatAttributes);
		attributes.nsList = MergeNamespaces(attributes.nsList, stylesheetNsList);
		XmlQualifiedName xmlQualifiedName;
		if (input.MoveToXsltAttribute(0, "name"))
		{
			compiler.EnterForwardsCompatible();
			xmlQualifiedName = ResolveQName(ignoreDefaultNs: true, input.Value);
			if (!compiler.ExitForwardsCompatible(input.ForwardCompatibility))
			{
				xmlQualifiedName = new XmlQualifiedName();
			}
		}
		else
		{
			xmlQualifiedName = new XmlQualifiedName();
		}
		string text = DecimalFormatDecl.Default.InfinitySymbol;
		if (input.MoveToXsltAttribute(1, "infinity"))
		{
			text = input.Value;
		}
		string text2 = DecimalFormatDecl.Default.NanSymbol;
		if (input.MoveToXsltAttribute(2, "NaN"))
		{
			text2 = input.Value;
		}
		char[] characters = DecimalFormatDecl.Default.Characters;
		char[] array = new char[8];
		for (int i = 0; i < 8; i++)
		{
			array[i] = ParseCharAttribute(3 + i, decimalFormatAttributes[3 + i].name, characters[i]);
		}
		for (int j = 0; j < 7; j++)
		{
			for (int k = j + 1; k < 7; k++)
			{
				if (array[j] == array[k])
				{
					if (input.MoveToXsltAttribute(3 + k, decimalFormatAttributes[3 + k].name))
					{
						_ = 1;
					}
					else
						input.MoveToXsltAttribute(3 + j, decimalFormatAttributes[3 + j].name);
					ReportError("The '{0}' and '{1}' attributes of 'xsl:decimal-format' must have distinct values.", decimalFormatAttributes[3 + j].name, decimalFormatAttributes[3 + k].name);
					break;
				}
			}
		}
		if (compiler.DecimalFormats.Contains(xmlQualifiedName))
		{
			DecimalFormatDecl decimalFormatDecl = compiler.DecimalFormats[xmlQualifiedName];
			input.MoveToXsltAttribute(1, "infinity");
			CheckError(text != decimalFormatDecl.InfinitySymbol, "The '{0}' attribute of 'xsl:decimal-format' cannot be redefined with a value of '{1}'.", "infinity", text);
			input.MoveToXsltAttribute(2, "NaN");
			CheckError(text2 != decimalFormatDecl.NanSymbol, "The '{0}' attribute of 'xsl:decimal-format' cannot be redefined with a value of '{1}'.", "NaN", text2);
			for (int l = 0; l < 8; l++)
			{
				input.MoveToXsltAttribute(3 + l, decimalFormatAttributes[3 + l].name);
				CheckError(array[l] != decimalFormatDecl.Characters[l], "The '{0}' attribute of 'xsl:decimal-format' cannot be redefined with a value of '{1}'.", decimalFormatAttributes[3 + l].name, char.ToString(array[l]));
			}
		}
		else
		{
			DecimalFormatDecl item = new DecimalFormatDecl(xmlQualifiedName, text, text2, new string(array));
			compiler.DecimalFormats.Add(item);
		}
		CheckNoContent();
	}

	private void LoadNamespaceAlias(NsDecl stylesheetNsList)
	{
		XsltInput.ContextInfo attributes = input.GetAttributes(namespaceAliasAttributes);
		attributes.nsList = MergeNamespaces(attributes.nsList, stylesheetNsList);
		string text = null;
		string text2 = null;
		string text3 = null;
		if (input.MoveToXsltAttribute(0, "stylesheet-prefix"))
		{
			if (input.Value.Length == 0)
			{
				ReportError("The value of the '{0}' attribute cannot be empty. Use '#default' to specify the default namespace.", "stylesheet-prefix");
			}
			else
			{
				text = input.LookupXmlNamespace((input.Value == "#default") ? string.Empty : input.Value);
			}
		}
		if (input.MoveToXsltAttribute(1, "result-prefix"))
		{
			if (input.Value.Length == 0)
			{
				ReportError("The value of the '{0}' attribute cannot be empty. Use '#default' to specify the default namespace.", "result-prefix");
			}
			else
			{
				text2 = ((input.Value == "#default") ? string.Empty : input.Value);
				text3 = input.LookupXmlNamespace(text2);
			}
		}
		CheckNoContent();
		if (text != null && text3 != null && compiler.SetNsAlias(text, text3, text2, curStylesheet.ImportPrecedence))
		{
			input.MoveToElement();
			ReportWarning("Namespace URI '{0}' is declared to be an alias for multiple different namespace URIs with the same import precedence.", text);
		}
	}

	private void LoadAttributeSet(NsDecl stylesheetNsList)
	{
		XsltInput.ContextInfo attributes = input.GetAttributes(attributeSetAttributes);
		attributes.nsList = MergeNamespaces(attributes.nsList, stylesheetNsList);
		QilName qilName = ParseQNameAttribute(0);
		if (!curStylesheet.AttributeSets.TryGetValue(qilName, out var value))
		{
			value = AstFactory.AttributeSet(qilName);
			curStylesheet.AttributeSets[qilName] = value;
			if (!compiler.AttributeSets.ContainsKey(qilName))
			{
				compiler.AllTemplates.Add(value);
			}
		}
		List<XslNode> list = new List<XslNode>();
		if (input.MoveToXsltAttribute(1, "use-attribute-sets"))
		{
			AddUseAttributeSets(list);
		}
		XsltInput.DelayedQName elementName = input.ElementName;
		if (input.MoveToFirstChild())
		{
			do
			{
				switch (input.NodeType)
				{
				case XmlNodeType.Element:
					if (input.IsXsltKeyword(atoms.Attribute))
					{
						AddInstruction(list, XslAttribute());
						break;
					}
					ReportError("'{0}' cannot be a child of the '{1}' element.", input.QualifiedName, elementName);
					input.SkipNode();
					break;
				default:
					ReportError("'{0}' element cannot have text node children.", elementName);
					break;
				case XmlNodeType.Whitespace:
				case XmlNodeType.SignificantWhitespace:
					break;
				}
			}
			while (input.MoveToNextSibling());
		}
		value.AddContent(SetInfo(AstFactory.List(), LoadEndTag(list), attributes));
	}

	private void LoadGlobalVariableOrParameter(NsDecl stylesheetNsList, XslNodeType nodeType)
	{
		VarPar varPar = XslVarPar();
		varPar.Namespaces = MergeNamespaces(varPar.Namespaces, stylesheetNsList);
		CheckError(!curStylesheet.AddVarPar(varPar), "The variable or parameter '{0}' was duplicated with the same import precedence.", varPar.Name.QualifiedName);
	}

	private void LoadTemplate(NsDecl stylesheetNsList)
	{
		XsltInput.ContextInfo attributes = input.GetAttributes(templateAttributes);
		attributes.nsList = MergeNamespaces(attributes.nsList, stylesheetNsList);
		string text = ParseStringAttribute(0, "match");
		QilName name = ParseQNameAttribute(1);
		double num = double.NaN;
		if (input.MoveToXsltAttribute(2, "priority"))
		{
			num = XPathConvert.StringToDouble(input.Value);
			if (double.IsNaN(num) && !input.ForwardCompatibility)
			{
				ReportError("'{1}' is an invalid value for the '{0}' attribute.", "priority", input.Value);
			}
		}
		QilName mode = (V1 ? ParseModeAttribute(3) : ParseModeListAttribute(3));
		if (text == null)
		{
			CheckError(!input.AttributeExists(1, "name"), "'xsl:template' must have either a 'match' attribute or a 'name' attribute, or both.");
			CheckError(input.AttributeExists(3, "mode"), "An 'xsl:template' element without a 'match' attribute cannot have a 'mode' attribute.");
			mode = nullMode;
			if (input.AttributeExists(2, "priority"))
			{
				if (V1)
				{
					ReportWarning("An 'xsl:template' element without a 'match' attribute cannot have a 'priority' attribute.");
				}
				else
				{
					ReportError("An 'xsl:template' element without a 'match' attribute cannot have a 'priority' attribute.");
				}
			}
		}
		if (input.MoveToXsltAttribute(4, "as"))
		{
			ReportNYI("xsl:template/@as");
		}
		curTemplate = AstFactory.Template(name, text, mode, num, input.XslVersion);
		input.CanHaveApplyImports = text != null;
		SetInfo(curTemplate, LoadEndTag(LoadInstructions(InstructionFlags.AllowParam)), attributes);
		if (!curStylesheet.AddTemplate(curTemplate))
		{
			ReportError("'{0}' is a duplicate template name.", curTemplate.Name.QualifiedName);
		}
		curTemplate = null;
	}

	private void LoadMsScript(NsDecl stylesheetNsList)
	{
		XsltInput.ContextInfo attributes = input.GetAttributes(scriptAttributes);
		attributes.nsList = MergeNamespaces(attributes.nsList, stylesheetNsList);
		string text = null;
		if (input.MoveToXsltAttribute(0, "implements-prefix"))
		{
			if (input.Value.Length == 0)
			{
				ReportError("The value of the '{0}' attribute cannot be empty.", "implements-prefix", input.Value);
			}
			else
			{
				text = input.LookupXmlNamespace(input.Value);
				if (text == "http://www.w3.org/1999/XSL/Transform")
				{
					ReportError("Script block cannot implement the XSLT namespace.");
					text = null;
				}
			}
		}
		if (text == null)
		{
			text = compiler.CreatePhantomNamespace();
		}
		string text2 = ParseStringAttribute(1, "language");
		if (text2 == null)
		{
			text2 = "jscript";
		}
		if (!compiler.Settings.EnableScript)
		{
			compiler.Scripts.ScriptClasses[text] = null;
			input.SkipNode();
			return;
		}
		StringBuilder stringBuilder = new StringBuilder();
		string uri = input.Uri;
		int lineNumber = 0;
		int num = 0;
		ScriptClass scriptClass = compiler.Scripts.GetScriptClass(text, text2, this);
		if (scriptClass == null)
		{
			input.SkipNode();
			return;
		}
		XsltInput.DelayedQName elementName = input.ElementName;
		if (input.MoveToFirstChild())
		{
			do
			{
				XmlNodeType nodeType = input.NodeType;
				if (nodeType != XmlNodeType.Element)
				{
					if (nodeType == XmlNodeType.Text || stringBuilder.Length != 0)
					{
						int line = input.Start.Line;
						int line2 = input.End.Line;
						if (stringBuilder.Length == 0)
						{
							lineNumber = line;
						}
						else if (num < line)
						{
							stringBuilder.Append('\n', line - num);
						}
						stringBuilder.Append(input.Value);
						num = line2;
					}
				}
				else if (input.IsNs(atoms.UrnMsxsl) && (input.IsKeyword(atoms.Assembly) || input.IsKeyword(atoms.Using)))
				{
					if (stringBuilder.Length != 0)
					{
						ReportError("Element '{0}' must precede script code.", input.QualifiedName);
						input.SkipNode();
					}
					else if (input.IsKeyword(atoms.Assembly))
					{
						LoadMsAssembly(scriptClass);
					}
					else if (input.IsKeyword(atoms.Using))
					{
						LoadMsUsing(scriptClass);
					}
				}
				else
				{
					ReportError("'{0}' cannot be a child of the '{1}' element.", input.QualifiedName, elementName);
					input.SkipNode();
				}
			}
			while (input.MoveToNextSibling());
		}
		if (stringBuilder.Length == 0)
		{
			lineNumber = input.Start.Line;
		}
		scriptClass.AddScriptBlock(stringBuilder.ToString(), uri, lineNumber, input.Start);
	}

	private void LoadMsAssembly(ScriptClass scriptClass)
	{
		input.GetAttributes(assemblyAttributes);
		string text = ParseStringAttribute(0, "name");
		string text2 = ParseStringAttribute(1, "href");
		if (text != null == (text2 != null))
		{
			ReportError("'msxsl:assembly' must have either a 'name' attribute or an 'href' attribute, but not both.");
		}
		else
		{
			string text3 = null;
			if (text != null)
			{
				try
				{
					text3 = Assembly.Load(text).Location;
				}
				catch
				{
					AssemblyName assemblyName = new AssemblyName(text);
					byte[] publicKeyToken = assemblyName.GetPublicKeyToken();
					if ((publicKeyToken != null && publicKeyToken.Length != 0) || !(assemblyName.Version == null))
					{
						throw;
					}
					text3 = assemblyName.Name + ".dll";
				}
			}
			else
			{
				text3 = Assembly.LoadFrom(ResolveUri(text2, input.BaseUri).ToString()).Location;
				scriptClass.refAssembliesByHref = true;
			}
			if (text3 != null)
			{
				scriptClass.refAssemblies.Add(text3);
			}
		}
		CheckNoContent();
	}

	private void LoadMsUsing(ScriptClass scriptClass)
	{
		input.GetAttributes(usingAttributes);
		if (input.MoveToXsltAttribute(0, "namespace"))
		{
			scriptClass.nsImports.Add(input.Value);
		}
		CheckNoContent();
	}

	private List<XslNode> LoadInstructions()
	{
		return LoadInstructions(new List<XslNode>(), InstructionFlags.None);
	}

	private List<XslNode> LoadInstructions(InstructionFlags flags)
	{
		return LoadInstructions(new List<XslNode>(), flags);
	}

	private List<XslNode> LoadInstructions(List<XslNode> content)
	{
		return LoadInstructions(content, InstructionFlags.None);
	}

	private List<XslNode> LoadInstructions(List<XslNode> content, InstructionFlags flags)
	{
		if (++loadInstructionsDepth > 1024 && XsltConfigSection.LimitXPathComplexity)
		{
			throw XsltException.Create("The stylesheet is too complex.");
		}
		XsltInput.DelayedQName elementName = input.ElementName;
		if (input.MoveToFirstChild())
		{
			bool flag = true;
			int num = 0;
			do
			{
				XmlNodeType nodeType = input.NodeType;
				XslNode instruction;
				if (nodeType != XmlNodeType.Element)
				{
					if (nodeType == XmlNodeType.Whitespace)
					{
						continue;
					}
					if (nodeType != XmlNodeType.SignificantWhitespace)
					{
						flag = false;
					}
					instruction = SetLineInfo(AstFactory.Text(input.Value), input.BuildLineInfo());
				}
				else
				{
					string namespaceUri = input.NamespaceUri;
					string localName = input.LocalName;
					if (namespaceUri == atoms.UriXsl)
					{
						InstructionFlags instructionFlags = (Ref.Equal(localName, atoms.Param) ? InstructionFlags.AllowParam : (Ref.Equal(localName, atoms.Sort) ? InstructionFlags.AllowSort : InstructionFlags.None));
						if (instructionFlags != 0)
						{
							string text = (((flags & instructionFlags) == 0) ? "'{0}' cannot be a child of the '{1}' element." : ((!flag) ? "'{0}' element children must precede all other children of the '{1}' element." : null));
							if (text != null)
							{
								ReportError(text, input.QualifiedName, elementName);
								flag = false;
								input.SkipNode();
								continue;
							}
						}
						else
						{
							flag = false;
						}
						instruction = (Ref.Equal(localName, atoms.ApplyImports) ? XslApplyImports() : (Ref.Equal(localName, atoms.ApplyTemplates) ? XslApplyTemplates() : (Ref.Equal(localName, atoms.CallTemplate) ? XslCallTemplate() : (Ref.Equal(localName, atoms.Copy) ? XslCopy() : (Ref.Equal(localName, atoms.CopyOf) ? XslCopyOf() : (Ref.Equal(localName, atoms.Fallback) ? XslFallback() : (Ref.Equal(localName, atoms.If) ? XslIf() : (Ref.Equal(localName, atoms.Choose) ? XslChoose() : (Ref.Equal(localName, atoms.ForEach) ? XslForEach() : (Ref.Equal(localName, atoms.Message) ? XslMessage() : (Ref.Equal(localName, atoms.Number) ? XslNumber() : (Ref.Equal(localName, atoms.ValueOf) ? XslValueOf() : (Ref.Equal(localName, atoms.Comment) ? XslComment() : (Ref.Equal(localName, atoms.ProcessingInstruction) ? XslProcessingInstruction() : (Ref.Equal(localName, atoms.Text) ? XslText() : (Ref.Equal(localName, atoms.Element) ? XslElement() : (Ref.Equal(localName, atoms.Attribute) ? XslAttribute() : (Ref.Equal(localName, atoms.Variable) ? XslVarPar() : (Ref.Equal(localName, atoms.Param) ? XslVarPar() : (Ref.Equal(localName, atoms.Sort) ? XslSort(num++) : LoadUnknownXsltInstruction(elementName)))))))))))))))))))));
					}
					else
					{
						flag = false;
						instruction = LoadLiteralResultElement(asStylesheet: false);
					}
				}
				AddInstruction(content, instruction);
			}
			while (input.MoveToNextSibling());
		}
		loadInstructionsDepth--;
		return content;
	}

	private List<XslNode> LoadWithParams(InstructionFlags flags)
	{
		XsltInput.DelayedQName elementName = input.ElementName;
		List<XslNode> list = new List<XslNode>();
		if (input.MoveToFirstChild())
		{
			int num = 0;
			do
			{
				switch (input.NodeType)
				{
				case XmlNodeType.Element:
					if (input.IsXsltKeyword(atoms.WithParam))
					{
						XslNode xslNode = XslVarPar();
						CheckWithParam(list, xslNode);
						AddInstruction(list, xslNode);
					}
					else if (flags == InstructionFlags.AllowSort && input.IsXsltKeyword(atoms.Sort))
					{
						AddInstruction(list, XslSort(num++));
					}
					else if (flags == InstructionFlags.AllowFallback && input.IsXsltKeyword(atoms.Fallback))
					{
						XslFallback();
					}
					else
					{
						ReportError("'{0}' cannot be a child of the '{1}' element.", input.QualifiedName, elementName);
						input.SkipNode();
					}
					break;
				default:
					ReportError("'{0}' element cannot have text node children.", elementName);
					break;
				case XmlNodeType.Whitespace:
				case XmlNodeType.SignificantWhitespace:
					break;
				}
			}
			while (input.MoveToNextSibling());
		}
		return list;
	}

	private XslNode XslApplyImports()
	{
		XsltInput.ContextInfo attributes = input.GetAttributes();
		if (!input.CanHaveApplyImports)
		{
			ReportError("An 'xsl:apply-imports' element can only occur within an 'xsl:template' element with a 'match' attribute, and cannot occur within an 'xsl:for-each' element.");
			input.SkipNode();
			return null;
		}
		List<XslNode> list = LoadWithParams(InstructionFlags.None);
		attributes.SaveExtendedLineInfo(input);
		if (V1)
		{
			if (list.Count != 0)
			{
				ISourceLineInfo sourceLine = list[0].SourceLine;
				if (input.ForwardCompatibility)
				{
					return SetInfo(AstFactory.Error(XslLoadException.CreateMessage(sourceLine, "The contents of '{0}' must be empty.", atoms.ApplyImports)), null, attributes);
				}
				compiler.ReportError(sourceLine, "The contents of '{0}' must be empty.", atoms.ApplyImports);
			}
			list = null;
		}
		else
		{
			if (list.Count != 0)
			{
				ReportNYI("xsl:apply-imports/xsl:with-param");
			}
			list = null;
		}
		return SetInfo(AstFactory.ApplyImports(curTemplate.Mode, curStylesheet, input.XslVersion), list, attributes);
	}

	private XslNode XslApplyTemplates()
	{
		XsltInput.ContextInfo attributes = input.GetAttributes(applyTemplatesAttributes);
		string text = ParseStringAttribute(0, "select");
		if (text == null)
		{
			text = "node()";
		}
		QilName mode = ParseModeAttribute(1);
		List<XslNode> content = LoadWithParams(InstructionFlags.AllowSort);
		attributes.SaveExtendedLineInfo(input);
		return SetInfo(AstFactory.ApplyTemplates(mode, text, attributes, input.XslVersion), content, attributes);
	}

	private XslNode XslCallTemplate()
	{
		XsltInput.ContextInfo attributes = input.GetAttributes(callTemplateAttributes);
		QilName name = ParseQNameAttribute(0);
		List<XslNode> content = LoadWithParams(InstructionFlags.None);
		attributes.SaveExtendedLineInfo(input);
		return SetInfo(AstFactory.CallTemplate(name, attributes), content, attributes);
	}

	private XslNode XslCopy()
	{
		XsltInput.ContextInfo attributes = input.GetAttributes(copyAttributes);
		bool num = ParseYesNoAttribute(0, "copy-namespaces") != TriState.False;
		bool flag = ParseYesNoAttribute(1, "inherit-namespaces") != TriState.False;
		if (!num)
		{
			ReportNYI("xsl:copy[@copy-namespaces    = 'no']");
		}
		if (!flag)
		{
			ReportNYI("xsl:copy[@inherit-namespaces = 'no']");
		}
		List<XslNode> list = new List<XslNode>();
		if (input.MoveToXsltAttribute(2, "use-attribute-sets"))
		{
			AddUseAttributeSets(list);
		}
		ParseTypeAttribute(3);
		ParseValidationAttribute(4, defVal: false);
		return SetInfo(AstFactory.Copy(), LoadEndTag(LoadInstructions(list)), attributes);
	}

	private XslNode XslCopyOf()
	{
		XsltInput.ContextInfo attributes = input.GetAttributes(copyOfAttributes);
		string select = ParseStringAttribute(0, "select");
		if (ParseYesNoAttribute(1, "copy-namespaces") == TriState.False)
		{
			ReportNYI("xsl:copy-of[@copy-namespaces    = 'no']");
		}
		ParseTypeAttribute(2);
		ParseValidationAttribute(3, defVal: false);
		CheckNoContent();
		return SetInfo(AstFactory.CopyOf(select, input.XslVersion), null, attributes);
	}

	private XslNode XslFallback()
	{
		input.GetAttributes();
		input.SkipNode();
		return null;
	}

	private XslNode XslIf()
	{
		XsltInput.ContextInfo attributes = input.GetAttributes(ifAttributes);
		return SetInfo(AstFactory.If(ParseStringAttribute(0, "test"), input.XslVersion), LoadInstructions(), attributes);
	}

	private XslNode XslChoose()
	{
		XsltInput.ContextInfo attributes = input.GetAttributes();
		List<XslNode> content = new List<XslNode>();
		bool flag = false;
		bool flag2 = false;
		XsltInput.DelayedQName elementName = input.ElementName;
		if (input.MoveToFirstChild())
		{
			do
			{
				switch (input.NodeType)
				{
				case XmlNodeType.Element:
				{
					XslNode xslNode = null;
					if (Ref.Equal(input.NamespaceUri, atoms.UriXsl))
					{
						if (Ref.Equal(input.LocalName, atoms.When))
						{
							if (flag)
							{
								ReportError("'xsl:when' must precede the 'xsl:otherwise' element.");
								input.SkipNode();
								break;
							}
							flag2 = true;
							xslNode = XslIf();
						}
						else if (Ref.Equal(input.LocalName, atoms.Otherwise))
						{
							if (flag)
							{
								ReportError("An 'xsl:choose' element can have only one 'xsl:otherwise' child.");
								input.SkipNode();
								break;
							}
							flag = true;
							xslNode = XslOtherwise();
						}
					}
					if (xslNode == null)
					{
						ReportError("'{0}' cannot be a child of the '{1}' element.", input.QualifiedName, elementName);
						input.SkipNode();
					}
					else
					{
						AddInstruction(content, xslNode);
					}
					break;
				}
				default:
					ReportError("'{0}' element cannot have text node children.", elementName);
					break;
				case XmlNodeType.Whitespace:
				case XmlNodeType.SignificantWhitespace:
					break;
				}
			}
			while (input.MoveToNextSibling());
		}
		CheckError(!flag2, "An 'xsl:choose' element must have at least one 'xsl:when' child.");
		return SetInfo(AstFactory.Choose(), content, attributes);
	}

	private XslNode XslOtherwise()
	{
		XsltInput.ContextInfo attributes = input.GetAttributes();
		return SetInfo(AstFactory.Otherwise(), LoadInstructions(), attributes);
	}

	private XslNode XslForEach()
	{
		XsltInput.ContextInfo attributes = input.GetAttributes(forEachAttributes);
		string select = ParseStringAttribute(0, "select");
		input.CanHaveApplyImports = false;
		List<XslNode> content = LoadInstructions(InstructionFlags.AllowSort);
		attributes.SaveExtendedLineInfo(input);
		return SetInfo(AstFactory.ForEach(select, attributes, input.XslVersion), content, attributes);
	}

	private XslNode XslMessage()
	{
		XsltInput.ContextInfo attributes = input.GetAttributes(messageAttributes);
		string text = ParseStringAttribute(0, "select");
		bool term = ParseYesNoAttribute(1, "terminate") == TriState.True;
		List<XslNode> list = LoadInstructions();
		if (list.Count != 0)
		{
			list = LoadEndTag(list);
		}
		if (text != null)
		{
			list.Insert(0, AstFactory.CopyOf(text, input.XslVersion));
		}
		return SetInfo(AstFactory.Message(term), list, attributes);
	}

	private XslNode XslNumber()
	{
		XsltInput.ContextInfo attributes = input.GetAttributes(numberAttributes);
		string value = ParseStringAttribute(0, "value");
		if (ParseStringAttribute(1, "select") != null)
		{
			ReportNYI("xsl:number/@select");
		}
		NumberLevel level = NumberLevel.Single;
		if (input.MoveToXsltAttribute(2, "level"))
		{
			switch (input.Value)
			{
			case "single":
				level = NumberLevel.Single;
				break;
			case "multiple":
				level = NumberLevel.Multiple;
				break;
			case "any":
				level = NumberLevel.Any;
				break;
			default:
				if (!input.ForwardCompatibility)
				{
					ReportError("'{1}' is an invalid value for the '{0}' attribute.", "level", input.Value);
				}
				break;
			}
		}
		string count = ParseStringAttribute(3, "count");
		string from = ParseStringAttribute(4, "from");
		string text = ParseStringAttribute(5, "format");
		string lang = ParseStringAttribute(6, "lang");
		string letterValue = ParseStringAttribute(7, "letter-value");
		if (!string.IsNullOrEmpty(ParseStringAttribute(8, "ordinal")))
		{
			ReportNYI("xsl:number/@ordinal");
		}
		string groupingSeparator = ParseStringAttribute(9, "grouping-separator");
		string groupingSize = ParseStringAttribute(10, "grouping-size");
		if (text == null)
		{
			text = "1";
		}
		CheckNoContent();
		return SetInfo(AstFactory.Number(level, count, from, value, text, lang, letterValue, groupingSeparator, groupingSize, input.XslVersion), null, attributes);
	}

	private XslNode XslValueOf()
	{
		XsltInput.ContextInfo attributes = input.GetAttributes(valueOfAttributes);
		string text = ParseStringAttribute(0, "select");
		string text2 = ParseStringAttribute(1, "separator");
		bool flag = ParseYesNoAttribute(2, "disable-output-escaping") == TriState.True;
		if (text2 == null)
		{
			if (!input.BackwardCompatibility && text == null)
			{
				_ = string.Empty;
			}
		}
		else
		{
			ReportNYI("xsl:value-of/@separator");
		}
		List<XslNode> list = null;
		if (V1)
		{
			if (text == null)
			{
				input.SkipNode();
				return SetInfo(AstFactory.Error(XslLoadException.CreateMessage(attributes.lineInfo, "Missing mandatory attribute '{0}'.", "select")), null, attributes);
			}
			CheckNoContent();
		}
		else
		{
			list = LoadContent(text != null);
			CheckError(text == null && list.Count == 0, "Element '{0}' must have either 'select' attribute or non-empty content.", input.ElementName);
			if (list.Count != 0)
			{
				ReportNYI("xsl:value-of/*");
				list = null;
			}
		}
		return SetInfo(AstFactory.XslNode(flag ? XslNodeType.ValueOfDoe : XslNodeType.ValueOf, null, text, input.XslVersion), null, attributes);
	}

	private VarPar XslVarPar()
	{
		string localName = input.LocalName;
		XslNodeType xslNodeType = (Ref.Equal(localName, atoms.Variable) ? XslNodeType.Variable : (Ref.Equal(localName, atoms.Param) ? XslNodeType.Param : (Ref.Equal(localName, atoms.WithParam) ? XslNodeType.WithParam : XslNodeType.Unknown)));
		Ref.Equal(localName, atoms.Param);
		XsltInput.ContextInfo attributes = input.GetAttributes(xslNodeType switch
		{
			XslNodeType.Param => paramAttributes, 
			XslNodeType.Variable => variableAttributes, 
			_ => withParamAttributes, 
		});
		QilName qilName = ParseQNameAttribute(0);
		string text = ParseStringAttribute(1, "select");
		string text2 = ParseStringAttribute(2, "as");
		TriState triState = ParseYesNoAttribute(3, "required");
		if (xslNodeType == XslNodeType.Param && curFunction != null)
		{
			if (!input.ForwardCompatibility)
			{
				CheckError(triState != TriState.Unknown, "The 'required' attribute must not be specified for parameter '{0}'. Function parameters are always mandatory.", qilName.ToString());
			}
			triState = TriState.True;
		}
		else if (triState == TriState.True)
		{
			ReportNYI("xsl:param/@required == true()");
		}
		if (text2 != null)
		{
			ReportNYI("xsl:param/@as");
		}
		TriState triState2 = ParseYesNoAttribute(4, "tunnel");
		if (triState2 != TriState.Unknown)
		{
			if (xslNodeType == XslNodeType.Param && curTemplate == null)
			{
				if (!input.ForwardCompatibility)
				{
					ReportError("Stylesheet or function parameter '{0}' cannot have attribute 'tunnel'.", qilName.ToString());
				}
			}
			else if (triState2 == TriState.True)
			{
				ReportNYI("xsl:param/@tunnel == true()");
			}
		}
		List<XslNode> list = LoadContent(text != null);
		CheckError(triState == TriState.True && (text != null || list.Count != 0), "Mandatory parameter '{0}' must be empty and must not have a 'select' attribute.", qilName.ToString());
		VarPar varPar = AstFactory.VarPar(xslNodeType, qilName, text, input.XslVersion);
		SetInfo(varPar, list, attributes);
		return varPar;
	}

	private XslNode XslComment()
	{
		XsltInput.ContextInfo attributes = input.GetAttributes(commentAttributes);
		string text = ParseStringAttribute(0, "select");
		if (text != null)
		{
			ReportNYI("xsl:comment/@select");
		}
		return SetInfo(AstFactory.Comment(), LoadContent(text != null), attributes);
	}

	private List<XslNode> LoadContent(bool hasSelect)
	{
		XsltInput.DelayedQName elementName = input.ElementName;
		List<XslNode> list = LoadInstructions();
		CheckError(hasSelect && list.Count != 0, "The element '{0}' cannot have both a 'select' attribute and non-empty content.", elementName);
		if (list.Count != 0)
		{
			list = LoadEndTag(list);
		}
		return list;
	}

	private XslNode XslProcessingInstruction()
	{
		XsltInput.ContextInfo attributes = input.GetAttributes(processingInstructionAttributes);
		string name = ParseNCNameAttribute(0);
		string text = ParseStringAttribute(1, "select");
		if (text != null)
		{
			ReportNYI("xsl:processing-instruction/@select");
		}
		return SetInfo(AstFactory.PI(name, input.XslVersion), LoadContent(text != null), attributes);
	}

	private XslNode XslText()
	{
		XsltInput.ContextInfo attributes = input.GetAttributes(textAttributes);
		SerializationHints hints = ((ParseYesNoAttribute(0, "disable-output-escaping") == TriState.True) ? SerializationHints.DisableOutputEscaping : SerializationHints.None);
		List<XslNode> list = new List<XslNode>();
		XsltInput.DelayedQName elementName = input.ElementName;
		if (input.MoveToFirstChild())
		{
			do
			{
				XmlNodeType nodeType = input.NodeType;
				if (nodeType == XmlNodeType.Text || (uint)(nodeType - 13) <= 1u)
				{
					list.Add(AstFactory.Text(input.Value, hints));
					continue;
				}
				ReportError("'{0}' cannot be a child of the '{1}' element.", input.QualifiedName, elementName);
				input.SkipNode();
			}
			while (input.MoveToNextSibling());
		}
		return SetInfo(AstFactory.List(), list, attributes);
	}

	private XslNode XslElement()
	{
		XsltInput.ContextInfo attributes = input.GetAttributes(elementAttributes);
		string nameAvt = ParseNCNameAttribute(0);
		string text = ParseStringAttribute(1, "namespace");
		CheckError(text == "http://www.w3.org/2000/xmlns/", "Elements and attributes cannot belong to the reserved namespace '{0}'.", text);
		if (ParseYesNoAttribute(2, "inherit-namespaces") == TriState.False)
		{
			ReportNYI("xsl:copy[@inherit-namespaces = 'no']");
		}
		ParseTypeAttribute(4);
		ParseValidationAttribute(5, defVal: false);
		List<XslNode> list = new List<XslNode>();
		if (input.MoveToXsltAttribute(3, "use-attribute-sets"))
		{
			AddUseAttributeSets(list);
		}
		return SetInfo(AstFactory.Element(nameAvt, text, input.XslVersion), LoadEndTag(LoadInstructions(list)), attributes);
	}

	private XslNode XslAttribute()
	{
		XsltInput.ContextInfo attributes = input.GetAttributes(attributeAttributes);
		string nameAvt = ParseNCNameAttribute(0);
		string text = ParseStringAttribute(1, "namespace");
		CheckError(text == "http://www.w3.org/2000/xmlns/", "Elements and attributes cannot belong to the reserved namespace '{0}'.", text);
		string text2 = ParseStringAttribute(2, "select");
		if (text2 != null)
		{
			ReportNYI("xsl:attribute/@select");
		}
		string text3 = ParseStringAttribute(3, "separator");
		if (text3 != null)
		{
			ReportNYI("xsl:attribute/@separator");
		}
		text3 = ((text3 != null) ? text3 : ((text2 != null) ? " " : string.Empty));
		ParseTypeAttribute(4);
		ParseValidationAttribute(5, defVal: false);
		return SetInfo(AstFactory.Attribute(nameAvt, text, input.XslVersion), LoadContent(text2 != null), attributes);
	}

	private XslNode XslSort(int sortNumber)
	{
		XsltInput.ContextInfo attributes = input.GetAttributes(sortAttributes);
		string text = ParseStringAttribute(0, "select");
		string lang = ParseStringAttribute(1, "lang");
		string order = ParseStringAttribute(2, "order");
		ParseCollationAttribute(3);
		TriState num = ParseYesNoAttribute(4, "stable");
		string caseOrder = ParseStringAttribute(5, "case-order");
		string dataType = ParseStringAttribute(6, "data-type");
		if (num != TriState.Unknown)
		{
			CheckError(sortNumber != 0, "Only the first 'xsl:sort' element may have 'stable' attribute.");
		}
		if (V1)
		{
			CheckNoContent();
		}
		else if (LoadContent(text != null).Count != 0)
		{
			ReportNYI("xsl:sort/*");
		}
		if (text == null)
		{
			text = ".";
		}
		return SetInfo(AstFactory.Sort(text, lang, dataType, order, caseOrder, input.XslVersion), null, attributes);
	}

	private XslNode LoadLiteralResultElement(bool asStylesheet)
	{
		string prefix = input.Prefix;
		string localName = input.LocalName;
		string namespaceUri = input.NamespaceUri;
		XsltInput.ContextInfo literalAttributes = input.GetLiteralAttributes(asStylesheet);
		if (input.IsExtensionNamespace(namespaceUri))
		{
			return SetInfo(AstFactory.List(), LoadFallbacks(localName), literalAttributes);
		}
		List<XslNode> list = new List<XslNode>();
		for (int i = 1; input.MoveToLiteralAttribute(i); i++)
		{
			if (input.IsXsltNamespace() && input.IsKeyword(atoms.UseAttributeSets))
			{
				AddUseAttributeSets(list);
			}
		}
		for (int j = 1; input.MoveToLiteralAttribute(j); j++)
		{
			if (!input.IsXsltNamespace())
			{
				XslNode node = AstFactory.LiteralAttribute(AstFactory.QName(input.LocalName, input.NamespaceUri, input.Prefix), input.Value, input.XslVersion);
				AddInstruction(list, SetLineInfo(node, literalAttributes.lineInfo));
			}
		}
		list = LoadEndTag(LoadInstructions(list));
		return SetInfo(AstFactory.LiteralElement(AstFactory.QName(localName, namespaceUri, prefix)), list, literalAttributes);
	}

	private void CheckWithParam(List<XslNode> content, XslNode withParam)
	{
		foreach (XslNode item in content)
		{
			if (item.NodeType == XslNodeType.WithParam && item.Name.Equals(withParam.Name))
			{
				ReportError("Value of parameter '{0}' cannot be specified more than once within a single 'xsl:call-template' or 'xsl:apply-templates' element.", withParam.Name.QualifiedName);
				break;
			}
		}
	}

	private static void AddInstruction(List<XslNode> content, XslNode instruction)
	{
		if (instruction != null)
		{
			content.Add(instruction);
		}
	}

	private List<XslNode> LoadEndTag(List<XslNode> content)
	{
		if (compiler.IsDebug && !input.IsEmptyElement)
		{
			AddInstruction(content, SetLineInfo(AstFactory.Nop(), input.BuildLineInfo()));
		}
		return content;
	}

	private XslNode LoadUnknownXsltInstruction(string parentName)
	{
		input.GetVersionAttribute();
		if (!input.ForwardCompatibility)
		{
			ReportError("'{0}' cannot be a child of the '{1}' element.", input.QualifiedName, parentName);
			input.SkipNode();
			return null;
		}
		XsltInput.ContextInfo attributes = input.GetAttributes();
		List<XslNode> content = LoadFallbacks(input.LocalName);
		return SetInfo(AstFactory.List(), content, attributes);
	}

	private List<XslNode> LoadFallbacks(string instrName)
	{
		input.MoveToElement();
		ISourceLineInfo lineInfo = input.BuildNameLineInfo();
		List<XslNode> list = new List<XslNode>();
		if (input.MoveToFirstChild())
		{
			do
			{
				if (input.IsXsltKeyword(atoms.Fallback))
				{
					XsltInput.ContextInfo attributes = input.GetAttributes();
					list.Add(SetInfo(AstFactory.List(), LoadInstructions(), attributes));
				}
				else
				{
					input.SkipNode();
				}
			}
			while (input.MoveToNextSibling());
		}
		if (list.Count == 0)
		{
			list.Add(AstFactory.Error(XslLoadException.CreateMessage(lineInfo, "'{0}' is not a recognized extension element.", instrName)));
		}
		return list;
	}

	private QilName ParseModeAttribute(int attNum)
	{
		if (!input.MoveToXsltAttribute(attNum, "mode"))
		{
			return nullMode;
		}
		compiler.EnterForwardsCompatible();
		string value = input.Value;
		QilName result;
		if (!V1 && value == "#default")
		{
			result = nullMode;
		}
		else if (!V1 && value == "#current")
		{
			ReportNYI("xsl:apply-templates[@mode='#current']");
			result = nullMode;
		}
		else if (!V1 && value == "#all")
		{
			ReportError("List of modes in 'xsl:template' element can't contain token '#all' together with any other value.");
			result = nullMode;
		}
		else
		{
			result = CreateXPathQName(value);
		}
		if (!compiler.ExitForwardsCompatible(input.ForwardCompatibility))
		{
			result = nullMode;
		}
		return result;
	}

	private QilName ParseModeListAttribute(int attNum)
	{
		if (!input.MoveToXsltAttribute(attNum, "mode"))
		{
			return nullMode;
		}
		string value = input.Value;
		if (value == "#all")
		{
			ReportNYI("xsl:template[@mode='#all']");
			return nullMode;
		}
		string[] array = XmlConvert.SplitString(value);
		List<QilName> list = new List<QilName>(array.Length);
		compiler.EnterForwardsCompatible();
		if (array.Length == 0)
		{
			ReportError("List of modes in 'xsl:template' element can't be empty.");
		}
		else
		{
			string[] array2 = array;
			foreach (string text in array2)
			{
				QilName qilName;
				bool flag;
				switch (text)
				{
				case "#default":
					qilName = nullMode;
					goto IL_00e6;
				case "#current":
					ReportNYI("xsl:apply-templates[@mode='#current']");
					break;
				case "#all":
					ReportError("List of modes in 'xsl:template' element can't contain token '#all' together with any other value.");
					break;
				default:
					{
						qilName = CreateXPathQName(text);
						goto IL_00e6;
					}
					IL_00e6:
					flag = false;
					foreach (QilName item in list)
					{
						flag |= item.Equals(qilName);
					}
					if (flag)
					{
						ReportError("List of modes in 'xsl:template' element can't contain duplicates ('{0}').", text);
					}
					else
					{
						list.Add(qilName);
					}
					continue;
				}
				break;
			}
		}
		if (!compiler.ExitForwardsCompatible(input.ForwardCompatibility))
		{
			list.Clear();
			list.Add(nullMode);
		}
		if (1 < list.Count)
		{
			ReportNYI("Multipe modes");
			return nullMode;
		}
		if (list.Count == 0)
		{
			return nullMode;
		}
		return list[0];
	}

	private string ParseCollationAttribute(int attNum)
	{
		if (input.MoveToXsltAttribute(attNum, "collation"))
		{
			ReportNYI("@collation");
		}
		return null;
	}

	private bool ResolveQName(bool ignoreDefaultNs, string qname, out string localName, out string namespaceName, out string prefix)
	{
		if (qname == null)
		{
			prefix = compiler.PhantomNCName;
			localName = compiler.PhantomNCName;
			namespaceName = compiler.CreatePhantomNamespace();
			return false;
		}
		if (!compiler.ParseQName(qname, out prefix, out localName, this))
		{
			namespaceName = compiler.CreatePhantomNamespace();
			return false;
		}
		if (ignoreDefaultNs && prefix.Length == 0)
		{
			namespaceName = string.Empty;
		}
		else
		{
			namespaceName = input.LookupXmlNamespace(prefix);
			if (namespaceName == null)
			{
				namespaceName = compiler.CreatePhantomNamespace();
				return false;
			}
		}
		return true;
	}

	private QilName ParseQNameAttribute(int attNum)
	{
		bool flag = input.IsRequiredAttribute(attNum);
		QilName qilName = null;
		if (!flag)
		{
			compiler.EnterForwardsCompatible();
		}
		if (input.MoveToXsltAttribute(attNum, "name") && ResolveQName(ignoreDefaultNs: true, input.Value, out var localName, out var namespaceName, out var prefix))
		{
			qilName = AstFactory.QName(localName, namespaceName, prefix);
		}
		if (!flag)
		{
			compiler.ExitForwardsCompatible(input.ForwardCompatibility);
		}
		if (qilName == null && flag)
		{
			qilName = AstFactory.QName(compiler.PhantomNCName, compiler.CreatePhantomNamespace(), compiler.PhantomNCName);
		}
		return qilName;
	}

	private string ParseNCNameAttribute(int attNum)
	{
		if (input.MoveToXsltAttribute(attNum, "name"))
		{
			return input.Value;
		}
		return compiler.PhantomNCName;
	}

	private QilName CreateXPathQName(string qname)
	{
		ResolveQName(ignoreDefaultNs: true, qname, out var localName, out var namespaceName, out var prefix);
		return AstFactory.QName(localName, namespaceName, prefix);
	}

	private XmlQualifiedName ResolveQName(bool ignoreDefaultNs, string qname)
	{
		ResolveQName(ignoreDefaultNs, qname, out var localName, out var namespaceName, out var _);
		return new XmlQualifiedName(localName, namespaceName);
	}

	private void ParseWhitespaceRules(string elements, bool preserveSpace)
	{
		if (elements == null || elements.Length == 0)
		{
			return;
		}
		string[] array = XmlConvert.SplitString(elements);
		for (int i = 0; i < array.Length; i++)
		{
			string text;
			if (!compiler.ParseNameTest(array[i], out var prefix, out var localName, this))
			{
				text = compiler.CreatePhantomNamespace();
			}
			else if (prefix == null || prefix.Length == 0)
			{
				text = prefix;
			}
			else
			{
				text = input.LookupXmlNamespace(prefix);
				if (text == null)
				{
					text = compiler.CreatePhantomNamespace();
				}
			}
			int index = ((localName == null) ? 1 : 0) + ((text == null) ? 1 : 0);
			curStylesheet.AddWhitespaceRule(index, new WhitespaceRule(localName, text, preserveSpace));
		}
	}

	private XmlQualifiedName ParseOutputMethod(string attValue, out XmlOutputMethod method)
	{
		ResolveQName(ignoreDefaultNs: true, attValue, out var localName, out var namespaceName, out var prefix);
		method = XmlOutputMethod.AutoDetect;
		if (compiler.IsPhantomNamespace(namespaceName))
		{
			return null;
		}
		if (prefix.Length == 0)
		{
			switch (localName)
			{
			case "xml":
				method = XmlOutputMethod.Xml;
				break;
			case "html":
				method = XmlOutputMethod.Html;
				break;
			case "text":
				method = XmlOutputMethod.Text;
				break;
			default:
				ReportError("'{1}' is an invalid value for the '{0}' attribute.", "method", attValue);
				return null;
			}
		}
		else if (!input.ForwardCompatibility)
		{
			ReportWarning("'{0}' is not a supported output method. Supported methods are 'xml', 'html', and 'text'.", attValue);
		}
		return new XmlQualifiedName(localName, namespaceName);
	}

	private void AddUseAttributeSets(List<XslNode> list)
	{
		compiler.EnterForwardsCompatible();
		string[] array = XmlConvert.SplitString(input.Value);
		foreach (string qname in array)
		{
			AddInstruction(list, SetLineInfo(AstFactory.UseAttributeSet(CreateXPathQName(qname)), input.BuildLineInfo()));
		}
		if (!compiler.ExitForwardsCompatible(input.ForwardCompatibility))
		{
			list.Clear();
		}
	}

	private List<QilName> ParseUseCharacterMaps(int attNum)
	{
		List<QilName> list = new List<QilName>();
		if (input.MoveToXsltAttribute(attNum, "use-character-maps"))
		{
			compiler.EnterForwardsCompatible();
			string[] array = XmlConvert.SplitString(input.Value);
			foreach (string qname in array)
			{
				list.Add(CreateXPathQName(qname));
			}
			if (!compiler.ExitForwardsCompatible(input.ForwardCompatibility))
			{
				list.Clear();
			}
		}
		return list;
	}

	private string ParseStringAttribute(int attNum, string attName)
	{
		if (input.MoveToXsltAttribute(attNum, attName))
		{
			return input.Value;
		}
		return null;
	}

	private char ParseCharAttribute(int attNum, string attName, char defVal)
	{
		if (input.MoveToXsltAttribute(attNum, attName))
		{
			if (input.Value.Length == 1)
			{
				return input.Value[0];
			}
			if (input.IsRequiredAttribute(attNum) || !input.ForwardCompatibility)
			{
				ReportError("The value of the '{0}' attribute must be a single character.", attName);
			}
		}
		return defVal;
	}

	private TriState ParseYesNoAttribute(int attNum, string attName)
	{
		if (input.MoveToXsltAttribute(attNum, attName))
		{
			string value = input.Value;
			if (value == "yes")
			{
				return TriState.True;
			}
			if (value == "no")
			{
				return TriState.False;
			}
			if (!input.ForwardCompatibility)
			{
				ReportError("The value of the '{0}' attribute must be '{1}' or '{2}'.", attName, "yes", "no");
			}
		}
		return TriState.Unknown;
	}

	private void ParseTypeAttribute(int attNum)
	{
		if (input.MoveToXsltAttribute(attNum, "type"))
		{
			CheckError(true, "Attribute '{0}' is not permitted in basic XSLT processor (http://www.w3.org/TR/xslt20/#dt-basic-xslt-processor).", "type");
		}
	}

	private void ParseValidationAttribute(int attNum, bool defVal)
	{
		string text = (defVal ? atoms.DefaultValidation : "validation");
		if (!input.MoveToXsltAttribute(attNum, text))
		{
			return;
		}
		string value = input.Value;
		switch (value)
		{
		case "strict":
			if (defVal)
			{
				goto default;
			}
			goto case "preserve";
		default:
			if (!(value == "lax") || defVal)
			{
				break;
			}
			goto case "preserve";
		case "preserve":
			ReportError("Value '{1}' of attribute '{0}' is not permitted in basic XSLT processor (http://www.w3.org/TR/xslt20/#dt-basic-xslt-processor).", text, value);
			return;
		}
		if (!input.ForwardCompatibility)
		{
			ReportError("'{1}' is an invalid value for the '{0}' attribute.", text, value);
		}
	}

	private void ParseInputTypeAnnotationsAttribute(int attNum)
	{
		if (!input.MoveToXsltAttribute(attNum, "input-type-annotations"))
		{
			return;
		}
		string value = input.Value;
		switch (value)
		{
		case "strip":
		case "preserve":
			if (compiler.inputTypeAnnotations == null)
			{
				compiler.inputTypeAnnotations = value;
			}
			else
			{
				CheckError(compiler.inputTypeAnnotations != value, "It is an error if there is a stylesheet module in the stylesheet that specifies 'input-type-annotations'=\"strip\" and another stylesheet module that specifies 'input-type-annotations'=\"preserve\".");
			}
			return;
		}
		if (!input.ForwardCompatibility)
		{
			ReportError("'{1}' is an invalid value for the '{0}' attribute.", "input-type-annotations", value);
		}
	}

	private void CheckNoContent()
	{
		input.MoveToElement();
		XsltInput.DelayedQName elementName = input.ElementName;
		ISourceLineInfo sourceLineInfo = SkipEmptyContent();
		if (sourceLineInfo != null)
		{
			compiler.ReportError(sourceLineInfo, "The contents of '{0}' must be empty.", elementName);
		}
	}

	private ISourceLineInfo SkipEmptyContent()
	{
		ISourceLineInfo sourceLineInfo = null;
		if (input.MoveToFirstChild())
		{
			do
			{
				if (input.NodeType != XmlNodeType.Whitespace)
				{
					if (sourceLineInfo == null)
					{
						sourceLineInfo = input.BuildNameLineInfo();
					}
					input.SkipNode();
				}
			}
			while (input.MoveToNextSibling());
		}
		return sourceLineInfo;
	}

	private static XslNode SetLineInfo(XslNode node, ISourceLineInfo lineInfo)
	{
		node.SourceLine = lineInfo;
		return node;
	}

	private static void SetContent(XslNode node, List<XslNode> content)
	{
		if (content != null && content.Count == 0)
		{
			content = null;
		}
		node.SetContent(content);
	}

	internal static XslNode SetInfo(XslNode to, List<XslNode> content, XsltInput.ContextInfo info)
	{
		to.Namespaces = info.nsList;
		SetContent(to, content);
		SetLineInfo(to, info.lineInfo);
		return to;
	}

	private static NsDecl MergeNamespaces(NsDecl thisList, NsDecl parentList)
	{
		if (parentList == null)
		{
			return thisList;
		}
		if (thisList == null)
		{
			return parentList;
		}
		while (parentList != null)
		{
			bool flag = false;
			for (NsDecl nsDecl = thisList; nsDecl != null; nsDecl = nsDecl.Prev)
			{
				if (Ref.Equal(nsDecl.Prefix, parentList.Prefix) && (nsDecl.Prefix != null || nsDecl.NsUri == parentList.NsUri))
				{
					flag = true;
					break;
				}
			}
			if (!flag)
			{
				thisList = new NsDecl(thisList, parentList.Prefix, parentList.NsUri);
			}
			parentList = parentList.Prev;
		}
		return thisList;
	}

	public void ReportError(string res, params string[] args)
	{
		compiler.ReportError(input.BuildNameLineInfo(), res, args);
	}

	public void ReportWarning(string res, params string[] args)
	{
		compiler.ReportWarning(input.BuildNameLineInfo(), res, args);
	}

	private void ReportNYI(string arg)
	{
		if (!input.ForwardCompatibility)
		{
			ReportError("'{0}' is not yet implemented.", arg);
		}
	}

	public void CheckError(bool cond, string res, params string[] args)
	{
		if (cond)
		{
			compiler.ReportError(input.BuildNameLineInfo(), res, args);
		}
	}
}
