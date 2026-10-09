using System.Diagnostics;
using System.Diagnostics.SymbolStore;
using System.Reflection;
using System.Reflection.Emit;
using System.Xml.Schema;
using System.Xml.XPath;
using System.Xml.Xsl.Qil;
using System.Xml.Xsl.Runtime;

namespace System.Xml.Xsl.IlGen;

internal class GenerateHelper
{
	private MethodBase methInfo;

	private ILGenerator ilgen;

	private LocalBuilder locXOut;

	private XmlILModule module;

	private bool isDebug;

	private bool initWriters;

	private StaticDataManager staticData;

	private ISourceLineInfo lastSourceInfo;

	private MethodInfo methSyncToNav;

	private string lastUriString;

	private string lastFileName;

	public StaticDataManager StaticData => staticData;

	public GenerateHelper(XmlILModule module, bool isDebug)
	{
		this.isDebug = isDebug;
		this.module = module;
		staticData = new StaticDataManager();
	}

	public void MethodBegin(MethodBase methInfo, ISourceLineInfo sourceInfo, bool initWriters)
	{
		this.methInfo = methInfo;
		ilgen = XmlILModule.DefineMethodBody(methInfo);
		lastSourceInfo = null;
		if (isDebug)
		{
			DebugStartScope();
			if (sourceInfo != null)
			{
				MarkSequencePoint(sourceInfo);
				Emit(OpCodes.Nop);
			}
		}
		else if (module.EmitSymbols && sourceInfo != null)
		{
			MarkSequencePoint(sourceInfo);
			lastSourceInfo = null;
		}
		this.initWriters = false;
		if (initWriters)
		{
			EnsureWriter();
			LoadQueryRuntime();
			Call(XmlILMethods.GetOutput);
			Emit(OpCodes.Stloc, locXOut);
		}
	}

	public void MethodEnd()
	{
		Emit(OpCodes.Ret);
		if (isDebug)
		{
			DebugEndScope();
		}
	}

	public void CallSyncToNavigator()
	{
		if (methSyncToNav == null)
		{
			methSyncToNav = module.FindMethod("SyncToNavigator");
		}
		Call(methSyncToNav);
	}

	public void LoadInteger(int intVal)
	{
		if (intVal >= -1 && intVal < 9)
		{
			OpCode opcode;
			switch (intVal)
			{
			default:
				return;
			case -1:
				opcode = OpCodes.Ldc_I4_M1;
				break;
			case 0:
				opcode = OpCodes.Ldc_I4_0;
				break;
			case 1:
				opcode = OpCodes.Ldc_I4_1;
				break;
			case 2:
				opcode = OpCodes.Ldc_I4_2;
				break;
			case 3:
				opcode = OpCodes.Ldc_I4_3;
				break;
			case 4:
				opcode = OpCodes.Ldc_I4_4;
				break;
			case 5:
				opcode = OpCodes.Ldc_I4_5;
				break;
			case 6:
				opcode = OpCodes.Ldc_I4_6;
				break;
			case 7:
				opcode = OpCodes.Ldc_I4_7;
				break;
			case 8:
				opcode = OpCodes.Ldc_I4_8;
				break;
			}
			Emit(opcode);
		}
		else if (intVal >= -128 && intVal <= 127)
		{
			Emit(OpCodes.Ldc_I4_S, (sbyte)intVal);
		}
		else
		{
			Emit(OpCodes.Ldc_I4, intVal);
		}
	}

	public void LoadBoolean(bool boolVal)
	{
		Emit(boolVal ? OpCodes.Ldc_I4_1 : OpCodes.Ldc_I4_0);
	}

	public void LoadType(Type clrTyp)
	{
		Emit(OpCodes.Ldtoken, clrTyp);
		Call(XmlILMethods.GetTypeFromHandle);
	}

	public LocalBuilder DeclareLocal(string name, Type type)
	{
		return ilgen.DeclareLocal(type);
	}

	public void LoadQueryRuntime()
	{
		Emit(OpCodes.Ldarg_0);
	}

	public void LoadQueryContext()
	{
		Emit(OpCodes.Ldarg_0);
		Call(XmlILMethods.Context);
	}

	public void LoadXsltLibrary()
	{
		Emit(OpCodes.Ldarg_0);
		Call(XmlILMethods.XsltLib);
	}

	public void LoadQueryOutput()
	{
		Emit(OpCodes.Ldloc, locXOut);
	}

	public void LoadParameter(int paramPos)
	{
		switch (paramPos)
		{
		case 0:
			Emit(OpCodes.Ldarg_0);
			return;
		case 1:
			Emit(OpCodes.Ldarg_1);
			return;
		case 2:
			Emit(OpCodes.Ldarg_2);
			return;
		case 3:
			Emit(OpCodes.Ldarg_3);
			return;
		}
		if (paramPos <= 255)
		{
			Emit(OpCodes.Ldarg_S, (byte)paramPos);
			return;
		}
		if (paramPos <= 65535)
		{
			Emit(OpCodes.Ldarg, paramPos);
			return;
		}
		throw new XslTransformException("Functions may not have more than 65535 parameters.");
	}

	public void SetParameter(object paramId)
	{
		int num = (int)paramId;
		if (num <= 255)
		{
			Emit(OpCodes.Starg_S, (byte)num);
			return;
		}
		if (num <= 65535)
		{
			Emit(OpCodes.Starg, num);
			return;
		}
		throw new XslTransformException("Functions may not have more than 65535 parameters.");
	}

	public void BranchAndMark(Label lblBranch, Label lblMark)
	{
		if (!lblBranch.Equals(lblMark))
		{
			EmitUnconditionalBranch(OpCodes.Br, lblBranch);
		}
		MarkLabel(lblMark);
	}

	public void TestAndBranch(int i4, Label lblBranch, OpCode opcodeBranch)
	{
		if (i4 != 0)
		{
			goto IL_007f;
		}
		if (opcodeBranch.Value == OpCodes.Beq.Value)
		{
			opcodeBranch = OpCodes.Brfalse;
		}
		else if (opcodeBranch.Value == OpCodes.Beq_S.Value)
		{
			opcodeBranch = OpCodes.Brfalse_S;
		}
		else if (opcodeBranch.Value == OpCodes.Bne_Un.Value)
		{
			opcodeBranch = OpCodes.Brtrue;
		}
		else
		{
			if (opcodeBranch.Value != OpCodes.Bne_Un_S.Value)
			{
				goto IL_007f;
			}
			opcodeBranch = OpCodes.Brtrue_S;
		}
		goto IL_0086;
		IL_0086:
		Emit(opcodeBranch, lblBranch);
		return;
		IL_007f:
		LoadInteger(i4);
		goto IL_0086;
	}

	public void ConvBranchToBool(Label lblBranch, bool isTrueBranch)
	{
		Label label = DefineLabel();
		Emit(isTrueBranch ? OpCodes.Ldc_I4_0 : OpCodes.Ldc_I4_1);
		EmitUnconditionalBranch(OpCodes.Br_S, label);
		MarkLabel(lblBranch);
		Emit(isTrueBranch ? OpCodes.Ldc_I4_1 : OpCodes.Ldc_I4_0);
		MarkLabel(label);
	}

	public void TailCall(MethodInfo meth)
	{
		Emit(OpCodes.Tailcall);
		Call(meth);
		Emit(OpCodes.Ret);
	}

	[Conditional("DEBUG")]
	private void TraceCall(OpCode opcode, MethodInfo meth)
	{
	}

	public void Call(MethodInfo meth)
	{
		OpCode opcode = ((meth.IsVirtual || meth.IsAbstract) ? OpCodes.Callvirt : OpCodes.Call);
		ilgen.Emit(opcode, meth);
		if (lastSourceInfo != null)
		{
			MarkSequencePoint(SourceLineInfo.NoSource);
		}
	}

	public void CallToken(MethodInfo meth)
	{
		MethodBuilder methodBuilder = methInfo as MethodBuilder;
		if (methodBuilder != null)
		{
			OpCode opcode = ((meth.IsVirtual || meth.IsAbstract) ? OpCodes.Callvirt : OpCodes.Call);
			ilgen.Emit(opcode, ((ModuleBuilder)methodBuilder.GetModule()).GetMethodToken(meth).Token);
			if (lastSourceInfo != null)
			{
				MarkSequencePoint(SourceLineInfo.NoSource);
			}
		}
		else
		{
			Call(meth);
		}
	}

	public void Construct(ConstructorInfo constr)
	{
		Emit(OpCodes.Newobj, constr);
	}

	public void CallConcatStrings(int cStrings)
	{
		switch (cStrings)
		{
		case 0:
			Emit(OpCodes.Ldstr, "");
			break;
		case 2:
			Call(XmlILMethods.StrCat2);
			break;
		case 3:
			Call(XmlILMethods.StrCat3);
			break;
		case 4:
			Call(XmlILMethods.StrCat4);
			break;
		case 1:
			break;
		}
	}

	public void TreatAs(Type clrTypeSrc, Type clrTypeDst)
	{
		if (!(clrTypeSrc == clrTypeDst))
		{
			if (clrTypeSrc.IsValueType)
			{
				Emit(OpCodes.Box, clrTypeSrc);
			}
			else if (clrTypeDst.IsValueType)
			{
				Emit(OpCodes.Unbox, clrTypeDst);
				Emit(OpCodes.Ldobj, clrTypeDst);
			}
			else if (clrTypeDst != typeof(object))
			{
				Emit(OpCodes.Castclass, clrTypeDst);
			}
		}
	}

	public void ConstructLiteralDecimal(decimal dec)
	{
		if (dec >= -2147483648m && dec <= 2147483647m && decimal.Truncate(dec) == dec)
		{
			LoadInteger((int)dec);
			Construct(XmlILConstructors.DecFromInt32);
			return;
		}
		int[] bits = decimal.GetBits(dec);
		LoadInteger(bits[0]);
		LoadInteger(bits[1]);
		LoadInteger(bits[2]);
		LoadBoolean(bits[3] < 0);
		LoadInteger(bits[3] >> 16);
		Construct(XmlILConstructors.DecFromParts);
	}

	public void ConstructLiteralQName(string localName, string namespaceName)
	{
		Emit(OpCodes.Ldstr, localName);
		Emit(OpCodes.Ldstr, namespaceName);
		Construct(XmlILConstructors.QName);
	}

	public void CallArithmeticOp(QilNodeType opType, XmlTypeCode code)
	{
		MethodInfo meth = null;
		switch (code)
		{
		case XmlTypeCode.Float:
		case XmlTypeCode.Double:
		case XmlTypeCode.Integer:
		case XmlTypeCode.Int:
			switch (opType)
			{
			case QilNodeType.Add:
				Emit(OpCodes.Add);
				break;
			case QilNodeType.Subtract:
				Emit(OpCodes.Sub);
				break;
			case QilNodeType.Multiply:
				Emit(OpCodes.Mul);
				break;
			case QilNodeType.Divide:
				Emit(OpCodes.Div);
				break;
			case QilNodeType.Modulo:
				Emit(OpCodes.Rem);
				break;
			case QilNodeType.Negate:
				Emit(OpCodes.Neg);
				break;
			}
			break;
		case XmlTypeCode.Decimal:
			switch (opType)
			{
			case QilNodeType.Add:
				meth = XmlILMethods.DecAdd;
				break;
			case QilNodeType.Subtract:
				meth = XmlILMethods.DecSub;
				break;
			case QilNodeType.Multiply:
				meth = XmlILMethods.DecMul;
				break;
			case QilNodeType.Divide:
				meth = XmlILMethods.DecDiv;
				break;
			case QilNodeType.Modulo:
				meth = XmlILMethods.DecRem;
				break;
			case QilNodeType.Negate:
				meth = XmlILMethods.DecNeg;
				break;
			}
			Call(meth);
			break;
		}
	}

	public void CallCompareEquals(XmlTypeCode code)
	{
		MethodInfo meth = null;
		switch (code)
		{
		case XmlTypeCode.String:
			meth = XmlILMethods.StrEq;
			break;
		case XmlTypeCode.QName:
			meth = XmlILMethods.QNameEq;
			break;
		case XmlTypeCode.Decimal:
			meth = XmlILMethods.DecEq;
			break;
		}
		Call(meth);
	}

	public void CallCompare(XmlTypeCode code)
	{
		MethodInfo meth = null;
		switch (code)
		{
		case XmlTypeCode.String:
			meth = XmlILMethods.StrCmp;
			break;
		case XmlTypeCode.Decimal:
			meth = XmlILMethods.DecCmp;
			break;
		}
		Call(meth);
	}

	public void CallStartRtfConstruction(string baseUri)
	{
		EnsureWriter();
		LoadQueryRuntime();
		Emit(OpCodes.Ldstr, baseUri);
		Emit(OpCodes.Ldloca, locXOut);
		Call(XmlILMethods.StartRtfConstr);
	}

	public void CallEndRtfConstruction()
	{
		LoadQueryRuntime();
		Emit(OpCodes.Ldloca, locXOut);
		Call(XmlILMethods.EndRtfConstr);
	}

	public void CallStartSequenceConstruction()
	{
		EnsureWriter();
		LoadQueryRuntime();
		Emit(OpCodes.Ldloca, locXOut);
		Call(XmlILMethods.StartSeqConstr);
	}

	public void CallEndSequenceConstruction()
	{
		LoadQueryRuntime();
		Emit(OpCodes.Ldloca, locXOut);
		Call(XmlILMethods.EndSeqConstr);
	}

	public void CallGetEarlyBoundObject(int idxObj, Type clrType)
	{
		LoadQueryRuntime();
		LoadInteger(idxObj);
		Call(XmlILMethods.GetEarly);
		TreatAs(typeof(object), clrType);
	}

	public void CallGetAtomizedName(int idxName)
	{
		LoadQueryRuntime();
		LoadInteger(idxName);
		Call(XmlILMethods.GetAtomizedName);
	}

	public void CallGetNameFilter(int idxFilter)
	{
		LoadQueryRuntime();
		LoadInteger(idxFilter);
		Call(XmlILMethods.GetNameFilter);
	}

	public void CallGetTypeFilter(XPathNodeType nodeType)
	{
		LoadQueryRuntime();
		LoadInteger((int)nodeType);
		Call(XmlILMethods.GetTypeFilter);
	}

	public void CallParseTagName(GenerateNameType nameType)
	{
		if (nameType == GenerateNameType.TagNameAndMappings)
		{
			Call(XmlILMethods.TagAndMappings);
		}
		else
		{
			Call(XmlILMethods.TagAndNamespace);
		}
	}

	public void CallGetGlobalValue(int idxValue, Type clrType)
	{
		LoadQueryRuntime();
		LoadInteger(idxValue);
		Call(XmlILMethods.GetGlobalValue);
		TreatAs(typeof(object), clrType);
	}

	public void CallSetGlobalValue(Type clrType)
	{
		TreatAs(clrType, typeof(object));
		Call(XmlILMethods.SetGlobalValue);
	}

	public void CallGetCollation(int idxName)
	{
		LoadQueryRuntime();
		LoadInteger(idxName);
		Call(XmlILMethods.GetCollation);
	}

	private void EnsureWriter()
	{
		if (!initWriters)
		{
			locXOut = DeclareLocal("$$$xwrtChk", typeof(XmlQueryOutput));
			initWriters = true;
		}
	}

	public void CallGetParameter(string localName, string namespaceUri)
	{
		LoadQueryContext();
		Emit(OpCodes.Ldstr, localName);
		Emit(OpCodes.Ldstr, namespaceUri);
		Call(XmlILMethods.GetParam);
	}

	public void CallStartTree(XPathNodeType rootType)
	{
		LoadQueryOutput();
		LoadInteger((int)rootType);
		Call(XmlILMethods.StartTree);
	}

	public void CallEndTree()
	{
		LoadQueryOutput();
		Call(XmlILMethods.EndTree);
	}

	public void CallWriteStartRoot()
	{
		LoadQueryOutput();
		Call(XmlILMethods.StartRoot);
	}

	public void CallWriteEndRoot()
	{
		LoadQueryOutput();
		Call(XmlILMethods.EndRoot);
	}

	public void CallWriteStartElement(GenerateNameType nameType, bool callChk)
	{
		MethodInfo meth = null;
		if (callChk)
		{
			switch (nameType)
			{
			case GenerateNameType.LiteralLocalName:
				meth = XmlILMethods.StartElemLocName;
				break;
			case GenerateNameType.LiteralName:
				meth = XmlILMethods.StartElemLitName;
				break;
			case GenerateNameType.CopiedName:
				meth = XmlILMethods.StartElemCopyName;
				break;
			case GenerateNameType.TagNameAndMappings:
				meth = XmlILMethods.StartElemMapName;
				break;
			case GenerateNameType.TagNameAndNamespace:
				meth = XmlILMethods.StartElemNmspName;
				break;
			case GenerateNameType.QName:
				meth = XmlILMethods.StartElemQName;
				break;
			}
		}
		else
		{
			switch (nameType)
			{
			case GenerateNameType.LiteralLocalName:
				meth = XmlILMethods.StartElemLocNameUn;
				break;
			case GenerateNameType.LiteralName:
				meth = XmlILMethods.StartElemLitNameUn;
				break;
			}
		}
		Call(meth);
	}

	public void CallWriteEndElement(GenerateNameType nameType, bool callChk)
	{
		MethodInfo meth = null;
		if (callChk)
		{
			meth = XmlILMethods.EndElemStackName;
		}
		else
		{
			switch (nameType)
			{
			case GenerateNameType.LiteralLocalName:
				meth = XmlILMethods.EndElemLocNameUn;
				break;
			case GenerateNameType.LiteralName:
				meth = XmlILMethods.EndElemLitNameUn;
				break;
			}
		}
		Call(meth);
	}

	public void CallStartElementContent()
	{
		LoadQueryOutput();
		Call(XmlILMethods.StartContentUn);
	}

	public void CallWriteStartAttribute(GenerateNameType nameType, bool callChk)
	{
		MethodInfo meth = null;
		if (callChk)
		{
			switch (nameType)
			{
			case GenerateNameType.LiteralLocalName:
				meth = XmlILMethods.StartAttrLocName;
				break;
			case GenerateNameType.LiteralName:
				meth = XmlILMethods.StartAttrLitName;
				break;
			case GenerateNameType.CopiedName:
				meth = XmlILMethods.StartAttrCopyName;
				break;
			case GenerateNameType.TagNameAndMappings:
				meth = XmlILMethods.StartAttrMapName;
				break;
			case GenerateNameType.TagNameAndNamespace:
				meth = XmlILMethods.StartAttrNmspName;
				break;
			case GenerateNameType.QName:
				meth = XmlILMethods.StartAttrQName;
				break;
			}
		}
		else
		{
			switch (nameType)
			{
			case GenerateNameType.LiteralLocalName:
				meth = XmlILMethods.StartAttrLocNameUn;
				break;
			case GenerateNameType.LiteralName:
				meth = XmlILMethods.StartAttrLitNameUn;
				break;
			}
		}
		Call(meth);
	}

	public void CallWriteEndAttribute(bool callChk)
	{
		LoadQueryOutput();
		if (callChk)
		{
			Call(XmlILMethods.EndAttr);
		}
		else
		{
			Call(XmlILMethods.EndAttrUn);
		}
	}

	public void CallWriteNamespaceDecl(bool callChk)
	{
		if (callChk)
		{
			Call(XmlILMethods.NamespaceDecl);
		}
		else
		{
			Call(XmlILMethods.NamespaceDeclUn);
		}
	}

	public void CallWriteString(bool disableOutputEscaping, bool callChk)
	{
		if (callChk)
		{
			if (disableOutputEscaping)
			{
				Call(XmlILMethods.NoEntText);
			}
			else
			{
				Call(XmlILMethods.Text);
			}
		}
		else if (disableOutputEscaping)
		{
			Call(XmlILMethods.NoEntTextUn);
		}
		else
		{
			Call(XmlILMethods.TextUn);
		}
	}

	public void CallWriteStartPI()
	{
		Call(XmlILMethods.StartPI);
	}

	public void CallWriteEndPI()
	{
		LoadQueryOutput();
		Call(XmlILMethods.EndPI);
	}

	public void CallWriteStartComment()
	{
		LoadQueryOutput();
		Call(XmlILMethods.StartComment);
	}

	public void CallWriteEndComment()
	{
		LoadQueryOutput();
		Call(XmlILMethods.EndComment);
	}

	public void CallCacheCount(Type itemStorageType)
	{
		XmlILStorageMethods xmlILStorageMethods = XmlILMethods.StorageMethods[itemStorageType];
		Call(xmlILStorageMethods.IListCount);
	}

	public void CallCacheItem(Type itemStorageType)
	{
		Call(XmlILMethods.StorageMethods[itemStorageType].IListItem);
	}

	public void CallValueAs(Type clrType)
	{
		MethodInfo valueAs = XmlILMethods.StorageMethods[clrType].ValueAs;
		if (valueAs == null)
		{
			LoadType(clrType);
			Emit(OpCodes.Ldnull);
			Call(XmlILMethods.ValueAsAny);
			TreatAs(typeof(object), clrType);
		}
		else
		{
			Call(valueAs);
		}
	}

	public void AddSortKey(XmlQueryType keyType)
	{
		MethodInfo meth = null;
		if (keyType == null)
		{
			meth = XmlILMethods.SortKeyEmpty;
		}
		else
		{
			switch (keyType.TypeCode)
			{
			case XmlTypeCode.String:
				meth = XmlILMethods.SortKeyString;
				break;
			case XmlTypeCode.Decimal:
				meth = XmlILMethods.SortKeyDecimal;
				break;
			case XmlTypeCode.Integer:
				meth = XmlILMethods.SortKeyInteger;
				break;
			case XmlTypeCode.Int:
				meth = XmlILMethods.SortKeyInt;
				break;
			case XmlTypeCode.Boolean:
				meth = XmlILMethods.SortKeyInt;
				break;
			case XmlTypeCode.Double:
				meth = XmlILMethods.SortKeyDouble;
				break;
			case XmlTypeCode.DateTime:
				meth = XmlILMethods.SortKeyDateTime;
				break;
			case XmlTypeCode.None:
				Emit(OpCodes.Pop);
				meth = XmlILMethods.SortKeyEmpty;
				break;
			case XmlTypeCode.AnyAtomicType:
				return;
			}
		}
		Call(meth);
	}

	public void DebugStartScope()
	{
		ilgen.BeginScope();
	}

	public void DebugEndScope()
	{
		ilgen.EndScope();
	}

	public void DebugSequencePoint(ISourceLineInfo sourceInfo)
	{
		Emit(OpCodes.Nop);
		MarkSequencePoint(sourceInfo);
	}

	private string GetFileName(ISourceLineInfo sourceInfo)
	{
		string uri = sourceInfo.Uri;
		if ((object)uri == lastUriString)
		{
			return lastFileName;
		}
		lastUriString = uri;
		lastFileName = SourceLineInfo.GetFileName(uri);
		return lastFileName;
	}

	private void MarkSequencePoint(ISourceLineInfo sourceInfo)
	{
		if (!sourceInfo.IsNoSource || lastSourceInfo == null || !lastSourceInfo.IsNoSource)
		{
			string fileName = GetFileName(sourceInfo);
			ISymbolDocumentWriter document = module.AddSourceDocument(fileName);
			ilgen.MarkSequencePoint(document, sourceInfo.Start.Line, sourceInfo.Start.Pos, sourceInfo.End.Line, sourceInfo.End.Pos);
			lastSourceInfo = sourceInfo;
		}
	}

	public Label DefineLabel()
	{
		return ilgen.DefineLabel();
	}

	public void MarkLabel(Label lbl)
	{
		if (lastSourceInfo != null && !lastSourceInfo.IsNoSource)
		{
			DebugSequencePoint(SourceLineInfo.NoSource);
		}
		ilgen.MarkLabel(lbl);
	}

	public void Emit(OpCode opcode)
	{
		ilgen.Emit(opcode);
	}

	public void Emit(OpCode opcode, byte byteVal)
	{
		ilgen.Emit(opcode, byteVal);
	}

	public void Emit(OpCode opcode, ConstructorInfo constrInfo)
	{
		ilgen.Emit(opcode, constrInfo);
	}

	public void Emit(OpCode opcode, double dblVal)
	{
		ilgen.Emit(opcode, dblVal);
	}

	public void Emit(OpCode opcode, float fltVal)
	{
		ilgen.Emit(opcode, fltVal);
	}

	public void Emit(OpCode opcode, FieldInfo fldInfo)
	{
		ilgen.Emit(opcode, fldInfo);
	}

	public void Emit(OpCode opcode, short shrtVal)
	{
		ilgen.Emit(opcode, shrtVal);
	}

	public void Emit(OpCode opcode, int intVal)
	{
		ilgen.Emit(opcode, intVal);
	}

	public void Emit(OpCode opcode, long longVal)
	{
		ilgen.Emit(opcode, longVal);
	}

	public void Emit(OpCode opcode, Label lblVal)
	{
		ilgen.Emit(opcode, lblVal);
	}

	public void Emit(OpCode opcode, Label[] arrLabels)
	{
		ilgen.Emit(opcode, arrLabels);
	}

	public void Emit(OpCode opcode, LocalBuilder locBldr)
	{
		ilgen.Emit(opcode, locBldr);
	}

	public void Emit(OpCode opcode, MethodInfo methInfo)
	{
		ilgen.Emit(opcode, methInfo);
	}

	public void Emit(OpCode opcode, sbyte sbyteVal)
	{
		ilgen.Emit(opcode, sbyteVal);
	}

	public void Emit(OpCode opcode, string strVal)
	{
		ilgen.Emit(opcode, strVal);
	}

	public void Emit(OpCode opcode, Type typVal)
	{
		ilgen.Emit(opcode, typVal);
	}

	public void EmitUnconditionalBranch(OpCode opcode, Label lblTarget)
	{
		if (!opcode.Equals(OpCodes.Br) && !opcode.Equals(OpCodes.Br_S))
		{
			Emit(OpCodes.Ldc_I4_1);
		}
		ilgen.Emit(opcode, lblTarget);
		if (lastSourceInfo != null && (opcode.Equals(OpCodes.Br) || opcode.Equals(OpCodes.Br_S)))
		{
			MarkSequencePoint(SourceLineInfo.NoSource);
		}
	}
}
