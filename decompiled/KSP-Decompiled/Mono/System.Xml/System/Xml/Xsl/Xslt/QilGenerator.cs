using System.Collections.Generic;
using System.Collections.Specialized;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Text;
using System.Xml.Schema;
using System.Xml.XPath;
using System.Xml.Xsl.Qil;
using System.Xml.Xsl.Runtime;
using System.Xml.Xsl.XPath;

namespace System.Xml.Xsl.Xslt;

internal class QilGenerator : IErrorHelper, IXPathEnvironment, IFocus
{
	private class VariableHelper
	{
		private Stack<QilIterator> vars = new Stack<QilIterator>();

		private XPathQilFactory f;

		public VariableHelper(XPathQilFactory f)
		{
			this.f = f;
		}

		public int StartVariables()
		{
			return vars.Count;
		}

		public void AddVariable(QilIterator let)
		{
			vars.Push(let);
		}

		public QilNode FinishVariables(QilNode node, int varScope)
		{
			int num = vars.Count - varScope;
			while (num-- != 0)
			{
				node = f.Loop(vars.Pop(), node);
			}
			return node;
		}

		[Conditional("DEBUG")]
		public void CheckEmpty()
		{
		}
	}

	[StructLayout(LayoutKind.Sequential, Size = 1)]
	private struct ThrowErrorHelper : IErrorHelper
	{
		public void ReportError(string res, params string[] args)
		{
			throw new XslLoadException("{0}", res);
		}

		public void ReportWarning(string res, params string[] args)
		{
		}
	}

	public enum FuncId
	{
		Current,
		Document,
		Key,
		FormatNumber,
		UnparsedEntityUri,
		GenerateId,
		SystemProperty,
		ElementAvailable,
		FunctionAvailable
	}

	private CompilerScopeManager<QilIterator> scope;

	private OutputScopeManager outputScope;

	private HybridDictionary prefixesInUse;

	private XsltQilFactory f;

	private XPathBuilder xpathBuilder;

	private XPathParser<QilNode> xpathParser;

	private XPathPatternBuilder ptrnBuilder;

	private XPathPatternParser ptrnParser;

	private ReferenceReplacer refReplacer;

	private KeyMatchBuilder keyMatchBuilder;

	private InvokeGenerator invkGen;

	private MatcherBuilder matcherBuilder;

	private QilStrConcatenator strConcat;

	private VariableHelper varHelper;

	private Compiler compiler;

	private QilList functions;

	private QilFunction generalKey;

	private bool formatNumberDynamicUsed;

	private QilList extPars;

	private QilList gloVars;

	private QilList nsVars;

	private XmlQueryType elementOrDocumentType;

	private XmlQueryType textOrAttributeType;

	private XslNode lastScope;

	private XslVersion xslVersion;

	private QilName nameCurrent;

	private QilName namePosition;

	private QilName nameLast;

	private QilName nameNamespaces;

	private QilName nameInit;

	private SingletonFocus singlFocus;

	private FunctionFocus funcFocus;

	private LoopFocus curLoop;

	private int formatterCnt;

	private readonly StringBuilder unescapedText = new StringBuilder();

	private static readonly char[] curlyBraces = new char[2] { '{', '}' };

	private const XmlNodeKindFlags InvalidatingNodes = XmlNodeKindFlags.Attribute | XmlNodeKindFlags.Namespace;

	private bool allowVariables = true;

	private bool allowCurrent = true;

	private bool allowKey = true;

	private static readonly XmlTypeCode[] argFnDocument = new XmlTypeCode[2]
	{
		XmlTypeCode.Item,
		XmlTypeCode.Node
	};

	private static readonly XmlTypeCode[] argFnKey = new XmlTypeCode[2]
	{
		XmlTypeCode.String,
		XmlTypeCode.Item
	};

	private static readonly XmlTypeCode[] argFnFormatNumber = new XmlTypeCode[3]
	{
		XmlTypeCode.Double,
		XmlTypeCode.String,
		XmlTypeCode.String
	};

	public static Dictionary<string, XPathBuilder.FunctionInfo<FuncId>> FunctionTable = CreateFunctionTable();

	private bool IsDebug => compiler.IsDebug;

	private bool EvaluateFuncCalls => !IsDebug;

	private bool InferXPathTypes => !IsDebug;

	XPathQilFactory IXPathEnvironment.Factory => f;

	public static QilExpression CompileStylesheet(Compiler compiler)
	{
		return new QilGenerator(compiler.IsDebug).Compile(compiler);
	}

	private QilGenerator(bool debug)
	{
		scope = new CompilerScopeManager<QilIterator>();
		outputScope = new OutputScopeManager();
		prefixesInUse = new HybridDictionary();
		f = new XsltQilFactory(new QilFactory(), debug);
		xpathBuilder = new XPathBuilder(this);
		xpathParser = new XPathParser<QilNode>();
		ptrnBuilder = new XPathPatternBuilder(this);
		ptrnParser = new XPathPatternParser();
		refReplacer = new ReferenceReplacer(f.BaseFactory);
		invkGen = new InvokeGenerator(f, debug);
		matcherBuilder = new MatcherBuilder(f, refReplacer, invkGen);
		singlFocus = new SingletonFocus(f);
		funcFocus = default(FunctionFocus);
		curLoop = new LoopFocus(f);
		strConcat = new QilStrConcatenator(f);
		varHelper = new VariableHelper(f);
		elementOrDocumentType = XmlQueryTypeFactory.DocumentOrElement;
		textOrAttributeType = XmlQueryTypeFactory.NodeChoice(XmlNodeKindFlags.Attribute | XmlNodeKindFlags.Text);
		nameCurrent = f.QName("current", "urn:schemas-microsoft-com:xslt-debug");
		namePosition = f.QName("position", "urn:schemas-microsoft-com:xslt-debug");
		nameLast = f.QName("last", "urn:schemas-microsoft-com:xslt-debug");
		nameNamespaces = f.QName("namespaces", "urn:schemas-microsoft-com:xslt-debug");
		nameInit = f.QName("init", "urn:schemas-microsoft-com:xslt-debug");
		formatterCnt = 0;
	}

	private QilExpression Compile(Compiler compiler)
	{
		this.compiler = compiler;
		functions = f.FunctionList();
		extPars = f.GlobalParameterList();
		gloVars = f.GlobalVariableList();
		nsVars = f.GlobalVariableList();
		compiler.Scripts.CompileScripts();
		new XslAstRewriter().Rewrite(compiler);
		if (!IsDebug)
		{
			new XslAstAnalyzer().Analyze(compiler);
		}
		CreateGlobalVarPars();
		try
		{
			CompileKeys();
			CompileAndSortMatches(compiler.Root.Imports[0]);
			PrecompileProtoTemplatesHeaders();
			CompileGlobalVariables();
			foreach (ProtoTemplate allTemplate in compiler.AllTemplates)
			{
				CompileProtoTemplate(allTemplate);
			}
		}
		catch (XslLoadException ex)
		{
			ex.SetSourceLineInfo(lastScope.SourceLine);
			throw;
		}
		catch (Exception ex2)
		{
			if (!XmlException.IsCatchableException(ex2))
			{
				throw;
			}
			throw new XslLoadException(ex2, lastScope.SourceLine);
		}
		CompileInitializationCode();
		QilNode root = CompileRootExpression(compiler.StartApplyTemplates);
		foreach (ProtoTemplate allTemplate2 in compiler.AllTemplates)
		{
			foreach (QilParameter argument in allTemplate2.Function.Arguments)
			{
				if (!IsDebug || argument.Name.Equals(nameNamespaces))
				{
					argument.DefaultValue = null;
				}
			}
		}
		Dictionary<string, Type> scriptClasses = compiler.Scripts.ScriptClasses;
		List<EarlyBoundInfo> list = new List<EarlyBoundInfo>(scriptClasses.Count);
		foreach (KeyValuePair<string, Type> item in scriptClasses)
		{
			if (item.Value != null)
			{
				list.Add(new EarlyBoundInfo(item.Key, item.Value));
			}
		}
		QilExpression qilExpression = f.QilExpression(root, f.BaseFactory);
		qilExpression.EarlyBoundTypes = list;
		qilExpression.FunctionList = functions;
		qilExpression.GlobalParameterList = extPars;
		qilExpression.GlobalVariableList = gloVars;
		qilExpression.WhitespaceRules = compiler.WhitespaceRules;
		qilExpression.IsDebug = IsDebug;
		qilExpression.DefaultWriterSettings = compiler.Output.Settings;
		QilDepthChecker.Check(qilExpression);
		return qilExpression;
	}

	private QilNode InvokeOnCurrentNodeChanged()
	{
		QilIterator qilIterator;
		return f.Loop(qilIterator = f.Let(f.InvokeOnCurrentNodeChanged(curLoop.GetCurrent())), f.Sequence());
	}

	[Conditional("DEBUG")]
	private void CheckSingletonFocus()
	{
	}

	private void CompileInitializationCode()
	{
		QilNode qilNode = f.Int32(0);
		if (formatNumberDynamicUsed || IsDebug)
		{
			bool flag = false;
			foreach (DecimalFormatDecl decimalFormat in compiler.DecimalFormats)
			{
				qilNode = f.Add(qilNode, f.InvokeRegisterDecimalFormat(decimalFormat));
				flag |= decimalFormat.Name == DecimalFormatDecl.Default.Name;
			}
			if (!flag)
			{
				qilNode = f.Add(qilNode, f.InvokeRegisterDecimalFormat(DecimalFormatDecl.Default));
			}
		}
		foreach (string key in compiler.Scripts.ScriptClasses.Keys)
		{
			qilNode = f.Add(qilNode, f.InvokeCheckScriptNamespace(key));
		}
		if (qilNode.NodeType == QilNodeType.Add)
		{
			QilFunction qilFunction = f.Function(f.FormalParameterList(), qilNode, f.True());
			qilFunction.DebugName = "Init";
			functions.Add(qilFunction);
			QilNode qilNode2 = f.Invoke(qilFunction, f.ActualParameterList());
			if (IsDebug)
			{
				qilNode2 = f.TypeAssert(qilNode2, XmlQueryTypeFactory.ItemS);
			}
			QilIterator qilIterator = f.Let(qilNode2);
			qilIterator.DebugName = nameInit.ToString();
			gloVars.Insert(0, qilIterator);
		}
	}

	private QilNode CompileRootExpression(XslNode applyTmpls)
	{
		singlFocus.SetFocus(SingletonFocusType.InitialContextNode);
		QilNode child = GenerateApply(compiler.Root, applyTmpls);
		singlFocus.SetFocus(null);
		return f.DocumentCtor(child);
	}

	private QilList EnterScope(XslNode node)
	{
		lastScope = node;
		xslVersion = node.XslVersion;
		if (scope.EnterScope(node.Namespaces))
		{
			return BuildDebuggerNamespaces();
		}
		return null;
	}

	private void ExitScope()
	{
		scope.ExitScope();
	}

	private QilList BuildDebuggerNamespaces()
	{
		if (IsDebug)
		{
			QilList qilList = f.BaseFactory.Sequence();
			CompilerScopeManager<QilIterator>.NamespaceEnumerator enumerator = scope.GetEnumerator();
			while (enumerator.MoveNext())
			{
				CompilerScopeManager<QilIterator>.ScopeRecord current = enumerator.Current;
				qilList.Add(f.NamespaceDecl(f.String(current.ncName), f.String(current.nsUri)));
			}
			return qilList;
		}
		return null;
	}

	private QilNode GetCurrentNode()
	{
		if (curLoop.IsFocusSet)
		{
			return curLoop.GetCurrent();
		}
		if (funcFocus.IsFocusSet)
		{
			return funcFocus.GetCurrent();
		}
		return singlFocus.GetCurrent();
	}

	private QilNode GetCurrentPosition()
	{
		if (curLoop.IsFocusSet)
		{
			return curLoop.GetPosition();
		}
		if (funcFocus.IsFocusSet)
		{
			return funcFocus.GetPosition();
		}
		return singlFocus.GetPosition();
	}

	private QilNode GetLastPosition()
	{
		if (curLoop.IsFocusSet)
		{
			return curLoop.GetLast();
		}
		if (funcFocus.IsFocusSet)
		{
			return funcFocus.GetLast();
		}
		return singlFocus.GetLast();
	}

	private XmlQueryType ChooseBestType(VarPar var)
	{
		if (IsDebug || !InferXPathTypes)
		{
			return XmlQueryTypeFactory.ItemS;
		}
		return (var.Flags & XslFlags.TypeFilter) switch
		{
			XslFlags.String => XmlQueryTypeFactory.StringX, 
			XslFlags.Number => XmlQueryTypeFactory.DoubleX, 
			XslFlags.Boolean => XmlQueryTypeFactory.BooleanX, 
			XslFlags.Node => XmlQueryTypeFactory.NodeNotRtf, 
			XslFlags.Nodeset => XmlQueryTypeFactory.NodeNotRtfS, 
			XslFlags.Rtf => XmlQueryTypeFactory.Node, 
			XslFlags.Node | XslFlags.Rtf => XmlQueryTypeFactory.Node, 
			XslFlags.Node | XslFlags.Nodeset => XmlQueryTypeFactory.NodeNotRtfS, 
			XslFlags.Nodeset | XslFlags.Rtf => XmlQueryTypeFactory.NodeS, 
			XslFlags.Node | XslFlags.Nodeset | XslFlags.Rtf => XmlQueryTypeFactory.NodeS, 
			_ => XmlQueryTypeFactory.ItemS, 
		};
	}

	private QilIterator GetNsVar(QilList nsList)
	{
		foreach (QilIterator nsVar in nsVars)
		{
			QilList qilList = (QilList)nsVar.Binding;
			if (qilList.Count != nsList.Count)
			{
				continue;
			}
			bool flag = true;
			for (int i = 0; i < nsList.Count; i++)
			{
				if (((QilLiteral)((QilBinary)nsList[i]).Right).Value != ((QilLiteral)((QilBinary)qilList[i]).Right).Value || ((QilLiteral)((QilBinary)nsList[i]).Left).Value != ((QilLiteral)((QilBinary)qilList[i]).Left).Value)
				{
					flag = false;
					break;
				}
			}
			if (flag)
			{
				return nsVar;
			}
		}
		QilIterator qilIterator2 = f.Let(nsList);
		qilIterator2.DebugName = f.QName("ns" + nsVars.Count, "urn:schemas-microsoft-com:xslt-debug").ToString();
		gloVars.Add(qilIterator2);
		nsVars.Add(qilIterator2);
		return qilIterator2;
	}

	private void PrecompileProtoTemplatesHeaders()
	{
		List<VarPar> list = null;
		Dictionary<VarPar, Template> dictionary = null;
		Dictionary<VarPar, QilFunction> dictionary2 = null;
		foreach (ProtoTemplate allTemplate in compiler.AllTemplates)
		{
			QilList qilList = f.FormalParameterList();
			XslFlags xslFlags = ((!IsDebug) ? allTemplate.Flags : XslFlags.FocusFilter);
			QilList qilList2 = EnterScope(allTemplate);
			if ((xslFlags & XslFlags.Current) != 0)
			{
				qilList.Add(CreateXslParam(CloneName(nameCurrent), XmlQueryTypeFactory.NodeNotRtf));
			}
			if ((xslFlags & XslFlags.Position) != 0)
			{
				qilList.Add(CreateXslParam(CloneName(namePosition), XmlQueryTypeFactory.DoubleX));
			}
			if ((xslFlags & XslFlags.Last) != 0)
			{
				qilList.Add(CreateXslParam(CloneName(nameLast), XmlQueryTypeFactory.DoubleX));
			}
			if (IsDebug && qilList2 != null)
			{
				QilParameter qilParameter = CreateXslParam(CloneName(nameNamespaces), XmlQueryTypeFactory.NamespaceS);
				qilParameter.DefaultValue = GetNsVar(qilList2);
				qilList.Add(qilParameter);
			}
			if (allTemplate is Template template)
			{
				funcFocus.StartFocus(qilList, xslFlags);
				for (int i = 0; i < allTemplate.Content.Count; i++)
				{
					XslNode xslNode = allTemplate.Content[i];
					if (xslNode.NodeType == XslNodeType.Text)
					{
						continue;
					}
					if (xslNode.NodeType != XslNodeType.Param)
					{
						break;
					}
					VarPar varPar = (VarPar)xslNode;
					EnterScope(varPar);
					if (scope.IsLocalVariable(varPar.Name.LocalName, varPar.Name.NamespaceUri))
					{
						ReportError("The variable or parameter '{0}' was duplicated within the same scope.", varPar.Name.QualifiedName);
					}
					QilParameter qilParameter2 = CreateXslParam(varPar.Name, ChooseBestType(varPar));
					if (IsDebug)
					{
						qilParameter2.Annotation = varPar;
					}
					else if ((varPar.DefValueFlags & XslFlags.HasCalls) == 0)
					{
						qilParameter2.DefaultValue = CompileVarParValue(varPar);
					}
					else
					{
						QilList qilList3 = f.FormalParameterList();
						QilList qilList4 = f.ActualParameterList();
						for (int j = 0; j < qilList.Count; j++)
						{
							QilParameter qilParameter3 = f.Parameter(qilList[j].XmlType);
							qilParameter3.DebugName = ((QilParameter)qilList[j]).DebugName;
							qilParameter3.Name = CloneName(((QilParameter)qilList[j]).Name);
							SetLineInfo(qilParameter3, qilList[j].SourceLine);
							qilList3.Add(qilParameter3);
							qilList4.Add(qilList[j]);
						}
						varPar.Flags |= template.Flags & XslFlags.FocusFilter;
						QilFunction qilFunction = f.Function(qilList3, f.Boolean((varPar.DefValueFlags & XslFlags.SideEffects) != 0), ChooseBestType(varPar));
						qilFunction.SourceLine = SourceLineInfo.NoSource;
						qilFunction.DebugName = "<xsl:param name=\"" + varPar.Name.QualifiedName + "\">";
						qilParameter2.DefaultValue = f.Invoke(qilFunction, qilList4);
						if (list == null)
						{
							list = new List<VarPar>();
							dictionary = new Dictionary<VarPar, Template>();
							dictionary2 = new Dictionary<VarPar, QilFunction>();
						}
						list.Add(varPar);
						dictionary.Add(varPar, template);
						dictionary2.Add(varPar, qilFunction);
					}
					SetLineInfo(qilParameter2, varPar.SourceLine);
					ExitScope();
					scope.AddVariable(varPar.Name, qilParameter2);
					qilList.Add(qilParameter2);
				}
				funcFocus.StopFocus();
			}
			ExitScope();
			allTemplate.Function = f.Function(qilList, f.Boolean((allTemplate.Flags & XslFlags.SideEffects) != 0), (allTemplate is AttributeSet) ? XmlQueryTypeFactory.AttributeS : XmlQueryTypeFactory.NodeNotRtfS);
			allTemplate.Function.DebugName = allTemplate.GetDebugName();
			SetLineInfo(allTemplate.Function, allTemplate.SourceLine ?? SourceLineInfo.NoSource);
			functions.Add(allTemplate.Function);
		}
		if (list == null)
		{
			return;
		}
		foreach (VarPar item in list)
		{
			Template node = dictionary[item];
			QilFunction qilFunction2 = dictionary2[item];
			funcFocus.StartFocus(qilFunction2.Arguments, item.Flags);
			EnterScope(node);
			EnterScope(item);
			foreach (QilParameter argument in qilFunction2.Arguments)
			{
				scope.AddVariable(argument.Name, argument);
			}
			qilFunction2.Definition = CompileVarParValue(item);
			SetLineInfo(qilFunction2.Definition, item.SourceLine);
			ExitScope();
			ExitScope();
			funcFocus.StopFocus();
			functions.Add(qilFunction2);
		}
	}

	private QilParameter CreateXslParam(QilName name, XmlQueryType xt)
	{
		QilParameter qilParameter = f.Parameter(xt);
		qilParameter.DebugName = name.ToString();
		qilParameter.Name = name;
		return qilParameter;
	}

	private void CompileProtoTemplate(ProtoTemplate tmpl)
	{
		EnterScope(tmpl);
		funcFocus.StartFocus(tmpl.Function.Arguments, (!IsDebug) ? tmpl.Flags : XslFlags.FocusFilter);
		foreach (QilParameter argument in tmpl.Function.Arguments)
		{
			if (argument.Name.NamespaceUri != "urn:schemas-microsoft-com:xslt-debug")
			{
				if (IsDebug)
				{
					VarPar node = (VarPar)argument.Annotation;
					QilList nsList = EnterScope(node);
					argument.DefaultValue = CompileVarParValue(node);
					ExitScope();
					argument.DefaultValue = SetDebugNs(argument.DefaultValue, nsList);
				}
				scope.AddVariable(argument.Name, argument);
			}
		}
		tmpl.Function.Definition = CompileInstructions(tmpl.Content);
		funcFocus.StopFocus();
		ExitScope();
	}

	private QilList InstructionList()
	{
		return f.BaseFactory.Sequence();
	}

	private QilNode CompileInstructions(IList<XslNode> instructions)
	{
		return CompileInstructions(instructions, 0, InstructionList());
	}

	private QilNode CompileInstructions(IList<XslNode> instructions, int from)
	{
		return CompileInstructions(instructions, from, InstructionList());
	}

	private QilNode CompileInstructions(IList<XslNode> instructions, QilList content)
	{
		return CompileInstructions(instructions, 0, content);
	}

	private QilNode CompileInstructions(IList<XslNode> instructions, int from, QilList content)
	{
		for (int i = from; i < instructions.Count; i++)
		{
			XslNode xslNode = instructions[i];
			XslNodeType nodeType = xslNode.NodeType;
			if (nodeType == XslNodeType.Param)
			{
				continue;
			}
			QilList nsList = EnterScope(xslNode);
			QilNode qilNode = nodeType switch
			{
				XslNodeType.ApplyImports => CompileApplyImports(xslNode), 
				XslNodeType.ApplyTemplates => CompileApplyTemplates((XslNodeEx)xslNode), 
				XslNodeType.Attribute => CompileAttribute((NodeCtor)xslNode), 
				XslNodeType.CallTemplate => CompileCallTemplate((XslNodeEx)xslNode), 
				XslNodeType.Choose => CompileChoose(xslNode), 
				XslNodeType.Comment => CompileComment(xslNode), 
				XslNodeType.Copy => CompileCopy(xslNode), 
				XslNodeType.CopyOf => CompileCopyOf(xslNode), 
				XslNodeType.Element => CompileElement((NodeCtor)xslNode), 
				XslNodeType.Error => CompileError(xslNode), 
				XslNodeType.ForEach => CompileForEach((XslNodeEx)xslNode), 
				XslNodeType.If => CompileIf(xslNode), 
				XslNodeType.List => CompileList(xslNode), 
				XslNodeType.LiteralAttribute => CompileLiteralAttribute(xslNode), 
				XslNodeType.LiteralElement => CompileLiteralElement(xslNode), 
				XslNodeType.Message => CompileMessage(xslNode), 
				XslNodeType.Nop => CompileNop(xslNode), 
				XslNodeType.Number => CompileNumber((Number)xslNode), 
				XslNodeType.PI => CompilePI(xslNode), 
				XslNodeType.Text => CompileText((Text)xslNode), 
				XslNodeType.UseAttributeSet => CompileUseAttributeSet(xslNode), 
				XslNodeType.ValueOf => CompileValueOf(xslNode), 
				XslNodeType.ValueOfDoe => CompileValueOfDoe(xslNode), 
				XslNodeType.Variable => CompileVariable(xslNode), 
				_ => null, 
			};
			ExitScope();
			if (qilNode.NodeType != QilNodeType.Sequence || qilNode.Count != 0)
			{
				if (nodeType != XslNodeType.LiteralAttribute && nodeType != XslNodeType.UseAttributeSet)
				{
					SetLineInfoCheck(qilNode, xslNode.SourceLine);
				}
				qilNode = SetDebugNs(qilNode, nsList);
				if (nodeType == XslNodeType.Variable)
				{
					QilIterator qilIterator = f.Let(qilNode);
					qilIterator.DebugName = xslNode.Name.ToString();
					scope.AddVariable(xslNode.Name, qilIterator);
					qilNode = f.Loop(qilIterator, CompileInstructions(instructions, i + 1));
					i = instructions.Count;
				}
				content.Add(qilNode);
			}
		}
		if (!IsDebug && content.Count == 1)
		{
			return content[0];
		}
		return content;
	}

	private QilNode CompileList(XslNode node)
	{
		return CompileInstructions(node.Content);
	}

	private QilNode CompileNop(XslNode node)
	{
		return f.Nop(f.Sequence());
	}

	private void AddNsDecl(QilList content, string prefix, string nsUri)
	{
		if (!(outputScope.LookupNamespace(prefix) == nsUri))
		{
			outputScope.AddNamespace(prefix, nsUri);
			content.Add(f.NamespaceDecl(f.String(prefix), f.String(nsUri)));
		}
	}

	private QilNode CompileLiteralElement(XslNode node)
	{
		bool flag = true;
		while (true)
		{
			prefixesInUse.Clear();
			QilName name = node.Name;
			string prefix = name.Prefix;
			string nsUri = name.NamespaceUri;
			compiler.ApplyNsAliases(ref prefix, ref nsUri);
			if (flag)
			{
				prefixesInUse.Add(prefix, nsUri);
			}
			else
			{
				prefix = name.Prefix;
			}
			outputScope.PushScope();
			QilList content = InstructionList();
			CompilerScopeManager<QilIterator>.NamespaceEnumerator enumerator = scope.GetEnumerator();
			while (true)
			{
				if (enumerator.MoveNext())
				{
					CompilerScopeManager<QilIterator>.ScopeRecord current = enumerator.Current;
					string prefix2 = current.ncName;
					string nsUri2 = current.nsUri;
					if (!(nsUri2 != "http://www.w3.org/1999/XSL/Transform") || scope.IsExNamespace(nsUri2))
					{
						continue;
					}
					compiler.ApplyNsAliases(ref prefix2, ref nsUri2);
					if (flag)
					{
						if (prefixesInUse.Contains(prefix2))
						{
							if ((string)prefixesInUse[prefix2] != nsUri2)
							{
								break;
							}
						}
						else
						{
							prefixesInUse.Add(prefix2, nsUri2);
						}
					}
					else
					{
						prefix2 = current.ncName;
					}
					AddNsDecl(content, prefix2, nsUri2);
					continue;
				}
				QilNode content2 = CompileInstructions(node.Content, content);
				outputScope.PopScope();
				name.Prefix = prefix;
				name.NamespaceUri = nsUri;
				return f.ElementCtor(name, content2);
			}
			outputScope.PopScope();
			flag = false;
		}
	}

	private QilNode CompileElement(NodeCtor node)
	{
		QilNode qilNode = CompileStringAvt(node.NsAvt);
		QilNode qilNode2 = CompileStringAvt(node.NameAvt);
		QilNode name;
		if (qilNode2.NodeType != QilNodeType.LiteralString || (qilNode != null && qilNode.NodeType != QilNodeType.LiteralString))
		{
			name = ((qilNode == null) ? ResolveQNameDynamic(ignoreDefaultNs: false, qilNode2) : f.StrParseQName(qilNode2, qilNode));
		}
		else
		{
			string qname = (QilLiteral)qilNode2;
			string prefix;
			string localName;
			bool flag = compiler.ParseQName(qname, out prefix, out localName, this);
			string uri = ((qilNode != null) ? ((string)(QilLiteral)qilNode) : (flag ? ResolvePrefix(ignoreDefaultNs: false, prefix) : compiler.CreatePhantomNamespace()));
			name = f.QName(localName, uri, prefix);
		}
		outputScope.PushScope();
		outputScope.InvalidateAllPrefixes();
		QilNode content = CompileInstructions(node.Content);
		outputScope.PopScope();
		return f.ElementCtor(name, content);
	}

	private QilNode CompileLiteralAttribute(XslNode node)
	{
		QilName name = node.Name;
		string prefix = name.Prefix;
		string nsUri = name.NamespaceUri;
		if (prefix.Length != 0)
		{
			compiler.ApplyNsAliases(ref prefix, ref nsUri);
		}
		name.Prefix = prefix;
		name.NamespaceUri = nsUri;
		return f.AttributeCtor(name, CompileTextAvt(node.Select));
	}

	private QilNode CompileAttribute(NodeCtor node)
	{
		QilNode qilNode = CompileStringAvt(node.NsAvt);
		QilNode qilNode2 = CompileStringAvt(node.NameAvt);
		bool flag = false;
		QilNode name;
		if (qilNode2.NodeType != QilNodeType.LiteralString || (qilNode != null && qilNode.NodeType != QilNodeType.LiteralString))
		{
			name = ((qilNode == null) ? ResolveQNameDynamic(ignoreDefaultNs: true, qilNode2) : f.StrParseQName(qilNode2, qilNode));
		}
		else
		{
			string text = (QilLiteral)qilNode2;
			string prefix;
			string localName;
			bool flag2 = compiler.ParseQName(text, out prefix, out localName, this);
			string text2;
			if (qilNode == null)
			{
				text2 = (flag2 ? ResolvePrefix(ignoreDefaultNs: true, prefix) : compiler.CreatePhantomNamespace());
			}
			else
			{
				text2 = (QilLiteral)qilNode;
				flag = true;
			}
			if (text == "xmlns" || (localName == "xmlns" && text2.Length == 0))
			{
				ReportError("An attribute with a local name 'xmlns' and a null namespace URI cannot be created.", "name", text);
			}
			name = f.QName(localName, text2, prefix);
		}
		if (flag)
		{
			outputScope.InvalidateNonDefaultPrefixes();
		}
		return f.AttributeCtor(name, CompileInstructions(node.Content));
	}

	private QilNode ExtractText(string source, ref int pos)
	{
		int num = pos;
		unescapedText.Length = 0;
		int i;
		for (i = pos; i < source.Length; i++)
		{
			char c = source[i];
			if (c != '{' && c != '}')
			{
				continue;
			}
			if (i + 1 < source.Length && source[i + 1] == c)
			{
				i++;
				unescapedText.Append(source, num, i - num);
				num = i + 1;
				continue;
			}
			if (c == '{')
			{
				break;
			}
			pos = source.Length;
			if (xslVersion != XslVersion.ForwardsCompatible)
			{
				ReportError("The right curly brace in an attribute value template '{0}' outside an expression must be doubled.", source);
				return null;
			}
			return f.Error(lastScope.SourceLine, "The right curly brace in an attribute value template '{0}' outside an expression must be doubled.", source);
		}
		pos = i;
		if (unescapedText.Length == 0)
		{
			if (i <= num)
			{
				return null;
			}
			return f.String(source.Substring(num, i - num));
		}
		unescapedText.Append(source, num, i - num);
		return f.String(unescapedText.ToString());
	}

	private QilNode CompileAvt(string source)
	{
		QilList qilList = f.BaseFactory.Sequence();
		int pos = 0;
		while (pos < source.Length)
		{
			QilNode qilNode = ExtractText(source, ref pos);
			if (qilNode != null)
			{
				qilList.Add(qilNode);
			}
			if (pos < source.Length)
			{
				pos++;
				QilNode n = CompileXPathExpressionWithinAvt(source, ref pos);
				qilList.Add(f.ConvertToString(n));
			}
		}
		if (qilList.Count == 1)
		{
			return qilList[0];
		}
		return qilList;
	}

	private QilNode CompileStringAvt(string avt)
	{
		if (avt == null)
		{
			return null;
		}
		if (avt.IndexOfAny(curlyBraces) == -1)
		{
			return f.String(avt);
		}
		return f.StrConcat(CompileAvt(avt));
	}

	private QilNode CompileTextAvt(string avt)
	{
		if (avt.IndexOfAny(curlyBraces) == -1)
		{
			return f.TextCtor(f.String(avt));
		}
		QilNode qilNode = CompileAvt(avt);
		if (qilNode.NodeType == QilNodeType.Sequence)
		{
			QilList qilList = InstructionList();
			{
				foreach (QilNode item in qilNode)
				{
					qilList.Add(f.TextCtor(item));
				}
				return qilList;
			}
		}
		return f.TextCtor(qilNode);
	}

	private QilNode CompileText(Text node)
	{
		if (node.Hints == SerializationHints.None)
		{
			return f.TextCtor(f.String(node.Select));
		}
		return f.RawTextCtor(f.String(node.Select));
	}

	private QilNode CompilePI(XslNode node)
	{
		QilNode qilNode = CompileStringAvt(node.Select);
		if (qilNode.NodeType == QilNodeType.LiteralString)
		{
			string name = (QilLiteral)qilNode;
			compiler.ValidatePiName(name, this);
		}
		return f.PICtor(qilNode, CompileInstructions(node.Content));
	}

	private QilNode CompileComment(XslNode node)
	{
		return f.CommentCtor(CompileInstructions(node.Content));
	}

	private QilNode CompileError(XslNode node)
	{
		return f.Error(f.String(node.Select));
	}

	private QilNode WrapLoopBody(ISourceLineInfo before, QilNode expr, ISourceLineInfo after)
	{
		if (IsDebug)
		{
			return f.Sequence(SetLineInfo(InvokeOnCurrentNodeChanged(), before), expr, SetLineInfo(f.Nop(f.Sequence()), after));
		}
		return expr;
	}

	private QilNode CompileForEach(XslNodeEx node)
	{
		IList<XslNode> content = node.Content;
		LoopFocus parentLoop = curLoop;
		QilIterator focus = f.For(CompileNodeSetExpression(node.Select));
		curLoop.SetFocus(focus);
		int varScope = varHelper.StartVariables();
		curLoop.Sort(CompileSorts(content, ref parentLoop));
		QilNode expr = CompileInstructions(content);
		expr = WrapLoopBody(node.ElemNameLi, expr, node.EndTagLi);
		expr = AddCurrentPositionLast(expr);
		expr = curLoop.ConstructLoop(expr);
		expr = varHelper.FinishVariables(expr, varScope);
		curLoop = parentLoop;
		return expr;
	}

	private QilNode CompileApplyTemplates(XslNodeEx node)
	{
		IList<XslNode> content = node.Content;
		int varScope = varHelper.StartVariables();
		QilIterator qilIterator = f.Let(CompileNodeSetExpression(node.Select));
		varHelper.AddVariable(qilIterator);
		for (int i = 0; i < content.Count; i++)
		{
			if (content[i] is VarPar varPar)
			{
				CompileWithParam(varPar);
				QilNode value = varPar.Value;
				if (IsDebug || (!(value is QilIterator) && !(value is QilLiteral)))
				{
					QilIterator qilIterator2 = f.Let(value);
					qilIterator2.DebugName = f.QName("with-param " + varPar.Name.QualifiedName, "urn:schemas-microsoft-com:xslt-debug").ToString();
					varHelper.AddVariable(qilIterator2);
					varPar.Value = qilIterator2;
				}
			}
		}
		LoopFocus parentLoop = curLoop;
		QilIterator focus = f.For(qilIterator);
		curLoop.SetFocus(focus);
		curLoop.Sort(CompileSorts(content, ref parentLoop));
		QilNode expr = GenerateApply(compiler.Root, node);
		expr = WrapLoopBody(node.ElemNameLi, expr, node.EndTagLi);
		expr = AddCurrentPositionLast(expr);
		expr = curLoop.ConstructLoop(expr);
		curLoop = parentLoop;
		return varHelper.FinishVariables(expr, varScope);
	}

	private QilNode CompileApplyImports(XslNode node)
	{
		return GenerateApply((StylesheetLevel)node.Arg, node);
	}

	private QilNode CompileCallTemplate(XslNodeEx node)
	{
		int varScope = varHelper.StartVariables();
		IList<XslNode> content = node.Content;
		foreach (VarPar item in content)
		{
			CompileWithParam(item);
			if (IsDebug)
			{
				QilNode value = item.Value;
				QilIterator qilIterator = f.Let(value);
				qilIterator.DebugName = f.QName("with-param " + item.Name.QualifiedName, "urn:schemas-microsoft-com:xslt-debug").ToString();
				varHelper.AddVariable(qilIterator);
				item.Value = qilIterator;
			}
		}
		QilNode qilNode;
		if (compiler.NamedTemplates.TryGetValue(node.Name, out var value2))
		{
			qilNode = invkGen.GenerateInvoke(value2.Function, AddRemoveImplicitArgs(node.Content, value2.Flags));
		}
		else
		{
			if (!compiler.IsPhantomName(node.Name))
			{
				compiler.ReportError(node.SourceLine, "The named template '{0}' does not exist.", node.Name.QualifiedName);
			}
			qilNode = f.Sequence();
		}
		if (content.Count > 0)
		{
			qilNode = SetLineInfo(qilNode, node.ElemNameLi);
		}
		qilNode = varHelper.FinishVariables(qilNode, varScope);
		if (IsDebug)
		{
			return f.Nop(qilNode);
		}
		return qilNode;
	}

	private QilNode CompileUseAttributeSet(XslNode node)
	{
		outputScope.InvalidateAllPrefixes();
		if (compiler.AttributeSets.TryGetValue(node.Name, out var value))
		{
			return invkGen.GenerateInvoke(value.Function, AddRemoveImplicitArgs(node.Content, value.Flags));
		}
		if (!compiler.IsPhantomName(node.Name))
		{
			compiler.ReportError(node.SourceLine, "A reference to attribute set '{0}' cannot be resolved. An 'xsl:attribute-set' of this name must be declared at the top level of the stylesheet.", node.Name.QualifiedName);
		}
		return f.Sequence();
	}

	private QilNode CompileCopy(XslNode copy)
	{
		QilNode currentNode = GetCurrentNode();
		if ((currentNode.XmlType.NodeKinds & (XmlNodeKindFlags.Attribute | XmlNodeKindFlags.Namespace)) != 0)
		{
			outputScope.InvalidateAllPrefixes();
		}
		if (currentNode.XmlType.NodeKinds == XmlNodeKindFlags.Element)
		{
			QilList qilList = InstructionList();
			qilList.Add(f.XPathNamespace(currentNode));
			outputScope.PushScope();
			outputScope.InvalidateAllPrefixes();
			QilNode content = CompileInstructions(copy.Content, qilList);
			outputScope.PopScope();
			return f.ElementCtor(f.NameOf(currentNode), content);
		}
		if (currentNode.XmlType.NodeKinds == XmlNodeKindFlags.Document)
		{
			return CompileInstructions(copy.Content);
		}
		if ((currentNode.XmlType.NodeKinds & (XmlNodeKindFlags.Document | XmlNodeKindFlags.Element)) == 0)
		{
			return currentNode;
		}
		return f.XsltCopy(currentNode, CompileInstructions(copy.Content));
	}

	private QilNode CompileCopyOf(XslNode node)
	{
		QilNode qilNode = CompileXPathExpression(node.Select);
		if (qilNode.XmlType.IsNode)
		{
			if ((qilNode.XmlType.NodeKinds & (XmlNodeKindFlags.Attribute | XmlNodeKindFlags.Namespace)) != 0)
			{
				outputScope.InvalidateAllPrefixes();
			}
			if (qilNode.XmlType.IsNotRtf && (qilNode.XmlType.NodeKinds & XmlNodeKindFlags.Document) == 0)
			{
				return qilNode;
			}
			if (qilNode.XmlType.IsSingleton)
			{
				return f.XsltCopyOf(qilNode);
			}
			QilIterator expr;
			return f.Loop(expr = f.For(qilNode), f.XsltCopyOf(expr));
		}
		if (qilNode.XmlType.IsAtomicValue)
		{
			return f.TextCtor(f.ConvertToString(qilNode));
		}
		outputScope.InvalidateAllPrefixes();
		QilIterator expr2;
		return f.Loop(expr2 = f.For(qilNode), f.Conditional(f.IsType(expr2, XmlQueryTypeFactory.Node), f.XsltCopyOf(f.TypeAssert(expr2, XmlQueryTypeFactory.Node)), f.TextCtor(f.XsltConvert(expr2, XmlQueryTypeFactory.StringX))));
	}

	private QilNode CompileValueOf(XslNode valueOf)
	{
		return f.TextCtor(f.ConvertToString(CompileXPathExpression(valueOf.Select)));
	}

	private QilNode CompileValueOfDoe(XslNode valueOf)
	{
		return f.RawTextCtor(f.ConvertToString(CompileXPathExpression(valueOf.Select)));
	}

	private QilNode CompileWhen(XslNode whenNode, QilNode otherwise)
	{
		return f.Conditional(f.ConvertToBoolean(CompileXPathExpression(whenNode.Select)), CompileInstructions(whenNode.Content), otherwise);
	}

	private QilNode CompileIf(XslNode ifNode)
	{
		return CompileWhen(ifNode, InstructionList());
	}

	private QilNode CompileChoose(XslNode node)
	{
		IList<XslNode> content = node.Content;
		QilNode qilNode = null;
		int num = content.Count - 1;
		while (0 <= num)
		{
			XslNode xslNode = content[num];
			QilList nsList = EnterScope(xslNode);
			qilNode = ((xslNode.NodeType != XslNodeType.Otherwise) ? CompileWhen(xslNode, qilNode ?? InstructionList()) : CompileInstructions(xslNode.Content));
			ExitScope();
			SetLineInfoCheck(qilNode, xslNode.SourceLine);
			qilNode = SetDebugNs(qilNode, nsList);
			num--;
		}
		if (qilNode == null)
		{
			return f.Sequence();
		}
		if (!IsDebug)
		{
			return qilNode;
		}
		return f.Sequence(qilNode);
	}

	private QilNode CompileMessage(XslNode node)
	{
		string uri = lastScope.SourceLine.Uri;
		QilNode n = f.RtfCtor(CompileInstructions(node.Content), f.String(uri));
		n = f.InvokeOuterXml(n);
		if (!(bool)node.Arg)
		{
			return f.Warning(n);
		}
		QilIterator text;
		return f.Loop(text = f.Let(n), f.Sequence(f.Warning(text), f.Error(text)));
	}

	private QilNode CompileVariable(XslNode node)
	{
		if (scope.IsLocalVariable(node.Name.LocalName, node.Name.NamespaceUri))
		{
			ReportError("The variable or parameter '{0}' was duplicated within the same scope.", node.Name.QualifiedName);
		}
		return CompileVarParValue(node);
	}

	private QilNode CompileVarParValue(XslNode node)
	{
		string uri = lastScope.SourceLine.Uri;
		IList<XslNode> content = node.Content;
		string select = node.Select;
		QilNode qilNode;
		if (select != null)
		{
			QilList qilList = InstructionList();
			qilList.Add(CompileXPathExpression(select));
			qilNode = CompileInstructions(content, qilList);
		}
		else if (content.Count != 0)
		{
			outputScope.PushScope();
			outputScope.InvalidateAllPrefixes();
			qilNode = f.RtfCtor(CompileInstructions(content), f.String(uri));
			outputScope.PopScope();
		}
		else
		{
			qilNode = f.String(string.Empty);
		}
		if (IsDebug)
		{
			qilNode = f.TypeAssert(qilNode, XmlQueryTypeFactory.ItemS);
		}
		return qilNode;
	}

	private void CompileWithParam(VarPar withParam)
	{
		QilList nsList = EnterScope(withParam);
		QilNode n = CompileVarParValue(withParam);
		ExitScope();
		SetLineInfo(n, withParam.SourceLine);
		n = SetDebugNs(n, nsList);
		withParam.Value = n;
	}

	private QilNode CompileSorts(IList<XslNode> content, ref LoopFocus parentLoop)
	{
		QilList qilList = f.BaseFactory.SortKeyList();
		int num = 0;
		while (num < content.Count)
		{
			if (content[num] is Sort sort)
			{
				CompileSort(sort, qilList, ref parentLoop);
				content.RemoveAt(num);
			}
			else
			{
				num++;
			}
		}
		if (qilList.Count == 0)
		{
			return null;
		}
		return qilList;
	}

	private QilNode CompileLangAttribute(string attValue, bool fwdCompat)
	{
		QilNode qilNode = CompileStringAvt(attValue);
		if (qilNode != null)
		{
			if (qilNode.NodeType == QilNodeType.LiteralString)
			{
				if (XsltLibrary.LangToLcidInternal((QilLiteral)qilNode, fwdCompat, this) == 127)
				{
					qilNode = null;
				}
			}
			else
			{
				QilIterator qilIterator;
				qilNode = f.Loop(qilIterator = f.Let(qilNode), f.Conditional(f.Eq(f.InvokeLangToLcid(qilIterator, fwdCompat), f.Int32(127)), f.String(string.Empty), qilIterator));
			}
		}
		return qilNode;
	}

	private QilNode CompileLangAttributeToLcid(string attValue, bool fwdCompat)
	{
		return CompileLangToLcid(CompileStringAvt(attValue), fwdCompat);
	}

	private QilNode CompileLangToLcid(QilNode lang, bool fwdCompat)
	{
		if (lang == null)
		{
			return f.Double(127.0);
		}
		if (lang.NodeType == QilNodeType.LiteralString)
		{
			return f.Double(XsltLibrary.LangToLcidInternal((QilLiteral)lang, fwdCompat, this));
		}
		return f.XsltConvert(f.InvokeLangToLcid(lang, fwdCompat), XmlQueryTypeFactory.DoubleX);
	}

	private void CompileDataTypeAttribute(string attValue, bool fwdCompat, ref QilNode select, out QilNode select2)
	{
		QilNode qilNode = CompileStringAvt(attValue);
		if (qilNode != null)
		{
			if (qilNode.NodeType != QilNodeType.LiteralString)
			{
				QilIterator qilIterator;
				QilIterator qilIterator2;
				qilNode = f.Loop(qilIterator = f.Let(qilNode), f.Conditional(f.Eq(qilIterator, f.String("number")), f.False(), f.Conditional(f.Eq(qilIterator, f.String("text")), f.True(), fwdCompat ? f.True() : f.Loop(qilIterator2 = f.Let(ResolveQNameDynamic(ignoreDefaultNs: true, qilIterator)), f.Error(lastScope.SourceLine, "The value of the '{0}' attribute must be '{1}' or '{2}'.", "data-type", "text", "number")))));
				QilIterator qilIterator3 = f.Let(qilNode);
				varHelper.AddVariable(qilIterator3);
				select2 = select.DeepClone(f.BaseFactory);
				select = f.Conditional(qilIterator3, f.ConvertToString(select), f.String(string.Empty));
				select2 = f.Conditional(qilIterator3, f.Double(0.0), f.ConvertToNumber(select2));
				return;
			}
			string text = (QilLiteral)qilNode;
			if (text == "number")
			{
				select = f.ConvertToNumber(select);
				select2 = null;
				return;
			}
			if (!(text == "text") && !fwdCompat)
			{
				_ = (compiler.ParseQName(text, out var prefix, out var _, this) ? ResolvePrefix(ignoreDefaultNs: true, prefix) : compiler.CreatePhantomNamespace()).Length;
				ReportError("The value of the '{0}' attribute must be '{1}' or '{2}'.", "data-type", "text", "number");
			}
		}
		select = f.ConvertToString(select);
		select2 = null;
	}

	private QilNode CompileOrderAttribute(string attName, string attValue, string value0, string value1, bool fwdCompat)
	{
		QilNode qilNode = CompileStringAvt(attValue);
		if (qilNode != null)
		{
			if (qilNode.NodeType == QilNodeType.LiteralString)
			{
				string text = (QilLiteral)qilNode;
				if (text == value1)
				{
					qilNode = f.String("1");
				}
				else
				{
					if (text != value0 && !fwdCompat)
					{
						ReportError("The value of the '{0}' attribute must be '{1}' or '{2}'.", attName, value0, value1);
					}
					qilNode = f.String("0");
				}
			}
			else
			{
				QilIterator left;
				qilNode = f.Loop(left = f.Let(qilNode), f.Conditional(f.Eq(left, f.String(value1)), f.String("1"), fwdCompat ? f.String("0") : f.Conditional(f.Eq(left, f.String(value0)), f.String("0"), f.Error(lastScope.SourceLine, "The value of the '{0}' attribute must be '{1}' or '{2}'.", attName, value0, value1))));
			}
		}
		return qilNode;
	}

	private void CompileSort(Sort sort, QilList keyList, ref LoopFocus parentLoop)
	{
		EnterScope(sort);
		bool forwardsCompatible = sort.ForwardsCompatible;
		QilNode select = CompileXPathExpression(sort.Select);
		QilNode value;
		QilNode select2;
		QilNode qilNode;
		QilNode qilNode2;
		if (sort.Lang != null || sort.DataType != null || sort.Order != null || sort.CaseOrder != null)
		{
			LoopFocus loopFocus = curLoop;
			curLoop = parentLoop;
			value = CompileLangAttribute(sort.Lang, forwardsCompatible);
			CompileDataTypeAttribute(sort.DataType, forwardsCompatible, ref select, out select2);
			qilNode = CompileOrderAttribute("order", sort.Order, "ascending", "descending", forwardsCompatible);
			qilNode2 = CompileOrderAttribute("case-order", sort.CaseOrder, "lower-first", "upper-first", forwardsCompatible);
			curLoop = loopFocus;
		}
		else
		{
			select = f.ConvertToString(select);
			select2 = (value = (qilNode = (qilNode2 = null)));
		}
		strConcat.Reset();
		strConcat.Append("http://collations.microsoft.com");
		strConcat.Append('/');
		strConcat.Append(value);
		char value2 = '?';
		if (qilNode != null)
		{
			strConcat.Append(value2);
			strConcat.Append("descendingOrder=");
			strConcat.Append(qilNode);
			value2 = '&';
		}
		if (qilNode2 != null)
		{
			strConcat.Append(value2);
			strConcat.Append("upperFirst=");
			strConcat.Append(qilNode2);
			value2 = '&';
		}
		QilNode qilNode3 = strConcat.ToQil();
		QilSortKey node = f.SortKey(select, qilNode3);
		keyList.Add(node);
		if (select2 != null)
		{
			node = f.SortKey(select2, qilNode3.DeepClone(f.BaseFactory));
			keyList.Add(node);
		}
		ExitScope();
	}

	private QilNode MatchPattern(QilNode pattern, QilIterator testNode)
	{
		if (pattern.NodeType == QilNodeType.Error)
		{
			return pattern;
		}
		QilList qilList;
		if (pattern.NodeType == QilNodeType.Sequence)
		{
			qilList = (QilList)pattern;
		}
		else
		{
			qilList = f.BaseFactory.Sequence();
			qilList.Add(pattern);
		}
		QilNode qilNode = f.False();
		int num = qilList.Count - 1;
		while (0 <= num)
		{
			QilLoop qilLoop = (QilLoop)qilList[num];
			qilNode = f.Or(refReplacer.Replace(qilLoop.Body, qilLoop.Variable, testNode), qilNode);
			num--;
		}
		return qilNode;
	}

	private QilNode MatchCountPattern(QilNode countPattern, QilIterator testNode)
	{
		if (countPattern != null)
		{
			return MatchPattern(countPattern, testNode);
		}
		QilNode currentNode = GetCurrentNode();
		XmlNodeKindFlags nodeKinds = currentNode.XmlType.NodeKinds;
		if ((nodeKinds & (nodeKinds - 1)) != 0)
		{
			return f.InvokeIsSameNodeSort(testNode, currentNode);
		}
		QilNode left;
		switch (nodeKinds)
		{
		case XmlNodeKindFlags.Document:
			return f.IsType(testNode, XmlQueryTypeFactory.Document);
		case XmlNodeKindFlags.Element:
			left = f.IsType(testNode, XmlQueryTypeFactory.Element);
			break;
		case XmlNodeKindFlags.Attribute:
			left = f.IsType(testNode, XmlQueryTypeFactory.Attribute);
			break;
		case XmlNodeKindFlags.Text:
			return f.IsType(testNode, XmlQueryTypeFactory.Text);
		case XmlNodeKindFlags.Comment:
			return f.IsType(testNode, XmlQueryTypeFactory.Comment);
		case XmlNodeKindFlags.PI:
			return f.And(f.IsType(testNode, XmlQueryTypeFactory.PI), f.Eq(f.LocalNameOf(testNode), f.LocalNameOf(currentNode)));
		case XmlNodeKindFlags.Namespace:
			return f.And(f.IsType(testNode, XmlQueryTypeFactory.Namespace), f.Eq(f.LocalNameOf(testNode), f.LocalNameOf(currentNode)));
		default:
			return f.False();
		}
		return f.And(left, f.And(f.Eq(f.LocalNameOf(testNode), f.LocalNameOf(currentNode)), f.Eq(f.NamespaceUriOf(testNode), f.NamespaceUriOf(GetCurrentNode()))));
	}

	private QilNode PlaceMarker(QilNode countPattern, QilNode fromPattern, bool multiple)
	{
		QilNode countPattern2 = countPattern?.DeepClone(f.BaseFactory);
		QilIterator testNode;
		QilNode qilNode = f.Filter(testNode = f.For(f.AncestorOrSelf(GetCurrentNode())), MatchCountPattern(countPattern, testNode));
		QilNode qilNode2 = ((!multiple) ? f.Filter(testNode = f.For(qilNode), f.Eq(f.PositionOf(testNode), f.Int32(1))) : f.DocOrderDistinct(qilNode));
		QilNode binding;
		QilIterator right;
		if (fromPattern == null)
		{
			binding = qilNode2;
		}
		else
		{
			QilNode binding2 = f.Filter(testNode = f.For(f.AncestorOrSelf(GetCurrentNode())), MatchPattern(fromPattern, testNode));
			QilNode binding3 = f.Filter(testNode = f.For(binding2), f.Eq(f.PositionOf(testNode), f.Int32(1)));
			binding = f.Loop(testNode = f.For(binding3), f.Filter(right = f.For(qilNode2), f.Before(testNode, right)));
		}
		return f.Loop(right = f.For(binding), f.Add(f.Int32(1), f.Length(f.Filter(testNode = f.For(f.PrecedingSibling(right)), MatchCountPattern(countPattern2, testNode)))));
	}

	private QilNode PlaceMarkerAny(QilNode countPattern, QilNode fromPattern)
	{
		QilNode child;
		QilIterator testNode2;
		if (fromPattern == null)
		{
			QilNode binding = f.NodeRange(f.Root(GetCurrentNode()), GetCurrentNode());
			QilIterator testNode;
			child = f.Filter(testNode = f.For(binding), MatchCountPattern(countPattern, testNode));
		}
		else
		{
			QilIterator testNode;
			QilNode binding2 = f.Filter(testNode = f.For(f.Preceding(GetCurrentNode())), MatchPattern(fromPattern, testNode));
			QilNode binding3 = f.Filter(testNode = f.For(binding2), f.Eq(f.PositionOf(testNode), f.Int32(1)));
			QilIterator right;
			child = f.Loop(testNode = f.For(binding3), f.Filter(right = f.For(f.Filter(testNode2 = f.For(f.NodeRange(testNode, GetCurrentNode())), MatchCountPattern(countPattern, testNode2))), f.Not(f.Is(testNode, right))));
		}
		return f.Loop(testNode2 = f.Let(f.Length(child)), f.Conditional(f.Eq(testNode2, f.Int32(0)), f.Sequence(), testNode2));
	}

	private QilNode CompileLetterValueAttribute(string attValue, bool fwdCompat)
	{
		QilNode qilNode = CompileStringAvt(attValue);
		if (qilNode != null)
		{
			if (qilNode.NodeType == QilNodeType.LiteralString)
			{
				string text = (QilLiteral)qilNode;
				if (text != "alphabetic" && text != "traditional")
				{
					if (fwdCompat)
					{
						return f.String("default");
					}
					ReportError("The value of the '{0}' attribute must be '{1}' or '{2}'.", "letter-value", "alphabetic", "traditional");
				}
				return qilNode;
			}
			QilIterator qilIterator = f.Let(qilNode);
			return f.Loop(qilIterator, f.Conditional(f.Or(f.Eq(qilIterator, f.String("alphabetic")), f.Eq(qilIterator, f.String("traditional"))), qilIterator, fwdCompat ? f.String("default") : f.Error(lastScope.SourceLine, "The value of the '{0}' attribute must be '{1}' or '{2}'.", "letter-value", "alphabetic", "traditional")));
		}
		return f.String("default");
	}

	private QilNode CompileGroupingSeparatorAttribute(string attValue, bool fwdCompat)
	{
		QilNode qilNode = CompileStringAvt(attValue);
		if (qilNode == null)
		{
			qilNode = f.String(string.Empty);
		}
		else if (qilNode.NodeType == QilNodeType.LiteralString)
		{
			if (((string)(QilLiteral)qilNode).Length != 1)
			{
				if (!fwdCompat)
				{
					ReportError("The value of the '{0}' attribute must be a single character.", "grouping-separator");
				}
				qilNode = f.String(string.Empty);
			}
		}
		else
		{
			QilIterator qilIterator = f.Let(qilNode);
			qilNode = f.Loop(qilIterator, f.Conditional(f.Eq(f.StrLength(qilIterator), f.Int32(1)), qilIterator, fwdCompat ? f.String(string.Empty) : f.Error(lastScope.SourceLine, "The value of the '{0}' attribute must be a single character.", "grouping-separator")));
		}
		return qilNode;
	}

	private QilNode CompileGroupingSizeAttribute(string attValue, bool fwdCompat)
	{
		QilNode qilNode = CompileStringAvt(attValue);
		if (qilNode == null)
		{
			return f.Double(0.0);
		}
		if (qilNode.NodeType == QilNodeType.LiteralString)
		{
			double num = XsltFunctions.Round(XPathConvert.StringToDouble((QilLiteral)qilNode));
			if (0.0 <= num && num <= 2147483647.0)
			{
				return f.Double(num);
			}
			return f.Double(0.0);
		}
		QilIterator qilIterator = f.Let(f.ConvertToNumber(qilNode));
		return f.Loop(qilIterator, f.Conditional(f.And(f.Lt(f.Double(0.0), qilIterator), f.Lt(qilIterator, f.Double(2147483647.0))), qilIterator, f.Double(0.0)));
	}

	private QilNode CompileNumber(Number num)
	{
		QilNode value;
		if (num.Value != null)
		{
			value = f.ConvertToNumber(CompileXPathExpression(num.Value));
		}
		else
		{
			QilNode countPattern = ((num.Count != null) ? CompileNumberPattern(num.Count) : null);
			QilNode fromPattern = ((num.From != null) ? CompileNumberPattern(num.From) : null);
			value = num.Level switch
			{
				NumberLevel.Single => PlaceMarker(countPattern, fromPattern, multiple: false), 
				NumberLevel.Multiple => PlaceMarker(countPattern, fromPattern, multiple: true), 
				_ => PlaceMarkerAny(countPattern, fromPattern), 
			};
		}
		bool forwardsCompatible = num.ForwardsCompatible;
		return f.TextCtor(f.InvokeNumberFormat(value, CompileStringAvt(num.Format), CompileLangAttributeToLcid(num.Lang, forwardsCompatible), CompileLetterValueAttribute(num.LetterValue, forwardsCompatible), CompileGroupingSeparatorAttribute(num.GroupingSeparator, forwardsCompatible), CompileGroupingSizeAttribute(num.GroupingSize, forwardsCompatible)));
	}

	private void CompileAndSortMatches(Stylesheet sheet)
	{
		foreach (Template template in sheet.Templates)
		{
			if (template.Match == null)
			{
				continue;
			}
			EnterScope(template);
			QilNode qilNode = CompileMatchPattern(template.Match);
			if (qilNode.NodeType == QilNodeType.Sequence)
			{
				QilList qilList = (QilList)qilNode;
				for (int i = 0; i < qilList.Count; i++)
				{
					sheet.AddTemplateMatch(template, (QilLoop)qilList[i]);
				}
			}
			else
			{
				sheet.AddTemplateMatch(template, (QilLoop)qilNode);
			}
			ExitScope();
		}
		sheet.SortTemplateMatches();
		Stylesheet[] imports = sheet.Imports;
		foreach (Stylesheet sheet2 in imports)
		{
			CompileAndSortMatches(sheet2);
		}
	}

	private void CompileKeys()
	{
		for (int i = 0; i < compiler.Keys.Count; i++)
		{
			foreach (Key item in compiler.Keys[i])
			{
				EnterScope(item);
				QilParameter qilParameter = f.Parameter(XmlQueryTypeFactory.NodeNotRtf);
				singlFocus.SetFocus(qilParameter);
				QilIterator qilIterator = f.For(f.OptimizeBarrier(CompileKeyMatch(item.Match)));
				singlFocus.SetFocus(qilIterator);
				QilIterator qilIterator2 = f.For(CompileKeyUse(item));
				qilIterator2 = f.For(f.OptimizeBarrier(f.Loop(qilIterator2, f.ConvertToString(qilIterator2))));
				QilParameter qilParameter2 = f.Parameter(XmlQueryTypeFactory.StringX);
				QilFunction qilFunction = f.Function(f.FormalParameterList(qilParameter, qilParameter2), f.Filter(qilIterator, f.Not(f.IsEmpty(f.Filter(qilIterator2, f.Eq(qilIterator2, qilParameter2))))), f.False());
				qilFunction.DebugName = item.GetDebugName();
				SetLineInfo(qilFunction, item.SourceLine);
				item.Function = qilFunction;
				functions.Add(qilFunction);
				ExitScope();
			}
		}
		singlFocus.SetFocus(null);
	}

	private void CreateGlobalVarPars()
	{
		foreach (VarPar externalPar in compiler.ExternalPars)
		{
			CreateGlobalVarPar(externalPar);
		}
		foreach (VarPar globalVar in compiler.GlobalVars)
		{
			CreateGlobalVarPar(globalVar);
		}
	}

	private void CreateGlobalVarPar(VarPar varPar)
	{
		XmlQueryType t = ChooseBestType(varPar);
		QilIterator qilIterator = ((varPar.NodeType != XslNodeType.Variable) ? f.Parameter(null, varPar.Name, t) : f.Let(f.Unknown(t)));
		qilIterator.DebugName = varPar.Name.ToString();
		varPar.Value = qilIterator;
		SetLineInfo(qilIterator, varPar.SourceLine);
		scope.AddVariable(varPar.Name, qilIterator);
	}

	private void CompileGlobalVariables()
	{
		singlFocus.SetFocus(SingletonFocusType.InitialDocumentNode);
		foreach (VarPar externalPar in compiler.ExternalPars)
		{
			extPars.Add(CompileGlobalVarPar(externalPar));
		}
		foreach (VarPar globalVar in compiler.GlobalVars)
		{
			gloVars.Add(CompileGlobalVarPar(globalVar));
		}
		singlFocus.SetFocus(null);
	}

	private QilIterator CompileGlobalVarPar(VarPar varPar)
	{
		QilIterator qilIterator = (QilIterator)varPar.Value;
		QilList nsList = EnterScope(varPar);
		QilNode qilNode = CompileVarParValue(varPar);
		SetLineInfo(qilNode, qilIterator.SourceLine);
		qilNode = AddCurrentPositionLast(qilNode);
		qilNode = SetDebugNs(qilNode, nsList);
		qilIterator.SourceLine = SourceLineInfo.NoSource;
		qilIterator.Binding = qilNode;
		ExitScope();
		return qilIterator;
	}

	private void ReportErrorInXPath(XslLoadException e)
	{
		string text = ((e is XPathCompileException ex) ? ex.FormatDetailedMessage() : e.Message);
		compiler.ReportError(lastScope.SourceLine, "{0}", text);
	}

	private QilNode PhantomXPathExpression()
	{
		return f.TypeAssert(f.Sequence(), XmlQueryTypeFactory.ItemS);
	}

	private QilNode PhantomKeyMatch()
	{
		return f.TypeAssert(f.Sequence(), XmlQueryTypeFactory.NodeNotRtfS);
	}

	private QilNode CompileXPathExpression(string expr)
	{
		SetEnvironmentFlags(allowVariables: true, allowCurrent: true, allowKey: true);
		QilNode qilNode;
		if (expr == null)
		{
			qilNode = PhantomXPathExpression();
		}
		else
		{
			try
			{
				XPathScanner scanner = new XPathScanner(expr);
				qilNode = xpathParser.Parse(scanner, xpathBuilder, LexKind.Eof);
			}
			catch (XslLoadException ex)
			{
				if (xslVersion != XslVersion.ForwardsCompatible)
				{
					ReportErrorInXPath(ex);
				}
				qilNode = f.Error(f.String(ex.Message));
			}
		}
		if (qilNode is QilIterator)
		{
			qilNode = f.Nop(qilNode);
		}
		return qilNode;
	}

	private QilNode CompileNodeSetExpression(string expr)
	{
		QilNode qilNode = f.TryEnsureNodeSet(CompileXPathExpression(expr));
		if (qilNode == null)
		{
			XPathCompileException ex = new XPathCompileException(expr, 0, expr.Length, "Expression must evaluate to a node-set.", (string[])null);
			if (xslVersion != XslVersion.ForwardsCompatible)
			{
				ReportErrorInXPath(ex);
			}
			qilNode = f.Error(f.String(ex.Message));
		}
		return qilNode;
	}

	private QilNode CompileXPathExpressionWithinAvt(string expr, ref int pos)
	{
		SetEnvironmentFlags(allowVariables: true, allowCurrent: true, allowKey: true);
		QilNode qilNode;
		try
		{
			XPathScanner xPathScanner = new XPathScanner(expr, pos);
			qilNode = xpathParser.Parse(xPathScanner, xpathBuilder, LexKind.RBrace);
			pos = xPathScanner.LexStart + 1;
		}
		catch (XslLoadException ex)
		{
			if (xslVersion != XslVersion.ForwardsCompatible)
			{
				ReportErrorInXPath(ex);
			}
			qilNode = f.Error(f.String(ex.Message));
			pos = expr.Length;
		}
		if (qilNode is QilIterator)
		{
			qilNode = f.Nop(qilNode);
		}
		return qilNode;
	}

	private QilNode CompileMatchPattern(string pttrn)
	{
		SetEnvironmentFlags(allowVariables: false, allowCurrent: false, allowKey: true);
		QilNode qilNode;
		try
		{
			XPathScanner scanner = new XPathScanner(pttrn);
			qilNode = ptrnParser.Parse(scanner, ptrnBuilder);
		}
		catch (XslLoadException ex)
		{
			if (xslVersion != XslVersion.ForwardsCompatible)
			{
				ReportErrorInXPath(ex);
			}
			qilNode = f.Loop(f.For(ptrnBuilder.FixupNode), f.Error(f.String(ex.Message)));
			XPathPatternBuilder.SetPriority(qilNode, 0.5);
		}
		return qilNode;
	}

	private QilNode CompileNumberPattern(string pttrn)
	{
		SetEnvironmentFlags(allowVariables: true, allowCurrent: false, allowKey: true);
		try
		{
			XPathScanner scanner = new XPathScanner(pttrn);
			return ptrnParser.Parse(scanner, ptrnBuilder);
		}
		catch (XslLoadException ex)
		{
			if (xslVersion != XslVersion.ForwardsCompatible)
			{
				ReportErrorInXPath(ex);
			}
			return f.Error(f.String(ex.Message));
		}
	}

	private QilNode CompileKeyMatch(string pttrn)
	{
		if (keyMatchBuilder == null)
		{
			keyMatchBuilder = new KeyMatchBuilder(this);
		}
		SetEnvironmentFlags(allowVariables: false, allowCurrent: false, allowKey: false);
		if (pttrn == null)
		{
			return PhantomKeyMatch();
		}
		try
		{
			XPathScanner scanner = new XPathScanner(pttrn);
			return ptrnParser.Parse(scanner, keyMatchBuilder);
		}
		catch (XslLoadException ex)
		{
			if (xslVersion != XslVersion.ForwardsCompatible)
			{
				ReportErrorInXPath(ex);
			}
			return f.Error(f.String(ex.Message));
		}
	}

	private QilNode CompileKeyUse(Key key)
	{
		string use = key.Use;
		SetEnvironmentFlags(allowVariables: false, allowCurrent: true, allowKey: false);
		QilNode qilNode;
		if (use == null)
		{
			qilNode = f.Error(f.String(XslLoadException.CreateMessage(key.SourceLine, "Missing mandatory attribute '{0}'.", "use")));
		}
		else
		{
			try
			{
				XPathScanner scanner = new XPathScanner(use);
				qilNode = xpathParser.Parse(scanner, xpathBuilder, LexKind.Eof);
			}
			catch (XslLoadException ex)
			{
				if (xslVersion != XslVersion.ForwardsCompatible)
				{
					ReportErrorInXPath(ex);
				}
				qilNode = f.Error(f.String(ex.Message));
			}
		}
		if (qilNode is QilIterator)
		{
			qilNode = f.Nop(qilNode);
		}
		return qilNode;
	}

	private QilNode ResolveQNameDynamic(bool ignoreDefaultNs, QilNode qilName)
	{
		QilList qilList = f.BaseFactory.Sequence();
		if (ignoreDefaultNs)
		{
			qilList.Add(f.NamespaceDecl(f.String(string.Empty), f.String(string.Empty)));
		}
		CompilerScopeManager<QilIterator>.NamespaceEnumerator enumerator = scope.GetEnumerator();
		while (enumerator.MoveNext())
		{
			CompilerScopeManager<QilIterator>.ScopeRecord current = enumerator.Current;
			string ncName = current.ncName;
			string nsUri = current.nsUri;
			if (!ignoreDefaultNs || ncName.Length != 0)
			{
				qilList.Add(f.NamespaceDecl(f.String(ncName), f.String(nsUri)));
			}
		}
		return f.StrParseQName(qilName, qilList);
	}

	private QilNode GenerateApply(StylesheetLevel sheet, XslNode node)
	{
		if (compiler.Settings.CheckOnly)
		{
			return f.Sequence();
		}
		return InvokeApplyFunction(sheet, node.Name, node.Content);
	}

	private void SetArg(IList<XslNode> args, int pos, QilName name, QilNode value)
	{
		VarPar varPar;
		if (args.Count <= pos || args[pos].Name != name)
		{
			varPar = AstFactory.WithParam(name);
			args.Insert(pos, varPar);
		}
		else
		{
			varPar = (VarPar)args[pos];
		}
		varPar.Value = value;
	}

	private IList<XslNode> AddRemoveImplicitArgs(IList<XslNode> args, XslFlags flags)
	{
		if (IsDebug)
		{
			flags = XslFlags.FocusFilter;
		}
		if ((flags & XslFlags.FocusFilter) != 0)
		{
			if (args == null || args.IsReadOnly)
			{
				args = new List<XslNode>(3);
			}
			int num = 0;
			if ((flags & XslFlags.Current) != 0)
			{
				SetArg(args, num++, nameCurrent, GetCurrentNode());
			}
			if ((flags & XslFlags.Position) != 0)
			{
				SetArg(args, num++, namePosition, GetCurrentPosition());
			}
			if ((flags & XslFlags.Last) != 0)
			{
				SetArg(args, num++, nameLast, GetLastPosition());
			}
		}
		return args;
	}

	private bool FillupInvokeArgs(IList<QilNode> formalArgs, IList<XslNode> actualArgs, QilList invokeArgs)
	{
		if (actualArgs.Count != formalArgs.Count)
		{
			return false;
		}
		invokeArgs.Clear();
		for (int i = 0; i < formalArgs.Count; i++)
		{
			QilName name = ((QilParameter)formalArgs[i]).Name;
			XmlQueryType xmlType = formalArgs[i].XmlType;
			QilNode qilNode = null;
			for (int j = 0; j < actualArgs.Count; j++)
			{
				VarPar varPar = (VarPar)actualArgs[j];
				if (name.Equals(varPar.Name))
				{
					QilNode value = varPar.Value;
					XmlQueryType xmlType2 = value.XmlType;
					if (xmlType2 != xmlType && (!xmlType2.IsNode || !xmlType.IsNode || !xmlType2.IsSubtypeOf(xmlType)))
					{
						return false;
					}
					qilNode = value;
					break;
				}
			}
			if (qilNode == null)
			{
				return false;
			}
			invokeArgs.Add(qilNode);
		}
		return true;
	}

	private QilNode InvokeApplyFunction(StylesheetLevel sheet, QilName mode, IList<XslNode> actualArgs)
	{
		if (!sheet.ModeFlags.TryGetValue(mode, out var value))
		{
			value = XslFlags.None;
		}
		value |= XslFlags.Current;
		actualArgs = AddRemoveImplicitArgs(actualArgs, value);
		QilList qilList = f.ActualParameterList();
		QilFunction qilFunction = null;
		if (!sheet.ApplyFunctions.TryGetValue(mode, out var value2))
		{
			List<QilFunction> list2 = (sheet.ApplyFunctions[mode] = new List<QilFunction>());
			value2 = list2;
		}
		foreach (QilFunction item in value2)
		{
			if (FillupInvokeArgs(item.Arguments, actualArgs, qilList))
			{
				qilFunction = item;
				break;
			}
		}
		if (qilFunction == null)
		{
			qilList.Clear();
			QilList qilList2 = f.FormalParameterList();
			for (int i = 0; i < actualArgs.Count; i++)
			{
				VarPar varPar = (VarPar)actualArgs[i];
				qilList.Add(varPar.Value);
				QilParameter qilParameter = f.Parameter((i == 0) ? XmlQueryTypeFactory.NodeNotRtf : varPar.Value.XmlType);
				qilParameter.Name = CloneName(varPar.Name);
				qilList2.Add(qilParameter);
				varPar.Value = qilParameter;
			}
			qilFunction = f.Function(qilList2, f.Boolean((value & XslFlags.SideEffects) != 0), XmlQueryTypeFactory.NodeNotRtfS);
			string text = ((mode.LocalName.Length == 0) ? string.Empty : (" mode=\"" + mode.QualifiedName + "\""));
			qilFunction.DebugName = ((sheet is RootLevel) ? "<xsl:apply-templates" : "<xsl:apply-imports") + text + ">";
			value2.Add(qilFunction);
			functions.Add(qilFunction);
			QilIterator qilIterator = (QilIterator)qilList2[0];
			QilIterator qilIterator2 = f.For(f.Content(qilIterator));
			QilNode qilNode = f.Filter(qilIterator2, f.IsType(qilIterator2, XmlQueryTypeFactory.Content));
			qilNode.XmlType = XmlQueryTypeFactory.ContentS;
			LoopFocus loopFocus = curLoop;
			curLoop.SetFocus(f.For(qilNode));
			QilNode qilNode2 = InvokeApplyFunction(compiler.Root, mode, null);
			if (IsDebug)
			{
				qilNode2 = f.Sequence(InvokeOnCurrentNodeChanged(), qilNode2);
			}
			QilLoop center = curLoop.ConstructLoop(qilNode2);
			curLoop = loopFocus;
			QilTernary otherwise = f.BaseFactory.Conditional(f.IsType(qilIterator, elementOrDocumentType), center, f.Conditional(f.IsType(qilIterator, textOrAttributeType), f.TextCtor(f.XPathNodeValue(qilIterator)), f.Sequence()));
			matcherBuilder.CollectPatterns(sheet, mode);
			qilFunction.Definition = matcherBuilder.BuildMatcher(qilIterator, actualArgs, otherwise);
		}
		return f.Invoke(qilFunction, qilList);
	}

	public void ReportError(string res, params string[] args)
	{
		compiler.ReportError(lastScope.SourceLine, res, args);
	}

	public void ReportWarning(string res, params string[] args)
	{
		compiler.ReportWarning(lastScope.SourceLine, res, args);
	}

	[Conditional("DEBUG")]
	private void VerifyXPathQName(QilName qname)
	{
	}

	private string ResolvePrefix(bool ignoreDefaultNs, string prefix)
	{
		if (ignoreDefaultNs && prefix.Length == 0)
		{
			return string.Empty;
		}
		string text = scope.LookupNamespace(prefix);
		if (text == null)
		{
			if (prefix.Length == 0)
			{
				text = string.Empty;
			}
			else
			{
				ReportError("Prefix '{0}' is not defined.", prefix);
				text = compiler.CreatePhantomNamespace();
			}
		}
		return text;
	}

	private void SetLineInfoCheck(QilNode n, ISourceLineInfo lineInfo)
	{
		if (n.SourceLine == null)
		{
			SetLineInfo(n, lineInfo);
		}
	}

	private static QilNode SetLineInfo(QilNode n, ISourceLineInfo lineInfo)
	{
		if (lineInfo != null && 0 < lineInfo.Start.Line && lineInfo.Start.LessOrEqual(lineInfo.End))
		{
			n.SourceLine = lineInfo;
		}
		return n;
	}

	private QilNode AddDebugVariable(QilName name, QilNode value, QilNode content)
	{
		QilIterator qilIterator = f.Let(value);
		qilIterator.DebugName = name.ToString();
		return f.Loop(qilIterator, content);
	}

	private QilNode SetDebugNs(QilNode n, QilList nsList)
	{
		if (n != null && nsList != null)
		{
			QilNode qilNode = GetNsVar(nsList);
			if (qilNode.XmlType.Cardinality == XmlQueryCardinality.One)
			{
				qilNode = f.TypeAssert(qilNode, XmlQueryTypeFactory.NamespaceS);
			}
			n = AddDebugVariable(CloneName(nameNamespaces), qilNode, n);
		}
		return n;
	}

	private QilNode AddCurrentPositionLast(QilNode content)
	{
		if (IsDebug)
		{
			content = AddDebugVariable(CloneName(nameLast), GetLastPosition(), content);
			content = AddDebugVariable(CloneName(namePosition), GetCurrentPosition(), content);
			content = AddDebugVariable(CloneName(nameCurrent), GetCurrentNode(), content);
		}
		return content;
	}

	private QilName CloneName(QilName name)
	{
		return (QilName)name.ShallowClone(f.BaseFactory);
	}

	private void SetEnvironmentFlags(bool allowVariables, bool allowCurrent, bool allowKey)
	{
		this.allowVariables = allowVariables;
		this.allowCurrent = allowCurrent;
		this.allowKey = allowKey;
	}

	QilNode IFocus.GetCurrent()
	{
		return GetCurrentNode();
	}

	QilNode IFocus.GetPosition()
	{
		return GetCurrentPosition();
	}

	QilNode IFocus.GetLast()
	{
		return GetLastPosition();
	}

	string IXPathEnvironment.ResolvePrefix(string prefix)
	{
		return ResolvePrefixThrow(ignoreDefaultNs: true, prefix);
	}

	QilNode IXPathEnvironment.ResolveVariable(string prefix, string name)
	{
		if (!allowVariables)
		{
			throw new XslLoadException("Variables cannot be used within this expression.");
		}
		string uri = ResolvePrefixThrow(ignoreDefaultNs: true, prefix);
		QilNode qilNode = scope.LookupVariable(name, uri);
		if (qilNode == null)
		{
			throw new XslLoadException("The variable or parameter '{0}' is either not defined or it is out of scope.", Compiler.ConstructQName(prefix, name));
		}
		XmlQueryType xmlType = qilNode.XmlType;
		if (qilNode.NodeType == QilNodeType.Parameter && xmlType.IsNode && xmlType.IsNotRtf && xmlType.MaybeMany && !xmlType.IsDod)
		{
			qilNode = f.TypeAssert(qilNode, XmlQueryTypeFactory.NodeSDod);
		}
		return qilNode;
	}

	QilNode IXPathEnvironment.ResolveFunction(string prefix, string name, IList<QilNode> args, IFocus env)
	{
		if (prefix.Length == 0)
		{
			if (FunctionTable.TryGetValue(name, out var value))
			{
				value.CastArguments(args, name, f);
				switch (value.id)
				{
				case FuncId.Current:
					if (!allowCurrent)
					{
						throw new XslLoadException("The 'current()' function cannot be used in a pattern.");
					}
					return ((IFocus)this).GetCurrent();
				case FuncId.Key:
					if (!allowKey)
					{
						throw new XslLoadException("The 'key()' function cannot be used in 'use' and 'match' attributes of 'xsl:key' element.");
					}
					return CompileFnKey(args[0], args[1], env);
				case FuncId.Document:
					return CompileFnDocument(args[0], (args.Count > 1) ? args[1] : null);
				case FuncId.FormatNumber:
					return CompileFormatNumber(args[0], args[1], (args.Count > 2) ? args[2] : null);
				case FuncId.UnparsedEntityUri:
					return CompileUnparsedEntityUri(args[0]);
				case FuncId.GenerateId:
					return CompileGenerateId((args.Count > 0) ? args[0] : env.GetCurrent());
				case FuncId.SystemProperty:
					return CompileSystemProperty(args[0]);
				case FuncId.ElementAvailable:
					return CompileElementAvailable(args[0]);
				case FuncId.FunctionAvailable:
					return CompileFunctionAvailable(args[0]);
				default:
					return null;
				}
			}
			throw new XslLoadException("'{0}()' is an unknown XSLT function.", Compiler.ConstructQName(prefix, name));
		}
		string text = ResolvePrefixThrow(ignoreDefaultNs: true, prefix);
		if (text == "urn:schemas-microsoft-com:xslt")
		{
			switch (name)
			{
			case "node-set":
				XPathBuilder.FunctionInfo<FuncId>.CheckArity(1, 1, name, args.Count);
				return CompileMsNodeSet(args[0]);
			case "string-compare":
				XPathBuilder.FunctionInfo<FuncId>.CheckArity(2, 4, name, args.Count);
				return f.InvokeMsStringCompare(f.ConvertToString(args[0]), f.ConvertToString(args[1]), (2 < args.Count) ? f.ConvertToString(args[2]) : f.String(string.Empty), (3 < args.Count) ? f.ConvertToString(args[3]) : f.String(string.Empty));
			case "utc":
				XPathBuilder.FunctionInfo<FuncId>.CheckArity(1, 1, name, args.Count);
				return f.InvokeMsUtc(f.ConvertToString(args[0]));
			case "format-date":
			case "format-time":
				XPathBuilder.FunctionInfo<FuncId>.CheckArity(1, 3, name, args.Count);
				_ = xslVersion;
				return f.InvokeMsFormatDateTime(f.ConvertToString(args[0]), (1 < args.Count) ? f.ConvertToString(args[1]) : f.String(string.Empty), (2 < args.Count) ? f.ConvertToString(args[2]) : f.String(string.Empty), f.Boolean(name == "format-date"));
			case "local-name":
				XPathBuilder.FunctionInfo<FuncId>.CheckArity(1, 1, name, args.Count);
				return f.InvokeMsLocalName(f.ConvertToString(args[0]));
			case "namespace-uri":
				XPathBuilder.FunctionInfo<FuncId>.CheckArity(1, 1, name, args.Count);
				return f.InvokeMsNamespaceUri(f.ConvertToString(args[0]), env.GetCurrent());
			case "number":
				XPathBuilder.FunctionInfo<FuncId>.CheckArity(1, 1, name, args.Count);
				return f.InvokeMsNumber(args[0]);
			}
		}
		if (text == "http://exslt.org/common")
		{
			if (name == "node-set")
			{
				XPathBuilder.FunctionInfo<FuncId>.CheckArity(1, 1, name, args.Count);
				return CompileMsNodeSet(args[0]);
			}
			if (name == "object-type")
			{
				XPathBuilder.FunctionInfo<FuncId>.CheckArity(1, 1, name, args.Count);
				return EXslObjectType(args[0]);
			}
		}
		for (int i = 0; i < args.Count; i++)
		{
			args[i] = f.SafeDocOrderDistinct(args[i]);
		}
		if (compiler.Settings.EnableScript)
		{
			XmlExtensionFunction xmlExtensionFunction = compiler.Scripts.ResolveFunction(name, text, args.Count, this);
			if (xmlExtensionFunction != null)
			{
				return GenerateScriptCall(f.QName(name, text, prefix), xmlExtensionFunction, args);
			}
		}
		else if (compiler.Scripts.ScriptClasses.ContainsKey(text))
		{
			ReportWarning("Execution of scripts was prohibited. Use the XsltSettings.EnableScript property to enable it.");
			return f.Error(lastScope.SourceLine, "Execution of scripts was prohibited. Use the XsltSettings.EnableScript property to enable it.");
		}
		return f.XsltInvokeLateBound(f.QName(name, text, prefix), args);
	}

	private QilNode GenerateScriptCall(QilName name, XmlExtensionFunction scrFunc, IList<QilNode> args)
	{
		for (int i = 0; i < args.Count; i++)
		{
			XmlQueryType xmlArgumentType = scrFunc.GetXmlArgumentType(i);
			switch (xmlArgumentType.TypeCode)
			{
			case XmlTypeCode.Boolean:
				args[i] = f.ConvertToBoolean(args[i]);
				break;
			case XmlTypeCode.Double:
				args[i] = f.ConvertToNumber(args[i]);
				break;
			case XmlTypeCode.String:
				args[i] = f.ConvertToString(args[i]);
				break;
			case XmlTypeCode.Node:
				args[i] = (xmlArgumentType.IsSingleton ? f.ConvertToNode(args[i]) : f.ConvertToNodeSet(args[i]));
				break;
			}
		}
		return f.XsltInvokeEarlyBound(name, scrFunc.Method, scrFunc.XmlReturnType, args);
	}

	private string ResolvePrefixThrow(bool ignoreDefaultNs, string prefix)
	{
		if (ignoreDefaultNs && prefix.Length == 0)
		{
			return string.Empty;
		}
		string text = scope.LookupNamespace(prefix);
		if (text == null)
		{
			if (prefix.Length != 0)
			{
				throw new XslLoadException("Prefix '{0}' is not defined.", prefix);
			}
			text = string.Empty;
		}
		return text;
	}

	private static Dictionary<string, XPathBuilder.FunctionInfo<FuncId>> CreateFunctionTable()
	{
		return new Dictionary<string, XPathBuilder.FunctionInfo<FuncId>>(16)
		{
			{
				"current",
				new XPathBuilder.FunctionInfo<FuncId>(FuncId.Current, 0, 0, null)
			},
			{
				"document",
				new XPathBuilder.FunctionInfo<FuncId>(FuncId.Document, 1, 2, argFnDocument)
			},
			{
				"key",
				new XPathBuilder.FunctionInfo<FuncId>(FuncId.Key, 2, 2, argFnKey)
			},
			{
				"format-number",
				new XPathBuilder.FunctionInfo<FuncId>(FuncId.FormatNumber, 2, 3, argFnFormatNumber)
			},
			{
				"unparsed-entity-uri",
				new XPathBuilder.FunctionInfo<FuncId>(FuncId.UnparsedEntityUri, 1, 1, XPathBuilder.argString)
			},
			{
				"generate-id",
				new XPathBuilder.FunctionInfo<FuncId>(FuncId.GenerateId, 0, 1, XPathBuilder.argNodeSet)
			},
			{
				"system-property",
				new XPathBuilder.FunctionInfo<FuncId>(FuncId.SystemProperty, 1, 1, XPathBuilder.argString)
			},
			{
				"element-available",
				new XPathBuilder.FunctionInfo<FuncId>(FuncId.ElementAvailable, 1, 1, XPathBuilder.argString)
			},
			{
				"function-available",
				new XPathBuilder.FunctionInfo<FuncId>(FuncId.FunctionAvailable, 1, 1, XPathBuilder.argString)
			}
		};
	}

	public static bool IsFunctionAvailable(string localName, string nsUri)
	{
		if (XPathBuilder.IsFunctionAvailable(localName, nsUri))
		{
			return true;
		}
		if (nsUri.Length == 0)
		{
			if (FunctionTable.ContainsKey(localName))
			{
				return localName != "unparsed-entity-uri";
			}
			return false;
		}
		if (nsUri == "urn:schemas-microsoft-com:xslt")
		{
			switch (localName)
			{
			default:
				return localName == "utc";
			case "node-set":
			case "format-date":
			case "format-time":
			case "local-name":
			case "namespace-uri":
			case "number":
			case "string-compare":
				return true;
			}
		}
		if (nsUri == "http://exslt.org/common")
		{
			if (!(localName == "node-set"))
			{
				return localName == "object-type";
			}
			return true;
		}
		return false;
	}

	public static bool IsElementAvailable(XmlQualifiedName name)
	{
		if (name.Namespace == "http://www.w3.org/1999/XSL/Transform")
		{
			string name2 = name.Name;
			switch (name2)
			{
			default:
				return name2 == "variable";
			case "apply-imports":
			case "apply-templates":
			case "attribute":
			case "call-template":
			case "choose":
			case "comment":
			case "copy":
			case "copy-of":
			case "element":
			case "fallback":
			case "for-each":
			case "if":
			case "message":
			case "number":
			case "processing-instruction":
			case "text":
			case "value-of":
				return true;
			}
		}
		return false;
	}

	private QilNode CompileFnKey(QilNode name, QilNode keys, IFocus env)
	{
		QilIterator name2;
		QilIterator expr;
		QilIterator n;
		QilNode collection = (keys.XmlType.IsNode ? ((!keys.XmlType.IsSingleton) ? f.Loop(n = f.For(keys), CompileSingleKey(name, f.ConvertToString(n), env)) : CompileSingleKey(name, f.ConvertToString(keys), env)) : ((!keys.XmlType.IsAtomicValue) ? f.Loop(name2 = f.Let(name), f.Loop(expr = f.Let(keys), f.Conditional(f.Not(f.IsType(expr, XmlQueryTypeFactory.AnyAtomicType)), f.Loop(n = f.For(f.TypeAssert(expr, XmlQueryTypeFactory.NodeS)), CompileSingleKey(name2, f.ConvertToString(n), env)), CompileSingleKey(name2, f.XsltConvert(expr, XmlQueryTypeFactory.StringX), env)))) : CompileSingleKey(name, f.ConvertToString(keys), env)));
		return f.DocOrderDistinct(collection);
	}

	private QilNode CompileSingleKey(QilNode name, QilNode key, IFocus env)
	{
		if (name.NodeType == QilNodeType.LiteralString)
		{
			string text = (QilLiteral)name;
			compiler.ParseQName(text, out var prefix, out var localName, default(ThrowErrorHelper));
			string uri = ResolvePrefixThrow(ignoreDefaultNs: true, prefix);
			QilName key2 = f.QName(localName, uri, prefix);
			if (!compiler.Keys.Contains(key2))
			{
				throw new XslLoadException("A reference to key '{0}' cannot be resolved. An 'xsl:key' of this name must be declared at the top level of the stylesheet.", text);
			}
			return CompileSingleKey(compiler.Keys[key2], key, env);
		}
		if (generalKey == null)
		{
			generalKey = CreateGeneralKeyFunction();
		}
		QilIterator qilIterator = f.Let(name);
		QilNode qilNode = ResolveQNameDynamic(ignoreDefaultNs: true, qilIterator);
		QilNode body = f.Invoke(generalKey, f.ActualParameterList(qilIterator, qilNode, key, env.GetCurrent()));
		return f.Loop(qilIterator, body);
	}

	private QilNode CompileSingleKey(List<Key> defList, QilNode key, IFocus env)
	{
		if (defList.Count == 1)
		{
			return f.Invoke(defList[0].Function, f.ActualParameterList(env.GetCurrent(), key));
		}
		QilIterator qilIterator = f.Let(key);
		QilNode qilNode = f.Sequence();
		foreach (Key def in defList)
		{
			qilNode.Add(f.Invoke(def.Function, f.ActualParameterList(env.GetCurrent(), qilIterator)));
		}
		return f.Loop(qilIterator, qilNode);
	}

	private QilNode CompileSingleKey(List<Key> defList, QilIterator key, QilIterator context)
	{
		QilList qilList = f.BaseFactory.Sequence();
		QilNode qilNode = null;
		foreach (Key def in defList)
		{
			qilNode = f.Invoke(def.Function, f.ActualParameterList(context, key));
			qilList.Add(qilNode);
		}
		if (defList.Count != 1)
		{
			return qilList;
		}
		return qilNode;
	}

	private QilFunction CreateGeneralKeyFunction()
	{
		QilIterator qilIterator = f.Parameter(XmlQueryTypeFactory.StringX);
		QilIterator qilIterator2 = f.Parameter(XmlQueryTypeFactory.QNameX);
		QilIterator qilIterator3 = f.Parameter(XmlQueryTypeFactory.StringX);
		QilIterator qilIterator4 = f.Parameter(XmlQueryTypeFactory.NodeNotRtf);
		QilNode qilNode = f.Error("A reference to key '{0}' cannot be resolved. An 'xsl:key' of this name must be declared at the top level of the stylesheet.", qilIterator);
		for (int i = 0; i < compiler.Keys.Count; i++)
		{
			qilNode = f.Conditional(f.Eq(qilIterator2, compiler.Keys[i][0].Name.DeepClone(f.BaseFactory)), CompileSingleKey(compiler.Keys[i], qilIterator3, qilIterator4), qilNode);
		}
		QilFunction qilFunction = f.Function(f.FormalParameterList(qilIterator, qilIterator2, qilIterator3, qilIterator4), qilNode, f.False());
		qilFunction.DebugName = "key";
		functions.Add(qilFunction);
		return qilFunction;
	}

	private QilNode CompileFnDocument(QilNode uris, QilNode baseNode)
	{
		if (!compiler.Settings.EnableDocumentFunction)
		{
			ReportWarning("Execution of the 'document()' function was prohibited. Use the XsltSettings.EnableDocumentFunction property to enable it.");
			return f.Error(lastScope.SourceLine, "Execution of the 'document()' function was prohibited. Use the XsltSettings.EnableDocumentFunction property to enable it.");
		}
		QilIterator qilIterator;
		if (uris.XmlType.IsNode)
		{
			return f.DocOrderDistinct(f.Loop(qilIterator = f.For(uris), CompileSingleDocument(f.ConvertToString(qilIterator), baseNode ?? qilIterator)));
		}
		if (uris.XmlType.IsAtomicValue)
		{
			return CompileSingleDocument(f.ConvertToString(uris), baseNode);
		}
		QilIterator qilIterator2 = f.Let(uris);
		QilIterator qilIterator3 = ((baseNode != null) ? f.Let(baseNode) : null);
		QilNode qilNode = f.Conditional(f.Not(f.IsType(qilIterator2, XmlQueryTypeFactory.AnyAtomicType)), f.DocOrderDistinct(f.Loop(qilIterator = f.For(f.TypeAssert(qilIterator2, XmlQueryTypeFactory.NodeS)), CompileSingleDocument(f.ConvertToString(qilIterator), qilIterator3 ?? qilIterator))), CompileSingleDocument(f.XsltConvert(qilIterator2, XmlQueryTypeFactory.StringX), qilIterator3));
		qilNode = ((baseNode != null) ? f.Loop(qilIterator3, qilNode) : qilNode);
		return f.Loop(qilIterator2, qilNode);
	}

	private QilNode CompileSingleDocument(QilNode uri, QilNode baseNode)
	{
		QilIterator n;
		QilNode baseUri = ((baseNode == null) ? f.String(lastScope.SourceLine.Uri) : ((!baseNode.XmlType.IsSingleton) ? f.StrConcat(f.Loop(n = f.FirstNode(baseNode), f.InvokeBaseUri(n))) : f.InvokeBaseUri(baseNode)));
		return f.DataSource(uri, baseUri);
	}

	private QilNode CompileFormatNumber(QilNode value, QilNode formatPicture, QilNode formatName)
	{
		XmlQualifiedName xmlQualifiedName;
		if (formatName != null)
		{
			xmlQualifiedName = ((formatName.NodeType != QilNodeType.LiteralString) ? null : ResolveQNameThrow(ignoreDefaultNs: true, formatName));
		}
		else
		{
			xmlQualifiedName = new XmlQualifiedName();
			formatName = f.String(string.Empty);
		}
		if (xmlQualifiedName != null)
		{
			DecimalFormatDecl format;
			if (compiler.DecimalFormats.Contains(xmlQualifiedName))
			{
				format = compiler.DecimalFormats[xmlQualifiedName];
			}
			else
			{
				if (xmlQualifiedName != DecimalFormatDecl.Default.Name)
				{
					throw new XslLoadException("Decimal format '{0}' is not defined.", (QilLiteral)formatName);
				}
				format = DecimalFormatDecl.Default;
			}
			if (formatPicture.NodeType == QilNodeType.LiteralString)
			{
				QilIterator qilIterator = f.Let(f.InvokeRegisterDecimalFormatter(formatPicture, format));
				qilIterator.DebugName = f.QName("formatter" + formatterCnt++, "urn:schemas-microsoft-com:xslt-debug").ToString();
				gloVars.Add(qilIterator);
				return f.InvokeFormatNumberStatic(value, qilIterator);
			}
			formatNumberDynamicUsed = true;
			QilNode decimalFormatName = f.QName(xmlQualifiedName.Name, xmlQualifiedName.Namespace);
			return f.InvokeFormatNumberDynamic(value, formatPicture, decimalFormatName, formatName);
		}
		formatNumberDynamicUsed = true;
		QilIterator qilIterator2 = f.Let(formatName);
		QilNode decimalFormatName2 = ResolveQNameDynamic(ignoreDefaultNs: true, qilIterator2);
		return f.Loop(qilIterator2, f.InvokeFormatNumberDynamic(value, formatPicture, decimalFormatName2, qilIterator2));
	}

	private QilNode CompileUnparsedEntityUri(QilNode n)
	{
		return f.Error(lastScope.SourceLine, "'{0}()' is an unsupported XSLT function.", "unparsed-entity-uri");
	}

	private QilNode CompileGenerateId(QilNode n)
	{
		if (n.XmlType.IsSingleton)
		{
			return f.XsltGenerateId(n);
		}
		QilIterator expr;
		return f.StrConcat(f.Loop(expr = f.FirstNode(n), f.XsltGenerateId(expr)));
	}

	private XmlQualifiedName ResolveQNameThrow(bool ignoreDefaultNs, QilNode qilName)
	{
		string qname = (QilLiteral)qilName;
		compiler.ParseQName(qname, out var prefix, out var localName, default(ThrowErrorHelper));
		string ns = ResolvePrefixThrow(ignoreDefaultNs, prefix);
		return new XmlQualifiedName(localName, ns);
	}

	private QilNode CompileSystemProperty(QilNode name)
	{
		if (name.NodeType == QilNodeType.LiteralString)
		{
			XmlQualifiedName xmlQualifiedName = ResolveQNameThrow(ignoreDefaultNs: true, name);
			if (EvaluateFuncCalls)
			{
				XPathItem xPathItem = XsltFunctions.SystemProperty(xmlQualifiedName);
				if (xPathItem.ValueType == XsltConvert.StringType)
				{
					return f.String(xPathItem.Value);
				}
				return f.Double(xPathItem.ValueAsDouble);
			}
			name = f.QName(xmlQualifiedName.Name, xmlQualifiedName.Namespace);
		}
		else
		{
			name = ResolveQNameDynamic(ignoreDefaultNs: true, name);
		}
		return f.InvokeSystemProperty(name);
	}

	private QilNode CompileElementAvailable(QilNode name)
	{
		if (name.NodeType == QilNodeType.LiteralString)
		{
			XmlQualifiedName xmlQualifiedName = ResolveQNameThrow(ignoreDefaultNs: false, name);
			if (EvaluateFuncCalls)
			{
				return f.Boolean(IsElementAvailable(xmlQualifiedName));
			}
			name = f.QName(xmlQualifiedName.Name, xmlQualifiedName.Namespace);
		}
		else
		{
			name = ResolveQNameDynamic(ignoreDefaultNs: false, name);
		}
		return f.InvokeElementAvailable(name);
	}

	private QilNode CompileFunctionAvailable(QilNode name)
	{
		if (name.NodeType == QilNodeType.LiteralString)
		{
			XmlQualifiedName xmlQualifiedName = ResolveQNameThrow(ignoreDefaultNs: true, name);
			if (EvaluateFuncCalls && (xmlQualifiedName.Namespace.Length == 0 || xmlQualifiedName.Namespace == "http://www.w3.org/1999/XSL/Transform"))
			{
				return f.Boolean(IsFunctionAvailable(xmlQualifiedName.Name, xmlQualifiedName.Namespace));
			}
			name = f.QName(xmlQualifiedName.Name, xmlQualifiedName.Namespace);
		}
		else
		{
			name = ResolveQNameDynamic(ignoreDefaultNs: true, name);
		}
		return f.InvokeFunctionAvailable(name);
	}

	private QilNode CompileMsNodeSet(QilNode n)
	{
		if (n.XmlType.IsNode && n.XmlType.IsNotRtf)
		{
			return n;
		}
		return f.XsltConvert(n, XmlQueryTypeFactory.NodeSDod);
	}

	private QilNode EXslObjectType(QilNode n)
	{
		if (EvaluateFuncCalls)
		{
			switch (n.XmlType.TypeCode)
			{
			case XmlTypeCode.Boolean:
				return f.String("boolean");
			case XmlTypeCode.Double:
				return f.String("number");
			case XmlTypeCode.String:
				return f.String("string");
			}
			if (n.XmlType.IsNode && n.XmlType.IsNotRtf)
			{
				return f.String("node-set");
			}
		}
		return f.InvokeEXslObjectType(n);
	}
}
