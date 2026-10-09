using System.Collections.Generic;
using System.Reflection;

namespace System.Xml.Xsl.Qil;

internal class QilPatternFactory
{
	private bool debug;

	private QilFactory f;

	public QilFactory BaseFactory => f;

	public bool IsDebug => debug;

	public QilPatternFactory(QilFactory f, bool debug)
	{
		this.f = f;
		this.debug = debug;
	}

	public QilLiteral String(string val)
	{
		return f.LiteralString(val);
	}

	public QilLiteral Int32(int val)
	{
		return f.LiteralInt32(val);
	}

	public QilLiteral Double(double val)
	{
		return f.LiteralDouble(val);
	}

	public QilName QName(string local, string uri, string prefix)
	{
		return f.LiteralQName(local, uri, prefix);
	}

	public QilName QName(string local, string uri)
	{
		return f.LiteralQName(local, uri, string.Empty);
	}

	public QilName QName(string local)
	{
		return f.LiteralQName(local, string.Empty, string.Empty);
	}

	public QilNode Unknown(XmlQueryType t)
	{
		return f.Unknown(t);
	}

	public QilExpression QilExpression(QilNode root, QilFactory factory)
	{
		return f.QilExpression(root, factory);
	}

	public QilList FunctionList()
	{
		return f.FunctionList();
	}

	public QilList GlobalVariableList()
	{
		return f.GlobalVariableList();
	}

	public QilList GlobalParameterList()
	{
		return f.GlobalParameterList();
	}

	public QilList ActualParameterList()
	{
		return f.ActualParameterList();
	}

	public QilList ActualParameterList(QilNode arg1)
	{
		QilList qilList = f.ActualParameterList();
		qilList.Add(arg1);
		return qilList;
	}

	public QilList ActualParameterList(QilNode arg1, QilNode arg2)
	{
		QilList qilList = f.ActualParameterList();
		qilList.Add(arg1);
		qilList.Add(arg2);
		return qilList;
	}

	public QilList ActualParameterList(params QilNode[] args)
	{
		return f.ActualParameterList(args);
	}

	public QilList FormalParameterList()
	{
		return f.FormalParameterList();
	}

	public QilList FormalParameterList(QilNode arg1)
	{
		QilList qilList = f.FormalParameterList();
		qilList.Add(arg1);
		return qilList;
	}

	public QilList FormalParameterList(QilNode arg1, QilNode arg2)
	{
		QilList qilList = f.FormalParameterList();
		qilList.Add(arg1);
		qilList.Add(arg2);
		return qilList;
	}

	public QilList FormalParameterList(params QilNode[] args)
	{
		return f.FormalParameterList(args);
	}

	public QilList SortKeyList()
	{
		return f.SortKeyList();
	}

	public QilList SortKeyList(QilSortKey key)
	{
		QilList qilList = f.SortKeyList();
		qilList.Add(key);
		return qilList;
	}

	public QilList BranchList(params QilNode[] args)
	{
		return f.BranchList(args);
	}

	public QilNode OptimizeBarrier(QilNode child)
	{
		return f.OptimizeBarrier(child);
	}

	public QilNode DataSource(QilNode name, QilNode baseUri)
	{
		return f.DataSource(name, baseUri);
	}

	public QilNode Nop(QilNode child)
	{
		return f.Nop(child);
	}

	public QilNode Error(QilNode text)
	{
		return f.Error(text);
	}

	public QilNode Warning(QilNode text)
	{
		return f.Warning(text);
	}

	public QilIterator For(QilNode binding)
	{
		return f.For(binding);
	}

	public QilIterator Let(QilNode binding)
	{
		return f.Let(binding);
	}

	public QilParameter Parameter(XmlQueryType t)
	{
		return f.Parameter(t);
	}

	public QilParameter Parameter(QilNode defaultValue, QilName name, XmlQueryType t)
	{
		return f.Parameter(defaultValue, name, t);
	}

	public QilNode PositionOf(QilIterator expr)
	{
		return f.PositionOf(expr);
	}

	public QilNode True()
	{
		return f.True();
	}

	public QilNode False()
	{
		return f.False();
	}

	public QilNode Boolean(bool b)
	{
		if (!b)
		{
			return False();
		}
		return True();
	}

	private static void CheckLogicArg(QilNode arg)
	{
	}

	public QilNode And(QilNode left, QilNode right)
	{
		CheckLogicArg(left);
		CheckLogicArg(right);
		if (!debug)
		{
			if (left.NodeType == QilNodeType.True || right.NodeType == QilNodeType.False)
			{
				return right;
			}
			if (left.NodeType == QilNodeType.False || right.NodeType == QilNodeType.True)
			{
				return left;
			}
		}
		return f.And(left, right);
	}

	public QilNode Or(QilNode left, QilNode right)
	{
		CheckLogicArg(left);
		CheckLogicArg(right);
		if (!debug)
		{
			if (left.NodeType == QilNodeType.True || right.NodeType == QilNodeType.False)
			{
				return left;
			}
			if (left.NodeType == QilNodeType.False || right.NodeType == QilNodeType.True)
			{
				return right;
			}
		}
		return f.Or(left, right);
	}

	public QilNode Not(QilNode child)
	{
		if (!debug)
		{
			switch (child.NodeType)
			{
			case QilNodeType.True:
				return f.False();
			case QilNodeType.False:
				return f.True();
			case QilNodeType.Not:
				return ((QilUnary)child).Child;
			}
		}
		return f.Not(child);
	}

	public QilNode Conditional(QilNode condition, QilNode trueBranch, QilNode falseBranch)
	{
		if (!debug)
		{
			switch (condition.NodeType)
			{
			case QilNodeType.True:
				return trueBranch;
			case QilNodeType.False:
				return falseBranch;
			case QilNodeType.Not:
				return Conditional(((QilUnary)condition).Child, falseBranch, trueBranch);
			}
		}
		return f.Conditional(condition, trueBranch, falseBranch);
	}

	public QilNode Choice(QilNode expr, QilList branches)
	{
		if (!debug)
		{
			switch (branches.Count)
			{
			case 1:
				return f.Loop(f.Let(expr), branches[0]);
			case 2:
				return f.Conditional(f.Eq(expr, f.LiteralInt32(0)), branches[0], branches[1]);
			}
		}
		return f.Choice(expr, branches);
	}

	public QilNode Length(QilNode child)
	{
		return f.Length(child);
	}

	public QilNode Sequence()
	{
		return f.Sequence();
	}

	public QilNode Sequence(QilNode child)
	{
		if (!debug)
		{
			return child;
		}
		QilList qilList = f.Sequence();
		qilList.Add(child);
		return qilList;
	}

	public QilNode Sequence(QilNode child1, QilNode child2)
	{
		QilList qilList = f.Sequence();
		qilList.Add(child1);
		qilList.Add(child2);
		return qilList;
	}

	public QilNode Sequence(params QilNode[] args)
	{
		if (!debug)
		{
			switch (args.Length)
			{
			case 0:
				return f.Sequence();
			case 1:
				return args[0];
			}
		}
		QilList qilList = f.Sequence();
		foreach (QilNode node in args)
		{
			qilList.Add(node);
		}
		return qilList;
	}

	public QilNode Union(QilNode left, QilNode right)
	{
		return f.Union(left, right);
	}

	public QilNode Sum(QilNode collection)
	{
		return f.Sum(collection);
	}

	public QilNode Negate(QilNode child)
	{
		return f.Negate(child);
	}

	public QilNode Add(QilNode left, QilNode right)
	{
		return f.Add(left, right);
	}

	public QilNode Subtract(QilNode left, QilNode right)
	{
		return f.Subtract(left, right);
	}

	public QilNode Multiply(QilNode left, QilNode right)
	{
		return f.Multiply(left, right);
	}

	public QilNode Divide(QilNode left, QilNode right)
	{
		return f.Divide(left, right);
	}

	public QilNode Modulo(QilNode left, QilNode right)
	{
		return f.Modulo(left, right);
	}

	public QilNode StrLength(QilNode str)
	{
		return f.StrLength(str);
	}

	public QilNode StrConcat(QilNode values)
	{
		if (!debug && values.XmlType.IsSingleton)
		{
			return values;
		}
		return f.StrConcat(values);
	}

	public QilNode StrConcat(params QilNode[] args)
	{
		return StrConcat((IList<QilNode>)args);
	}

	public QilNode StrConcat(IList<QilNode> args)
	{
		if (!debug)
		{
			switch (args.Count)
			{
			case 0:
				return f.LiteralString(string.Empty);
			case 1:
				return StrConcat(args[0]);
			}
		}
		return StrConcat(f.Sequence(args));
	}

	public QilNode StrParseQName(QilNode str, QilNode ns)
	{
		return f.StrParseQName(str, ns);
	}

	public QilNode Ne(QilNode left, QilNode right)
	{
		return f.Ne(left, right);
	}

	public QilNode Eq(QilNode left, QilNode right)
	{
		return f.Eq(left, right);
	}

	public QilNode Gt(QilNode left, QilNode right)
	{
		return f.Gt(left, right);
	}

	public QilNode Ge(QilNode left, QilNode right)
	{
		return f.Ge(left, right);
	}

	public QilNode Lt(QilNode left, QilNode right)
	{
		return f.Lt(left, right);
	}

	public QilNode Le(QilNode left, QilNode right)
	{
		return f.Le(left, right);
	}

	public QilNode Is(QilNode left, QilNode right)
	{
		return f.Is(left, right);
	}

	public QilNode After(QilNode left, QilNode right)
	{
		return f.After(left, right);
	}

	public QilNode Before(QilNode left, QilNode right)
	{
		return f.Before(left, right);
	}

	public QilNode Loop(QilIterator variable, QilNode body)
	{
		if (!debug && body == variable.Binding)
		{
			return body;
		}
		return f.Loop(variable, body);
	}

	public QilNode Filter(QilIterator variable, QilNode expr)
	{
		if (!debug && expr.NodeType == QilNodeType.True)
		{
			return variable.Binding;
		}
		return f.Filter(variable, expr);
	}

	public QilNode Sort(QilIterator iter, QilNode keys)
	{
		return f.Sort(iter, keys);
	}

	public QilSortKey SortKey(QilNode key, QilNode collation)
	{
		return f.SortKey(key, collation);
	}

	public QilNode DocOrderDistinct(QilNode collection)
	{
		if (collection.NodeType == QilNodeType.DocOrderDistinct)
		{
			return collection;
		}
		return f.DocOrderDistinct(collection);
	}

	public QilFunction Function(QilList args, QilNode sideEffects, XmlQueryType resultType)
	{
		return f.Function(args, sideEffects, resultType);
	}

	public QilFunction Function(QilList args, QilNode defn, QilNode sideEffects)
	{
		return f.Function(args, defn, sideEffects, defn.XmlType);
	}

	public QilNode Invoke(QilFunction func, QilList args)
	{
		return f.Invoke(func, args);
	}

	public QilNode Content(QilNode context)
	{
		return f.Content(context);
	}

	public QilNode Parent(QilNode context)
	{
		return f.Parent(context);
	}

	public QilNode Root(QilNode context)
	{
		return f.Root(context);
	}

	public QilNode XmlContext()
	{
		return f.XmlContext();
	}

	public QilNode Descendant(QilNode expr)
	{
		return f.Descendant(expr);
	}

	public QilNode DescendantOrSelf(QilNode context)
	{
		return f.DescendantOrSelf(context);
	}

	public QilNode Ancestor(QilNode expr)
	{
		return f.Ancestor(expr);
	}

	public QilNode AncestorOrSelf(QilNode expr)
	{
		return f.AncestorOrSelf(expr);
	}

	public QilNode Preceding(QilNode expr)
	{
		return f.Preceding(expr);
	}

	public QilNode FollowingSibling(QilNode expr)
	{
		return f.FollowingSibling(expr);
	}

	public QilNode PrecedingSibling(QilNode expr)
	{
		return f.PrecedingSibling(expr);
	}

	public QilNode NodeRange(QilNode left, QilNode right)
	{
		return f.NodeRange(left, right);
	}

	public QilBinary Deref(QilNode context, QilNode id)
	{
		return f.Deref(context, id);
	}

	public QilNode ElementCtor(QilNode name, QilNode content)
	{
		return f.ElementCtor(name, content);
	}

	public QilNode AttributeCtor(QilNode name, QilNode val)
	{
		return f.AttributeCtor(name, val);
	}

	public QilNode CommentCtor(QilNode content)
	{
		return f.CommentCtor(content);
	}

	public QilNode PICtor(QilNode name, QilNode content)
	{
		return f.PICtor(name, content);
	}

	public QilNode TextCtor(QilNode content)
	{
		return f.TextCtor(content);
	}

	public QilNode RawTextCtor(QilNode content)
	{
		return f.RawTextCtor(content);
	}

	public QilNode DocumentCtor(QilNode child)
	{
		return f.DocumentCtor(child);
	}

	public QilNode NamespaceDecl(QilNode prefix, QilNode uri)
	{
		return f.NamespaceDecl(prefix, uri);
	}

	public QilNode RtfCtor(QilNode content, QilNode baseUri)
	{
		return f.RtfCtor(content, baseUri);
	}

	public QilNode NameOf(QilNode expr)
	{
		return f.NameOf(expr);
	}

	public QilNode LocalNameOf(QilNode expr)
	{
		return f.LocalNameOf(expr);
	}

	public QilNode NamespaceUriOf(QilNode expr)
	{
		return f.NamespaceUriOf(expr);
	}

	public QilNode PrefixOf(QilNode expr)
	{
		return f.PrefixOf(expr);
	}

	public QilNode TypeAssert(QilNode expr, XmlQueryType t)
	{
		return f.TypeAssert(expr, t);
	}

	public QilNode IsType(QilNode expr, XmlQueryType t)
	{
		return f.IsType(expr, t);
	}

	public QilNode IsEmpty(QilNode set)
	{
		return f.IsEmpty(set);
	}

	public QilNode XPathNodeValue(QilNode expr)
	{
		return f.XPathNodeValue(expr);
	}

	public QilNode XPathFollowing(QilNode expr)
	{
		return f.XPathFollowing(expr);
	}

	public QilNode XPathNamespace(QilNode expr)
	{
		return f.XPathNamespace(expr);
	}

	public QilNode XPathPreceding(QilNode expr)
	{
		return f.XPathPreceding(expr);
	}

	public QilNode XsltGenerateId(QilNode expr)
	{
		return f.XsltGenerateId(expr);
	}

	public QilNode XsltInvokeEarlyBound(QilNode name, MethodInfo d, XmlQueryType t, IList<QilNode> args)
	{
		QilList qilList = f.ActualParameterList();
		qilList.Add(args);
		return f.XsltInvokeEarlyBound(name, f.LiteralObject(d), qilList, t);
	}

	public QilNode XsltInvokeLateBound(QilNode name, IList<QilNode> args)
	{
		QilList qilList = f.ActualParameterList();
		qilList.Add(args);
		return f.XsltInvokeLateBound(name, qilList);
	}

	public QilNode XsltCopy(QilNode expr, QilNode content)
	{
		return f.XsltCopy(expr, content);
	}

	public QilNode XsltCopyOf(QilNode expr)
	{
		return f.XsltCopyOf(expr);
	}

	public QilNode XsltConvert(QilNode expr, XmlQueryType t)
	{
		return f.XsltConvert(expr, t);
	}
}
