using System.Collections.Generic;
using System.Globalization;
using System.Reflection;
using System.Text.RegularExpressions;
using System.Xml.Schema;

namespace System.Xml.Xsl.Qil;

internal sealed class QilXmlReader
{
	private class ReaderAnnotation
	{
		public string Id;

		public QilName Name;

		public XmlQueryType XmlType;

		public string ClrNamespace;
	}

	private static Regex lineInfoRegex;

	private static Regex typeInfoRegex;

	private static Dictionary<string, MethodInfo> nameToFactoryMethod;

	private QilFactory f;

	private XmlReader r;

	private Stack<QilList> stk;

	private bool inFwdDecls;

	private Dictionary<string, QilNode> scope;

	private Dictionary<string, QilNode> fwdDecls;

	static QilXmlReader()
	{
		lineInfoRegex = new Regex("\\[(\\d+),(\\d+) -- (\\d+),(\\d+)\\]");
		typeInfoRegex = new Regex("(\\w+);([\\w|\\|]+);(\\w+)");
		nameToFactoryMethod = new Dictionary<string, MethodInfo>();
		MethodInfo[] methods = typeof(QilFactory).GetMethods(BindingFlags.Instance | BindingFlags.Public);
		foreach (MethodInfo methodInfo in methods)
		{
			ParameterInfo[] parameters = methodInfo.GetParameters();
			int j;
			for (j = 0; j < parameters.Length && !(parameters[j].ParameterType != typeof(QilNode)); j++)
			{
			}
			if (j == parameters.Length && (!nameToFactoryMethod.ContainsKey(methodInfo.Name) || nameToFactoryMethod[methodInfo.Name].GetParameters().Length < parameters.Length))
			{
				nameToFactoryMethod[methodInfo.Name] = methodInfo;
			}
		}
	}

	public QilXmlReader(XmlReader r)
	{
		this.r = r;
		f = new QilFactory();
	}

	public QilExpression Read()
	{
		stk = new Stack<QilList>();
		inFwdDecls = false;
		scope = new Dictionary<string, QilNode>();
		fwdDecls = new Dictionary<string, QilNode>();
		stk.Push(f.Sequence());
		while (r.Read())
		{
			switch (r.NodeType)
			{
			case XmlNodeType.Element:
			{
				bool isEmptyElement = r.IsEmptyElement;
				if (StartElement() && isEmptyElement)
				{
					EndElement();
				}
				break;
			}
			case XmlNodeType.EndElement:
				EndElement();
				break;
			}
		}
		return (QilExpression)stk.Peek()[0];
	}

	private bool StartElement()
	{
		ReaderAnnotation readerAnnotation = new ReaderAnnotation();
		_ = r.LocalName;
		QilNode qilNode;
		switch (r.LocalName)
		{
		case "LiteralString":
			qilNode = f.LiteralString(ReadText());
			break;
		case "LiteralInt32":
			qilNode = f.LiteralInt32(int.Parse(ReadText(), CultureInfo.InvariantCulture));
			break;
		case "LiteralInt64":
			qilNode = f.LiteralInt64(long.Parse(ReadText(), CultureInfo.InvariantCulture));
			break;
		case "LiteralDouble":
			qilNode = f.LiteralDouble(double.Parse(ReadText(), CultureInfo.InvariantCulture));
			break;
		case "LiteralDecimal":
			qilNode = f.LiteralDecimal(decimal.Parse(ReadText(), CultureInfo.InvariantCulture));
			break;
		case "LiteralType":
			qilNode = f.LiteralType(ParseType(ReadText()));
			break;
		case "LiteralQName":
			qilNode = ParseName(r.GetAttribute("name"));
			break;
		case "For":
		case "Let":
		case "Parameter":
		case "Function":
		case "RefTo":
			readerAnnotation.Id = r.GetAttribute("id");
			readerAnnotation.Name = ParseName(r.GetAttribute("name"));
			goto default;
		case "XsltInvokeEarlyBound":
			readerAnnotation.ClrNamespace = r.GetAttribute("clrNamespace");
			goto default;
		case "ForwardDecls":
			inFwdDecls = true;
			goto default;
		default:
			qilNode = f.Sequence();
			break;
		}
		readerAnnotation.XmlType = ParseType(r.GetAttribute("xmlType"));
		qilNode.SourceLine = ParseLineInfo(r.GetAttribute("lineInfo"));
		qilNode.Annotation = readerAnnotation;
		if (qilNode is QilList)
		{
			stk.Push((QilList)qilNode);
			return true;
		}
		stk.Peek().Add(qilNode);
		return false;
	}

	private void EndElement()
	{
		MethodInfo methodInfo = null;
		QilList qilList = stk.Pop();
		ReaderAnnotation readerAnnotation = (ReaderAnnotation)qilList.Annotation;
		_ = r.LocalName;
		QilNode qilNode;
		switch (r.LocalName)
		{
		case "QilExpression":
		{
			QilExpression qilExpression = f.QilExpression(qilList[qilList.Count - 1]);
			for (int k = 0; k < qilList.Count - 1; k++)
			{
				switch (qilList[k].NodeType)
				{
				case QilNodeType.True:
				case QilNodeType.False:
					qilExpression.IsDebug = qilList[k].NodeType == QilNodeType.True;
					break;
				case QilNodeType.FunctionList:
					qilExpression.FunctionList = (QilList)qilList[k];
					break;
				case QilNodeType.GlobalVariableList:
					qilExpression.GlobalVariableList = (QilList)qilList[k];
					break;
				case QilNodeType.GlobalParameterList:
					qilExpression.GlobalParameterList = (QilList)qilList[k];
					break;
				}
			}
			qilNode = qilExpression;
			break;
		}
		case "ForwardDecls":
			inFwdDecls = false;
			return;
		case "Parameter":
		case "Let":
		case "For":
		case "Function":
		{
			string id2 = readerAnnotation.Id;
			QilName name = readerAnnotation.Name;
			qilNode = r.LocalName switch
			{
				"Parameter" => (!inFwdDecls && qilList.Count != 0) ? f.Parameter(qilList[0], name, readerAnnotation.XmlType) : f.Parameter(null, name, readerAnnotation.XmlType), 
				"Let" => (!inFwdDecls) ? f.Let(qilList[0]) : f.Let(f.Unknown(readerAnnotation.XmlType)), 
				"For" => f.For(qilList[0]), 
				_ => (!inFwdDecls) ? f.Function(qilList[0], qilList[1], qilList[2], (readerAnnotation.XmlType != null) ? readerAnnotation.XmlType : qilList[1].XmlType) : f.Function(qilList[0], qilList[1], readerAnnotation.XmlType), 
			};
			if (name != null)
			{
				((QilReference)qilNode).DebugName = name.ToString();
			}
			if (inFwdDecls)
			{
				fwdDecls[id2] = qilNode;
				scope[id2] = qilNode;
			}
			else if (fwdDecls.ContainsKey(id2))
			{
				qilNode = fwdDecls[id2];
				fwdDecls.Remove(id2);
				if (qilList.Count > 0)
				{
					qilNode[0] = qilList[0];
				}
				if (qilList.Count > 1)
				{
					qilNode[1] = qilList[1];
				}
			}
			else
			{
				scope[id2] = qilNode;
			}
			qilNode.Annotation = readerAnnotation;
			break;
		}
		case "RefTo":
		{
			string id = readerAnnotation.Id;
			stk.Peek().Add(scope[id]);
			return;
		}
		case "Sequence":
			qilNode = f.Sequence(qilList);
			break;
		case "FunctionList":
			qilNode = f.FunctionList(qilList);
			break;
		case "GlobalVariableList":
			qilNode = f.GlobalVariableList(qilList);
			break;
		case "GlobalParameterList":
			qilNode = f.GlobalParameterList(qilList);
			break;
		case "ActualParameterList":
			qilNode = f.ActualParameterList(qilList);
			break;
		case "FormalParameterList":
			qilNode = f.FormalParameterList(qilList);
			break;
		case "SortKeyList":
			qilNode = f.SortKeyList(qilList);
			break;
		case "BranchList":
			qilNode = f.BranchList(qilList);
			break;
		case "XsltInvokeEarlyBound":
		{
			MethodInfo value = null;
			QilName qilName = (QilName)qilList[0];
			Assembly[] assemblies = AppDomain.CurrentDomain.GetAssemblies();
			for (int j = 0; j < assemblies.Length; j++)
			{
				Type type = assemblies[j].GetType(readerAnnotation.ClrNamespace);
				if (type != null)
				{
					value = type.GetMethod(qilName.LocalName);
					break;
				}
			}
			qilNode = f.XsltInvokeEarlyBound(qilName, f.LiteralObject(value), qilList[1], readerAnnotation.XmlType);
			break;
		}
		default:
		{
			methodInfo = nameToFactoryMethod[r.LocalName];
			object[] array = new object[qilList.Count];
			for (int i = 0; i < array.Length; i++)
			{
				array[i] = qilList[i];
			}
			qilNode = (QilNode)methodInfo.Invoke(f, array);
			break;
		}
		}
		qilNode.SourceLine = qilList.SourceLine;
		stk.Peek().Add(qilNode);
	}

	private string ReadText()
	{
		string text = string.Empty;
		if (!r.IsEmptyElement)
		{
			while (r.Read())
			{
				XmlNodeType nodeType = r.NodeType;
				if (nodeType != XmlNodeType.Text && (uint)(nodeType - 13) > 1u)
				{
					break;
				}
				text += r.Value;
			}
		}
		return text;
	}

	private ISourceLineInfo ParseLineInfo(string s)
	{
		if (s != null && s.Length > 0)
		{
			Match match = lineInfoRegex.Match(s);
			return new SourceLineInfo("", int.Parse(match.Groups[1].Value, CultureInfo.InvariantCulture), int.Parse(match.Groups[2].Value, CultureInfo.InvariantCulture), int.Parse(match.Groups[3].Value, CultureInfo.InvariantCulture), int.Parse(match.Groups[4].Value, CultureInfo.InvariantCulture));
		}
		return null;
	}

	private XmlQueryType ParseType(string s)
	{
		if (s != null && s.Length > 0)
		{
			Match match = typeInfoRegex.Match(s);
			XmlQueryCardinality c = new XmlQueryCardinality(match.Groups[1].Value);
			bool isStrict = bool.Parse(match.Groups[3].Value);
			string[] array = match.Groups[2].Value.Split('|');
			XmlQueryType[] array2 = new XmlQueryType[array.Length];
			for (int i = 0; i < array.Length; i++)
			{
				array2[i] = XmlQueryTypeFactory.Type((XmlTypeCode)Enum.Parse(typeof(XmlTypeCode), array[i]), isStrict);
			}
			return XmlQueryTypeFactory.Product(XmlQueryTypeFactory.Choice(array2), c);
		}
		return null;
	}

	private QilName ParseName(string name)
	{
		if (name != null && name.Length > 0)
		{
			int num = name.LastIndexOf('}');
			string namespaceUri;
			if (num != -1 && name[0] == '{')
			{
				namespaceUri = name.Substring(1, num - 1);
				name = name.Substring(num + 1);
			}
			else
			{
				namespaceUri = string.Empty;
			}
			ValidateNames.ParseQNameThrow(name, out var prefix, out var localName);
			return f.LiteralQName(localName, namespaceUri, prefix);
		}
		return null;
	}
}
