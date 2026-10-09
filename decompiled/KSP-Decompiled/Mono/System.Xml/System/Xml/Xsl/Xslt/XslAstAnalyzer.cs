using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Xml.XPath;
using System.Xml.Xsl.Qil;
using System.Xml.Xsl.Runtime;
using System.Xml.Xsl.XPath;

namespace System.Xml.Xsl.Xslt;

internal class XslAstAnalyzer : XslVisitor<XslFlags>
{
	internal class Graph<V> : Dictionary<V, List<V>> where V : XslNode
	{
		private static IList<V> empty = new List<V>().AsReadOnly();

		public IEnumerable<V> GetAdjList(V v)
		{
			if (TryGetValue(v, out var value) && value != null)
			{
				return value;
			}
			return empty;
		}

		public void AddEdge(V v1, V v2)
		{
			if (v1 != v2)
			{
				if (!TryGetValue(v1, out var value) || value == null)
				{
					List<V> list2 = (base[v1] = new List<V>());
					value = list2;
				}
				value.Add(v2);
				if (!TryGetValue(v2, out value))
				{
					base[v2] = null;
				}
			}
		}

		public void PropagateFlag(XslFlags flag)
		{
			foreach (V key in base.Keys)
			{
				key.Flags &= ~XslFlags.Stop;
			}
			foreach (V key2 in base.Keys)
			{
				if ((key2.Flags & XslFlags.Stop) == 0 && (key2.Flags & flag) != 0)
				{
					DepthFirstSearch(key2, flag);
				}
			}
		}

		private void DepthFirstSearch(V v, XslFlags flag)
		{
			v.Flags |= flag | XslFlags.Stop;
			foreach (V adj in GetAdjList(v))
			{
				if ((adj.Flags & XslFlags.Stop) == 0)
				{
					DepthFirstSearch(adj, flag);
				}
			}
		}
	}

	internal struct ModeName
	{
		public QilName Mode;

		public QilName Name;

		public ModeName(QilName mode, QilName name)
		{
			Mode = mode;
			Name = name;
		}

		public override int GetHashCode()
		{
			return Mode.GetHashCode() ^ Name.GetHashCode();
		}
	}

	[StructLayout(LayoutKind.Sequential, Size = 1)]
	internal struct NullErrorHelper : IErrorHelper
	{
		public void ReportError(string res, params string[] args)
		{
		}

		public void ReportWarning(string res, params string[] args)
		{
		}
	}

	internal class XPathAnalyzer : IXPathBuilder<XslFlags>
	{
		private XPathParser<XslFlags> xpathParser = new XPathParser<XslFlags>();

		private CompilerScopeManager<VarPar> scope;

		private Compiler compiler;

		private bool xsltCurrentNeeded;

		private VarPar typeDonor;

		private static XslFlags[] OperatorType = new XslFlags[16]
		{
			XslFlags.TypeFilter,
			XslFlags.Boolean,
			XslFlags.Boolean,
			XslFlags.Boolean,
			XslFlags.Boolean,
			XslFlags.Boolean,
			XslFlags.Boolean,
			XslFlags.Boolean,
			XslFlags.Boolean,
			XslFlags.Number,
			XslFlags.Number,
			XslFlags.Number,
			XslFlags.Number,
			XslFlags.Number,
			XslFlags.Number,
			XslFlags.Nodeset
		};

		private static XslFlags[] XPathFunctionFlags = new XslFlags[27]
		{
			XslFlags.Number | XslFlags.Last,
			XslFlags.Number | XslFlags.Position,
			XslFlags.Number,
			XslFlags.String,
			XslFlags.String,
			XslFlags.String,
			XslFlags.String,
			XslFlags.Number,
			XslFlags.Boolean,
			XslFlags.Boolean,
			XslFlags.Boolean,
			XslFlags.Boolean,
			XslFlags.Nodeset | XslFlags.Current,
			XslFlags.String,
			XslFlags.Boolean,
			XslFlags.Boolean,
			XslFlags.String,
			XslFlags.String,
			XslFlags.String,
			XslFlags.Number,
			XslFlags.String,
			XslFlags.String,
			XslFlags.Boolean | XslFlags.Current,
			XslFlags.Number,
			XslFlags.Number,
			XslFlags.Number,
			XslFlags.Number
		};

		private static XslFlags[] XsltFunctionFlags = new XslFlags[9]
		{
			XslFlags.Node,
			XslFlags.Nodeset,
			XslFlags.Nodeset | XslFlags.Current,
			XslFlags.String,
			XslFlags.String,
			XslFlags.String,
			XslFlags.String | XslFlags.Number,
			XslFlags.Boolean,
			XslFlags.Boolean
		};

		public VarPar TypeDonor => typeDonor;

		public XPathAnalyzer(Compiler compiler, CompilerScopeManager<VarPar> scope)
		{
			this.compiler = compiler;
			this.scope = scope;
		}

		public XslFlags Analyze(string xpathExpr)
		{
			typeDonor = null;
			if (xpathExpr == null)
			{
				return XslFlags.None;
			}
			try
			{
				xsltCurrentNeeded = false;
				XPathScanner scanner = new XPathScanner(xpathExpr);
				XslFlags xslFlags = xpathParser.Parse(scanner, this, LexKind.Eof);
				if (xsltCurrentNeeded)
				{
					xslFlags |= XslFlags.Current;
				}
				return xslFlags;
			}
			catch (XslLoadException)
			{
				return XslFlags.TypeFilter | XslFlags.FocusFilter;
			}
		}

		public XslFlags AnalyzeAvt(string source)
		{
			typeDonor = null;
			if (source == null)
			{
				return XslFlags.None;
			}
			try
			{
				xsltCurrentNeeded = false;
				XslFlags xslFlags = XslFlags.None;
				int num = 0;
				while (num < source.Length)
				{
					num = source.IndexOf('{', num);
					if (num == -1)
					{
						break;
					}
					num++;
					if (num < source.Length && source[num] == '{')
					{
						num++;
					}
					else if (num < source.Length)
					{
						XPathScanner xPathScanner = new XPathScanner(source, num);
						xslFlags |= xpathParser.Parse(xPathScanner, this, LexKind.RBrace);
						num = xPathScanner.LexStart + 1;
					}
				}
				if (xsltCurrentNeeded)
				{
					xslFlags |= XslFlags.Current;
				}
				return xslFlags & ~XslFlags.TypeFilter;
			}
			catch (XslLoadException)
			{
				return XslFlags.FocusFilter;
			}
		}

		private VarPar ResolveVariable(string prefix, string name)
		{
			string text = ResolvePrefix(prefix);
			if (text == null)
			{
				return null;
			}
			return scope.LookupVariable(name, text);
		}

		private string ResolvePrefix(string prefix)
		{
			if (prefix.Length == 0)
			{
				return string.Empty;
			}
			return scope.LookupNamespace(prefix);
		}

		public virtual void StartBuild()
		{
		}

		public virtual XslFlags EndBuild(XslFlags result)
		{
			return result;
		}

		public virtual XslFlags String(string value)
		{
			typeDonor = null;
			return XslFlags.String;
		}

		public virtual XslFlags Number(double value)
		{
			typeDonor = null;
			return XslFlags.Number;
		}

		public virtual XslFlags Operator(XPathOperator op, XslFlags left, XslFlags right)
		{
			typeDonor = null;
			return ((left | right) & ~XslFlags.TypeFilter) | OperatorType[(int)op];
		}

		public virtual XslFlags Axis(XPathAxis xpathAxis, XPathNodeType nodeType, string prefix, string name)
		{
			typeDonor = null;
			if (xpathAxis == XPathAxis.Self && nodeType == XPathNodeType.All && prefix == null && name == null)
			{
				return XslFlags.Node | XslFlags.Current;
			}
			return XslFlags.Nodeset | XslFlags.Current;
		}

		public virtual XslFlags JoinStep(XslFlags left, XslFlags right)
		{
			typeDonor = null;
			return (left & ~XslFlags.TypeFilter) | XslFlags.Nodeset;
		}

		public virtual XslFlags Predicate(XslFlags nodeset, XslFlags predicate, bool isReverseStep)
		{
			typeDonor = null;
			return (nodeset & ~XslFlags.TypeFilter) | XslFlags.Nodeset | (predicate & XslFlags.SideEffects);
		}

		public virtual XslFlags Variable(string prefix, string name)
		{
			typeDonor = ResolveVariable(prefix, name);
			if (typeDonor == null)
			{
				return XslFlags.TypeFilter;
			}
			return XslFlags.None;
		}

		public virtual XslFlags Function(string prefix, string name, IList<XslFlags> args)
		{
			typeDonor = null;
			XslFlags xslFlags = XslFlags.None;
			foreach (XslFlags arg in args)
			{
				xslFlags |= arg;
			}
			XslFlags xslFlags2 = XslFlags.None;
			if (prefix.Length == 0)
			{
				XPathBuilder.FunctionInfo<QilGenerator.FuncId> value2;
				if (XPathBuilder.FunctionTable.TryGetValue(name, out var value))
				{
					XPathBuilder.FuncId id = value.id;
					xslFlags2 = XPathFunctionFlags[(int)id];
					if (args.Count == 0 && (id == XPathBuilder.FuncId.LocalName || id == XPathBuilder.FuncId.NamespaceUri || id == XPathBuilder.FuncId.Name || id == XPathBuilder.FuncId.String || id == XPathBuilder.FuncId.Number || id == XPathBuilder.FuncId.StringLength || id == XPathBuilder.FuncId.Normalize))
					{
						xslFlags2 |= XslFlags.Current;
					}
				}
				else if (QilGenerator.FunctionTable.TryGetValue(name, out value2))
				{
					QilGenerator.FuncId id2 = value2.id;
					xslFlags2 = XsltFunctionFlags[(int)id2];
					switch (id2)
					{
					case QilGenerator.FuncId.Current:
						xsltCurrentNeeded = true;
						break;
					case QilGenerator.FuncId.GenerateId:
						if (args.Count == 0)
						{
							xslFlags2 |= XslFlags.Current;
						}
						break;
					}
				}
			}
			else
			{
				string text = ResolvePrefix(prefix);
				if (text == "urn:schemas-microsoft-com:xslt")
				{
					switch (name)
					{
					case "node-set":
						xslFlags2 = XslFlags.Nodeset;
						break;
					case "string-compare":
						xslFlags2 = XslFlags.Number;
						break;
					case "utc":
						xslFlags2 = XslFlags.String;
						break;
					case "format-date":
						xslFlags2 = XslFlags.String;
						break;
					case "format-time":
						xslFlags2 = XslFlags.String;
						break;
					case "local-name":
						xslFlags2 = XslFlags.String;
						break;
					case "namespace-uri":
						xslFlags2 = XslFlags.String | XslFlags.Current;
						break;
					case "number":
						xslFlags2 = XslFlags.Number;
						break;
					}
				}
				else if (text == "http://exslt.org/common")
				{
					if (!(name == "node-set"))
					{
						if (name == "object-type")
						{
							xslFlags2 = XslFlags.String;
						}
					}
					else
					{
						xslFlags2 = XslFlags.Nodeset;
					}
				}
				if (xslFlags2 == XslFlags.None)
				{
					xslFlags2 = XslFlags.TypeFilter;
					if (compiler.Settings.EnableScript && text != null)
					{
						XmlExtensionFunction xmlExtensionFunction = compiler.Scripts.ResolveFunction(name, text, args.Count, default(NullErrorHelper));
						if (xmlExtensionFunction != null)
						{
							XmlQueryType xmlReturnType = xmlExtensionFunction.XmlReturnType;
							if (xmlReturnType == XmlQueryTypeFactory.StringX)
							{
								xslFlags2 = XslFlags.String;
							}
							else if (xmlReturnType == XmlQueryTypeFactory.DoubleX)
							{
								xslFlags2 = XslFlags.Number;
							}
							else if (xmlReturnType == XmlQueryTypeFactory.BooleanX)
							{
								xslFlags2 = XslFlags.Boolean;
							}
							else if (xmlReturnType == XmlQueryTypeFactory.NodeNotRtf)
							{
								xslFlags2 = XslFlags.Node;
							}
							else if (xmlReturnType == XmlQueryTypeFactory.NodeSDod)
							{
								xslFlags2 = XslFlags.Nodeset;
							}
							else if (xmlReturnType == XmlQueryTypeFactory.ItemS)
							{
								xslFlags2 = XslFlags.TypeFilter;
							}
							else if (xmlReturnType == XmlQueryTypeFactory.Empty)
							{
								xslFlags2 = XslFlags.Nodeset;
							}
						}
					}
					xslFlags2 |= XslFlags.SideEffects;
				}
			}
			return (xslFlags & ~XslFlags.TypeFilter) | xslFlags2;
		}
	}

	private CompilerScopeManager<VarPar> scope;

	private Compiler compiler;

	private int forEachDepth;

	private XPathAnalyzer xpathAnalyzer;

	private ProtoTemplate currentTemplate;

	private VarPar typeDonor;

	private Graph<ProtoTemplate> revCall0Graph = new Graph<ProtoTemplate>();

	private Graph<ProtoTemplate> revCall1Graph = new Graph<ProtoTemplate>();

	private Dictionary<Template, Stylesheet> fwdApplyImportsGraph = new Dictionary<Template, Stylesheet>();

	private Dictionary<QilName, List<ProtoTemplate>> revApplyTemplatesGraph = new Dictionary<QilName, List<ProtoTemplate>>();

	private Graph<VarPar> dataFlow = new Graph<VarPar>();

	private Dictionary<ModeName, VarPar> applyTemplatesParams = new Dictionary<ModeName, VarPar>();

	public XslFlags Analyze(Compiler compiler)
	{
		this.compiler = compiler;
		scope = new CompilerScopeManager<VarPar>();
		xpathAnalyzer = new XPathAnalyzer(compiler, scope);
		foreach (VarPar externalPar in compiler.ExternalPars)
		{
			scope.AddVariable(externalPar.Name, externalPar);
		}
		foreach (VarPar globalVar in compiler.GlobalVars)
		{
			scope.AddVariable(globalVar.Name, globalVar);
		}
		foreach (VarPar externalPar2 in compiler.ExternalPars)
		{
			Visit(externalPar2);
			externalPar2.Flags |= XslFlags.TypeFilter;
		}
		foreach (VarPar globalVar2 in compiler.GlobalVars)
		{
			Visit(globalVar2);
		}
		XslFlags xslFlags = XslFlags.None;
		foreach (ProtoTemplate allTemplate in compiler.AllTemplates)
		{
			xslFlags |= Visit(currentTemplate = allTemplate);
		}
		foreach (ProtoTemplate allTemplate2 in compiler.AllTemplates)
		{
			foreach (XslNode item in allTemplate2.Content)
			{
				if (item.NodeType != XslNodeType.Text)
				{
					if (item.NodeType != XslNodeType.Param)
					{
						break;
					}
					VarPar varPar = (VarPar)item;
					if ((varPar.Flags & XslFlags.MayBeDefault) != 0)
					{
						varPar.Flags |= varPar.DefValueFlags;
					}
				}
			}
		}
		for (int num = 32; num != 0; num >>= 1)
		{
			dataFlow.PropagateFlag((XslFlags)num);
		}
		dataFlow = null;
		foreach (KeyValuePair<Template, Stylesheet> item2 in fwdApplyImportsGraph)
		{
			Stylesheet[] imports = item2.Value.Imports;
			foreach (Stylesheet sheet in imports)
			{
				AddImportDependencies(sheet, item2.Key);
			}
		}
		fwdApplyImportsGraph = null;
		if ((xslFlags & XslFlags.Current) != 0)
		{
			revCall0Graph.PropagateFlag(XslFlags.Current);
		}
		if ((xslFlags & XslFlags.Position) != 0)
		{
			revCall0Graph.PropagateFlag(XslFlags.Position);
		}
		if ((xslFlags & XslFlags.Last) != 0)
		{
			revCall0Graph.PropagateFlag(XslFlags.Last);
		}
		if ((xslFlags & XslFlags.SideEffects) != 0)
		{
			PropagateSideEffectsFlag();
		}
		revCall0Graph = null;
		revCall1Graph = null;
		revApplyTemplatesGraph = null;
		FillModeFlags(compiler.Root.ModeFlags, compiler.Root.Imports[0]);
		TraceResults();
		return xslFlags;
	}

	private void AddImportDependencies(Stylesheet sheet, Template focusDonor)
	{
		foreach (Template template in sheet.Templates)
		{
			if (template.Mode.Equals(focusDonor.Mode))
			{
				revCall0Graph.AddEdge(template, focusDonor);
			}
		}
		Stylesheet[] imports = sheet.Imports;
		foreach (Stylesheet sheet2 in imports)
		{
			AddImportDependencies(sheet2, focusDonor);
		}
	}

	private void FillModeFlags(Dictionary<QilName, XslFlags> parentModeFlags, Stylesheet sheet)
	{
		Stylesheet[] imports = sheet.Imports;
		foreach (Stylesheet sheet2 in imports)
		{
			FillModeFlags(sheet.ModeFlags, sheet2);
		}
		foreach (KeyValuePair<QilName, XslFlags> modeFlag in sheet.ModeFlags)
		{
			if (!parentModeFlags.TryGetValue(modeFlag.Key, out var value))
			{
				value = XslFlags.None;
			}
			parentModeFlags[modeFlag.Key] = value | modeFlag.Value;
		}
		foreach (Template template in sheet.Templates)
		{
			XslFlags xslFlags = template.Flags & (XslFlags.FocusFilter | XslFlags.SideEffects);
			if (xslFlags != 0)
			{
				if (!parentModeFlags.TryGetValue(template.Mode, out var value2))
				{
					value2 = XslFlags.None;
				}
				parentModeFlags[template.Mode] = value2 | xslFlags;
			}
		}
	}

	private void TraceResults()
	{
	}

	protected override XslFlags Visit(XslNode node)
	{
		scope.EnterScope(node.Namespaces);
		XslFlags result = base.Visit(node);
		scope.ExitScope();
		if (currentTemplate != null && (node.NodeType == XslNodeType.Variable || node.NodeType == XslNodeType.Param))
		{
			scope.AddVariable(node.Name, (VarPar)node);
		}
		return result;
	}

	protected override XslFlags VisitChildren(XslNode node)
	{
		XslFlags xslFlags = XslFlags.None;
		foreach (XslNode item in node.Content)
		{
			xslFlags |= Visit(item);
		}
		return xslFlags;
	}

	protected override XslFlags VisitAttributeSet(AttributeSet node)
	{
		node.Flags = VisitChildren(node);
		return node.Flags;
	}

	protected override XslFlags VisitTemplate(Template node)
	{
		node.Flags = VisitChildren(node);
		return node.Flags;
	}

	protected override XslFlags VisitApplyImports(XslNode node)
	{
		fwdApplyImportsGraph[(Template)currentTemplate] = (Stylesheet)node.Arg;
		return XslFlags.Rtf | XslFlags.Current | XslFlags.HasCalls;
	}

	protected override XslFlags VisitApplyTemplates(XslNode node)
	{
		XslFlags xslFlags = ProcessExpr(node.Select);
		foreach (XslNode item in node.Content)
		{
			xslFlags |= Visit(item);
			if (item.NodeType == XslNodeType.WithParam)
			{
				ModeName key = new ModeName(node.Name, item.Name);
				if (!applyTemplatesParams.TryGetValue(key, out var value))
				{
					VarPar varPar2 = (applyTemplatesParams[key] = AstFactory.WithParam(item.Name));
					value = varPar2;
				}
				if (typeDonor != null)
				{
					dataFlow.AddEdge(typeDonor, value);
				}
				else
				{
					value.Flags |= item.Flags & XslFlags.TypeFilter;
				}
			}
		}
		if (currentTemplate != null)
		{
			AddApplyTemplatesEdge(node.Name, currentTemplate);
		}
		return XslFlags.Rtf | XslFlags.HasCalls | xslFlags;
	}

	protected override XslFlags VisitAttribute(NodeCtor node)
	{
		return XslFlags.Rtf | ProcessAvt(node.NameAvt) | ProcessAvt(node.NsAvt) | VisitChildren(node);
	}

	protected override XslFlags VisitCallTemplate(XslNode node)
	{
		XslFlags xslFlags = XslFlags.None;
		if (compiler.NamedTemplates.TryGetValue(node.Name, out var value) && currentTemplate != null)
		{
			if (forEachDepth == 0)
			{
				revCall0Graph.AddEdge(value, currentTemplate);
			}
			else
			{
				revCall1Graph.AddEdge(value, currentTemplate);
			}
		}
		VarPar[] array = new VarPar[node.Content.Count];
		int num = 0;
		foreach (XslNode item in node.Content)
		{
			xslFlags |= Visit(item);
			array[num++] = typeDonor;
		}
		if (value != null)
		{
			foreach (XslNode item2 in value.Content)
			{
				if (item2.NodeType == XslNodeType.Text)
				{
					continue;
				}
				if (item2.NodeType != XslNodeType.Param)
				{
					break;
				}
				VarPar varPar = (VarPar)item2;
				VarPar varPar2 = null;
				num = 0;
				foreach (XslNode item3 in node.Content)
				{
					if (item3.Name.Equals(varPar.Name))
					{
						varPar2 = (VarPar)item3;
						typeDonor = array[num];
						break;
					}
					num++;
				}
				if (varPar2 != null)
				{
					if (typeDonor != null)
					{
						dataFlow.AddEdge(typeDonor, varPar);
					}
					else
					{
						varPar.Flags |= varPar2.Flags & XslFlags.TypeFilter;
					}
				}
				else
				{
					varPar.Flags |= XslFlags.MayBeDefault;
				}
			}
		}
		return XslFlags.Rtf | XslFlags.HasCalls | xslFlags;
	}

	protected override XslFlags VisitComment(XslNode node)
	{
		return XslFlags.Rtf | VisitChildren(node);
	}

	protected override XslFlags VisitCopy(XslNode node)
	{
		return XslFlags.Rtf | XslFlags.Current | VisitChildren(node);
	}

	protected override XslFlags VisitCopyOf(XslNode node)
	{
		return XslFlags.Rtf | ProcessExpr(node.Select);
	}

	protected override XslFlags VisitElement(NodeCtor node)
	{
		return XslFlags.Rtf | ProcessAvt(node.NameAvt) | ProcessAvt(node.NsAvt) | VisitChildren(node);
	}

	protected override XslFlags VisitError(XslNode node)
	{
		return (VisitChildren(node) & ~XslFlags.TypeFilter) | XslFlags.SideEffects;
	}

	protected override XslFlags VisitForEach(XslNode node)
	{
		XslFlags xslFlags = ProcessExpr(node.Select);
		forEachDepth++;
		foreach (XslNode item in node.Content)
		{
			xslFlags = ((item.NodeType != XslNodeType.Sort) ? (xslFlags | (Visit(item) & ~XslFlags.FocusFilter)) : (xslFlags | Visit(item)));
		}
		forEachDepth--;
		return xslFlags;
	}

	protected override XslFlags VisitIf(XslNode node)
	{
		return ProcessExpr(node.Select) | VisitChildren(node);
	}

	protected override XslFlags VisitLiteralAttribute(XslNode node)
	{
		return XslFlags.Rtf | ProcessAvt(node.Select) | VisitChildren(node);
	}

	protected override XslFlags VisitLiteralElement(XslNode node)
	{
		return XslFlags.Rtf | VisitChildren(node);
	}

	protected override XslFlags VisitMessage(XslNode node)
	{
		return (VisitChildren(node) & ~XslFlags.TypeFilter) | XslFlags.SideEffects;
	}

	protected override XslFlags VisitNumber(Number node)
	{
		return XslFlags.Rtf | ProcessPattern(node.Count) | ProcessPattern(node.From) | ((node.Value != null) ? ProcessExpr(node.Value) : XslFlags.Current) | ProcessAvt(node.Format) | ProcessAvt(node.Lang) | ProcessAvt(node.LetterValue) | ProcessAvt(node.GroupingSeparator) | ProcessAvt(node.GroupingSize);
	}

	protected override XslFlags VisitPI(XslNode node)
	{
		return XslFlags.Rtf | ProcessAvt(node.Select) | VisitChildren(node);
	}

	protected override XslFlags VisitSort(Sort node)
	{
		return (ProcessExpr(node.Select) & ~XslFlags.FocusFilter) | ProcessAvt(node.Lang) | ProcessAvt(node.DataType) | ProcessAvt(node.Order) | ProcessAvt(node.CaseOrder);
	}

	protected override XslFlags VisitText(Text node)
	{
		return XslFlags.Rtf | VisitChildren(node);
	}

	protected override XslFlags VisitUseAttributeSet(XslNode node)
	{
		if (compiler.AttributeSets.TryGetValue(node.Name, out var value) && currentTemplate != null)
		{
			if (forEachDepth == 0)
			{
				revCall0Graph.AddEdge(value, currentTemplate);
			}
			else
			{
				revCall1Graph.AddEdge(value, currentTemplate);
			}
		}
		return XslFlags.Rtf | XslFlags.HasCalls;
	}

	protected override XslFlags VisitValueOf(XslNode node)
	{
		return XslFlags.Rtf | ProcessExpr(node.Select);
	}

	protected override XslFlags VisitValueOfDoe(XslNode node)
	{
		return XslFlags.Rtf | ProcessExpr(node.Select);
	}

	protected override XslFlags VisitParam(VarPar node)
	{
		if (currentTemplate is Template { Match: not null } template)
		{
			node.Flags |= XslFlags.MayBeDefault;
			ModeName key = new ModeName(template.Mode, node.Name);
			if (!applyTemplatesParams.TryGetValue(key, out var value))
			{
				VarPar varPar2 = (applyTemplatesParams[key] = AstFactory.WithParam(node.Name));
				value = varPar2;
			}
			dataFlow.AddEdge(value, node);
		}
		node.DefValueFlags = ProcessVarPar(node);
		return node.DefValueFlags & ~XslFlags.TypeFilter;
	}

	protected override XslFlags VisitVariable(VarPar node)
	{
		node.Flags = ProcessVarPar(node);
		return node.Flags & ~XslFlags.TypeFilter;
	}

	protected override XslFlags VisitWithParam(VarPar node)
	{
		node.Flags = ProcessVarPar(node);
		return node.Flags & ~XslFlags.TypeFilter;
	}

	private XslFlags ProcessVarPar(VarPar node)
	{
		XslFlags result;
		if (node.Select != null)
		{
			if (node.Content.Count != 0)
			{
				result = xpathAnalyzer.Analyze(node.Select) | VisitChildren(node) | XslFlags.TypeFilter;
				typeDonor = null;
			}
			else
			{
				result = xpathAnalyzer.Analyze(node.Select);
				typeDonor = xpathAnalyzer.TypeDonor;
				if (typeDonor != null && node.NodeType != XslNodeType.WithParam)
				{
					dataFlow.AddEdge(typeDonor, node);
				}
			}
		}
		else if (node.Content.Count != 0)
		{
			result = XslFlags.Rtf | VisitChildren(node);
			typeDonor = null;
		}
		else
		{
			result = XslFlags.String;
			typeDonor = null;
		}
		return result;
	}

	private XslFlags ProcessExpr(string expr)
	{
		return xpathAnalyzer.Analyze(expr) & ~XslFlags.TypeFilter;
	}

	private XslFlags ProcessAvt(string avt)
	{
		return xpathAnalyzer.AnalyzeAvt(avt) & ~XslFlags.TypeFilter;
	}

	private XslFlags ProcessPattern(string pattern)
	{
		return xpathAnalyzer.Analyze(pattern) & ~XslFlags.TypeFilter & ~XslFlags.FocusFilter;
	}

	private void AddApplyTemplatesEdge(QilName mode, ProtoTemplate dependentTemplate)
	{
		if (!revApplyTemplatesGraph.TryGetValue(mode, out var value))
		{
			value = new List<ProtoTemplate>();
			revApplyTemplatesGraph.Add(mode, value);
		}
		else if (value[value.Count - 1] == dependentTemplate)
		{
			return;
		}
		value.Add(dependentTemplate);
	}

	private void PropagateSideEffectsFlag()
	{
		foreach (ProtoTemplate key in revCall0Graph.Keys)
		{
			key.Flags &= ~XslFlags.Stop;
		}
		foreach (ProtoTemplate key2 in revCall1Graph.Keys)
		{
			key2.Flags &= ~XslFlags.Stop;
		}
		foreach (ProtoTemplate key3 in revCall0Graph.Keys)
		{
			if ((key3.Flags & XslFlags.Stop) == 0 && (key3.Flags & XslFlags.SideEffects) != 0)
			{
				DepthFirstSearch(key3);
			}
		}
		foreach (ProtoTemplate key4 in revCall1Graph.Keys)
		{
			if ((key4.Flags & XslFlags.Stop) == 0 && (key4.Flags & XslFlags.SideEffects) != 0)
			{
				DepthFirstSearch(key4);
			}
		}
	}

	private void DepthFirstSearch(ProtoTemplate t)
	{
		t.Flags |= XslFlags.SideEffects | XslFlags.Stop;
		foreach (ProtoTemplate adj in revCall0Graph.GetAdjList(t))
		{
			if ((adj.Flags & XslFlags.Stop) == 0)
			{
				DepthFirstSearch(adj);
			}
		}
		foreach (ProtoTemplate adj2 in revCall1Graph.GetAdjList(t))
		{
			if ((adj2.Flags & XslFlags.Stop) == 0)
			{
				DepthFirstSearch(adj2);
			}
		}
		if (!(t is Template template) || !revApplyTemplatesGraph.TryGetValue(template.Mode, out var value))
		{
			return;
		}
		revApplyTemplatesGraph.Remove(template.Mode);
		foreach (ProtoTemplate item in value)
		{
			if ((item.Flags & XslFlags.Stop) == 0)
			{
				DepthFirstSearch(item);
			}
		}
	}
}
