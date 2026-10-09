using System.Collections.Generic;
using System.Reflection;
using System.Reflection.Emit;
using System.Xml.Schema;
using System.Xml.Utils;
using System.Xml.XPath;
using System.Xml.Xsl.Qil;
using System.Xml.Xsl.Runtime;

namespace System.Xml.Xsl.IlGen;

internal class XmlILVisitor : QilVisitor
{
	private QilExpression qil;

	private GenerateHelper helper;

	private IteratorDescriptor iterCurr;

	private IteratorDescriptor iterNested;

	private int indexId;

	public void Visit(QilExpression qil, GenerateHelper helper, MethodInfo methRoot)
	{
		this.qil = qil;
		this.helper = helper;
		iterNested = null;
		indexId = 0;
		PrepareGlobalValues(qil.GlobalParameterList);
		PrepareGlobalValues(qil.GlobalVariableList);
		VisitGlobalValues(qil.GlobalParameterList);
		VisitGlobalValues(qil.GlobalVariableList);
		foreach (QilFunction function in qil.FunctionList)
		{
			Function(function);
		}
		this.helper.MethodBegin(methRoot, null, initWriters: true);
		StartNestedIterator(qil.Root);
		Visit(qil.Root);
		EndNestedIterator(qil.Root);
		this.helper.MethodEnd();
	}

	private void PrepareGlobalValues(QilList globalIterators)
	{
		foreach (QilIterator globalIterator in globalIterators)
		{
			MethodInfo functionBinding = XmlILAnnotation.Write(globalIterator).FunctionBinding;
			IteratorDescriptor iteratorDescriptor = new IteratorDescriptor(helper);
			iteratorDescriptor.Storage = StorageDescriptor.Global(functionBinding, GetItemStorageType(globalIterator), !globalIterator.XmlType.IsSingleton);
			XmlILAnnotation.Write(globalIterator).CachedIteratorDescriptor = iteratorDescriptor;
		}
	}

	private void VisitGlobalValues(QilList globalIterators)
	{
		foreach (QilIterator globalIterator in globalIterators)
		{
			QilParameter qilParameter = globalIterator as QilParameter;
			MethodInfo globalLocation = XmlILAnnotation.Write(globalIterator).CachedIteratorDescriptor.Storage.GlobalLocation;
			bool isCached = !globalIterator.XmlType.IsSingleton;
			int num = helper.StaticData.DeclareGlobalValue(globalIterator.DebugName);
			helper.MethodBegin(globalLocation, globalIterator.SourceLine, initWriters: false);
			Label label = helper.DefineLabel();
			Label label2 = helper.DefineLabel();
			helper.LoadQueryRuntime();
			helper.LoadInteger(num);
			helper.Call(XmlILMethods.GlobalComputed);
			helper.Emit(OpCodes.Brtrue, label);
			StartNestedIterator(globalIterator);
			if (qilParameter != null)
			{
				LocalBuilder locBldr = helper.DeclareLocal("$$$param", typeof(object));
				helper.CallGetParameter(qilParameter.Name.LocalName, qilParameter.Name.NamespaceUri);
				helper.Emit(OpCodes.Stloc, locBldr);
				helper.Emit(OpCodes.Ldloc, locBldr);
				helper.Emit(OpCodes.Brfalse, label2);
				helper.LoadQueryRuntime();
				helper.LoadInteger(num);
				helper.LoadQueryRuntime();
				helper.LoadInteger(helper.StaticData.DeclareXmlType(XmlQueryTypeFactory.ItemS));
				helper.Emit(OpCodes.Ldloc, locBldr);
				helper.Call(XmlILMethods.ChangeTypeXsltResult);
				helper.CallSetGlobalValue(typeof(object));
				helper.EmitUnconditionalBranch(OpCodes.Br, label);
			}
			helper.MarkLabel(label2);
			if (globalIterator.Binding != null)
			{
				helper.LoadQueryRuntime();
				helper.LoadInteger(num);
				NestedVisitEnsureStack(globalIterator.Binding, GetItemStorageType(globalIterator), isCached);
				helper.CallSetGlobalValue(GetStorageType(globalIterator));
			}
			else
			{
				helper.LoadQueryRuntime();
				helper.Emit(OpCodes.Ldstr, System.Xml.Utils.Res.GetString("Supplied XsltArgumentList does not contain a parameter with local name '{0}' and namespace '{1}'.", qilParameter.Name.LocalName, qilParameter.Name.NamespaceUri));
				helper.Call(XmlILMethods.ThrowException);
			}
			EndNestedIterator(globalIterator);
			helper.MarkLabel(label);
			helper.CallGetGlobalValue(num, GetStorageType(globalIterator));
			helper.MethodEnd();
		}
	}

	private void Function(QilFunction ndFunc)
	{
		foreach (QilIterator argument in ndFunc.Arguments)
		{
			IteratorDescriptor iteratorDescriptor = new IteratorDescriptor(helper);
			int paramIndex = XmlILAnnotation.Write(argument).ArgumentPosition + 1;
			iteratorDescriptor.Storage = StorageDescriptor.Parameter(paramIndex, GetItemStorageType(argument), !argument.XmlType.IsSingleton);
			XmlILAnnotation.Write(argument).CachedIteratorDescriptor = iteratorDescriptor;
		}
		MethodInfo functionBinding = XmlILAnnotation.Write(ndFunc).FunctionBinding;
		bool flag = XmlILConstructInfo.Read(ndFunc).ConstructMethod == XmlILConstructMethod.Writer;
		helper.MethodBegin(functionBinding, ndFunc.SourceLine, flag);
		foreach (QilIterator argument2 in ndFunc.Arguments)
		{
			if (qil.IsDebug && argument2.SourceLine != null)
			{
				helper.DebugSequencePoint(argument2.SourceLine);
			}
			if (argument2.Binding != null)
			{
				int paramIndex = (argument2.Annotation as XmlILAnnotation).ArgumentPosition + 1;
				Label label = helper.DefineLabel();
				helper.LoadQueryRuntime();
				helper.LoadParameter(paramIndex);
				helper.LoadInteger(29);
				helper.Call(XmlILMethods.SeqMatchesCode);
				helper.Emit(OpCodes.Brfalse, label);
				StartNestedIterator(argument2);
				NestedVisitEnsureStack(argument2.Binding, GetItemStorageType(argument2), !argument2.XmlType.IsSingleton);
				EndNestedIterator(argument2);
				helper.SetParameter(paramIndex);
				helper.MarkLabel(label);
			}
		}
		StartNestedIterator(ndFunc);
		if (flag)
		{
			NestedVisit(ndFunc.Definition);
		}
		else
		{
			NestedVisitEnsureStack(ndFunc.Definition, GetItemStorageType(ndFunc), !ndFunc.XmlType.IsSingleton);
		}
		EndNestedIterator(ndFunc);
		helper.MethodEnd();
	}

	protected override QilNode Visit(QilNode nd)
	{
		if (nd == null)
		{
			return null;
		}
		if (qil.IsDebug && nd.SourceLine != null && !(nd is QilIterator))
		{
			helper.DebugSequencePoint(nd.SourceLine);
		}
		switch (XmlILConstructInfo.Read(nd).ConstructMethod)
		{
		case XmlILConstructMethod.WriterThenIterator:
			NestedConstruction(nd);
			break;
		case XmlILConstructMethod.IteratorThenWriter:
			CopySequence(nd);
			break;
		default:
			base.Visit(nd);
			break;
		}
		return nd;
	}

	protected override QilNode VisitChildren(QilNode parent)
	{
		return parent;
	}

	private void NestedConstruction(QilNode nd)
	{
		helper.CallStartSequenceConstruction();
		base.Visit(nd);
		helper.CallEndSequenceConstruction();
		iterCurr.Storage = StorageDescriptor.Stack(typeof(XPathItem), isCached: true);
	}

	private void CopySequence(QilNode nd)
	{
		XmlQueryType xmlType = nd.XmlType;
		StartWriterLoop(nd, out var hasOnEnd, out var lblOnEnd);
		if (xmlType.IsSingleton)
		{
			helper.LoadQueryOutput();
			base.Visit(nd);
			iterCurr.EnsureItemStorageType(nd.XmlType, typeof(XPathItem));
		}
		else
		{
			base.Visit(nd);
			iterCurr.EnsureItemStorageType(nd.XmlType, typeof(XPathItem));
			iterCurr.EnsureNoStackNoCache("$$$copyTemp");
			helper.LoadQueryOutput();
		}
		iterCurr.EnsureStackNoCache();
		helper.Call(XmlILMethods.WriteItem);
		EndWriterLoop(nd, hasOnEnd, lblOnEnd);
	}

	protected override QilNode VisitDataSource(QilDataSource ndSrc)
	{
		helper.LoadQueryContext();
		NestedVisitEnsureStack(ndSrc.Name);
		NestedVisitEnsureStack(ndSrc.BaseUri);
		helper.Call(XmlILMethods.GetDataSource);
		LocalBuilder localBuilder = helper.DeclareLocal("$$$navDoc", typeof(XPathNavigator));
		helper.Emit(OpCodes.Stloc, localBuilder);
		helper.Emit(OpCodes.Ldloc, localBuilder);
		helper.Emit(OpCodes.Brfalse, iterCurr.GetLabelNext());
		iterCurr.Storage = StorageDescriptor.Local(localBuilder, typeof(XPathNavigator), isCached: false);
		return ndSrc;
	}

	protected override QilNode VisitNop(QilUnary ndNop)
	{
		return Visit(ndNop.Child);
	}

	protected override QilNode VisitOptimizeBarrier(QilUnary ndBarrier)
	{
		return Visit(ndBarrier.Child);
	}

	protected override QilNode VisitError(QilUnary ndErr)
	{
		helper.LoadQueryRuntime();
		NestedVisitEnsureStack(ndErr.Child);
		helper.Call(XmlILMethods.ThrowException);
		if (XmlILConstructInfo.Read(ndErr).ConstructMethod == XmlILConstructMethod.Writer)
		{
			iterCurr.Storage = StorageDescriptor.None();
		}
		else
		{
			helper.Emit(OpCodes.Ldnull);
			iterCurr.Storage = StorageDescriptor.Stack(typeof(XPathItem), isCached: false);
		}
		return ndErr;
	}

	protected override QilNode VisitWarning(QilUnary ndWarning)
	{
		helper.LoadQueryRuntime();
		NestedVisitEnsureStack(ndWarning.Child);
		helper.Call(XmlILMethods.SendMessage);
		if (XmlILConstructInfo.Read(ndWarning).ConstructMethod == XmlILConstructMethod.Writer)
		{
			iterCurr.Storage = StorageDescriptor.None();
		}
		else
		{
			VisitEmpty(ndWarning);
		}
		return ndWarning;
	}

	protected override QilNode VisitTrue(QilNode ndTrue)
	{
		if (iterCurr.CurrentBranchingContext != 0)
		{
			helper.EmitUnconditionalBranch((iterCurr.CurrentBranchingContext == BranchingContext.OnTrue) ? OpCodes.Brtrue : OpCodes.Brfalse, iterCurr.LabelBranch);
			iterCurr.Storage = StorageDescriptor.None();
		}
		else
		{
			helper.LoadBoolean(boolVal: true);
			iterCurr.Storage = StorageDescriptor.Stack(typeof(bool), isCached: false);
		}
		return ndTrue;
	}

	protected override QilNode VisitFalse(QilNode ndFalse)
	{
		if (iterCurr.CurrentBranchingContext != 0)
		{
			helper.EmitUnconditionalBranch((iterCurr.CurrentBranchingContext == BranchingContext.OnFalse) ? OpCodes.Brtrue : OpCodes.Brfalse, iterCurr.LabelBranch);
			iterCurr.Storage = StorageDescriptor.None();
		}
		else
		{
			helper.LoadBoolean(boolVal: false);
			iterCurr.Storage = StorageDescriptor.Stack(typeof(bool), isCached: false);
		}
		return ndFalse;
	}

	protected override QilNode VisitLiteralString(QilLiteral ndStr)
	{
		helper.Emit(OpCodes.Ldstr, (string)ndStr);
		iterCurr.Storage = StorageDescriptor.Stack(typeof(string), isCached: false);
		return ndStr;
	}

	protected override QilNode VisitLiteralInt32(QilLiteral ndInt)
	{
		helper.LoadInteger(ndInt);
		iterCurr.Storage = StorageDescriptor.Stack(typeof(int), isCached: false);
		return ndInt;
	}

	protected override QilNode VisitLiteralInt64(QilLiteral ndLong)
	{
		helper.Emit(OpCodes.Ldc_I8, (long)ndLong);
		iterCurr.Storage = StorageDescriptor.Stack(typeof(long), isCached: false);
		return ndLong;
	}

	protected override QilNode VisitLiteralDouble(QilLiteral ndDbl)
	{
		helper.Emit(OpCodes.Ldc_R8, (double)ndDbl);
		iterCurr.Storage = StorageDescriptor.Stack(typeof(double), isCached: false);
		return ndDbl;
	}

	protected override QilNode VisitLiteralDecimal(QilLiteral ndDec)
	{
		helper.ConstructLiteralDecimal(ndDec);
		iterCurr.Storage = StorageDescriptor.Stack(typeof(decimal), isCached: false);
		return ndDec;
	}

	protected override QilNode VisitLiteralQName(QilName ndQName)
	{
		helper.ConstructLiteralQName(ndQName.LocalName, ndQName.NamespaceUri);
		iterCurr.Storage = StorageDescriptor.Stack(typeof(XmlQualifiedName), isCached: false);
		return ndQName;
	}

	protected override QilNode VisitAnd(QilBinary ndAnd)
	{
		IteratorDescriptor iteratorDescriptor = iterCurr;
		StartNestedIterator(ndAnd.Left);
		Label lblOnFalse = StartConjunctiveTests(iteratorDescriptor.CurrentBranchingContext, iteratorDescriptor.LabelBranch);
		Visit(ndAnd.Left);
		EndNestedIterator(ndAnd.Left);
		StartNestedIterator(ndAnd.Right);
		StartLastConjunctiveTest(iteratorDescriptor.CurrentBranchingContext, iteratorDescriptor.LabelBranch, lblOnFalse);
		Visit(ndAnd.Right);
		EndNestedIterator(ndAnd.Right);
		EndConjunctiveTests(iteratorDescriptor.CurrentBranchingContext, iteratorDescriptor.LabelBranch, lblOnFalse);
		return ndAnd;
	}

	private Label StartConjunctiveTests(BranchingContext brctxt, Label lblBranch)
	{
		if (brctxt == BranchingContext.OnFalse)
		{
			iterCurr.SetBranching(BranchingContext.OnFalse, lblBranch);
			return lblBranch;
		}
		Label label = helper.DefineLabel();
		iterCurr.SetBranching(BranchingContext.OnFalse, label);
		return label;
	}

	private void StartLastConjunctiveTest(BranchingContext brctxt, Label lblBranch, Label lblOnFalse)
	{
		if (brctxt == BranchingContext.OnTrue)
		{
			iterCurr.SetBranching(BranchingContext.OnTrue, lblBranch);
		}
		else
		{
			iterCurr.SetBranching(BranchingContext.OnFalse, lblOnFalse);
		}
	}

	private void EndConjunctiveTests(BranchingContext brctxt, Label lblBranch, Label lblOnFalse)
	{
		switch (brctxt)
		{
		case BranchingContext.OnTrue:
			helper.MarkLabel(lblOnFalse);
			goto case BranchingContext.OnFalse;
		case BranchingContext.OnFalse:
			iterCurr.Storage = StorageDescriptor.None();
			break;
		case BranchingContext.None:
			helper.ConvBranchToBool(lblOnFalse, isTrueBranch: false);
			iterCurr.Storage = StorageDescriptor.Stack(typeof(bool), isCached: false);
			break;
		}
	}

	protected override QilNode VisitOr(QilBinary ndOr)
	{
		Label label = default(Label);
		switch (iterCurr.CurrentBranchingContext)
		{
		case BranchingContext.OnFalse:
			label = helper.DefineLabel();
			NestedVisitWithBranch(ndOr.Left, BranchingContext.OnTrue, label);
			break;
		case BranchingContext.OnTrue:
			NestedVisitWithBranch(ndOr.Left, BranchingContext.OnTrue, iterCurr.LabelBranch);
			break;
		default:
			label = helper.DefineLabel();
			NestedVisitWithBranch(ndOr.Left, BranchingContext.OnTrue, label);
			break;
		}
		switch (iterCurr.CurrentBranchingContext)
		{
		case BranchingContext.OnFalse:
			NestedVisitWithBranch(ndOr.Right, BranchingContext.OnFalse, iterCurr.LabelBranch);
			break;
		case BranchingContext.OnTrue:
			NestedVisitWithBranch(ndOr.Right, BranchingContext.OnTrue, iterCurr.LabelBranch);
			break;
		default:
			NestedVisitWithBranch(ndOr.Right, BranchingContext.OnTrue, label);
			break;
		}
		switch (iterCurr.CurrentBranchingContext)
		{
		case BranchingContext.OnFalse:
			helper.MarkLabel(label);
			goto case BranchingContext.OnTrue;
		case BranchingContext.OnTrue:
			iterCurr.Storage = StorageDescriptor.None();
			break;
		case BranchingContext.None:
			helper.ConvBranchToBool(label, isTrueBranch: true);
			iterCurr.Storage = StorageDescriptor.Stack(typeof(bool), isCached: false);
			break;
		}
		return ndOr;
	}

	protected override QilNode VisitNot(QilUnary ndNot)
	{
		Label lblBranch = default(Label);
		switch (iterCurr.CurrentBranchingContext)
		{
		case BranchingContext.OnFalse:
			NestedVisitWithBranch(ndNot.Child, BranchingContext.OnTrue, iterCurr.LabelBranch);
			break;
		case BranchingContext.OnTrue:
			NestedVisitWithBranch(ndNot.Child, BranchingContext.OnFalse, iterCurr.LabelBranch);
			break;
		default:
			lblBranch = helper.DefineLabel();
			NestedVisitWithBranch(ndNot.Child, BranchingContext.OnTrue, lblBranch);
			break;
		}
		if (iterCurr.CurrentBranchingContext == BranchingContext.None)
		{
			helper.ConvBranchToBool(lblBranch, isTrueBranch: false);
			iterCurr.Storage = StorageDescriptor.Stack(typeof(bool), isCached: false);
		}
		else
		{
			iterCurr.Storage = StorageDescriptor.None();
		}
		return ndNot;
	}

	protected override QilNode VisitConditional(QilTernary ndCond)
	{
		if (XmlILConstructInfo.Read(ndCond).ConstructMethod == XmlILConstructMethod.Writer)
		{
			Label label = helper.DefineLabel();
			NestedVisitWithBranch(ndCond.Left, BranchingContext.OnFalse, label);
			NestedVisit(ndCond.Center);
			if (ndCond.Right.NodeType == QilNodeType.Sequence && ndCond.Right.Count == 0)
			{
				helper.MarkLabel(label);
				NestedVisit(ndCond.Right);
			}
			else
			{
				Label label2 = helper.DefineLabel();
				helper.EmitUnconditionalBranch(OpCodes.Br, label2);
				helper.MarkLabel(label);
				NestedVisit(ndCond.Right);
				helper.MarkLabel(label2);
			}
			iterCurr.Storage = StorageDescriptor.None();
		}
		else
		{
			LocalBuilder localBuilder = null;
			LocalBuilder localBuilder2 = null;
			Type itemStorageType = GetItemStorageType(ndCond);
			Label label3 = helper.DefineLabel();
			if (ndCond.XmlType.IsSingleton)
			{
				NestedVisitWithBranch(ndCond.Left, BranchingContext.OnFalse, label3);
			}
			else
			{
				localBuilder2 = helper.DeclareLocal("$$$cond", itemStorageType);
				localBuilder = helper.DeclareLocal("$$$boolResult", typeof(bool));
				NestedVisitEnsureLocal(ndCond.Left, localBuilder);
				helper.Emit(OpCodes.Ldloc, localBuilder);
				helper.Emit(OpCodes.Brfalse, label3);
			}
			ConditionalBranch(ndCond.Center, itemStorageType, localBuilder2);
			IteratorDescriptor iteratorDescriptor = iterNested;
			Label label4 = helper.DefineLabel();
			helper.EmitUnconditionalBranch(OpCodes.Br, label4);
			helper.MarkLabel(label3);
			ConditionalBranch(ndCond.Right, itemStorageType, localBuilder2);
			if (!ndCond.XmlType.IsSingleton)
			{
				helper.EmitUnconditionalBranch(OpCodes.Brtrue, label4);
				Label label5 = helper.DefineLabel();
				helper.MarkLabel(label5);
				helper.Emit(OpCodes.Ldloc, localBuilder);
				helper.Emit(OpCodes.Brtrue, iteratorDescriptor.GetLabelNext());
				helper.EmitUnconditionalBranch(OpCodes.Br, iterNested.GetLabelNext());
				iterCurr.SetIterator(label5, StorageDescriptor.Local(localBuilder2, itemStorageType, isCached: false));
			}
			helper.MarkLabel(label4);
		}
		return ndCond;
	}

	private void ConditionalBranch(QilNode ndBranch, Type itemStorageType, LocalBuilder locResult)
	{
		if (locResult == null)
		{
			if (iterCurr.IsBranching)
			{
				NestedVisitWithBranch(ndBranch, iterCurr.CurrentBranchingContext, iterCurr.LabelBranch);
			}
			else
			{
				NestedVisitEnsureStack(ndBranch, itemStorageType, isCached: false);
			}
		}
		else
		{
			NestedVisit(ndBranch, iterCurr.GetLabelNext());
			iterCurr.EnsureItemStorageType(ndBranch.XmlType, itemStorageType);
			iterCurr.EnsureLocalNoCache(locResult);
		}
	}

	protected override QilNode VisitChoice(QilChoice ndChoice)
	{
		NestedVisit(ndChoice.Expression);
		QilNode branches = ndChoice.Branches;
		int num = branches.Count - 1;
		Label[] array = new Label[num];
		int i;
		for (i = 0; i < num; i++)
		{
			array[i] = helper.DefineLabel();
		}
		Label label = helper.DefineLabel();
		Label label2 = helper.DefineLabel();
		helper.Emit(OpCodes.Switch, array);
		helper.EmitUnconditionalBranch(OpCodes.Br, label);
		for (i = 0; i < num; i++)
		{
			helper.MarkLabel(array[i]);
			NestedVisit(branches[i]);
			helper.EmitUnconditionalBranch(OpCodes.Br, label2);
		}
		helper.MarkLabel(label);
		NestedVisit(branches[i]);
		helper.MarkLabel(label2);
		iterCurr.Storage = StorageDescriptor.None();
		return ndChoice;
	}

	protected override QilNode VisitLength(QilUnary ndSetLen)
	{
		Label label = helper.DefineLabel();
		OptimizerPatterns optimizerPatterns = OptimizerPatterns.Read(ndSetLen);
		if (CachesResult(ndSetLen.Child))
		{
			NestedVisitEnsureStack(ndSetLen.Child);
			helper.CallCacheCount(iterNested.Storage.ItemStorageType);
		}
		else
		{
			helper.Emit(OpCodes.Ldc_I4_0);
			StartNestedIterator(ndSetLen.Child, label);
			Visit(ndSetLen.Child);
			iterCurr.EnsureNoCache();
			iterCurr.DiscardStack();
			helper.Emit(OpCodes.Ldc_I4_1);
			helper.Emit(OpCodes.Add);
			if (optimizerPatterns.MatchesPattern(OptimizerPatternName.MaxPosition))
			{
				helper.Emit(OpCodes.Dup);
				helper.LoadInteger((int)optimizerPatterns.GetArgument(OptimizerPatternArgument.ElementQName));
				helper.Emit(OpCodes.Bgt, label);
			}
			iterCurr.LoopToEnd(label);
			EndNestedIterator(ndSetLen.Child);
		}
		iterCurr.Storage = StorageDescriptor.Stack(typeof(int), isCached: false);
		return ndSetLen;
	}

	protected override QilNode VisitSequence(QilList ndSeq)
	{
		if (XmlILConstructInfo.Read(ndSeq).ConstructMethod == XmlILConstructMethod.Writer)
		{
			foreach (QilNode item in ndSeq)
			{
				NestedVisit(item);
			}
		}
		else if (ndSeq.Count == 0)
		{
			VisitEmpty(ndSeq);
		}
		else
		{
			Sequence(ndSeq);
		}
		return ndSeq;
	}

	private void VisitEmpty(QilNode nd)
	{
		helper.EmitUnconditionalBranch(OpCodes.Brtrue, iterCurr.GetLabelNext());
		helper.Emit(OpCodes.Ldnull);
		iterCurr.Storage = StorageDescriptor.Stack(typeof(XPathItem), isCached: false);
	}

	private void Sequence(QilList ndSeq)
	{
		Label label = default(Label);
		Type itemStorageType = GetItemStorageType(ndSeq);
		if (ndSeq.XmlType.IsSingleton)
		{
			foreach (QilNode item in ndSeq)
			{
				if (item.XmlType.IsSingleton)
				{
					NestedVisitEnsureStack(item);
					continue;
				}
				label = helper.DefineLabel();
				NestedVisit(item, label);
				iterCurr.DiscardStack();
				helper.MarkLabel(label);
			}
			iterCurr.Storage = StorageDescriptor.Stack(itemStorageType, isCached: false);
			return;
		}
		LocalBuilder localBuilder = helper.DeclareLocal("$$$itemList", itemStorageType);
		LocalBuilder locBldr = helper.DeclareLocal("$$$idxList", typeof(int));
		Label[] array = new Label[ndSeq.Count];
		Label label2 = helper.DefineLabel();
		for (int i = 0; i < ndSeq.Count; i++)
		{
			if (i != 0)
			{
				helper.MarkLabel(label);
			}
			label = ((i != ndSeq.Count - 1) ? helper.DefineLabel() : iterCurr.GetLabelNext());
			helper.LoadInteger(i);
			helper.Emit(OpCodes.Stloc, locBldr);
			NestedVisit(ndSeq[i], label);
			iterCurr.EnsureItemStorageType(ndSeq[i].XmlType, itemStorageType);
			iterCurr.EnsureLocalNoCache(localBuilder);
			array[i] = iterNested.GetLabelNext();
			helper.EmitUnconditionalBranch(OpCodes.Brtrue, label2);
		}
		Label label3 = helper.DefineLabel();
		helper.MarkLabel(label3);
		helper.Emit(OpCodes.Ldloc, locBldr);
		helper.Emit(OpCodes.Switch, array);
		helper.MarkLabel(label2);
		iterCurr.SetIterator(label3, StorageDescriptor.Local(localBuilder, itemStorageType, isCached: false));
	}

	protected override QilNode VisitUnion(QilBinary ndUnion)
	{
		return CreateSetIterator(ndUnion, "$$$iterUnion", typeof(UnionIterator), XmlILMethods.UnionCreate, XmlILMethods.UnionNext);
	}

	protected override QilNode VisitIntersection(QilBinary ndInter)
	{
		return CreateSetIterator(ndInter, "$$$iterInter", typeof(IntersectIterator), XmlILMethods.InterCreate, XmlILMethods.InterNext);
	}

	protected override QilNode VisitDifference(QilBinary ndDiff)
	{
		return CreateSetIterator(ndDiff, "$$$iterDiff", typeof(DifferenceIterator), XmlILMethods.DiffCreate, XmlILMethods.DiffNext);
	}

	private QilNode CreateSetIterator(QilBinary ndSet, string iterName, Type iterType, MethodInfo methCreate, MethodInfo methNext)
	{
		LocalBuilder localBuilder = helper.DeclareLocal(iterName, iterType);
		LocalBuilder localBuilder2 = helper.DeclareLocal("$$$navSet", typeof(XPathNavigator));
		helper.Emit(OpCodes.Ldloca, localBuilder);
		helper.LoadQueryRuntime();
		helper.Call(methCreate);
		Label label = helper.DefineLabel();
		Label label2 = helper.DefineLabel();
		Label label3 = helper.DefineLabel();
		NestedVisit(ndSet.Left, label);
		Label labelNext = iterNested.GetLabelNext();
		iterCurr.EnsureLocal(localBuilder2);
		helper.EmitUnconditionalBranch(OpCodes.Brtrue, label2);
		helper.MarkLabel(label3);
		NestedVisit(ndSet.Right, label);
		Label labelNext2 = iterNested.GetLabelNext();
		iterCurr.EnsureLocal(localBuilder2);
		helper.EmitUnconditionalBranch(OpCodes.Brtrue, label2);
		helper.MarkLabel(label);
		helper.Emit(OpCodes.Ldnull);
		helper.Emit(OpCodes.Stloc, localBuilder2);
		helper.MarkLabel(label2);
		helper.Emit(OpCodes.Ldloca, localBuilder);
		helper.Emit(OpCodes.Ldloc, localBuilder2);
		helper.Call(methNext);
		if (ndSet.XmlType.IsSingleton)
		{
			helper.Emit(OpCodes.Switch, new Label[3] { label3, labelNext, labelNext2 });
			iterCurr.Storage = StorageDescriptor.Current(localBuilder, typeof(XPathNavigator));
		}
		else
		{
			helper.Emit(OpCodes.Switch, new Label[4]
			{
				iterCurr.GetLabelNext(),
				label3,
				labelNext,
				labelNext2
			});
			iterCurr.SetIterator(label, StorageDescriptor.Current(localBuilder, typeof(XPathNavigator)));
		}
		return ndSet;
	}

	protected override QilNode VisitAverage(QilUnary ndAvg)
	{
		XmlILStorageMethods xmlILStorageMethods = XmlILMethods.StorageMethods[GetItemStorageType(ndAvg)];
		return CreateAggregator(ndAvg, "$$$aggAvg", xmlILStorageMethods, xmlILStorageMethods.AggAvg, xmlILStorageMethods.AggAvgResult);
	}

	protected override QilNode VisitSum(QilUnary ndSum)
	{
		XmlILStorageMethods xmlILStorageMethods = XmlILMethods.StorageMethods[GetItemStorageType(ndSum)];
		return CreateAggregator(ndSum, "$$$aggSum", xmlILStorageMethods, xmlILStorageMethods.AggSum, xmlILStorageMethods.AggSumResult);
	}

	protected override QilNode VisitMinimum(QilUnary ndMin)
	{
		XmlILStorageMethods xmlILStorageMethods = XmlILMethods.StorageMethods[GetItemStorageType(ndMin)];
		return CreateAggregator(ndMin, "$$$aggMin", xmlILStorageMethods, xmlILStorageMethods.AggMin, xmlILStorageMethods.AggMinResult);
	}

	protected override QilNode VisitMaximum(QilUnary ndMax)
	{
		XmlILStorageMethods xmlILStorageMethods = XmlILMethods.StorageMethods[GetItemStorageType(ndMax)];
		return CreateAggregator(ndMax, "$$$aggMax", xmlILStorageMethods, xmlILStorageMethods.AggMax, xmlILStorageMethods.AggMaxResult);
	}

	private QilNode CreateAggregator(QilUnary ndAgg, string aggName, XmlILStorageMethods methods, MethodInfo methAgg, MethodInfo methResult)
	{
		Label lblOnEnd = helper.DefineLabel();
		Type declaringType = methAgg.DeclaringType;
		LocalBuilder locBldr = helper.DeclareLocal(aggName, declaringType);
		helper.Emit(OpCodes.Ldloca, locBldr);
		helper.Call(methods.AggCreate);
		StartNestedIterator(ndAgg.Child, lblOnEnd);
		helper.Emit(OpCodes.Ldloca, locBldr);
		Visit(ndAgg.Child);
		iterCurr.EnsureStackNoCache();
		iterCurr.EnsureItemStorageType(ndAgg.XmlType, GetItemStorageType(ndAgg));
		helper.Call(methAgg);
		helper.Emit(OpCodes.Ldloca, locBldr);
		iterCurr.LoopToEnd(lblOnEnd);
		EndNestedIterator(ndAgg.Child);
		if (ndAgg.XmlType.MaybeEmpty)
		{
			helper.Call(methods.AggIsEmpty);
			helper.Emit(OpCodes.Brtrue, iterCurr.GetLabelNext());
			helper.Emit(OpCodes.Ldloca, locBldr);
		}
		helper.Call(methResult);
		iterCurr.Storage = StorageDescriptor.Stack(GetItemStorageType(ndAgg), isCached: false);
		return ndAgg;
	}

	protected override QilNode VisitNegate(QilUnary ndNeg)
	{
		NestedVisitEnsureStack(ndNeg.Child);
		helper.CallArithmeticOp(QilNodeType.Negate, ndNeg.XmlType.TypeCode);
		iterCurr.Storage = StorageDescriptor.Stack(GetItemStorageType(ndNeg), isCached: false);
		return ndNeg;
	}

	protected override QilNode VisitAdd(QilBinary ndPlus)
	{
		return ArithmeticOp(ndPlus);
	}

	protected override QilNode VisitSubtract(QilBinary ndMinus)
	{
		return ArithmeticOp(ndMinus);
	}

	protected override QilNode VisitMultiply(QilBinary ndMul)
	{
		return ArithmeticOp(ndMul);
	}

	protected override QilNode VisitDivide(QilBinary ndDiv)
	{
		return ArithmeticOp(ndDiv);
	}

	protected override QilNode VisitModulo(QilBinary ndMod)
	{
		return ArithmeticOp(ndMod);
	}

	private QilNode ArithmeticOp(QilBinary ndOp)
	{
		NestedVisitEnsureStack(ndOp.Left, ndOp.Right);
		helper.CallArithmeticOp(ndOp.NodeType, ndOp.XmlType.TypeCode);
		iterCurr.Storage = StorageDescriptor.Stack(GetItemStorageType(ndOp), isCached: false);
		return ndOp;
	}

	protected override QilNode VisitStrLength(QilUnary ndLen)
	{
		NestedVisitEnsureStack(ndLen.Child);
		helper.Call(XmlILMethods.StrLen);
		iterCurr.Storage = StorageDescriptor.Stack(typeof(int), isCached: false);
		return ndLen;
	}

	protected override QilNode VisitStrConcat(QilStrConcat ndStrConcat)
	{
		QilNode qilNode = ndStrConcat.Delimiter;
		if (qilNode.NodeType == QilNodeType.LiteralString && ((string)(QilLiteral)qilNode).Length == 0)
		{
			qilNode = null;
		}
		QilNode values = ndStrConcat.Values;
		bool flag;
		if (values.NodeType == QilNodeType.Sequence && values.Count < 5)
		{
			flag = true;
			foreach (QilNode item in values)
			{
				if (!item.XmlType.IsSingleton)
				{
					flag = false;
				}
			}
		}
		else
		{
			flag = false;
		}
		if (flag)
		{
			foreach (QilNode item2 in values)
			{
				NestedVisitEnsureStack(item2);
			}
			helper.CallConcatStrings(values.Count);
		}
		else
		{
			LocalBuilder localBuilder = helper.DeclareLocal("$$$strcat", typeof(StringConcat));
			helper.Emit(OpCodes.Ldloca, localBuilder);
			helper.Call(XmlILMethods.StrCatClear);
			if (qilNode != null)
			{
				helper.Emit(OpCodes.Ldloca, localBuilder);
				NestedVisitEnsureStack(qilNode);
				helper.Call(XmlILMethods.StrCatDelim);
			}
			helper.Emit(OpCodes.Ldloca, localBuilder);
			if (values.NodeType == QilNodeType.Sequence)
			{
				foreach (QilNode item3 in values)
				{
					GenerateConcat(item3, localBuilder);
				}
			}
			else
			{
				GenerateConcat(values, localBuilder);
			}
			helper.Call(XmlILMethods.StrCatResult);
		}
		iterCurr.Storage = StorageDescriptor.Stack(typeof(string), isCached: false);
		return ndStrConcat;
	}

	private void GenerateConcat(QilNode ndStr, LocalBuilder locStringConcat)
	{
		Label lblOnEnd = helper.DefineLabel();
		StartNestedIterator(ndStr, lblOnEnd);
		Visit(ndStr);
		iterCurr.EnsureStackNoCache();
		iterCurr.EnsureItemStorageType(ndStr.XmlType, typeof(string));
		helper.Call(XmlILMethods.StrCatCat);
		helper.Emit(OpCodes.Ldloca, locStringConcat);
		iterCurr.LoopToEnd(lblOnEnd);
		EndNestedIterator(ndStr);
	}

	protected override QilNode VisitStrParseQName(QilBinary ndParsedTagName)
	{
		VisitStrParseQName(ndParsedTagName, preservePrefix: false);
		return ndParsedTagName;
	}

	private void VisitStrParseQName(QilBinary ndParsedTagName, bool preservePrefix)
	{
		if (!preservePrefix)
		{
			helper.LoadQueryRuntime();
		}
		NestedVisitEnsureStack(ndParsedTagName.Left);
		if (ndParsedTagName.Right.XmlType.TypeCode == XmlTypeCode.String)
		{
			NestedVisitEnsureStack(ndParsedTagName.Right);
			if (!preservePrefix)
			{
				helper.CallParseTagName(GenerateNameType.TagNameAndNamespace);
			}
		}
		else
		{
			if (ndParsedTagName.Right.NodeType == QilNodeType.Sequence)
			{
				helper.LoadInteger(helper.StaticData.DeclarePrefixMappings(ndParsedTagName.Right));
			}
			else
			{
				helper.LoadInteger(helper.StaticData.DeclarePrefixMappings(new QilNode[1] { ndParsedTagName.Right }));
			}
			if (!preservePrefix)
			{
				helper.CallParseTagName(GenerateNameType.TagNameAndMappings);
			}
		}
		iterCurr.Storage = StorageDescriptor.Stack(typeof(XmlQualifiedName), isCached: false);
	}

	protected override QilNode VisitNe(QilBinary ndNe)
	{
		Compare(ndNe);
		return ndNe;
	}

	protected override QilNode VisitEq(QilBinary ndEq)
	{
		Compare(ndEq);
		return ndEq;
	}

	protected override QilNode VisitGt(QilBinary ndGt)
	{
		Compare(ndGt);
		return ndGt;
	}

	protected override QilNode VisitGe(QilBinary ndGe)
	{
		Compare(ndGe);
		return ndGe;
	}

	protected override QilNode VisitLt(QilBinary ndLt)
	{
		Compare(ndLt);
		return ndLt;
	}

	protected override QilNode VisitLe(QilBinary ndLe)
	{
		Compare(ndLe);
		return ndLe;
	}

	private void Compare(QilBinary ndComp)
	{
		QilNodeType nodeType = ndComp.NodeType;
		if ((nodeType == QilNodeType.Eq || nodeType == QilNodeType.Ne) && (TryZeroCompare(nodeType, ndComp.Left, ndComp.Right) || TryZeroCompare(nodeType, ndComp.Right, ndComp.Left) || TryNameCompare(nodeType, ndComp.Left, ndComp.Right) || TryNameCompare(nodeType, ndComp.Right, ndComp.Left)))
		{
			return;
		}
		NestedVisitEnsureStack(ndComp.Left, ndComp.Right);
		XmlTypeCode typeCode = ndComp.Left.XmlType.TypeCode;
		switch (typeCode)
		{
		case XmlTypeCode.String:
		case XmlTypeCode.Decimal:
		case XmlTypeCode.QName:
			if (nodeType == QilNodeType.Eq || nodeType == QilNodeType.Ne)
			{
				helper.CallCompareEquals(typeCode);
				ZeroCompare((nodeType == QilNodeType.Eq) ? QilNodeType.Ne : QilNodeType.Eq, isBoolVal: true);
			}
			else
			{
				helper.CallCompare(typeCode);
				helper.Emit(OpCodes.Ldc_I4_0);
				ClrCompare(nodeType, typeCode);
			}
			break;
		case XmlTypeCode.Boolean:
		case XmlTypeCode.Double:
		case XmlTypeCode.Integer:
		case XmlTypeCode.Int:
			ClrCompare(nodeType, typeCode);
			break;
		}
	}

	protected override QilNode VisitIs(QilBinary ndIs)
	{
		NestedVisitEnsureStack(ndIs.Left, ndIs.Right);
		helper.Call(XmlILMethods.NavSamePos);
		ZeroCompare(QilNodeType.Ne, isBoolVal: true);
		return ndIs;
	}

	protected override QilNode VisitBefore(QilBinary ndBefore)
	{
		ComparePosition(ndBefore);
		return ndBefore;
	}

	protected override QilNode VisitAfter(QilBinary ndAfter)
	{
		ComparePosition(ndAfter);
		return ndAfter;
	}

	private void ComparePosition(QilBinary ndComp)
	{
		helper.LoadQueryRuntime();
		NestedVisitEnsureStack(ndComp.Left, ndComp.Right);
		helper.Call(XmlILMethods.CompPos);
		helper.LoadInteger(0);
		ClrCompare((ndComp.NodeType == QilNodeType.Before) ? QilNodeType.Lt : QilNodeType.Gt, XmlTypeCode.String);
	}

	protected override QilNode VisitFor(QilIterator ndFor)
	{
		IteratorDescriptor cachedIteratorDescriptor = XmlILAnnotation.Write(ndFor).CachedIteratorDescriptor;
		iterCurr.Storage = cachedIteratorDescriptor.Storage;
		if (iterCurr.Storage.Location == ItemLocation.Global)
		{
			iterCurr.EnsureStack();
		}
		return ndFor;
	}

	protected override QilNode VisitLet(QilIterator ndLet)
	{
		return VisitFor(ndLet);
	}

	protected override QilNode VisitParameter(QilParameter ndParameter)
	{
		return VisitFor(ndParameter);
	}

	protected override QilNode VisitLoop(QilLoop ndLoop)
	{
		StartWriterLoop(ndLoop, out var hasOnEnd, out var lblOnEnd);
		StartBinding(ndLoop.Variable);
		Visit(ndLoop.Body);
		EndBinding(ndLoop.Variable);
		EndWriterLoop(ndLoop, hasOnEnd, lblOnEnd);
		return ndLoop;
	}

	protected override QilNode VisitFilter(QilLoop ndFilter)
	{
		if (HandleFilterPatterns(ndFilter))
		{
			return ndFilter;
		}
		StartBinding(ndFilter.Variable);
		iterCurr.SetIterator(iterNested);
		StartNestedIterator(ndFilter.Body);
		iterCurr.SetBranching(BranchingContext.OnFalse, iterCurr.ParentIterator.GetLabelNext());
		Visit(ndFilter.Body);
		EndNestedIterator(ndFilter.Body);
		EndBinding(ndFilter.Variable);
		return ndFilter;
	}

	private bool HandleFilterPatterns(QilLoop ndFilter)
	{
		OptimizerPatterns optimizerPatterns = OptimizerPatterns.Read(ndFilter);
		bool flag = optimizerPatterns.MatchesPattern(OptimizerPatternName.FilterElements);
		if (flag || optimizerPatterns.MatchesPattern(OptimizerPatternName.FilterContentKind))
		{
			XmlNodeKindFlags xmlNodeKindFlags;
			QilName qilName;
			if (flag)
			{
				xmlNodeKindFlags = XmlNodeKindFlags.Element;
				qilName = (QilName)optimizerPatterns.GetArgument(OptimizerPatternArgument.ElementQName);
			}
			else
			{
				xmlNodeKindFlags = ((XmlQueryType)optimizerPatterns.GetArgument(OptimizerPatternArgument.ElementQName)).NodeKinds;
				qilName = null;
			}
			QilNode qilNode = (QilNode)optimizerPatterns.GetArgument(OptimizerPatternArgument.StepNode);
			QilNode qilNode2 = (QilNode)optimizerPatterns.GetArgument(OptimizerPatternArgument.StepInput);
			switch (qilNode.NodeType)
			{
			case QilNodeType.Content:
				if (flag)
				{
					LocalBuilder localBuilder = helper.DeclareLocal("$$$iterElemContent", typeof(ElementContentIterator));
					helper.Emit(OpCodes.Ldloca, localBuilder);
					NestedVisitEnsureStack(qilNode2);
					helper.CallGetAtomizedName(helper.StaticData.DeclareName(qilName.LocalName));
					helper.CallGetAtomizedName(helper.StaticData.DeclareName(qilName.NamespaceUri));
					helper.Call(XmlILMethods.ElemContentCreate);
					GenerateSimpleIterator(typeof(XPathNavigator), localBuilder, XmlILMethods.ElemContentNext);
				}
				else if (xmlNodeKindFlags == XmlNodeKindFlags.Content)
				{
					CreateSimpleIterator(qilNode2, "$$$iterContent", typeof(ContentIterator), XmlILMethods.ContentCreate, XmlILMethods.ContentNext);
				}
				else
				{
					LocalBuilder localBuilder = helper.DeclareLocal("$$$iterContent", typeof(NodeKindContentIterator));
					helper.Emit(OpCodes.Ldloca, localBuilder);
					NestedVisitEnsureStack(qilNode2);
					helper.LoadInteger((int)QilXmlToXPathNodeType(xmlNodeKindFlags));
					helper.Call(XmlILMethods.KindContentCreate);
					GenerateSimpleIterator(typeof(XPathNavigator), localBuilder, XmlILMethods.KindContentNext);
				}
				return true;
			case QilNodeType.Parent:
				CreateFilteredIterator(qilNode2, "$$$iterPar", typeof(ParentIterator), XmlILMethods.ParentCreate, XmlILMethods.ParentNext, xmlNodeKindFlags, qilName, TriState.Unknown, null);
				return true;
			case QilNodeType.Ancestor:
			case QilNodeType.AncestorOrSelf:
				CreateFilteredIterator(qilNode2, "$$$iterAnc", typeof(AncestorIterator), XmlILMethods.AncCreate, XmlILMethods.AncNext, xmlNodeKindFlags, qilName, (qilNode.NodeType != QilNodeType.Ancestor) ? TriState.True : TriState.False, null);
				return true;
			case QilNodeType.Descendant:
			case QilNodeType.DescendantOrSelf:
				CreateFilteredIterator(qilNode2, "$$$iterDesc", typeof(DescendantIterator), XmlILMethods.DescCreate, XmlILMethods.DescNext, xmlNodeKindFlags, qilName, (qilNode.NodeType != QilNodeType.Descendant) ? TriState.True : TriState.False, null);
				return true;
			case QilNodeType.Preceding:
				CreateFilteredIterator(qilNode2, "$$$iterPrec", typeof(PrecedingIterator), XmlILMethods.PrecCreate, XmlILMethods.PrecNext, xmlNodeKindFlags, qilName, TriState.Unknown, null);
				return true;
			case QilNodeType.FollowingSibling:
				CreateFilteredIterator(qilNode2, "$$$iterFollSib", typeof(FollowingSiblingIterator), XmlILMethods.FollSibCreate, XmlILMethods.FollSibNext, xmlNodeKindFlags, qilName, TriState.Unknown, null);
				return true;
			case QilNodeType.PrecedingSibling:
				CreateFilteredIterator(qilNode2, "$$$iterPreSib", typeof(PrecedingSiblingIterator), XmlILMethods.PreSibCreate, XmlILMethods.PreSibNext, xmlNodeKindFlags, qilName, TriState.Unknown, null);
				return true;
			case QilNodeType.NodeRange:
				CreateFilteredIterator(qilNode2, "$$$iterRange", typeof(NodeRangeIterator), XmlILMethods.NodeRangeCreate, XmlILMethods.NodeRangeNext, xmlNodeKindFlags, qilName, TriState.Unknown, ((QilBinary)qilNode).Right);
				return true;
			case QilNodeType.XPathFollowing:
				CreateFilteredIterator(qilNode2, "$$$iterFoll", typeof(XPathFollowingIterator), XmlILMethods.XPFollCreate, XmlILMethods.XPFollNext, xmlNodeKindFlags, qilName, TriState.Unknown, null);
				return true;
			case QilNodeType.XPathPreceding:
				CreateFilteredIterator(qilNode2, "$$$iterPrec", typeof(XPathPrecedingIterator), XmlILMethods.XPPrecCreate, XmlILMethods.XPPrecNext, xmlNodeKindFlags, qilName, TriState.Unknown, null);
				return true;
			}
		}
		else
		{
			if (optimizerPatterns.MatchesPattern(OptimizerPatternName.FilterAttributeKind))
			{
				QilNode qilNode2 = (QilNode)optimizerPatterns.GetArgument(OptimizerPatternArgument.StepInput);
				CreateSimpleIterator(qilNode2, "$$$iterAttr", typeof(AttributeIterator), XmlILMethods.AttrCreate, XmlILMethods.AttrNext);
				return true;
			}
			if (optimizerPatterns.MatchesPattern(OptimizerPatternName.EqualityIndex))
			{
				Label lblOnEnd = helper.DefineLabel();
				Label label = helper.DefineLabel();
				QilIterator qilIterator = (QilIterator)optimizerPatterns.GetArgument(OptimizerPatternArgument.StepNode);
				QilNode n = (QilNode)optimizerPatterns.GetArgument(OptimizerPatternArgument.StepInput);
				LocalBuilder locBldr = helper.DeclareLocal("$$$index", typeof(XmlILIndex));
				helper.LoadQueryRuntime();
				helper.Emit(OpCodes.Ldarg_1);
				helper.LoadInteger(indexId);
				helper.Emit(OpCodes.Ldloca, locBldr);
				helper.Call(XmlILMethods.FindIndex);
				helper.Emit(OpCodes.Brtrue, label);
				helper.LoadQueryRuntime();
				helper.Emit(OpCodes.Ldarg_1);
				helper.LoadInteger(indexId);
				helper.Emit(OpCodes.Ldloc, locBldr);
				StartNestedIterator(qilIterator, lblOnEnd);
				StartBinding(qilIterator);
				Visit(n);
				iterCurr.EnsureStackNoCache();
				VisitFor(qilIterator);
				iterCurr.EnsureStackNoCache();
				iterCurr.EnsureItemStorageType(qilIterator.XmlType, typeof(XPathNavigator));
				helper.Call(XmlILMethods.IndexAdd);
				helper.Emit(OpCodes.Ldloc, locBldr);
				iterCurr.LoopToEnd(lblOnEnd);
				EndBinding(qilIterator);
				EndNestedIterator(qilIterator);
				helper.Call(XmlILMethods.AddNewIndex);
				helper.MarkLabel(label);
				helper.Emit(OpCodes.Ldloc, locBldr);
				helper.Emit(OpCodes.Ldarg_2);
				helper.Call(XmlILMethods.IndexLookup);
				iterCurr.Storage = StorageDescriptor.Stack(typeof(XPathNavigator), isCached: true);
				indexId++;
				return true;
			}
		}
		return false;
	}

	private void StartBinding(QilIterator ndIter)
	{
		OptimizerPatterns patt = OptimizerPatterns.Read(ndIter);
		if (qil.IsDebug && ndIter.SourceLine != null)
		{
			helper.DebugSequencePoint(ndIter.SourceLine);
		}
		if (ndIter.NodeType == QilNodeType.For || ndIter.XmlType.IsSingleton)
		{
			StartForBinding(ndIter, patt);
		}
		else
		{
			StartLetBinding(ndIter);
		}
		XmlILAnnotation.Write(ndIter).CachedIteratorDescriptor = iterNested;
	}

	private void StartForBinding(QilIterator ndFor, OptimizerPatterns patt)
	{
		LocalBuilder localBuilder = null;
		if (iterCurr.HasLabelNext)
		{
			StartNestedIterator(ndFor.Binding, iterCurr.GetLabelNext());
		}
		else
		{
			StartNestedIterator(ndFor.Binding);
		}
		if (patt.MatchesPattern(OptimizerPatternName.IsPositional))
		{
			localBuilder = helper.DeclareLocal("$$$pos", typeof(int));
			helper.Emit(OpCodes.Ldc_I4_0);
			helper.Emit(OpCodes.Stloc, localBuilder);
		}
		Visit(ndFor.Binding);
		if (qil.IsDebug && ndFor.DebugName != null)
		{
			helper.DebugStartScope();
			iterCurr.EnsureLocalNoCache("$$$for");
			iterCurr.Storage.LocalLocation.SetLocalSymInfo(ndFor.DebugName);
		}
		else
		{
			iterCurr.EnsureNoStackNoCache("$$$for");
		}
		if (patt.MatchesPattern(OptimizerPatternName.IsPositional))
		{
			helper.Emit(OpCodes.Ldloc, localBuilder);
			helper.Emit(OpCodes.Ldc_I4_1);
			helper.Emit(OpCodes.Add);
			helper.Emit(OpCodes.Stloc, localBuilder);
			if (patt.MatchesPattern(OptimizerPatternName.MaxPosition))
			{
				helper.Emit(OpCodes.Ldloc, localBuilder);
				helper.LoadInteger((int)patt.GetArgument(OptimizerPatternArgument.ElementQName));
				helper.Emit(OpCodes.Bgt, iterCurr.ParentIterator.GetLabelNext());
			}
			iterCurr.LocalPosition = localBuilder;
		}
		EndNestedIterator(ndFor.Binding);
		iterCurr.SetIterator(iterNested);
	}

	public void StartLetBinding(QilIterator ndLet)
	{
		StartNestedIterator(ndLet);
		NestedVisit(ndLet.Binding, GetItemStorageType(ndLet), !ndLet.XmlType.IsSingleton);
		if (qil.IsDebug && ndLet.DebugName != null)
		{
			helper.DebugStartScope();
			iterCurr.EnsureLocal("$$$cache");
			iterCurr.Storage.LocalLocation.SetLocalSymInfo(ndLet.DebugName);
		}
		else
		{
			iterCurr.EnsureNoStack("$$$cache");
		}
		EndNestedIterator(ndLet);
	}

	private void EndBinding(QilIterator ndIter)
	{
		if (qil.IsDebug && ndIter.DebugName != null)
		{
			helper.DebugEndScope();
		}
	}

	protected override QilNode VisitPositionOf(QilUnary ndPos)
	{
		LocalBuilder localPosition = XmlILAnnotation.Write(ndPos.Child as QilIterator).CachedIteratorDescriptor.LocalPosition;
		iterCurr.Storage = StorageDescriptor.Local(localPosition, typeof(int), isCached: false);
		return ndPos;
	}

	protected override QilNode VisitSort(QilLoop ndSort)
	{
		Type itemStorageType = GetItemStorageType(ndSort);
		Label lblOnEnd = helper.DefineLabel();
		XmlILStorageMethods xmlILStorageMethods = XmlILMethods.StorageMethods[itemStorageType];
		LocalBuilder localBuilder = helper.DeclareLocal("$$$cache", xmlILStorageMethods.SeqType);
		helper.Emit(OpCodes.Ldloc, localBuilder);
		helper.CallToken(xmlILStorageMethods.SeqReuse);
		helper.Emit(OpCodes.Stloc, localBuilder);
		helper.Emit(OpCodes.Ldloc, localBuilder);
		LocalBuilder localBuilder2 = helper.DeclareLocal("$$$keys", typeof(XmlSortKeyAccumulator));
		helper.Emit(OpCodes.Ldloca, localBuilder2);
		helper.Call(XmlILMethods.SortKeyCreate);
		StartNestedIterator(ndSort.Variable, lblOnEnd);
		StartBinding(ndSort.Variable);
		iterCurr.EnsureStackNoCache();
		iterCurr.EnsureItemStorageType(ndSort.Variable.XmlType, GetItemStorageType(ndSort.Variable));
		helper.Call(xmlILStorageMethods.SeqAdd);
		helper.Emit(OpCodes.Ldloca, localBuilder2);
		foreach (QilSortKey item in ndSort.Body)
		{
			VisitSortKey(item, localBuilder2);
		}
		helper.Call(XmlILMethods.SortKeyFinish);
		helper.Emit(OpCodes.Ldloc, localBuilder);
		iterCurr.LoopToEnd(lblOnEnd);
		helper.Emit(OpCodes.Pop);
		helper.Emit(OpCodes.Ldloc, localBuilder);
		helper.Emit(OpCodes.Ldloca, localBuilder2);
		helper.Call(XmlILMethods.SortKeyKeys);
		helper.Call(xmlILStorageMethods.SeqSortByKeys);
		iterCurr.Storage = StorageDescriptor.Local(localBuilder, itemStorageType, isCached: true);
		EndBinding(ndSort.Variable);
		EndNestedIterator(ndSort.Variable);
		iterCurr.SetIterator(iterNested);
		return ndSort;
	}

	private void VisitSortKey(QilSortKey ndKey, LocalBuilder locKeys)
	{
		helper.Emit(OpCodes.Ldloca, locKeys);
		if (ndKey.Collation.NodeType == QilNodeType.LiteralString)
		{
			helper.CallGetCollation(helper.StaticData.DeclareCollation((QilLiteral)ndKey.Collation));
		}
		else
		{
			helper.LoadQueryRuntime();
			NestedVisitEnsureStack(ndKey.Collation);
			helper.Call(XmlILMethods.CreateCollation);
		}
		if (ndKey.XmlType.IsSingleton)
		{
			NestedVisitEnsureStack(ndKey.Key);
			helper.AddSortKey(ndKey.Key.XmlType);
			return;
		}
		Label label = helper.DefineLabel();
		StartNestedIterator(ndKey.Key, label);
		Visit(ndKey.Key);
		iterCurr.EnsureStackNoCache();
		iterCurr.EnsureItemStorageType(ndKey.Key.XmlType, GetItemStorageType(ndKey.Key));
		helper.AddSortKey(ndKey.Key.XmlType);
		Label label2 = helper.DefineLabel();
		helper.EmitUnconditionalBranch(OpCodes.Br_S, label2);
		helper.MarkLabel(label);
		helper.AddSortKey(null);
		helper.MarkLabel(label2);
		EndNestedIterator(ndKey.Key);
	}

	protected override QilNode VisitDocOrderDistinct(QilUnary ndDod)
	{
		if (ndDod.XmlType.IsSingleton)
		{
			return Visit(ndDod.Child);
		}
		if (HandleDodPatterns(ndDod))
		{
			return ndDod;
		}
		helper.LoadQueryRuntime();
		NestedVisitEnsureCache(ndDod.Child, typeof(XPathNavigator));
		iterCurr.EnsureStack();
		helper.Call(XmlILMethods.DocOrder);
		return ndDod;
	}

	private bool HandleDodPatterns(QilUnary ndDod)
	{
		OptimizerPatterns optimizerPatterns = OptimizerPatterns.Read(ndDod);
		bool flag = optimizerPatterns.MatchesPattern(OptimizerPatternName.JoinAndDod);
		if (flag || optimizerPatterns.MatchesPattern(OptimizerPatternName.DodReverse))
		{
			OptimizerPatterns optimizerPatterns2 = OptimizerPatterns.Read((QilNode)optimizerPatterns.GetArgument(OptimizerPatternArgument.ElementQName));
			XmlNodeKindFlags kinds;
			QilName ndName;
			if (optimizerPatterns2.MatchesPattern(OptimizerPatternName.FilterElements))
			{
				kinds = XmlNodeKindFlags.Element;
				ndName = (QilName)optimizerPatterns2.GetArgument(OptimizerPatternArgument.ElementQName);
			}
			else if (optimizerPatterns2.MatchesPattern(OptimizerPatternName.FilterContentKind))
			{
				kinds = ((XmlQueryType)optimizerPatterns2.GetArgument(OptimizerPatternArgument.ElementQName)).NodeKinds;
				ndName = null;
			}
			else
			{
				kinds = (((ndDod.XmlType.NodeKinds & XmlNodeKindFlags.Attribute) != 0) ? XmlNodeKindFlags.Any : XmlNodeKindFlags.Content);
				ndName = null;
			}
			QilNode qilNode = (QilNode)optimizerPatterns2.GetArgument(OptimizerPatternArgument.StepNode);
			if (flag)
			{
				switch (qilNode.NodeType)
				{
				case QilNodeType.Content:
					CreateContainerIterator(ndDod, "$$$iterContent", typeof(ContentMergeIterator), XmlILMethods.ContentMergeCreate, XmlILMethods.ContentMergeNext, kinds, ndName, TriState.Unknown);
					return true;
				case QilNodeType.Descendant:
				case QilNodeType.DescendantOrSelf:
					CreateContainerIterator(ndDod, "$$$iterDesc", typeof(DescendantMergeIterator), XmlILMethods.DescMergeCreate, XmlILMethods.DescMergeNext, kinds, ndName, (qilNode.NodeType != QilNodeType.Descendant) ? TriState.True : TriState.False);
					return true;
				case QilNodeType.XPathFollowing:
					CreateContainerIterator(ndDod, "$$$iterFoll", typeof(XPathFollowingMergeIterator), XmlILMethods.XPFollMergeCreate, XmlILMethods.XPFollMergeNext, kinds, ndName, TriState.Unknown);
					return true;
				case QilNodeType.FollowingSibling:
					CreateContainerIterator(ndDod, "$$$iterFollSib", typeof(FollowingSiblingMergeIterator), XmlILMethods.FollSibMergeCreate, XmlILMethods.FollSibMergeNext, kinds, ndName, TriState.Unknown);
					return true;
				case QilNodeType.XPathPreceding:
					CreateContainerIterator(ndDod, "$$$iterPrec", typeof(XPathPrecedingMergeIterator), XmlILMethods.XPPrecMergeCreate, XmlILMethods.XPPrecMergeNext, kinds, ndName, TriState.Unknown);
					return true;
				}
			}
			else
			{
				QilNode ndCtxt = (QilNode)optimizerPatterns2.GetArgument(OptimizerPatternArgument.StepInput);
				switch (qilNode.NodeType)
				{
				case QilNodeType.Ancestor:
				case QilNodeType.AncestorOrSelf:
					CreateFilteredIterator(ndCtxt, "$$$iterAnc", typeof(AncestorDocOrderIterator), XmlILMethods.AncDOCreate, XmlILMethods.AncDONext, kinds, ndName, (qilNode.NodeType != QilNodeType.Ancestor) ? TriState.True : TriState.False, null);
					return true;
				case QilNodeType.PrecedingSibling:
					CreateFilteredIterator(ndCtxt, "$$$iterPreSib", typeof(PrecedingSiblingDocOrderIterator), XmlILMethods.PreSibDOCreate, XmlILMethods.PreSibDONext, kinds, ndName, TriState.Unknown, null);
					return true;
				case QilNodeType.XPathPreceding:
					CreateFilteredIterator(ndCtxt, "$$$iterPrec", typeof(XPathPrecedingDocOrderIterator), XmlILMethods.XPPrecDOCreate, XmlILMethods.XPPrecDONext, kinds, ndName, TriState.Unknown, null);
					return true;
				}
			}
		}
		else if (optimizerPatterns.MatchesPattern(OptimizerPatternName.DodMerge))
		{
			LocalBuilder locBldr = helper.DeclareLocal("$$$dodMerge", typeof(DodSequenceMerge));
			Label lblOnEnd = helper.DefineLabel();
			helper.Emit(OpCodes.Ldloca, locBldr);
			helper.LoadQueryRuntime();
			helper.Call(XmlILMethods.DodMergeCreate);
			helper.Emit(OpCodes.Ldloca, locBldr);
			StartNestedIterator(ndDod.Child, lblOnEnd);
			Visit(ndDod.Child);
			iterCurr.EnsureStack();
			helper.Call(XmlILMethods.DodMergeAdd);
			helper.Emit(OpCodes.Ldloca, locBldr);
			iterCurr.LoopToEnd(lblOnEnd);
			EndNestedIterator(ndDod.Child);
			helper.Call(XmlILMethods.DodMergeSeq);
			iterCurr.Storage = StorageDescriptor.Stack(typeof(XPathNavigator), isCached: true);
			return true;
		}
		return false;
	}

	protected override QilNode VisitInvoke(QilInvoke ndInvoke)
	{
		QilFunction function = ndInvoke.Function;
		MethodInfo functionBinding = XmlILAnnotation.Write(function).FunctionBinding;
		bool flag = XmlILConstructInfo.Read(function).ConstructMethod == XmlILConstructMethod.Writer;
		helper.LoadQueryRuntime();
		for (int i = 0; i < ndInvoke.Arguments.Count; i++)
		{
			QilNode nd = ndInvoke.Arguments[i];
			QilNode qilNode = ndInvoke.Function.Arguments[i];
			NestedVisitEnsureStack(nd, GetItemStorageType(qilNode), !qilNode.XmlType.IsSingleton);
		}
		if (OptimizerPatterns.Read(ndInvoke).MatchesPattern(OptimizerPatternName.TailCall))
		{
			helper.TailCall(functionBinding);
		}
		else
		{
			helper.Call(functionBinding);
		}
		if (!flag)
		{
			iterCurr.Storage = StorageDescriptor.Stack(GetItemStorageType(ndInvoke), !ndInvoke.XmlType.IsSingleton);
		}
		else
		{
			iterCurr.Storage = StorageDescriptor.None();
		}
		return ndInvoke;
	}

	protected override QilNode VisitContent(QilUnary ndContent)
	{
		CreateSimpleIterator(ndContent.Child, "$$$iterAttrContent", typeof(AttributeContentIterator), XmlILMethods.AttrContentCreate, XmlILMethods.AttrContentNext);
		return ndContent;
	}

	protected override QilNode VisitAttribute(QilBinary ndAttr)
	{
		QilName qilName = ndAttr.Right as QilName;
		LocalBuilder localBuilder = helper.DeclareLocal("$$$navAttr", typeof(XPathNavigator));
		SyncToNavigator(localBuilder, ndAttr.Left);
		helper.Emit(OpCodes.Ldloc, localBuilder);
		helper.CallGetAtomizedName(helper.StaticData.DeclareName(qilName.LocalName));
		helper.CallGetAtomizedName(helper.StaticData.DeclareName(qilName.NamespaceUri));
		helper.Call(XmlILMethods.NavMoveAttr);
		helper.Emit(OpCodes.Brfalse, iterCurr.GetLabelNext());
		iterCurr.Storage = StorageDescriptor.Local(localBuilder, typeof(XPathNavigator), isCached: false);
		return ndAttr;
	}

	protected override QilNode VisitParent(QilUnary ndParent)
	{
		LocalBuilder localBuilder = helper.DeclareLocal("$$$navParent", typeof(XPathNavigator));
		SyncToNavigator(localBuilder, ndParent.Child);
		helper.Emit(OpCodes.Ldloc, localBuilder);
		helper.Call(XmlILMethods.NavMoveParent);
		helper.Emit(OpCodes.Brfalse, iterCurr.GetLabelNext());
		iterCurr.Storage = StorageDescriptor.Local(localBuilder, typeof(XPathNavigator), isCached: false);
		return ndParent;
	}

	protected override QilNode VisitRoot(QilUnary ndRoot)
	{
		LocalBuilder localBuilder = helper.DeclareLocal("$$$navRoot", typeof(XPathNavigator));
		SyncToNavigator(localBuilder, ndRoot.Child);
		helper.Emit(OpCodes.Ldloc, localBuilder);
		helper.Call(XmlILMethods.NavMoveRoot);
		iterCurr.Storage = StorageDescriptor.Local(localBuilder, typeof(XPathNavigator), isCached: false);
		return ndRoot;
	}

	protected override QilNode VisitXmlContext(QilNode ndCtxt)
	{
		helper.LoadQueryContext();
		helper.Call(XmlILMethods.GetDefaultDataSource);
		iterCurr.Storage = StorageDescriptor.Stack(typeof(XPathNavigator), isCached: false);
		return ndCtxt;
	}

	protected override QilNode VisitDescendant(QilUnary ndDesc)
	{
		CreateFilteredIterator(ndDesc.Child, "$$$iterDesc", typeof(DescendantIterator), XmlILMethods.DescCreate, XmlILMethods.DescNext, XmlNodeKindFlags.Any, null, TriState.False, null);
		return ndDesc;
	}

	protected override QilNode VisitDescendantOrSelf(QilUnary ndDesc)
	{
		CreateFilteredIterator(ndDesc.Child, "$$$iterDesc", typeof(DescendantIterator), XmlILMethods.DescCreate, XmlILMethods.DescNext, XmlNodeKindFlags.Any, null, TriState.True, null);
		return ndDesc;
	}

	protected override QilNode VisitAncestor(QilUnary ndAnc)
	{
		CreateFilteredIterator(ndAnc.Child, "$$$iterAnc", typeof(AncestorIterator), XmlILMethods.AncCreate, XmlILMethods.AncNext, XmlNodeKindFlags.Any, null, TriState.False, null);
		return ndAnc;
	}

	protected override QilNode VisitAncestorOrSelf(QilUnary ndAnc)
	{
		CreateFilteredIterator(ndAnc.Child, "$$$iterAnc", typeof(AncestorIterator), XmlILMethods.AncCreate, XmlILMethods.AncNext, XmlNodeKindFlags.Any, null, TriState.True, null);
		return ndAnc;
	}

	protected override QilNode VisitPreceding(QilUnary ndPrec)
	{
		CreateFilteredIterator(ndPrec.Child, "$$$iterPrec", typeof(PrecedingIterator), XmlILMethods.PrecCreate, XmlILMethods.PrecNext, XmlNodeKindFlags.Any, null, TriState.Unknown, null);
		return ndPrec;
	}

	protected override QilNode VisitFollowingSibling(QilUnary ndFollSib)
	{
		CreateFilteredIterator(ndFollSib.Child, "$$$iterFollSib", typeof(FollowingSiblingIterator), XmlILMethods.FollSibCreate, XmlILMethods.FollSibNext, XmlNodeKindFlags.Any, null, TriState.Unknown, null);
		return ndFollSib;
	}

	protected override QilNode VisitPrecedingSibling(QilUnary ndPreSib)
	{
		CreateFilteredIterator(ndPreSib.Child, "$$$iterPreSib", typeof(PrecedingSiblingIterator), XmlILMethods.PreSibCreate, XmlILMethods.PreSibNext, XmlNodeKindFlags.Any, null, TriState.Unknown, null);
		return ndPreSib;
	}

	protected override QilNode VisitNodeRange(QilBinary ndRange)
	{
		CreateFilteredIterator(ndRange.Left, "$$$iterRange", typeof(NodeRangeIterator), XmlILMethods.NodeRangeCreate, XmlILMethods.NodeRangeNext, XmlNodeKindFlags.Any, null, TriState.Unknown, ndRange.Right);
		return ndRange;
	}

	protected override QilNode VisitDeref(QilBinary ndDeref)
	{
		LocalBuilder localBuilder = helper.DeclareLocal("$$$iterId", typeof(IdIterator));
		helper.Emit(OpCodes.Ldloca, localBuilder);
		NestedVisitEnsureStack(ndDeref.Left);
		NestedVisitEnsureStack(ndDeref.Right);
		helper.Call(XmlILMethods.IdCreate);
		GenerateSimpleIterator(typeof(XPathNavigator), localBuilder, XmlILMethods.IdNext);
		return ndDeref;
	}

	protected override QilNode VisitElementCtor(QilBinary ndElem)
	{
		XmlILConstructInfo xmlILConstructInfo = XmlILConstructInfo.Read(ndElem);
		bool flag = CheckWithinContent(xmlILConstructInfo) || !xmlILConstructInfo.IsNamespaceInScope || ElementCachesAttributes(xmlILConstructInfo);
		if (XmlILConstructInfo.Read(ndElem.Right).FinalStates == PossibleXmlStates.Any)
		{
			flag = true;
		}
		if (xmlILConstructInfo.FinalStates == PossibleXmlStates.Any)
		{
			flag = true;
		}
		if (!flag)
		{
			BeforeStartChecks(ndElem);
		}
		GenerateNameType nameType = LoadNameAndType(XPathNodeType.Element, ndElem.Left, isStart: true, flag);
		helper.CallWriteStartElement(nameType, flag);
		NestedVisit(ndElem.Right);
		if (XmlILConstructInfo.Read(ndElem.Right).FinalStates == PossibleXmlStates.EnumAttrs && !flag)
		{
			helper.CallStartElementContent();
		}
		nameType = LoadNameAndType(XPathNodeType.Element, ndElem.Left, isStart: false, flag);
		helper.CallWriteEndElement(nameType, flag);
		if (!flag)
		{
			AfterEndChecks(ndElem);
		}
		iterCurr.Storage = StorageDescriptor.None();
		return ndElem;
	}

	protected override QilNode VisitAttributeCtor(QilBinary ndAttr)
	{
		XmlILConstructInfo xmlILConstructInfo = XmlILConstructInfo.Read(ndAttr);
		bool flag = CheckEnumAttrs(xmlILConstructInfo) || !xmlILConstructInfo.IsNamespaceInScope;
		if (!flag)
		{
			BeforeStartChecks(ndAttr);
		}
		GenerateNameType nameType = LoadNameAndType(XPathNodeType.Attribute, ndAttr.Left, isStart: true, flag);
		helper.CallWriteStartAttribute(nameType, flag);
		NestedVisit(ndAttr.Right);
		helper.CallWriteEndAttribute(flag);
		if (!flag)
		{
			AfterEndChecks(ndAttr);
		}
		iterCurr.Storage = StorageDescriptor.None();
		return ndAttr;
	}

	protected override QilNode VisitCommentCtor(QilUnary ndComment)
	{
		helper.CallWriteStartComment();
		NestedVisit(ndComment.Child);
		helper.CallWriteEndComment();
		iterCurr.Storage = StorageDescriptor.None();
		return ndComment;
	}

	protected override QilNode VisitPICtor(QilBinary ndPI)
	{
		helper.LoadQueryOutput();
		NestedVisitEnsureStack(ndPI.Left);
		helper.CallWriteStartPI();
		NestedVisit(ndPI.Right);
		helper.CallWriteEndPI();
		iterCurr.Storage = StorageDescriptor.None();
		return ndPI;
	}

	protected override QilNode VisitTextCtor(QilUnary ndText)
	{
		return VisitTextCtor(ndText, disableOutputEscaping: false);
	}

	protected override QilNode VisitRawTextCtor(QilUnary ndText)
	{
		return VisitTextCtor(ndText, disableOutputEscaping: true);
	}

	private QilNode VisitTextCtor(QilUnary ndText, bool disableOutputEscaping)
	{
		XmlILConstructInfo xmlILConstructInfo = XmlILConstructInfo.Read(ndText);
		PossibleXmlStates initialStates = xmlILConstructInfo.InitialStates;
		bool flag = (uint)(initialStates - 4) > 2u && CheckWithinContent(xmlILConstructInfo);
		if (!flag)
		{
			BeforeStartChecks(ndText);
		}
		helper.LoadQueryOutput();
		NestedVisitEnsureStack(ndText.Child);
		switch (xmlILConstructInfo.InitialStates)
		{
		case PossibleXmlStates.WithinAttr:
			helper.CallWriteString(disableOutputEscaping: false, flag);
			break;
		case PossibleXmlStates.WithinComment:
			helper.Call(XmlILMethods.CommentText);
			break;
		case PossibleXmlStates.WithinPI:
			helper.Call(XmlILMethods.PIText);
			break;
		default:
			helper.CallWriteString(disableOutputEscaping, flag);
			break;
		}
		if (!flag)
		{
			AfterEndChecks(ndText);
		}
		iterCurr.Storage = StorageDescriptor.None();
		return ndText;
	}

	protected override QilNode VisitDocumentCtor(QilUnary ndDoc)
	{
		helper.CallWriteStartRoot();
		NestedVisit(ndDoc.Child);
		helper.CallWriteEndRoot();
		iterCurr.Storage = StorageDescriptor.None();
		return ndDoc;
	}

	protected override QilNode VisitNamespaceDecl(QilBinary ndNmsp)
	{
		XmlILConstructInfo info = XmlILConstructInfo.Read(ndNmsp);
		bool flag = CheckEnumAttrs(info) || MightHaveNamespacesAfterAttributes(info);
		if (!flag)
		{
			BeforeStartChecks(ndNmsp);
		}
		helper.LoadQueryOutput();
		NestedVisitEnsureStack(ndNmsp.Left);
		NestedVisitEnsureStack(ndNmsp.Right);
		helper.CallWriteNamespaceDecl(flag);
		if (!flag)
		{
			AfterEndChecks(ndNmsp);
		}
		iterCurr.Storage = StorageDescriptor.None();
		return ndNmsp;
	}

	protected override QilNode VisitRtfCtor(QilBinary ndRtf)
	{
		OptimizerPatterns optimizerPatterns = OptimizerPatterns.Read(ndRtf);
		string text = (QilLiteral)ndRtf.Right;
		if (optimizerPatterns.MatchesPattern(OptimizerPatternName.SingleTextRtf))
		{
			helper.LoadQueryRuntime();
			NestedVisitEnsureStack((QilNode)optimizerPatterns.GetArgument(OptimizerPatternArgument.ElementQName));
			helper.Emit(OpCodes.Ldstr, text);
			helper.Call(XmlILMethods.RtfConstr);
		}
		else
		{
			helper.CallStartRtfConstruction(text);
			NestedVisit(ndRtf.Left);
			helper.CallEndRtfConstruction();
		}
		iterCurr.Storage = StorageDescriptor.Stack(typeof(XPathNavigator), isCached: false);
		return ndRtf;
	}

	protected override QilNode VisitNameOf(QilUnary ndName)
	{
		return VisitNodeProperty(ndName);
	}

	protected override QilNode VisitLocalNameOf(QilUnary ndName)
	{
		return VisitNodeProperty(ndName);
	}

	protected override QilNode VisitNamespaceUriOf(QilUnary ndName)
	{
		return VisitNodeProperty(ndName);
	}

	protected override QilNode VisitPrefixOf(QilUnary ndName)
	{
		return VisitNodeProperty(ndName);
	}

	private QilNode VisitNodeProperty(QilUnary ndProp)
	{
		NestedVisitEnsureStack(ndProp.Child);
		switch (ndProp.NodeType)
		{
		case QilNodeType.NameOf:
			helper.Emit(OpCodes.Dup);
			helper.Call(XmlILMethods.NavLocalName);
			helper.Call(XmlILMethods.NavNmsp);
			helper.Construct(XmlILConstructors.QName);
			iterCurr.Storage = StorageDescriptor.Stack(typeof(XmlQualifiedName), isCached: false);
			break;
		case QilNodeType.LocalNameOf:
			helper.Call(XmlILMethods.NavLocalName);
			iterCurr.Storage = StorageDescriptor.Stack(typeof(string), isCached: false);
			break;
		case QilNodeType.NamespaceUriOf:
			helper.Call(XmlILMethods.NavNmsp);
			iterCurr.Storage = StorageDescriptor.Stack(typeof(string), isCached: false);
			break;
		case QilNodeType.PrefixOf:
			helper.Call(XmlILMethods.NavPrefix);
			iterCurr.Storage = StorageDescriptor.Stack(typeof(string), isCached: false);
			break;
		}
		return ndProp;
	}

	protected override QilNode VisitTypeAssert(QilTargetType ndTypeAssert)
	{
		if (!ndTypeAssert.Source.XmlType.IsSingleton && ndTypeAssert.XmlType.IsSingleton && !iterCurr.HasLabelNext)
		{
			Label label = helper.DefineLabel();
			helper.MarkLabel(label);
			NestedVisit(ndTypeAssert.Source, label);
		}
		else
		{
			Visit(ndTypeAssert.Source);
		}
		iterCurr.EnsureItemStorageType(ndTypeAssert.Source.XmlType, GetItemStorageType(ndTypeAssert));
		return ndTypeAssert;
	}

	protected override QilNode VisitIsType(QilTargetType ndIsType)
	{
		XmlQueryType xmlType = ndIsType.Source.XmlType;
		XmlQueryType targetType = ndIsType.TargetType;
		if (xmlType.IsSingleton && (object)targetType == XmlQueryTypeFactory.Node)
		{
			NestedVisitEnsureStack(ndIsType.Source);
			helper.Call(XmlILMethods.ItemIsNode);
			ZeroCompare(QilNodeType.Ne, isBoolVal: true);
			return ndIsType;
		}
		if (MatchesNodeKinds(ndIsType, xmlType, targetType))
		{
			return ndIsType;
		}
		XmlTypeCode xmlTypeCode = (((object)targetType == XmlQueryTypeFactory.Double) ? XmlTypeCode.Double : (((object)targetType == XmlQueryTypeFactory.String) ? XmlTypeCode.String : (((object)targetType == XmlQueryTypeFactory.Boolean) ? XmlTypeCode.Boolean : (((object)targetType == XmlQueryTypeFactory.Node) ? XmlTypeCode.Node : XmlTypeCode.None))));
		if (xmlTypeCode != 0)
		{
			helper.LoadQueryRuntime();
			NestedVisitEnsureStack(ndIsType.Source, typeof(XPathItem), !xmlType.IsSingleton);
			helper.LoadInteger((int)xmlTypeCode);
			helper.Call(xmlType.IsSingleton ? XmlILMethods.ItemMatchesCode : XmlILMethods.SeqMatchesCode);
			ZeroCompare(QilNodeType.Ne, isBoolVal: true);
			return ndIsType;
		}
		helper.LoadQueryRuntime();
		NestedVisitEnsureStack(ndIsType.Source, typeof(XPathItem), !xmlType.IsSingleton);
		helper.LoadInteger(helper.StaticData.DeclareXmlType(targetType));
		helper.Call(xmlType.IsSingleton ? XmlILMethods.ItemMatchesType : XmlILMethods.SeqMatchesType);
		ZeroCompare(QilNodeType.Ne, isBoolVal: true);
		return ndIsType;
	}

	private bool MatchesNodeKinds(QilTargetType ndIsType, XmlQueryType typDerived, XmlQueryType typBase)
	{
		bool flag = true;
		if (!typBase.IsNode || !typBase.IsSingleton)
		{
			return false;
		}
		if (!typDerived.IsNode || !typDerived.IsSingleton || !typDerived.IsNotRtf)
		{
			return false;
		}
		XmlNodeKindFlags xmlNodeKindFlags = XmlNodeKindFlags.None;
		foreach (XmlQueryType item in typBase)
		{
			if ((object)item == XmlQueryTypeFactory.Element)
			{
				xmlNodeKindFlags |= XmlNodeKindFlags.Element;
				continue;
			}
			if ((object)item == XmlQueryTypeFactory.Attribute)
			{
				xmlNodeKindFlags |= XmlNodeKindFlags.Attribute;
				continue;
			}
			if ((object)item == XmlQueryTypeFactory.Text)
			{
				xmlNodeKindFlags |= XmlNodeKindFlags.Text;
				continue;
			}
			if ((object)item == XmlQueryTypeFactory.Document)
			{
				xmlNodeKindFlags |= XmlNodeKindFlags.Document;
				continue;
			}
			if ((object)item == XmlQueryTypeFactory.Comment)
			{
				xmlNodeKindFlags |= XmlNodeKindFlags.Comment;
				continue;
			}
			if ((object)item == XmlQueryTypeFactory.PI)
			{
				xmlNodeKindFlags |= XmlNodeKindFlags.PI;
				continue;
			}
			if ((object)item == XmlQueryTypeFactory.Namespace)
			{
				xmlNodeKindFlags |= XmlNodeKindFlags.Namespace;
				continue;
			}
			return false;
		}
		xmlNodeKindFlags = typDerived.NodeKinds & xmlNodeKindFlags;
		if (!Bits.ExactlyOne((uint)xmlNodeKindFlags))
		{
			xmlNodeKindFlags = ~xmlNodeKindFlags & XmlNodeKindFlags.Any;
			flag = !flag;
		}
		XPathNodeType xPathNodeType;
		switch (xmlNodeKindFlags)
		{
		case XmlNodeKindFlags.Element:
			xPathNodeType = XPathNodeType.Element;
			break;
		case XmlNodeKindFlags.Attribute:
			xPathNodeType = XPathNodeType.Attribute;
			break;
		case XmlNodeKindFlags.Namespace:
			xPathNodeType = XPathNodeType.Namespace;
			break;
		case XmlNodeKindFlags.PI:
			xPathNodeType = XPathNodeType.ProcessingInstruction;
			break;
		case XmlNodeKindFlags.Comment:
			xPathNodeType = XPathNodeType.Comment;
			break;
		case XmlNodeKindFlags.Document:
			xPathNodeType = XPathNodeType.Root;
			break;
		default:
			helper.Emit(OpCodes.Ldc_I4_1);
			xPathNodeType = XPathNodeType.All;
			break;
		}
		NestedVisitEnsureStack(ndIsType.Source);
		helper.Call(XmlILMethods.NavType);
		if (xPathNodeType == XPathNodeType.All)
		{
			helper.Emit(OpCodes.Shl);
			int num = 0;
			if ((xmlNodeKindFlags & XmlNodeKindFlags.Document) != 0)
			{
				num |= 1;
			}
			if ((xmlNodeKindFlags & XmlNodeKindFlags.Element) != 0)
			{
				num |= 2;
			}
			if ((xmlNodeKindFlags & XmlNodeKindFlags.Attribute) != 0)
			{
				num |= 4;
			}
			if ((xmlNodeKindFlags & XmlNodeKindFlags.Text) != 0)
			{
				num |= 0x70;
			}
			if ((xmlNodeKindFlags & XmlNodeKindFlags.Comment) != 0)
			{
				num |= 0x100;
			}
			if ((xmlNodeKindFlags & XmlNodeKindFlags.PI) != 0)
			{
				num |= 0x80;
			}
			if ((xmlNodeKindFlags & XmlNodeKindFlags.Namespace) != 0)
			{
				num |= 8;
			}
			helper.LoadInteger(num);
			helper.Emit(OpCodes.And);
			ZeroCompare(flag ? QilNodeType.Ne : QilNodeType.Eq, isBoolVal: false);
		}
		else
		{
			helper.LoadInteger((int)xPathNodeType);
			ClrCompare(flag ? QilNodeType.Eq : QilNodeType.Ne, XmlTypeCode.Int);
		}
		return true;
	}

	protected override QilNode VisitIsEmpty(QilUnary ndIsEmpty)
	{
		if (CachesResult(ndIsEmpty.Child))
		{
			NestedVisitEnsureStack(ndIsEmpty.Child);
			helper.CallCacheCount(iterNested.Storage.ItemStorageType);
			switch (iterCurr.CurrentBranchingContext)
			{
			case BranchingContext.OnFalse:
				helper.TestAndBranch(0, iterCurr.LabelBranch, OpCodes.Bne_Un);
				break;
			case BranchingContext.OnTrue:
				helper.TestAndBranch(0, iterCurr.LabelBranch, OpCodes.Beq);
				break;
			default:
			{
				Label label = helper.DefineLabel();
				helper.Emit(OpCodes.Brfalse_S, label);
				helper.ConvBranchToBool(label, isTrueBranch: true);
				break;
			}
			}
		}
		else
		{
			Label label2 = helper.DefineLabel();
			IteratorDescriptor iteratorDescriptor = iterCurr;
			if (iteratorDescriptor.CurrentBranchingContext == BranchingContext.OnTrue)
			{
				StartNestedIterator(ndIsEmpty.Child, iterCurr.LabelBranch);
			}
			else
			{
				StartNestedIterator(ndIsEmpty.Child, label2);
			}
			Visit(ndIsEmpty.Child);
			iterCurr.EnsureNoCache();
			iterCurr.DiscardStack();
			switch (iteratorDescriptor.CurrentBranchingContext)
			{
			case BranchingContext.OnFalse:
				helper.EmitUnconditionalBranch(OpCodes.Br, iteratorDescriptor.LabelBranch);
				helper.MarkLabel(label2);
				break;
			case BranchingContext.None:
				helper.ConvBranchToBool(label2, isTrueBranch: true);
				break;
			}
			EndNestedIterator(ndIsEmpty.Child);
		}
		if (iterCurr.IsBranching)
		{
			iterCurr.Storage = StorageDescriptor.None();
		}
		else
		{
			iterCurr.Storage = StorageDescriptor.Stack(typeof(bool), isCached: false);
		}
		return ndIsEmpty;
	}

	protected override QilNode VisitXPathNodeValue(QilUnary ndVal)
	{
		if (ndVal.Child.XmlType.IsSingleton)
		{
			NestedVisitEnsureStack(ndVal.Child, typeof(XPathNavigator), isCached: false);
			helper.Call(XmlILMethods.Value);
		}
		else
		{
			Label label = helper.DefineLabel();
			StartNestedIterator(ndVal.Child, label);
			Visit(ndVal.Child);
			iterCurr.EnsureStackNoCache();
			helper.Call(XmlILMethods.Value);
			Label label2 = helper.DefineLabel();
			helper.EmitUnconditionalBranch(OpCodes.Br, label2);
			helper.MarkLabel(label);
			helper.Emit(OpCodes.Ldstr, "");
			helper.MarkLabel(label2);
			EndNestedIterator(ndVal.Child);
		}
		iterCurr.Storage = StorageDescriptor.Stack(typeof(string), isCached: false);
		return ndVal;
	}

	protected override QilNode VisitXPathFollowing(QilUnary ndFoll)
	{
		CreateFilteredIterator(ndFoll.Child, "$$$iterFoll", typeof(XPathFollowingIterator), XmlILMethods.XPFollCreate, XmlILMethods.XPFollNext, XmlNodeKindFlags.Any, null, TriState.Unknown, null);
		return ndFoll;
	}

	protected override QilNode VisitXPathPreceding(QilUnary ndPrec)
	{
		CreateFilteredIterator(ndPrec.Child, "$$$iterPrec", typeof(XPathPrecedingIterator), XmlILMethods.XPPrecCreate, XmlILMethods.XPPrecNext, XmlNodeKindFlags.Any, null, TriState.Unknown, null);
		return ndPrec;
	}

	protected override QilNode VisitXPathNamespace(QilUnary ndNmsp)
	{
		CreateSimpleIterator(ndNmsp.Child, "$$$iterNmsp", typeof(NamespaceIterator), XmlILMethods.NmspCreate, XmlILMethods.NmspNext);
		return ndNmsp;
	}

	protected override QilNode VisitXsltGenerateId(QilUnary ndGenId)
	{
		helper.LoadQueryRuntime();
		if (ndGenId.Child.XmlType.IsSingleton)
		{
			NestedVisitEnsureStack(ndGenId.Child, typeof(XPathNavigator), isCached: false);
			helper.Call(XmlILMethods.GenId);
		}
		else
		{
			Label label = helper.DefineLabel();
			StartNestedIterator(ndGenId.Child, label);
			Visit(ndGenId.Child);
			iterCurr.EnsureStackNoCache();
			iterCurr.EnsureItemStorageType(ndGenId.Child.XmlType, typeof(XPathNavigator));
			helper.Call(XmlILMethods.GenId);
			Label label2 = helper.DefineLabel();
			helper.EmitUnconditionalBranch(OpCodes.Br, label2);
			helper.MarkLabel(label);
			helper.Emit(OpCodes.Pop);
			helper.Emit(OpCodes.Ldstr, "");
			helper.MarkLabel(label2);
			EndNestedIterator(ndGenId.Child);
		}
		iterCurr.Storage = StorageDescriptor.Stack(typeof(string), isCached: false);
		return ndGenId;
	}

	protected override QilNode VisitXsltInvokeLateBound(QilInvokeLateBound ndInvoke)
	{
		LocalBuilder locBldr = helper.DeclareLocal("$$$args", typeof(IList<XPathItem>[]));
		QilName name = ndInvoke.Name;
		helper.LoadQueryContext();
		helper.Emit(OpCodes.Ldstr, name.LocalName);
		helper.Emit(OpCodes.Ldstr, name.NamespaceUri);
		helper.LoadInteger(ndInvoke.Arguments.Count);
		helper.Emit(OpCodes.Newarr, typeof(IList<XPathItem>));
		helper.Emit(OpCodes.Stloc, locBldr);
		for (int i = 0; i < ndInvoke.Arguments.Count; i++)
		{
			QilNode nd = ndInvoke.Arguments[i];
			helper.Emit(OpCodes.Ldloc, locBldr);
			helper.LoadInteger(i);
			helper.Emit(OpCodes.Ldelema, typeof(IList<XPathItem>));
			NestedVisitEnsureCache(nd, typeof(XPathItem));
			iterCurr.EnsureStack();
			helper.Emit(OpCodes.Stobj, typeof(IList<XPathItem>));
		}
		helper.Emit(OpCodes.Ldloc, locBldr);
		helper.Call(XmlILMethods.InvokeXsltLate);
		iterCurr.Storage = StorageDescriptor.Stack(typeof(XPathItem), isCached: true);
		return ndInvoke;
	}

	protected override QilNode VisitXsltInvokeEarlyBound(QilInvokeEarlyBound ndInvoke)
	{
		QilName name = ndInvoke.Name;
		XmlExtensionFunction xmlExtensionFunction = new XmlExtensionFunction(name.LocalName, name.NamespaceUri, ndInvoke.ClrMethod);
		Type clrReturnType = xmlExtensionFunction.ClrReturnType;
		Type storageType = GetStorageType(ndInvoke);
		if (clrReturnType != storageType && !ndInvoke.XmlType.IsEmpty)
		{
			helper.LoadQueryRuntime();
			helper.LoadInteger(helper.StaticData.DeclareXmlType(ndInvoke.XmlType));
		}
		if (!xmlExtensionFunction.Method.IsStatic)
		{
			if (name.NamespaceUri.Length == 0)
			{
				helper.LoadXsltLibrary();
			}
			else
			{
				helper.CallGetEarlyBoundObject(helper.StaticData.DeclareEarlyBound(name.NamespaceUri, xmlExtensionFunction.Method.DeclaringType), xmlExtensionFunction.Method.DeclaringType);
			}
		}
		for (int i = 0; i < ndInvoke.Arguments.Count; i++)
		{
			QilNode qilNode = ndInvoke.Arguments[i];
			XmlQueryType xmlArgumentType = xmlExtensionFunction.GetXmlArgumentType(i);
			Type clrArgumentType = xmlExtensionFunction.GetClrArgumentType(i);
			if (name.NamespaceUri.Length == 0)
			{
				Type itemStorageType = GetItemStorageType(qilNode);
				if (clrArgumentType == XmlILMethods.StorageMethods[itemStorageType].IListType)
				{
					NestedVisitEnsureStack(qilNode, itemStorageType, isCached: true);
				}
				else if (clrArgumentType == XmlILMethods.StorageMethods[typeof(XPathItem)].IListType)
				{
					NestedVisitEnsureStack(qilNode, typeof(XPathItem), isCached: true);
				}
				else if ((qilNode.XmlType.IsSingleton && clrArgumentType == itemStorageType) || qilNode.XmlType.TypeCode == XmlTypeCode.None)
				{
					NestedVisitEnsureStack(qilNode, clrArgumentType, isCached: false);
				}
				else if (qilNode.XmlType.IsSingleton && clrArgumentType == typeof(XPathItem))
				{
					NestedVisitEnsureStack(qilNode, typeof(XPathItem), isCached: false);
				}
			}
			else
			{
				Type storageType2 = GetStorageType(xmlArgumentType);
				if (xmlArgumentType.TypeCode == XmlTypeCode.Item || !clrArgumentType.IsAssignableFrom(storageType2))
				{
					helper.LoadQueryRuntime();
					helper.LoadInteger(helper.StaticData.DeclareXmlType(xmlArgumentType));
					NestedVisitEnsureStack(qilNode, GetItemStorageType(xmlArgumentType), !xmlArgumentType.IsSingleton);
					helper.TreatAs(storageType2, typeof(object));
					helper.LoadType(clrArgumentType);
					helper.Call(XmlILMethods.ChangeTypeXsltArg);
					helper.TreatAs(typeof(object), clrArgumentType);
				}
				else
				{
					NestedVisitEnsureStack(qilNode, GetItemStorageType(xmlArgumentType), !xmlArgumentType.IsSingleton);
				}
			}
		}
		helper.Call(xmlExtensionFunction.Method);
		if (ndInvoke.XmlType.IsEmpty)
		{
			helper.Emit(OpCodes.Ldsfld, XmlILMethods.StorageMethods[typeof(XPathItem)].SeqEmpty);
		}
		else if (clrReturnType != storageType)
		{
			helper.TreatAs(clrReturnType, typeof(object));
			helper.Call(XmlILMethods.ChangeTypeXsltResult);
			helper.TreatAs(typeof(object), storageType);
		}
		else if (name.NamespaceUri.Length != 0 && !clrReturnType.IsValueType)
		{
			Label label = helper.DefineLabel();
			helper.Emit(OpCodes.Dup);
			helper.Emit(OpCodes.Brtrue, label);
			helper.LoadQueryRuntime();
			helper.Emit(OpCodes.Ldstr, System.Xml.Utils.Res.GetString("Extension functions cannot return null values."));
			helper.Call(XmlILMethods.ThrowException);
			helper.MarkLabel(label);
		}
		iterCurr.Storage = StorageDescriptor.Stack(GetItemStorageType(ndInvoke), !ndInvoke.XmlType.IsSingleton);
		return ndInvoke;
	}

	protected override QilNode VisitXsltCopy(QilBinary ndCopy)
	{
		Label label = helper.DefineLabel();
		helper.LoadQueryOutput();
		NestedVisitEnsureStack(ndCopy.Left);
		helper.Call(XmlILMethods.StartCopy);
		helper.Emit(OpCodes.Brfalse, label);
		NestedVisit(ndCopy.Right);
		helper.LoadQueryOutput();
		NestedVisitEnsureStack(ndCopy.Left);
		helper.Call(XmlILMethods.EndCopy);
		helper.MarkLabel(label);
		iterCurr.Storage = StorageDescriptor.None();
		return ndCopy;
	}

	protected override QilNode VisitXsltCopyOf(QilUnary ndCopyOf)
	{
		helper.LoadQueryOutput();
		NestedVisitEnsureStack(ndCopyOf.Child);
		helper.Call(XmlILMethods.CopyOf);
		iterCurr.Storage = StorageDescriptor.None();
		return ndCopyOf;
	}

	protected override QilNode VisitXsltConvert(QilTargetType ndConv)
	{
		XmlQueryType xmlType = ndConv.Source.XmlType;
		XmlQueryType targetType = ndConv.TargetType;
		if (GetXsltConvertMethod(xmlType, targetType, out var meth))
		{
			NestedVisitEnsureStack(ndConv.Source);
		}
		else
		{
			NestedVisitEnsureStack(ndConv.Source, typeof(XPathItem), !xmlType.IsSingleton);
			GetXsltConvertMethod(xmlType.IsSingleton ? XmlQueryTypeFactory.Item : XmlQueryTypeFactory.ItemS, targetType, out meth);
		}
		if (meth != null)
		{
			helper.Call(meth);
		}
		iterCurr.Storage = StorageDescriptor.Stack(GetItemStorageType(targetType), !targetType.IsSingleton);
		return ndConv;
	}

	private bool GetXsltConvertMethod(XmlQueryType typSrc, XmlQueryType typDst, out MethodInfo meth)
	{
		meth = null;
		if ((object)typDst == XmlQueryTypeFactory.BooleanX)
		{
			if ((object)typSrc == XmlQueryTypeFactory.Item)
			{
				meth = XmlILMethods.ItemToBool;
			}
			else if ((object)typSrc == XmlQueryTypeFactory.ItemS)
			{
				meth = XmlILMethods.ItemsToBool;
			}
		}
		else if ((object)typDst == XmlQueryTypeFactory.DateTimeX)
		{
			if ((object)typSrc == XmlQueryTypeFactory.StringX)
			{
				meth = XmlILMethods.StrToDT;
			}
		}
		else if ((object)typDst == XmlQueryTypeFactory.DecimalX)
		{
			if ((object)typSrc == XmlQueryTypeFactory.DoubleX)
			{
				meth = XmlILMethods.DblToDec;
			}
		}
		else if ((object)typDst == XmlQueryTypeFactory.DoubleX)
		{
			if ((object)typSrc == XmlQueryTypeFactory.DecimalX)
			{
				meth = XmlILMethods.DecToDbl;
			}
			else if ((object)typSrc == XmlQueryTypeFactory.IntX)
			{
				meth = XmlILMethods.IntToDbl;
			}
			else if ((object)typSrc == XmlQueryTypeFactory.Item)
			{
				meth = XmlILMethods.ItemToDbl;
			}
			else if ((object)typSrc == XmlQueryTypeFactory.ItemS)
			{
				meth = XmlILMethods.ItemsToDbl;
			}
			else if ((object)typSrc == XmlQueryTypeFactory.LongX)
			{
				meth = XmlILMethods.LngToDbl;
			}
			else if ((object)typSrc == XmlQueryTypeFactory.StringX)
			{
				meth = XmlILMethods.StrToDbl;
			}
		}
		else if ((object)typDst == XmlQueryTypeFactory.IntX)
		{
			if ((object)typSrc == XmlQueryTypeFactory.DoubleX)
			{
				meth = XmlILMethods.DblToInt;
			}
		}
		else if ((object)typDst == XmlQueryTypeFactory.LongX)
		{
			if ((object)typSrc == XmlQueryTypeFactory.DoubleX)
			{
				meth = XmlILMethods.DblToLng;
			}
		}
		else if ((object)typDst == XmlQueryTypeFactory.NodeNotRtf)
		{
			if ((object)typSrc == XmlQueryTypeFactory.Item)
			{
				meth = XmlILMethods.ItemToNode;
			}
			else if ((object)typSrc == XmlQueryTypeFactory.ItemS)
			{
				meth = XmlILMethods.ItemsToNode;
			}
		}
		else if ((object)typDst == XmlQueryTypeFactory.NodeSDod || (object)typDst == XmlQueryTypeFactory.NodeNotRtfS)
		{
			if ((object)typSrc == XmlQueryTypeFactory.Item)
			{
				meth = XmlILMethods.ItemToNodes;
			}
			else if ((object)typSrc == XmlQueryTypeFactory.ItemS)
			{
				meth = XmlILMethods.ItemsToNodes;
			}
		}
		else if ((object)typDst == XmlQueryTypeFactory.StringX)
		{
			if ((object)typSrc == XmlQueryTypeFactory.DateTimeX)
			{
				meth = XmlILMethods.DTToStr;
			}
			else if ((object)typSrc == XmlQueryTypeFactory.DoubleX)
			{
				meth = XmlILMethods.DblToStr;
			}
			else if ((object)typSrc == XmlQueryTypeFactory.Item)
			{
				meth = XmlILMethods.ItemToStr;
			}
			else if ((object)typSrc == XmlQueryTypeFactory.ItemS)
			{
				meth = XmlILMethods.ItemsToStr;
			}
		}
		return meth != null;
	}

	private void SyncToNavigator(LocalBuilder locNav, QilNode ndCtxt)
	{
		helper.Emit(OpCodes.Ldloc, locNav);
		NestedVisitEnsureStack(ndCtxt);
		helper.CallSyncToNavigator();
		helper.Emit(OpCodes.Stloc, locNav);
	}

	private void CreateSimpleIterator(QilNode ndCtxt, string iterName, Type iterType, MethodInfo methCreate, MethodInfo methNext)
	{
		LocalBuilder localBuilder = helper.DeclareLocal(iterName, iterType);
		helper.Emit(OpCodes.Ldloca, localBuilder);
		NestedVisitEnsureStack(ndCtxt);
		helper.Call(methCreate);
		GenerateSimpleIterator(typeof(XPathNavigator), localBuilder, methNext);
	}

	private void CreateFilteredIterator(QilNode ndCtxt, string iterName, Type iterType, MethodInfo methCreate, MethodInfo methNext, XmlNodeKindFlags kinds, QilName ndName, TriState orSelf, QilNode ndEnd)
	{
		LocalBuilder localBuilder = helper.DeclareLocal(iterName, iterType);
		helper.Emit(OpCodes.Ldloca, localBuilder);
		NestedVisitEnsureStack(ndCtxt);
		LoadSelectFilter(kinds, ndName);
		if (orSelf != TriState.Unknown)
		{
			helper.LoadBoolean(orSelf == TriState.True);
		}
		if (ndEnd != null)
		{
			NestedVisitEnsureStack(ndEnd);
		}
		helper.Call(methCreate);
		GenerateSimpleIterator(typeof(XPathNavigator), localBuilder, methNext);
	}

	private void CreateContainerIterator(QilUnary ndDod, string iterName, Type iterType, MethodInfo methCreate, MethodInfo methNext, XmlNodeKindFlags kinds, QilName ndName, TriState orSelf)
	{
		LocalBuilder localBuilder = helper.DeclareLocal(iterName, iterType);
		QilLoop qilLoop = (QilLoop)ndDod.Child;
		helper.Emit(OpCodes.Ldloca, localBuilder);
		LoadSelectFilter(kinds, ndName);
		if (orSelf != TriState.Unknown)
		{
			helper.LoadBoolean(orSelf == TriState.True);
		}
		helper.Call(methCreate);
		Label label = helper.DefineLabel();
		StartNestedIterator(qilLoop, label);
		StartBinding(qilLoop.Variable);
		EndBinding(qilLoop.Variable);
		EndNestedIterator(qilLoop.Variable);
		iterCurr.Storage = iterNested.Storage;
		GenerateContainerIterator(ndDod, localBuilder, label, methNext, typeof(XPathNavigator));
	}

	private void GenerateSimpleIterator(Type itemStorageType, LocalBuilder locIter, MethodInfo methNext)
	{
		Label label = helper.DefineLabel();
		helper.MarkLabel(label);
		helper.Emit(OpCodes.Ldloca, locIter);
		helper.Call(methNext);
		helper.Emit(OpCodes.Brfalse, iterCurr.GetLabelNext());
		iterCurr.SetIterator(label, StorageDescriptor.Current(locIter, itemStorageType));
	}

	private void GenerateContainerIterator(QilNode nd, LocalBuilder locIter, Label lblOnEndNested, MethodInfo methNext, Type itemStorageType)
	{
		Label label = helper.DefineLabel();
		iterCurr.EnsureNoStackNoCache(nd.XmlType.IsNode ? "$$$navInput" : "$$$itemInput");
		helper.Emit(OpCodes.Ldloca, locIter);
		iterCurr.PushValue();
		helper.EmitUnconditionalBranch(OpCodes.Br, label);
		helper.MarkLabel(lblOnEndNested);
		helper.Emit(OpCodes.Ldloca, locIter);
		helper.Emit(OpCodes.Ldnull);
		helper.MarkLabel(label);
		helper.Call(methNext);
		if (nd.XmlType.IsSingleton)
		{
			helper.LoadInteger(1);
			helper.Emit(OpCodes.Beq, iterNested.GetLabelNext());
			iterCurr.Storage = StorageDescriptor.Current(locIter, itemStorageType);
		}
		else
		{
			helper.Emit(OpCodes.Switch, new Label[2]
			{
				iterCurr.GetLabelNext(),
				iterNested.GetLabelNext()
			});
			iterCurr.SetIterator(lblOnEndNested, StorageDescriptor.Current(locIter, itemStorageType));
		}
	}

	private GenerateNameType LoadNameAndType(XPathNodeType nodeType, QilNode ndName, bool isStart, bool callChk)
	{
		helper.LoadQueryOutput();
		GenerateNameType result = GenerateNameType.StackName;
		if (ndName.NodeType == QilNodeType.LiteralQName)
		{
			if (isStart || !callChk)
			{
				QilName qilName = ndName as QilName;
				string prefix = qilName.Prefix;
				string localName = qilName.LocalName;
				string namespaceUri = qilName.NamespaceUri;
				if (qilName.NamespaceUri.Length == 0)
				{
					helper.Emit(OpCodes.Ldstr, qilName.LocalName);
					return GenerateNameType.LiteralLocalName;
				}
				if (!ValidateNames.ValidateName(prefix, localName, namespaceUri, nodeType, ValidateNames.Flags.CheckPrefixMapping))
				{
					if (isStart)
					{
						helper.Emit(OpCodes.Ldstr, localName);
						helper.Emit(OpCodes.Ldstr, namespaceUri);
						helper.Construct(XmlILConstructors.QName);
						result = GenerateNameType.QName;
					}
				}
				else
				{
					helper.Emit(OpCodes.Ldstr, prefix);
					helper.Emit(OpCodes.Ldstr, localName);
					helper.Emit(OpCodes.Ldstr, namespaceUri);
					result = GenerateNameType.LiteralName;
				}
			}
		}
		else if (isStart)
		{
			if (ndName.NodeType == QilNodeType.NameOf)
			{
				NestedVisitEnsureStack((ndName as QilUnary).Child);
				result = GenerateNameType.CopiedName;
			}
			else if (ndName.NodeType == QilNodeType.StrParseQName)
			{
				VisitStrParseQName(ndName as QilBinary, preservePrefix: true);
				result = (((ndName as QilBinary).Right.XmlType.TypeCode != XmlTypeCode.String) ? GenerateNameType.TagNameAndMappings : GenerateNameType.TagNameAndNamespace);
			}
			else
			{
				NestedVisitEnsureStack(ndName);
				result = GenerateNameType.QName;
			}
		}
		return result;
	}

	private bool TryZeroCompare(QilNodeType relOp, QilNode ndFirst, QilNode ndSecond)
	{
		switch (ndFirst.NodeType)
		{
		case QilNodeType.LiteralInt64:
			if ((int)(QilLiteral)ndFirst != 0)
			{
				return false;
			}
			break;
		case QilNodeType.LiteralInt32:
			if ((int)(QilLiteral)ndFirst != 0)
			{
				return false;
			}
			break;
		case QilNodeType.True:
			relOp = ((relOp == QilNodeType.Eq) ? QilNodeType.Ne : QilNodeType.Eq);
			break;
		default:
			return false;
		case QilNodeType.False:
			break;
		}
		NestedVisitEnsureStack(ndSecond);
		ZeroCompare(relOp, ndSecond.XmlType.TypeCode == XmlTypeCode.Boolean);
		return true;
	}

	private bool TryNameCompare(QilNodeType relOp, QilNode ndFirst, QilNode ndSecond)
	{
		if (ndFirst.NodeType == QilNodeType.NameOf)
		{
			QilNodeType nodeType = ndSecond.NodeType;
			if (nodeType == QilNodeType.LiteralQName || nodeType == QilNodeType.NameOf)
			{
				helper.LoadQueryRuntime();
				NestedVisitEnsureStack((ndFirst as QilUnary).Child);
				if (ndSecond.NodeType == QilNodeType.LiteralQName)
				{
					QilName qilName = ndSecond as QilName;
					helper.LoadInteger(helper.StaticData.DeclareName(qilName.LocalName));
					helper.LoadInteger(helper.StaticData.DeclareName(qilName.NamespaceUri));
					helper.Call(XmlILMethods.QNameEqualLit);
				}
				else
				{
					NestedVisitEnsureStack(ndSecond);
					helper.Call(XmlILMethods.QNameEqualNav);
				}
				ZeroCompare((relOp == QilNodeType.Eq) ? QilNodeType.Ne : QilNodeType.Eq, isBoolVal: true);
				return true;
			}
		}
		return false;
	}

	private void ClrCompare(QilNodeType relOp, XmlTypeCode code)
	{
		OpCode opcode;
		switch (iterCurr.CurrentBranchingContext)
		{
		case BranchingContext.OnFalse:
			opcode = ((code == XmlTypeCode.Double || code == XmlTypeCode.Float) ? (relOp switch
			{
				QilNodeType.Gt => OpCodes.Ble_Un, 
				QilNodeType.Ge => OpCodes.Blt_Un, 
				QilNodeType.Lt => OpCodes.Bge_Un, 
				QilNodeType.Le => OpCodes.Bgt_Un, 
				QilNodeType.Eq => OpCodes.Bne_Un, 
				QilNodeType.Ne => OpCodes.Beq, 
				_ => OpCodes.Nop, 
			}) : (relOp switch
			{
				QilNodeType.Gt => OpCodes.Ble, 
				QilNodeType.Ge => OpCodes.Blt, 
				QilNodeType.Lt => OpCodes.Bge, 
				QilNodeType.Le => OpCodes.Bgt, 
				QilNodeType.Eq => OpCodes.Bne_Un, 
				QilNodeType.Ne => OpCodes.Beq, 
				_ => OpCodes.Nop, 
			}));
			helper.Emit(opcode, iterCurr.LabelBranch);
			iterCurr.Storage = StorageDescriptor.None();
			return;
		case BranchingContext.OnTrue:
			opcode = relOp switch
			{
				QilNodeType.Gt => OpCodes.Bgt, 
				QilNodeType.Ge => OpCodes.Bge, 
				QilNodeType.Lt => OpCodes.Blt, 
				QilNodeType.Le => OpCodes.Ble, 
				QilNodeType.Eq => OpCodes.Beq, 
				QilNodeType.Ne => OpCodes.Bne_Un, 
				_ => OpCodes.Nop, 
			};
			helper.Emit(opcode, iterCurr.LabelBranch);
			iterCurr.Storage = StorageDescriptor.None();
			return;
		}
		Label label;
		switch (relOp)
		{
		case QilNodeType.Gt:
			helper.Emit(OpCodes.Cgt);
			break;
		case QilNodeType.Lt:
			helper.Emit(OpCodes.Clt);
			break;
		case QilNodeType.Eq:
			helper.Emit(OpCodes.Ceq);
			break;
		case QilNodeType.Ge:
			opcode = OpCodes.Bge_S;
			goto IL_0207;
		case QilNodeType.Le:
			opcode = OpCodes.Ble_S;
			goto IL_0207;
		case QilNodeType.Ne:
			opcode = OpCodes.Bne_Un_S;
			goto IL_0207;
		default:
			{
				opcode = OpCodes.Nop;
				goto IL_0207;
			}
			IL_0207:
			label = helper.DefineLabel();
			helper.Emit(opcode, label);
			helper.ConvBranchToBool(label, isTrueBranch: true);
			break;
		}
		iterCurr.Storage = StorageDescriptor.Stack(typeof(bool), isCached: false);
	}

	private void ZeroCompare(QilNodeType relOp, bool isBoolVal)
	{
		switch (iterCurr.CurrentBranchingContext)
		{
		case BranchingContext.OnTrue:
			helper.Emit((relOp == QilNodeType.Eq) ? OpCodes.Brfalse : OpCodes.Brtrue, iterCurr.LabelBranch);
			iterCurr.Storage = StorageDescriptor.None();
			return;
		case BranchingContext.OnFalse:
			helper.Emit((relOp == QilNodeType.Eq) ? OpCodes.Brtrue : OpCodes.Brfalse, iterCurr.LabelBranch);
			iterCurr.Storage = StorageDescriptor.None();
			return;
		}
		if (!isBoolVal || relOp == QilNodeType.Eq)
		{
			Label label = helper.DefineLabel();
			helper.Emit((relOp == QilNodeType.Eq) ? OpCodes.Brfalse : OpCodes.Brtrue, label);
			helper.ConvBranchToBool(label, isTrueBranch: true);
		}
		iterCurr.Storage = StorageDescriptor.Stack(typeof(bool), isCached: false);
	}

	private void StartWriterLoop(QilNode nd, out bool hasOnEnd, out Label lblOnEnd)
	{
		XmlILConstructInfo xmlILConstructInfo = XmlILConstructInfo.Read(nd);
		hasOnEnd = false;
		lblOnEnd = default(Label);
		if (xmlILConstructInfo.PushToWriterLast && !nd.XmlType.IsSingleton && !iterCurr.HasLabelNext)
		{
			hasOnEnd = true;
			lblOnEnd = helper.DefineLabel();
			iterCurr.SetIterator(lblOnEnd, StorageDescriptor.None());
		}
	}

	private void EndWriterLoop(QilNode nd, bool hasOnEnd, Label lblOnEnd)
	{
		if (XmlILConstructInfo.Read(nd).PushToWriterLast)
		{
			iterCurr.Storage = StorageDescriptor.None();
			if (!nd.XmlType.IsSingleton && hasOnEnd)
			{
				iterCurr.LoopToEnd(lblOnEnd);
			}
		}
	}

	private bool MightHaveNamespacesAfterAttributes(XmlILConstructInfo info)
	{
		if (info != null)
		{
			info = info.ParentElementInfo;
		}
		return info?.MightHaveNamespacesAfterAttributes ?? true;
	}

	private bool ElementCachesAttributes(XmlILConstructInfo info)
	{
		if (!info.MightHaveDuplicateAttributes)
		{
			return info.MightHaveNamespacesAfterAttributes;
		}
		return true;
	}

	private void BeforeStartChecks(QilNode ndCtor)
	{
		switch (XmlILConstructInfo.Read(ndCtor).InitialStates)
		{
		case PossibleXmlStates.WithinSequence:
			helper.CallStartTree(QilConstructorToNodeType(ndCtor.NodeType));
			break;
		case PossibleXmlStates.EnumAttrs:
		{
			QilNodeType nodeType = ndCtor.NodeType;
			if (nodeType == QilNodeType.ElementCtor || (uint)(nodeType - 83) <= 3u)
			{
				helper.CallStartElementContent();
			}
			break;
		}
		}
	}

	private void AfterEndChecks(QilNode ndCtor)
	{
		if (XmlILConstructInfo.Read(ndCtor).FinalStates == PossibleXmlStates.WithinSequence)
		{
			helper.CallEndTree();
		}
	}

	private bool CheckWithinContent(XmlILConstructInfo info)
	{
		PossibleXmlStates initialStates = info.InitialStates;
		if ((uint)(initialStates - 1) <= 2u)
		{
			return false;
		}
		return true;
	}

	private bool CheckEnumAttrs(XmlILConstructInfo info)
	{
		PossibleXmlStates initialStates = info.InitialStates;
		if ((uint)(initialStates - 1) <= 1u)
		{
			return false;
		}
		return true;
	}

	private XPathNodeType QilXmlToXPathNodeType(XmlNodeKindFlags xmlTypes)
	{
		return xmlTypes switch
		{
			XmlNodeKindFlags.Element => XPathNodeType.Element, 
			XmlNodeKindFlags.Attribute => XPathNodeType.Attribute, 
			XmlNodeKindFlags.Text => XPathNodeType.Text, 
			XmlNodeKindFlags.Comment => XPathNodeType.Comment, 
			_ => XPathNodeType.ProcessingInstruction, 
		};
	}

	private XPathNodeType QilConstructorToNodeType(QilNodeType typ)
	{
		return typ switch
		{
			QilNodeType.DocumentCtor => XPathNodeType.Root, 
			QilNodeType.ElementCtor => XPathNodeType.Element, 
			QilNodeType.TextCtor => XPathNodeType.Text, 
			QilNodeType.RawTextCtor => XPathNodeType.Text, 
			QilNodeType.PICtor => XPathNodeType.ProcessingInstruction, 
			QilNodeType.CommentCtor => XPathNodeType.Comment, 
			QilNodeType.AttributeCtor => XPathNodeType.Attribute, 
			QilNodeType.NamespaceDecl => XPathNodeType.Namespace, 
			_ => XPathNodeType.All, 
		};
	}

	private void LoadSelectFilter(XmlNodeKindFlags xmlTypes, QilName ndName)
	{
		if (ndName != null)
		{
			helper.CallGetNameFilter(helper.StaticData.DeclareNameFilter(ndName.LocalName, ndName.NamespaceUri));
		}
		else if (IsNodeTypeUnion(xmlTypes))
		{
			if ((xmlTypes & XmlNodeKindFlags.Attribute) != 0)
			{
				helper.CallGetTypeFilter(XPathNodeType.All);
			}
			else
			{
				helper.CallGetTypeFilter(XPathNodeType.Attribute);
			}
		}
		else
		{
			helper.CallGetTypeFilter(QilXmlToXPathNodeType(xmlTypes));
		}
	}

	private static bool IsNodeTypeUnion(XmlNodeKindFlags xmlTypes)
	{
		return (xmlTypes & (xmlTypes - 1)) != 0;
	}

	private void StartNestedIterator(QilNode nd)
	{
		IteratorDescriptor iteratorDescriptor = iterCurr;
		if (iteratorDescriptor == null)
		{
			iterCurr = new IteratorDescriptor(helper);
		}
		else
		{
			iterCurr = new IteratorDescriptor(iteratorDescriptor);
		}
		iterNested = null;
	}

	private void StartNestedIterator(QilNode nd, Label lblOnEnd)
	{
		StartNestedIterator(nd);
		iterCurr.SetIterator(lblOnEnd, StorageDescriptor.None());
	}

	private void EndNestedIterator(QilNode nd)
	{
		if (iterCurr.IsBranching && iterCurr.Storage.Location != 0)
		{
			iterCurr.EnsureItemStorageType(nd.XmlType, typeof(bool));
			iterCurr.EnsureStackNoCache();
			if (iterCurr.CurrentBranchingContext == BranchingContext.OnTrue)
			{
				helper.Emit(OpCodes.Brtrue, iterCurr.LabelBranch);
			}
			else
			{
				helper.Emit(OpCodes.Brfalse, iterCurr.LabelBranch);
			}
			iterCurr.Storage = StorageDescriptor.None();
		}
		iterNested = iterCurr;
		iterCurr = iterCurr.ParentIterator;
	}

	private void NestedVisit(QilNode nd, Type itemStorageType, bool isCached)
	{
		if (XmlILConstructInfo.Read(nd).PushToWriterLast)
		{
			StartNestedIterator(nd);
			Visit(nd);
			EndNestedIterator(nd);
			iterCurr.Storage = StorageDescriptor.None();
		}
		else if (!isCached && nd.XmlType.IsSingleton)
		{
			StartNestedIterator(nd);
			Visit(nd);
			iterCurr.EnsureNoCache();
			iterCurr.EnsureItemStorageType(nd.XmlType, itemStorageType);
			EndNestedIterator(nd);
			iterCurr.Storage = iterNested.Storage;
		}
		else
		{
			NestedVisitEnsureCache(nd, itemStorageType);
		}
	}

	private void NestedVisit(QilNode nd)
	{
		NestedVisit(nd, GetItemStorageType(nd), !nd.XmlType.IsSingleton);
	}

	private void NestedVisit(QilNode nd, Label lblOnEnd)
	{
		StartNestedIterator(nd, lblOnEnd);
		Visit(nd);
		iterCurr.EnsureNoCache();
		iterCurr.EnsureItemStorageType(nd.XmlType, GetItemStorageType(nd));
		EndNestedIterator(nd);
		iterCurr.Storage = iterNested.Storage;
	}

	private void NestedVisitEnsureStack(QilNode nd)
	{
		NestedVisit(nd);
		iterCurr.EnsureStack();
	}

	private void NestedVisitEnsureStack(QilNode ndLeft, QilNode ndRight)
	{
		NestedVisitEnsureStack(ndLeft);
		NestedVisitEnsureStack(ndRight);
	}

	private void NestedVisitEnsureStack(QilNode nd, Type itemStorageType, bool isCached)
	{
		NestedVisit(nd, itemStorageType, isCached);
		iterCurr.EnsureStack();
	}

	private void NestedVisitEnsureLocal(QilNode nd, LocalBuilder loc)
	{
		NestedVisit(nd);
		iterCurr.EnsureLocal(loc);
	}

	private void NestedVisitWithBranch(QilNode nd, BranchingContext brctxt, Label lblBranch)
	{
		StartNestedIterator(nd);
		iterCurr.SetBranching(brctxt, lblBranch);
		Visit(nd);
		EndNestedIterator(nd);
		iterCurr.Storage = StorageDescriptor.None();
	}

	private void NestedVisitEnsureCache(QilNode nd, Type itemStorageType)
	{
		bool flag = CachesResult(nd);
		Label lblOnEnd = helper.DefineLabel();
		if (flag)
		{
			StartNestedIterator(nd);
			Visit(nd);
			EndNestedIterator(nd);
			iterCurr.Storage = iterNested.Storage;
			if (iterCurr.Storage.ItemStorageType == itemStorageType)
			{
				return;
			}
			if (iterCurr.Storage.ItemStorageType == typeof(XPathNavigator) || itemStorageType == typeof(XPathNavigator))
			{
				iterCurr.EnsureItemStorageType(nd.XmlType, itemStorageType);
				return;
			}
			iterCurr.EnsureNoStack("$$$cacheResult");
		}
		Type type = ((GetItemStorageType(nd) == typeof(XPathNavigator)) ? typeof(XPathNavigator) : itemStorageType);
		XmlILStorageMethods xmlILStorageMethods = XmlILMethods.StorageMethods[type];
		LocalBuilder localBuilder = helper.DeclareLocal("$$$cache", xmlILStorageMethods.SeqType);
		helper.Emit(OpCodes.Ldloc, localBuilder);
		if (nd.XmlType.IsSingleton)
		{
			NestedVisitEnsureStack(nd, type, isCached: false);
			helper.CallToken(xmlILStorageMethods.SeqReuseSgl);
			helper.Emit(OpCodes.Stloc, localBuilder);
		}
		else
		{
			helper.CallToken(xmlILStorageMethods.SeqReuse);
			helper.Emit(OpCodes.Stloc, localBuilder);
			helper.Emit(OpCodes.Ldloc, localBuilder);
			StartNestedIterator(nd, lblOnEnd);
			if (flag)
			{
				iterCurr.Storage = iterCurr.ParentIterator.Storage;
			}
			else
			{
				Visit(nd);
			}
			iterCurr.EnsureItemStorageType(nd.XmlType, type);
			iterCurr.EnsureStackNoCache();
			helper.Call(xmlILStorageMethods.SeqAdd);
			helper.Emit(OpCodes.Ldloc, localBuilder);
			iterCurr.LoopToEnd(lblOnEnd);
			EndNestedIterator(nd);
			helper.Emit(OpCodes.Pop);
		}
		iterCurr.Storage = StorageDescriptor.Local(localBuilder, itemStorageType, isCached: true);
	}

	private bool CachesResult(QilNode nd)
	{
		switch (nd.NodeType)
		{
		case QilNodeType.Let:
		case QilNodeType.Parameter:
		case QilNodeType.Invoke:
		case QilNodeType.XsltInvokeLateBound:
		case QilNodeType.XsltInvokeEarlyBound:
			return !nd.XmlType.IsSingleton;
		case QilNodeType.Filter:
		{
			OptimizerPatterns optimizerPatterns = OptimizerPatterns.Read(nd);
			return optimizerPatterns.MatchesPattern(OptimizerPatternName.EqualityIndex);
		}
		case QilNodeType.DocOrderDistinct:
		{
			if (nd.XmlType.IsSingleton)
			{
				return false;
			}
			OptimizerPatterns optimizerPatterns = OptimizerPatterns.Read(nd);
			if (!optimizerPatterns.MatchesPattern(OptimizerPatternName.JoinAndDod))
			{
				return !optimizerPatterns.MatchesPattern(OptimizerPatternName.DodReverse);
			}
			return false;
		}
		case QilNodeType.TypeAssert:
		{
			QilTargetType qilTargetType = (QilTargetType)nd;
			if (CachesResult(qilTargetType.Source))
			{
				return GetItemStorageType(qilTargetType.Source) == GetItemStorageType(qilTargetType);
			}
			return false;
		}
		default:
			return false;
		}
	}

	private Type GetStorageType(QilNode nd)
	{
		return XmlILTypeHelper.GetStorageType(nd.XmlType);
	}

	private Type GetStorageType(XmlQueryType typ)
	{
		return XmlILTypeHelper.GetStorageType(typ);
	}

	private Type GetItemStorageType(QilNode nd)
	{
		return XmlILTypeHelper.GetStorageType(nd.XmlType.Prime);
	}

	private Type GetItemStorageType(XmlQueryType typ)
	{
		return XmlILTypeHelper.GetStorageType(typ.Prime);
	}
}
